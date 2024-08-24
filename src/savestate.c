#include <proto/dos.h>            /* debug */
#include <proto/iffparse.h>
#include <proto/intuition.h>

#include "main.h"

#define ID_UNTG MAKE_ID('U','N','T','G')
#define ID_HSCR MAKE_ID('H','S','C','R')
#define ID_WINP MAKE_ID('W','I','N','P')

struct HighScoreRecord
{
	LONG seconds;
	LONG moves;
};

/*--------------------------------------------------------------------------*/

static void SaveStateError(struct App *app, STRPTR opdesc, LONG ecode)
{
	Printf("Save state error: '%s' error code %ld.\n", opdesc, ecode);
}

/*--------------------------------------------------------------------------*/

static void SaveStateHighScores(struct App *app, struct IFFHandle *out)
{
	LONG ecode;
	struct HighScoreRecord hsr;
	struct MinList *hslist = &app->Selector.HighScores;

	ecode = PushChunk(out, ID_UNTG, ID_HSCR, IFFSIZE_UNKNOWN);
	
	if (!ecode)
	{
		struct HighScore *hs;

		ForEachFwd(hslist, struct HighScore, hs)
		{
			if (hs->Seconds != THE_WORST_TIME_POSSIBLE)
			{
				hsr.seconds = hs->Seconds;
				hsr.moves = hs->Moves;
				ecode = WriteChunkRecords(out, &hsr, sizeof(struct HighScoreRecord), 1);
				if (ecode == 1) ecode = 0;
				if (ecode < 0) break;
			}
		}

		ecode = PopChunk(out);
	}

	if (ecode) SaveStateError(app, "writing high scores", ecode);
}

/*--------------------------------------------------------------------------*/

static void SaveStateWinPositions(struct App *app, struct IFFHandle *out)
{
	LONG ecode;

	ecode = PushChunk(out, ID_UNTG, ID_WINP, sizeof(struct WinPosRecord) * 2);

	if (!ecode)
	{
		ecode = WriteChunkRecords(out, &app->MainWinPos, sizeof(struct WinPosRecord), 1);
		if (ecode == 1) ecode = 0;

		if (!ecode)
		{
			ecode = WriteChunkRecords(out, &app->Selector.SelWinPos, sizeof(struct WinPosRecord), 1);
			if (ecode == 1) ecode = 0;
		}	

		ecode = PopChunk(out);
	}

	if (ecode) SaveStateError(app, "writing windows positions", ecode);
}

/*--------------------------------------------------------------------------*/

static void SaveStateHeader(struct App *app, struct IFFHandle *out)
{
	LONG ecode;

	ecode = PushChunk(out, ID_UNTG, ID_FORM, IFFSIZE_UNKNOWN);

	if (!ecode)
	{
		SaveStateHighScores(app, out);
		SaveStateWinPositions(app, out);
		ecode = PopChunk(out);
	}

	if (ecode) SaveStateError(app, "writing IFF header", ecode);
}

/*--------------------------------------------------------------------------*/

static void SaveState2(struct App *app, BPTR output)
{
	struct IFFHandle *out;

	if (out = AllocIFF())
	{
		LONG ecode;

		out->iff_Stream = output;
		InitIFFasDOS(out);

		if ((ecode = OpenIFF(out, IFFF_WRITE)) == 0)
		{
			SaveStateHeader(app, out);
			CloseIFF(out);
		}
		else SaveStateError(app, "opening IFF stream", ecode);

		FreeIFF(out);
	}
	else SaveStateError(app, "opening IFF stream", ERROR_NO_FREE_STORE);
}

/*--------------------------------------------------------------------------*/

void SaveState(struct App *app)
{
	BPTR output;

	if (output = Open("PROGDIR:Untangle.state", MODE_NEWFILE))
	{
		SaveState2(app, output);
		Close(output);
	}
	else SaveStateError(app, "opening file", IoErr());
}

