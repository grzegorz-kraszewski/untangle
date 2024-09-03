/*-----------------------------------*/
/* level selector / high score table */
/*-----------------------------------*/

#include "main.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/rpattr.h>

/* for tests */

#include <proto/dos.h>

/* multiplication by 0xFFFF made easy */

#define PROPSCALE(x) (((ULONG)(x) << 16) - x) 


void PrintHighScores(struct Selector *selector);

struct Image ScrRender = {0};
struct PropInfo ScrProp = {0};
struct Gadget Scroller = {0};


/*--------------------------------------------------------------------------------------------*/

static struct HighScore* CreateFirstTimeHighScore(struct Selector *selector)
{
	struct HighScore *hscore;

	hscore = AllocPooled(selector->HighScorePool, sizeof(struct HighScore));
	if (!hscore) return NULL;
	hscore->Seconds = THE_WORST_TIME_POSSIBLE;
	hscore->Moves = THE_WORST_MOVECOUNT_POSSIBLE;
	return hscore;
}

/*-------------------------------------------------------------------------------------------*/
/* The highscore list should always contain entry for the first unsolved level, so it can be */
/* selected by player. The function checks if the last entry in the list is unsolved. If no  */
/* it inserts one. Does not call selector refresh.                                           */
/*-------------------------------------------------------------------------------------------*/

static void InsertFirstUnsolved(struct Selector *selector)
{
	struct HighScore *last;

	last = (struct HighScore*)selector->HighScores.mlh_TailPred;

	/* if highscore list is not empty and the last one is unsolved, do nothing. */

	if (last && (last->Seconds == THE_WORST_TIME_POSSIBLE)) return;

	if (last = CreateFirstTimeHighScore(selector))
	{
		AddTail((struct List*)&selector->HighScores, (struct Node*)last);
		selector->EntryCount++;
	}
}


/*-------------------------------------------------------------------------------------------*/
/* This function does not require level selector window to be opened. Main window RastPort   */
/* is used, as both windows are on the same screen and use the same font. Therefore it is    */
/* called at game startup, after opening the main window.                                    */
/*-------------------------------------------------------------------------------------------*/

void SelectorLayout(struct Window *mainwin, struct Selector *selector)
{
	struct RastPort *rp;
	struct TagItem tags[2] = {{ RPTAG_Font, 0 }, { TAG_END, 0 }};
	WORD unit, len_level, len_time, len_moves;

	rp = mainwin->RPort;
	tags[0].ti_Data = (LONG)&selector->Font;
	GetRPAttrsA(rp, tags);
	unit = (selector->Font->tf_YSize >> 2) + 1;
	len_level = TextLength(rp, "Level", 5);
	len_time = TextLength(rp, "Time", 4);
	len_moves = TextLength(rp, "Moves", 5);
	
	selector->LevelREdge = mainwin->BorderLeft + unit + len_level;
	selector->LevelLabelX = selector->LevelREdge - len_level;
	selector->TimeREdge = selector->LevelREdge + unit + TextLength(rp, " 00:00", 6);
	selector->TimeLabelX = selector->TimeREdge - len_time;
	selector->MovesREdge = selector->TimeREdge + unit + TextLength(rp, "00000", 5);
	selector->MovesLabelX = selector->MovesREdge - len_moves;
	selector->InnerWidth = selector->MovesREdge + unit;
	selector->TotalWidth = selector->InnerWidth + mainwin->BorderRight;
	selector->FirstEntry = 0x7FFF;

	InsertFirstUnsolved(selector);
}

/*-------------------------------------------------------------------------------------------*/
/* After selector window is made bigger, adjust FirstEntry to show as many rows as possible. */         
/*-------------------------------------------------------------------------------------------*/

static void ScrollEntriesToFillWindow(struct Selector *selector)
{
	LONG to_be_shown;

	to_be_shown = selector->EntryCount - selector->FirstEntry;
	if (to_be_shown < selector->SlotsVisible) selector->FirstEntry -= selector->SlotsVisible - to_be_shown;
	if (selector->FirstEntry < 0) selector->FirstEntry = 0;
}

/*-------------------------------------------------------------------------------------------*/
/* Called when level selector needs to be redrawn, or prop gadget recalculated.              */
/*-------------------------------------------------------------------------------------------*/

void SelectorRefresh(struct Selector *selector)
{
	UWORD pot, body;
	
	if (!selector->Win) return;
	
	selector->SlotsVisible = div16(selector->Win->Height - selector->Win->BorderTop -
		selector->Win->BorderBottom - 1, selector->Font->tf_YSize) - 1;

	ScrollEntriesToFillWindow(selector);

	pot = 0;
	body = MAXBODY;

	if (selector->SlotsVisible < selector->EntryCount)
	{
		body = divu16(PROPSCALE(selector->SlotsVisible), selector->EntryCount);
		pot = divu16(PROPSCALE(selector->FirstEntry), selector->EntryCount - selector->SlotsVisible);
	}

	NewModifyProp(&Scroller, selector->Win, NULL, FREEVERT | PROPNEWLOOK | PROPBORDERLESS |
	 AUTOKNOB, 0, pot, MAXBODY, body, 1);

	PrintHighScores(selector);
}


/*-------------------------------------------------------------------------------------------*/

void OpenSelector(struct Window *mainwin, struct Selector *selector)
{
	STRPTR title;

	ScrProp.Flags = FREEVERT | PROPNEWLOOK | PROPBORDERLESS | AUTOKNOB;
	ScrProp.HorizPot = 0;
	ScrProp.HorizBody = MAXBODY;
	ScrProp.VertPot = 0;
	ScrProp.VertBody = MAXBODY;

	Scroller.TopEdge = mainwin->BorderTop + 2;
	Scroller.LeftEdge = -mainwin->BorderRight + 5;
	Scroller.Width = mainwin->BorderRight - 8;
	Scroller.Height = -Scroller.TopEdge - 12;
	Scroller.Flags = GFLG_GADGHNONE | GFLG_RELRIGHT | GFLG_RELHEIGHT;
	Scroller.Activation = GACT_RELVERIFY | GACT_IMMEDIATE | GACT_FOLLOWMOUSE | GACT_RIGHTBORDER;
	Scroller.GadgetType = GTYP_PROPGADGET;
	Scroller.GadgetRender = &ScrRender;
	Scroller.SpecialInfo = &ScrProp;

	if (selector->Win = OpenWindowTags(NULL,
		WA_Top, selector->SelWinPos.y,
		WA_Left, selector->SelWinPos.x,
		WA_Width, selector->TotalWidth,
		WA_MinWidth, selector->TotalWidth,
		WA_MaxWidth, selector->TotalWidth,
		WA_Height, selector->SelWinPos.h,
		WA_MinHeight, 100,
		WA_MaxHeight, 1024,
		WA_DragBar, TRUE,
		WA_CloseGadget, TRUE,
		WA_DepthGadget, TRUE,
		WA_SizeGadget, TRUE,
		WA_Title, selector->WinTitle,
		WA_ScreenTitle, selector->ScreenTitle,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_GADGETUP | IDCMP_GADGETDOWN |
			IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | IDCMP_CHANGEWINDOW,
		WA_Gadgets, (ULONG)&Scroller,
		WA_NewLookMenus, TRUE,
		WA_Activate, TRUE,
	TAG_END))
	{
		StoreWindowPosition(selector->Win, &selector->SelWinPos);
		selector->SigMask = 1 << selector->Win->UserPort->mp_SigBit;
		selector->ScrollerActive = FALSE;
		SelectorRefresh(selector);
	}
}

/*--------------------------------------------------------------------------------------------*/
/* User has moved the scroller. Redraw the content accordingly.                               */

static void ScrollContent(struct Selector *selector)
{
	WORD newfirst;

	newfirst = selector->EntryCount - selector->SlotsVisible;

	if (newfirst > 0)
	{
		newfirst = divu16(mulu16(newfirst, ScrProp.VertPot), MAXPOT);

		if (newfirst != selector->FirstEntry)
		{
			selector->FirstEntry = newfirst;
			PrintHighScores(selector);
		}
	}
}

/*--------------------------------------------------------------------------------------------*/

static LONG HandleClick(struct Selector *selector, WORD x, WORD y)
{
	WORD unit = (selector->Font->tf_YSize >> 2) + 1;

	if (x < selector->Win->BorderLeft + unit) return NO_LEVEL_CHANGE;
	if (x >= selector->MovesREdge) return NO_LEVEL_CHANGE;
	y -= selector->ListYStart;
	if (y < 0) return NO_LEVEL_CHANGE;
	y = divu16(y, selector->LineHeight);
	y += selector->FirstEntry;
	if (y >= selector->EntryCount) return NO_LEVEL_CHANGE; /* handle first unsolved level here */
	return y;
}

/*--------------------------------------------------------------------------------------------*/
/* Called when the main loop receives signal from selector window IDCMP port. Returns level   */
/* number to load or NO_LEVEL_CHANGE.                                                         */

LONG HandleSelector(struct Selector *selector)
{
	struct IntuiMessage *imsg;
	LONG result = NO_LEVEL_CHANGE;
	BOOL closed = FALSE;

	while (imsg = (struct IntuiMessage*)GetMsg(selector->Win->UserPort))
	{
		switch (imsg->Class)
		{
			case IDCMP_CLOSEWINDOW:
				closed = TRUE;
			break;
			
			case IDCMP_NEWSIZE:
				SelectorRefresh(selector);
			break;

			case IDCMP_GADGETUP:
				if (imsg->IAddress == &Scroller) selector->ScrollerActive = FALSE;
			break;

			case IDCMP_GADGETDOWN:
				if (imsg->IAddress == &Scroller)
				{
					selector->ScrollerActive = TRUE;
					ScrollContent(selector);
				}
			break;

			case IDCMP_MOUSEMOVE:
				if (selector->ScrollerActive) ScrollContent(selector);
			break;

			case IDCMP_MOUSEBUTTONS:
				if (imsg->Code == SELECTDOWN)
					result = HandleClick(selector, imsg->MouseX, imsg->MouseY);
			break;

			case IDCMP_CHANGEWINDOW:
				StoreWindowPosition(selector->Win, &selector->SelWinPos);
			break;
		}

		ReplyMsg(&imsg->ExecMessage);
	}

	if (closed)
	{
		CloseWindow(selector->Win);
		selector->Win = NULL;
		selector->SigMask = 0;
	}

	return result;
}

/*--------------------------------------------------------------------------------------------*/

static void PrintHighScoreHeader(struct Selector *selector, WORD y)
{
	struct RastPort *rp = selector->Win->RPort;
	
	Move(rp, selector->LevelLabelX, y);
	Text(rp, "Level", 5);
	Move(rp, selector->TimeLabelX, y);
	Text(rp, "Time", 4);
	Move(rp, selector->MovesLabelX, y);
	Text(rp, "Moves", 5);
}

/*--------------------------------------------------------------------------------------------*/
/* As any item in the listing may be shorter (in pixels) than previous one, using JAM2 text   */
/* mode is not enough, each line has to be pre-erased.                                        */
/*--------------------------------------------------------------------------------------------*/

static void EraseHighScoreLine(struct Selector *selector, WORD y)
{
	y -= selector->Font->tf_Baseline;
	EraseRect(selector->Win->RPort, selector->LevelLabelX, y, selector->MovesREdge,
		y + selector->Font->tf_YSize - 1);
}

/*--------------------------------------------------------------------------------------------*/

static void SecondsToTime(LONG time, UBYTE *buf)
{
	LONG p[2];

	if (time < 6000)
	{
		p[0] = div16(time, 60);
		p[1] = time - mul16(p[0], 60);
		VFmtPut(buf, "%ld:%02ld", p);
	}
	else StrCopy("--:--", buf);
}

/*--------------------------------------------------------------------------------------------*/

static void PrintHighScoreLine(struct Selector *selector, struct HighScore *hs, LONG lvl,
 WORD y)
{
	LONG txtlen, p[1];
	WORD x;
	static UBYTE outbuf[16];
	struct RastPort *rp = selector->Win->RPort;

	EraseHighScoreLine(selector, y);
		
	p[0] = lvl;
	VFmtPut(outbuf, "%ld", p);
	txtlen = StrLen(outbuf);	
	x = selector->LevelREdge - TextLength(rp, outbuf, txtlen);
	Move(rp, x, y);
	Text(rp, outbuf, txtlen);
	
	if (hs->Seconds == THE_WORST_TIME_POSSIBLE) StrCopy("--", outbuf);
	else SecondsToTime(hs->Seconds, outbuf);
	txtlen = StrLen(outbuf);	
	x = selector->TimeREdge - TextLength(rp, outbuf, txtlen);
	Move(rp, x, y);
	Text(rp, outbuf, txtlen);

	if ((p[0] = hs->Moves) == THE_WORST_MOVECOUNT_POSSIBLE) StrCopy("--", outbuf);
	else VFmtPut(outbuf, "%ld", p);
	txtlen = StrLen(outbuf);	
	x = selector->MovesREdge - TextLength(rp, outbuf, txtlen);
	Move(rp, x, y);
	Text(rp, outbuf, txtlen);
}

/*--------------------------------------------------------------------------------------------*/

static struct HighScore* FindHighScoreByLevelNumber(struct MinList *hslist, LONG level)
{
	LONG level_counter = 1;
	struct HighScore *hs;
	
	ForEachFwd(hslist, struct HighScore, hs)
	{
		if (level == level_counter) return hs;
		level_counter++;
	} 
	
	return NULL;
}

/*--------------------------------------------------------------------------------------------*/

void PrintHighScores(struct Selector *selector)
{
	WORD vpos, hpos, level, lcount = 0;
	struct DrawInfo *dri;
	struct RastPort *rp;	
	struct TextFont *font;
	struct HighScore *hs;
	
	if (!selector->Win) return;  /* selector window is closed (possibly redundant test) */

	dri = GetScreenDrawInfo(selector->Win->WScreen);
	
	rp = selector->Win->RPort;
	font = dri->dri_Font;
	SetFont(rp, font);
	SetAPen(rp, 1);
	vpos = selector->Win->BorderTop + font->tf_Baseline + 1;
	hpos = selector->Win->BorderLeft;
	PrintHighScoreHeader(selector, vpos);
	vpos += font->tf_YSize;
	level = selector->FirstEntry + 1;
	selector->ListYStart = selector->Win->BorderTop + font->tf_YSize + 1;
	selector->LineHeight = font->tf_YSize;

	// Printf("listing starts at level %ld.\n", level);
	
	hs = FindHighScoreByLevelNumber(&selector->HighScores, level); 

	while(hs && hs->Node.mln_Succ && (lcount++ < selector->SlotsVisible))
	{
		PrintHighScoreLine(selector, hs, level++, vpos);
		hs = (struct HighScore*)hs->Node.mln_Succ;
		vpos += font->tf_YSize; 
	}
	
	SetAPen(rp, 0);
	RectFill(rp, selector->Win->BorderLeft, vpos - selector->Font->tf_Baseline,
	 selector->Win->Width - selector->Win->BorderRight - 1, selector->Win->Height - 
	 selector->Win->BorderBottom - 1);
	FreeScreenDrawInfo(selector->Win->WScreen, dri);
}


/*--------------------------------------------------------------------------------------------*/
/* Number of moves has priority over time. If a level is beaten in less moves than the        */
/* current best, the new score becomes best, regardless of solving time. On the other hand    */
/* shorter time is not enough, if more moves have been made.                                  */
/*--------------------------------------------------------------------------------------------*/

static BOOL BestScore(struct HighScore *current_best, LONG seconds, LONG moves)
{
	if (moves < current_best->Moves)
	{
		current_best->Moves = moves;
		current_best->Seconds = seconds;
		return TRUE;
	}

	if ((moves == current_best->Moves) && (seconds < current_best->Seconds))
	{
		current_best->Seconds = seconds;
		return TRUE;
	}

	return FALSE;	
}

/*--------------------------------------------------------------------------------------------*/

void HighScoreLevelCompleted(struct Selector *selector, LONG level, LONG seconds, LONG moves)
{
	struct HighScore *hscore;
	BOOL new_high = FALSE;

	hscore = FindHighScoreByLevelNumber(&selector->HighScores, level);

	if (!hscore) return; /* panic, should never happen */

	if (BestScore(hscore, seconds, moves)) selector->FirstEntry++; /* WTF is this? */
	InsertFirstUnsolved(selector);
	SelectorRefresh(selector);
}
