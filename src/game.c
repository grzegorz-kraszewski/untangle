#include "main.h"
#include "loader.h"
#include "lscm.h"
#include "strutils.h"
#include "version.h"

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/timer.h>

#include <intuition/intuition.h>

static void PrintLevelInfo(struct App *app);

/*---------------------------------------------------------------------------*/
/* Game levels are generated in virtual coordinate system: a square of 32768 */
/* x 32768 units, zero in upper left corner. At start and after every window */
/* resize dot coordinates are transformed to the game area in window         */
/* RastPort. Transformation does not mainatin aspect ratio, game is squeezed */
/* as needed to fit the game area.                                           */
/*                                                                           */
/* When a dot is dragged, everything is done in RastPort coordinates. When   */
/* dragging is finished, virtual coordinates of the dot are updated with     */
/* inverse transformation. This is the only place where division is needed.  */
/*                                                                           */
/* Level completion test is done on virtual coordinates.                     */
/*---------------------------------------------------------------------------*/

static void TransformAllDots(struct MinList *dots, struct Rectangle *field)
{
	struct GameDot *gd;
	WORD fieldw = field->MaxX - field->MinX + 1;
	WORD fieldh = field->MaxY - field->MinY + 1;

	ForEachFwd(dots, struct GameDot, gd)
	{
		gd->Pixel.x = (mul16(gd->Virtual.x, fieldw) >> 15) + field->MinX;
		gd->Pixel.y = (mul16(gd->Virtual.y, fieldh) >> 15) + field->MinY;
	}
}

/*---------------------------------------------------------------------------*/

static inline void InverseTransformDot(struct GameDot *gd, struct Rectangle *field)
{
	WORD fieldw = field->MaxX - field->MinX + 1;
	WORD fieldh = field->MaxY - field->MinY + 1;
	gd->Virtual.x = div16((gd->Pixel.x - field->MinX) << 15, fieldw);
	gd->Virtual.y = div16((gd->Pixel.y - field->MinY) << 15, fieldh);
}

/*---------------------------------------------------------------------------*/

void DrawLines(struct RastPort *rp, struct MinList *lines)
{
	struct GameLine *gl;

	ForEachFwd(lines, struct GameLine, gl)
	{
		Move(rp, gl->StartDot->Pixel.x, gl->StartDot->Pixel.y);
		Draw(rp, gl->EndDot->Pixel.x, gl->EndDot->Pixel.y);
	}
}

/*---------------------------------------------------------------------------*/

void DrawDots(struct RastPort *rp, struct App *app)
{
	WORD dotradius_x = app->DotWidth >> 1;
	WORD dotradius_y = app->DotHeight >> 1;
	struct GameDot *gd;

	SetDrMd(rp, JAM1);
	SetAPen(rp, 1);

	ForEachFwd(&app->Level->DotList, struct GameDot, gd)
	{
		BltTemplate(app->DotRaster, 0, 2, rp, gd->Pixel.x - dotradius_x, gd->Pixel.y -
			dotradius_y, app->DotWidth, app->DotHeight);
	}

	SetAPen(rp, 2);

	ForEachFwd(&app->Level->DotList, struct GameDot, gd)
	{
		BltTemplate(&app->DotRaster[app->DotHeight], 0, 2, rp, gd->Pixel.x - dotradius_x, gd->Pixel.y -
			dotradius_y, app->DotWidth, app->DotHeight);
	}

}

/*---------------------------------------------------------------------------*/

static void DrawInfoBar(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;
	WORD xs = app->Win->BorderLeft;
	WORD xe = app->Win->Width - app->Win->BorderRight - 1;
	WORD y = app->InfoBarY;
	ULONG chars;
	struct TextExtent te;

	SetAPen(rp, 1);
	SetFont(rp, app->InfoFont);
	RectFill(rp, xs, y, xe, y);
	SetAPen(rp, 2);
	RectFill(rp, xs, y + 1, xe, y + 1);
	PrintLevelInfo(app);
}

/*---------------------------------------------------------------------------*/

void DrawGame(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;
	SetAPen(rp, 1);
	SetDrMd(rp, JAM1);

	if (app->Level)
	{
		DrawLines(rp, &app->Level->LineList);
		DrawDots(rp, app);
	}

	DrawInfoBar(app);
}

/*---------------------------------------------------------------------------*/

void ScaleGame(struct App *app)
{
	WORD margin_x = (app->DotWidth >> 1) + 1;
	WORD margin_y = (app->DotHeight >> 1) + 1;
	WORD fonth = app->InfoFont->tf_YSize;
	WORD infobarh = fonth + (fonth >> 3) + (fonth >> 3) + 2;

	app->Field.MinX = app->Win->BorderLeft + margin_x;
	app->Field.MinY = app->Win->BorderTop + margin_y;
	app->Field.MaxX = app->Win->Width - app->Win->BorderRight - margin_x - 1;
	app->Field.MaxY = app->Win->Height - app->Win->BorderBottom - margin_y - infobarh - 1;

	if (app->Level) TransformAllDots(&app->Level->DotList, &app->Field);

	/* InfoBar */

	app->InfoBarY = app->Field.MaxY + margin_y;
	app->InfoText.x = app->Field.MinX;
	app->InfoText.y = app->InfoBarY + 2 + (fonth >> 3) + app->InfoFont->tf_Baseline;
}

/*---------------------------------------------------------------------------*/

void EraseGame(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;
	SetAPen(rp, 0);
	SetDrMd(rp, JAM1);
	RectFill(rp, app->Win->BorderLeft, app->Win->BorderTop, app->Win->Width -
	 app->Win->BorderRight - 1, app->Win->Height - app->Win->BorderBottom - 1);
}

/*---------------------------------------------------------------------------*/

static inline BOOL MaskHit(UWORD x, UWORD y, CONST UWORD *mask)
{
	return (mask[y] & (0x8000 >> x));
}

/*---------------------------------------------------------------------------*/

static inline BOOL InsideBox(UWORD clkx, UWORD clky, UWORD dotx, UWORD doty,
	UWORD dotrx, UWORD dotry)
{
	if (clkx < dotx - dotrx) return FALSE;
	if (clkx > dotx + dotrx) return FALSE;
	if (clky < doty - dotry) return FALSE;
	if (clky > doty + dotry) return FALSE;
	return TRUE;
}

/*---------------------------------------------------------------------------*/

BOOL DotClicked(struct App *app, struct GameDot* dot, UWORD x, UWORD y)
{
	UWORD dotrx = app->DotWidth >> 1;
	UWORD dotry = app->DotHeight >> 1;

	if (InsideBox(dot->Pixel.x, dot->Pixel.y, x, y, dotrx, dotry))
	{
		UWORD bbx = x + dotrx - dot->Pixel.x;
		UWORD bby = y + dotry - dot->Pixel.y;
		if (MaskHit(bbx, bby, app->DotRaster)) return TRUE;
	}

	return FALSE;
}

/*---------------------------------------------------------------------------*/

struct GameDot* FindClickedDot(struct App *app, UWORD x, UWORD y)
{
	struct GameDot *gd;

	ForEachRev(&app->Level->DotList, struct GameDot, gd)
	{
		if (DotClicked(app, gd, x, y)) return gd;
	}

	return NULL;
}

/*---------------------------------------------------------------------------*/

static inline void MoveDraggedDot(struct GameLevel *glv, struct GameDot *clicked)
{
	Remove((struct Node*)clicked);
	glv->DraggedDot = clicked;
}

/*---------------------------------------------------------------------------*/

static inline void MoveDraggedLines(struct GameLevel *glv, struct GameDot *clicked)
{
	struct GameLine *gl, *gl2;

	gl = (struct GameLine*)glv->LineList.mlh_Head;

	while (gl->Node.mln_Succ)
	{
		gl2 = (struct GameLine*)gl->Node.mln_Succ;

		if ((gl->StartDot == clicked) || (gl->EndDot == clicked))
		{
			Remove((struct Node*)gl);
			AddTail((struct List*)&glv->DraggedLines, (struct Node*)gl);
		}

		gl = gl2;
	}
}

/*---------------------------------------------------------------------------*/

void MoveDraggedItems(struct GameLevel *glv, struct GameDot *clicked)
{
	MoveDraggedDot(glv, clicked);
	MoveDraggedLines(glv, clicked);
}

/*---------------------------------------------------------------------------*/

static inline void MoveDraggedDotBack(struct GameLevel *glv)
{
	AddTail((struct List*)&glv->DotList, (struct Node*)glv->DraggedDot);
	glv->DraggedDot = NULL;
}

/*---------------------------------------------------------------------------*/

static inline void MoveDraggedLinesBack(struct GameLevel *glv)
{
	struct GameLine *gl;

	while (gl = (struct GameLine*)RemHead((struct List*)&glv->DraggedLines))
	{
		AddTail((struct List*)&glv->LineList, (struct Node*)gl);
	}
}

/*---------------------------------------------------------------------------*/

void MoveDraggedItemsBack(struct GameLevel *glv)
{
	MoveDraggedDotBack(glv);
	MoveDraggedLinesBack(glv);
}

/*---------------------------------------------------------------------------*/

void EraseDot(struct App *app, struct GameDot *gd)
{
	WORD dotrx = app->DotWidth >> 1;
	WORD dotry = app->DotHeight >> 1;

	BltTemplate((CONST PLANEPTR)app->DotRaster, 0, 2, app->Win->RPort, gd->Pixel.x - dotrx,
	 gd->Pixel.y - dotry, app->DotWidth, app->DotHeight);
}

/*---------------------------------------------------------------------------*/

void DrawDraggedItems(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;

	SetDrMd(rp, JAM1 | COMPLEMENT);
	SetAPen(rp, 1);
	DrawLines(rp, &app->Level->DraggedLines);
	EraseDot(app, app->Level->DraggedDot);
}

/*---------------------------------------------------------------------------*/

void EraseDraggedItems(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;

	SetAPen(rp, 0);
	EraseDot(app, app->Level->DraggedDot);
	DrawLines(rp, &app->Level->DraggedLines);
}

/*---------------------------------------------------------------------------*/

void GameClick(struct App *app, WORD x, WORD y)
{
	struct GameDot *clicked;

	if (app->Level)
	{
		if (clicked = FindClickedDot(app, x, y))
		{
			MoveDraggedItems(app->Level, clicked);
			EraseDraggedItems(app);
			DrawGame(app);
			DrawDraggedItems(app);
			app->Win->Flags |= WFLG_REPORTMOUSE;
		}
	}
}

/*---------------------------------------------------------------------------*/

static inline void UpdateDragPosition(struct GameDot *gd, struct Rectangle *field, WORD x, WORD y)
{
	if (x < field->MinX) x = field->MinX;
	if (x > field->MaxX) x = field->MaxX;
	if (y < field->MinY) y = field->MinY;
	if (y > field->MaxY) y = field->MaxY;
	gd->Pixel.x = x;
	gd->Pixel.y = y;
}

/*---------------------------------------------------------------------------*/

void GameUnclick(struct App *app, WORD x, WORD y)
{
	app->Win->Flags &= ~WFLG_REPORTMOUSE;
	DrawDraggedItems(app);
	UpdateDragPosition(app->Level->DraggedDot, &app->Field, x, y);
	InverseTransformDot(app->Level->DraggedDot, &app->Field);
	UpdateIntersections(app->Level);
	app->Level->MoveCount++;
	MoveDraggedItemsBack(app->Level);
	PrintLevelInfo(app);
	DrawGame(app);
}

/*---------------------------------------------------------------------------*/

void GameDotDrag(struct App *app, WORD x, WORD y)
{
	DrawDraggedItems(app);
	UpdateDragPosition(app->Level->DraggedDot, &app->Field, x, y);
	DrawDraggedItems(app);
}

/*---------------------------------------------------------------------------*/
/* - Updates screen bar with level set name and author.                      */
/* - Updates window bar with level set name and number of level.             */
/* - Updates bottom info bar.                                                */
/*---------------------------------------------------------------------------*/

void UpdateInfosAfterLevelLoad(struct App *app)
{
	STRPTR title;

	if (title = FmtNew("Untangle " VERSION ": %s by %s", (LONG)app->Level->LevelSetName,
	(LONG)app->Level->LevelSetAuthor))
	{
		if (app->DynamicScreenTitle) StrFree(app->DynamicScreenTitle);
		app->DynamicScreenTitle = title;
	}

	if (title = FmtNew("Untangle: %s, Level %ld", (LONG)app->Level->LevelSetName,
	app->LevelNumber))
	{
		if (app->DynamicWindowTitle) StrFree(app->DynamicWindowTitle);
		app->DynamicWindowTitle = title;
	}

	if (title = FmtNew("Untangle: %s", (LONG)app->Level->LevelSetName))
	{
		if (app->Selector.WinTitle) StrFree(app->Selector.WinTitle);
		app->Selector.WinTitle = title;
	}

	SetWindowTitles(app->Win, app->DynamicWindowTitle, app->DynamicScreenTitle);
	if (app->Selector.Win) SetWindowTitles(app->Selector.Win, app->Selector.WinTitle,
		app->DynamicScreenTitle);
}

/*---------------------------------------------------------------------------*/

static void PrintLevelTime(struct App *app)
{
	static UBYTE tbuf[32];
	struct RastPort *rp = app->Win->RPort;

	FmtPut(tbuf, "%ld:%02ld", app->LevelTime.Min, app->LevelTime.Sec);
	SetDrMd(rp, JAM2);
	SetAPen(rp, 1);
	SetBPen(rp, 0);
	Move(rp, app->TimeTextX, app->InfoText.y);
	Text(rp, tbuf, StrLen(tbuf));
	SetAPen(rp, 0);
	RectFill(rp, rp->cp_x, app->InfoText.y - rp->TxBaseline, app->Field.MaxX,
		app->InfoText.y - rp->TxBaseline + rp->TxHeight);
}

/*---------------------------------------------------------------------------*/

static void PrintLevelInfo(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;

	if (app->CurrentInfoText) StrFree(app->CurrentInfoText);

	if (app->Level)
	{
		app->CurrentInfoText = FmtNew("Intersections: %ld  Moves: %ld  Time: ",
			app->Level->InterCount, app->Level->MoveCount);
		SetDrMd(rp, JAM2);
		SetAPen(rp, 1);
		SetBPen(rp, 0);
		Move(rp, app->InfoText.x, app->InfoText.y);
		Text(rp, app->CurrentInfoText, StrLen(app->CurrentInfoText));
		app->TimeTextX = rp->cp_x;
		PrintLevelTime(app);
	}
}

/*---------------------------------------------------------------------------*/

void StopTimer(struct App *app)
{
	AbortIO(&app->TimerReq->tr_node);
	WaitIO(&app->TimerReq->tr_node);
}

/*---------------------------------------------------------------------------*/

void PushNextSecond(struct App *app, BOOL redraw)
{
	if (redraw) PrintLevelTime(app);
	app->LevelTime.Sec++;

	if (app->LevelTime.Sec == 60)
	{
		app->LevelTime.Sec = 0;
		app->LevelTime.Min++;
	}

	app->NextSecond.tv_secs++;
	app->TimerReq->tr_node.io_Command = TR_ADDREQUEST;
	app->TimerReq->tr_time = app->NextSecond;
	SendIO(&app->TimerReq->tr_node);
}

/*---------------------------------------------------------------------------*/

static void StartTimer(struct App *app)
{
	app->LevelTime.Sec = 0;
	app->LevelTime.Min = 0;
	GetSysTime(&app->LevelStart);
	app->NextSecond = app->LevelStart;
	PushNextSecond(app, TRUE);
}

/*---------------------------------------------------------------------------*/

void NewGame(struct App *app)
{
	BPTR olddir;

	app->LevelTime.Min = 0;
	app->LevelTime.Sec = 0;
	if (app->LevelSetFile.wa_Lock) olddir = CurrentDir(app->LevelSetFile.wa_Lock);

	if (app->Level = LoadLevel(app->Win, app->LevelNumber, app->LevelSetFile.wa_Name))
	{
		PrecalculateLevel(app->Level);
		ScaleGame(app);
		UpdateInfosAfterLevelLoad(app);
		DrawGame(app);
		StartTimer(app);
	}

	if (app->LevelSetFile.wa_Lock) CurrentDir(olddir);
}
