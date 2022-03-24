/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#ifndef _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
#define _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_

#include "cmd_queue.h"

class cbDebuggerPlugin;
class PipedProcess;
class wxEvtHandler;

namespace dbg_mi
{

class GDBExecutor : public CommandExecutor
{
        GDBExecutor(GDBExecutor & o);
        GDBExecutor & operator =(GDBExecutor & o);
    public:
        GDBExecutor();
        ~GDBExecutor();

        int LaunchProcess(wxString const & cmd, wxString const & cwd, int id_gdb_process, wxEvtHandler * event_handler, LogPaneLogger * logger);

        bool ProcessHasInput();
        bool IsRunning() const;
        bool IsStopped() const {
            return m_stopped;
        }
        bool Interupting() const {
            return m_interupting;
        }
        bool IsTemporaryInterupt() const {
            return m_temporary_interupt;
        }

        void Stopped(bool flag);
        void Interupt(bool temporary = true);
        void ForceStop();

        virtual wxString GetOutput();

        void SetAttachedPID(long pid) {
            m_attached_pid = pid;
        }
        long GetAttachedPID() const {
            return m_attached_pid;
        }

        void SetChildPID(long pid) {
            m_child_pid = pid;
        }
        bool HasChildPID() const {
            return m_child_pid >= 0;
        }

    protected:
        virtual bool DoExecute(dbg_mi::CommandID const & id, wxString const & cmd);
        virtual void DoClear();
    private:
        long GetChildPID();
    private:
        PipedProcess * m_process;
        long m_pid, m_child_pid, m_attached_pid;

        bool m_stopped;
        bool m_interupting;
        bool m_temporary_interupt;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
