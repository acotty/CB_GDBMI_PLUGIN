/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef __EDITWATCHDLG_H__
#define __EDITWATCHDLG_H__

#include "scrollingdialog.h"

#include "definitions.h"

namespace dbg_mi
{
    class EditWatchDlg : public wxScrollingDialog
    {
        public:
            EditWatchDlg(cb::shared_ptr<dbg_mi::GDBWatch> w, wxWindow* parent);
            ~EditWatchDlg() override;

        protected:
            void EndModal(int retCode) override;

            cb::shared_ptr<dbg_mi::GDBWatch> m_watch;
    };
}; // namespace dbg_mi

#endif // __EDITWATCHDLG_H__
