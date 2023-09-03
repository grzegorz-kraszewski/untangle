/*------------------*/
/* loader of levels */
/*------------------*/

#include "loader.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <exec/memory.h>

#define ID_UNTG MAKE_ID('U','N','T','G')
#define ID_DOTS MAKE_ID('D','O','T','S')
#define ID_LINE MAKE_ID('L','I','N','E')

/*---------------------------------------------------------------------------*/

static BOOL LoadLines(struct GameLevel *gl, struct IFFHandle *level, LONG count)
{
	struct GameLine *gli;
	LONG err = 0;

	if (gli = AllocVec(sizeof(struct GameLine) * count, MEMF_ANY))
	{
		LONG i;
		
		gl->LineStorage = gli;

		for (i = 0; i < count; i++)
		{
			UBYTE buf[2];

			if ((err = ReadChunkRecords(level, buf, 2, 1)) == 1)
			{
				gli->StartDot = gl->DotStorage + buf[0];
				gli->EndDot = gl->DotStorage + buf[1];
				gli->Index = i;
				AddTail((struct List*)&gl->LineList, (struct Node*)gli);
				gli++;
			}
			else break;
		}

		if (err == 1) err = 0;    /* read one chunk record in the last loop turn -> OK */
	}
	else err = LERR_OUT_OF_MEMORY;

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG LoadDots(struct GameLevel *gl, struct IFFHandle *handle, LONG count)
{
	struct GameDot *gd;
	LONG err = 0;

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
				AddTail((struct List*)&gl->DotList, (struct Node*)gd);
				gd++;
			}
			else break;
		}

		if (err == 1) err = 0;    /* read one chunk record in the last loop turn -> OK */
	}
	else err = LERR_OUT_OF_MEMORY;

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG OpenStructure(struct GameLevel *gl, struct IFFHandle *handle)
{
	LONG err;

	if ((err = OpenIFF(handle, IFFF_READ)) == 0)
	{
		BOOL have_lines = FALSE;
		BOOL have_dots = FALSE;
		struct ContextNode *curch;
		UBYTE strid[5];

		StopChunk(handle, ID_UNTG, ID_DOTS);
		StopChunk(handle, ID_UNTG, ID_LINE);

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

		if (err == IFFERR_EOF)  /* it means parsing has been finished with no errors */
		{
			err = 0;
			if (!have_lines) err = LERR_MISSING_LINES;
			if (!have_dots) err = LERR_MISSING_DOTS;
		}

		CloseIFF(handle);
	}

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG OpenFile(struct GameLevel *gl, struct IFFHandle *handle)
{
	LONG err = LERR_FILE_OPEN_FAILED;
	BPTR levfile;

	SetIoErr(0);

	if (levfile = Open("PROGDIR:level.iff", MODE_OLDFILE))
	{
		handle->iff_Stream = levfile;
		InitIFFasDOS(handle);
		err = OpenStructure(gl, handle);
		Close(levfile);
	}

	return err;
}

/*---------------------------------------------------------------------------*/

static LONG LoadLevel2(struct GameLevel *gl)
{
	struct IFFHandle *handle;
	LONG err;

	if (handle = AllocIFF())
	{
		err = OpenFile(gl, handle);
		FreeIFF(handle);
	}
	else err = LERR_OUT_OF_MEMORY;

	return err;
}

/*---------------------------------------------------------------------------*/

static STRPTR LoadErrorMessages[] = {
	"Out of memory.",
	"Can't open file \"PROGDIR:level.iff\":\n%s.",
	"No 'DOTS' chunk found in level file.",
	"No 'LINE' chunk found in level file.",
	"'LINE' chunk encountered before 'DOTS' one."
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
		FreeMem(gl, sizeof(struct GameLevel));
	}
}

/*---------------------------------------------------------------------------*/

struct GameLevel* LoadLevel(struct Window *gwin)
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
		err = LoadLevel2(gl);
	}

	if (err)
	{
		ReportLoadError(gwin, err);
		UnloadLevel(gl);
		gl = NULL;
	}

	return gl;
}
