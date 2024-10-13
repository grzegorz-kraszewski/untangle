/*------------------*/
/* loader of levels */
/*------------------*/

#include "loader.h"
#include "strutils.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <exec/memory.h>


/*---------------------------------------------------------------------------*/

static LONG CheckLineDuplicate(struct GameLevel *glv, WORD sdotidx, WORD edotidx)
{
	struct GameDot *sdotptr = glv->DotStorage + sdotidx;
	struct GameDot *edotptr = glv->DotStorage + edotidx;
	struct GameLine *gl;

	ForEachFwd(&glv->LineList, struct GameLine, gl)
	{
		if (((sdotptr == gl->StartDot) && (edotptr == gl->EndDot))
			|| ((sdotptr == gl->EndDot) && (edotptr == gl->StartDot)))
			return LERR_DUPLICATE_LINE;
	}

	return 0;
}

/*---------------------------------------------------------------------------*/

static LONG VerifyLine(struct GameLevel *glv, WORD sdotidx, WORD edotidx)
{
	if ((sdotidx < glv->DotCount) && (edotidx < glv->DotCount))
		return CheckLineDuplicate(glv, sdotidx, edotidx);
	else return LERR_DOT_INDEX_OUT_OF_RANGE;
}

/*---------------------------------------------------------------------------*/
/* Why limit of number of lines is not checked?                              */
/* 1. Out of range dot indexes are detected, level is rejected if found.     */
/* 2. Duplicated lines are detected, level is rejected if found.             */
/* These two impose upper limit of lines: complete graph of given number of  */
/* dots. As dots are limited to 256 including, lines are implicitly limited  */
/* to 32640.                                                                 */
/*---------------------------------------------------------------------------*/

static LONG LoadLines(struct GameLevel *gl, struct IFFHandle *level, WORD count)
{
	struct GameLine *gli;
	LONG err = 0;

	gl->LineCount = count;

	if (gli = AllocVec(sizeof(struct GameLine) * count, MEMF_ANY))
	{
		if (gl->Intersections = AllocVec(mul16(count, count - 1) / 2, MEMF_ANY | MEMF_CLEAR))
		{
			LONG i;

			gl->LineStorage = gli;

			for (i = 0; i < count; i++)
			{
				UBYTE buf[2];

				if ((err = ReadChunkRecords(level, buf, 2, 1)) == 1)
				{
					if (!(err = VerifyLine(gl, buf[0], buf[1])))
					{
						gli->StartDot = gl->DotStorage + buf[0];
						gli->EndDot = gl->DotStorage + buf[1];
						gli->Index = i;
						AddTail((struct List*)&gl->LineList, (struct Node*)gli);
						gli++;
					}
					else break;
				}
				else break;
			}

			if (err == 1) err = 0;    /* read one chunk record in the last loop turn -> OK */
		}
		else err = LERR_OUT_OF_MEMORY;
	}
	else err = LERR_OUT_OF_MEMORY;

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG LoadDots(struct GameLevel *gl, struct IFFHandle *handle, WORD count)
{
	struct GameDot *gd;
	LONG err = 0;

	if (count <= 256)
	{
		gl->DotCount = count;

		if (gd = AllocVec(sizeof(struct GameDot) * count, MEMF_ANY))
		{
			LONG i;

			gl->DotStorage = gd;

			for (i = 0; i < count; i++)
			{
				UBYTE buf[4];

				if ((err = ReadChunkRecords(handle, buf, 4, 1)) == 1)
				{
					gd->Virtual.x = *(WORD*)&buf[0];
					gd->Virtual.y = *(WORD*)&buf[2];

					if ((gd->Virtual.x >= 0) && (gd->Virtual.y >= 0))
					{
						AddTail((struct List*)&gl->DotList, (struct Node*)gd);
						gd++;
					}
					else
					{
						err = LERR_DOT_COORDINATE_NEGATIVE;
						break;
					}
				}
				else break;
			}

			if (err == 1) err = 0;    /* read one chunk record in the last loop turn -> OK */
		}
		else err = LERR_OUT_OF_MEMORY;
	}
	else err = LERR_TOO_MANY_DOTS;

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG LoadLevelData(struct GameLevel *gl, struct IFFHandle *handle)
{
	LONG err;
	BOOL have_lines = FALSE;
	BOOL have_dots = FALSE;
	struct ContextNode *curch;

	StopChunk(handle, ID_UNTG, ID_DOTS);
	StopChunk(handle, ID_UNTG, ID_LINE);
	StopOnExit(handle, ID_UNTG, ID_FORM);

	do
	{
		if (!(err = ParseIFF(handle, IFFPARSE_SCAN)))
		{
			curch = CurrentChunk(handle);

			switch (curch->cn_ID)
			{
				case ID_DOTS:
					if (!(err = LoadDots(gl, handle, curch->cn_Size / 4))) have_dots = TRUE;
				break;

				case ID_LINE:
					if (have_dots)
					{
						 if (!(err = LoadLines(gl, handle, curch->cn_Size / 2))) have_lines = TRUE;
					}
					else err = LERR_LINES_BEFORE_DOTS;
				break;
			}
		}
	}
	while (!err);

	if (err == IFFERR_EOC)  /* it means parsing of level FORM as been finished */
	{
		err = 0;
		if (!have_lines) err = LERR_MISSING_LINES;
		if (!have_dots) err = LERR_MISSING_DOTS;
	}

	return err;
}

/*---------------------------------------------------------------------------*/

APTR CopyProperty(struct IFFHandle *handle, ULONG type, ULONG id)
{
	struct StoredProperty *sp;
	APTR pdata = NULL;

	if (sp = FindProp(handle, type, id))
	{
		if (pdata = AllocVec(sp->sp_Size, MEMF_ANY))
		{
			CopyMem(sp->sp_Data, pdata, sp->sp_Size);
		}
	}

	return pdata;
}

/*---------------------------------------------------------------------------*/

void StoreLevelProperties(struct GameLevel *gl, struct IFFHandle *handle)
{
	/* Properties are optional, so when they are missing, or cannot be copied */
	/* due to lack of free memory, it is not fatal.                           */

	gl->LevelSetName = CopyProperty(handle, ID_UNTG, ID_SNME);
	gl->LevelSetAuthor = CopyProperty(handle, ID_UNTG, ID_AUTH);	
}

/*---------------------------------------------------------------------------*/

static LONG SkipToLevel(struct GameLevel *gl, struct IFFHandle *handle, LONG level)
{
	LONG err;
	const LONG propchunks[4] = { ID_UNTG, ID_AUTH, ID_UNTG, ID_SNME };

	if ((err = OpenIFF(handle, IFFF_READ)) == 0)
	{
		if (!(err = PropChunks(handle, propchunks, 2)))
		{
			StopChunk(handle, ID_UNTG, ID_FORM);

			while (!err && (level > 0))
			{
				err = ParseIFF(handle, IFFPARSE_SCAN);
				if (!err) level--;
			}

			if (!err) err = LoadLevelData(gl, handle);
			else if (err == IFFERR_EOF)  /* file ended before desired level has been reached */
			{
				err = LERR_NO_SUCH_LEVEL;
			}

			if (!err) StoreLevelProperties(gl, handle); 
		}

		CloseIFF(handle);
	}

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG OpenFile(struct GameLevel *gl, struct IFFHandle *handle, LONG level, STRPTR filename)
{
	LONG err = LERR_FILE_OPEN_FAILED;
	BPTR levfile;

	if (levfile = Open(filename, MODE_OLDFILE))
	{
		handle->iff_Stream = levfile;
		InitIFFasDOS(handle);
		err = SkipToLevel(gl, handle, level);
		Close(levfile);
	}

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG LoadLevel2(struct GameLevel *gl, LONG level, STRPTR filename)
{
	struct IFFHandle *handle;
	LONG err;

	if (handle = AllocIFF())
	{
		err = OpenFile(gl, handle, level, filename);
		FreeIFF(handle);
	}
	else err = LERR_OUT_OF_MEMORY;

	return err;
}

/*---------------------------------------------------------------------------*/

static STRPTR LoadErrorMessages[] = {
	"Out of memory.",
	"Can't open level set \"StandardSet.iff\":\n%s.",
	"No 'DOTS' chunk found in level.",
	"No 'LINE' chunk found in level.",
	"'LINE' chunk encountered before 'DOTS' one.",
	"More than 256 dots defined in the file.",
	"Negative dot coordinates are not allowed.",
	"Index of non-existent dot in a line definition.",
	"Duplicate line detected.",
	"No more levels in set."
};

static void ReportLoadError(struct Window *gwin, LONG err)
{
	STRPTR extrainfo = NULL;
	UBYTE doserrbuf[88];

	struct EasyStruct es = {
		sizeof(struct EasyStruct),
		0,
		"Untangle Level Loader",
		NULL,
		"OK"
	};

	if (err == LERR_FILE_OPEN_FAILED)
	{
		Fault(IoErr(), "", doserrbuf, 88);
		extrainfo = &doserrbuf[2];
	}

	/*-----------------*/
	/* iffparse errors */
	/*-----------------*/

	if ((err < 0) && (err >= IFF_RETURN2CLIENT))
	{
		es.es_TextFormat = "IFF structure parsing error %ld.";
		extrainfo = (STRPTR)err; 
	}
	else es.es_TextFormat = LoadErrorMessages[err - 1];	

	EasyRequest(gwin, &es, NULL, (LONG)extrainfo);
}

/*---------------------------------------------------------------------------*/

void UnloadLevel(struct GameLevel *gl)
{
	if (gl)
	{
		if (gl->DotStorage) FreeVec(gl->DotStorage);
		if (gl->LineStorage) FreeVec(gl->LineStorage);
		if (gl->Intersections) FreeVec(gl->Intersections);
		if (gl->LevelSetName) FreeVec(gl->LevelSetName);
		if (gl->LevelSetAuthor) FreeVec(gl->LevelSetAuthor);
		FreeMem(gl, sizeof(struct GameLevel));
	}
}

/*---------------------------------------------------------------------------*/

struct GameLevel* LoadLevel(struct Window *gwin, LONG level, STRPTR filename, BOOL *endgame)
{
	struct GameLevel *gl;
	LONG err = LERR_OUT_OF_MEMORY;

	if (gl = (struct GameLevel*)AllocMem(sizeof(struct GameLevel), MEMF_ANY))
	{
		gl->DotStorage = NULL;
		gl->LineStorage = NULL;
		InitList(&gl->DotList);
		InitList(&gl->LineList);
		InitList(&gl->DraggedLines);
		gl->DraggedDot = NULL;
		gl->Intersections = NULL;
		gl->LevelSetName = NULL;
		gl->LevelSetAuthor = NULL;
		gl->MoveCount = 0;
		err = LoadLevel2(gl, level, filename);
	}

	if (err)
	{
		if (err == LERR_NO_SUCH_LEVEL) *endgame = TRUE;
		else ReportLoadError(gwin, err);
		UnloadLevel(gl);
		gl = NULL;
	}

	return gl;
}
