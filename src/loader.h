/*-------------------------------------*/
/* prototypes for handling level files */
/*-------------------------------------*/

#include "main.h"

struct GameLevel *LoadLevel(void);

/*---------------------*/
/* loading error codes */
/*---------------------*/

#define LERR_OUT_OF_MEMORY                   1
#define LERR_FILE_OPEN_FAILED                2
