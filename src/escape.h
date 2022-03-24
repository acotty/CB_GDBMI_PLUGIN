/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#ifndef _DEBUGGER_MI_GDB_ESCAPE_H_
#define _DEBUGGER_MI_GDB_ESCAPE_H_

#include <wx/string.h>

namespace dbg_mi
{

wxString EscapePath(wxString const & path);
void ConvertDirectory(wxString & str, wxString base, bool relative);

} // namespace dbg_mi

#endif // _DEBUGGER_MI_GDB_ESCAPE_H_
