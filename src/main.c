struct Library
	*GfxBase,
	*LayersBase,
	*IntuitionBase,
	*GadToolsBase,
	*IFFParseBase,
	*AslBase,
	*IconBase,
	*TimerBase;

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <workbench/startup.h>

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
	BOOL redraw_timer = TRUE;
	struct IntuiMessage *imsg;
	ULONG signals, portmask, timermask;

	portmask = 1 << app->Win->UserPort->mp_SigBit;
	timermask = 1 << app->TimerPort->mp_SigBit;

	NewGame(app);  /* later it should load last played level from save */

	while (running)
	{
		signals = Wait(portmask | timermask | SIGBREAKF_CTRL_C);

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

					case IDCMP_SIZEVERIFY:
						redraw_timer = FALSE;   /* do not draw while window is resized */
					break;

					case IDCMP_NEWSIZE:
						redraw_timer = TRUE;
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
									StopTimer(app);
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

		if (signals & timermask)
		{
			WaitIO(&app->TimerReq->tr_node);
			PushNextSecond(app, redraw_timer);
		}
	}

	return;
}

/*---------------------------------------------------------------------------*/

BOOL RectVertPixels(struct App *app)
{
	struct DrawInfo *dri;
	WORD pcf;

	if (dri = GetScreenDrawInfo(app->Win->WScreen))
	{
		pcf = div16(dri->dri_Resolution.Y << 3, dri->dri_Resolution.X);
		FreeScreenDrawInfo(app->Win->WScreen, dri);
	}

	return (pcf > 12);
}

/*---------------------------------------------------------------------------*/


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

UWORD DotRaster5x3[6] = {
	0x7000, 0xF800, 0x7000,	0x0000, 0x2000, 0x0000 };

UWORD DotRaster7x3[6] = {
	0x7C00, 0xFE00, 0x7C00,	0x0000, 0x3800, 0x0000 };

UWORD DotRaster9x5[10] = {
	0x1C00, 0x7F00, 0xFF80, 0x7F00, 0x1C00, 0x0000, 0x1C00, 0x3E00, 0x1C00, 0x0000 };

UWORD DotRaster11x5[10] = {
	0x1F00, 0x7FC0, 0xFFE0, 0x7FC0, 0x1F00, 0x0000, 0x1F00, 0x3F80, 0x1F00, 0x0000 };

UWORD DotRaster13x7[14] = {
	0x0F80, 0x3FE0, 0xFFF8, 0xFFF8, 0xFFF8, 0x3FE0, 0x0F80, 0x0000, 0x0F80, 0x3FE0, 0x3FE0,
	0x3FE0, 0x0F80, 0x0000 };

UWORD DotRaster15x7[14] = {
	0x0FE0, 0x3FF8, 0xFFFE, 0xFFFE, 0xFFFE, 0x3FF8, 0x0FE0, 0x0000, 0x0FE0, 0x3FF8, 0x3FF8,
	0x3FF8, 0x0FE0, 0x0000 };

UWORD *DotDataSqr[6] = { DotRaster5, DotRaster7, DotRaster9, DotRaster11, DotRaster13, DotRaster15 };
UWORD *DotDataRct[6] = { DotRaster5x3, DotRaster7x3, DotRaster9x5, DotRaster11x5, DotRaster13x7,
	DotRaster15x7 };

#define DOTRASTER_MODULO 2

static LONG PrepareDotImage(struct App *app)
{
	LONG rassize, err = SERR_NO_CHIP_MEM;
	UWORD *fastraster;

	if (RectVertPixels(app))
	{
		fastraster = DotDataRct[app->DotWidth - 1];
		app->DotHeight = ((app->DotWidth + 1) & 0xFFFE) + 1;
		app->DotWidth = app->DotWidth * 2 + 3;
	}
	else
	{ 
		fastraster = DotDataSqr[app->DotWidth - 1];
		app->DotWidth = app->DotWidth * 2 + 3;
		app->DotHeight = app->DotWidth;
	}

	rassize = app->DotHeight * DOTRASTER_MODULO * 2;   /* 2 for 2 bitplanes */

	if (app->DotRaster = AllocMem(rassize, MEMF_CHIP))
	{
		CopyMem(fastraster, app->DotRaster, rassize);

		if (app->DotBitMap = AllocBitMap(app->DotWidth, app->DotHeight, 2, BMF_CLEAR, app->Win->RPort->BitMap))
		{
			struct RastPort tmrp;

			InitRastPort(&tmrp);
			tmrp.BitMap = app->DotBitMap;
			SetDrMd(&tmrp, JAM1);
			SetAPen(&tmrp, 1);
			BltTemplate((UBYTE*)app->DotRaster, 0, DOTRASTER_MODULO, &tmrp, 0, 0, app->DotWidth,
				app->DotHeight);
			SetAPen(&tmrp, 2);
			BltTemplate((UBYTE*)&app->DotRaster[app->DotHeight], 0, DOTRASTER_MODULO, &tmrp, 0, 0,
				app->DotWidth, app->DotHeight);
			err = SetupMenus(app);
			FreeBitMap(app->DotBitMap);
		}

		FreeMem(app->DotRaster, rassize);
	}

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG GetScreenFont(struct App *app)
{
	LONG err = 430;

	if (app->InfoFont = OpenFont(app->Win->WScreen->Font))
	{
		err = PrepareDotImage(app);
		CloseFont(app->InfoFont);
	}

	return err;
}

/*---------------------------------------------------------------------------*/
/* Minimum window width is determined by pixel length of text in the info    */
/* bar. 
/*---------------------------------------------------------------------------*/

LONG CalculateMinWidth(struct App *app, struct Screen *wb)
{
	STRPTR sstr = "Intersections: 0000  Moves: 0000  Time: 000:00";

	return (TextLength(&wb->RastPort, sstr, StrLen(sstr)) + (app->DotWidth & ~1));
}

/*---------------------------------------------------------------------------*/

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
	{ WA_ScreenTitle, 0 /* DefScreenTitle */ },
	{ WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS
	  | IDCMP_MOUSEMOVE | IDCMP_SIZEVERIFY },
	{ WA_NewLookMenus, TRUE },
	{ WA_Activate, TRUE },
	{ TAG_END, 0 }
};

static LONG OpenMyWindow(struct App *app)
{
	LONG err = SERR_NO_WINDOW;
	struct Screen *wb;

	wintags[10].ti_Data = (LONG)DefWindowTitle;
	wintags[11].ti_Data = (LONG)DefScreenTitle;

	if (wb = LockPubScreen(NULL))
	{
		wintags[2].ti_Data = CalculateMinWidth(app, wb);
		Printf("min_width = %ld.\n", wintags[2].ti_Data);
		app->Win = OpenWindowTagList(NULL, wintags);
		UnlockPubScreen(NULL, wb);

		if (app->Win)
		{
			err = GetScreenFont(app);
			CloseWindow(app->Win);
		}
	}

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG GetTimer(struct App *app)
{
	LONG err = SERR_TIMER;

	if (app->TimerPort = CreateMsgPort())
	{
		if (app->TimerReq = (struct timerequest*)CreateIORequest(app->TimerPort,
		sizeof(struct timerequest)))
		{
			if (OpenDevice("timer.device", UNIT_WAITUNTIL, &app->TimerReq->tr_node, 0) == 0)
			{
				TimerBase = &app->TimerReq->tr_node.io_Device->dd_Library;
				err = OpenMyWindow(app);
				AbortIO(&app->TimerReq->tr_node);
				WaitIO(&app->TimerReq->tr_node);
				CloseDevice(&app->TimerReq->tr_node);
			}

			DeleteIORequest(&app->TimerReq->tr_node);
		}

		DeleteMsgPort(app->TimerPort);
	}
	return err;
}

/*---------------------------------------------------------------------------*/

static LONG GetUntanglePrefs(struct App *app, struct WBStartup *wbmsg)
{
	if (wbmsg)     /* if launched from CLI sorry, no prefs */
	{
		struct DiskObject *dobj;
		BPTR olddir;

		olddir = CurrentDir(wbmsg->sm_ArgList[0].wa_Lock);

		if (dobj = GetDiskObject(wbmsg->sm_ArgList[0].wa_Name))
		{
			LONG dotsize;
			STRPTR value;

			value = FindToolType(dobj->do_ToolTypes, "DOTSIZE");

			if (value)
			{
				StrToLong(value, &dotsize);

				if ((dotsize >= 1) && (dotsize <= 6)) app->DotWidth = dotsize;
			}

			FreeDiskObject(dobj);
		}

		CurrentDir(olddir);
	}

	return GetTimer(app);
}

/*---------------------------------------------------------------------------*/

static LONG GetKickstartLibs(struct App *app, struct WBStartup *wbmsg)
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
							result = GetUntanglePrefs(app, wbmsg);
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
	"Can't open timer.device.\n"
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


ULONG Main(struct WBStartup *wbmsg)
{
	struct App app;
	LONG error, result = RETURN_OK;

	app.Level = NULL;
	app.LevelNumber = 1;                 /* will be loaded from progress file(?) */
	app.DynamicScreenTitle = NULL;
	app.DynamicWindowTitle = NULL;
	app.DotWidth = 4;                    /* default if icon toolype does not exist / can't be read */
	app.DotHeight = 0;
	app.CurrentInfoText = StrClone("");

	if (error = GetKickstartLibs(&app, wbmsg))
	{
		ReportStartupError(error);
		result = RETURN_FAIL;
	}

	if (app.CurrentInfoText) StrFree(app.CurrentInfoText);
	if (app.DynamicScreenTitle) StrFree(app.DynamicScreenTitle);
	if (app.DynamicWindowTitle) StrFree(app.DynamicWindowTitle);

	return result;
}
