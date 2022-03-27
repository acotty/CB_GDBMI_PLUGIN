/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "helpers.h"

#include <string.h>
#include <stdio.h>

namespace dbg_mi
{
    int ParseParentPID(const char * line)
    {
        const char * p = strchr(line, '(');

        if (!p)
        {
            return -1;
        }

        ++p;
        int open_paren_count = 1;

        while (*p && open_paren_count > 0)
        {
            switch (*p)
            {
                case '(':
                    open_paren_count++;
                    break;

                case ')':
                    open_paren_count--;
                    break;
            }

            ++p;
        }

        if (*p == ' ')
        {
            ++p;
        }

        int dummy;
        int ppid;
        int count = sscanf(p, "%c %d", (char *) &dummy, &ppid);
        return count == 2 ? ppid : -1;
    }

} // namespace dbg_mi
