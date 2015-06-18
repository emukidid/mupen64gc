### Introduction ###

Most of these shouldn't be major goals, but things we would like to see in the emulator, or optimizations we think would get things running better.  Try to explain features, and try to describe the optimizations if possible.


# Checklist #

  * **Dev Feature:** Debug prints that collect text and display it on the FB for a few frames/seconds (Good enough for now)

  * **Dev Feature:** Profiling different plugins or code paths (More or less done)

  * **Optimization:** Initialize the correct video mode for each ROM region (to achieve proper FPS)

  * **Optimization:** Change some RSP code to utilize the DSP (maaaaaaybe later)

  * **Optimization:** In dynarec, eliminate redundant work on adjacent interpreted instructions

  * **Optimization:** In dynarec, implement blocks' code\_addr and/or jump\_tables' dst\_instr as offsets rather than pointers if it saves work resizing and other usage

  * **Optimization:** In dynarec, identify known memory address and recompile those loads/stores (maybe even stack?)

  * **Optimization:** In dynarec, try to keep track of constant values to simplify later operations on those values

  * **Optimization:** Reduce the size of ROM blocks to hopefully reduce thrashing on GC

  * **Optimization:** Use inline assembly code for loading byteswapped (and halfword swapped) ROM files

  * **Feature:** ROM Information menu option

  * **Feature:** Save states (done, might need more testing)

  * **Feature:** Widescreen and 480p support (NTSC 480p support now)

  * **Feature:** Modular controller code which support for multiple types simultaneously (Mostly working, needs configuration)

  * **Feature:** TCP Loading on Wii (and on GC?)

  * **Feature:** Set preferred source for ROMs/saves (rather than setting every time)

  * **Feature:** Save manager (not needed?)

  * **Feature:** Improved ROM browsing (icons, info, etc)
