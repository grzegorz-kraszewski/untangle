/*-----------------------------------*/
/* level selector / high score table */
/*-----------------------------------*/

#ifndef UNTANGLE_SELECTOR_H
#define UNTANGLE_SELECTOR_H

#include <intuition/screens.h>
#include <intuition/intuition.h>

struct WinPosRecord
{
	WORD x;
	WORD y;
	WORD w;
	WORD h;
};

struct HighScore
{
	struct MinNode Node;
	LONG Seconds;
	LONG Moves;
};

#define THE_WORST_TIME_POSSIBLE          0x7FFFFFFF
#define THE_WORST_MOVECOUNT_POSSIBLE     0x7FFFFFFF

#define NO_LEVEL_CHANGE   -1

struct Selector
{
	struct Window *Win;
	STRPTR WinTitle;
	STRPTR ScreenTitle;                /* the same as app->DynamicScreenTitle */
	struct TextFont *Font;
	ULONG SigMask;
	APTR HighScorePool;
	struct MinList HighScores;         /* list of HighScore structures */
	BOOL ScrollerActive;
	WORD EntryCount;                   /* number of highscore list entries */
	WORD SlotsVisible;                 /* how many entries will fit into window */
	WORD FirstEntry;                   /* first displayed entry counted from 0 */
	WORD LevelREdge;
	WORD LevelLabelX;
	WORD TimeREdge;
	WORD TimeLabelX;
	WORD MovesREdge;
	WORD MovesLabelX;
	WORD InnerWidth;
	WORD TotalWidth;
	WORD ListYStart;
	WORD LineHeight;
	struct WinPosRecord SelWinPos;
};

void SelectorLayout(struct Window *mainwin, struct Selector *selector);
void OpenSelector(struct Window *mainwin, struct Selector *selector);
LONG HandleSelector(struct Selector *selector);
void HighScoreLevelCompleted(struct Selector *selector, LONG level, LONG seconds, LONG moves);

#endif    /* UNTANGLE_SELECTOR_H */