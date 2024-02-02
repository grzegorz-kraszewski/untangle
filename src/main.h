#ifndef UNTANGLE_MAIN_H
#define UNTANGLE_MAIN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/gfx.h>

#define REG(arg, reg) arg __asm(reg)

#ifdef __mc68000__
#define mul16(a,b) ({ \
LONG _r; \
WORD _a = (a), _b = (b); \
asm("MULS.W %2,%0": "=d" (_r): "0" (_a), "dmi" (_b): "cc"); \
_r;})

#define div16(a,b) ({ \
WORD _r, _b = (b); \
LONG _a = (a); \
asm("DIVS.W %2,%0": "=d" (_r): "0" (_a), "dmi" (_b): "cc"); \
_r;})
#else
#define mul16(a,b) a*b
#define div16(a,b) a/b
#endif

#define ForEachFwd(l, t, v) for (v = (t*)(l)->mlh_Head; v->Node.mln_Succ; v = (t*)v->Node.mln_Succ)
#define ForEachRev(l, t, v) for (v = (t*)(l)->mlh_TailPred; v->Node.mln_Pred; v = (t*)v->Node.mln_Pred)

void InitList(struct MinList *list);

extern struct Library
	*SysBase,
	*DOSBase,
	*GfxBase,
	*LayersBase,
	*IntuitionBase,
	*GadToolsBase,
	*IFFParseBase,
	*AslBase;

#define DOT_SIZE 7

/*---------------------*/
/* startup error codes */
/*---------------------*/

#define SERR_SYSTEM_TOO_OLD             1
#define SERR_NO_IFFPARSE                2
#define SERR_NO_WINDOW                  3
#define SERR_NO_CHIP_MEM                4
#define SERR_MENU_LAYOUT                5
#define SERR_NO_ASL                     6

struct GameDot
{
	struct MinNode Node;
	Point Virtual;         /* virtual coordinate system */
	Point Pixel;           /* window coordinate system */
};

struct GameLine
{
	struct MinNode Node;
	struct GameDot *StartDot;
	struct GameDot *EndDot;
	WORD Index;
};

struct GameLevel
{
	struct GameDot *DotStorage;
	struct GameLine *LineStorage;
	struct MinList DotList;
	struct MinList LineList;
	struct MinList DraggedLines;
	struct GameDot *DraggedDot;
	UBYTE *Intersections;              /* triangle matrix of intersections between lines */
	LONG DotCount;                     /* total number of dots */
	LONG LineCount;                    /* total number of lines */
	LONG InterCount;                   /* current number of intersections, 0 = level solved */
};

struct App
{
	struct Window *Win;
	struct Menu *WinMenu;
	UWORD *DotRaster;
	struct BitMap *DotBitMap;
	struct Rectangle Field;
	struct GameLevel *Level;
	LONG LevelNumber;                  /* counted from 1, ordinal number in a set */
};

#endif  /* UNTANGLE_MAIN_H */
