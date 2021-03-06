/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef __DEBUGGEROPTIONSPRJDLG_H__
#define __DEBUGGEROPTIONSPRJDLG_H__

#include <wx/intl.h>
#include "configurationpanel.h"
#include <settings.h>

#include "remotedebugging.h"
#include "plugin.h"

class cbProject;
class wxListBox;
class CodeBlocksEvent;

namespace dbg_mi
{

    class DebuggerOptionsProjectDlg : public cbConfigurationPanel
    {
        public:
            DebuggerOptionsProjectDlg(wxWindow* parent, Debugger_GDB_MI* debugger, cbProject* project);
            ~DebuggerOptionsProjectDlg() override;

            wxString GetTitle() const override { return "Debugger GDB/MI"; }
            wxString GetBitmapBaseName() const override { return "debugger"; }
            void OnApply() override;
            void OnCancel() override {}
        protected:
            void OnTargetSel(wxCommandEvent& event);
            void OnAdd(wxCommandEvent& event);
            void OnEdit(wxCommandEvent& event);
            void OnDelete(wxCommandEvent& event);
            void OnUpdateUI(wxUpdateUIEvent& event);
        private:
            void OnBuildTargetRemoved(CodeBlocksEvent& event);
            void OnBuildTargetAdded(CodeBlocksEvent& event);
            void OnBuildTargetRenamed(CodeBlocksEvent& event);
            void LoadCurrentRemoteDebuggingRecord();
            void SaveCurrentRemoteDebuggingRecord();

            Debugger_GDB_MI* m_pDebuggerGDBMI;
            cbProject* m_pProject;
            wxArrayString m_OldPaths;
            RemoteDebuggingMap m_OldRemoteDebugging;
            RemoteDebuggingMap m_CurrentRemoteDebugging;
            int m_LastTargetSel;
            DECLARE_EVENT_TABLE()
    };
}; // namespace dbg_mi

#endif // __DEBUGGEROPTIONSPRJDLG_H__
