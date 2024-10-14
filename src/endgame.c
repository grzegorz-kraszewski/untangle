//-------------
// game ending
//-------------

#include "main.h"
#include "strutils.h"
#include "selector.h"

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/layers.h>
#include <graphics/rpattr.h>

#define SEGMENT_UP     0
#define SEGMENT_LEFT   1
#define SEGMENT_DOWN   2
#define SEGMENT_RIGHT  3 

struct EndGameText
{
	char *text;
	LONG len;
};

//---------------------------------------------------------------------------------------------

static inline short addrot(short rot1, short rot2)
{
	return (rot1 + rot2) & 0x3;
}

//---------------------------------------------------------------------------------------------

static void deltaDraw(struct RastPort *rp, Point *p, short dx, short dy)
{
	WORD ddx = dx >> 1;
	WORD ddy = dy >> 1;

	p->x += ddx;
	p->y += ddy;
	if (/*ReadPixel(rp, p->x, p->y)*/ 0 == 0)	WritePixel(rp, p->x, p->y);
	p->x += ddx;
	p->y += ddy;
	if (/*ReadPixel(rp, p->x, p->y)*/ 0 == 0)	WritePixel(rp, p->x, p->y);
}

//---------------------------------------------------------------------------------------------

void Segment(struct RastPort *rp, short direction, short size, Point *p)
{
	if (size == 0)
	{
		switch (direction)
		{
			case SEGMENT_UP:     deltaDraw(rp, p, 0, -2);  break;
			case SEGMENT_LEFT:   deltaDraw(rp, p, -2, 0);  break;
			case SEGMENT_DOWN:   deltaDraw(rp, p, 0, 2);   break;
			case SEGMENT_RIGHT:  deltaDraw(rp, p, 2, 0);   break;
		}
	}
	else
	{
		size--;
		Segment(rp, addrot(direction, SEGMENT_RIGHT), size, p);
		Segment(rp, addrot(direction, SEGMENT_UP), size, p);
		Segment(rp, addrot(direction, SEGMENT_LEFT), size, p);
		Segment(rp, addrot(direction, SEGMENT_UP), size, p);
	}	
}

//---------------------------------------------------------------------------------------------

void SetWindowCenter(struct Window *win, Point *p)
{
	p->x = win->BorderLeft + ((win->Width - win->BorderLeft - win->BorderRight) >> 1);
	p->y = win->BorderTop + ((win->Height - win->BorderTop - win->BorderBottom) >> 1);
}

//---------------------------------------------------------------------------------------------

void Fractal(struct App *app)
{
	Point p;
	struct RastPort *rp = app->Win->RPort;
	short startsize = 6;

	SetWindowCenter(app->Win, &p);
	p.x += 65;
	p.y += 65;

	SetRPAttrs(rp,
		RPTAG_APen, 3,
		RPTAG_DrMd, JAM1,
	TAG_END);

	Move(rp, p.x, p.y);
	Segment(rp, SEGMENT_UP, startsize, &p);
	Segment(rp, SEGMENT_LEFT, startsize, &p);
	Segment(rp, SEGMENT_DOWN, startsize, &p);
	Segment(rp, SEGMENT_RIGHT, startsize, &p);
}

//---------------------------------------------------------------------------------------------

void ClipFractal(struct App *app)
{
	struct Window *win = app->Win;
	struct Region *rg, *org;
	struct Rectangle in;

	in.MinX = win->BorderLeft;
	in.MinY = win->BorderTop;
	in.MaxX = win->Width - win->BorderRight - 1;
	in.MaxY = win->Height - win->BorderBottom -1;

	if (rg = NewRegion())
	{
		if (OrRectRegion(rg, &in))
		{
			org = InstallClipRegion(win->WLayer, rg);

			if (org != rg)
			{
				Fractal(app);
				InstallClipRegion(win->WLayer, org);
			}
		}

		DisposeRegion(rg);
	}
}

//---------------------------------------------------------------------------------------------

BOOL PrepareTexts(struct App *app, struct EndGameText *texts)
{
	LONG moves = 0, seconds = 0;
	WORD hours, minutes;

	struct HighScore *hs;

	for (hs = (struct HighScore*)app->Selector.HighScores.mlh_Head; hs->Node.mln_Succ;
	 hs = (struct HighScore*)hs->Node.mln_Succ)
	{
		if (hs->Seconds != THE_WORST_TIME_POSSIBLE)   // skip the last, unfinished level
		{
			seconds += hs->Seconds;
			moves += hs->Moves;
		}
	}

	hours = div16(seconds, 3600);
	seconds -= mul16(hours, 3600);
	minutes = div16(seconds, 60);
	seconds -= mul16(minutes, 60);

	texts[0].text = "Congratulations!";
	texts[1].text = FmtNew("You have completed all the %ld levels.", app->LevelNumber - 1);
	texts[2].text = FmtNew("Your total number of moves: %ld.", moves);
	texts[3].text = FmtNew("Your total time: %ld:%02ld:%02ld.", hours, minutes, seconds);

	if (texts[1].text && texts[2].text && texts[3].text)
	{
		WORD i;

		for (i = 0; i < 4; i++) texts[i].len = StrLen(texts[i].text);
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------------------------

void PrintTextsCentered(struct App *app, struct EndGameText *texts)
{
	Point center;
	WORD x, y, i;
	struct RastPort *rp = app->Win->RPort;
	struct TextFont *tf;

	SetWindowCenter(app->Win, &center);
	GetRPAttrs(rp, RPTAG_Font, &tf, TAG_END);
	y = center.y - tf->tf_YSize - tf->tf_YSize + tf->tf_Baseline;

	SetRPAttrs(rp, RPTAG_DrMd, JAM1, TAG_END);

	for (i = 0; i < 4; i++)
	{
		WORD tw;

		tw = TextLength(rp, texts[i].text, texts[i].len);
		x = center.x - (tw >> 1);
		SetRPAttrs(rp, RPTAG_APen, 1, TAG_END);
		Move(rp, x + 1, y + 1);
		Text(rp, texts[i].text, texts[i].len);
		SetRPAttrs(rp, RPTAG_APen, 2, TAG_END);
		Move(rp, x, y);
		Text(rp, texts[i].text, texts[i].len);
		y += tf->tf_YSize;
	}
}

//---------------------------------------------------------------------------------------------

void FreeTexts(struct EndGameText *texts)
{
	if (texts[1].text) StrFree(texts[1].text);
	if (texts[2].text) StrFree(texts[2].text);
	if (texts[3].text) StrFree(texts[3].text);
}

//---------------------------------------------------------------------------------------------

void EndGame(struct App *app)
{
	struct EndGameText texts[4];

	if (PrepareTexts(app, texts))
	{
		PrintTextsCentered(app, texts);
		ClipFractal(app);
	}

	FreeTexts(texts);
}
