# Description
This GitHub repo has the source code for the new Code::Blocks GDB/MI debugger. The source is to be built with the current C::B nightly and a GCC compiler that supports C++11. The code is desigend for GDB 9 or later using the GDB/MI version 3 interface.

This uses the GDB Machine Interface instead of the old annotations (deprecated) interface. This has a number of advantages:

1. Supports all of the existing GDB debugger features once finished
2. Quicker
3. Supports GDB Python pretty printing if GDB supports it.
4. Should be easier easier to add new features compared to the old code.
5. GDB/MI interface is supported.

**The debugger is not production quality at this point in time.**

**It can be tested NOW, so give it a go in order to help with finding missing functionality and/or bugs.**

# OUTSTANDING ITEMS

## High Priority
1. Project->Properties->Debugger tab:
    Done:
        - wire up debugger_project_options_dlg.xrc to display the dialog
        - added hacked debuggeroptionsprjdlg.cpp/.h files into the project
    Outstanding:
        - load variables/data
        - save variables/data

## Medium Priority
1. Debug -> Memory view dialog
    a) Plugin calls Debugger_GDB_MI::AddMemoryRange
    b) Need to spend time analysing existing code
2. Disassembly view
3. Add control to vary the amount/type of logging
4. Publish plugin to github
5. Check addresses are 64 or 32 bit compatable
    An example of this is:
#if wxCHECK_VERSION(3, 1, 5)
        if (wxPlatformInfo::Get().GetBitness() == wxBITNESS_64)
#else
        if (wxPlatformInfo::Get().GetArchitecture() == wxARCH_64)
#endif
        {
            sAddressToShow = wxString::Format("%#018llx", llAddrLineStart); // 18 = 0x + 16 digits
        }
        else
        {
            sAddressToShow = wxString::Format("%#10llx", llAddrLineStart); // 10 = 0x + 8 digits
        }


## Low Priority
1. Remote debugging 
2. Create Linux project file
3. Create MacOS project file
4. Update Linux makefile build process
5. Update MSYS2 makefile build process
6. Update MacOS makefile build process
7. CPU registry dialog modify to fix value column to say 50 characters.

## Check List

|                  Item                                      |   Date    |   Result   |
|------------------------------------------------------------|-----------|------------|
|* Stepping                                                  |           |            |
|  * Start/Continue       (F8)                               | 26MAR2022 |    Pass    |
|  * Break debugger                                          | 26MAR2022 |    Pass    |
|  * Stop debugger        (Shift-F8)                         | 26MAR2022 |    Pass    |
|  * Run to cursor        (F4)                               | 26MAR2022 |    Pass    |
|  * Next line            (F7)                               | 26MAR2022 |    Pass    |
|  * Step Into            (Shift-F7)                         | 26MAR2022 |    Pass    |
|  * Step out             (Ctrl-F7)                          | 26MAR2022 |    Pass    |
|  * Next instruction     (Alt-F7)                           | 25MAR2022 |   To test  |
|  * Step into instruction(ALT-Shift-F7)                     | 25MAR2022 |   To test  |
|  * Set next statement                                      | 25MAR2022 |   To test  |
|  * Notification that the debugging has ended               | 26MAR2022 |    Pass    |
|* Watches                                                   |           |            |
|  * Simple data types                                       | 26MAR2022 |    Pass    |
|  * Simple structure                                        | 26MAR2022 |    Pass    |
|  * Array of simple structures                              | 26MAR2022 |    Pass    |
|  * Complex structures                                      | 26MAR2022 |    Pass    |
|  * Edit watches                                            | 30MAR2022 |    Pass    |
|* Breakpoints                                               |           |            |
|  * adding break point after the start of the debugger      | 26MAR2022 |    Pass    |
|  * Debug menu option to Toggle break point (F5)            | 26MAR2022 |    Pass    |
|  * Debug menu option to Remove all breakpoints             | 26MAR2022 |    Pass    |
|  * Disable/Enable break point via pop up menu              | 28MAR2022 |    Pass    |
|  * Remove break point                                      | 26MAR2022 |    Pass    |
|  * Edit break point                                        | 26MAR2022 |    Pass    |
|    * ignore count before break                             | 26MAR2022 |    Pass    |
|    * break when expression is true                         | 26MAR2022 |    Pass    |
|* Debug show Running Threads                                | 26MAR2022 |    Pass    |
|* Debug show CPU Registers                                  | 27MAR2022 |    Pass    |
|* Debug show Call Stack                                     | 26MAR2022 |    Pass    |
|  * Double click on entry should open and go to the line    | 28MAR2022 |    Pass    |
|* Debug show Disassembly                                    | 26MAR2022 |  *Broken*  |
|* Debug -> Memory Dump Dialog                               | 28MAR2022 |    Pass    | 
|* Debug -> Memory view Dialog                               | 26MAR2022 |  *Broken*  | 
|* Show tty for console projects                             | 28MAR2022 |    Pass    |
|* Projects - edit see debuggeroptionsprjdlg                 | 29MAR2022 |  *Broken*  |

# COMPLETED ITEMS

* 30MAR2022 Done - Conditional break points
* 30MAR2022 Done - Edit watches now working
* 28MAR2022 Done - Examine Memory Dialog now working
* 28MAR2022 Done - fixed Disable/enable break point via pop up menu
* 28MAR2022 Done - Show tty for console projects 
* 27MAR2022 Done - CPU Registers showing and updating
* 26MAR2022 Fixed - display of structures with multiple depths now working
* 26MAR2022 Done - Debug "simple" source code sample with the following "data" types:
* 26MAR2022 Done - First pass of the check list completed
* 26MAR2022 Done - Project file now creates the debugger_gdbmi.cbplugin file with the non striped DLL (still includes debugging)
* 25MAR2022 Done - Update logged data to be more readable/understanding
* 25MAR2022 Fixed - removed array watch limit of 100
* 25MAR2022 Fixed - updates of array items > 9
* 24MAR2022 Done - Update logging - merge new logging code
* 24MAR2022 Done - Fix warnings
* 24MAR2022 Done - Update file header - C::B GPL text
* 24MAR2022 Done - Add more logging to help find issues

