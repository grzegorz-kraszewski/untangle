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
			case SEGMENT_UP:     deltaDraw(rp, p, -1, -2);  break;
			case SEGMENT_LEFT:   deltaDraw(rp, p, -2, 1);  break;
			case SEGMENT_DOWN:   deltaDraw(rp, p, 1, 2);   break;
			case SEGMENT_RIGHT:  deltaDraw(rp, p, 2, -1);   break;
		}

		Delay(1);
	}
	else
	{
		size--;
		Segment(rp, addrot(direction, SEGMENT_UP), size, p);
		Segment(rp, addrot(direction, SEGMENT_RIGHT), size, p);
		Segment(rp, addrot(direction, SEGMENT_UP), size, p);
		Segment(rp, addrot(direction, SEGMENT_LEFT), size, p);
		Segment(rp, addrot(direction, SEGMENT_UP), size, p);
	}	
}

//---------------------------------------------------------------------------------------------

void Fractal(struct App *app)
{
	Point p = {300, 300};
	struct RastPort *rp = app->Win->RPort;
	short startsize = 4;
	
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

void EndGame(struct App *app)
{
	Fractal(app);
}
