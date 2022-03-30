/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef EDITBREAKPOINT_H
#define EDITBREAKPOINT_H

#include "scrollingdialog.h"
#include "definitions.h"

namespace dbg_mi
{

    class EditBreakpointDlg : public wxScrollingDialog
    {
        public:
            EditBreakpointDlg(GDBBreakpoint & breakpoint, wxWindow* parent = 0);
            ~EditBreakpointDlg() override;

            dbg_mi::GDBBreakpoint& GetBreakpoint()
            {
                return m_breakpoint;
            }

        protected:
            void OnUpdateUI(wxUpdateUIEvent & event);
            void EndModal(int retCode) override;

            GDBBreakpoint & m_breakpoint;
        private:
            DECLARE_EVENT_TABLE()
    };
} // namespace dbg_mi

#endif // EDITBREAKPOINT_H
