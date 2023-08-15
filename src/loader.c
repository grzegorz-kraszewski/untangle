/*------------------*/
/* loader of levels */
/*------------------*/

#include "main.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <exec/memory.h>

#define ID_UNTG MAKE_ID('U','N','T','G')
#define ID_DOTS MAKE_ID('D','O','T','S')
#define ID_LINE MAKE_ID('L','I','N','E')

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

void OpenStructure(struct App *app, struct IFFHandle *level)
{
	if (OpenIFF(level, IFFF_READ) == 0)
	{
		BOOL have_lines = FALSE;
		BOOL have_dots = FALSE;
		struct ContextNode *curch;
		LONG error = 0;
		UBYTE strid[5];
		
		StopChunk(level, ID_UNTG, ID_DOTS);
		StopChunk(level, ID_UNTG, ID_LINE);
		
		for (;;)
		{
			error = ParseIFF(level, IFFPARSE_SCAN);
			if (error) break;
			curch = CurrentChunk(level);
			
			switch(curch->cn_ID)
			{
				case ID_DOTS:
					have_dots = LoadDots(app, level, curch->cn_Size / 4);
				break;
				
				case ID_LINE:
					if (have_dots) have_lines = LoadLines(app, level, curch->cn_Size / 2);
				break;
			}
		}

		CloseIFF(level);
	}
}

/*---------------------------------------------------------------------------*/

void OpenFile(struct App *app, struct IFFHandle *level)
{
	BPTR levfile;

	if (levfile = Open("PROGDIR:level.iff", MODE_OLDFILE))
	{
		level->iff_Stream = levfile;
		InitIFFasDOS(level);
		OpenStructure(app, level);
		Close(levfile);
	}
}

/*---------------------------------------------------------------------------*/

void LoadLevel(struct App *app)
{
	struct IFFHandle *level;

	if (level = AllocIFF())
	{
		OpenFile(app, level);
		FreeIFF(level);
	} 	
}

