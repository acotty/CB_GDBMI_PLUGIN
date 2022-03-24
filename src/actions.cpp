/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "actions.h"

#include <cbdebugger_interfaces.h>
#include <cbplugin.h>
#include <logmanager.h>

#include "cmd_result_parser.h"
#include "frame.h"
#include "updated_variable.h"
#include "definitions.h"

namespace dbg_mi
{

void BreakpointAddAction::OnStart()
{
    wxString cmd("-break-insert -f ");

    if (m_breakpoint->HasCondition())
    {
        cmd += "-c " + m_breakpoint->GetCondition() + " ";
    }

    if (m_breakpoint->HasIgnoreCount())
    {
        cmd += "-i " + wxString::Format("%d ", m_breakpoint->GetIgnoreCount());
    }

    cmd += wxString::Format("%s:%d", m_breakpoint->GetLocation().c_str(), m_breakpoint->GetLine());
    m_initial_cmd = Execute(cmd);
    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::m_initial_cmd = " + m_initial_cmd.ToString()), LogPaneLogger::LineType::Debug);
}

void BreakpointAddAction::OnCommandOutput(CommandID const & id, ResultParser const & result)
{
    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::OnCommandResult: " + id.ToString()), LogPaneLogger::LineType::Debug);

    if (m_initial_cmd == id)
    {
        bool finish = true;
        const ResultValue & value = result.GetResultValue();

        if (result.GetResultClass() == ResultParser::ClassDone)
        {
            const ResultValue * number = value.GetTupleValue("bkpt.number");

            if (number)
            {
                const wxString & number_value = number->GetSimpleValue();
                long n;

                if (number_value.ToLong(&n, 10))
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("BreakpointAddAction::breakpoint index is %d"), n), LogPaneLogger::LineType::Debug);
                    m_breakpoint->SetIndex(n);

                    if (!m_breakpoint->IsEnabled())
                    {
                        m_disable_cmd = Execute(wxString::Format("-break-disable %d", n));
                        finish = false;
                    }
                }
                else
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::error getting the index :( "), LogPaneLogger::LineType::Debug);
                }
            }
            else
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("BreakpointAddAction::error getting number value:==>%s<== "),value.MakeDebugString()), LogPaneLogger::LineType::Debug);
            }
        }
        else
            if (result.GetResultClass() == ResultParser::ClassError)
            {
                wxString message;

                if (Lookup(value, "msg", message))
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Error detected:==>%s<== "),message), LogPaneLogger::LineType::Error);
                }
            }

        if (finish)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::Finishing1"), LogPaneLogger::LineType::Debug);
            Finish();
        }
    }
    else
        if (m_disable_cmd == id)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::Finishing2"), LogPaneLogger::LineType::Debug);
            Finish();
        }
}

GenerateBacktrace::GenerateBacktrace(SwitchToFrameInvoker * switch_to_frame, BacktraceContainer & backtrace,
                                     CurrentFrame & current_frame, LogPaneLogger * logger) :
    m_switch_to_frame(switch_to_frame),
    m_backtrace(backtrace),
    m_logger(logger),
    m_current_frame(current_frame),
    m_first_valid(-1),
    m_old_active_frame(-1),
    m_parsed_backtrace(false),
    m_parsed_args(false),
    m_parsed_frame_info(false)
{
}

GenerateBacktrace::~GenerateBacktrace()
{
    delete m_switch_to_frame;
}

void GenerateBacktrace::OnCommandOutput(CommandID const & id, ResultParser const & result)
{
    if (id == m_backtrace_id)
    {
        ResultValue const * stack = result.GetResultValue().GetTupleValue("stack");

        if (!stack)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GenerateBacktrace::OnCommandOutput: no stack tuple in the output"), LogPaneLogger::LineType::Debug);
        }
        else
        {
            int size = stack->GetTupleSize();
            m_logger->LogGDBMsgType(    __PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("GenerateBacktrace::OnCommandOutput: tuple size %d %s"), size, stack->MakeDebugString()),
                                        LogPaneLogger::LineType::Debug
                                    );
            m_backtrace.clear();

            for (int ii = 0; ii < size; ++ii)
            {
                ResultValue const * frame_value = stack->GetTupleValueByIndex(ii);
                assert(frame_value);
                Frame frame;

                if (frame.ParseFrame(*frame_value))
                {
                    cbStackFrame s;

                    if (frame.HasValidSource())
                    {
                        s.SetFile(frame.GetFilename(), wxString::Format("%d", frame.GetLine()));
                    }
                    else
                    {
                        s.SetFile(frame.GetFrom(), wxEmptyString);
                    }

                    s.SetSymbol(frame.GetFunction());
                    s.SetNumber(ii);
                    s.SetAddress(frame.GetAddress());
                    s.MakeValid(frame.HasValidSource());

                    if (s.IsValid() && m_first_valid == -1)
                    {
                        m_first_valid = ii;
                    }

                    m_backtrace.push_back(cb::shared_ptr<cbStackFrame>(new cbStackFrame(s)));
                }
                else
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("can't parse frame:==>%s<=="), frame_value->MakeDebugString()), LogPaneLogger::LineType::Debug);
                }
            }
        }

        m_parsed_backtrace = true;
    }
    else
        if (id == m_args_id)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("GenerateBacktrace::OnCommandOutput arguments"), LogPaneLogger::LineType::Debug);
            FrameArguments arguments;

            if (!arguments.Attach(result.GetResultValue()))
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("GenerateBacktrace::OnCommandOutput: can't attach to output of command:==>%s<=="), id.ToString()),
                                        LogPaneLogger::LineType::Debug
                                        );
            }
            else
                if (arguments.GetCount() != static_cast<int>(m_backtrace.size()))
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                            __LINE__,
                                            _("GenerateBacktrace::OnCommandOutput: stack arg count differ from the number of frames"),
                                            LogPaneLogger::LineType::Debug
                                           );
                }
                else
                {
                    int size = arguments.GetCount();

                    for (int ii = 0; ii < size; ++ii)
                    {
                        wxString args;

                        if (arguments.GetFrame(ii, args))
                        {
                            m_backtrace[ii]->SetSymbol(m_backtrace[ii]->GetSymbol() + "(" + args + ")");
                        }
                        else
                        {
                            m_logger->LogGDBMsgType(    __PRETTY_FUNCTION__,
                                                        __LINE__,
                                                        wxString::Format(_("GenerateBacktrace::OnCommandOutput: can't get args for frame %d"),ii),
                                                        LogPaneLogger::LineType::Debug
                                                    );
                        }
                    }
                }

            m_parsed_args = true;
        }
        else
            if (id == m_frame_info_id)
            {
                m_parsed_frame_info = true;

                //^done,frame={level="0",addr="0x0000000000401060",func="main",
                //file="/path/main.cpp",fullname="/path/main.cpp",line="80"}
                if (result.GetResultClass() != ResultParser::ClassDone)
                {
                    m_old_active_frame = 0;
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Wrong result class, using default value!"), LogPaneLogger::LineType::Debug);
                }
                else
                {
                    if (!Lookup(result.GetResultValue(), "frame.level", m_old_active_frame))
                    {
                        m_old_active_frame = 0;
                    }
                }
            }

    if (m_parsed_backtrace && m_parsed_args && m_parsed_frame_info)
    {
        if (!m_backtrace.empty())
        {
            int frame = m_current_frame.GetUserSelectedFrame();

            if (frame < 0 && cbDebuggerCommonConfig::GetFlag(cbDebuggerCommonConfig::AutoSwitchFrame))
            {
                frame = m_first_valid;
            }

            if (frame < 0)
            {
                frame = 0;
            }

            m_current_frame.SetFrame(frame);
            int number = m_backtrace.empty() ? 0 : m_backtrace[frame]->GetNumber();

            if (m_old_active_frame != number)
            {
                m_switch_to_frame->Invoke(number);
            }
        }

        Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
        Finish();
    }
}
void GenerateBacktrace::OnStart()
{
    m_frame_info_id = Execute("-stack-info-frame");
    m_backtrace_id = Execute("-stack-list-frames 0 30");
    m_args_id = Execute("-stack-list-arguments 1 0 30");
}

GenerateThreadsList::GenerateThreadsList(ThreadsContainer & threads, int current_thread_id, LogPaneLogger * logger) :
    m_threads(threads),
    m_logger(logger),
    m_current_thread_id(current_thread_id)
{
}

void GenerateThreadsList::OnCommandOutput(CommandID const & /*id*/, ResultParser const & result)
{
    Finish();
    m_threads.clear();
    int current_thread_id = 0;

    if (!Lookup(result.GetResultValue(), "current-thread-id", current_thread_id))
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GenerateThreadsList::OnCommandOutput - no current thread id"), LogPaneLogger::LineType::Debug);
        return;
    }

    ResultValue const * threads = result.GetResultValue().GetTupleValue("threads");

    if (!threads || (threads->GetType() != ResultValue::Tuple && threads->GetType() != ResultValue::Array))
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("GenerateThreadsList::OnCommandOutput - no threads"), LogPaneLogger::LineType::Debug);
        return;
    }

    int count = threads->GetTupleSize();

    for (int ii = 0; ii < count; ++ii)
    {
        ResultValue const & thread_value = *threads->GetTupleValueByIndex(ii);
        int thread_id;

        if (!Lookup(thread_value, "id", thread_id))
        {
            continue;
        }

        wxString info;

        if (!Lookup(thread_value, "target-id", info))
        {
            info = wxEmptyString;
        }

        ResultValue const * frame_value = thread_value.GetTupleValue("frame");

        if (frame_value)
        {
            wxString str;

            if (Lookup(*frame_value, "addr", str))
            {
                info += " " + str;
            }

            if (Lookup(*frame_value, "func", str))
            {
                info += " " + str;

                if (FrameArguments::ParseFrame(*frame_value, str))
                {
                    info += "(" + str + ")";
                }
                else
                {
                    info += "()";
                }
            }

            int line;

            if (Lookup(*frame_value, "file", str) && Lookup(*frame_value, "line", line))
            {
                info += wxString::Format(" in %s:%d", str.c_str(), line);
            }
            else
                if (Lookup(*frame_value, "from", str))
                {
                    info += " in " + str;
                }
        }

        m_threads.push_back(cb::shared_ptr<cbThread>(new cbThread(thread_id == current_thread_id, thread_id, info)));
    }

    Manager::Get()->GetDebuggerManager()->GetThreadsDialog()->Reload();
}

void GenerateThreadsList::OnStart()
{
    Execute("-thread-info");
}

void ParseWatchInfo(ResultValue const & value, int & children_count, bool & dynamic, bool & has_more)
{
    dynamic = has_more = false;
    int temp;

    if (Lookup(value, "dynamic", temp))
    {
        dynamic = (temp == 1);
    }

    if (Lookup(value, "has_more", temp))
    {
        has_more = (temp == 1);
    }

    if (!Lookup(value, "numchild", children_count))
    {
        children_count = -1;
    }
}

void ParseWatchValueID(Watch & watch, ResultValue const & value)
{
    wxString s;

    if (Lookup(value, "name", s))
    {
        watch.SetID(s);
    }

    if (Lookup(value, "value", s))
    {
        watch.SetValue(s);
    }

    if (Lookup(value, "type", s))
    {
        watch.SetType(s);
    }
}

bool WatchHasType(ResultValue const & value)
{
    wxString s;
    return Lookup(value, "type", s);
}

void AppendNullChild(cb::shared_ptr<Watch> watch)
{
    cbWatch::AddChild(watch, cb::shared_ptr<cbWatch>(new Watch("updating...", watch->ForTooltip())));
}

cb::shared_ptr<Watch> AddChild(cb::shared_ptr<Watch> parent, ResultValue const & child_value, wxString const & symbol,
                               WatchesContainer & watches)
{
    wxString id;

    if (!Lookup(child_value, "name", id))
    {
        return cb::shared_ptr<Watch>();
    }

    cb::shared_ptr<Watch> child = FindWatch(id, watches);

    if (child)
    {
        wxString s;

        if (Lookup(child_value, "value", s))
        {
            child->SetValue(s);
        }

        if (Lookup(child_value, "type", s))
        {
            child->SetType(s);
        }
    }
    else
    {
        child = cb::shared_ptr<Watch>(new dbg_mi::Watch(symbol, parent->ForTooltip()));
        ParseWatchValueID(*child, child_value);
        cbWatch::AddChild(parent, child);
    }

    child->MarkAsRemoved(false);
    return child;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateWatches(LogPaneLogger * logger)
{
#ifndef TEST_PROJECT
    logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("updating watches"), LogPaneLogger::LineType::Debug);

    //Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->OnDebuggerUpdated();

    CodeBlocksEvent event(cbEVT_DEBUGGER_UPDATED);
    event.SetInt(int(cbDebuggerPlugin::DebugWindows::Watches));
    //event.SetPlugin(m_pDriver->GetDebugger());
    Manager::Get()->ProcessEvent(event);
#endif
}

void UpdateWatchesTooltipOrAll(const cb::shared_ptr<Watch> & watch, LogPaneLogger * logger)
{
#ifndef TEST_PROJECT

    if (watch->ForTooltip())
    {
        logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("updating tooltip watches"), LogPaneLogger::LineType::Debug);
        Manager::Get()->GetDebuggerManager()->GetInterfaceFactory()->UpdateValueTooltip();
    }
    else
    {
        UpdateWatches(logger);
    }

#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WatchBaseAction::WatchBaseAction(WatchesContainer & watches, LogPaneLogger * logger) :
    m_watches(watches),
    m_logger(logger),
    m_sub_commands_left(0),
    m_start(-1),
    m_end(-1)
{
}

WatchBaseAction::~WatchBaseAction()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool WatchBaseAction::ParseListCommand(CommandID const & id, ResultValue const & value)
{
    bool error = false;
    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                            __LINE__,
                            wxString::Format(_("WatchBaseAction::ParseListCommand - steplistchildren for id: %s ==>$s>=="), id.ToString(), value.MakeDebugString()),
                            LogPaneLogger::LineType::Debug);
    ListCommandParentMap::iterator it = m_parent_map.find(id);

    if (it == m_parent_map.end() || !it->second)
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                __LINE__,
                                wxString::Format(_("WatchBaseAction::ParseListCommand - no parent for id: ==>%s<=="), id.ToString()),
                                LogPaneLogger::LineType::Debug);
        return false;
    }

    struct DisplayHint
    {
        enum Enum { None = 0, Array, Map };
    };

    DisplayHint::Enum displayHint = DisplayHint::None;

    wxString strDisplayHint;

    if (Lookup(value, "displayhint", strDisplayHint))
    {
        if (strDisplayHint == "map")
        {
            displayHint = DisplayHint::Map;
        }
        else
            if (strDisplayHint == "array")
            {
                displayHint = DisplayHint::Array;
            }
    }

    ResultValue const * children = value.GetTupleValue("children");

    if (children)
    {
        int count = children->GetTupleSize();
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                __LINE__,
                                wxString::Format(_("WatchBaseAction::ParseListCommand - children %d"), count),
                                LogPaneLogger::LineType::Debug);
        cb::shared_ptr<Watch> parent_watch = it->second;
        wxString strMapKey;

        for (int ii = 0; ii < count; ++ii)
        {
            ResultValue const * child_value;
            child_value = children->GetTupleValueByIndex(ii);

            if (child_value->GetName() == "child")
            {
                wxString symbol;

                if (!Lookup(*child_value, "exp", symbol))
                {
                    symbol = "--unknown--";
                }

                cb::shared_ptr<Watch> child;
                bool dynamic, has_more;
                int children_count;
                ParseWatchInfo(*child_value, children_count, dynamic, has_more);
                bool mapValue = false;

                if (displayHint == DisplayHint::Map)
                {
                    if ((ii & 1) == 0)
                    {
                        if (!Lookup(*child_value, "value", strMapKey))
                        {
                            strMapKey = wxEmptyString;
                        }

                        continue;
                    }
                    else
                    {
                        mapValue = true;
                    }
                }

                if (dynamic && has_more)
                {
                    child = cb::shared_ptr<Watch>(new Watch(symbol, parent_watch->ForTooltip(), false));
                    ParseWatchValueID(*child, *child_value);
                    ExecuteListCommand(child, parent_watch);
                }
                else
                {
                    switch (children_count)
                    {
                        case -1:
                            error = true;
                            break;

                        case 0:
                            if (!parent_watch->HasBeenExpanded())
                            {
                                parent_watch->SetHasBeenExpanded(true);
                                parent_watch->RemoveChildren();
                            }

                            child = AddChild(parent_watch, *child_value, (mapValue ? strMapKey : symbol), m_watches);

                            if (dynamic)
                            {
                                child->SetDeleteOnCollapse(false);
                                wxString id;

                                if (Lookup(*child_value, "name", id))
                                {
                                    ExecuteListCommand(id, child);
                                }
                            }

                            child = cb::shared_ptr<Watch>();
                            break;

                        default:
                            if (WatchHasType(*child_value))
                            {
                                if (!parent_watch->HasBeenExpanded())
                                {
                                    parent_watch->SetHasBeenExpanded(true);
                                    parent_watch->RemoveChildren();
                                }

                                child = AddChild(parent_watch, *child_value, (mapValue ? strMapKey : symbol), m_watches);
                                AppendNullChild(child);
                                m_logger->LogGDBMsgType(    __PRETTY_FUNCTION__,
                                                            __LINE__,
                                                            wxString::Format(_("WatchBaseAction::ParseListCommand - adding child ==>%s<== to ==>%s<=="), child->GetDebugString(),  parent_watch->GetDebugString()),
                                                            LogPaneLogger::LineType::Debug
                                                       );
                                child = cb::shared_ptr<Watch>();
                            }
                            else
                            {
                                wxString id;

                                if (Lookup(*child_value, "name", id))
                                {
                                    ExecuteListCommand(id, parent_watch);
                                }
                            }
                    }
                }
            }
            else
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("WatchBaseAction::ParseListCommand - can't find child in ==>%s<=="), children->GetTupleValueByIndex(ii)->MakeDebugString()),
                                        LogPaneLogger::LineType::Debug);
            }
        }

        parent_watch->RemoveMarkedChildren();
    }

    return !error;
}

void WatchBaseAction::ExecuteListCommand(cb::shared_ptr<Watch> watch, cb::shared_ptr<Watch> parent)
{
    CommandID id;

    if (m_start > -1 && m_end > -1)
    {
        id = Execute(wxString::Format("-var-list-children 2 \"%s\" %d %d ",
                                      watch->GetID().c_str(), m_start, m_end));
    }
    else
    {
        id = Execute(wxString::Format("-var-list-children 2 \"%s\"", watch->GetID().c_str()));
    }

    m_parent_map[id] = parent ? parent : watch;
    ++m_sub_commands_left;
}

void WatchBaseAction::ExecuteListCommand(wxString const & watch_id, cb::shared_ptr<Watch> parent)
{
    if (!parent)
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Parent for '" + watch_id + "' is NULL; skipping this watch"), LogPaneLogger::LineType::Debug);
        return;
    }

    CommandID id;

    if (m_start > -1 && m_end > -1)
    {
        id = Execute(wxString::Format("-var-list-children 2 \"%s\" %d %d ", watch_id.c_str(), m_start, m_end));
    }
    else
    {
        id = Execute(wxString::Format("-var-list-children 2 \"%s\"", watch_id.c_str()));
    }

    m_parent_map[id] = parent;
    ++m_sub_commands_left;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WatchCreateAction::WatchCreateAction(cb::shared_ptr<Watch> const & watch, WatchesContainer & watches, LogPaneLogger * logger) :
    WatchBaseAction(watches, logger),
    m_watch(watch),
    m_step(StepCreate)
{
}

void WatchCreateAction::OnCommandOutput(CommandID const & id, ResultParser const & result)
{
    --m_sub_commands_left;
    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("WatchCreateAction::OnCommandOutput - processing command ==>%s<=="), id.ToString()), LogPaneLogger::LineType::Debug);
    bool error = false;

    if (result.GetResultClass() == ResultParser::ClassDone)
    {
        ResultValue const & value = result.GetResultValue();

        switch (m_step)
        {
            case StepCreate:
            {
                bool dynamic, has_more;
                int children;
                ParseWatchInfo(value, children, dynamic, has_more);
                ParseWatchValueID(*m_watch, value);

                if (dynamic && has_more)
                {
                    m_step = StepSetRange;
                    Execute("-var-set-update-range \"" + m_watch->GetID() + "\" 0 100");
                    AppendNullChild(m_watch);
                }
                else
                    if (children > 0)
                    {
                        m_step = StepListChildren;
                        AppendNullChild(m_watch);
                    }
                    else
                    {
                        Finish();
                    }
            }
            break;

            case StepListChildren:
                error = !ParseListCommand(id, value);
                break;
        }
    }
    else
    {
        if (result.GetResultClass() == ResultParser::ClassError)
        {
            m_watch->SetValue("The expression can't be evaluated");
        }

        error = true;
    }

    if (error)
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                __LINE__,
                                wxString::Format(_("WatchCreateAction::OnCommandOutput - error in command: ==>%s<<=="), id.ToString()),
                                LogPaneLogger::LineType::Debug);
        UpdateWatches(m_logger);
        Finish();
    }
    else
        if (m_sub_commands_left == 0)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                    __LINE__,
                                    wxString::Format(_("WatchCreateAction::Output - finishing at ==>%s<<=="),  id.ToString()),
                                    LogPaneLogger::LineType::Debug);
            UpdateWatches(m_logger);
            Finish();
        }
}

void WatchCreateAction::OnStart()
{
    wxString symbol;
    m_watch->GetSymbol(symbol);
    symbol.Replace("\"", "\\\"");
    Execute(wxString::Format("-var-create - @ \"%s\"", symbol.c_str()));
    m_sub_commands_left = 1;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WatchCreateTooltipAction::~WatchCreateTooltipAction()
{
    if (m_watch->ForTooltip())
    {
        Manager::Get()->GetDebuggerManager()->GetInterfaceFactory()->ShowValueTooltip(m_watch, m_rect);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WatchesUpdateAction::WatchesUpdateAction(WatchesContainer & watches, LogPaneLogger * logger) :
    WatchBaseAction(watches, logger)
{
}

void WatchesUpdateAction::OnStart()
{
    m_update_command = Execute("-var-update 1 *");
    m_sub_commands_left = 1;
}

bool WatchesUpdateAction::ParseUpdate(ResultParser const & result)
{
    if (result.GetResultClass() == ResultParser::ClassError)
    {
        Finish();
        return false;
    }

    ResultValue const * list = result.GetResultValue().GetTupleValue("changelist");

    if (list)
    {
        int count = list->GetTupleSize();

        for (int ii = 0; ii < count; ++ii)
        {
            ResultValue const * value = list->GetTupleValueByIndex(ii);
            wxString expression;

            if (!Lookup(*value, "name", expression))
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("WatchesUpdateAction::Output - no name in ==>%s<=="),value->MakeDebugString()),
                                        LogPaneLogger::LineType::Debug);
                continue;
            }

            cb::shared_ptr<Watch> watch = FindWatch(expression, m_watches);

            if (!watch)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("WatchesUpdateAction::Output - can't find watch ==>%s<<=="), expression),
                                        LogPaneLogger::LineType::Debug);
                continue;
            }

            UpdatedVariable updated_var;

            if (updated_var.Parse(*value))
            {
                switch (updated_var.GetInScope())
                {
                    case UpdatedVariable::InScope_No:
                        watch->Expand(false);
                        watch->RemoveChildren();
                        watch->SetValue("-- not in scope --");
                        break;

                    case UpdatedVariable::InScope_Invalid:
                        watch->Expand(false);
                        watch->RemoveChildren();
                        watch->SetValue("-- invalid -- ");
                        break;

                    case UpdatedVariable::InScope_Yes:
                        if (updated_var.IsDynamic())
                        {
                            if (updated_var.HasNewNumberOfChildren())
                            {
                                watch->RemoveChildren();

                                if (updated_var.GetNewNumberOfChildren() > 0)
                                {
                                    ExecuteListCommand(watch);
                                }
                            }
                            else
                                if (updated_var.HasMore())
                                {
                                    watch->MarkChildsAsRemoved(); // watch->RemoveChildren();
                                    ExecuteListCommand(watch);
                                }
                                else
                                    if (updated_var.HasValue())
                                    {
                                        watch->SetValue(updated_var.GetValue());
                                        watch->MarkAsChanged(true);
                                    }
                                    else
                                    {
                                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                                                __LINE__,
                                                                wxString::Format(_("WatchesUpdateAction::Output - unhandled dynamic variable ==>%s<=="),updated_var.MakeDebugString()),
                                                                LogPaneLogger::LineType::Debug);
                                    }
                        }
                        else
                        {
                            if (updated_var.HasNewNumberOfChildren())
                            {
                                watch->RemoveChildren();

                                if (updated_var.GetNewNumberOfChildren() > 0)
                                {
                                    ExecuteListCommand(watch);
                                }
                            }

                            if (updated_var.HasValue())
                            {
                                watch->SetValue(updated_var.GetValue());
                                watch->MarkAsChanged(true);
                                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                                        __LINE__,
                                                        wxString::Format(_("WatchesUpdateAction::Output - ==>%s<<== = ==>%s<<=="),  expression, updated_var.GetValue()),
                                                        LogPaneLogger::LineType::Debug
                                                        );
                            }
                            else
                            {
                                watch->SetValue(wxEmptyString);
                            }
                        }

                        break;
                }
            }
        }
    }

    return true;
}

void WatchesUpdateAction::OnCommandOutput(CommandID const & id, ResultParser const & result)
{
    --m_sub_commands_left;

    if (id == m_update_command)
    {
        for (WatchesContainer::iterator it = m_watches.begin();  it != m_watches.end(); ++it)
        {
            (*it)->MarkAsChangedRecursive(false);
        }

        if (!ParseUpdate(result))
        {
            Finish();
            return;
        }
    }
    else
    {
        ResultValue const & value = result.GetResultValue();

        if (!ParseListCommand(id, value))
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                    __LINE__,
                                    wxString::Format(_("WatchUpdateAction::Output - ParseListCommand failed ==>%s<<=="), id.ToString()),
                                    LogPaneLogger::LineType::Debug);
            Finish();
            return;
        }
    }

    if (m_sub_commands_left == 0)
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                __LINE__,
                                wxString::Format(_("WatchUpdateAction::Output - finishing at==>%s<<=="), id.ToString()),
                                LogPaneLogger::LineType::Debug
                                );
        UpdateWatches(m_logger);
        Finish();
    }
}

void WatchExpandedAction::OnStart()
{
    m_update_id = Execute("-var-update " + m_expanded_watch->GetID());
    ExecuteListCommand(m_expanded_watch, cb::shared_ptr<Watch>());
}

void WatchExpandedAction::OnCommandOutput(CommandID const & id, ResultParser const & result)
{
    if (id == m_update_id)
    {
        return;
    }

    --m_sub_commands_left;
    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                            __LINE__,
                            wxString::Format(_("WatchExpandedAction::Output - ==>%s<<=="), result.GetResultValue().MakeDebugString()),
                            LogPaneLogger::LineType::Debug);

    if (!ParseListCommand(id, result.GetResultValue()))
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                __LINE__,
                                wxString::Format(_("WatchExpandedAction::Output - error in command ==>%s<<=="), id.ToString()),
                                LogPaneLogger::LineType::Debug);
        // Update the watches even if there is an error, so some partial information can be displayed.
        UpdateWatchesTooltipOrAll(m_expanded_watch, m_logger);
        Finish();
    }
    else
        if (m_sub_commands_left == 0)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                    __LINE__,
                                    _("WatchExpandedAction::Output - done"),
                                    LogPaneLogger::LineType::Debug);
            UpdateWatchesTooltipOrAll(m_expanded_watch, m_logger);
            Finish();
        }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WatchCollapseAction::OnStart()
{
    Execute("-var-delete -c " + m_collapsed_watch->GetID());
}

void WatchCollapseAction::OnCommandOutput(CommandID const & id, ResultParser const & result)
{
    if (result.GetResultClass() == ResultParser::ClassDone)
    {
        m_collapsed_watch->SetHasBeenExpanded(false);
        m_collapsed_watch->RemoveChildren();
        AppendNullChild(m_collapsed_watch);
        UpdateWatchesTooltipOrAll(m_collapsed_watch, m_logger);
    }

    Finish();
}

} // namespace dbg_mi
