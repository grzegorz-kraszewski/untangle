/*-------------------------------------*/
/* prototypes for handling level files */
/*-------------------------------------*/

#include "main.h"

struct GameLevel *LoadLevel(struct Window *gamewin);

/*---------------------*/
/* loading error codes */
/*---------------------*/

#define LERR_OUT_OF_MEMORY                   1
#define LERR_FILE_OPEN_FAILED                2
