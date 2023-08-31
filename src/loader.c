/*------------------*/
/* loader of levels */
/*------------------*/

#include "loader.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <exec/memory.h>

#define ID_UNTG MAKE_ID('U','N','T','G')
#define ID_DOTS MAKE_ID('D','O','T','S')
#define ID_LINE MAKE_ID('L','I','N','E')

#if 0
/*---------------------------------------------------------------------------*/

BOOL LoadLines(struct App *app, struct IFFHandle *level, LONG count)
{
	struct GameLine *gl;
	
	if (gl = AllocVec(sizeof(struct GameLine) * count, MEMF_ANY))
	{
		LONG i;
		
		app->LineStorage = gl;
		
		for (i = 0; i < count; i++)
		{
			UBYTE buf[2];
			
			if (ReadChunkRecords(level, buf, 2, 1) == 1)
			{
				gl->StartDot = app->DotStorage + buf[0];
				gl->EndDot = app->DotStorage + buf[1];
				gl->Index = i;
				AddTail((struct List*)&app->LineList, (struct Node*)gl);
				gl++;
			}
		}
		
		return TRUE;
	}
	
	return FALSE;
}

/*---------------------------------------------------------------------------*/

BOOL LoadDots(struct App *app, struct IFFHandle *level, LONG count)
{
	struct GameDot *gd;

	if (gd = AllocVec(sizeof(struct GameDot) * count, MEMF_ANY))
	{
		LONG i;

		app->DotStorage = gd;

		for (i = 0; i < count; i++)
		{
			UBYTE buf[4];
			
			if (ReadChunkRecords(level, buf, 4, 1) == 1)
			{
				gd->Virtual.x = *(WORD*)&buf[0];
				gd->Virtual.y = *(WORD*)&buf[2];
				AddTail((struct List*)&app->DotList, (struct Node*)gd);
				gd++;
			}
		}
		
		return TRUE;
	}
	
	return FALSE;
}

/*---------------------------------------------------------------------------*/

#endif

LONG OpenStructure(struct GameLevel *gl, struct IFFHandle *handle)
{
	if (OpenIFF(handle, IFFF_READ) == 0)
	{
		BOOL have_lines = FALSE;
		BOOL have_dots = FALSE;
		struct ContextNode *curch;
		LONG error = 0;
		UBYTE strid[5];
		
		StopChunk(handle, ID_UNTG, ID_DOTS);
		StopChunk(handle, ID_UNTG, ID_LINE);
		
		for (;;)
		{
			error = ParseIFF(handle, IFFPARSE_SCAN);
			if (error) break;
			curch = CurrentChunk(handle);
			
			switch(curch->cn_ID)
			{
				case ID_DOTS:
					//have_dots = LoadDots(app, level, curch->cn_Size / 4);
				break;
				
				case ID_LINE:
					//if (have_dots) have_lines = LoadLines(app, level, curch->cn_Size / 2);
				break;
			}
		}

		CloseIFF(handle);
	}
}

/*---------------------------------------------------------------------------*/

static LONG OpenFile(struct GameLevel *gl, struct IFFHandle *handle)
{
	LONG err;
	BPTR levfile;

	if (levfile = Open("PROGDIR:level.iff", MODE_OLDFILE))
	{
		handle->iff_Stream = levfile;
		InitIFFasDOS(handle);
		//err = OpenStructure(gl, handle);
		err = LERR_FILE_OPEN_FAILED;
		Close(levfile);
	}
	else err = LERR_FILE_OPEN_FAILED;
	
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
};

static void ReportLoadError(LONG err)
{
}

/*---------------------------------------------------------------------------*/

struct GameLevel* LoadLevel()
{
	struct GameLevel *gl;
	
	if (gl = (struct GameLevel*)AllocMem(sizeof(struct GameLevel), MEMF_ANY))
	{
		LONG err;
		
		err = LoadLevel2(gl);
		
		if (err)
		{
			ReportLoadError(err);
			FreeMem(gl, sizeof(struct GameLevel));
			gl = NULL;
		}
	}
	else ReportLoadError(LERR_OUT_OF_MEMORY);
	
	return gl;
}

