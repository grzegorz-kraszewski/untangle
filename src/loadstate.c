/*----------------------*/
/* loader of game state */
/*----------------------*/

#include "main.h"
#include "savestate.h"
#include "selector.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

/*---------------------------------------------------------------------------*/
/* Should be called before the first unsolved level is added to the list.    */

static void LoadHighScores(struct App *app, struct IFFHandle *in)
{
	struct HighScoreRecord hsr;
	LONG cr;

	do
	{
		cr = ReadChunkRecords(in, &hsr, sizeof(struct HighScoreRecord), 1);

		if (cr == 1)
		{
			struct HighScore *hs;

			hs = AllocPooled(app->Selector.HighScorePool, sizeof(struct HighScore));

			if (hs)
			{
				hs->Seconds = hsr.seconds;
				hs->Moves = hsr.moves;
				AddTail((struct List*)&app->Selector.HighScores, (struct Node*)hs);
				app->LevelNumber++;
				app->Selector.EntryCount++;
			}
			else cr = ERROR_NO_FREE_STORE;
		}
	}
	while (cr == 1);

	if (cr < 0) StateError(app, "reading highscores", cr);
}

/*---------------------------------------------------------------------------*/

static void LoadWinPositions(struct App *app, struct IFFHandle *in)
{
	struct WinPosRecord wpr[2];
	LONG err = 0;

	err = ReadChunkRecords(in, wpr, sizeof(struct WinPosRecord) * 2, 1);

	if (err == 1)
	{
		app->MainWinPos = wpr[0];
		app->Selector.SelWinPos = wpr[1];
	}
	else StateError(app, "reading window positions", err);
}

/*---------------------------------------------------------------------------*/

static void ParseState(struct App *app, struct IFFHandle *in)
{
	LONG err;
	struct ContextNode *curch;
	UBYTE strid[5];

	StopChunk(in, ID_UNTG, ID_HSCR);
	StopChunk(in, ID_UNTG, ID_WINP);

	do
	{
		if (!(err = ParseIFF(in, IFFPARSE_SCAN)))
		{
			curch = CurrentChunk(in);

			switch (curch->cn_ID)
			{
				case ID_HSCR:   LoadHighScores(app, in);	break;
				case ID_WINP:   LoadWinPositions(app, in);  break;
			}
		}
	}
	while (!err);

	if (err != IFFERR_EOF) StateError(app, "parse", err);
}

/*--------------------------------------------------------------------------*/

static void LoadState2(struct App *app, BPTR input)
{
	struct IFFHandle *in;

	if (in = AllocIFF())
	{
		LONG ecode;

		in->iff_Stream = input;
		InitIFFasDOS(in);

		if ((ecode = OpenIFF(in, IFFF_READ)) == 0)
		{
			ParseState(app, in);
			CloseIFF(in);
		}
		else StateError(app, "opening IFF stream for read", ecode);

		FreeIFF(in);
	}
	else StateError(app, "opening IFF stream for read", ERROR_NO_FREE_STORE);
}

/*--------------------------------------------------------------------------*/

void LoadState(struct App *app)
{
	BPTR input;

	if (input = Open("PROGDIR:Untangle.state", MODE_OLDFILE))
	{
		LoadState2(app, input);
		Close(input);
	}
	else 
	{
		LONG err = IoErr();

		// do not report "file not found", it may happen and should be
		// silently ignored

		if (err != ERROR_OBJECT_NOT_FOUND)
			StateError(app, "opening file for read", IoErr());
	}
}
