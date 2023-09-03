#include "main.h"
#include "loader.h"

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>

#include <intuition/intuition.h>

#define MARGIN (DOT_SIZE / 2 + 1)
#define DOT_RADIUS (DOT_SIZE / 2)

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
	WORD dotradius = DOT_SIZE >> 1;
	struct GameDot *gd;
	
	ForEachFwd(&app->Level->DotList, struct GameDot, gd)
	{
		BltMaskBitMapRastPort(app->DotBitMap, 0, 0, rp, gd->Pixel.x - dotradius,
			gd->Pixel.y - dotradius, DOT_SIZE, DOT_SIZE, 0xE0, (APTR)app->DotRaster);
	}
}

/*---------------------------------------------------------------------------*/

void DrawGame(struct App *app)
{
	struct RastPort *rp = app->Win->RPort;
	SetAPen(rp, 1);
	SetDrMd(rp, JAM1);
	DrawLines(rp, &app->Level->LineList);
	DrawDots(rp, app);	
}

/*---------------------------------------------------------------------------*/

void ScaleGame(struct App *app)
{
	app->Field.MinX = app->Win->BorderLeft + MARGIN;
	app->Field.MinY = app->Win->BorderTop + MARGIN;
	app->Field.MaxX = app->Win->Width - app->Win->BorderRight - MARGIN - 1;
	app->Field.MaxY = app->Win->Height - app->Win->BorderBottom - MARGIN - 1;
	TransformAllDots(&app->Level->DotList, &app->Field);
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

static inline BOOL InsideBox(UWORD clkx, UWORD clky, UWORD dotx, UWORD doty)
{
	if (clkx < dotx - DOT_RADIUS) return FALSE;
	if (clkx > dotx + DOT_RADIUS) return FALSE;
	if (clky < doty - DOT_RADIUS) return FALSE;
	if (clky > doty + DOT_RADIUS) return FALSE;
	return TRUE;
}

/*---------------------------------------------------------------------------*/

BOOL DotClicked(struct App *app, struct GameDot* dot, UWORD x, UWORD y)
{
	if (InsideBox(dot->Pixel.x, dot->Pixel.y, x, y))
	{
		UWORD bbx = x + DOT_RADIUS - dot->Pixel.x;
		UWORD bby = y + DOT_RADIUS - dot->Pixel.y;
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
	BltTemplate((CONST PLANEPTR)app->DotRaster, 0, 2, app->Win->RPort, gd->Pixel.x - DOT_RADIUS,
	 gd->Pixel.y - DOT_RADIUS, DOT_SIZE, DOT_SIZE);
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
	MoveDraggedItemsBack(app->Level);
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

void NewGame(struct App *app)
{
	app->Level = LoadLevel(app->Win);
}
