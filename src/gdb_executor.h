#ifndef _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
#define _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_

#include "cmd_queue.h"

class PipedProcess;
class wxEvtHandler;

namespace dbg_mi
{
class LogPaneLogger : public Logger
{
public:
    LogPaneLogger() : m_shutdowned(false) {}

    virtual void Debug(wxString const &line, Line::Type type = Line::Debug);
    virtual Line const* GetDebugLine(int index) const { return NULL; }

    virtual void AddCommand(wxString const &command) { m_commands.push_back(command); }
    virtual int GetCommandCount() const { return m_commands.size(); }
    virtual wxString const& GetCommand(int index) const
    {
        Commands::const_iterator it = m_commands.begin();
        std::advance(it, index);
        return *it;
    }
    virtual void ClearCommand() { m_commands.clear(); }

    void MarkAsShutdowned() { m_shutdowned = true; }
private:
    typedef std::vector<wxString> Commands;
    Commands m_commands;
    bool m_shutdowned;
};

class GDBExecutor : public CommandExecutor
{
    GDBExecutor(GDBExecutor &o);
    GDBExecutor& operator =(GDBExecutor &o);
public:
    GDBExecutor();
    ~GDBExecutor();

    void Init(int log_page, int debug_page);
    int LaunchProcess(wxString const &cmd, wxString const& cwd, int id_gdb_process, wxEvtHandler *event_handler);

    bool ProcessHasInput();
    bool IsRunning() const;
    bool IsStopped() const { return m_stopped; }
    bool Interupting() const { return m_interupting; }
    bool IsTemporaryInterupt() const { return m_temporary_interupt; }

    void Stopped(bool flag);
    void Interupt(bool temporary = true);
    void ForceStop();

    virtual wxString GetOutput();

    void SetAttachedPID(long pid) { m_attached_pid = pid; }
    long GetAttachedPID() const { return m_attached_pid; }

protected:
    virtual bool DoExecute(dbg_mi::CommandID const &id, wxString const &cmd);
    virtual void DoClear();
private:
    long GetChildPID();
private:
    PipedProcess *m_process;
    int m_log_page;
    int m_debug_page;
    long m_pid, m_child_pid, m_attached_pid;

    bool m_stopped;
    bool m_interupting;
    bool m_temporary_interupt;
};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_GDB_EXECUTOR_H_
