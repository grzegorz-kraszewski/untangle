#ifndef UNTANGLE_MAIN_H
#define UNTANGLE_MAIN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/gfx.h>

#define REG(arg, reg) arg __asm(reg)

#define mul16(a,b) ({ \
LONG _r; \
WORD _a = (a), _b = (b); \
asm("MULS.W %2,%0": "=d" (_r): "0" (_a), "dmi" (_b): "cc"); \
_r;})

void InitList(struct MinList *list);

extern struct Library
	*SysBase,
	*DOSBase,
	*GfxBase,
	*LayersBase,
	*IntuitionBase,
	*GadToolsBase,
	*IFFParseBase;

#define DOT_SIZE 7

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
};

struct App
{
	struct Window *Win;
	struct Menu *WinMenu;
	UWORD *DotRaster;
	struct BitMap *DotBitMap;
	struct GameDot *DotStorage;
	struct GameLine *LineStorage;
	struct Rectangle Field;
	struct MinList DotList;
	struct MinList LineList;
	struct MinList DraggedLines;
	struct GameDot *DraggedDot;
};

#endif  /* UNTANGLE_MAIN_H */
