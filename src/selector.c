/*-----------------------------------*/
/* level selector / high score table */
/*-----------------------------------*/

#include "selector.h"

#include <proto/exec.h>
#include <proto/intuition.h>

struct Image ScrRender = {0};
struct PropInfo ScrProp = {0};
struct Gadget Scroller = {0};


void OpenSelector(struct App *app)
{
	STRPTR title;

	ScrProp.Flags = FREEVERT | PROPNEWLOOK | PROPBORDERLESS | AUTOKNOB;
	ScrProp.HorizPot = 0;
	ScrProp.HorizBody = MAXBODY;
	ScrProp.VertPot = 0;
	ScrProp.VertBody = MAXBODY;

	Scroller.TopEdge = app->Win->BorderTop + 1;
	Scroller.LeftEdge = -app->Win->BorderRight + 2;
	Scroller.Width = app->Win->BorderRight - 4;
	Scroller.Height = -Scroller.TopEdge - 11;
	Scroller.Flags = GFLG_GADGHNONE | GFLG_RELRIGHT | GFLG_RELHEIGHT;
	Scroller.Activation = GACT_RELVERIFY | GACT_IMMEDIATE | GACT_RIGHTBORDER;
	Scroller.GadgetType = GTYP_PROPGADGET;
	Scroller.GadgetRender = &ScrRender;
	Scroller.SpecialInfo = &ScrProp;

	if (app->Selector = OpenWindowTags(NULL,
		WA_Width, 400,
		WA_MinWidth, 400,
		WA_MaxWidth, 400,
		WA_Height, 400,
		WA_MinHeight, 100,
		WA_MaxHeight, 1024,
		WA_DragBar, TRUE,
		WA_CloseGadget, TRUE,
		WA_DepthGadget, TRUE,
		WA_SizeGadget, TRUE,
		WA_Title, app->SelectorWindowTitle,
		WA_ScreenTitle, app->DynamicScreenTitle,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MOUSEBUTTONS,
		WA_Gadgets, (ULONG)&Scroller,
		WA_NewLookMenus, TRUE,
		WA_Activate, TRUE,
	TAG_END))
	{
		app->SelectorMask = 1 << app->Selector->UserPort->mp_SigBit;
	}
}

/* Called when the main loop receives signal from selector window IDCMP port. */

void HandleSelector(struct App *app)
{
	struct IntuiMessage *imsg;
	BOOL closed = FALSE;

	while (imsg = (struct IntuiMessage*)GetMsg(app->Selector->UserPort))
	{
		switch (imsg->Class)
		{
			case IDCMP_CLOSEWINDOW:
				closed = TRUE;
			break;
		}

		ReplyMsg(&imsg->ExecMessage);
	}

	if (closed)
	{
		CloseWindow(app->Selector);
		app->Selector = NULL;
		app->SelectorMask = 0;
	}
}

