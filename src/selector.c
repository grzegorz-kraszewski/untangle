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

/*-------------------------------------------------------------------------------------------*/
/* This function does not require level selector window to be opened. Main window RastPort   */
/* is used, as both windows are on the same screen and use the same font. Therefore it is    */
/* called at game startup, after opening the main window.                                   */
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
	selector->TimeREdge = selector->LevelREdge + unit + TextLength(rp, "00:00", 5);
	selector->TimeLabelX = selector->TimeREdge - len_time;
	selector->MovesREdge = selector->TimeREdge + unit + TextLength(rp, "00000", 5);
	selector->MovesLabelX = selector->MovesREdge - len_moves;
	selector->InnerWidth = selector->MovesREdge + unit;
	selector->TotalWidth = selector->InnerWidth + mainwin->BorderRight;
	selector->FirstEntry = 0;
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

	Printf("SlotsVisible = %ld, EntryCount = %ld.\n", selector->SlotsVisible, selector->EntryCount);
	
	if (selector->SlotsVisible < selector->EntryCount)
	{
		body = udiv16(PROPSCALE(selector->SlotsVisible), selector->EntryCount);
		pot = udiv16(PROPSCALE(selector->FirstEntry), selector->EntryCount - selector->SlotsVisible);
		Printf("[t: %ld, f: %ld, v: %ld] new body = %ld, pot = %ld.\n", selector->EntryCount,
			selector->FirstEntry, selector->SlotsVisible, body, pot);
	}

	NewModifyProp(&Scroller, selector->Win, NULL, FREEVERT | PROPNEWLOOK | PROPBORDERLESS |
	 AUTOKNOB, 0, pot, MAXBODY, body, 1);

	Printf("Prop modified to (%ld, %ld).\n",ScrProp.VertPot, ScrProp.VertBody);
		
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
		WA_Width, selector->TotalWidth,
		WA_MinWidth, selector->TotalWidth,
		WA_MaxWidth, selector->TotalWidth,
		WA_Height, 400,
		WA_MinHeight, 100,
		WA_MaxHeight, 1024,
		WA_DragBar, TRUE,
		WA_CloseGadget, TRUE,
		WA_DepthGadget, TRUE,
		WA_SizeGadget, TRUE,
		WA_Title, selector->WinTitle,
		//WA_ScreenTitle, app->DynamicScreenTitle,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_GADGETUP | IDCMP_GADGETDOWN |
			IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS,
		WA_Gadgets, (ULONG)&Scroller,
		WA_NewLookMenus, TRUE,
		WA_Activate, TRUE,
	TAG_END))
	{
		selector->SigMask = 1 << selector->Win->UserPort->mp_SigBit;
		SelectorRefresh(selector);
	}
}

/*--------------------------------------------------------------------------------------------*/
/* Called when the main loop receives signal from selector window IDCMP port.                 */

void HandleSelector(struct Selector *selector)
{
	struct IntuiMessage *imsg;
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
				Printf("GAD UP\n");
			break;

			case IDCMP_GADGETDOWN:
				Printf("GAD DOWN\n");
			break;

			case IDCMP_MOUSEMOVE:
				Printf("MMOVE\n");
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
	
	p[0] = hs->Seconds;
	VFmtPut(outbuf, "%ld", p);
	txtlen = StrLen(outbuf);	
	x = selector->TimeREdge - TextLength(rp, outbuf, txtlen);
	Move(rp, x, y);
	Text(rp, outbuf, txtlen);

	p[0] = hs->Moves;
	VFmtPut(outbuf, "%ld", p);
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

	Printf("listing starts at level %ld.\n", level);
	
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

static struct HighScore* CreateFirstTimeHighScore(struct Selector *selector)
{
	struct HighScore *hscore;

	hscore = AllocPooled(selector->HighScorePool, sizeof(struct HighScore));
	if (!hscore) return NULL;
	hscore->Seconds = THE_WORST_TIME_POSSIBLE;
	hscore->Moves = THE_WORST_MOVECOUNT_POSSIBLE;
	return hscore;
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

	if (!hscore)
	{
		if (hscore = CreateFirstTimeHighScore(selector))
		{
			AddTail((struct List*)&selector->HighScores, (struct Node*)hscore);
			selector->EntryCount++;
		}
	}

	if (!hscore) return;
	if (BestScore(hscore, seconds, moves))
	{
		selector->FirstEntry++;
		SelectorRefresh(selector);
	}
}
