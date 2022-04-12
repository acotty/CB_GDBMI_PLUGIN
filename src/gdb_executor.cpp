/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "gdb_executor.h"

#include <cassert>

#include <sdk.h>
#include <cbplugin.h>
#include <loggers.h>
#include <logmanager.h>
#include <manager.h>
#include <debuggermanager.h>
#include <pipedprocess.h>

#include "helpers.h"

namespace
{
    // function pointer to DebugBreakProcess under windows (XP+)
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0501)
#include "Tlhelp32.h"
    typedef BOOL WINAPI(*DebugBreakProcessApiCall)(HANDLE);
    typedef HANDLE WINAPI(*CreateToolhelp32SnapshotApiCall)(DWORD  dwFlags,   DWORD             th32ProcessID);
    typedef BOOL WINAPI(*Process32FirstApiCall)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
    typedef BOOL WINAPI(*Process32NextApiCall)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);

    DebugBreakProcessApiCall        DebugBreakProcessFunc = 0;
    CreateToolhelp32SnapshotApiCall CreateToolhelp32SnapshotFunc = 0;
    Process32FirstApiCall           Process32FirstFunc = 0;
    Process32NextApiCall            Process32NextFunc = 0;

    HINSTANCE kernelLib = 0;


    void InitDebuggingFuncs()
    {
        // get a function pointer to DebugBreakProcess under windows (XP+)
        kernelLib = LoadLibrary(TEXT("kernel32.dll"));

        if (kernelLib)
        {
            DebugBreakProcessFunc = (DebugBreakProcessApiCall)GetProcAddress(kernelLib, "DebugBreakProcess");
            //Windows XP
            CreateToolhelp32SnapshotFunc = (CreateToolhelp32SnapshotApiCall)GetProcAddress(kernelLib, "CreateToolhelp32Snapshot");
            Process32FirstFunc = (Process32FirstApiCall)GetProcAddress(kernelLib, "Process32First");
            Process32NextFunc = (Process32NextApiCall)GetProcAddress(kernelLib, "Process32Next");
        }
    }

    void FreeDebuggingFuncs()
    {
        if (kernelLib)
        {
            FreeLibrary(kernelLib);
        }
    }
#else
    void InitDebuggingFuncs()
    {
    }
    void FreeDebuggingFuncs()
    {
    }
#endif

    void InteruptChild(int child_pid)
    {
#ifndef __WXMSW__
        wxKillError error;
        wxKill(child_pid, wxSIGINT, &error);
#else

        if (DebugBreakProcessFunc)
        {
            HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)child_pid);

            if (proc)
            {
                DebugBreakProcessFunc(proc); // yay!
                CloseHandle(proc);
            }
        }

#endif
    }

    void GetChildPIDs(int parent, std::vector<int> & childs)
    {
#ifndef __WXMSW__
        const char * c_proc_base = "/proc";
        DIR * dir = opendir(c_proc_base);

        if (!dir)
        {
            return;
        }

        struct dirent * entry;

        do
        {
            entry = readdir(dir);

            if (entry)
            {
                int pid = atoi(entry->d_name);

                if (pid != 0)
                {
                    char filestr[PATH_MAX + 1];
                    snprintf(filestr, PATH_MAX, "%s/%d/stat", c_proc_base, pid);
                    FILE * file = fopen(filestr, "r");

                    if (file)
                    {
                        char line[101];
                        fgets(line, 100, file);
                        fclose(file);
                        int ppid = dbg_mi::ParseParentPID(line);

                        if (ppid == parent)
                        {
                            childs.push_back(pid);
                        }
                    }
                }
            }
        } while (entry);

        closedir(dir);
#else

        if ((CreateToolhelp32SnapshotFunc != NULL) && (Process32FirstFunc != NULL) && (Process32NextFunc != NULL))
        {
            HANDLE snap = CreateToolhelp32SnapshotFunc(TH32CS_SNAPALL, 0);

            if (snap != INVALID_HANDLE_VALUE)
            {
                PROCESSENTRY32 lppe;
                lppe.dwSize = sizeof(PROCESSENTRY32);
                BOOL ok = Process32FirstFunc(snap, &lppe);

                while (ok == TRUE)
                {
                    if (static_cast<int>(lppe.th32ParentProcessID) == parent) // Have my Child...
                    {
                        childs.push_back(lppe.th32ProcessID);
                    }

                    lppe.dwSize = sizeof(PROCESSENTRY32);
                    ok = Process32NextFunc(snap, &lppe);
                }

                CloseHandle(snap);
            }
        }

#endif
    }
}

namespace dbg_mi
{

    GDBExecutor::GDBExecutor() :
        m_process(NULL),
        m_pid(-1),
        m_child_pid(-1),
        m_attached_pid(-1),
        m_stopped(true),
        m_interupting(false),
        m_temporary_interupt(false)
    {
        InitDebuggingFuncs();
    }

    GDBExecutor::~GDBExecutor()
    {
        FreeDebuggingFuncs();
    }

    int GDBExecutor::LaunchProcess(wxString const & cmd, wxString const & cwd, int id_gdb_process,
                                   wxEvtHandler * event_handler, LogPaneLogger * logger)
    {
        if (m_process)
        {
            return -1;
        }

        // start the gdb process
        m_process = new PipedProcess(&m_process, event_handler, id_gdb_process, true, cwd);
        m_pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, m_process);
        m_child_pid = -1;
#ifdef __WXMAC__

        if (m_Pid == -1)
        {
            // Great! We got a fake PID. Time to Go Fish with our "ps" rod:
            m_Pid = 0;
            pid_t mypid = getpid();
            wxString mypidStr;
            mypidStr << mypid;
            long pspid = 0;
            wxString psCmd;
            wxArrayString psOutput;
            wxArrayString psErrors;
            psCmd << wxT("/bin/ps -o ppid,pid,command");
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Executing: %s"), psCmd), LogPaneLogger::LineType::Debug);
            int result = wxExecute(psCmd, psOutput, psErrors, wxEXEC_SYNC);
            mypidStr << wxT(" ");

            for (int i = 0; i < psOutput.GetCount(); ++i)
            {
                //  PPID   PID COMMAND
                wxString psLine = psOutput.Item(i);

                if (psLine.StartsWith(mypidStr) && psLine.Contains(wxT("gdb")))
                {
                    wxString pidStr = psLine.Mid(mypidStr.Length());
                    pidStr = pidStr.BeforeFirst(' ');

                    if (pidStr.ToLong(&pspid))
                    {
                        m_Pid = pspid;
                        break;
                    }
                }
            }

            for (int i = 0; i < psErrors.GetCount(); ++i)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("PS Error: %s"), psErrors.Item(i)), LogPaneLogger::LineType::Debug);
            }
        }

#endif

        if (!m_pid)
        {
            delete m_process;
            m_process = 0;
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Failed to start debugger! (%s)"), cmd), LogPaneLogger::LineType::Error);
            return -1;
        }
        else
            if (!m_process->GetOutputStream())
            {
                delete m_process;
                m_process = 0;
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Failed to start debugger! (could not get debugger's stdin) (%s)"), cmd), LogPaneLogger::LineType::Error);
                return -2;
            }
            else
                if (!m_process->GetInputStream())
                {
                    delete m_process;
                    m_process = 0;
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Failed to start debugger! (could not get debugger's stdout) (%s)"), cmd), LogPaneLogger::LineType::Error);
                    return -2;
                }
                else
                    if (!m_process->GetErrorStream())
                    {
                        delete m_process;
                        m_process = 0;
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Failed to start debugger! (could not get debugger's stderr) (%s)"), cmd), LogPaneLogger::LineType::Error);
                        return -2;
                    }

        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Started debugger: %s"), cmd), LogPaneLogger::LineType::UserDisplay);
        return 0;
    }

    long GDBExecutor::GetChildPID()
    {
        if (m_pid <= 0)
        {
            m_child_pid = -1;
        }
        else
            if (m_child_pid <= 0)
            {
                std::vector<int> children;
                GetChildPIDs(m_pid, children);

                if (children.size() != 0)
                {
                    if (children.size() > 1)
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("the debugger has more that one child."), LogPaneLogger::LineType::UserDisplay);
                    }

                    m_child_pid = children.front();
                }
            }

        return m_child_pid;
    }


    bool GDBExecutor::ProcessHasInput()
    {
        return m_process && m_process->HasInput();
    }

    bool GDBExecutor::IsRunning() const
    {
        return m_process;
    }

    void GDBExecutor::Stopped(bool flag)
    {
        if (m_logger)
        {
            if (flag)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("Executor stopped"), LogPaneLogger::LineType::Debug);
            }
            else
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("Executor started"), LogPaneLogger::LineType::Debug);
            }
        }

        m_stopped = flag;

        if (flag)
        {
            m_interupting = false;
        }
        else
        {
            m_temporary_interupt = false;
        }
    }

    void GDBExecutor::Interupt(bool temporary)
    {
        if (!IsRunning() || IsStopped())
        {
            return;
        }

        if (m_logger)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("Interupting debugger"), LogPaneLogger::LineType::Debug);
        }

        // FIXME (obfuscated#): do something similar for the windows platform
        // non-windows gdb can interrupt the running process. yay!
        if (m_pid <= 0) // look out for the "fake" PIDs (killall)
        {
            cbMessageBox(_("Unable to stop the debug process!"), _("Error"), wxOK | wxICON_WARNING);
            return;
        }
        else
        {
            m_temporary_interupt = temporary;
            m_interupting = true;

            if (m_attached_pid > 0)
            {
                InteruptChild(m_attached_pid);
            }
            else
            {
                GetChildPID();

                if (m_child_pid > 0)
                {
                    InteruptChild(m_child_pid);
                }
            }

            return;
        }
    }

    void GDBExecutor::ForceStop()
    {
        if (!IsRunning())
        {
            return;
        }

        // FIXME (obfuscated#): do something similar for the windows platform
        // non-windows gdb can interrupt the running process. yay!
        if (m_pid <= 0) // look out for the "fake" PIDs (killall)
        {
            cbMessageBox(_("Unable to stop the debug process!"), _("Error"), wxOK | wxICON_WARNING);
            return;
        }
        else
        {
            Interupt(false);

            if (m_attached_pid > 0)
            {
                Execute("kill");
            }

            Execute("-gdb-exit");
            m_process->CloseOutput();
            wxYieldIfNeeded();
            return;
        }
    }

    wxString GDBExecutor::GetOutput()
    {
        assert(false);
        return wxEmptyString;
    }

    bool GDBExecutor::DoExecute(dbg_mi::CommandID const & id, wxString const & cmd)
    {
        if (!m_process)
        {
            if (m_logger)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("m_process: false, so aborting SendString: %s%s"), id.ToString(), cmd), LogPaneLogger::LineType::Error);
            }
            return false;
        }

        if (m_logger)
        {
            if (m_stopped)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("SendString: %s%s"), id.ToString(), cmd), LogPaneLogger::LineType::Transmit);
            }
            else
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("GDBExecutor is not stopped!")), LogPaneLogger::LineType::Warning);
            }
        }

        m_process->SendString(id.ToString() + cmd);
        return true;
    }
    void GDBExecutor::DoClear()
    {
        m_stopped = true;
        delete m_process;
        m_process = NULL;
    }

} // namespace dbg_mi
