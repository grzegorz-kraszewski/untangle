#include <libraries/iffparse.h>

#define ID_UNTG MAKE_ID('U','N','T','G')
#define ID_HSCR MAKE_ID('H','S','C','R')
#define ID_WINP MAKE_ID('W','I','N','P')

struct HighScoreRecord
{
	LONG seconds;
	LONG moves;
};

void SaveState(struct App *app);
void LoadState(struct App *app);
void StateError(struct App *app, STRPTR opdesc, LONG ecode);
