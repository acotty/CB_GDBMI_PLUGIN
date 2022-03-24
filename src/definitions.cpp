/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "definitions.h"

namespace dbg_mi
{
void Breakpoint::SetEnabled(bool flag)
{
}

wxString Breakpoint::GetLocation() const
{
    return m_filename;
}

int Breakpoint::GetLine() const
{
    return m_line;
}

wxString Breakpoint::GetLineString() const
{
    return wxString::Format("%d", m_line);
}

wxString Breakpoint::GetType() const
{
    return _("Code");
}

wxString Breakpoint::GetInfo() const
{
    return wxEmptyString;
}

bool Breakpoint::IsEnabled() const
{
    return m_enabled;
}

bool Breakpoint::IsVisibleInEditor() const
{
    return true;
}

bool Breakpoint::IsTemporary() const
{
    return m_temporary;
}

cb::shared_ptr<Watch> FindWatch(wxString const & expression, WatchesContainer & watches)
{
    for (WatchesContainer::iterator it = watches.begin(); it != watches.end(); ++it)
    {
        if (expression.StartsWith(it->get()->GetID()))
        {
            if (expression.length() == it->get()->GetID().length())
            {
                return *it;
            }
            else
            {
                cb::shared_ptr<Watch> curr = *it;

                while (curr)
                {
                    cb::shared_ptr<Watch> temp = curr;
                    curr = cb::shared_ptr<Watch>();

                    for (int child = 0; child < temp->GetChildCount(); ++child)
                    {
                        cb::shared_ptr<Watch> p = cb::static_pointer_cast<Watch>(temp->GetChild(child));

                        if (expression.StartsWith(p->GetID()))
                        {
                            if (expression.length() == p->GetID().length())
                            {
                                return p;
                            }
                            else
                            {
                                curr = p;
                                break;
                            }
                        }
                    }
                }

                if (curr)
                {
                    return curr;
                }
            }
        }
    }

    return cb::shared_ptr<Watch>();
}

} // namespace dbg_mi
