/*-------------------------------------*/
/* prototypes for handling level files */
/*-------------------------------------*/

#include "main.h"

#define ID_UNTG MAKE_ID('U','N','T','G')
#define ID_DOTS MAKE_ID('D','O','T','S')
#define ID_LINE MAKE_ID('L','I','N','E')
#define ID_SNME MAKE_ID('S','N','M','E')
#define ID_AUTH MAKE_ID('A','U','T','H')

struct GameLevel *LoadLevel(struct Window *gamewin, LONG level, STRPTR filename, BOOL *endgame);
void UnloadLevel(struct GameLevel *gl);

/*---------------------*/
/* loading error codes */
/*---------------------*/

#define LERR_OUT_OF_MEMORY                   1
#define LERR_FILE_OPEN_FAILED                2
#define LERR_MISSING_DOTS                    3
#define LERR_MISSING_LINES                   4
#define LERR_LINES_BEFORE_DOTS               5
#define LERR_TOO_MANY_DOTS                   6
#define LERR_DOT_COORDINATE_NEGATIVE         7
#define LERR_DOT_INDEX_OUT_OF_RANGE          8
#define LERR_DUPLICATE_LINE                  9
#define LERR_NO_SUCH_LEVEL                  10