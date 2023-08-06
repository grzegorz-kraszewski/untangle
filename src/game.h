#include "main.h"

void NewGame(struct App *app);
void DisposeGame(struct App *app);
void DrawGame(struct App *app);
void ScaleGame(struct App *app);
void EraseGame(struct App *app);
void GameClick(struct App *app, WORD x, WORD y);
void GameUnclick(struct App *app, WORD x, WORD y);
void GameDotDrag(struct App *app, WORD x, WORD y);
