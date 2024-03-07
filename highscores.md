# Hihgscores

The game stores the best time and minimal number of moves achieved for each level. Best time is stored as number of seconds. Both highscores are stored as 32-bit unsigned integers. In memory, highscores for all the levels solved in the current session are stored in an Exec bidirectional list. The structure storing highscores for a single level is defined as follows:
```
struct HighScore
{
  struct MinNode *Node;
  ULONG BestTime;
  ULONG MinMoves;
};
```
List header is placed in the `App` structure. To avoid memory fragmentation and make cleanup faster, `HighScore` structures are allocated from a memory pool. Pool pointer is placed in `App` too.

## Highscores management

### Program startup

* Memory pool pointer is set to NULL.

### Level set selection

* Memory pool is disposed if not NULL, then pointer is set to NULL.
* List header is reset to empty list.
* New memory pool is created.

### Level start

* `HighScore` for the level is searched using list linear traversing.
* If not found, a new `HighScore` is allocated from the pool, initialized and appended to the list. Both `BestTime` and `MinMoves` are set to 0, which means "level is not finished", as it is impossible to finish a level in 0 seconds making 0 moves.
> [!NOTE]
> It is assumed that user solves levels in linear order or uses level selector UI. Both ways allow to attempt level *n* only when level *n–1* is solved already. Then search for level *n* in the list:
> * either succeeds, it means user tries to improve on already solved level,
> * or fails, it means level *n* has not been solved yet, but all previous ones are solved. The last `HighScore` in the list is for level *n–1*, so new one for level *n* can be added at the end.
>
> This approach solves edge case of level 0 automatically. 

### Level completed

* Time and move counter are compared to best results, and if a new high score has been achieved, the structure is updated.

### Program exit

* Memory pool is disposed if not NULL.
