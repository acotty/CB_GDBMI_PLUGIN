#include <algorithm>
#include <wx/xrc/xmlres.h>
#include <wx/wxscintilla.h>

#include "sdk.h" // Code::Blocks SDK

#include "plugin.h"
#include "cbdebugger_interfaces.h"
#include "cbproject.h"
#include "compilercommandgenerator.h"
#include "compilerfactory.h"
#include "configurationpanel.h"
#include "configmanager.h"
//#include "editbreakpointdlg.h"
#include "infowindow.h"
#include "macrosmanager.h"
#include "pipedprocess.h"
#include "projectmanager.h"

#include "actions.h"
#include "cmd_result_parser.h"
//#include "config.h"
#include "escape.h"
#include "frame.h"
#include "debuggeroptionsdlg.h"

#ifndef __WX_MSW__
    #include <dirent.h>
    #include <stdlib.h>
#endif


namespace
{
    int const id_gdb_process = wxNewId();
    int const id_gdb_poll_timer = wxNewId();
    int const id_menu_info_command_stream = wxNewId();

    // Register the plugin with Code::Blocks.
    // We are using an anonymous namespace so we don't litter the global one.
    // this auto-registers the plugin
    PluginRegistrant<Debugger_GDB_MI> reg("debugger_gdbmi");
}


namespace
{
wxString GetLibraryPath(const wxString & oldLibPath, Compiler * compiler, ProjectBuildTarget * target, cbProject * project)
{
    if (compiler && target)
    {
        wxString newLibPath;
        const wxString libPathSep = platform::windows ? ";" : ":";
        newLibPath << "." << libPathSep;
        CompilerCommandGenerator * generator = compiler->GetCommandGenerator(project);
        newLibPath << GetStringFromArray(generator->GetLinkerSearchDirs(target), libPathSep);
        delete generator;

        if (newLibPath.Mid(newLibPath.Length() - 1, 1) != libPathSep)
        {
            newLibPath << libPathSep;
        }

        newLibPath << oldLibPath;
        return newLibPath;
    }
    else
    {
        return oldLibPath;
    }
}

} // anonymous namespace

// events handling
BEGIN_EVENT_TABLE(Debugger_GDB_MI, cbDebuggerPlugin)

    EVT_PIPEDPROCESS_STDOUT(id_gdb_process, Debugger_GDB_MI::OnGDBOutput)
    EVT_PIPEDPROCESS_STDERR(id_gdb_process, Debugger_GDB_MI::OnGDBOutput)
    EVT_PIPEDPROCESS_TERMINATED(id_gdb_process, Debugger_GDB_MI::OnGDBTerminated)

    EVT_IDLE(Debugger_GDB_MI::OnIdle)
    EVT_TIMER(id_gdb_poll_timer, Debugger_GDB_MI::OnTimer)

    EVT_MENU(id_menu_info_command_stream, Debugger_GDB_MI::OnMenuInfoCommandStream)
END_EVENT_TABLE()

// constructor
Debugger_GDB_MI::Debugger_GDB_MI() :
    cbDebuggerPlugin("GDB/MI", "gdbmi_debugger"),
    m_project(nullptr),
    m_execution_logger(this),
    m_command_stream_dialog(nullptr),
    m_console_pid(-1),
    m_pid_attached(0)
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if (!Manager::LoadResource("debugger_gdbmi.zip"))
    {
        NotifyMissingFile("debugger_gdbmi.zip");
    }

    m_executor.SetLogger(&m_execution_logger);
}

// destructor
Debugger_GDB_MI::~Debugger_GDB_MI()
{
}

void Debugger_GDB_MI::OnAttachReal()
{
    m_timer_poll_debugger.SetOwner(this, id_gdb_poll_timer);
    DebuggerManager & dbg_manager = *Manager::Get()->GetDebuggerManager();
    dbg_manager.RegisterDebugger(this);
}

void Debugger_GDB_MI::OnReleaseReal(bool appShutDown)
{
    // TODO: remove this when the loggers are fixed in C::B
    if (appShutDown)
    {
        m_execution_logger.MarkAsShutdowned();
    }

    Manager::Get()->GetDebuggerManager()->UnregisterDebugger(this);
    KillConsole();
    m_executor.ForceStop();

    if (m_command_stream_dialog)
    {
        m_command_stream_dialog->Destroy();
        m_command_stream_dialog = nullptr;
    }
}

void Debugger_GDB_MI::SetupToolsMenu(wxMenu & menu)
{
    menu.Append(id_menu_info_command_stream, _("Show command stream"));
}

bool Debugger_GDB_MI::SupportsFeature(cbDebuggerFeature::Flags flag)
{
    switch (flag)
    {
        case cbDebuggerFeature::Breakpoints:
        case cbDebuggerFeature::Callstack:
        case cbDebuggerFeature::CPURegisters:
        case cbDebuggerFeature::Disassembly:
        case cbDebuggerFeature::ExamineMemory:
        case cbDebuggerFeature::Threads:
        case cbDebuggerFeature::Watches:
        case cbDebuggerFeature::RunToCursor:
        case cbDebuggerFeature::SetNextStatement:
        case cbDebuggerFeature::ValueTooltips:
            return true;

        default:
            return false;
    }
}

cbDebuggerConfiguration * Debugger_GDB_MI::LoadConfig(const ConfigManagerWrapper & config)
{
    return new dbg_mi::DebuggerConfiguration(config);
}

dbg_mi::DebuggerConfiguration & Debugger_GDB_MI::GetActiveConfigEx()
{
    return static_cast<dbg_mi::DebuggerConfiguration &>(GetActiveConfig());
}

bool Debugger_GDB_MI::SelectCompiler(cbProject & project, Compiler *& compiler,
                                     ProjectBuildTarget *& target, long pid_to_attach)
{
    // select the build target to debug
    target = NULL;
    compiler = NULL;
    wxString active_build_target = project.GetActiveBuildTarget();

    if (pid_to_attach == 0)
    {
        Log(_("Selecting target: "));

        if (!project.BuildTargetValid(active_build_target, false))
        {
            int tgtIdx = project.SelectTarget();

            if (tgtIdx == -1)
            {
                Log(_("canceled"), Logger::error);
                return false;
            }

            target = project.GetBuildTarget(tgtIdx);
            active_build_target = target->GetTitle();
        }
        else
        {
            target = project.GetBuildTarget(active_build_target);
        }

        // make sure it's not a commands-only target
        if (target->GetTargetType() == ttCommandsOnly)
        {
            cbMessageBox(_("The selected target is only running pre/post build step commands\n"
                           "Can't debug such a target..."), _("Information"), wxICON_INFORMATION);
            Log(_("aborted"), Logger::error);
            return false;
        }

        Log(target->GetTitle());
        // find the target's compiler (to see which debugger to use)
        compiler = CompilerFactory::GetCompiler(target ? target->GetCompilerID() : project.GetCompilerID());
    }
    else
    {
        compiler = CompilerFactory::GetDefaultCompiler();
    }

    return true;
}

void Debugger_GDB_MI::OnGDBOutput(wxCommandEvent & event)
{
    wxString const & msg = event.GetString();

    if (!msg.IsEmpty())
    {
        ParseOutput(msg);
    }
}

void Debugger_GDB_MI::OnGDBTerminated(wxCommandEvent & /*event*/)
{
    ClearActiveMarkFromAllEditors();
    Log(_T("debugger terminated!"), Logger::warning);
    m_timer_poll_debugger.Stop();
    m_actions.Clear();
    m_executor.Clear();
    // Notify debugger plugins for end of debug session
    PluginManager * plm = Manager::Get()->GetPluginManager();
    CodeBlocksEvent evt(cbEVT_DEBUGGER_FINISHED);
    plm->NotifyPlugins(evt);
    SwitchToPreviousLayout();
    KillConsole();
    MarkAsStopped();

    for (Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        (*it)->SetIndex(-1);
    }
}

void Debugger_GDB_MI::OnIdle(wxIdleEvent & event)
{
    if (m_executor.IsStopped() && m_executor.IsRunning())
    {
        m_actions.Run(m_executor);
    }

    if (m_executor.ProcessHasInput())
    {
        event.RequestMore();
    }
    else
    {
        event.Skip();
    }
}

void Debugger_GDB_MI::OnTimer(wxTimerEvent & /*event*/)
{
    RunQueue();
    wxWakeUpIdle();
}

void Debugger_GDB_MI::OnMenuInfoCommandStream(wxCommandEvent & /*event*/)
{
    wxString full;

    for (int ii = 0; ii < m_execution_logger.GetCommandCount(); ++ii)
    {
        full += m_execution_logger.GetCommand(ii) + "\n";
    }

    if (m_command_stream_dialog)
    {
        m_command_stream_dialog->SetText(full);
        m_command_stream_dialog->Show();
    }
    else
    {
        m_command_stream_dialog = new dbg_mi::TextInfoWindow(Manager::Get()->GetAppWindow(), wxT("Command stream"), full);
        m_command_stream_dialog->Show();
    }
}

void Debugger_GDB_MI::AddStringCommand(wxString const & command)
{
    //-    Manager::Get()->GetLogManager()->Log("Queue command: " + command, m_dbg_page_index);
    //-    dbg_mi::Command *cmd = new dbg_mi::Command();
    //-    cmd->SetString(command);
    //-    m_command_queue.AddCommand(cmd, true);
    //    m_executor.Execute(command);
    if (IsRunning())
    {
        m_actions.Add(new dbg_mi::SimpleAction(command));
    }
}

struct Notifications
{
        Notifications(Debugger_GDB_MI * plugin, dbg_mi::GDBExecutor & executor, bool simple_mode) :
            m_plugin(plugin),
            m_executor(executor),
            m_simple_mode(simple_mode)
        {
        }

        void operator()(dbg_mi::ResultParser const & parser)
        {
            dbg_mi::ResultValue const & result_value = parser.GetResultValue();
            m_plugin->DebugLog(wxString::Format(_("notification event received: %s\n"), parser.MakeDebugString()));

            if (m_simple_mode)
            {
                ParseStateInfo(result_value);
                m_plugin->UpdateWhenStopped();
            }
            else
            {
                if (parser.GetResultType() == dbg_mi::ResultParser::NotifyAsyncOutput)
                {
                    ParseNotifyAsyncOutput(parser);
                }
                else
                    if (parser.GetResultClass() == dbg_mi::ResultParser::ClassStopped)
                    {
                        dbg_mi::StoppedReason reason = dbg_mi::StoppedReason::Parse(result_value);

                        switch (reason.GetType())
                        {
                            case dbg_mi::StoppedReason::SignalReceived:
                            {
                                wxString signal_name, signal_meaning;
                                dbg_mi::Lookup(result_value, "signal-name", signal_name);

                                if (signal_name != "SIGTRAP" && signal_name != "SIGINT")
                                {
                                    dbg_mi::Lookup(result_value, "signal-meaning", signal_meaning);
                                    InfoWindow::Display(_("Signal received"),
                                                        wxString::Format(_("\nProgram received signal: %s (%s)\n\n"),
                                                                         signal_meaning.c_str(),
                                                                         signal_name.c_str()));
                                }

                                Manager::Get()->GetDebuggerManager()->ShowBacktraceDialog();
                                UpdateCursor(result_value, true);
                            }
                            break;

                            case dbg_mi::StoppedReason::ExitedNormally:
                            case dbg_mi::StoppedReason::ExitedSignalled:
                                m_executor.Execute("-gdb-exit");
                                break;

                            case dbg_mi::StoppedReason::Exited:
                            {
                                int code = -1;

                                if (!dbg_mi::Lookup(result_value, "exit-code", code))
                                {
                                    code = -1;
                                }

                                m_plugin->SetExitCode(code);
                                m_executor.Execute("-gdb-exit");
                            }
                            break;

                            default:
                                UpdateCursor(result_value, !m_executor.IsTemporaryInterupt());
                        }

                        if (!m_executor.IsTemporaryInterupt())
                        {
                            m_plugin->BringCBToFront();
                        }
                    }
            }
        }

    private:
        void UpdateCursor(dbg_mi::ResultValue const & result_value, bool parse_state_info)
        {
            if (parse_state_info)
            {
                ParseStateInfo(result_value);
            }

            m_executor.Stopped(true);
            // Notify debugger plugins for end of debug session
            PluginManager * plm = Manager::Get()->GetPluginManager();
            CodeBlocksEvent evt(cbEVT_DEBUGGER_PAUSED);
            plm->NotifyPlugins(evt);
            m_plugin->UpdateWhenStopped();
        }
        void ParseStateInfo(dbg_mi::ResultValue const & result_value)
        {
            dbg_mi::Frame frame;

            if (!frame.ParseOutput(result_value))
            {
                m_plugin->DebugLog(wxString::Format("Debugger_GDB_MI::OnGDBNotification(...) can't find/parse frame value: %s", result_value.MakeDebugString()));
            }
            else
            {
                dbg_mi::ResultValue const * thread_id_value;
                thread_id_value = result_value.GetTupleValue(m_simple_mode ? "new-thread-id" : "thread-id");

                if (thread_id_value)
                {
                    long id;

                    if (!thread_id_value->GetSimpleValue().ToLong(&id, 10))
                    {
                        m_plugin->Log(wxString::Format("Debugger_GDB_MI::OnGDBNotification "
                                                       " thread_id parsing failed (%s)",
                                                       result_value.MakeDebugString().c_str()));
                    }
                    else
                    {
                        m_plugin->GetCurrentFrame().SetThreadId(id);
                    }
                }

                if (frame.HasValidSource())
                {
                    m_plugin->GetCurrentFrame().SetPosition(frame.GetFilename(), frame.GetLine());
                    m_plugin->SyncEditor(frame.GetFilename(), frame.GetLine(), true);
                }
                else
                {
                    m_plugin->DebugLog("ParseStateInfo does not have valid source", Logger::error);
                }
            }
        }

        void ParseNotifyAsyncOutput(dbg_mi::ResultParser const & parser)
        {
            if (parser.GetAsyncNotifyType() == "thread-group-started")
            {
                int pid;
                dbg_mi::Lookup(parser.GetResultValue(), "pid", pid);
                m_plugin->Log(wxString::Format("Found child pid: %d\n", pid));
                dbg_mi::GDBExecutor & exec = m_plugin->GetGDBExecutor();

                if (!exec.HasChildPID())
                {
                    exec.SetChildPID(pid);
                }
            }
            else
            {
                m_plugin->Log(wxString::Format("Notification: %s\n", parser.GetAsyncNotifyType()));
            }
        }

    private:
        Debugger_GDB_MI * m_plugin;
        int m_page_index;
        dbg_mi::GDBExecutor & m_executor;
        bool m_simple_mode;
};

void Debugger_GDB_MI::UpdateOnFrameChanged(bool wait)
{
    if (wait)
    {
        m_actions.Add(new dbg_mi::BarrierAction);
    }

    DebuggerManager * dbg_manager = Manager::Get()->GetDebuggerManager();

    if (IsWindowReallyShown(dbg_manager->GetWatchesDialog()->GetWindow()) && !m_watches.empty())
    {
        for (dbg_mi::WatchesContainer::iterator it = m_watches.begin(); it != m_watches.end(); ++it)
        {
            if ((*it)->GetID().empty() && !(*it)->ForTooltip())
            {
                m_actions.Add(new dbg_mi::WatchCreateAction(*it, m_watches, m_execution_logger));
            }
        }

        m_actions.Add(new dbg_mi::WatchesUpdateAction(m_watches, m_execution_logger));
    }
}

void Debugger_GDB_MI::UpdateWhenStopped()
{
    DebuggerManager * dbg_manager = Manager::Get()->GetDebuggerManager();

    if (dbg_manager->UpdateBacktrace())
    {
        RequestUpdate(Backtrace);
    }

    if (dbg_manager->UpdateThreads())
    {
        RequestUpdate(Threads);
    }

    UpdateOnFrameChanged(false);
}

void Debugger_GDB_MI::RunQueue()
{
    if (m_executor.IsRunning())
    {
        Notifications notifications(this, m_executor, false);
        dbg_mi::DispatchResults(m_executor, m_actions, notifications);

        if (m_executor.IsStopped())
        {
            m_actions.Run(m_executor);
        }
    }
}

void Debugger_GDB_MI::ParseOutput(wxString const & str)
{
    if (!str.IsEmpty())
    {
        wxArrayString const & lines = GetArrayFromString(str, _T('\n'));

        for (size_t ii = 0; ii < lines.GetCount(); ++ii)
        {
            m_executor.ProcessOutput(lines[ii]);
        }

        m_actions.Run(m_executor);
    }
}

struct StopNotification
{
    StopNotification(cbDebuggerPlugin * plugin, dbg_mi::GDBExecutor & executor) :
        m_plugin(plugin),
        m_executor(executor) {
    }

    void operator()(bool stopped) {
        m_executor.Stopped(stopped);

        if (!stopped) {
            m_plugin->ClearActiveMarkFromAllEditors();
        }
    }

    cbDebuggerPlugin * m_plugin;
    dbg_mi::GDBExecutor & m_executor;
};

bool Debugger_GDB_MI::Debug(bool breakOnEntry)
{
    m_hasStartUpError = false;
    //    ShowLog(true);
    Log("start debugger");
    ProjectManager & project_manager = *Manager::Get()->GetProjectManager();
    cbProject * project = project_manager.GetActiveProject();

    if (!project)
    {
        Log("no active project", Logger::error);
        return false;
    }

    StartType start_type = breakOnEntry ? StartTypeStepInto : StartTypeRun;

    if (!EnsureBuildUpToDate(start_type))
    {
        return false;
    }

    if (!WaitingCompilerToFinish() && !m_executor.IsRunning() && !m_hasStartUpError)
    {
        return StartDebugger(project, start_type) == 0;
    }
    else
    {
        return true;
    }
}

bool Debugger_GDB_MI::CompilerFinished(bool compilerFailed, StartType startType)
{
    if (compilerFailed || startType == StartTypeUnknown)
    {
        m_temporary_breakpoints.clear();
    }
    else
    {
        ProjectManager & project_manager = *Manager::Get()->GetProjectManager();
        cbProject * project = project_manager.GetActiveProject();

        if (project)
        {
            return StartDebugger(project, startType) == 0;
        }
        else
        {
            Log("no active project", Logger::error);
        }
    }

    return false;
}

void Debugger_GDB_MI::ConvertDirectory(wxString & str, wxString base, bool relative)
{
    dbg_mi::ConvertDirectory(str, base, relative);
}

struct BreakpointMatchProject
{
    BreakpointMatchProject(cbProject * project) : project(project) {}
    bool operator()(cb::shared_ptr<dbg_mi::Breakpoint> bp) const {
        return bp->GetProject() == project;
    }
    cbProject * project;
};

void Debugger_GDB_MI::CleanupWhenProjectClosed(cbProject * project)
{
    Breakpoints::iterator it = std::remove_if(m_breakpoints.begin(), m_breakpoints.end(),
                                              BreakpointMatchProject(project));

    if (it != m_breakpoints.end())
    {
        m_breakpoints.erase(it, m_breakpoints.end());
        // FIXME (#obfuscated): Optimize this when multiple projects are closed
        //                      (during workspace close operation for exmaple).
        cbBreakpointsDlg * dlg = Manager::Get()->GetDebuggerManager()->GetBreakpointDialog();
        dlg->Reload();
    }
}

int Debugger_GDB_MI::StartDebugger(cbProject * project, StartType start_type)
{
    //    ShowLog(true);
    m_execution_logger.ClearCommand();
    Compiler * compiler;
    ProjectBuildTarget * target;
    SelectCompiler(*project, compiler, target, 0);

    if (!compiler)
    {
        Log(_T("no compiler found!"), Logger::error);
        m_hasStartUpError = true;
        return 2;
    }

    if (!target)
    {
        Log(_T("no target found!"), Logger::error);
        m_hasStartUpError = true;
        return 3;
    }

    // is gdb accessible, i.e. can we find it?
    wxString debugger = GetActiveConfigEx().GetDebuggerExecutable();
    wxString args = target->GetExecutionParameters();
    wxString debuggee, working_dir;

    if (!GetDebuggee(debuggee, working_dir, target))
    {
        m_hasStartUpError = true;
        return 6;
    }

    bool console = target->GetTargetType() == ttConsoleOnly;
    wxString oldLibPath;
    wxGetEnv(CB_LIBRARY_ENVVAR, &oldLibPath);
    wxString newLibPath = GetLibraryPath(oldLibPath, compiler, target, project);

    if (oldLibPath != newLibPath)
    {
        wxSetEnv(CB_LIBRARY_ENVVAR, newLibPath);
        DebugLog(CB_LIBRARY_ENVVAR _T("=") + newLibPath);
    }

    int res = LaunchDebugger(debugger, debuggee, args, working_dir, 0, console, start_type);

    if (res != 0)
    {
        m_hasStartUpError = true;
        return res;
    }

    m_executor.SetAttachedPID(-1);
    m_project = project;
    m_hasStartUpError = false;

    if (oldLibPath != newLibPath)
    {
        wxSetEnv(CB_LIBRARY_ENVVAR, oldLibPath);
    }

    return 0;
}

int Debugger_GDB_MI::LaunchDebugger(wxString const & debugger, wxString const & debuggee,
                                    wxString const & args, wxString const & working_dir,
                                    int pid, bool console, StartType start_type)
{
    m_current_frame.Reset();

    if (debugger.IsEmpty())
    {
        Log(_T("no debugger executable found (full path)!"), Logger::error);
        return 5;
    }

    Log(_T("GDB path: ") + debugger);

    if (pid == 0)
    {
        Log(_T("DEBUGGEE path: ") + debuggee);
    }

    wxString cmd;
    cmd << debugger;
    //    cmd << _T(" -nx");          // don't run .gdbinit
    cmd << " -fullname ";   // report full-path filenames when breaking
    cmd << " -quiet";       // don't display version on startup
    cmd << " --interpreter=mi";

    if (pid == 0)
    {
        cmd << " -args " << debuggee;
    }
    else
    {
        cmd << " -pid=" << pid;
    }

    // start the gdb process
    Log(_T("Command-line: ") + cmd);

    if (pid == 0)
    {
        Log(_T("Working dir : ") + working_dir);
    }

    int ret = m_executor.LaunchProcess(cmd, working_dir, id_gdb_process, this, m_execution_logger);

    if (ret != 0)
    {
        return ret;
    }

    m_executor.Stopped(true);
    //    m_executor.Execute(_T("-enable-timings"));
    CommitBreakpoints(true);
    CommitWatches();
    // Set program arguments
    m_actions.Add(new dbg_mi::SimpleAction("-exec-arguments " + args));

    if (console)
    {
        wxString console_tty;
        m_console_pid = RunNixConsole(console_tty);

        if (m_console_pid >= 0)
        {
            m_actions.Add(new dbg_mi::SimpleAction("-inferior-tty-set " + console_tty));
        }
    }

    dbg_mi::DebuggerConfiguration & active_config = GetActiveConfigEx();

    if (active_config.GetFlag(dbg_mi::DebuggerConfiguration::CheckPrettyPrinters))
    {
        m_actions.Add(new dbg_mi::SimpleAction("-enable-pretty-printing"));
    }

    wxArrayString comandLines = GetArrayFromString(active_config.GetInitialCommands(), '\n');
    size_t CommandLineCount = comandLines.GetCount();

    for (unsigned int i = 0; i < CommandLineCount; ++i)
    {
        DoSendCommand(comandLines[i]);
    }

    if (active_config.GetFlag(dbg_mi::DebuggerConfiguration::CatchExceptions))
    {
        DoSendCommand("catch throw");
        DoSendCommand("catch catch");
    }

    if (pid == 0)
    {
        switch (start_type)
        {
            case StartTypeRun:
                CommitRunCommand("-exec-run");
                break;

            case StartTypeStepInto:
                CommitRunCommand("-exec-step");
                break;
        }
    }

    m_actions.Run(m_executor);
    m_timer_poll_debugger.Start(20);
    SwitchToDebuggingLayout();
    m_pid_attached = pid;
    return 0;
}

void Debugger_GDB_MI::CommitBreakpoints(bool force)
{
    DebugLog("Debugger_GDB_MI::CommitBreakpoints");

    for (Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    {
        // FIXME (obfuscated#): pointers inside the vector can be dangerous!!!
        if ((*it)->GetIndex() == -1 || force)
        {
            m_actions.Add(new dbg_mi::BreakpointAddAction(*it, m_execution_logger));
        }
    }

    for (Breakpoints::const_iterator it = m_temporary_breakpoints.begin(); it != m_temporary_breakpoints.end(); ++it)
    {
        AddStringCommand(wxString::Format(_T("-break-insert -t %s:%d"), (*it)->GetLocation().c_str(),
                                          (*it)->GetLine()));
    }

    m_temporary_breakpoints.clear();
}

void Debugger_GDB_MI::CommitWatches()
{
    for (dbg_mi::WatchesContainer::iterator it = m_watches.begin(); it != m_watches.end(); ++it)
    {
        (*it)->Reset();
    }

    if (!m_watches.empty())
    {
        // Original Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->UpdateWatches();
        CodeBlocksEvent event(cbEVT_DEBUGGER_UPDATED);
        event.SetInt(int(cbDebuggerPlugin::DebugWindows::Watches));
        //event.SetPlugin(m_pDriver->GetDebugger());
        Manager::Get()->ProcessEvent(event);
    }
}

void Debugger_GDB_MI::CommitRunCommand(wxString const & command)
{
    m_current_frame.Reset();
    m_actions.Add(new dbg_mi::RunAction<StopNotification>(this, command,
                                                          StopNotification(this, m_executor),
                                                          m_execution_logger)
                 );
}

bool Debugger_GDB_MI::RunToCursor(const wxString & filename, int line, const wxString & /*line_text*/)
{
    if (IsRunning())
    {
        if (IsStopped())
        {
            CommitRunCommand(wxString::Format("-exec-until %s:%d", filename.c_str(), line));
            return true;
        }

        return false;
    }
    else
    {
        cb::shared_ptr<dbg_mi::Breakpoint> ptr(new dbg_mi::Breakpoint(filename, line, nullptr));
        m_temporary_breakpoints.push_back(ptr);
        return Debug(false);
    }
}

void Debugger_GDB_MI::SetNextStatement(const wxString & filename, int line)
{
    if (IsStopped())
    {
        AddStringCommand(wxString::Format("-break-insert -t %s:%d", filename.c_str(), line));
        CommitRunCommand(wxString::Format("-exec-jump %s:%d", filename.c_str(), line));
    }
}

void Debugger_GDB_MI::Continue()
{
    if (!IsStopped() && !m_executor.Interupting())
    {
        dbg_mi::Logger * log = m_executor.GetLogger();

        if (log)
        {
            log->Debug("Continue failed -> debugger is not interupted!");
        }

        return;
    }

    DebugLog("Debugger_GDB_MI::Continue");
    CommitRunCommand("-exec-continue");
}

void Debugger_GDB_MI::Next()
{
    CommitRunCommand("-exec-next");
}

void Debugger_GDB_MI::NextInstruction()
{
    CommitRunCommand("-exec-next-instruction");
}

void Debugger_GDB_MI::StepIntoInstruction()
{
    CommitRunCommand("-exec-step-instruction");
}

void Debugger_GDB_MI::Step()
{
    CommitRunCommand("-exec-step");
}

void Debugger_GDB_MI::StepOut()
{
    CommitRunCommand("-exec-finish");
}

void Debugger_GDB_MI::Break()
{
    m_executor.Interupt(false);
    // cbEVT_DEBUGGER_PAUSED will be sent, when the debugger has pause for real
}

void Debugger_GDB_MI::Stop()
{
    if (!IsRunning())
    {
        return;
    }

    ClearActiveMarkFromAllEditors();
    Log("stop debugger");
    m_executor.ForceStop();
    MarkAsStopped();
}

bool Debugger_GDB_MI::IsRunning() const
{
    return m_executor.IsRunning();
}

bool Debugger_GDB_MI::IsStopped() const
{
    return m_executor.IsStopped();
}

bool Debugger_GDB_MI::IsBusy() const
{
    return !m_executor.IsStopped();
}

int Debugger_GDB_MI::GetExitCode() const
{
    return m_exit_code;
}

int Debugger_GDB_MI::GetStackFrameCount() const
{
    return m_backtrace.size();
}

cb::shared_ptr<const cbStackFrame> Debugger_GDB_MI::GetStackFrame(int index) const
{
    return m_backtrace[index];
}

struct SwitchToFrameNotification
{
    SwitchToFrameNotification(Debugger_GDB_MI * plugin) :
        m_plugin(plugin) {
    }

    void operator()(dbg_mi::ResultParser const & result, int frame_number, bool user_action) {
        if (m_frame_number < m_plugin->GetStackFrameCount()) {
            dbg_mi::CurrentFrame & current_frame = m_plugin->GetCurrentFrame();

            if (user_action) {
                current_frame.SwitchToFrame(frame_number);
            }
            else {
                current_frame.Reset();
                current_frame.SetFrame(frame_number);
            }

            cb::shared_ptr<const cbStackFrame> frame = m_plugin->GetStackFrame(frame_number);
            wxString const & filename = frame->GetFilename();
            long line;

            if (frame->GetLine().ToLong(&line)) {
                current_frame.SetPosition(filename, line);
                m_plugin->SyncEditor(filename, line, true);
            }

            m_plugin->UpdateOnFrameChanged(true);
            Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
        }
    }

    Debugger_GDB_MI * m_plugin;
    int m_frame_number;
};

void Debugger_GDB_MI::SwitchToFrame(int number)
{
    m_execution_logger.Debug("Debugger_GDB_MI::SwitchToFrame");

    if (IsRunning() && IsStopped())
    {
        if (number < static_cast<int>(m_backtrace.size()))
        {
            m_execution_logger.Debug("Debugger_GDB_MI::SwitchToFrame - adding commnad");
            int frame = m_backtrace[number]->GetNumber();
            typedef dbg_mi::SwitchToFrame<SwitchToFrameNotification> SwitchType;
            m_actions.Add(new SwitchType(frame, SwitchToFrameNotification(this), true));
        }
    }
}

int Debugger_GDB_MI::GetActiveStackFrame() const
{
    return m_current_frame.GetStackFrame();
}

cb::shared_ptr<cbBreakpoint> Debugger_GDB_MI::AddBreakpoint(const wxString & filename, int line)
{
    if (IsRunning())
    {
        if (!IsStopped())
        {
            DebugLog(wxString::Format("Debugger_GDB_MI::Addbreakpoint: %s:%d",
                                      filename.c_str(), line));
            m_executor.Interupt();
            cbProject * project;
            project = Manager::Get()->GetProjectManager()->FindProjectForFile(filename, nullptr, false, false);
            cb::shared_ptr<dbg_mi::Breakpoint> ptr(new dbg_mi::Breakpoint(filename, line, project));
            m_breakpoints.push_back(ptr);
            m_actions.Add(new dbg_mi::BreakpointAddAction(ptr, m_execution_logger));
            Continue();
        }
        else
        {
            cbProject * project;
            project = Manager::Get()->GetProjectManager()->FindProjectForFile(filename, nullptr, false, false);
            cb::shared_ptr<dbg_mi::Breakpoint> ptr(new dbg_mi::Breakpoint(filename, line, project));
            m_breakpoints.push_back(ptr);
            m_actions.Add(new dbg_mi::BreakpointAddAction(ptr, m_execution_logger));
        }
    }
    else
    {
        cbProject * project = Manager::Get()->GetProjectManager()->FindProjectForFile(filename, nullptr, false, false);
        cb::shared_ptr<dbg_mi::Breakpoint> ptr(new dbg_mi::Breakpoint(filename, line, project));
        m_breakpoints.push_back(ptr);
    }

    return cb::static_pointer_cast<cbBreakpoint>(m_breakpoints.back());
}

cb::shared_ptr<cbBreakpoint> Debugger_GDB_MI::AddDataBreakpoint(const wxString & /*dataExpression*/)
{
#warning "not implemented"
    return cb::shared_ptr<cbBreakpoint>();
}

int Debugger_GDB_MI::GetBreakpointsCount() const
{
    return m_breakpoints.size();
}

cb::shared_ptr<cbBreakpoint> Debugger_GDB_MI::GetBreakpoint(int index)
{
    return cb::static_pointer_cast<cbBreakpoint>(m_breakpoints[index]);
}
cb::shared_ptr<const cbBreakpoint> Debugger_GDB_MI::GetBreakpoint(int index) const
{
    return cb::static_pointer_cast<const cbBreakpoint>(m_breakpoints[index]);
}

void Debugger_GDB_MI::UpdateBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint)
{
#warning "not implemented"
    //    for(Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    //    {
    //        dbg_mi::Breakpoint &current = **it;
    //        if(&current.Get() != breakpoint)
    //            continue;
    //
    //        switch(breakpoint->GetType())
    //        {
    //        case cbBreakpoint::Code:
    //            {
    //                cbBreakpoint temp(*breakpoint);
    //                EditBreakpointDlg dialog(&temp);
    //                PlaceWindow(&dialog);
    //                if(dialog.ShowModal() == wxID_OK)
    //                {
    //                    // if the breakpoint is not sent to debugger, just copy
    //                    if(current.GetIndex() != -1 || !IsRunning())
    //                    {
    //                        current.Get() = temp;
    //                        current.SetIndex(-1);
    //                    }
    //                    else
    //                    {
    ////                        bool resume = !m_executor.IsStopped();
    ////                        if(resume)
    ////                            m_executor.Interupt();
    //#warning "not implemented"
    //                        bool changed = false;
    //                        if(breakpoint->IsEnabled() == temp.IsEnabled())
    //                        {
    //                            if(breakpoint->IsEnabled())
    //                                AddStringCommand(wxString::Format("-break-enable %d", current.GetIndex()));
    //                            else
    //                                AddStringCommand(wxString::Format("-break-disable %d", current.GetIndex()));
    //                            changed = true;
    //                        }
    //
    //                        if(changed)
    //                        {
    //                            m_executor.Interupt();
    //                            Continue();
    //                        }
    //                    }
    //
    //                }
    //            }
    //            break;
    //        case cbBreakpoint::Data:
    //#warning "not implemented"
    //            break;
    //        }
    //    }
}

void Debugger_GDB_MI::DeleteBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint)
{
    Breakpoints::iterator it = std::find(m_breakpoints.begin(), m_breakpoints.end(), breakpoint);

    if (it != m_breakpoints.end())
    {
        DebugLog(wxString::Format("Debugger_GDB_MI::DeleteBreakpoint: %s:%d",
                                  breakpoint->GetLocation().c_str(), breakpoint->GetLine()));
        int index = (*it)->GetIndex();

        if (index != -1)
        {
            if (!IsStopped())
            {
                m_executor.Interupt();
                AddStringCommand(wxString::Format("-break-delete %d", index));
                Continue();
            }
            else
            {
                AddStringCommand(wxString::Format("-break-delete %d", index));
            }
        }

        m_breakpoints.erase(it);
    }

    //    for(Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
    //    {
    //        dbg_mi::Breakpoint &current = **it;
    //        if(&current.Get() == breakpoint)
    //        {
    //            DebugLog(wxString::Format("Debugger_GDB_MI::DeleteBreakpoint: %s:%d",
    //                                      breakpoint->GetFilename().c_str(),
    //                                      breakpoint->GetLine()));
    //            if(current.GetIndex() != -1)
    //            {
    //                if(!IsStopped())
    //                {
    //                    m_executor.Interupt();
    //                    AddStringCommand(wxString::Format("-break-delete %d", current.GetIndex()));
    //                    Continue();
    //                }
    //                else
    //                    AddStringCommand(wxString::Format("-break-delete %d", current.GetIndex()));
    //            }
    //            m_breakpoints.erase(it);
    //            return;
    //        }
    //    }
}

void Debugger_GDB_MI::DeleteAllBreakpoints()
{
    if (IsRunning())
    {
        wxString breaklist;

        for (Breakpoints::iterator it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it)
        {
            dbg_mi::Breakpoint & current = **it;

            if (current.GetIndex() != -1)
            {
                breaklist += wxString::Format(" %d", current.GetIndex());
            }
        }

        if (!breaklist.empty())
        {
            if (!IsStopped())
            {
                m_executor.Interupt();
                AddStringCommand("-break-delete" + breaklist);
                Continue();
            }
            else
            {
                AddStringCommand("-break-delete" + breaklist);
            }
        }
    }

    m_breakpoints.clear();
}

void Debugger_GDB_MI::ShiftBreakpoint(int index, int lines_to_shift)
{
    if (index < 0 || index >= static_cast<int>(m_breakpoints.size()))
    {
        return;
    }

    cb::shared_ptr<dbg_mi::Breakpoint> bp = m_breakpoints[index];
    bp->ShiftLine(lines_to_shift);

    if (IsRunning())
    {
        // just remove the breakpoints as they will become invalid
        if (!IsStopped())
        {
            m_executor.Interupt();

            if (bp->GetIndex() >= 0)
            {
                AddStringCommand(wxString::Format("-break-delete %d", bp->GetIndex()));
                bp->SetIndex(-1);
            }

            Continue();
        }
        else
        {
            if (bp->GetIndex() >= 0)
            {
                AddStringCommand(wxString::Format("-break-delete %d", bp->GetIndex()));
                bp->SetIndex(-1);
            }
        }
    }
}

void Debugger_GDB_MI::EnableBreakpoint(cb::shared_ptr<cbBreakpoint> breakpoint, bool enable)
{
#warning "not implemented"
}

int Debugger_GDB_MI::GetThreadsCount() const
{
    return m_threads.size();
}

cb::shared_ptr<const cbThread> Debugger_GDB_MI::GetThread(int index) const
{
    return m_threads[index];
}

bool Debugger_GDB_MI::SwitchToThread(int thread_number)
{
    if (IsStopped())
    {
        dbg_mi::SwitchToThread<Notifications> * a;
        a = new dbg_mi::SwitchToThread<Notifications>(thread_number, m_execution_logger,
                                                      Notifications(this, m_executor, true));
        m_actions.Add(a);
        return true;
    }
    else
    {
        return false;
    }
}

cb::shared_ptr<cbWatch> Debugger_GDB_MI::AddWatch(const wxString & symbol, cb_unused bool update)
{
    cb::shared_ptr<dbg_mi::Watch> w(new dbg_mi::Watch(symbol, false));
    m_watches.push_back(w);

    if (IsRunning())
    {
        m_actions.Add(new dbg_mi::WatchCreateAction(w, m_watches, m_execution_logger));
    }

    return w;
}

cb::shared_ptr<cbWatch> Debugger_GDB_MI::AddMemoryRange(cb_unused uint64_t address, cb_unused uint64_t size, cb_unused const wxString & symbol, cb_unused bool update)
{
#warning THis needs to be changed
    // Keep compiler happy
    cb::shared_ptr<dbg_mi::Watch> w(new dbg_mi::Watch(symbol, false));
    return w;
}


void Debugger_GDB_MI::AddTooltipWatch(const wxString & symbol, wxRect const & rect)
{
    cb::shared_ptr<dbg_mi::Watch> w(new dbg_mi::Watch(symbol, true));
    m_watches.push_back(w);

    if (IsRunning())
    {
        m_actions.Add(new dbg_mi::WatchCreateTooltipAction(w, m_watches, m_execution_logger, rect));
    }
}

void Debugger_GDB_MI::DeleteWatch(cb::shared_ptr<cbWatch> watch)
{
    cb::shared_ptr<cbWatch> root_watch = cbGetRootWatch(watch);
    dbg_mi::WatchesContainer::iterator it = std::find(m_watches.begin(), m_watches.end(), root_watch);

    if (it == m_watches.end())
    {
        return;
    }

    if (IsRunning())
    {
        if (IsStopped())
        {
            AddStringCommand("-var-delete " + (*it)->GetID());
        }
        else
        {
            m_executor.Interupt();
            AddStringCommand("-var-delete " + (*it)->GetID());
            Continue();
        }
    }

    m_watches.erase(it);
}

bool Debugger_GDB_MI::HasWatch(cb::shared_ptr<cbWatch> watch)
{
    dbg_mi::WatchesContainer::iterator it = std::find(m_watches.begin(), m_watches.end(), watch);
    return it != m_watches.end();
}

void Debugger_GDB_MI::ShowWatchProperties(cb::shared_ptr<cbWatch> /*watch*/)
{
#warning "not implemented"
}

bool Debugger_GDB_MI::SetWatchValue(cb::shared_ptr<cbWatch> watch, const wxString & value)
{
    if (!IsStopped() || !IsRunning())
    {
        return false;
    }

    cb::shared_ptr<cbWatch> root_watch = cbGetRootWatch(watch);
    dbg_mi::WatchesContainer::iterator it = std::find(m_watches.begin(), m_watches.end(), root_watch);

    if (it == m_watches.end())
    {
        return false;
    }

    cb::shared_ptr<dbg_mi::Watch> real_watch = cb::static_pointer_cast<dbg_mi::Watch>(watch);
    AddStringCommand("-var-assign " + real_watch->GetID() + " " + value);
    //    m_actions.Add(new dbg_mi::WatchSetValueAction(*it, static_cast<dbg_mi::Watch*>(watch), value, m_execution_logger));
    dbg_mi::Action * update_action = new dbg_mi::WatchesUpdateAction(m_watches, m_execution_logger);
    update_action->SetWaitPrevious(true);
    m_actions.Add(update_action);
    return true;
}

void Debugger_GDB_MI::ExpandWatch(cb::shared_ptr<cbWatch> watch)
{
    if (!IsStopped() || !IsRunning())
    {
        return;
    }

    cb::shared_ptr<cbWatch> root_watch = cbGetRootWatch(watch);
    dbg_mi::WatchesContainer::iterator it = std::find(m_watches.begin(), m_watches.end(), root_watch);

    if (it != m_watches.end())
    {
        cb::shared_ptr<dbg_mi::Watch> real_watch = cb::static_pointer_cast<dbg_mi::Watch>(watch);

        if (!real_watch->HasBeenExpanded())
        {
            m_actions.Add(new dbg_mi::WatchExpandedAction(*it, real_watch, m_watches, m_execution_logger));
        }
    }
}

void Debugger_GDB_MI::CollapseWatch(cb::shared_ptr<cbWatch> watch)
{
    if (!IsStopped() || !IsRunning())
    {
        return;
    }

    cb::shared_ptr<cbWatch> root_watch = cbGetRootWatch(watch);
    dbg_mi::WatchesContainer::iterator it = std::find(m_watches.begin(), m_watches.end(), root_watch);

    if (it != m_watches.end())
    {
        cb::shared_ptr<dbg_mi::Watch> real_watch = cb::static_pointer_cast<dbg_mi::Watch>(watch);

        if (real_watch->HasBeenExpanded() && real_watch->DeleteOnCollapse())
        {
            m_actions.Add(new dbg_mi::WatchCollapseAction(*it, real_watch, m_watches, m_execution_logger));
        }
    }
}

void Debugger_GDB_MI::UpdateWatch(cb_unused cb::shared_ptr<cbWatch> watch)
{
#warning this is a blank function
}

void Debugger_GDB_MI::SendCommand(const wxString & cmd, bool debugLog)
{
    if (!IsRunning())
    {
        wxString message("Command will not be executed because the debugger is not running!");
        Log(message);
        DebugLog(message);
        return;
    }

    if (!IsStopped())
    {
        wxString message("Command will not be executed because the debugger/debuggee is not paused/interupted!");
        Log(message);
        DebugLog(message);
        return;
    }

    if (cmd.empty())
    {
        return;
    }

    DoSendCommand(cmd);
}

void Debugger_GDB_MI::DoSendCommand(const wxString & cmd)
{
    wxString escaped_cmd = cmd;
    escaped_cmd.Replace("\n", "\\n", true);

    if (escaped_cmd[0] == '-')
    {
        AddStringCommand(escaped_cmd);
    }
    else
    {
        escaped_cmd.Replace("\\", "\\\\", true);
        AddStringCommand("-interpreter-exec console \"" + escaped_cmd + "\"");
    }
}

void Debugger_GDB_MI::AttachToProcess(const wxString & pid)
{
    m_project = NULL;
    long number;

    if (!pid.ToLong(&number))
    {
        return;
    }

    LaunchDebugger(GetActiveConfigEx().GetDebuggerExecutable(), wxEmptyString, wxEmptyString,
                   wxEmptyString, number, false, StartTypeRun);
    m_executor.SetAttachedPID(number);
}

void Debugger_GDB_MI::DetachFromProcess()
{
    AddStringCommand(wxString::Format("-target-detach %ld", m_executor.GetAttachedPID()));
}

bool Debugger_GDB_MI::IsAttachedToProcess() const
{
    return m_pid_attached != 0;
}

void Debugger_GDB_MI::RequestUpdate(DebugWindows window)
{
    if (!IsStopped())
    {
        return;
    }

    switch (window)
    {
        case Backtrace:
        {
            struct Switcher : dbg_mi::SwitchToFrameInvoker
            {
                Switcher(Debugger_GDB_MI * plugin, dbg_mi::ActionsMap & actions) :
                    m_plugin(plugin),
                    m_actions(actions) {
                }

                virtual void Invoke(int frame_number) {
                    typedef dbg_mi::SwitchToFrame<SwitchToFrameNotification> SwitchType;
                    m_actions.Add(new SwitchType(frame_number, SwitchToFrameNotification(m_plugin), false));
                }

                Debugger_GDB_MI * m_plugin;
                dbg_mi::ActionsMap & m_actions;
            };
            Switcher * switcher = new Switcher(this, m_actions);
            m_actions.Add(new dbg_mi::GenerateBacktrace(switcher, m_backtrace, m_current_frame, m_execution_logger));
        }
        break;

        case Threads:
            m_actions.Add(new dbg_mi::GenerateThreadsList(m_threads, m_current_frame.GetThreadId(), m_execution_logger));
            break;
    }
}

void Debugger_GDB_MI::GetCurrentPosition(wxString & filename, int & line)
{
    m_current_frame.GetPosition(filename, line);
}

void Debugger_GDB_MI::KillConsole()
{
    if (m_console_pid >= 0)
    {
        wxKill(m_console_pid);
        m_console_pid = -1;
    }
}

void Debugger_GDB_MI::OnValueTooltip(const wxString & token, const wxRect & evalRect)
{
    AddTooltipWatch(token, evalRect);
}

bool Debugger_GDB_MI::ShowValueTooltip(int style)
{
    if (!IsRunning() || !IsStopped())
    {
        return false;
    }

    if (style != wxSCI_C_DEFAULT && style != wxSCI_C_OPERATOR && style != wxSCI_C_IDENTIFIER && style != wxSCI_C_WORD2)
    {
        return false;
    }

    return true;
}
