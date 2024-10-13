/*-----------------------------*/
/* level solution check module */
/*-----------------------------*/

#include "lscm.h"

#include <proto/dos.h>

/*---------------------------------------------------------------------------*/
/* Checks if two GameLines (segments, mathematically speaking) have a common */
/* dot (are connected).                                                      */
/*---------------------------------------------------------------------------*/

static BOOL SegmentsHaveCommonDot(struct GameLine *glia, struct GameLine *glib)
{
	if (glia->StartDot == glib->StartDot) return TRUE;
	if (glia->StartDot == glib->EndDot) return TRUE;
	if (glia->EndDot == glib->StartDot) return TRUE;
	if (glia->EndDot == glib->EndDot) return TRUE;
	return FALSE;
}


static void UpdateIntersection(struct GameLevel *glv, WORD x, WORD y, BOOL state)
{
	LONG index = (mul16(x, (glv->LineCount << 1) - x - 1) >> 1) + y - x - 1;
	UBYTE oldstate = glv->Intersections[index];
	glv->Intersections[index] = (UBYTE)state;
	if ((oldstate == 0) && (state == 1)) glv->InterCount++;
	else if ((oldstate == 1) && (state == 0)) glv->InterCount--;
}


/*---------------------------------------------------------------------------*/
/* Checks if tested segment (a GameLine) intersects an infinite line defined */
/* by two points of another GameLine. Line defining segment is treated as    */
/* a vector 'A'. Two new vectors 'B' and 'C' are created. 'B' is from the    */
/* start of line defining segment to start of tested segment. 'C' is from    */
/* the start of line defining segment to the end of tested segments. Then    */
/* vector products AB and AC are calculated. Depending on signs of products  */
/* conditions are as follows:                                                */
/* AB +, AC +  no intersections                                              */
/* AB 0, AC +  start of segment lays on 'line'                               */
/* AB -, AC +  'segment' intersects with 'line'                              */
/* AB +, AC 0  end of 'segment' lays on 'line'                               */
/* AB 0, AC 0  'segment' is placed on 'line'                                 */
/* AB -, AC 0  end of 'segment' lays on 'line'                               */
/* AB +, AC -  'segment' intersects with 'line'                              */
/* AB 0, AC -  start of 'segment' lays on 'line'                             */ 
/* AB -, AC -  no intersection                                               */
/*---------------------------------------------------------------------------*/

static BOOL SegmentIntersectsLine(struct GameLine *segment, struct GameLine *line)
{
	WORD ax, ay, bx, by, cx, cy;
	LONG prod_ab, prod_ac;

	ax = line->EndDot->Virtual.x - line->StartDot->Virtual.x;
	ay = line->EndDot->Virtual.y - line->StartDot->Virtual.y;
	bx = segment->StartDot->Virtual.x - line->StartDot->Virtual.x;
	by = segment->StartDot->Virtual.y - line->StartDot->Virtual.y;
	cx = segment->EndDot->Virtual.x - line->StartDot->Virtual.x;
	cy = segment->EndDot->Virtual.y - line->StartDot->Virtual.y;
	prod_ab = mul16(ax, by) - mul16(ay, bx);
	prod_ac = mul16(ax, cy) - mul16(ay, cx);

	if (prod_ab > 0)
	{
		if (prod_ac > 0) return FALSE;
		else if (prod_ac < 0) return TRUE;
	}
	else if (prod_ab < 0)
	{
		if (prod_ac < 0) return FALSE;
		else if (prod_ac > 0) return TRUE;
	}

	// PutStr("I don't know yet.\n");
	return TRUE;
}

/*---------------------------------------------------------------------------*/

static BOOL IntersectionTest(struct GameLine *glia, struct GameLine *glib)
{
	if (SegmentsHaveCommonDot(glia, glib)) return FALSE;
	else return (SegmentIntersectsLine(glia, glib) && SegmentIntersectsLine(glib, glia));
}

/*---------------------------------------------------------------------------*/
/* Precalculates all line intersections in a level. Sets 'InterCount' field  */
/* in GameLevel.                                                             */
/*---------------------------------------------------------------------------*/

void PrecalculateLevel(struct GameLevel *glv)
{
	struct GameLine *glia, *glib;
	UBYTE *inter = glv->Intersections;

	glv->InterCount = 0;

	ForEachFwd(&glv->LineList, struct GameLine, glia)
	{
		if (glia->Node.mln_Succ)
		{
			for (glib = (struct GameLine*)glia->Node.mln_Succ; glib->Node.mln_Succ; glib = (struct GameLine*)glib->Node.mln_Succ)
			{
				UBYTE test;
				
				test = (UBYTE)IntersectionTest(glia, glib);
				*inter++ = test;
				if (test) glv->InterCount++; 
			}
		}
	}
}

/*---------------------------------------------------------------------------*/
/* Updates state of intersections of just moved segments (placed on          */
/* DraggedLines list). Optimizations:                                        */
/* 1. Skip intersection test of a line with itself (obvious)                 */
/* 2. Skip intersection test between moved lines (they share a dot by defi-  */
/*    nition.                                                                */
/* Conclusion: each line from 'DraggedLines' should be checked against each  */
/* line from 'LineList'.                                                     */
/*---------------------------------------------------------------------------*/

void UpdateIntersections(struct GameLevel *glv)
{
	struct GameLine *glia, *glib;
	
	ForEachFwd(&glv->DraggedLines, struct GameLine, glia)
	{
		ForEachFwd(&glv->LineList, struct GameLine, glib)
		{
			BOOL test = IntersectionTest(glia, glib);

			if (glib->Index > glia->Index) UpdateIntersection(glv, glia->Index, glib->Index, test);
			else UpdateIntersection(glv, glib->Index, glia->Index, test);
		}
	}
}
