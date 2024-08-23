# Game state format
Game state consists of highscores of all the solved levels. The game state is stored as binary data in IFF container. IFF type is `UNTG` the same, as for level file. For now only one chunk, named `HSCR` is defined.
## HSCR
The chunk contains an array of fixed length records. Each record contains best score for a level. Level record is defined as follows:
```
int32 time;     the best time in seconds
int32 moves;    the lowest number of moves
```
As a level is not accessible until all the previous ones are solved, highscores are always ordered by level number and there are no gaps. Therefore level number is not explicitly stored.

The highscore list in the game always shows the first unsolved level at bottom. This way the player can always select it. This unsolved level is **not** stored in the file.
## Storage
The game state is saved to `Untangle.state` file in the game directory (via `PROGDIR:` automatic assign). The state is automatically saved each time when the best score for a level is made.

The state is loaded at the game start, then the first unsolved level is loaded automatically.
