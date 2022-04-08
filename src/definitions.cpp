/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

// System include files
#include <tinyxml2.h>
#include <wx/version.h>

// CB include files (not GDB)
#include "cbproject.h"
#include "cbeditor.h"
#include "editormanager.h"
#include "manager.h"
#include "cbdebugger_interfaces.h"

// GDB include files
#include "definitions.h"
#include "plugin.h"

namespace dbg_mi
{
    wxString BoolTowxString(bool value)
    {
        if (value)
        {
            return ("True");
        }
        else
        {
            return ("False");
        }
    }

    bool wxStringToBool(wxString value)
    {
        if (value.IsSameAs("True", false))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void AddChildNode(tinyxml2::XMLNode* pNodeParent,  const wxString name, const wxString value)
    {
        tinyxml2::XMLDocument* pDoc = pNodeParent->GetDocument();
		tinyxml2::XMLElement* pNewXMLElement = pDoc->NewElement(name);
		pNewXMLElement->SetText(value.mb_str());

        pNodeParent->InsertEndChild(pNewXMLElement);
    }

    void AddChildNode(tinyxml2::XMLNode* pNodeParent,  const wxString name, const int iValue)
    {
        wxString value = wxString::Format("%d", iValue);
        AddChildNode(pNodeParent, name, value);
    }

    void AddChildNode(tinyxml2::XMLNode* pNodeParent,  const wxString name, const long lValue)
    {
        wxString value = wxString::Format("%ld", lValue);
        AddChildNode(pNodeParent,  name, value);
    }

    void AddChildNode(tinyxml2::XMLNode* pNodeParent,  const wxString name, const uint64_t llValue)
    {
        wxString value = wxString::Format("%llu", llValue);
        AddChildNode(pNodeParent,  name, value);
    }

    void AddChildNodeHex(tinyxml2::XMLNode* pNodeParent,  const wxString name, const uint64_t llValue)
    {
        wxString value;
    #if wxCHECK_VERSION(3, 1, 5)
        if (wxPlatformInfo::Get().GetBitness() == wxBITNESS_64)
    #else
        if (wxPlatformInfo::Get().GetArchitecture() == wxARCH_64)
    #endif
        {
            value = wxString::Format("%#018llx", llValue);// 18 = 0x + 16 digits
        }
        else
        {
            value =  wxString::Format("%#10llx", llValue);// 10 = 0x + 8 digit
        }
        AddChildNode(pNodeParent, name, value);
    }

    void AddChildNode(tinyxml2::XMLNode* pNodeParent,  const wxString name, const bool bValue)
    {
        AddChildNode(pNodeParent, name, BoolTowxString(bValue));
    }

    wxString ReadChildNodewxString(tinyxml2::XMLElement* pElementParent,  const wxString childName)
    {
        tinyxml2::XMLElement* pElementChild = pElementParent->FirstChildElement(childName);

        if (pElementChild)
        {
            return pElementChild->GetText();
        }

        return wxEmptyString;
    }

    int ReadChildNodeInt(tinyxml2::XMLElement* pElementParent,  const wxString childName)
    {
        wxString value = ReadChildNodewxString(pElementParent, childName);
        long result;
        value.ToLong(&result, 10);
        return result;
    }

    long ReadChildNodeLong(tinyxml2::XMLElement* pElementParent,  const wxString childName)
    {
        wxString value = ReadChildNodewxString(pElementParent, childName);
        long result;
        value.ToLong(&result, 10);
        return result;
    }

    uint64_t ReadChildNodeUint64(tinyxml2::XMLElement* pElementParent,  const wxString childName)
    {
        wxString value = ReadChildNodewxString(pElementParent, childName);
        uint64_t result;
        value.ToULongLong(&result, 10);
        return result;
    }

    uint64_t ReadChildNodeHex(tinyxml2::XMLElement* pElementParent,  const wxString childName)
    {
        wxString value = ReadChildNodewxString(pElementParent, childName);
        uint64_t result;
        value.ToULongLong(&result, 16);
        return result;
    }

    bool ReadChildNodeBool(tinyxml2::XMLElement* pElementParent,  const wxString childName)
    {
        return wxStringToBool(ReadChildNodewxString(pElementParent, childName));
    }

    void GDBBreakpoint::SaveBreakpointToXML(tinyxml2::XMLNode* pNodeParent)
    {
        tinyxml2::XMLDocument* pDoc = pNodeParent->GetDocument();
        tinyxml2::XMLElement* pNewXMLElement = pDoc->NewElement("Breakpoint");
        tinyxml2::XMLNode* pNodeBreakpoint = pNodeParent->InsertEndChild(pNewXMLElement);

        //Change the absolute location to relative location
        wxFileName location(GetFilename());
        location.MakeRelativeTo(m_project->GetBasePath());

        AddChildNode(pNodeBreakpoint, "type", GetTypewxString());               // The type of this breakpoint.

        AddChildNode(pNodeBreakpoint, "projectTitle", m_project->GetTitle());   // The Project the file belongs to.
        AddChildNode(pNodeBreakpoint, "filename", m_filename);                  // The filename for the breakpoint.
        AddChildNode(pNodeBreakpoint, "line", m_line);                          // The line for the breakpoint.

        AddChildNode(pNodeBreakpoint, "index", m_index);                        // The breakpoint number. Set automatically. *Don't* write to it.
        AddChildNode(pNodeBreakpoint, "temporary", m_temporary);                // Is this a temporary (one-shot) breakpoint?
        AddChildNode(pNodeBreakpoint, "enabled", m_enabled);                    // Is the breakpoint enabled?
        AddChildNode(pNodeBreakpoint, "active", m_active);                      // Is the breakpoint active? (currently unused)
        AddChildNode(pNodeBreakpoint, "useIgnoreCount", m_useIgnoreCount);      // Should this breakpoint be ignored for the first X passes? (@c x == @c ignoreCount)
        AddChildNode(pNodeBreakpoint, "ignoreCount", m_ignoreCount);            // The number of passes before this breakpoint should hit. @c useIgnoreCount must be true.
        AddChildNode(pNodeBreakpoint, "useCondition", m_useCondition);          // Should this breakpoint hit only if a specific condition is met?
        AddChildNode(pNodeBreakpoint, "wantsCondition", m_wantsCondition);      // Evaluate condition for pending breakpoints at first stop !
        AddChildNode(pNodeBreakpoint, "condition", m_condition);                // The condition that must be met for the breakpoint to hit. @c useCondition must be true.
        AddChildNode(pNodeBreakpoint, "function", m_function);                  // The function to set the breakpoint. If this is set, it is preferred over the filename/line combination.
        AddChildNodeHex(pNodeBreakpoint, "address", m_address);                    // The actual breakpoint address. This is read back from the debugger. *Don't* write to it.
        AddChildNode(pNodeBreakpoint, "alreadySet", m_alreadySet);              // Is this already set? Used to mark temporary breakpoints for removal.
        AddChildNode(pNodeBreakpoint, "lineText", m_lineText);                  // Optionally, the breakpoint line's text (used by GDB for setting breapoints on ctors/dtors).
        AddChildNode(pNodeBreakpoint, "breakAddress", m_breakAddress);          // Valid only for type==bptData: address to break when read/written.
        AddChildNode(pNodeBreakpoint, "breakOnRead", m_breakOnRead);            // Valid only for type==bptData: break when memory is read from.
        AddChildNode(pNodeBreakpoint, "breakOnWrite", m_breakOnWrite);          // Valid only for type==bptData: break when memory is written to.
    }

    void GDBBreakpoint::LoadBreakpointFromXML(tinyxml2::XMLElement* pElementBreakpoint, Debugger_GDB_MI * dbgGDB)
    {
        //Only load the breakpoints that belong to the current project
        SetType(ReadChildNodewxString(pElementBreakpoint, "type"));                    // The type of this breakpoint.

        m_filename = ReadChildNodewxString(pElementBreakpoint, "filename");            // The filename for the breakpoint.
        m_line = ReadChildNodeInt(pElementBreakpoint, "line");                         // The line for the breakpoint.

        m_index = ReadChildNodeLong(pElementBreakpoint, "index");                      // The breakpoint number. Set automatically. *Don't* write to it.
        m_temporary = ReadChildNodeBool(pElementBreakpoint, "temporary");              // Is this a temporary (one-shot) breakpoint?
        m_enabled = ReadChildNodeBool(pElementBreakpoint, "enabled");                  // Is the breakpoint enabled?
        m_active = ReadChildNodeBool(pElementBreakpoint, "active");                    // Is the breakpoint active? (currently unused)
        m_useIgnoreCount = ReadChildNodeBool(pElementBreakpoint, "useIgnoreCount");    // Should this breakpoint be ignored for the first X passes? (@c x == @c ignoreCount)
        m_ignoreCount = ReadChildNodeInt(pElementBreakpoint, "ignoreCount");           // The number of passes before this breakpoint should hit. @c useIgnoreCount must be true.
        m_useCondition = ReadChildNodeBool(pElementBreakpoint, "useCondition");        // Should this breakpoint hit only if a specific condition is met?
        m_wantsCondition = ReadChildNodeBool(pElementBreakpoint, "wantsCondition");    // Evaluate condition for pending breakpoints at first stop !
        m_condition = ReadChildNodewxString(pElementBreakpoint, "condition");          // The condition that must be met for the breakpoint to hit. @c useCondition must be true.
        m_function = ReadChildNodewxString(pElementBreakpoint, "function");            // The function to set the breakpoint. If this is set, it is preferred over the filename/line combination.
        m_address = ReadChildNodeHex(pElementBreakpoint, "address");                   // The actual breakpoint address. This is read back from the debugger. *Don't* write to it.
        m_alreadySet = ReadChildNodeBool(pElementBreakpoint, "alreadySet");            // Is this already set? Used to mark temporary breakpoints for removal.
        m_lineText = ReadChildNodewxString(pElementBreakpoint, "lineText");            // Optionally, the breakpoint line's text (used by GDB for setting breapoints on ctors/dtors).
        m_breakAddress = ReadChildNodewxString(pElementBreakpoint, "breakAddress");    // Valid only for type==bptData: address to break when read/written.
        m_breakOnRead = ReadChildNodeBool(pElementBreakpoint, "breakOnRead");          // Valid only for type==bptData: break when memory is read from.
        m_breakOnWrite = ReadChildNodeBool(pElementBreakpoint, "breakOnWrite");        // Valid only for type==bptData: break when memory is written to.

        cbEditor* ed = Manager::Get()->GetEditorManager()->IsBuiltinOpen( m_filename );
        if (ed == nullptr)
        {
            dbgGDB->AddBreakpoint(m_filename, m_line);
        }
        else
        {
            ed->AddBreakpoint(m_line-1, true);
        }
    }

    void GDBBreakpoint::SetEnabled(bool flag)
    {
        m_enabled = flag;
    }

    wxString GDBBreakpoint::GetLocation() const
    {
        switch(m_type)
        {
            case BreakpointType::bptCode:
                return m_filename;

            case BreakpointType::bptFunction:
                return m_filename;

            case BreakpointType::bptData:
                return m_breakAddress;

            default:
                return "unknown";
        }
    }

    int GDBBreakpoint::GetLine() const
    {
        return m_line;
    }

    wxString GDBBreakpoint::GetLineString() const
    {
        return wxString::Format("%d", m_line);
    }

    wxString GDBBreakpoint::GetType() const
    {
        switch(m_type)
        {
            case BreakpointType::bptCode:
                return "bptCode";

            case BreakpointType::bptFunction:
                return "bptFunction";

            case BreakpointType::bptData:
                return "bptData";

            default:
                return "unknown";
        }
    }

    wxString GDBBreakpoint::GetInfo() const
    {
        switch(m_type)
        {
            case BreakpointType::bptCode:
                return wxEmptyString;

            case BreakpointType::bptFunction:
                return wxEmptyString;

            case BreakpointType::bptData:
                {
                    if (m_breakOnRead)
                    {
                        if (m_breakOnWrite)
                        {
                            return "read and write";
                        }
                        else
                        {
                            return "read only";
                        }
                    }
                    else
                    {
                        return "write only";
                    }
                }

            default:
                return "unknown";
        }
    }

    bool GDBBreakpoint::IsEnabled() const
    {
        return m_enabled;
    }

    bool GDBBreakpoint::IsVisibleInEditor() const
    {
        return true;
    }

    bool GDBBreakpoint::IsTemporary() const
    {
        return m_temporary;
    }

    cb::shared_ptr<GDBWatch> FindWatch(wxString const & expression, GDBWatchesContainer & watches)
    {
        size_t expLength = expression.length();

        for (GDBWatchesContainer::iterator it = watches.begin(); it != watches.end(); ++it)
        {
            if (expression.StartsWith(it->get()->GetID()))
            {
                if (expLength == it->get()->GetID().length())
                {
                    return *it;
                }
                else
                {
                    cb::shared_ptr<GDBWatch> curr = *it;

                    while (curr)
                    {
                        cb::shared_ptr<GDBWatch> temp = curr;
                        curr = cb::shared_ptr<GDBWatch>();

                        for (int child = 0; child < temp->GetChildCount(); ++child)
                        {
                            cb::shared_ptr<GDBWatch> p = cb::static_pointer_cast<GDBWatch>(temp->GetChild(child));
                            wxString id = p->GetID();

                            if (expression.StartsWith(id))
                            {
                                if (expLength == id.length())
                                {
                                    return p;
                                }
                                else
                                {
                                    if ((expLength > id.length()) && (expression[id.length()] == '.'))
                                    {
                                        // Go into sub child
                                        curr = p;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return cb::shared_ptr<GDBWatch>();
    }

    wxString GDBWatch::GetWatchFormatTowxString()
    {
        switch(m_format)
        {
            case Undefined:         // Format is undefined (whatever the debugger uses by default).
                return "Undefined";
            case Decimal:           // Variable should be displayed as decimal.
                return "Decimal";
            case Unsigned:          // Variable should be displayed as unsigned.
                return "Unsigned";
            case Hex:               // Variable should be displayed as hexadecimal (e.g. 0xFFFFFFFF).
                return "Hex";
            case Binary:            // Variable should be displayed as binary (e.g. 00011001).
                return "Binary";
            case Char:              // Variable should be displayed as a single character (e.g. 'x').
                return "Char";
            case Float:             // Variable should be displayed as floating point number (e.g. 14.35)
                return "Float";
            case Last:              // used for iterations
                return "Last";
            case Any:               // used for watches searches
                return "Any";
            default:
                return "unknown";
        };
    }

    GDBWatch::WatchFormat GDBWatch::GetWatchFormatFromwxString(wxString wFormat)
    {
        if (wFormat.IsSameAs("Undefined", false))         // Format is undefined (whatever the debugger uses by default).
        {
            return WatchFormat::Undefined;
        }

        if (wFormat.IsSameAs("Decimal", false))          // Variable should be displayed as decimal.
        {
            return WatchFormat::Decimal;
        }

        if (wFormat.IsSameAs("Unsigned", false))          // Variable should be displayed as unsigned.
        {
            return WatchFormat::Unsigned;
        }

        if (wFormat.IsSameAs("Hex", false))               // Variable should be displayed as hexadecimal (e.g. 0xFFFFFFFF).
        {
            return WatchFormat::Hex;
        }

        if (wFormat.IsSameAs("Binary", false))            // Variable should be displayed as binary (e.g. 00011001).
        {
            return WatchFormat::Binary;
        }

        if (wFormat.IsSameAs("Char", false))              // Variable should be displayed as a single character (e.g. 'x').
        {
            return WatchFormat::Char;
        }

        if (wFormat.IsSameAs("Float", false))            // Variable should be displayed as floating point number (e.g. 14.35)
        {
            return WatchFormat::Float;
        }

        if (wFormat.IsSameAs("Last", false))             // used for iterations
        {
            return WatchFormat::Last;
        }

        if (wFormat.IsSameAs("Any", false))               // used for watches searches
        {
            return WatchFormat::Any;
        }

        return WatchFormat::Undefined;
    }

    void GDBWatch::SaveWatchToXML(tinyxml2::XMLNode* pWatchesMasterNode)
    {
        tinyxml2::XMLDocument* pDoc = pWatchesMasterNode->GetDocument();
        tinyxml2::XMLElement* pNewXMLElement = pDoc->NewElement("Watch");
        tinyxml2::XMLNode* pNodeWatch = pWatchesMasterNode->InsertEndChild(pNewXMLElement);

        AddChildNode(pNodeWatch, "GDBWatchClassName", m_GDBWatchClassName);
        AddChildNode(pNodeWatch, "projectTitle", m_project->GetTitle());   // The Project the file belongs to.
        // AddChildNode(pNodeWatch, "id", m_id);
        AddChildNode(pNodeWatch, "symbol", m_symbol);
        // AddChildNode(pNodeWatch, "value", m_value);
        AddChildNode(pNodeWatch, "type", m_type);
        AddChildNode(pNodeWatch, "format", GetWatchFormatTowxString());

        AddChildNode(pNodeWatch, "debug_string", m_debug_string);
        AddChildNode(pNodeWatch, "has_been_expanded", m_has_been_expanded);
        AddChildNode(pNodeWatch, "for_tooltip", m_for_tooltip);
        AddChildNode(pNodeWatch, "delete_on_collapse", m_delete_on_collapse);

        AddChildNode(pNodeWatch, "array_start", m_array_start);
        AddChildNode(pNodeWatch, "array_end", m_array_end);
        AddChildNode(pNodeWatch, "is_array", m_is_array);
        AddChildNode(pNodeWatch, "forTooltip", m_forTooltip);
    }

    void GDBWatch::LoadWatchFromXML(tinyxml2::XMLElement* pElementWatch, Debugger_GDB_MI * dbgGDB)
    {
        //Only load the breakpoints that belong to the current project
        m_GDBWatchClassName = ReadChildNodewxString(pElementWatch, "GDBWatchClassName");

        m_id = ReadChildNodewxString(pElementWatch, "id");
        m_symbol = ReadChildNodewxString(pElementWatch, "symbol");
        m_value = ReadChildNodewxString(pElementWatch, "value");
        m_type = ReadChildNodewxString(pElementWatch, "type");
        SetFormat(ReadChildNodewxString(pElementWatch, "format"));

        m_debug_string = ReadChildNodewxString(pElementWatch, "debug_string");
        m_has_been_expanded = ReadChildNodeBool(pElementWatch, "has_been_expanded");
        m_for_tooltip = ReadChildNodeBool(pElementWatch, "for_tooltip");
        m_delete_on_collapse = ReadChildNodeBool(pElementWatch, "delete_on_collapse");

        m_array_start = ReadChildNodeInt(pElementWatch, "array_start");
        m_array_end = ReadChildNodeLong(pElementWatch, "array_end");
        m_is_array = ReadChildNodeLong(pElementWatch, "is_array");
        m_forTooltip = ReadChildNodeBool(pElementWatch, "forTooltip");

        if (!m_symbol.IsEmpty())
        {
            // See debuggermenu.cpp DebuggerMenuHandler::OnAddWatch(...) function
            cb::shared_ptr<cbWatch> watch = dbgGDB->AddWatch(this, true);
            cbWatchesDlg *dialog = Manager::Get()->GetDebuggerManager()->GetWatchesDialog();
            dialog->AddWatch(watch);   // This call adds the watch to the debugger and GUI
        }
    }

    GDBMemoryRangeWatch::GDBMemoryRangeWatch(cbProject * project, dbg_mi::LogPaneLogger * logger, uint64_t address, uint64_t size, const wxString &symbol) :
                m_project(project),
                m_pLogger(logger),
                m_GDBWatchClassName("GDBMemoryRangeWatch"),
                m_address(address),
                m_size(size),
                m_symbol(symbol),
                m_value(wxEmptyString)
    {
    }

    bool GDBMemoryRangeWatch::SetValue(const wxString &value)
    {
        if (m_value != value)
        {
            m_value = value;
            MarkAsChanged(true);
        }
        return true;
    }

    wxString GDBMemoryRangeWatch::MakeSymbolToAddress() const
    {
        return wxString::Format("&%s", m_symbol);
    };

    // Use this function to sanitize user input which might end as the last part of GDB commands.
    // If the last character is '\', GDB will treat it as line continuation and it will stall.
    wxString CleanStringValue(wxString value)
    {
        while (value.EndsWith("\\"))
        {
            value.RemoveLast();
        }
        return value;
    }

    void GDBMemoryRangeWatch::SaveWatchToXML(tinyxml2::XMLNode* pMemoryRangeMasterNode)
    {
        tinyxml2::XMLDocument* pDoc = pMemoryRangeMasterNode->GetDocument();
        tinyxml2::XMLElement* pNewXMLElement = pDoc->NewElement("MemoryRangeWatch");
        tinyxml2::XMLNode* pNodeMemoryRange= pMemoryRangeMasterNode->InsertEndChild(pNewXMLElement);

        AddChildNode(pNodeMemoryRange, "GDBMemoryRangeWatch", m_GDBWatchClassName);
        AddChildNode(pNodeMemoryRange, "projectTitle", m_project->GetTitle());   // The Project the file belongs to.

        AddChildNode(pNodeMemoryRange, "address", m_address);
        AddChildNode(pNodeMemoryRange, "size", m_size);
        AddChildNode(pNodeMemoryRange, "symbol", m_symbol);
    }

    void GDBMemoryRangeWatch::LoadWatchFromXML(tinyxml2::XMLElement* pElementWatch, Debugger_GDB_MI * dbgGDB)
    {
        //Only load the breakpoints that belong to the current project
        m_GDBWatchClassName = ReadChildNodewxString(pElementWatch, "GDBMemoryRangeWatch");

        m_address = ReadChildNodeUint64(pElementWatch, "address");
        m_size = ReadChildNodeUint64(pElementWatch, "size");
        m_symbol = ReadChildNodewxString(pElementWatch, "symbol");

        if (!m_symbol.IsEmpty())
        {
#if 0
            // See debuggermenu.cpp DebuggerMenuHandler::OnAddWatch(...) function
            cb::shared_ptr<cbWatch> watch = dbgGDB->AddWatch(this, true);
            cbWatchesDlg *dialog = Manager::Get()->GetDebuggerManager()->GetWatchesDialog();
            dialog->AddWatch(watch);   // This call adds the watch to the debugger and GUI
#endif
        }
    }

} // namespace dbg_mi
