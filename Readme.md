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
1. Fix broken items in the list. So far they are:
    a) CPU Registers
        - initial view on showing registers - done 27MAR2022
        - initial view on running app with registers dialog showing
        - update on step like existing GDB 
    b) Memory dump
    c) Memory view
    d) Disable break point
    e) Conditional break points:
        - add
        - edit
        - update
        - remove
    d) Disassembly
    

## Medium Priority

1. Add control to vary the amount/type of logging
2. Publish pluign to github

## Low Priority

1. Create Linux project file
2. Create MacOS project file
3. Update Linux makefile build process
4. Update MSYS2 makefile build process
5. Update MacOS makefile build process

## Check List

|                  Item                                      |   Date    |   Result     |
|------------------------------------------------------------|-----------|--------------|
|* Stepping                                                  |           |              |
|  * Start/Continue       (F8)                               | 26MAR2022 |    Pass      |
|  * Break debugger                                          | 26MAR2022 |    Pass      |
|  * Stop debugger        (Shift-F8)                         | 26MAR2022 |    Pass      |
|  * Run to cursor        (F4)                               | 26MAR2022 |    Pass      |
|  * Next line            (F7)                               | 26MAR2022 |    Pass      |
|  * Step Into            (Shift-F7)                         | 26MAR2022 |    Pass      |
|  * Step out             (Ctrl-F7)                          | 26MAR2022 |    Pass      |
|  * Next instruction     (Alt-F7)                           | 25MAR2022 | To be tested |
|  * Step into instruction(ALT-Shift-F7)                     | 25MAR2022 | To be tested |
|  * Set next statement                                      | 25MAR2022 | To be tested |
|  * Notification that the debugging has ended               | 26MAR2022 |    Pass      |
|* Watches                                                   |           |              |
|  * Simple data types                                       | 26MAR2022 |    Pass      |
|  * Simple structure                                        | 26MAR2022 |    Pass      |   
|  * Array of simple structures                              | 26MAR2022 |    Pass      |   
|  * Complex structures                                      | 26MAR2022 |    Pass      |
|* Breakpoints                                               |           |              |
|  * adding break point after the start of the debugger      | 26MAR2022 |    Pass      |
|  * Debug menu option to Toggle break point (F5)            | 26MAR2022 |    Pass      |
|  * Debug menu option to Remove all breakpoints             | 26MAR2022 |    Pass      |
|  * Disable break point                                     | 26MAR2022 |   *Broken*   |
|  * Remove break point                                      | 26MAR2022 |    Pass      |
|  * Edit break point                                        | 26MAR2022 |   *Broken*   |
|    * ignore count before break                             | 26MAR2022 |   *Broken*   |
|    * break when expression is true                         | 26MAR2022 |   *Broken*   |
|* Debug show Running Threads                                | 26MAR2022 |    Pass      |
|* Debug show CPU Registers                                  | 26MAR2022 |   *Broken*   |
|* Debug show Call Stack                                     | 26MAR2022 |    Pass      |
|* Debug show Disassembly                                    | 26MAR2022 |   *Broken*   |
|* Debug show Memory dump                                    | 26MAR2022 |   *Broken*   | 
|* Debug show Memory view                                    | 26MAR2022 |   *Broken*   | 
|* Show tty for console projects                             | 26MAR2022 |   *Broken*   |

# COMPLETED ITEMS

* 24MAR2022 Done - Update logging - merge new logging code
* 24MAR2022 Done - Fix warnings
* 24MAR2022 Done - Update file header - C::B GPL text
* 24MAR2022 Done - Add more logging to help find issues
* 25MAR2022 Done - Update logged data to be more readable/understanding
* 25MAR2022 Fixed - removed array watch limit of 100
* 25MAR2022 Fixed - updates of array items > 9
* 26MAR2022 Fixed - display of structures with multiple depths now working
* 26MAR2022 Done - Debug "simple" source code sample with the following "data" types:
* 26MAR2022 Done - First pass of the check list completed
* 26MAR2022 Done - Project file now creates the debugger_gdbmi.cbplugin file with the non striped DLL (still includes debugging)
