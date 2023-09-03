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
#define LERR_MISSING_DOTS                    3
#define LERR_MISSING_LINES                   4
#define LERR_LINES_BEFORE_DOTS               5
