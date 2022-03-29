/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef EDITBREAKPOINT_H
#define EDITBREAKPOINT_H

#include "scrollingdialog.h"
#include "definitions.h"

class EditBreakpointDlg : public wxScrollingDialog
{
    public:
        EditBreakpointDlg(const dbg_mi::GDBBreakpoint &breakpoint, wxWindow* parent = 0);
        ~EditBreakpointDlg() override;

        const dbg_mi::GDBBreakpoint& GetBreakpoint() const { return m_breakpoint; }
    protected:
        void OnUpdateUI(wxUpdateUIEvent& event);
        void EndModal(int retCode) override;

        dbg_mi::GDBBreakpoint m_breakpoint;
    private:
        DECLARE_EVENT_TABLE()
};

#endif // EDITBREAKPOINT_H
