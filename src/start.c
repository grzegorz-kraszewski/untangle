/*---------------------------------*/
/* Minimal startup for AmigaOS 3.x */
/* Grzegorz Kraszewski, 2016       */
/*---------------------------------*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <workbench/startup.h>

#include "version.h"

struct Library *SysBase;
struct Library *DOSBase;

UBYTE DOSName[];

extern ULONG Main(struct WBStartup*);


__saveds ULONG Start(void)
{
	struct Process *myproc = NULL;
	struct WBStartup *wbmsg = NULL;
	BOOL have_shell = FALSE;
	ULONG result = RETURN_OK;

	SysBase = *(struct Library**)4L;
	myproc = (struct Process*)FindTask(NULL);
	if (myproc->pr_CLI) have_shell = TRUE;

	if (!have_shell)
	{
		WaitPort(&myproc->pr_MsgPort);
		wbmsg = (struct WBStartup*)GetMsg(&myproc->pr_MsgPort);
	}

	if (DOSBase = OpenLibrary(DOSName, 39))
	{
		result = Main(wbmsg);
		CloseLibrary(DOSBase);
	}
	else result = RETURN_FAIL;

	if (wbmsg)
	{
		Forbid();
		ReplyMsg(&wbmsg->sm_Message);
	}

	return result;
}

UBYTE DOSName[] = "dos.library";

__attribute__((section(".text"))) UBYTE VString[] =
	"$VER: Untangle " VERSION " (" RELDATE ")\r\n";
