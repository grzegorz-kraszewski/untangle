# Game state format
Game state consists of highscores of all the solved levels. The game state is stored as binary data in IFF container. IFF type is `UNTG` the same, as for level file. For now two chunks are defined:
* `HSCR` contains highscores for each solved level.
* `WINP` contains position and size of both game windows on the screen.
  
## `HSCR`
The chunk contains an array of fixed length records. Each record contains best score for a level. Level record is defined as follows:
```
int32 time;     the best time in seconds
int32 moves;    the lowest number of moves
```
As a level is not accessible until all the previous ones are solved, highscores are always ordered by level number and there are no gaps. Therefore level number is not explicitly stored.

The highscore list in the game always shows the first unsolved level at bottom. This way the player can always select it. This unsolved level is **not** stored in the file.

## `WINP`
The chunk contains two fixed length records describing position and size of game windows on a screen. The first record is for main window, the second is for level selector. Records are defined as follows:
```
int16 x;        horizontal window position (upper left corner to the screen upper left corner)
int16 y;        vertical window position (upper left corner to the screen upper left corner)
int16 w;        window width
int16 h;        window height
```

All the above values are expressed in pixels. Any negative values are replaced with 0, then later `OpenWindowTags()` function automatically adjusts the values to hardcoded limits and current screen size.

The level selector window is not resizable horizontally. Its width depends on the screen font. Width stored in the chunk is ignored.

## Storage
The game state is saved to `Untangle.state` file in the game directory (via `PROGDIR:` automatic assign). The state is automatically saved each time when the best score for a level is made, and when any of game windows is resized.

The state is loaded at the game start, then the first unsolved level is loaded automatically.
