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
#include <map>

// GDB includes
#include "cmd_queue.h"
#include "definitions.h"
#include "gdb_logger.h"

class cbDebuggerPlugin;

namespace dbg_mi
{

    class GDBSimpleAction : public Action
    {
        public:
            GDBSimpleAction(wxString const & cmd) :
                m_command(cmd)
            {
            }

            virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & /*result*/)
            {
                Finish();
            }
        protected:
            virtual void OnStart()
            {
                Execute(m_command);
            }
        private:
            wxString m_command;
    };

    class GDBBarrierAction : public Action
    {
        public:
            GDBBarrierAction()
            {
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

    class GDBBreakpointAddAction : public Action
    {
        public:
            GDBBreakpointAddAction(cb::shared_ptr<GDBBreakpoint> const & breakpoint, LogPaneLogger * logger);
            virtual ~GDBBreakpointAddAction();
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();

        private:
            cb::shared_ptr<GDBBreakpoint> m_breakpoint;
            CommandID m_initial_cmd, m_disable_cmd;

            LogPaneLogger * m_logger;
    };

    template<typename StopNotification>
    class GDBRunAction : public Action
    {
        public:
            GDBRunAction(cbDebuggerPlugin * plugin, const wxString & command,
                      StopNotification notification, LogPaneLogger * logger) :
                m_plugin(plugin),
                m_command(command),
                m_notification(notification),
                m_logger(logger)
            {
                SetWaitPrevious(true);
            }
            virtual ~GDBRunAction()
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GDBRunAction::destructor"), LogPaneLogger::LineType::Debug);
            }

            virtual void OnCommandOutput(CommandID const & /*id*/, ResultParser const & result)
            {
                if (result.GetResultClass() == ResultParser::ClassRunning)
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GDBRunAction success, the debugger is !stopped!"), LogPaneLogger::LineType::Debug);
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GDBRunAction::Output - " + result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                    m_notification(false);
                }

                Finish();
            }
        protected:
            virtual void OnStart()
            {
                Execute(m_command);
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GDBRunAction::OnStart -> " + m_command), LogPaneLogger::LineType::Debug);
            }

        private:
            cbDebuggerPlugin * m_plugin;
            wxString m_command;
            StopNotification m_notification;
            LogPaneLogger * m_logger;
    };

    struct GDBSwitchToFrameInvoker
    {
        virtual ~GDBSwitchToFrameInvoker() {}
        virtual void Invoke(int frame_number) = 0;
    };

    class GDBGenerateBacktrace : public Action
    {
            GDBGenerateBacktrace(GDBGenerateBacktrace &);
            GDBGenerateBacktrace & operator =(GDBGenerateBacktrace &);
        public:
            GDBGenerateBacktrace(   GDBSwitchToFrameInvoker * switch_to_frame,
                                    GDBBacktraceContainer & backtrace,
                                    GDBCurrentFrame & current_frame,
                                    LogPaneLogger * logger);
            virtual ~GDBGenerateBacktrace();
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();
        private:
            GDBSwitchToFrameInvoker * m_switch_to_frame;
            CommandID m_backtrace_id, m_args_id, m_frame_info_id;
            GDBBacktraceContainer & m_backtrace;
            LogPaneLogger * m_logger;
            GDBCurrentFrame & m_current_frame;
            int m_first_valid, m_old_active_frame;
            bool m_parsed_backtrace, m_parsed_args, m_parsed_frame_info;
    };

    class GDBGenerateThreadsList : public Action
    {
        public:
            GDBGenerateThreadsList(GDBThreadsContainer & threads, int current_thread_id, LogPaneLogger * logger);
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();
        private:
            GDBThreadsContainer & m_threads;
            LogPaneLogger * m_logger;
            int m_current_thread_id;
    };


    class GDBGenerateCPUInfoRegisters : public Action
    {
        private:
            struct RegistryData
            {
                wxString RegistryName;
                wxString RegistryValue;
            };

        public:
            GDBGenerateCPUInfoRegisters(LogPaneLogger * logger);
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();
        private:
            bool m_bParsedRegisteryNamesReceived;
            bool m_bParsedRegisteryValuesReceived;
            CommandID m_reg_name_data_list_request_id;
            CommandID m_reg_value_data_list_request_id;
            std::map<long, RegistryData> m_ParsedRegisteryDataReceived;

            LogPaneLogger * m_logger;
    };

    class GDBGenerateExamineMemory : public Action
    {
        public:
            GDBGenerateExamineMemory(LogPaneLogger * logger);
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();
        private:
            wxString m_address;
            int m_length;
            CommandID m_examine_memory_request_id;

            LogPaneLogger * m_logger;
    };

    class GDBMemoryRangeWatchCreateAction : public Action
    {
        public:
            GDBMemoryRangeWatchCreateAction(cb::shared_ptr<GDBMemoryRangeWatch> const & watch, LogPaneLogger * logger);
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);

        protected:
            virtual void OnStart();

        private:
            CommandID m_memory_range_watch_request_id;

            cb::shared_ptr<GDBMemoryRangeWatch> m_watch;

            LogPaneLogger * m_logger;
    };



    class GDBDisassemble : public Action
    {
        public:
            GDBDisassemble(wxString disassemblyFlavor, LogPaneLogger * logger);
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();
        private:
            void ParseASMInsmLine(cbDisassemblyDlg * dialog, const ResultValue * pASMLineItem, int iASMIndex);
            wxString m_disassemblyFlavor;
            CommandID m_disassemble_frame_info_request_id;
            CommandID m_disassemble_data_request_id;

            LogPaneLogger * m_logger;
    };


    template<typename Notification>
    class GDBSwitchToThread : public Action
    {
        public:
            GDBSwitchToThread(int thread_id, LogPaneLogger * logger, Notification const & notification) :
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
    class GDBSwitchToFrame : public Action
    {
        public:
            GDBSwitchToFrame(int frame_id, Notification const & notification, bool user_action) :
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

    class GDBWatchBaseAction : public Action
    {
        public:
            GDBWatchBaseAction(GDBWatchesContainer & watches, LogPaneLogger * logger);
            virtual ~GDBWatchBaseAction();

        protected:
            void ExecuteListCommand(cb::shared_ptr<GDBWatch> watch, cb::shared_ptr<GDBWatch> parent = cb::shared_ptr<GDBWatch>());
            void ExecuteListCommand(wxString const & watch_id, cb::shared_ptr<GDBWatch> parent);
            bool ParseListCommand(CommandID const & id, ResultValue const & value);

        protected:
            typedef std::tr1::unordered_map<CommandID, cb::shared_ptr<GDBWatch> > ListCommandParentMap;
        protected:
            ListCommandParentMap m_parent_map;
            GDBWatchesContainer & m_watches;
            LogPaneLogger * m_logger;
            int m_sub_commands_left;
    };

    class GDBWatchCreateAction : public GDBWatchBaseAction
    {
            enum Step
            {
                StepCreate = 0,
                StepListChildren,
                StepSetRange
            };
        public:
            GDBWatchCreateAction(cb::shared_ptr<GDBWatch> const & watch, GDBWatchesContainer & watches, LogPaneLogger * logger);

            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();

        protected:
            cb::shared_ptr<GDBWatch> m_watch;
            Step m_step;
    };

    class GDBWatchCreateTooltipAction : public GDBWatchCreateAction
    {
        public:
            GDBWatchCreateTooltipAction(cb::shared_ptr<GDBWatch> const & watch, GDBWatchesContainer & watches,
                                     LogPaneLogger * logger, wxRect const & rect) :
                GDBWatchCreateAction(watch, watches, logger),
                m_rect(rect)
            {
            }
            virtual ~GDBWatchCreateTooltipAction();
        private:
            wxRect m_rect;
    };

    class GDBWatchesUpdateAction : public GDBWatchBaseAction
    {
        public:
            GDBWatchesUpdateAction(GDBWatchesContainer & watches, LogPaneLogger * logger);

            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();

        private:
            bool ParseUpdate(ResultParser const & result);
        private:
            CommandID   m_update_command;
    };

    class GDBWatchExpandedAction : public GDBWatchBaseAction
    {
        public:
            GDBWatchExpandedAction(cb::shared_ptr<GDBWatch> parent_watch, cb::shared_ptr<GDBWatch> expanded_watch,
                                GDBWatchesContainer & watches, LogPaneLogger * logger) :
                GDBWatchBaseAction(watches, logger),
                m_watch(parent_watch),
                m_expanded_watch(expanded_watch)
            {
            }

            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();

        private:
            CommandID m_update_id;
            cb::shared_ptr<GDBWatch> m_watch;
            cb::shared_ptr<GDBWatch> m_expanded_watch;
    };

    class GDBWatchCollapseAction : public GDBWatchBaseAction
    {
        public:
            GDBWatchCollapseAction(cb::shared_ptr<GDBWatch> parent_watch, cb::shared_ptr<GDBWatch> collapsed_watch,
                                GDBWatchesContainer & watches, LogPaneLogger * logger) :
                GDBWatchBaseAction(watches, logger),
                m_watch(parent_watch),
                m_collapsed_watch(collapsed_watch)
            {
            }

            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result);
        protected:
            virtual void OnStart();

        private:
            cb::shared_ptr<GDBWatch> m_watch;
            cb::shared_ptr<GDBWatch> m_collapsed_watch;
    };

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_ACTIONS_H_
