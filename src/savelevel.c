#include "savelevel.h"
#include "loader.h"

#include <proto/asl.h>
#include <proto/dos.h>            /* debug */
#include <proto/iffparse.h>

#include <intuition/intuition.h>

/*--------------------------------------------------------------------------*/

void WriteDots(struct GameLevel *glv, struct IFFHandle *out)
{
	if (PushChunk(out, ID_UNTG, ID_DOTS, glv->DotCount * 4) == 0)
	{
		WORD i, buf[2];

		for (i = 0; i < glv->DotCount; i++)
		{
			buf[0] = glv->DotStorage[i].Virtual.x;
			buf[1] = glv->DotStorage[i].Virtual.y;
			WriteChunkRecords(out, buf, sizeof(buf), 1);
		}

		PopChunk(out);
	}
}

/*--------------------------------------------------------------------------*/

void WriteLines(struct GameLevel *glv, struct IFFHandle *out)
{
	if (PushChunk(out, ID_UNTG, ID_LINE, glv->LineCount * 2) == 0)
	{
		WORD i;
		UBYTE buf[2];

		for (i = 0; i < glv->LineCount; i++)
		{
			buf[0] = glv->LineStorage[i].StartDot - glv->DotStorage;
			buf[1] = glv->LineStorage[i].EndDot - glv->DotStorage;
			WriteChunkRecords(out, buf, sizeof(buf), 1);
		}
		PopChunk(out);
	}
}

/*--------------------------------------------------------------------------*/

static void WriteLevelHeader(struct App *app, struct IFFHandle *out)
{
	if (PushChunk(out, ID_UNTG, ID_FORM, IFFSIZE_UNKNOWN) == 0)
	{
		LONG e;

		WriteDots(app->Level, out);
		WriteLines(app->Level, out);
		e = PopChunk(out);
	}
	else PutStr("push FORM failed\n"); 
}

/*--------------------------------------------------------------------------*/

static void WriteLevelIFF(struct App *app, BPTR output)
{
	struct IFFHandle *out;

	if (out = AllocIFF())
	{
		out->iff_Stream = output;
		InitIFFasDOS(out);

		if (OpenIFF(out, IFFF_WRITE) == 0)
		{
			WriteLevelHeader(app, out);
			CloseIFF(out);
		}
		else PutStr("openiff failed\n");

		FreeIFF(out);
	}
	else PutStr("allociff failed\n");
}

/*--------------------------------------------------------------------------*/

static void WriteLevelToFile(struct App *app, struct FileRequester *fr)
{
	BPTR tmpdir, olddir, output;

	if (tmpdir = Lock(fr->fr_Drawer, ACCESS_READ))
	{
		olddir = CurrentDir(tmpdir);

		if (output = Open(fr->fr_File, MODE_NEWFILE))
		{
			WriteLevelIFF(app, output);
			Close(output);
		}
		else Printf("Failed open of '%s'", fr->fr_File);

		CurrentDir(olddir);
		UnLock(tmpdir);
	}
	else Printf("Failed lock on '%s'\n", fr->fr_Drawer);
}

/*--------------------------------------------------------------------------*/

void SaveLevel(struct App *app)
{
	struct FileRequester *fr;

	if (app->Level)
	{
		if (fr = AllocAslRequest(ASL_FileRequest, NULL))
		{
			if (AslRequestTags(fr,
				ASLFR_Window, (LONG)app->Win,
				ASLFR_SleepWindow, TRUE,
				ASLFR_TitleText, (LONG)"Save current level",
				ASLFR_PositiveText, (LONG)"Save",
				ASLFR_InitialDrawer, (LONG)"PROGDIR:levels",
				ASLFR_InitialTopEdge, app->Win->TopEdge + app->Win->BorderTop,
				ASLFR_InitialLeftEdge, app->Win->LeftEdge + app->Win->BorderLeft,
				ASLFR_DoSaveMode, TRUE,
				ASLFR_RejectIcons, TRUE,
			TAG_END))
			{
				WriteLevelToFile(app, fr);
			}

			FreeAslRequest(fr);
		}
	}
}