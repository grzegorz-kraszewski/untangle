#include "main.h"
#include "game.h"
#include "selector.h"

#include <proto/intuition.h>
#include <proto/gadtools.h>

#define ACTION_NEW            43
#define ACTION_QUIT           44
#define ACTION_SELECT_LEVEL   45
#define ACTION_SAVE_LEVEL     46

struct NewMenu AppMenu[] = {
	{ NM_TITLE, "Project", 0, 0, 0, 0 },
	{ NM_ITEM, "New Game", "N", 0, 0, (APTR)ACTION_NEW },
	{ NM_ITEM, "Level Selector...", "L", 0, 0, (APTR)ACTION_SELECT_LEVEL },
	{ NM_ITEM, "Save Level...", "S", 0, 0, (APTR)ACTION_SAVE_LEVEL },
	{ NM_ITEM, NM_BARLABEL, 0, 0, 0, 0},
	{ NM_ITEM, "Quit", "Q", 0, 0, (APTR)ACTION_QUIT },
	{ NM_END, NULL, 0, 0, 0, 0 }
};


static BOOL Action(struct App *app, ULONG action)
{
	BOOL running = TRUE;
	
	switch (action)
	{
		case ACTION_NEW:
			EraseGame(app);
			UnloadLevel(app->Level);
			app->Level = NULL;
			app->LevelNumber = 1;
			NewGame(app);
		break;
		
		case ACTION_QUIT:
			running = FALSE;
		break;

		case ACTION_SAVE_LEVEL:
			SaveLevel(app);
		break;

		case ACTION_SELECT_LEVEL:
			if (app->Selector.Win) WindowToFront(app->Selector.Win);
			else OpenSelector(app->Win, &app->Selector);
		break;
	}
	
	return running;
}


BOOL HandleMenu(struct App *app, UWORD menucode)
{
	BOOL running = TRUE;
	struct MenuItem *mit;

	while (menucode != MENUNULL)
	{					
		if (mit = ItemAddress(app->WinMenu, menucode))
		{
			running = Action(app, (ULONG)GTMENUITEM_USERDATA(mit));
			menucode = mit->NextSelect;
		}
	}
	
	return running;
}



LONG SetupMenus(struct App *app)
{
	LONG err = SERR_MENU_LAYOUT;
	APTR vi;

	if (vi = GetVisualInfo(app->Win->WScreen, TAG_END))
	{
		if (app->WinMenu = CreateMenus(AppMenu, TAG_END))
		{
			if (LayoutMenus(app->WinMenu, vi,
			 GTMN_NewLookMenus, TRUE, 
			TAG_END))
			{
				if (SetMenuStrip(app->Win, app->WinMenu))
				{
					err = 0;
					TheLoop(app);
					UnloadLevel(app->Level);
					ClearMenuStrip(app->Win);
				}
			}

			FreeMenus(app->WinMenu);
		}

		FreeVisualInfo(vi);
	}

	return err;
}
