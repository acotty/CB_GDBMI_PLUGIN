/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#ifndef _DEBUGGER_GDB_MI_CMD_RESULT_TOKENS_H_
#define _DEBUGGER_GDB_MI_CMD_RESULT_TOKENS_H_

#include <wx/string.h>

namespace dbg_mi
{

struct Token
{
    enum Type
    {
        TupleStart = 0,
        TupleEnd,
        ListStart,
        ListEnd,
        Equal,
        String,
        Comma,
        Undefined
    };

    Token() :
        type(Undefined) {
    }
    Token(int start_, int end_, Type type_) :
        start(start_),
        end(end_),
        type(type_) {
    }

    bool operator == (Token const & t) const {
        return start == t.start && end == t.end && type == t.type;
    }
    wxString ExtractString(wxString const & s) const {
        assert(end <= static_cast<int>(s.length()));
        return s.substr(start, end - start);
    }

    int start, end;
    Type type;
};

bool GetNextToken(wxString const & str, int pos, Token & token);

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_CMD_RESULT_TOKENS_H_
