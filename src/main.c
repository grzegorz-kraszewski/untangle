struct Library
	*GfxBase,
	*LayersBase,
	*IntuitionBase,
	*GadToolsBase,
	*IFFParseBase,
	*AslBase,
	*IconBase;
	
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>

#include "main.h"
#include "menu.h"
#include "strutils.h"
#include "version.h"

STRPTR DefScreenTitle = "Untangle " VERSION " by RastPort " RELYEAR;
STRPTR DefWindowTitle = "Untangle";

/*---------------------------------------------------------------------------*/

void InitList(struct MinList *list)
{
	list->mlh_Head = (struct MinNode*)&list->mlh_Tail;
	list->mlh_Tail = NULL;
	list->mlh_TailPred = (struct MinNode*)&list->mlh_Head;
}

/*---------------------------------------------------------------------------*/

void TheLoop(struct App *app)
{
	BOOL running = TRUE;
	struct IntuiMessage *imsg;
	ULONG signals, portmask;

	portmask = 1 << app->Win->UserPort->mp_SigBit;

	NewGame(app);  /* later it should load last played level from save */
	
	while (running)
	{
		signals = Wait(portmask | SIGBREAKF_CTRL_C);

		if (signals & SIGBREAKF_CTRL_C) running = FALSE;

		if (signals & portmask)
		{
			while (imsg = (struct IntuiMessage*)GetMsg(app->Win->UserPort))
			{
				switch (imsg->Class)
				{
					case IDCMP_CLOSEWINDOW:
						running = FALSE;
					break;

					case IDCMP_MENUPICK:
						running = HandleMenu(app, imsg->Code);
					break;

					case IDCMP_NEWSIZE:
						EraseGame(app);
						ScaleGame(app);
						DrawGame(app);
					break;
					
					case IDCMP_MOUSEBUTTONS:
						if (imsg->Code == SELECTDOWN) GameClick(app, imsg->MouseX, imsg->MouseY);
						else if (imsg->Code == SELECTUP)
						{
							if (app->Win->Flags & WFLG_REPORTMOUSE)
							{
						 		GameUnclick(app, imsg->MouseX, imsg->MouseY);

						 		if (app->Level->InterCount == 0)
						 		{
									DisplayBeep(app->Win->WScreen);
									Delay(50);
									EraseGame(app);
									UnloadLevel(app->Level);
									app->Level = NULL;
									app->LevelNumber++;
									NewGame(app);
								}
							}
						}
					break;
					
					case IDCMP_MOUSEMOVE:
						if (app->Win->Flags & WFLG_REPORTMOUSE)
						{
							GameDotDrag(app, imsg->MouseX, imsg->MouseY);
						}
					break;
				}
				
				ReplyMsg(&imsg->ExecMessage);
			}
		}
	}
	
	return;
}


struct TagItem wintags[] = {
	{ WA_Width, 400 },
	{ WA_Height, 400 },
	{ WA_MinWidth, 160 },
	{ WA_MinHeight, 160 },
	{ WA_MaxWidth, 1024 },
	{ WA_MaxHeight, 1024 },
	{ WA_DragBar, TRUE },
	{ WA_CloseGadget, TRUE },
	{ WA_DepthGadget, TRUE },
	{ WA_SizeGadget, TRUE },
	{ WA_Title, 0 /* DefWindowTitle */ },
	{ WA_ScreenTitle, 0, /* DefScreenTitle */ },
	{ WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE },
	{ WA_NewLookMenus, TRUE },
	{ WA_Activate, TRUE },
	{ TAG_END, 0 }
};


/* The first plane of DotRaster also serves as a mask for blitting. */

UWORD DotRaster5[10] = {
	0x7000, 0xF800, 0xF800, 0xF800, 0x7000,
	0x0000, 0x7000, 0x7000, 0x7000, 0x0000 };

UWORD DotRaster7[14] = {
	0x3800, 0x7C00, 0xFE00, 0xFE00, 0xFE00, 0x7C00, 0x3800,
	0x0000, 0x3800, 0x7C00, 0x7C00, 0x7C00, 0x3800, 0x0000 };

UWORD DotRaster9[18] = {
	0x1C00, 0x3E00, 0x7F00, 0xFF80, 0xFF80, 0xFF80, 0x7F00, 0x3E00,	0x1C00,
	0x0000, 0x1C00, 0x3E00, 0x7F00, 0x7F00, 0x7F00, 0x3E00, 0x1C00,	0x0000 };

UWORD DotRaster11[22] = {
	0x1F00, 0x3F80, 0x7FC0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0,	0x7FC0, 0x3F80, 0x1F00,
	0x0000, 0x1F00, 0x3F80, 0x7FC0, 0x7FC0, 0x7FC0, 0x7FC0, 0x7FC0,	0x3F80, 0x1F00, 0x0000 };

UWORD DotRaster13[26] = {
	0x0F80, 0x3FE0, 0x7FF0, 0x7FF0, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8,	0xFFF8, 0x7FF0, 0x7FF0,
	0x3FE0, 0x0F80,	0x0000, 0x0F80, 0x3FE0, 0x3FE0, 0x7FF0, 0x7FF0, 0x7FF0, 0x7FF0,	0x7FF0,
	0x3FE0, 0x3FE0, 0x0F80, 0x0000 };

UWORD DotRaster15[30] = {
	0x07C0, 0x1FF0, 0x3FF8, 0x7FFC, 0x7FFC, 0xFFFE, 0xFFFE, 0xFFFE,	0xFFFE, 0xFFFE, 0x7FFC,
	0x7FFC, 0x3FF8, 0x1FF0, 0x07C0,	0x0000, 0x07C0, 0x1FF0, 0x3FF8, 0x3FF8, 0x7FFC, 0x7FFC,
	0x7FFC,	0x7FFC, 0x7FFC, 0x3FF8, 0x3FF8, 0x1FF0, 0x07C0, 0x0000 };

UWORD *DotData[6] = { DotRaster5, DotRaster7, DotRaster9, DotRaster11, DotRaster13, DotRaster15 };

#define DOTRASTER_MODULO 2

static LONG PrepareDotImage(struct App *app)
{
	LONG rassize, err = SERR_NO_CHIP_MEM;

	rassize = app->DotSize * DOTRASTER_MODULO * sizeof(UWORD);
 
	if (app->DotRaster = AllocMem(rassize, MEMF_CHIP))
	{
		CopyMem(DotData[(app->DotSize >> 1) - 2], app->DotRaster, rassize);

		if (app->DotBitMap = AllocBitMap(app->DotSize, app->DotSize, 2, BMF_CLEAR, app->Win->RPort->BitMap))
		{
			struct RastPort tmrp;

			InitRastPort(&tmrp);
			tmrp.BitMap = app->DotBitMap;
			SetDrMd(&tmrp, JAM1);
			SetAPen(&tmrp, 1);
			BltTemplate((UBYTE*)app->DotRaster, 0, DOTRASTER_MODULO, &tmrp, 0, 0, app->DotSize,
				app->DotSize);
			SetAPen(&tmrp, 2);
			BltTemplate((UBYTE*)&app->DotRaster[app->DotSize], 0, DOTRASTER_MODULO, &tmrp, 0, 0,
				app->DotSize, app->DotSize);
			err = SetupMenus(app);
			FreeBitMap(app->DotBitMap);
		}

		FreeMem(app->DotRaster, rassize);
	}

	return err;
}


static LONG OpenMyWindow(struct App *app)
{
	LONG err = SERR_NO_WINDOW;
	 
	wintags[10].ti_Data = (LONG)DefWindowTitle;
	wintags[11].ti_Data = (LONG)DefScreenTitle;

	if (app->Win = OpenWindowTagList(NULL, wintags))
	{
		err = PrepareDotImage(app);
		CloseWindow(app->Win);
	}

	return err;
}

/*---------------------------------------------------------------------------*/

LONG GetKickstartLibs(struct App *app)
{
	LONG result = SERR_SYSTEM_TOO_OLD;

	if (GfxBase = OpenLibrary("graphics.library", 39))
	{
		if (LayersBase = OpenLibrary("layers.library", 39))
		{
			if (IntuitionBase = OpenLibrary("intuition.library", 39))
			{
				if (GadToolsBase = OpenLibrary("gadtools.library", 39))
				{
					result = SERR_NO_IFFPARSE;

					if (IFFParseBase = OpenLibrary("iffparse.library", 39))
					{
						result = SERR_NO_ASL;

						if (AslBase = OpenLibrary("asl.library", 39))
						{
							/* icon.library is optional */

							IconBase = OpenLibrary("icon.library", 39);
							Printf("IconBase @ $%08lx.\n", (LONG)IconBase);
							result = OpenMyWindow(app);
							if (IconBase) CloseLibrary(IconBase);
							CloseLibrary(AslBase);
						}
						CloseLibrary(IFFParseBase);
					}
					CloseLibrary(GadToolsBase);
				}
				CloseLibrary(IntuitionBase);
			}			
			CloseLibrary(LayersBase);
		}
		CloseLibrary(GfxBase);
	}

	return result;
} 


static STRPTR StartupErrorMessages[] = {
	"Can't open iffparse.library v39+.\n",
	"Can't open game window.\n",
	"Out of chip memory.\n",
	"Can't create program menu (out of memory?).\n",
	"Can't open asl.library v39+.\n",
};


static void ReportStartupError(err)
{
	/*-------------------------------------------------------------*/
	/* In case of fail to open Kickstart libraries (error code 1), */
	/* the best I can do is just a silent quit.                    */
	/*-------------------------------------------------------------*/
	 
	if (err > 1) PutStr(StartupErrorMessages[err - 2]);
	return;
}


ULONG Main(void)
{
	struct App app;
	LONG error, result = RETURN_OK;

	app.Level = NULL;
	app.LevelNumber = 1;                 /* will be loaded from progress file(?) */
	app.DynamicScreenTitle = NULL;
	app.DynamicWindowTitle = NULL;
	app.DotSize = 9;                     /* default if icon toolype does not exist / can't be read */
	
	if (error = GetKickstartLibs(&app))
	{
		ReportStartupError(error);
		result = RETURN_FAIL;
	}

	if (app.DynamicScreenTitle) StrFree(app.DynamicScreenTitle);
	if (app.DynamicWindowTitle) StrFree(app.DynamicWindowTitle);

	return result;
}
