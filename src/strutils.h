/* Untangle: string utilities */

#define StrAlloc(len) (STRPTR)AllocVec((len), MEMF_ANY)
#define StrFree(str) FreeVec(str)

ULONG VFmtLen(STRPTR fmt, LONG *args);
void VFmtPut(STRPTR dest, STRPTR fmt, LONG *args);
void FmtPut(STRPTR dest, STRPTR fmt, LONG arg1, ...);
STRPTR VFmtNew(STRPTR fmt, LONG *args);
STRPTR FmtNew(STRPTR fmt, LONG arg1, ...);
ULONG StrLen(STRPTR s);
STRPTR StrCopy(STRPTR s, STRPTR d);
STRPTR StrClone(STRPTR s);
STRPTR VStrJoin(STRPTR d, STRPTR *strs, WORD count);
STRPTR VStrMerge(STRPTR *strs, WORD count);
