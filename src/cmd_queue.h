/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#ifndef _DEBUGGER_MI_GDB_CMD_QUEUE_H_
#define _DEBUGGER_MI_GDB_CMD_QUEUE_H_

// System and library includes
#include <deque>
#include <ostream>
#include <tr1/unordered_map>
#include <wx/string.h>

// GDB includes
#include "cmd_result_parser.h"
#include "gdb_logger.h"

namespace dbg_mi
{
    class CommandID
    {
        public:
            explicit CommandID(int32_t action = -1, int32_t command_in_action = -1) :
                m_action(action),
                m_command_in_action(command_in_action)
            {
            }

            bool operator ==(CommandID const & o) const
            {
                return m_action == o.m_action && m_command_in_action == o.m_command_in_action;
            }
            bool operator !=(CommandID const & o) const
            {
                return !(*this == o);
            }


            CommandID & operator++()   // prefix
            {
                ++m_command_in_action;
                return *this;
            }

            CommandID operator++(int)   // postfix
            {
                CommandID old = *this;
                ++m_command_in_action;
                return old;
            }

            wxString ToString() const
            {
                return wxString::Format("%d%010d", m_action, m_command_in_action);
            }

            int32_t GetActionID() const
            {
                return m_action;
            }

            int32_t GetCommandID() const
            {
                return m_command_in_action;
            }

            int64_t GetFullID() const
            {
                return (static_cast<int64_t>(m_action) >> 32) + m_command_in_action;
            }

        private:
            int32_t m_action, m_command_in_action;
    };

    inline std::ostream & operator<< (std::ostream & s, CommandID const & id)
    {
        s << id.ToString().utf8_str().data();
        return s;
    }

    bool ParseGDBOutputLine(wxString const & line, CommandID & id, wxString & result_str);

    class Action
    {
            struct Command
            {
                Command() {}
                Command(wxString const & string_, int id_) :
                    string(string_),
                    id(id_)
                {
                }

                wxString string;
                int id;
            };
            typedef std::deque<Command> PendingCommands;

        public:
            Action() :
                m_id(-1),
                m_last_command_id(0),
                m_started(false),
                m_finished(false),
                m_wait_previous(false),
                m_StallCountActionsMapRun(0)
            {
            }

            virtual ~Action() {}

            void SetID(int id)
            {
                m_id = id;
            }

            int GetID() const
            {
                return m_id;
            }

            void Start()
            {
                m_started = true;
                OnStart();
            }

            void Finish()
            {
                m_finished = true;
            }

            bool Started() const
            {
                return m_started;
            }

            bool Finished() const
            {
                return m_finished;
            }

            void SetWaitPrevious(bool flag)
            {
                m_wait_previous = flag;
            }

            bool GetWaitPrevious() const
            {
                return m_wait_previous;
            }

            CommandID Execute(wxString const & command)
            {
                m_pending_commands.push_back(Command(command, m_last_command_id));
                return CommandID(m_id, m_last_command_id++);
            }

            int GetPendingCommandsCount() const
            {
                return m_pending_commands.size();
            }

            bool HasPendingCommands() const
            {
                return !m_pending_commands.empty();
            }

            wxString PopPendingCommand(CommandID & id)
            {
                assert(HasPendingCommands());
                Command cmd = m_pending_commands.front();
                m_pending_commands.pop_front();
                id = CommandID(GetID(), cmd.id);
                return cmd.string;
            }

            bool PeekPendingCommand(CommandID & id, wxString & cmdString)
            {
                if (HasPendingCommands())
                {
                    Command cmd = m_pending_commands.front();
                    id = CommandID(GetID(), cmd.id);
                    cmdString = cmd.string;
                    return true;
                }
                else
                {
                    return false;
                }
            }

            bool ShowStallCountActionsMapRunMessage()
            {
                ++m_StallCountActionsMapRun;
                return ( m_StallCountActionsMapRun%50 == 0);
            }

            void ClearStallCountActionsMapRun()
            {
                m_StallCountActionsMapRun = 0;
            }


        public:
            virtual void OnCommandOutput(CommandID const & id, ResultParser const & result) = 0;
        protected:
            virtual void OnStart() = 0;
        private:
            PendingCommands m_pending_commands;
            int m_id;
            int m_last_command_id;
            bool m_started;
            bool m_finished;
            bool m_wait_previous;
            long m_StallCountActionsMapRun;
    };

    class CommandExecutor
    {
        public:
            struct Result
            {
                dbg_mi::CommandID id;
                wxString output;
            };
        public:
            CommandExecutor() :
                m_last(0),
                m_logger(NULL)
            {
            }
            virtual ~CommandExecutor() {}

            CommandID Execute(wxString const & cmd);
            void ExecuteSimple(dbg_mi::CommandID const & id, wxString const & cmd);

            virtual wxString GetOutput() = 0;

            bool HasOutput() const
            {
                return !m_results.empty();
            }
            bool ProcessOutput(wxString const & output);

            void Clear();

            dbg_mi::ResultParser * GetResult(dbg_mi::CommandID & id)
            {
                assert(!m_results.empty());
                Result const & r = m_results.front();
                id = r.id;
                dbg_mi::ResultParser * parser = new dbg_mi::ResultParser;

                if (parser->Parse(r.output))
                {
                    dbg_mi::ResultParser::Class rClass = parser->GetResultClass();

                    if (
                        (rClass == dbg_mi::ResultParser::ClassStopped)                      ||
                        (r.output.StartsWith("=library-loaded,id="))                        ||
                        (r.output.StartsWith("=breakpoint-modified,bkpt="))                 ||
                        (r.output.StartsWith("^done"))
                    )
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                                __LINE__,
                                                wxString::Format(_("Parsing: id: %s parser ==>%s<== for ==>%s<=="), id.ToString(), parser->MakeDebugString(), r.output),
                                                dbg_mi::LogPaneLogger::LineType::Info
                                               );
                    }
                    else
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                                __LINE__,
                                                wxString::Format(_("Parsing : id: %s parser ==>%s<== for ==>%s<=="), id.ToString(), parser->MakeDebugString(), r.output),
                                                dbg_mi::LogPaneLogger::LineType::Receive
                                               );
                    }
                }
                else
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                            __LINE__,
                                            wxString::Format(_("Received parsing failed : id: %s for %s"), id.ToString(), r.output),
                                            dbg_mi::LogPaneLogger::LineType::Error);
                    delete parser;
                    parser = NULL;
                }

                m_results.pop_front();
                return parser;
            }

            void SetLogger(dbg_mi::LogPaneLogger * logger)
            {
                m_logger = logger;
            }

            dbg_mi::LogPaneLogger * GetLogger()
            {
                return m_logger;
            }

            int32_t GetLastID() const
            {
                return m_last;
            }

            void AddCommandQueue(wxString const & command)
            {
                m_CMDQueue.push_back(command);
            }

            int GetCommandQueueCount() const
            {
                return m_CMDQueue.size();
            }

            wxString const & GetQueueCommand(long index) const
            {
                static const wxString emptyString = wxEmptyString;

                if (index < (long) m_CMDQueue.size())
                {
                    return m_CMDQueue[index];
                }

                return  emptyString;
            }

            void ClearQueueCommand()
            {
                m_CMDQueue.clear();
            }

        protected:
            virtual bool DoExecute(dbg_mi::CommandID const & id, wxString const & cmd) = 0;
            virtual void DoClear() = 0;

        protected:
            typedef std::deque<Result> Results;
            Results m_results;
            int32_t m_last;

            std::vector<wxString> m_CMDQueue;

            dbg_mi::LogPaneLogger * m_logger;
    };

    class ActionsMap
    {
        public:
            ActionsMap();
            ~ActionsMap();

            void Add(Action * action);
            Action * Find(int id);
            Action const * Find(int id) const;
            void Clear();
            int GetLastID() const
            {
                return m_last_id;
            }

            bool Empty() const
            {
                return m_actions.empty();
            }
            void Run(CommandExecutor & executor);
        private:
            typedef std::deque<Action *> Actions;

            Actions m_actions;
            int m_last_id;
    };

    template<typename OnNotify>
    bool DispatchResults(CommandExecutor & exec, ActionsMap & actions_map, OnNotify & on_notify)
    {
        while (exec.HasOutput())
        {
            CommandID id;
            ResultParser * parser = exec.GetResult(id);

            if (!parser)
            {
                return false;
            }

            switch (parser->GetResultType())
            {
                case ResultParser::Result:
                {
                    Action * action = actions_map.Find(id.GetActionID());

                    if (action)
                    {
                        action->OnCommandOutput(id, *parser);
                    }
                }
                break;

                case ResultParser::TypeUnknown:
                    break;

                default:
                    on_notify(*parser);
            }

            delete parser;
        }

        return true;
    }

} // namespace dbg_mi

namespace std
{
    namespace tr1
    {
        template <>
        struct hash<dbg_mi::CommandID> : public unary_function<dbg_mi::CommandID, size_t>
        {
            size_t operator()(dbg_mi::CommandID const & v) const
            {
                return std::tr1::hash<int64_t>()(v.GetFullID());
            }
        };

    }
}

#endif // _DEBUGGER_MI_GDB_CMD_QUEUE_H_
