/* Untangle: string utilities */

#define StrAlloc(len) (STRPTR)AllocVec((len), MEMF_ANY)
#define StrFree(str) FreeVec(str)

ULONG VFmtLen(STRPTR fmt, APTR *args);
void VFmtPut(STRPTR dest, STRPTR fmt, APTR *args);
STRPTR VFmtNew(STRPTR fmt, APTR *args);
ULONG StrLen(STRPTR s);
STRPTR StrCopy(STRPTR s, STRPTR d);
STRPTR StrClone(STRPTR s);
STRPTR VStrJoin(STRPTR d, STRPTR *strs, WORD count);
STRPTR VStrMerge(STRPTR *strs, WORD count);
