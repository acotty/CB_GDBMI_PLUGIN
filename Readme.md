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
1. Debug "simple" source code sample with the following "data" types:

```cpp
    char cTest[300];
    ....
    struct TestStruct
    {
        int iValue;
        bool bReadOnly;
        char cName[10];
    };
    ....
    TestStruct stTest;
    ....
    TestStruct aTest[3];
    ....
```
2. Test the items in the check list.

## Medium Priority

1. Add control to vary the amount/type of logging
2. Create plugin
3. Publish plign to github

## Low Priority

1. Create Linux project file
2. Create MacOS project file
3. Update Linux makefile build process
4. Update MSYS2 makefile build process
5. Update MacOS makefile build process

## Check List

* Stepping
  * Start/Continue  (F8)
  * Break debugger
  * Stop debugger   (Shift-F8)
  * Run to cursor   (F4)
  * Next line       (F7)
  * Step Into       (Shift-F7)
  * Step out        (Ctrl-F7)
  * Next instruction        (Alt-F7)
  * Step into instruction   (ALT-Shift-F7)
  * Set next statement
  * Interupting the debugger while running the program
  * Notification that the debugging has ended
* Watches
* Breakpoints
  * adding breakpoint after the start of the debugger
  * condition
  * Debug menu option to Toggle breakpoint (F5)
  * Debug menu option to Remove all breakpoints
  * Disable breakpoint
  * Remove breakpoint
  * Edit breakpoint
    * ignore count before break
    * break when expression is true
* Running Threads
* CPU Registers
* Call Stack
* Disassembly
* Memory dump
* Memory view
* System view
* Show tty for console projects


# COMPLETED ITEMS

* 24MAR2022 Done - Update logging - merge new logging code
* 24MAR2022 Done - Fix warnings
* 24MAR2022 Done - Update file header - C::B GPL text
* 24MAR2022 Done - Add more logging to help find issues
* 25MAR2022 Done - Update logged data to be more readable/understanding
* 25MAR2022 Fixed - removed array watch limit of 100
* 25MAR2022 Fixed - updates of array items > 9
