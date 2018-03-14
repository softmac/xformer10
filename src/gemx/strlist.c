/* strlist.c */


#include "precomp.h"
#include "strlist.h"


#define STRINGLISTINITIALSIZE 8
#define STRINGLISTGROWSIZE    8

#pragma pack(push, 4)
typedef struct tagStrListHeader
{
    int cStrings;
    int cStringSlots;
    char** rgsz;
} STRINGLISTHEADER;
#pragma pack(pop)


HSTRLIST StrListCreate()
{
    STRINGLISTHEADER* pslh;

    pslh = (STRINGLISTHEADER*)malloc(sizeof(STRINGLISTHEADER));
    if (pslh == NULL)
        return (HSTRLIST)NULL;

    pslh->rgsz = (char**)malloc(STRINGLISTINITIALSIZE * sizeof(char*));
    if (pslh->rgsz == NULL)
    {
        free(pslh);
        return (HSTRLIST)NULL;
    }

    pslh->cStrings = 0;
    pslh->cStringSlots = STRINGLISTINITIALSIZE;

    ASSERT(StrListIsValid((HSTRLIST)pslh));

    return (HSTRLIST)pslh;
} /* StrListCreate */


void StrListDestroy(HSTRLIST hsl)
{
    STRINGLISTHEADER* pslh;

    if (hsl == (HSTRLIST)NULL)
        return;

    ASSERT(StrListIsValid(hsl));

    pslh = (STRINGLISTHEADER*)hsl;
    StrListFreeAll(hsl);
    free(pslh->rgsz);
    free(pslh);
} /* StrListDestroy */


int StrListGetCount(HSTRLIST hsl)
{
    STRINGLISTHEADER* pslh;

    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));

    pslh = (STRINGLISTHEADER*)hsl;
    return pslh->cStrings;
} /* StrListGetCount */


const char* StrListGet(HSTRLIST hsl, int i)
{
    STRINGLISTHEADER* pslh;

    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));
    ASSERT(i >= 0);
    ASSERT(i < ((STRINGLISTHEADER*)hsl)->cStrings);

    pslh = (STRINGLISTHEADER*)hsl;
    return pslh->rgsz[i];
} /* StrListGet */


BOOL StrListAdd(HSTRLIST hsl, const char* sz)
{
    STRINGLISTHEADER* pslh;
    DWORD cb;

    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));
    ASSERT(sz != NULL);

    pslh = (STRINGLISTHEADER*)hsl;
    if (pslh->cStrings == pslh->cStringSlots)
    {
        char** rgsz;

        rgsz = (char**)realloc(pslh->rgsz, (pslh->cStringSlots + STRINGLISTGROWSIZE) * sizeof(char*));
        if (rgsz == NULL)
            return FALSE;

        pslh->rgsz = rgsz;
        pslh->cStringSlots += STRINGLISTGROWSIZE;
    }

    cb = strlen(sz) + 1;
    pslh->rgsz[pslh->cStrings] = (char*)malloc(cb);
    if (pslh->rgsz[pslh->cStrings] == (char*)NULL)
        return FALSE;

    CopyMemory(pslh->rgsz[pslh->cStrings], sz, cb);
    pslh->cStrings++;
    return TRUE;
} /* StrListAdd */


void StrListRemove(HSTRLIST hsl, int i)
{
    STRINGLISTHEADER* pslh;

    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));
    ASSERT(i >= 0);
    ASSERT(i < ((STRINGLISTHEADER*)hsl)->cStrings);

    pslh = (STRINGLISTHEADER*)hsl;
    free(pslh->rgsz[i]);
    MoveMemory(&pslh->rgsz[i], &pslh->rgsz[i + 1], (pslh->cStrings - i - 1) * sizeof(char*));
    pslh->cStrings--;
} /* StrListRemove */


void StrListFreeAll(HSTRLIST hsl)
{
    STRINGLISTHEADER* pslh;
    char** psz;
    int cLeft;

    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));

    pslh = (STRINGLISTHEADER*)hsl;
    psz = pslh->rgsz;
    cLeft = pslh->cStrings;
    while(cLeft-- > 0)
    {
        free(*psz);
        psz++;
    }

#ifdef _DEBUG
    ZeroMemory(pslh->rgsz, pslh->cStringSlots * sizeof(char*));
#endif /* _DEBUG */

    pslh->cStrings = 0;

    /* TODO: could possibly shrink pslh->rgsz here */
} /* StrListFreeAll */


#ifdef _DEBUG
BOOL StrListIsValid(HSTRLIST hsl)
{
    STRINGLISTHEADER* pslh;
    char** psz;
    int cLeft;

    pslh = (STRINGLISTHEADER*)hsl;

    ASSERT(pslh != (STRINGLISTHEADER*)NULL);
    ASSERT(pslh->cStrings >= 0);
    ASSERT(pslh->cStringSlots >= 0);
    ASSERT(pslh->cStringSlots >= STRINGLISTINITIALSIZE);
    ASSERT(pslh->cStrings <= pslh->cStringSlots);
    ASSERT(pslh->rgsz != NULL);

    psz = pslh->rgsz;
    cLeft = pslh->cStrings;
    while(cLeft-- > 0)
    {
        ASSERT(*psz != NULL);
        psz++;
    }

    return TRUE;
} /* StrListIsValid */
#endif /* _DEBUG */
