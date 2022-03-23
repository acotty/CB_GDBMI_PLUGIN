# Description
This GitHub repo has the source code for the new Code::Blocks GDB/MI debugger. The source is to be built with the current C::B nightly and a GCC compiler that supports C++11. The code is desigend for GDB 9 or later using the GDB/MI version 3 interface.

This uses the GDB Machine Interface instead of the old annotations (deprecated) interface. This has a number of advantages:

1. Supports all of the existing GDB debugger features once finished
2. Quicker
3. Supports GDB Python pretty printing if GDB supports it.
4. Should be easier easier to add new features compared to the old code.


**The debugger is not production quality at this point in time.** It can be tested, but please wait until the new logging is ready and then give it a go as the logging will be more extensive and will help allot more if you hit a bug.



#OUTSTANDING ITEMS

## High Priority
1. Update logging - merge new logging code

## Medium Priority
1. Create plugin
2. Publish plign to github
3. Fix warnings
4. Add control to vary the amount/type of logging 

## Low Priority
* Create Linux project file
* Create MacOS project file
* Update Linux makefile build process
* Update MSYS2 makefile build process
* Update MacOS makefile build process

## Existing/Old Issues/Check List

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