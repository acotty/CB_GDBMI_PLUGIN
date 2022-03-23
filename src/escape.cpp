#include "escape.h"

namespace dbg_mi
{

wxString EscapePath(wxString const & path)
{
    if (path.empty())
    {
        return path;
    }

    wxChar wxch;
    bool escape = false, escapeDoubleQuotes = false;

    for (size_t ii = 0; ii < path.length(); ++ii)
    {
        wxch = path[ii];

        switch (wxch)
        {
            case ' ':
                escape = true;
                break;

            case '"':
                if (ii > 0 && ii < path.length() - 1)
                {
                    escapeDoubleQuotes = true;
                }

                break;
        }
    }

    if (path[0] == '"' && path[path.length() - 1] == '"')
    {
        escape = false;
    }

    if (!escape && !escapeDoubleQuotes)
    {
        return path;
    }

    wxString result;

    if (path[0] == '"' && path[path.length() - 1] == '"')
    {
        result = path.substr(1, path.length() - 2);
    }
    else
    {
        result = path;
    }

    result.Replace("\"", "\\\"");
    return '"' + result + '"';
}

void ConvertDirectory(wxString & str, wxString base, bool relative)
{
    if (!base.empty())
    {
        str = base + "/" + str;
    }

    str = EscapePath(str);
}

} // namespace dbg_mi
