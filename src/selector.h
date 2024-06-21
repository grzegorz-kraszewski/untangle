/*-----------------------------------*/
/* level selector / high score table */
/*-----------------------------------*/

#ifndef UNTANGLE_SELECTOR_H
#define UNTANGLE_SELECTOR_H

#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <exec/lists.h>

struct HighScore
{
	struct MinNode Node;
	LONG Seconds;
	LONG Moves;
};

struct Selector
{
	struct Window *Win;
	STRPTR WinTitle;
	struct TextFont *Font;
	ULONG SigMask;
	APTR HighScorePool;
	struct MinList HighScores;         /* list of HighScore structures */
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
};

void SelectorLayout(struct Window *mainwin, struct Selector *selector);
void OpenSelector(struct Window *mainwin, struct Selector *selector);
void HandleSelector(struct Selector *selector);
void HighScoreLevelCompleted(struct Selector *selector, LONG level, LONG seconds, LONG moves);

#endif    /* UNTANGLE_SELECTOR_H */