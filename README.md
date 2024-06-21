# Untangle
Amiga port of a popular logic game.
## The Game
Untangle is a logic puzzle, where the player faces a set of dots connected by lines. Usually the set is untidy, many lines cross another ones. The game goal is to untangle lines by moving dots, so finally no line cross another.

![game screenshot, set of tangled lines connecting dots](https://cdn.rastport.com/var/untangle_mini.png)
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
To run the game a level set file "StandardSet.iff" has to be placed in the same directory as executable. The file is contained in releases. 
## Installation
Executable may be copied anywhere. "StandardSet.iff" file should be in the same directory as executable. It runs from shell as well as from Workbench.
## Usage
First level of the set is loaded automatically when game starts. Next level is loaded after current one is solved by the player.
## Requirements
* Amiga with any processor (code is compiled for 68000).
* Kickstart/Workbench 3.0 or higher.
