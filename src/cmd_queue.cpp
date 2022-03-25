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

CommandID CommandExecutor::Execute(wxString const & cmd)
{
    dbg_mi::CommandID id(0, m_last++);

    if (m_logger)
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("cmd==>%s%s==<",  id.ToString(),cmd), LogPaneLogger::LineType::Command);
        AddCommandQueue(id.ToString() + cmd);
    }

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
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("cmd==>%s%s==<",  id.ToString(),cmd), LogPaneLogger::LineType::Command);
        AddCommandQueue(id.ToString() + cmd);
    }

    DoExecute(id, cmd);
}

bool CommandExecutor::ProcessOutput(wxString const & output)
{
    //dbg_mi::CommandID id;
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
        }
        else
        {
            if (logger && action.HasPendingCommands())
            {
                logger->LogGDBMsgType(  __PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format("action[%p id: %d] has pending commands but is being removed", &action, action.GetID()),
                                        LogPaneLogger::LineType::Debug
                                    );
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
