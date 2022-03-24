/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#include "events.h"


namespace dbg_mi
{
// Could have been DEFINE_EVENT_TYPE( MyFooCommandEvent )
const wxEventType NotificationEventType = wxNewEventType();


} // namespace dbg_mi
