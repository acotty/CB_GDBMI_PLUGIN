/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef _DEBUGGER_GDB_MI_CMD_DEBUGGEROPTIONSDLG_H_
#define _DEBUGGER_GDB_MI_CMD_DEBUGGEROPTIONSDLG_H_

#include <debuggermanager.h>

class ConfigManagerWrapper;

namespace dbg_mi
{
    class DebuggerConfiguration : public cbDebuggerConfiguration
    {
        public:
            explicit DebuggerConfiguration(const ConfigManagerWrapper & config);

            cbDebuggerConfiguration * Clone() const override;
            wxPanel * MakePanel(wxWindow * parent) override;
            bool SaveChanges(wxPanel * panel) override;

        public:
            enum Flags
            {
                DisableInit,
                WatchFuncArgs,
                WatchLocals,
                CatchExceptions,
                EvalExpression,
                AddOtherProjectDirs,
                CheckPrettyPrinters,
                DoNotRun
            };

            bool GetFlag(Flags flag);
            void SetFlag(Flags flag, bool value);
            wxString GetDebuggerExecutable(bool expandMacro = true);
            wxString GetUserArguments(bool expandMacro = true);
            wxString GetDisassemblyFlavorCommand();
            wxString GetInitialCommands();

    };

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_CMD_DEBUGGEROPTIONSDLG_H_
