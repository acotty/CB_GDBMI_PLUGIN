/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <sdk.h>
#include "editwatchdlg.h"

#ifndef CB_PRECOMP
    #include <wx/button.h>
    #include <wx/checkbox.h>
    #include <wx/defs.h>
    #include <wx/intl.h>
    #include <wx/radiobox.h>
    #include <wx/sizer.h>
    #include <wx/spinctrl.h>
    #include <wx/textctrl.h>
    #include <wx/xrc/xmlres.h>
#endif // CB_PRECOMP

#include "definitions.h"

namespace dbg_mi
{

    EditWatchDlg::EditWatchDlg(cb::shared_ptr<dbg_mi::GDBWatch> watch, wxWindow* parent)
        : m_watch(watch)
    {
        wxXmlResource::Get()->LoadObject(this, parent, "dlgEditWatchGDBMI", "wxScrollingDialog");

        if (m_watch)
        {
            long lArrayStart = m_watch->GetRangeArrayStart();
            long lArrayLength = m_watch->GetRangeArrayEnd() - lArrayStart;

            wxString symbol;
            m_watch->GetSymbol(symbol);

            XRCCTRL(*this, "txtKeyword", wxTextCtrl)->SetValue(symbol);
            XRCCTRL(*this, "rbFormat", wxRadioBox)->SetSelection((int)m_watch->GetFormat());
            XRCCTRL(*this, "chkArray", wxCheckBox)->SetValue(m_watch->GetIsArray());
            XRCCTRL(*this, "spnArrStart", wxSpinCtrl)->SetValue(lArrayStart);
            XRCCTRL(*this, "spnArrCount", wxSpinCtrl)->SetValue(lArrayLength);
        }
        XRCCTRL(*this, "txtKeyword", wxTextCtrl)->SetFocus();
        XRCCTRL(*this, "wxID_OK", wxButton)->SetDefault();
    }

    EditWatchDlg::~EditWatchDlg()
    {
    }

    void EditWatchDlg::EndModal(int retCode)
    {
        if (retCode == wxID_OK && m_watch)
        {
            long lArrayStart = XRCCTRL(*this, "spnArrStart", wxSpinCtrl)->GetValue();
            long lArrayEnd = lArrayStart + XRCCTRL(*this, "spnArrCount", wxSpinCtrl)->GetValue();

            wxString symbol = CleanStringValue(XRCCTRL(*this, "txtKeyword", wxTextCtrl)->GetValue());
            m_watch->SetSymbol(symbol);
            m_watch->SetFormat((GDBWatch::WatchFormat)XRCCTRL(*this, "rbFormat", wxRadioBox)->GetSelection());
            m_watch->SetIsArray(XRCCTRL(*this, "chkArray", wxCheckBox)->GetValue());
            m_watch->SetRangeArray(lArrayStart, lArrayEnd);
        }
        wxScrollingDialog::EndModal(retCode);
    }

} // namespace dbg_mi
