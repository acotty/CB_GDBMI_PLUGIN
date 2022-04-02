# CODE::BLOCKS  GDB/MI DEBUGGER PLUGIN

**The debugger is not production quality yet, but you can check it out as it can debug itself.**

## Description
This GitHub repo contains the future Code::Blocks GDB/MI debugger plugin. The source is to be built with the current C::B nightly build and GCC compiler that supports C++11 or later (GCC 10 or 11 is recommended). The code is desigend for GDB 9.1 or later using the GDB/MI version 3 interface (GDB 11.2 is recommended).

This GDB/MI debugger plugin has a number of advantages over the existing GDB debugger plugin, like:

1. Supports all of the existing GDB annotations debugger plugin features once finished.
2. In theory quicker as there is less data transferred between C::B and GDB.
3. Supports GDB Python pretty printing if GDB supports it.
4. Should be easier easier to add new features compared to the old code.
5. GDB/MI interface is supported. The GDB annotations interface is deprecated.

## OUTSTANDING ITEMS

### High Priority

1. Persist debug data between sessions:
    status: 
        preliminary code is working.
        The code will change, but hopefully the XML file structure coudl stay the same, but with the addition of memory watches

    Working:
        - line breakpoint saved and loaded
        - simple data watch data saved and loaded
    Not done:
        - non simple breakpoints not coded up, but data is saved and loaded
        - edited watch is not coded, but data is saved and loaded
        - memory watch - nothing has been done
        - option to disable persistence via debugger dialog

### Medium Priority

1. Debug -> Memory view dialog
    a) Plugin calls Debugger_GDB_MI::AddMemoryRange
    b) Need to spend time analysing existing code
2. Fix "#warning" messages.
3. Re-test all items!!!! 
4. Add control to vary the amount/type of logging
5. Publish plugin to github

### Low Priority

1. Send project search paths to GDB. Search paths from  Project->Properties->"Debugger GDB/MI" tab.
2. Remote debugging - use Project->Properties->"Debugger GDB/MI" tab data.
3. Create Linux project file
4. Create MacOS project file
5. Update Linux makefile build process
6. Update MSYS2 makefile build process
7. Update MacOS makefile build process
8. CPU registry dialog modify to fix value column to say 50 characters.
9. Check addresses are 64 or 32 bit compatible. An example of this is:
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

### Future Work

1. Checkpoints (only available on linux )
2. Display the return value of a function after "step out"
3. Skipping functions
4. record replay (only linux)
5. More (easy) integration of pretty printing

## Testing/Coding/Feature Check List

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
|  * Next instruction     (Alt-F7)                           |  To test  |   To test  |
|  * Step into instruction(ALT-Shift-F7)                     |  To test  |   To test  |
|  * Set next statement                                      |  To test  |   To test  |
|  * Notification that the debugging has ended               | 26MAR2022 |    Pass    |
|                                                            |           |            |
|* Watches                                                   |           |            |
|  * Simple data types                                       | 26MAR2022 |    Pass    |
|  * Simple structure                                        | 26MAR2022 |    Pass    |
|  * Array of simple structures                              | 26MAR2022 |    Pass    |
|  * Complex structures                                      | 26MAR2022 |    Pass    |
|  * Edit watches                                            | 30MAR2022 |    Pass    |
|                                                            |           |            |
|* Breakpoints                                               |           |            |
|  * adding break point after the start of the debugger      | 26MAR2022 |    Pass    |
|  * Debug menu option to Toggle break point (F5)            | 26MAR2022 |    Pass    |
|  * Debug menu option to Remove all breakpoints             | 26MAR2022 |    Pass    |
|  * Disable/Enable break point via pop up menu              | 28MAR2022 |    Pass    |
|  * Remove break point                                      | 26MAR2022 |    Pass    |
|  * Edit break point                                        | 26MAR2022 |    Pass    |
|    * ignore count before break                             | 26MAR2022 |    Pass    |
|    * break when expression is true                         | 26MAR2022 |    Pass    |
|                                                            |           |            |
|* Debug show Running Threads                                | 26MAR2022 |    Pass    |
|                                                            |           |            |
|* Debug show CPU Registers                                  | 27MAR2022 |    Pass    |
|                                                            |           |            |
|* Debug show Call Stack                                     | 26MAR2022 |    Pass    |
|  * Double click on entry should open and go to the line    | 28MAR2022 |    Pass    |
|                                                            |           |            |
|* Debug show Disassembly                                    |           |            |
|  * Assembly only                                           | 31MAR2022 |    Pass    |
|  * Mixed mode                                              | 31MAR2022 |    Pass    |
|  * Adjust button feature                                   | 31MAR2022 |  *Broken*  |
|  * Save data to file feature                               | 31MAR2022 |    Pass    |
|                                                            |           |            |
|* Debug -> Memory Dump Dialog                               | 28MAR2022 |    Pass    | 
|                                                            |           |            |
|* Debug -> Memory view Dialog                               | 26MAR2022 |  *Broken*  | 
|                                                            |           |            |
|* Show tty for console projects                             | 28MAR2022 |    Pass    |
|                                                            |           |            |
|* Projects - edit see debuggeroptionsprjdlg                 | 29MAR2022 |  *Broken*  |
|                                                            |           |            |
|* GDB/MI Debugger configuration dialog                      |           |            |
|  * Executable path save/loaded/used                        |  To test  |   To test  |
|  * Arguments save/loaded/used                              |  To test  |   To test  |
|  * Arguments used in starting debugger                     |  To test  |   To test  |
|  * Debugger init commands save/loaded/used                 |  To test  |   To test  |
|  * Disable startup scripts checkbox save/loaded/used       |  To test  |   To test  |
|  * Watch function arguments checkbox save/loaded/used      |  To test  |   To test  |
|  * Watch local variables checkbox save/loaded/used         |  To test  |   To test  |
|  * Catch C++ exceptions checkbox save/loaded/used          |  To test  |   To test  |
|  * Evaluate expression under cursor cbox save/loaded/used  |  To test  |   To test  |
|  * Add other projects paths... checkbox save/loaded/used   |  To test  |   To test  |
|  * Do not run the debugee checkbox save/loaded/used        |  To test  |   To test  |
|  * Use python pretty printer checkbox save/loaded/used     |  To test  |   To test  |
|  * Disassembly flavor drop down list save/loaded/used      |  To test  |   To test  |
|    * ADD:                                                  |           |            |
|        * new Checkbox for save/load breakpoint/watch etc   |  Future   |   Future   |
|                                                            |           |            |

## COMPLETED ITEMS

* 02APR2022 Done - watch and line break points are persistent
* 30MAR2022 Done - Disassembly view working like the existing code (adjust is broken)
* 30MAR2022 Done - Wired up Project->Properties->"Debugger GDB/MI" tab. Loads and saves data only.
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

