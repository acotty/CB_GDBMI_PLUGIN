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

        cmd += wxString::Format("%s:%d", m_breakpoint->GetLocation(), m_breakpoint->GetLine());
        m_initial_cmd = Execute(cmd);
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("BreakpointAddAction::m_initial_cmd = " + m_initial_cmd.ToString()), LogPaneLogger::LineType::Debug);
    }

    void BreakpointAddAction::OnCommandOutput(CommandID const & id, ResultParser const & result)
    {
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
                        m_breakpoint->SetIndex(n);

                        if (m_breakpoint->IsEnabled())
                        {
                            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Currently disabled id: %s index is %d for =>%s<="), id.ToString(), n, result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                        }
                        else
                        {
                            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Disabling id: %s index is %d for =>%s<="), id.ToString(), n, result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                            m_disable_cmd = Execute(wxString::Format("-break-disable %d", n));
                            finish = false;
                        }
                    }
                    else
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("bkpt.number not a valid number,  id: %s for =>%s<="), id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Error);
                    }
                }
                else
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("bkpt.number invalid/missing value, id: %s for ==>%s<== for =>%s<="), id.ToString(), value.MakeDebugString(), result.MakeDebugString()), LogPaneLogger::LineType::Error);
                }
            }
            else
            {
                if (result.GetResultClass() == ResultParser::ClassError)
                {
                    wxString message;

                    if (Lookup(value, "msg", message))
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Error detected id: %s ==>%s<== "), id.ToString(), message), LogPaneLogger::LineType::Error);
                    }
                }
                else
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("id: %s for =>%s<=", id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                }
            }

            if (finish)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("finishing for id: %s for =>%s<=", id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                Finish();
            }
        }
        else
        {
            if (m_disable_cmd == id)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("finishing for id: %s for =>%s<=", id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Debug);
                Finish();
            }
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
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("no stack tuple in the output"), LogPaneLogger::LineType::Error);
            }
            else
            {
                int iCount = stack->GetTupleSize();
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("tuple size %d %s"), iCount, stack->MakeDebugString()),
                                        LogPaneLogger::LineType::Debug
                                       );
                m_backtrace.clear();

                for (int ii = 0; ii < iCount; ++ii)
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
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, _("arguments"), LogPaneLogger::LineType::Debug);
                FrameArguments arguments;

                if (!arguments.Attach(result.GetResultValue()))
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                            __LINE__,
                                            wxString::Format(_("can't attach to output of command:==>%s<=="), id.ToString()),
                                            LogPaneLogger::LineType::Error
                                           );
                }
                else
                    if (arguments.GetCount() != static_cast<int>(m_backtrace.size()))
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                                __LINE__,
                                                _("stack arg count differ from the number of frames"),
                                                LogPaneLogger::LineType::Warning
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
                                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                                        __LINE__,
                                                        wxString::Format(_("can't get args for frame %d"), ii),
                                                        LogPaneLogger::LineType::Error
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
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, "", LogPaneLogger::LineType::Debug);
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

    void GenerateThreadsList::OnCommandOutput(CommandID const & id, ResultParser const & result)
    {
        Finish();
        m_threads.clear();
        int current_thread_id = 0;

        if (!Lookup(result.GetResultValue(), "current-thread-id", current_thread_id))
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("no current thread id"), LogPaneLogger::LineType::Error);
            return;
        }

        ResultValue const * threads = result.GetResultValue().GetTupleValue("threads");

        if (!threads || (threads->GetType() != ResultValue::Tuple && threads->GetType() != ResultValue::Array))
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("no threads"), LogPaneLogger::LineType::Error);
            return;
        }

        int iCount = threads->GetTupleSize();

        for (int ii = 0; ii < iCount; ++ii)
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
                    info += wxString::Format(" in %s:%d", str, line);
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
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, "-thread-info", LogPaneLogger::LineType::Debug);
        Execute("-thread-info");
    }

    GenerateCPUInfoRegisters::GenerateCPUInfoRegisters(LogPaneLogger * logger) :
        m_bParsedRegisteryNamesReceived(false),
        m_bParsedRegisteryValuesReceived(false),
        m_logger(logger)
    {
        m_ParsedRegisteryDataReceived.clear();
    }

    void GenerateCPUInfoRegisters::OnCommandOutput(CommandID const & id, ResultParser const & result)
    {
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("FUTURE TO BE CODED!!! id:%s result: - %s", id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Debug);

        //    register-names=
        //    [ "rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp","r8","r9","r10","r11","r12","r13","r14","r15","rip",
        //    "eflags","cs","ss","ds","es","fs","gs","st0","st1","st2","st3","st4","st5","st6","st7","fctrl","fstat",
        //    "ftag","fiseg","fioff","foseg","fooff","fop","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
        //    "xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15","mxcsr",
        //    "","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","",
        //    "","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","",""
        //    ,"","","","","","","","","","","","","","","","","","","","","","","","","","","","","","al","bl","cl",
        //    "dl","sil","dil","bpl","spl","r8l","r9l","r10l","r11l","r12l","r13l","r14l","r15l","ah","bh","ch","dh",
        //    "ax","bx","cx","dx","si","di","bp","","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w","eax","ebx",
        //    "ecx","edx","esi","edi","ebp","esp","r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"]
        if (id == m_reg_name_data_list_request_id)
        {
            ResultValue const * registryNames = result.GetResultValue().GetTupleValue("register-names");
            if (registryNames)
            {
                bool bInsertRegistryData = (m_ParsedRegisteryDataReceived.size() ==  0);

                int iCount = registryNames->GetTupleSize();
                for (int iIndex = 0; iIndex < iCount; iIndex++)
                {
                    const ResultValue * pRegEntry = registryNames->GetTupleValueByIndex(iIndex);
                    if (pRegEntry)
                    {
                        wxString registryName = pRegEntry->GetSimpleValue();
                        if (bInsertRegistryData || (m_ParsedRegisteryDataReceived.find(iIndex) == m_ParsedRegisteryDataReceived.end() ))
                        {
                            RegistryData regData;
                            regData.RegistryName = registryName;
                            regData.RegistryValue = wxEmptyString;
                            m_ParsedRegisteryDataReceived.insert(std::make_pair(iIndex, regData));
                        }
                        else
                        {
                            RegistryData & regData = m_ParsedRegisteryDataReceived[iIndex];
                            regData.RegistryName = registryName;
                        }
                    }
                    else
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Could not parse the register-name index %d. Received id:%s result: - %s", iIndex, id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Error);
                    }
               }
                m_bParsedRegisteryNamesReceived= true;
            }
            else
            {
                m_bParsedRegisteryNamesReceived= false;
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Could not parse the \"-data-list-register-names\" GDB/MI request. Received id:%s result: - %s", id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Error);
            }
        }

        //    register-values=
        //    [
        //    {number="0",value="0xd1845ff7e0"},{number="1",value="0xd1845ff790"},{number="2",value="0xffffff2e7ba00870"},{number="3",value="0x0"},{number="4",value="0x2073692073696854"},
        //    {number="5",value="0x6120726168632061"},{number="6",value="0x1"},{number="7",value="0xd1845ff370"},{number="8",value="0x7efefefefefefeff"},{number="9",value="0x7efefefefefefeff"},
        //    {number="10",value="0x0"},{number="11",value="0x8101010101010100"},{number="12",value="0x10"},{number="13",value="0x1e9a4501710"},{number="14",value="0x0"},{number="15",value="0x0"},
        //    {number="16",value="0x7ff6e6df165d"},{number="17",value="0x202"},{number="18",value="0x33"},{number="19",value="0x2b"},{number="20",value="0x2b"},{number="21",value="0x2b"},
        //    {number="22",value="0x53"},{number="23",value="0x2b"},{number="24",value="0x0"},{number="25",value="0x0"},{number="26",value="0x0"},{number="27",value="0x0"},{number="28",value="0x0"},
        //    {number="29",value="0x0"},{number="30",value="0x0"},{number="31",value="0x0"},{number="32",value="0x37f"},{number="33",value="0x0"},{number="34",value="0x0"},{number="35",value="0x0"},
        //    {number="36",value="0x0"},{number="37",value="0x0"},{number="38",value="0x0"},{number="39",value="0x0"},{number="40",value="{v8_bfloat16 = {0xffff, 0x0, 0xffff, 0xffff, 0x0, 0x0, 0x0, 0xffff},
        //    v4_float = {0x0, 0xffffffff, 0x5c670000, 0xffffffff}, v2_double = {0x7fffffffffffffff, 0x7fffffffffffffff}, v16_int8 = {0x62, 0x69, 0x6e, 0x5c, 0x44, 0x65, 0x62, 0x75, 0x67, 0x5c,
        //    0x4d, 0x53, 0x59, 0x53, 0x32, 0x5f}, v8_int16 = {0x6962, 0x5c6e, 0x6544, 0x7562, 0x5c67, 0x534d, 0x5359, 0x5f32}, v4_int32 = {0x5c6e6962, 0x75626544, 0x534d5c67, 0x5f325359},
        //    v2_int64 = {0x756265445c6e6962, 0x5f325359534d5c67}, uint128 = 0x5f325359534d5c67756265445c6e6962}"},{number="41",value="{v8_bfloat16 = {0x0, 0x0, 0x0, 0xffff, 0xffff, 0x0, 0xffff, 0x0},
        //    v4_float = {0x0, 0xffffffff, 0x0, 0x0}, v2_double = {0x7fffffffffffffff, 0x0}, v16_int8 = {0x53, 0x59, 0x53, 0x32, 0x5f, 0x50, 0x72, 0x69, 0x6e, 0x74, 0x66, 0x2e, 0x65, 0x78, 0x65, 0x0},
        //    v8_int16 = {0x5953, 0x3253, 0x505f, 0x6972, 0x746e, 0x2e66, 0x7865, 0x65}, v4_int32 = {0x32535953, 0x6972505f, 0x2e66746e, 0x657865}, v2_int64 = {0x6972505f32535953, 0x6578652e66746e},
        //    uint128 = 0x6578652e66746e6972505f32535953}"},{number="42",value="{v8_bfloat16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0},
        //    v16_int8 = {0x0 <repeats 16 times>}, v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x0}"},
        //    {number="43",value="{v8_bfloat16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>},
        //    v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x0}"},{number="44",
        //    value="{v8_bfloat16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>},
        //    v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x0}"},{number="45",
        //    value="{v8_bfloat16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>},
        //    v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x0}"},{number="46",
        //    value="{v8_bfloat16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_float = {0x0, 0x0, 0x0, 0x0}, v2_double = {0x0, 0x0}, v16_int8 = {0x0 <repeats 16 times>},
        //    v8_int16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, v4_int32 = {0x0, 0x0, 0x0, 0x0}, v2_int64 = {0x0, 0x0}, uint128 = 0x0}"},{number="47",value="{v8_bfloat16 =
        //    ..... BLOCK OF ENTRIES REMOVED ...
        //    {number="201",value="0x1010100"},{number="202",value="0x10"},{number="203",value="0xa4501710"},{number="204",value="0x0"},{number="205",value="0x0"}]<==
        if (id == m_reg_value_data_list_request_id)
        {
            ResultValue const * registryValues = result.GetResultValue().GetTupleValue("register-values");
            if (registryValues)
            {
                bool bInsertRegistryData = (m_ParsedRegisteryDataReceived.size() ==  0);
                int iCount = registryValues->GetTupleSize();
                for (int iIndex = 0; iIndex < iCount; iIndex++)
                {
                    const ResultValue * pRegEntry = registryValues->GetTupleValueByIndex(iIndex);
                    if (pRegEntry)
                    {
                        const ResultValue * pRegValueIndex = pRegEntry->GetTupleValue("number");
                        const ResultValue * pRegValueData = pRegEntry->GetTupleValue("value");
                        if (pRegValueIndex && pRegValueData)
                        {
                            wxString registryIndex = pRegValueIndex->GetSimpleValue();
                            wxString registryValue = pRegValueData->GetSimpleValue();

                            long lregistryIndex;
                            if (registryIndex.ToLong(&lregistryIndex, 10))
                            {
                                if (bInsertRegistryData || (m_ParsedRegisteryDataReceived.find(lregistryIndex) == m_ParsedRegisteryDataReceived.end() ))
                                {
                                    RegistryData regData;
                                    regData.RegistryName = wxEmptyString;
                                    regData.RegistryValue = registryValue;
                                    m_ParsedRegisteryDataReceived.insert(std::make_pair(lregistryIndex, regData));
                                }
                                else
                                {
                                    RegistryData & regData = m_ParsedRegisteryDataReceived[lregistryIndex];
                                    regData.RegistryValue = registryValue;
                                }
                            }
                            else
                            {
                                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Could not parse the register value index at index %d. Entry result: - %s", iIndex, pRegValueIndex->MakeDebugString()), LogPaneLogger::LineType::Error);
                            }
                        }
                        else
                        {
                            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Could not parse the register value at index %d. Entry result: - %s", iIndex, pRegEntry->MakeDebugString()), LogPaneLogger::LineType::Error);
                        }
                    }
                    else
                    {
                        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Could not parse the register-value index %d. Received id:%s result: - %s", iIndex, id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Error);
                    }
                }
                // Failures above with parsing will reset startup values later
                m_bParsedRegisteryValuesReceived = true;
            }
            else
            {
                m_bParsedRegisteryValuesReceived = false;
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Could not parse the \"-data-list-register-values x\" GDB/MI request. Received id:%s result: - %s", id.ToString(), result.MakeDebugString()), LogPaneLogger::LineType::Error);
            }
        }
        // Just in case GDB returns the results in a different order
        if (m_bParsedRegisteryNamesReceived && m_bParsedRegisteryValuesReceived)
        {
            cbCPURegistersDlg * dialog = Manager::Get()->GetDebuggerManager()->GetCPURegistersDialog();

            /* iterate thru map */
            for (std::map<long, struct RegistryData>::iterator it = m_ParsedRegisteryDataReceived.begin();it != m_ParsedRegisteryDataReceived.end(); it++)
            {
                wxString registryName = it->second.RegistryName;
                wxString registryValue = it->second.RegistryValue;
                if (!registryName.IsEmpty() && !registryValue.IsEmpty())
                {
                    dialog->SetRegisterValue(registryName, registryValue, wxEmptyString);
                }
            }

            // Reset values as they have been processed
            m_bParsedRegisteryNamesReceived = false;
            m_bParsedRegisteryValuesReceived= false;
            m_ParsedRegisteryDataReceived.clear();

            Finish();
        }
    }

    void GenerateCPUInfoRegisters::OnStart()
    {
        // Do not use "info registers" with GDB/MI, but
        // On GDB/MI use "-data-list-register-names" and "-data-list-register-values x" - taken from CodeLite src
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, "-data-list-register-names and -data-list-register-values x", LogPaneLogger::LineType::Debug);
        m_bParsedRegisteryNamesReceived = false;
        m_bParsedRegisteryValuesReceived= false;
        m_ParsedRegisteryDataReceived.clear();
        m_reg_name_data_list_request_id = Execute("-data-list-register-names");
        m_reg_value_data_list_request_id = Execute("-data-list-register-values x");
    }

    GenerateExamineMemory::GenerateExamineMemory(LogPaneLogger * logger) :
        m_logger(logger)
    {
        cbExamineMemoryDlg * dialog = Manager::Get()->GetDebuggerManager()->GetExamineMemoryDialog();
        m_address = dialog->GetBaseAddress();
        m_length = dialog->GetBytes();
    }

    void GenerateExamineMemory::OnCommandOutput(CommandID const & id, ResultParser const & result)
    {
        Finish();
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("result: - %s", result.MakeDebugString()), LogPaneLogger::LineType::Debug);
    }

    void GenerateExamineMemory::OnStart()
    {
        const wxString cmd = wxString::Format("x/%dxb %s", m_length, m_address);
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("%s", cmd), LogPaneLogger::LineType::Debug);
        Execute(cmd);
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
        m_sub_commands_left(0)
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
                                wxString::Format(_("WatchBaseAction::ParseListCommand - steplistchildren for id: %s ==>%s>=="), id.ToString(), value.MakeDebugString()),
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
            {
                if (strDisplayHint == "array")
                {
                    displayHint = DisplayHint::Array;
                }
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
                                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
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
        int iStart = watch->GetRangeStart();
        int iEnd = watch->GetRangeEnd();

        if ((iStart > -1) && (iEnd > -1))
        {
            id = Execute(wxString::Format("-var-list-children 2 \"%s\" %d %d ", watch->GetID(), iStart, iEnd));
        }
        else
        {
            id = Execute(wxString::Format("-var-list-children 2 \"%s\"", watch->GetID()));
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
        int iStart = parent->GetRangeStart();
        int iEnd = parent->GetRangeEnd();

        if ((iStart > -1) && (iEnd > -1))
        {
            id = Execute(wxString::Format("-var-list-children 2 \"%s\" %d %d ", watch_id, iStart, iEnd));
        }
        else
        {
            id = Execute(wxString::Format("-var-list-children 2 \"%s\"", watch_id));
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
        bool error = false;
        wxString resultDebug = result.MakeDebugString();

        if (result.GetResultClass() == ResultParser::ClassDone)
        {
            ResultValue const & value = result.GetResultValue();

            switch (m_step)
            {
                case StepCreate:
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("StepCreate for ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Debug);
                    bool dynamic, has_more;
                    int children;
                    ParseWatchInfo(value, children, dynamic, has_more);
                    ParseWatchValueID(*m_watch, value);

                    if (dynamic && has_more)
                    {
                        m_step = StepSetRange;
#warning need to test this code!!!
                        Execute("-var-set-update-range \"" + m_watch->GetID() + "\" 0 100");
                        AppendNullChild(m_watch);
                    }
                    else
                    {
                        if (children > 0)
                        {
                            if (children > 1)
                            {
                                m_watch->SetRange(0, children);
                            }

                            m_step = StepListChildren;
                            AppendNullChild(m_watch);
                        }
                        else
                        {
                            Finish();
                        }
                    }
                }
                break;

                case StepListChildren:
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("StepListChildren for ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Debug);
                    error = !ParseListCommand(id, value);
                    break;

                case StepSetRange:
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("StepSetRange for ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Debug);
#warning code to be added for this case
                    break;

                default:
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("m_step unknown for ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Error);
                    break;
            }
        }
        else
        {
            if (result.GetResultClass() == ResultParser::ClassError)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("The expression can't be evaluated! ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Debug);
                m_watch->SetValue("The expression can't be evaluated");
            }
            else
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("processing command ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Debug);
            }

            error = true;
        }

        if (error)
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("Command ID: %s ==>%s<=="), id.ToString(), resultDebug), LogPaneLogger::LineType::Error);
            UpdateWatches(m_logger);
            Finish();
        }
        else
        {
            if (m_sub_commands_left == 0)
            {
                m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                        __LINE__,
                                        wxString::Format(_("Finished sub commands ID: %s"),  id.ToString()),
                                        LogPaneLogger::LineType::Debug);
                UpdateWatches(m_logger);
                Finish();
            }
        }
    }

    void WatchCreateAction::OnStart()
    {
        wxString symbol;
        m_watch->GetSymbol(symbol);
        symbol.Replace("\"", "\\\"");
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("Watch variable: \"%s\"", symbol), LogPaneLogger::LineType::UserDisplay);
        Execute(wxString::Format("-var-create - @ \"%s\"", symbol));
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
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, "-var-update 1 *", LogPaneLogger::LineType::Debug);
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
        wxString resultDebug = result.MakeDebugString();

        if (list)
        {
            int count = list->GetTupleSize();
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("List count: %d   , result: ==>%s<=="), count, resultDebug), LogPaneLogger::LineType::Debug);

            for (int ii = 0; ii < count; ++ii)
            {
                ResultValue const * value = list->GetTupleValueByIndex(ii);
                wxString expression;

                if (!Lookup(*value, "name", expression))
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                            __LINE__,
                                            wxString::Format(_("no name in ==>%s<=="), value->MakeDebugString()),
                                            LogPaneLogger::LineType::Debug);
                    continue;
                }

                cb::shared_ptr<Watch> watch = FindWatch(expression, m_watches);

                if (!watch)
                {
                    m_logger->LogGDBMsgType(__PRETTY_FUNCTION__,
                                            __LINE__,
                                            wxString::Format(_("can't find watch ==>%s<<=="), expression),
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
                                                                    wxString::Format(_("unhandled dynamic variable ==>%s<=="), updated_var.MakeDebugString()),
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
                                                            wxString::Format(_("Update ==>%s<<== = ==>%s<<=="),  expression, updated_var.GetValue()),
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
        else
        {
            m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format(_("No list. result: ==>%s<=="), resultDebug), LogPaneLogger::LineType::Debug);
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
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("-var-update %s", m_watch->GetID()), LogPaneLogger::LineType::Debug);
        m_update_id = Execute(wxString::Format("-var-update %s", m_watch->GetID()));
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
        m_logger->LogGDBMsgType(__PRETTY_FUNCTION__, __LINE__, wxString::Format("-var-delete -c  ", m_collapsed_watch->GetID()), LogPaneLogger::LineType::Debug);
        Execute(wxString::Format("-var-delete -c  ", m_collapsed_watch->GetID()));
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
