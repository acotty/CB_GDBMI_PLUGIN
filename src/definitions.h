/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
*/

#ifndef _DEBUGGER_GDB_MI_DEFINITIONS_H_
#define _DEBUGGER_GDB_MI_DEFINITIONS_H_

#include <deque>
#include <tr1/memory>

#include <wx/sizer.h>
#include <wx/string.h>

#include <debuggermanager.h>
#include <scrollingdialog.h>

namespace dbg_mi
{

    class GDBBreakpoint : public cbBreakpoint
    {
        public:
            enum BreakpointType
            {
                bptCode = 0,    // Normal file/line breakpoint
                bptFunction,    // Function signature breakpoint
                bptData         // Data breakpoint
            };

            GDBBreakpoint() :
                m_type(bptCode),
                m_project(nullptr),
                m_filename(wxEmptyString),
                m_line(-1),
                m_index(-1),
                m_temporary(false),
                m_enabled(true),
                m_active(true),
                m_useIgnoreCount(false),
                m_ignoreCount(0),
                m_useCondition(false),
                m_wantsCondition(false),
                m_address(0),
                m_alreadySet(false),
                m_breakOnRead(false),
                m_breakOnWrite(true),
                m_userData(0)

            {
            }

            GDBBreakpoint(const wxString & filename, int line, cbProject * project) :
                m_type(bptCode),
                m_project(project),
                m_filename(filename),
                m_line(line),
                m_index(-1),
                m_temporary(false),
                m_enabled(true),
                m_active(true),
                m_useIgnoreCount(false),
                m_ignoreCount(0),
                m_useCondition(false),
                m_wantsCondition(false),
                m_address(0),
                m_alreadySet(false),
                m_breakOnRead(false),
                m_breakOnWrite(true),
                m_userData(0)
            {
            }

            // from cbBreakpoint
            virtual void SetEnabled(bool flag);
            virtual wxString GetLocation() const;
            virtual int GetLine() const;
            virtual wxString GetLineString() const;
            virtual wxString GetType() const;
            virtual wxString GetInfo() const;
            virtual bool IsEnabled() const;
            virtual bool IsVisibleInEditor() const;
            virtual bool IsTemporary() const;

            // GDB additional
            int GetIndex() const
            {
                return m_index;
            }
            const wxString & GetCondition() const
            {
                return m_condition;
            }
            int GetIgnoreCount() const
            {
                return 0;
            }

            bool HasCondition() const
            {
                return false;
            }
            bool HasIgnoreCount() const
            {
                return false;
            }

            void SetIndex(int index)
            {
                m_index = index;
            }
            void ShiftLine(int linesToShift)
            {
                m_line += linesToShift;
            }

            cbProject * GetProject()
            {
                return m_project;
            }
            BreakpointType get_type()
            {
                return m_type;
            }
            void set_type(BreakpointType type)
            {
                m_type = type;
            }

            wxString get_filename()
            {
                return m_filename;
            }
            void set_filename(wxString filename)
            {
                m_filename = filename;
            }

            int get_line()
            {
                return m_line;
            }
            void set_line(int line)
            {
                m_line = line;
            }

            long get_index()
            {
                return m_index;
            }
            void set_index(long index)
            {
                m_index = index;
            }

            bool get_temporary()
            {
                return m_temporary;
            }
            void set_temporary(bool temporary)
            {
                m_temporary = temporary;
            }

            bool get_enabled()
            {
                return m_enabled;
            }
            void set_enabled(bool enabled)
            {
                m_enabled = enabled;
            }

            bool get_active()
            {
                return m_active;
            }
            void set_active(bool active)
            {
                m_active = active;
            }

            bool get_useIgnoreCount()
            {
                return m_useIgnoreCount;
            }
            void set_useIgnoreCount(bool useIgnoreCount)
            {
                m_useIgnoreCount = useIgnoreCount;
            }

            int get_ignoreCount()
            {
                return m_ignoreCount;
            }
            void set_ignoreCount(int ignoreCount)
            {
                m_ignoreCount = ignoreCount;
            }

            bool get_useCondition()
            {
                return m_useCondition;
            }
            void set_useCondition(bool useCondition)
            {
                m_useCondition = useCondition;
            }

            bool get_wantsCondition()
            {
                return m_wantsCondition;
            }
            void set_wantsCondition(bool wantsCondition)
            {
                m_wantsCondition = wantsCondition;
            }

            wxString get_condition()
            {
                return m_condition;
            }
            void set_condition(wxString condition)
            {
                m_condition = condition;
            }

            wxString get_func()
            {
                return m_func;
            }
            void set_func(wxString func)
            {
                m_func = func;
            }

            unsigned get_address()
            {
                return m_address;
            }
            void set_address(unsigned long int address)
            {
                m_address = address;
            }

            bool get_alreadySet()
            {
                return m_alreadySet;
            }
            void set_alreadySet(bool alreadySet)
            {
                m_alreadySet = alreadySet;
            }

            wxString get_lineText()
            {
                return m_lineText;
            }
            void set_lineText(wxString lineText)
            {
                m_lineText = lineText;
            }

            wxString get_breakAddress()
            {
                return m_breakAddress;
            }
            void set_breakAddress(wxString breakAddress)
            {
                m_breakAddress = breakAddress;
            }

            bool get_breakOnRead()
            {
                return m_breakOnRead;
            }
            void set_breakOnRead(bool breakOnRead)
            {
                m_breakOnRead = breakOnRead;
            }

            bool get_breakOnWrite()
            {
                return m_breakOnWrite;
            }
            void set_breakOnWrite(bool breakOnWrite)
            {
                m_breakOnWrite = breakOnWrite;
            }

            void * get_userData()
            {
                return m_userData;
            }
            void set_userData(void * userData)
            {
                m_userData = userData;
            }

        private:
            BreakpointType m_type;          // The type of this breakpoint.

            cbProject * m_project;          // The Project the file belongs to.
            wxString m_filename;            // The filename for the breakpoint.
            int m_line;                     // The line for the breakpoint.
            //wxString filenameAsPassed;    // The filename for the breakpoint as passed to the debugger (i.e. full filename).

            long m_index;                   // The breakpoint number. Set automatically. *Don't* write to it.
            bool m_temporary;               // Is this a temporary (one-shot) breakpoint?
            bool m_enabled;                 // Is the breakpoint enabled?
            bool m_active;                  // Is the breakpoint active? (currently unused)
            bool m_useIgnoreCount;          // Should this breakpoint be ignored for the first X passes? (@c x == @c ignoreCount)
            int m_ignoreCount;              // The number of passes before this breakpoint should hit. @c useIgnoreCount must be true.
            bool m_useCondition;            // Should this breakpoint hit only if a specific condition is met?
            bool m_wantsCondition;          // Evaluate condition for pending breakpoints at first stop !
            wxString m_condition;           // The condition that must be met for the breakpoint to hit. @c useCondition must be true.
            wxString m_func;                // The function to set the breakpoint. If this is set, it is preferred over the filename/line combination.
            unsigned long int m_address;    // The actual breakpoint address. This is read back from the debugger. *Don't* write to it.
            bool m_alreadySet;              // Is this already set? Used to mark temporary breakpoints for removal.
            wxString m_lineText;            // Optionally, the breakpoint line's text (used by GDB for setting breapoints on ctors/dtors).
            wxString m_breakAddress;        // Valid only for type==bptData: address to break when read/written.
            bool m_breakOnRead;             // Valid only for type==bptData: break when memory is read from.
            bool m_breakOnWrite;            // Valid only for type==bptData: break when memory is written to.
            void* m_userData;               // Custom user data.

    };


    typedef std::deque<cb::shared_ptr<cbStackFrame> > GDBBacktraceContainer;
    typedef std::deque<cb::shared_ptr<cbThread> > GDBThreadsContainer;

    class GDBWatch : public cbWatch
    {
        public:
            /** Watch variable format.
              *
              * @note not all formats are implemented for all debugger drivers.
              */
            enum WatchFormat
            {
                Undefined = 0, ///< Format is undefined (whatever the debugger uses by default).
                Decimal, ///< Variable should be displayed as decimal.
                Unsigned, ///< Variable should be displayed as unsigned.
                Hex, ///< Variable should be displayed as hexadecimal (e.g. 0xFFFFFFFF).
                Binary, ///< Variable should be displayed as binary (e.g. 00011001).
                Char, ///< Variable should be displayed as a single character (e.g. 'x').
                Float, ///< Variable should be displayed as floating point number (e.g. 14.35)

                // do not remove these
                Last, ///< used for iterations
                Any ///< used for watches searches
            };
        public:
            GDBWatch(wxString const & symbol, bool for_tooltip, bool delete_on_collapse = true) :
                m_symbol(symbol),
                m_has_been_expanded(false),
                m_for_tooltip(for_tooltip),
                m_delete_on_collapse(delete_on_collapse),
                m_array_start(-1),
                m_array_end(-1)
            {
            }

            void Reset()
            {
                m_id = m_type = m_value = wxEmptyString;
                m_has_been_expanded = false;
                RemoveChildren();
                m_array_start = -1;
                m_array_end = -1;
                Expand(false);
            }

            void SetRangeArray(long iStart, long iEnd)
            {
                m_array_start = iStart;
                m_array_end = iEnd;
            }

            long GetRangeArrayStart()
            {
                return m_array_start;
            }

            long GetRangeArrayEnd()
            {
                return m_array_end;
            }

            void SetIsArray(bool isArray)
            {
                 m_is_array = isArray;
            }

            bool GetIsArray()
            {
                return m_is_array;
            }

            void SetForTooltip(bool bForTooltip)
            {
                 m_forTooltip = bForTooltip;
            }

            bool GetForTooltip()
            {
                return m_forTooltip;
            }


            wxString const & GetID() const
            {
                return m_id;
            }

            void SetID(wxString const & id)
            {
                m_id = id;
            }

            bool HasBeenExpanded() const
            {
                return m_has_been_expanded;
            }

            void SetHasBeenExpanded(bool expanded)
            {
                m_has_been_expanded = expanded;
            }

            bool ForTooltip() const
            {
                return m_for_tooltip;
            }

            void SetDeleteOnCollapse(bool delete_on_collapse)
            {
                m_delete_on_collapse = delete_on_collapse;
            }

            bool DeleteOnCollapse() const
            {
                return m_delete_on_collapse;
            }

            void GetSymbol(wxString & symbol) const
            {
                symbol = m_symbol;
            }

            wxString GetSymbol() const
            {
                return m_symbol;
            }

            void SetSymbol(const wxString& symbol)
            {
                m_symbol = symbol;
            }

            void GetValue(wxString & value) const
            {
                value = m_value;
            }

            bool SetValue(const wxString & value)
            {
                m_value = value;
                return true;
            }

            void GetFullWatchString(wxString & full_watch) const
            {
                full_watch = m_value;
            }

            void GetType(wxString & type) const
            {
                type = m_type;
            }

            void SetType(const wxString & type)
            {
                m_type = type;
            }

            void SetFormat(WatchFormat format)
            {
                m_format = format;
            }

            WatchFormat GetFormat() const
            {
                return m_format;
            }

            wxString GetDebugString() const
            {
                m_debug_string = m_id + "->" + m_symbol + " = " + m_value;
                return m_debug_string;
            }

        protected:
            virtual void DoDestroy() {}
        private:
            wxString m_id;
            wxString m_symbol;
            wxString m_value;
            wxString m_type;
            WatchFormat m_format;

            mutable wxString m_debug_string;
            bool m_has_been_expanded;
            bool m_for_tooltip;
            bool m_delete_on_collapse;

            long m_array_start;
            long m_array_end;
            bool m_is_array;
            bool m_forTooltip;
    };

    typedef std::vector<cb::shared_ptr<GDBWatch> > GDBWatchesContainer;

    cb::shared_ptr<GDBWatch> FindWatch(wxString const & expression, GDBWatchesContainer & watches);

    class GDBMemoryRangeWatch  : public cbWatch
    {
        public:
            GDBMemoryRangeWatch(uint64_t address, uint64_t size, const wxString& symbol);

        public:
            void GetSymbol(wxString &symbol) const override
            {
                symbol = m_symbol;
            }

            void GetValue(wxString &value) const override
            {
                value = m_value;
            }

            bool SetValue(const wxString &value) override;
            void GetFullWatchString(wxString &full_watch) const override
            {
                full_watch = wxEmptyString;
            }

            void GetType(wxString &type) const override
            {
                type = wxT("Memory range");
            }

            void SetType(cb_unused const wxString &type) override
            {
            }

            wxString GetDebugString() const override
            {
                return wxString();
            }

            wxString MakeSymbolToAddress() const override;
            bool IsPointerType() const override
            {
                return false;
            }

            uint64_t GetAddress() const
            {
                return m_address;
            }

            uint64_t GetSize() const
            {
                return m_size;
            }

        private:
            uint64_t m_address;
            uint64_t m_size;
            wxString m_symbol;
            wxString m_value;
    };

    typedef std::vector<cb::shared_ptr<GDBWatch>> GDBWatchesContainer;
    typedef std::vector<cb::shared_ptr<GDBMemoryRangeWatch>> GDBMemoryRangeWatchesContainer;

    enum class GDBWatchType
    {
        Normal,
        MemoryRange
    };

    typedef std::unordered_map<cb::shared_ptr<cbWatch>, GDBWatchType> GDBMapWatchesToType;


    // Custom window to display output of DebuggerInfoCmd
    class GDBTextInfoWindow : public wxScrollingDialog
    {
        public:
            GDBTextInfoWindow(wxWindow * parent, const wxChar * title, const wxString & content) :
                wxScrollingDialog(parent, -1, title, wxDefaultPosition, wxDefaultSize,
                                  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX),
                m_font(8, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)
            {
                wxSizer * sizer = new wxBoxSizer(wxVERTICAL);
                m_text = new wxTextCtrl(this, -1, content, wxDefaultPosition, wxDefaultSize,
                                        wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH2 | wxHSCROLL);
                m_text->SetFont(m_font);
                sizer->Add(m_text, 1, wxGROW);
                SetSizer(sizer);
                sizer->Layout();
            }
            void SetText(const wxString & text)
            {
                m_text->SetValue(text);
                m_text->SetFont(m_font);
            }
        private:
            wxTextCtrl * m_text;
            wxFont m_font;
    };

    class GDBCurrentFrame
    {
        public:
            GDBCurrentFrame() :
                m_line(-1),
                m_stack_frame(-1),
                m_user_selected_stack_frame(-1),
                m_thread(-1)
            {
            }

            void Reset()
            {
                m_stack_frame = -1;
                m_user_selected_stack_frame = -1;
            }

            void GDBSwitchToFrame(int frame_number)
            {
                m_user_selected_stack_frame = m_stack_frame = frame_number;
            }

            void SetFrame(int frame_number)
            {
                if (m_user_selected_stack_frame >= 0)
                {
                    m_stack_frame = m_user_selected_stack_frame;
                }
                else
                {
                    m_stack_frame = frame_number;
                }
            }
            void SetThreadId(int thread_id)
            {
                m_thread = thread_id;
            }
            void SetPosition(wxString const & filename, int line)
            {
                m_filename = filename;
                m_line = line;
            }

            int GetStackFrame() const
            {
                return m_stack_frame;
            }
            int GetUserSelectedFrame() const
            {
                return m_user_selected_stack_frame;
            }
            void GetPosition(wxString & filename, int & line)
            {
                filename = m_filename;
                line = m_line;
            }
            int GetThreadId() const
            {
                return m_thread;
            }

        private:
            wxString m_filename;
            int m_line;
            int m_stack_frame;
            int m_user_selected_stack_frame;
            int m_thread;
    };

    // Use this function to sanitize user input which might end as the last part of GDB commands.
    // If the last character is '\', GDB will treat it as line continuation and it will stall.
    wxString CleanStringValue(wxString value);

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_DEFINITIONS_H_
