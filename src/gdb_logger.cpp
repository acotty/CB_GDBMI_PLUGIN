/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "plugin.h"
#include "gdb_logger.h"

namespace dbg_mi
{

    Debugger_GDB_MI * LogPaneLogger::m_dbgGDB = nullptr;

    LogPaneLogger::LogPaneLogger(Debugger_GDB_MI * dbgGDB)
    {
        m_dbgGDB = dbgGDB;
    }

    LogPaneLogger::~LogPaneLogger()
    {
        m_dbgGDB = nullptr;
    }

    void LogPaneLogger::LogGDBMsgType(wxString const & functionName, int const iLineNumber, wxString const & msg, LineType type)
    {
        if (!m_dbgGDB)
        {
            return;
        }

        if (
                !m_dbgGDB->HasDebugLog() &&
                (
                    //UserDisplay = 0,
                    (type == LineType::Info) ||
                    (type == LineType::Debug) ||
                    //LineType::Warning,
                    //LineType::Error,

                    //LineType::GDB_Stop_Start,
                    (type == LineType::Queue) ||
                    (type == LineType::Command) ||
                    (type == LineType::CommandResult) ||
                    (type == LineType::ProgramState) ||
                    (type == LineType::Event) ||
                    (type == LineType::Transmit) ||
                    (type == LineType::Receive) ||
                    (type == LineType::Receive_NoLine) ||
                    (type == LineType::Receive_Info)
                )
           )
        {
            return;
        }

        wxString logMsg = msg;
        wxString msgType;
        wxChar msgNewLineChar('-');
        wxString msgDeliminators = "[]";
        wxString msgNewLine(msgNewLineChar, 15);
        wxString msgPrefix = wxEmptyString;
        wxString msgAppend = wxEmptyString;

        switch (type)
        {
            case LineType::UserDisplay:
                msgDeliminators = "  ";
                msgType = "";
                break;

            case LineType::Info:
                msgType = "info";
                break;;

            case LineType::Debug:
                msgType = "debug";
                break;

            case LineType::Warning:
                msgDeliminators = "##";
                msgType = "warning";
                break;

            case LineType::Error:
                msgDeliminators = "**";
                msgType = " ERROR";
                break;

            case LineType::Queue:
                msgType = "queued";
                break;

            case LineType::Command:
                msgType = "cmd";
                break;

            case LineType::CommandResult:
                msgType = "cmd res";
                break;

            case LineType::ProgramState:
                msgType = "pgm sta";
                break;

            case LineType::Event:
                msgType = "event";
                break;

            case LineType::Transmit:
                msgDeliminators = "{}";
                msgType = "Transmit";
                //msgPrefix.Append("\n");
                msgPrefix.Append(msgNewLine);
                break;

            case LineType::Receive_Info:
            case LineType::Receive_NoLine:
            case LineType::Receive:
                msgDeliminators = "{}";
                msgType = "Receive";

                if (type == LineType::Receive)
                {
                    msgPrefix.Append(msgNewLine);
                }

                break;

            case LineType::GDB_Stop_Start:
                msgNewLineChar = '*';
                msgDeliminators = "==";
                msgType = "GDB_Stop_Start";
                msgPrefix.Append("\n");
                msgPrefix.Append(msgNewLine);
                msgAppend.Append(msgNewLine);
                msgAppend.Append('\n');
                break;

            default:
                msgDeliminators = "??";
                msgType = "Unknown";
                break;
        }

        if (msgPrefix.empty())
        {
            if (logMsg.StartsWith("\n"))
            {
                while (logMsg.StartsWith("\n"))
                {
                    msgPrefix.Append("\n");
                    logMsg = logMsg.AfterFirst('\n');
                }

                msgPrefix.Append(msgNewLine);
            }
        }
        else
        {
            while (logMsg.StartsWith("\n"))
            {
                logMsg = logMsg.AfterFirst('\n');
            }
        }

        if (msgAppend.empty())
        {
            if (logMsg.EndsWith("\n"))
            {
                msgAppend.Append(msgNewLine);

                while (logMsg.EndsWith("\n"))
                {
                    msgAppend.Append('\n');
                    logMsg = logMsg.BeforeLast('\n');
                }
            }
        }
        else
        {
            while (logMsg.EndsWith("\n"))
            {
                logMsg = logMsg.BeforeLast('\n');
            }
        }

        wxString classAndFunctionName;;

        if (functionName.Contains("::"))
        {
            size_t colons = functionName.find("::");
            size_t begin = functionName.substr(0, colons).rfind(" ") + 1;
            size_t end = functionName.rfind("(") - begin;
            classAndFunctionName = functionName.substr(begin, end);
        }
        else
        {
            classAndFunctionName = functionName;
        }

        if (!msgPrefix.empty())
        {
            m_dbgGDB->Log(msgPrefix);
        }

        m_dbgGDB->Log(wxString::Format("%c%-8s%c <%42s(L%6d)> %s", msgDeliminators[0], msgType, msgDeliminators[1], classAndFunctionName, iLineNumber, logMsg), ::Logger::info);

        if (!msgAppend.empty())
        {
            m_dbgGDB->Log(msgAppend);
        }
    }
} //namespace dbg_mi
