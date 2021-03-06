/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef __REMOTEDEBUGGING_H__
#define __REMOTEDEBUGGING_H__

#include <map>
#include <wx/string.h>

class ProjectBuildTarget;

namespace dbg_mi
{
    // per-target remote debugging support
    struct RemoteDebugging
    {
        enum ConnectionType
        {
            TCP = 0,
            UDP,
            Serial
        };

        RemoteDebugging() : connType(TCP), skipLDpath(false), extendedRemote(false) {}

        bool IsOk() const
        {
            return connType == Serial
                    ? (!serialPort.IsEmpty() && !serialBaud.IsEmpty())
                    : (!ip.IsEmpty() && !ipPort.IsEmpty());
        }

        void MergeWith(const RemoteDebugging& other)
        {
            if (other.IsOk())
            {
                connType = other.connType;
                serialPort = other.serialPort;
                serialBaud = other.serialBaud;
                ip = other.ip;
                ipPort = other.ipPort;
            }

            if (!additionalCmds.IsEmpty() && !other.additionalCmds.IsEmpty())
            {
                additionalCmds += _T('\n');
            }
            if (!other.additionalCmds.IsEmpty())
            {
                additionalCmds += other.additionalCmds;
            }

            if (!additionalCmdsBefore.IsEmpty() && !other.additionalCmdsBefore.IsEmpty())
            {
                additionalCmdsBefore += _T('\n');
            }
            if (!other.additionalCmdsBefore.IsEmpty())
            {
                additionalCmdsBefore += other.additionalCmdsBefore;
            }

            skipLDpath = other.skipLDpath;
            extendedRemote = other.extendedRemote;

            if (!additionalShellCmdsAfter.IsEmpty() && !other.additionalShellCmdsAfter.IsEmpty())
            {
                additionalShellCmdsAfter += _T('\n');
            }
            if (!other.additionalShellCmdsAfter.IsEmpty())
            {
                additionalShellCmdsAfter += other.additionalShellCmdsAfter;
            }

            if (!additionalShellCmdsBefore.IsEmpty() && !other.additionalShellCmdsBefore.IsEmpty())
            {
                additionalShellCmdsBefore += _T('\n');
            }
            if (!other.additionalShellCmdsBefore.IsEmpty())
            {
                additionalShellCmdsBefore += other.additionalShellCmdsBefore;
            }
        }

        bool operator == (const RemoteDebugging &rd) const
        {
            return (connType == rd.connType
                && serialPort == rd.serialPort
                && serialBaud == rd.serialBaud
                && ip == rd.ip
                && ipPort == rd.ipPort
                && additionalCmds == rd.additionalCmds
                && additionalCmdsBefore == rd.additionalCmdsBefore
                && additionalShellCmdsAfter == rd.additionalShellCmdsAfter
                && additionalShellCmdsBefore == rd.additionalShellCmdsBefore
                && skipLDpath == rd.skipLDpath
                && extendedRemote == rd.extendedRemote);
        }

        ConnectionType connType;
        wxString serialPort;
        wxString serialBaud;
        wxString ip;
        wxString ipPort;
        wxString additionalCmds;            // commands after remote connection established
        wxString additionalCmdsBefore;      // commands before establishing remote connection
        wxString additionalShellCmdsAfter;  // shell commands after remote connection established
        wxString additionalShellCmdsBefore; // shell commands before establishing remote connection
        bool skipLDpath;                    // skip adjusting LD_LIBRARY_PATH before launching debugger
        bool extendedRemote;                // connect with extended remote or not
    };

    typedef std::map<ProjectBuildTarget*, RemoteDebugging> RemoteDebuggingMap;

}; // namespace dbg_mi

#endif // __REMOTEDEBUGGING_H__
