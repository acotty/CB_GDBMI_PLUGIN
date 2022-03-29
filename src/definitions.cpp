/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "definitions.h"
#include <wx/version.h>

namespace dbg_mi
{
    void GDBBreakpoint::SetEnabled(bool flag)
    {
        m_enabled = flag;
    }

    wxString GDBBreakpoint::GetLocation() const
    {
        return m_filename;
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
        return _("Code");
    }

    wxString GDBBreakpoint::GetInfo() const
    {
        return wxEmptyString;
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


    GDBMemoryRangeWatch::GDBMemoryRangeWatch(uint64_t address, uint64_t size, const wxString& symbol) :
            m_address(address),
            m_size(size),
            m_symbol(symbol)
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
        wxString sAddress;
        uint64_t llAddress = GetAddress();
#if wxCHECK_VERSION(3, 1, 5)
        if (wxPlatformInfo::Get().GetBitness() == wxBITNESS_64)
#else
        if (wxPlatformInfo::Get().GetArchitecture() == wxARCH_64)
#endif
        {
            sAddress = wxString::Format("%#018llx", llAddress); // 18 = 0x + 16 digits
        }
        else
        {
            sAddress = wxString::Format("%#10llx", llAddress); // 10 = 0x + 8 digits
        }

        return sAddress;
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


} // namespace dbg_mi
