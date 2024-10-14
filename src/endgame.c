//-------------
// game ending
//-------------

#include "main.h"

#include <proto/graphics.h>
#include <proto/dos.h>
#include <graphics/rpattr.h>

#define SEGMENT_UP     0
#define SEGMENT_LEFT   1
#define SEGMENT_DOWN   2
#define SEGMENT_RIGHT  3 

//---------------------------------------------------------------------------------------------

static inline short addrot(short rot1, short rot2)
{
	return (rot1 + rot2) & 0x3;
}

//---------------------------------------------------------------------------------------------

static void deltaDraw(struct RastPort *rp, Point *p, short dx, short dy)
{
	p->x += dx;
	p->y += dy;
	Draw(rp, p->x, p->y); 
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

		Delay(1);
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

	SetRPAttrs(rp,
		RPTAG_APen, 2,
	TAG_END);

	WritePixel(rp, p.x, p.y);
}

//---------------------------------------------------------------------------------------------

BOOL ClipFractal(struct App *app)
{
	Fractal(app);
}

//---------------------------------------------------------------------------------------------

void EndGame(struct App *app)
{
	ClipFractal(app);
}
