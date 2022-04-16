/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "cmd_queue.h"
#include <wx/wxcrt.h>

namespace dbg_mi
{

    bool ParseGDBOutputLine(wxString const & line, CommandID & id, wxString & result_str)
    {
        size_t pos = 0;

        while (pos < line.length() && wxIsdigit(line[pos]))
        {
            ++pos;
        }

        if (pos <= 10)
        {
            if (pos != 0)
            {
                return false;
            }

            if (line[0] == '*' || line[0] == '^' || line[0] == '+' || line[0] == '=')
            {
                id = CommandID();
                result_str = line;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            long action_id, cmd_id;
            wxString const & str_action = line.substr(0, pos - 10);
            str_action.ToLong(&action_id, 10);
            wxString const & str_cmd = line.substr(pos - 10, 10);
            str_cmd.ToLong(&cmd_id, 10);
            id = dbg_mi::CommandID(action_id, cmd_id);
            result_str = line.substr(pos, line.length() - pos);
            return true;
        }
    }

    CommandExecutor::CommandExecutor() :
        m_last(0),
        m_logger(NULL)
    {
    }

    CommandExecutor::~CommandExecutor()
    {
    }

    CommandID CommandExecutor::Execute(wxString const & cmd)
    {
        dbg_mi::CommandID id(0, m_last++);

        if (m_logger)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("cmd: %s%s",  id.ToString(), cmd), LogPaneLogger::LineType::Command);
        }
        AddCommandQueue(id.ToString() + cmd);

        if (DoExecute(id, cmd))
        {
            return id;
        }
        else
        {
            return dbg_mi::CommandID();
        }
    }

    void CommandExecutor::ExecuteSimple(dbg_mi::CommandID const & id, wxString const & cmd)
    {
        if (m_logger)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("cmd: %s%s",  id.ToString(), cmd), LogPaneLogger::LineType::Command);
        }
        AddCommandQueue(id.ToString() + cmd);

        DoExecute(id, cmd);
    }

    bool CommandExecutor::ProcessOutput(wxString const & output)
    {
        Result r;

        if (dbg_mi::ParseGDBOutputLine(output, r.id, r.output))
        {
            if (m_logger)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Receive ==>%s<==", output), LogPaneLogger::LineType::Info);
            }
        }
        else
        {
            if (m_logger)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("unparsable ==>%s<==", output), LogPaneLogger::LineType::Error);
            }

            return false;
        }

        m_results.push_back(r);
        return true;
    }

    void CommandExecutor::Clear()
    {
        m_last = 0;
        m_results.clear();
        DoClear();
    }

    dbg_mi::ResultParser * CommandExecutor::GetResult(dbg_mi::CommandID & id)
    {
        dbg_mi::ResultParser * parser = new dbg_mi::ResultParser;

        if (m_results.empty())
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Results are empty!!!")), dbg_mi::LogPaneLogger::LineType::Error);

            delete parser;
            parser = nullptr;
        }
        else
        {
            Result const & r = m_results.front();
            id = r.id;

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
                parser = nullptr;
            }
        }

        m_results.pop_front();
        return parser;
    }

    void CommandExecutor::SetLogger(dbg_mi::LogPaneLogger * logger)
    {
        m_logger = logger;
    }

    dbg_mi::LogPaneLogger * CommandExecutor::GetLogger()
    {
        return m_logger;
    }

    int32_t CommandExecutor::GetLastID() const
    {
        return m_last;
    }

    void CommandExecutor::AddCommandQueue(wxString const & command)
    {
        m_CMDQueue.push_back(command);
    }

    int CommandExecutor::GetCommandQueueCount() const
    {
        return m_CMDQueue.size();
    }

    wxString const & CommandExecutor::GetQueueCommand(long index) const
    {
        static const wxString emptyString = wxEmptyString;

        if (index < (long) m_CMDQueue.size())
        {
            return m_CMDQueue[index];
        }

        return  emptyString;
    }

    void CommandExecutor::ClearQueueCommand()
    {
        m_CMDQueue.clear();
    }

    bool CommandExecutor::HasOutput() const
    {
        return !m_results.empty();
    }

    ActionsMap::ActionsMap() :
        m_last_id(1)
    {
    }

    ActionsMap::~ActionsMap()
    {
        for (Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            delete *it;
        }
    }

    void ActionsMap::Add(Action * action)
    {
        action->SetID(m_last_id++);
        m_actions.push_back(action);
    }

    Action * ActionsMap::Find(int id)
    {
        for (Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            if ((*it)->GetID() == id)
            {
                return *it;
            }
        }

        return NULL;
    }

    Action const * ActionsMap::Find(int id) const
    {
        for (Actions::const_iterator it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            if ((*it)->GetID() == id)
            {
                return *it;
            }
        }

        return NULL;
    }

    Action * ActionsMap::FindStalled()
    {
        for (Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            if (!(*it)->Finished())
            {
                return *it;
            }
        }

        return NULL;
    }

    void ActionsMap::Clear()
    {
        for (Actions::iterator it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            delete *it;
        }

        m_actions.clear();
        m_last_id = 1;
    }

    void ActionsMap::Run(CommandExecutor & executor)
    {
        if (Empty())
        {
            return;
        }

        LogPaneLogger * logger = executor.GetLogger();
        bool first = true;

        for (Actions::iterator it = m_actions.begin(); it != m_actions.end();)
        {
            Action & action = **it;

            // test if we have a barrier action
            if (action.GetWaitPrevious() && !first)
            {
                break;
            }

            if (!action.Started())
            {
                action.Start();
            }

            while (action.HasPendingCommands())
            {
                CommandID id;
                wxString const & command = action.PopPendingCommand(id);
                executor.ExecuteSimple(id, command);
            }

            first = false;

            if (!action.Finished())
            {
                ++it;
                if (logger && ( action.ShowStallCountActionsMapRunMessage()))
                {
                    logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("action [id: %d] has not finished.", action.GetID()), LogPaneLogger::LineType::Warning );
                }
            }
            else
            {
                action.ClearStallCountActionsMapRun();

                if (logger && action.HasPendingCommands())
                {
                    logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("action [ID %d] has pending commands but is being removed", action.GetID()), LogPaneLogger::LineType::Debug);
                }

                delete *it;
                it = m_actions.erase(it);

                if (it == m_actions.begin())
                {
                    first = true;
                }
            }
        }
    }
} // namespace dbg_mi
