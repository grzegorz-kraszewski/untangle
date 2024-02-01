struct Library
	*GfxBase,
	*LayersBase,
	*IntuitionBase,
	*GadToolsBase,
	*IFFParseBase,
	*AslBase;
	
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>

#include "main.h"
#include "menu.h"

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
	{ WA_Title, (ULONG)"Untangle" },
	{ WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE },
	{ WA_NewLookMenus, TRUE },
	{ WA_Activate, TRUE },
	{ TAG_END, 0 }
};


/* The first plane of DotRaster also serves as a mask for blitting. */

UWORD DotRaster[DOT_SIZE * 2] = {0x3800, 0x7C00, 0xFE00, 0xFE00, 0xFE00, 0x7C00, 0x3800,
                                 0x0000, 0x3800, 0x7C00, 0x7C00, 0x7C00, 0x3800, 0x0000};

#define DOTRASTER_MODULO 2

static LONG PrepareDotImage(struct App *app)
{
	LONG err = SERR_NO_CHIP_MEM;
	
	if (app->DotRaster = AllocMem(sizeof(DotRaster), MEMF_CHIP))
	{
		CopyMem(DotRaster, app->DotRaster, sizeof(DotRaster));
		
		if (app->DotBitMap = AllocBitMap(DOT_SIZE, DOT_SIZE, 2, BMF_CLEAR, app->Win->RPort->BitMap))
		{
			struct RastPort tmrp;

			InitRastPort(&tmrp);
			tmrp.BitMap = app->DotBitMap;
			SetDrMd(&tmrp, JAM1);
			SetAPen(&tmrp, 1);
			BltTemplate((UBYTE*)app->DotRaster, 0, DOTRASTER_MODULO, &tmrp, 0, 0, DOT_SIZE, DOT_SIZE);
			SetAPen(&tmrp, 2);
			BltTemplate((UBYTE*)&app->DotRaster[DOT_SIZE], 0, DOTRASTER_MODULO, &tmrp, 0, 0, DOT_SIZE, DOT_SIZE);
			err = SetupMenus(app);
			FreeBitMap(app->DotBitMap);
		}
		FreeMem(app->DotRaster, sizeof(DotRaster));
	}
	return err;
}


static LONG OpenMyWindow(struct App *app)
{
	LONG err = SERR_NO_WINDOW;
	 
	if (app->Win = OpenWindowTagList(NULL, wintags))
	{
		err = PrepareDotImage(app);
		CloseWindow(app->Win);
	}

	return err;
}


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
							result = OpenMyWindow(app);
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
	"Can't open asl.library v39+.\n"
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
	LONG error;

	app.Level = NULL;
	
	if (error = GetKickstartLibs(&app))
	{
		ReportStartupError(error);
		return RETURN_FAIL;
	}

	return 0;
}
