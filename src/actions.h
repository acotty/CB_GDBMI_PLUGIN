/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#ifndef _DEBUGGER_GDB_MI_ACTIONS_H_
#define _DEBUGGER_GDB_MI_ACTIONS_H_

// System and library includes
#include <tr1/memory>
#include <tr1/unordered_map>

// GDB includes
#include "cmd_queue.h"
#include "definitions.h"
#include "gdb_logger.h"

class cbDebuggerPlugin;

namespace dbg_mi
{

class SimpleAction : public Action
{
    public:
        SimpleAction(wxString const & cmd) :
            m_command(cmd)
        {
        }

        virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & /*result*/)
        {
            Finish();
        }
    protected:
        virtual void OnStart() {
            Execute(m_command);
        }
    private:
        wxString m_command;
};

class BarrierAction : public Action
{
    public:
        BarrierAction() {
            SetWaitPrevious(true);
        }
        virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & /*result*/) {}
    protected:
        virtual void OnStart()
        {
            Finish();
        }
};

class Breakpoint;

class BreakpointAddAction : public Action
{
    public:
        BreakpointAddAction(cb::shared_ptr<Breakpoint> const & breakpoint, LogPaneLogger * logger) :
            m_breakpoint(breakpoint),
            m_logger(logger)
        {

        }

        virtual ~BreakpointAddAction()
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::destructor"), LogPaneLogger::LineType::Info);
        }
        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();

    private:
        cb::shared_ptr<Breakpoint> m_breakpoint;
        CommandID m_initial_cmd, m_disable_cmd;

        LogPaneLogger * m_logger;
};

template<typename StopNotification>
class RunAction : public Action
{
    public:
        RunAction(cbDebuggerPlugin * plugin, const wxString & command,
                  StopNotification notification, LogPaneLogger * logger) :
            m_plugin(plugin),
            m_command(command),
            m_notification(notification),
            m_logger(logger)
        {
            SetWaitPrevious(true);
        }
        virtual ~RunAction()
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("RunAction::destructor"), LogPaneLogger::LineType::Debug);
        }

        virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & result)
        {
            if (result.GetResultClass() == ResultParser::ClassRunning)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("RunAction success, the debugger is !stopped!"), LogPaneLogger::LineType::Debug);
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("RunAction::Output - " + result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                m_notification(false);
            }

            Finish();
        }
    protected:
        virtual void OnStart()
        {
            Execute(m_command);
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("RunAction::OnStart -> " + m_command), LogPaneLogger::LineType::Debug);
        }

    private:
        cbDebuggerPlugin * m_plugin;
        wxString m_command;
        StopNotification m_notification;
        LogPaneLogger * m_logger;
};

struct SwitchToFrameInvoker
{
    virtual ~SwitchToFrameInvoker() {}

    virtual void Invoke(int frame_number) = 0;
};

class GenerateBacktrace : public Action
{
        GenerateBacktrace(GenerateBacktrace &);
        GenerateBacktrace & operator =(GenerateBacktrace &);
    public:
        GenerateBacktrace(SwitchToFrameInvoker * switch_to_frame, BacktraceContainer & backtrace,
                          CurrentFrame & current_frame, LogPaneLogger * logger);
        virtual ~GenerateBacktrace();
        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();
    private:
        SwitchToFrameInvoker * m_switch_to_frame;
        CommandID m_backtrace_id, m_args_id, m_frame_info_id;
        BacktraceContainer & m_backtrace;
        LogPaneLogger * m_logger;
        CurrentFrame & m_current_frame;
        int m_first_valid, m_old_active_frame;
        bool m_parsed_backtrace, m_parsed_args, m_parsed_frame_info;
};

class GenerateThreadsList : public Action
{
    public:
        GenerateThreadsList(ThreadsContainer & threads, int current_thread_id, LogPaneLogger * logger);
        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();
    private:
        ThreadsContainer & m_threads;
        LogPaneLogger * m_logger;
        int m_current_thread_id;
};

class GenerateCPUInfoRegisters : public Action
{
    public:
        GenerateCPUInfoRegisters( wxString disassemblyFlavor, LogPaneLogger * logger);
        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();
    private:
        wxString m_disassemblyFlavor;
        LogPaneLogger * m_logger;
};

class GenerateExamineMemory : public Action
{
    public:
        GenerateExamineMemory(LogPaneLogger * logger);
        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();
    private:
        wxString m_address;
        int m_length;
        LogPaneLogger * m_logger;
};

template<typename Notification>
class SwitchToThread : public Action
{
    public:
        SwitchToThread(int thread_id, LogPaneLogger * logger, Notification const & notification) :
            m_thread_id(thread_id),
            m_logger(logger),
            m_notification(notification)
        {
        }

        virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & result)
        {
            m_notification(result);
            Finish();
        }
    protected:
        virtual void OnStart()
        {
            Execute(wxString::Format("-thread-select %d", m_thread_id));
        }

    private:
        int m_thread_id;
        LogPaneLogger * m_logger;
        Notification m_notification;
};

template<typename Notification>
class SwitchToFrame : public Action
{
    public:
        SwitchToFrame(int frame_id, Notification const & notification, bool user_action) :
            m_frame_id(frame_id),
            m_notification(notification),
            m_user_action(user_action)
        {
        }

        virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & result)
        {
            m_notification(result, m_frame_id, m_user_action);
            Finish();
        }
    protected:
        virtual void OnStart()
        {
            Execute(wxString::Format("-stack-select-frame %d", m_frame_id));
        }
    private:
        int m_frame_id;
        Notification m_notification;
        bool m_user_action;
};

class WatchBaseAction : public Action
{
    public:
        WatchBaseAction(WatchesContainer & watches, LogPaneLogger * logger);
        virtual ~WatchBaseAction();

    protected:
        void ExecuteListCommand(cb::shared_ptr<Watch> watch, cb::shared_ptr<Watch> parent = cb::shared_ptr<Watch>());
        void ExecuteListCommand(wxString const & watch_id, cb::shared_ptr<Watch> parent);
        bool ParseListCommand(CommandID const & id, ResultValue const & value);

    protected:
        typedef std::tr1::unordered_map<CommandID, cb::shared_ptr<Watch> > ListCommandParentMap;
    protected:
        ListCommandParentMap m_parent_map;
        WatchesContainer & m_watches;
        LogPaneLogger * m_logger;
        int m_sub_commands_left;
};

class WatchCreateAction : public WatchBaseAction
{
        enum Step
        {
            StepCreate = 0,
            StepListChildren,
            StepSetRange
        };
    public:
        WatchCreateAction(cb::shared_ptr<Watch> const & watch, WatchesContainer & watches, LogPaneLogger * logger);

        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();

    protected:
        cb::shared_ptr<Watch> m_watch;
        Step m_step;
};

class WatchCreateTooltipAction : public WatchCreateAction
{
    public:
        WatchCreateTooltipAction(cb::shared_ptr<Watch> const & watch, WatchesContainer & watches,
                                 LogPaneLogger * logger, wxRect const & rect) :
            WatchCreateAction(watch, watches, logger),
            m_rect(rect)
        {
        }
        virtual ~WatchCreateTooltipAction();
    private:
        wxRect m_rect;
};

class WatchesUpdateAction : public WatchBaseAction
{
    public:
        WatchesUpdateAction(WatchesContainer & watches, LogPaneLogger * logger);

        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();

    private:
        bool ParseUpdate(ResultParser const & result);
    private:
        CommandID   m_update_command;
};

class WatchExpandedAction : public WatchBaseAction
{
    public:
        WatchExpandedAction(cb::shared_ptr<Watch> parent_watch, cb::shared_ptr<Watch> expanded_watch,
                            WatchesContainer & watches, LogPaneLogger * logger) :
            WatchBaseAction(watches, logger),
            m_watch(parent_watch),
            m_expanded_watch(expanded_watch)
        {
        }

        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();

    private:
        CommandID m_update_id;
        cb::shared_ptr<Watch> m_watch;
        cb::shared_ptr<Watch> m_expanded_watch;
};

class WatchCollapseAction : public WatchBaseAction
{
    public:
        WatchCollapseAction(cb::shared_ptr<Watch> parent_watch, cb::shared_ptr<Watch> collapsed_watch,
                            WatchesContainer & watches, LogPaneLogger * logger) :
            WatchBaseAction(watches, logger),
            m_watch(parent_watch),
            m_collapsed_watch(collapsed_watch)
        {
        }

        virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
    protected:
        virtual void OnStart();

    private:
        cb::shared_ptr<Watch> m_watch;
        cb::shared_ptr<Watch> m_collapsed_watch;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_ACTIONS_H_
