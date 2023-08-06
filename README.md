# Untangle
Amiga port of a popular logic game.
## The Game
Untangle is a logic puzzle, where the player faces a set of dots connected by lines. Usually the set is untidy, many lines cross another ones. The game goal is to untangle lines by moving dots, so finally no line cross another.
## Project Goals
* Game should be system friendly, working in a window on Workbench, using only documented APIs.
* Minimum system version is 3.0.
* RTG (graphic cards) should be supported out of the box. Without dedicated code paths and explicit opening of RTG libraries, if possible.
* Would be nice if it works on MorphOS, AmigaOS 4, AROS using M68k emulation built into these operating systems.
* Should be comfortably playable on stock A1200, stock A500 is not a Holy Grail, but why not.
## Building
Project is developed on a real Amiga 1200 with 68020 @ 28 MHz and 64 MB fast RAM. Compiler used is native GCC 2.95.3 with usual GeekGadgets environment, and "make" tool. To build the game just enter
```
make
```
in a shell.
At the moment test level file ("level.iff") is built manually from assembler source "level.s" with
```
vasm -Fbin level.s -o level.iff
```
It implies that "vasm" assembler is also installed. This is a temporary solution, I plan to add feature of creation and saving levels in the game itself.
## Installation
Executable may be copied anywhere. "level.iff" file should be in the same directory as executable. It runs from shell as well as from Workbench. For now there is no icon for it.
## Usage
After starting select "New" from menu (or `RAmiga + n` from keyboard) to load the test level.
## Requirements
* Amiga with any processor (code is compiled for 68000).
* Kickstart/Workbench 3.0 or higher.
* For now there is no detection of non-square pixel display modes, so dots are distorted on, for example, "PAL HighRes".
