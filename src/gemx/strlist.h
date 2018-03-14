/* strlist.h */


#ifndef __STRLIST_H__
#define __STRLIST_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef DWORD HSTRLIST;

HSTRLIST    StrListCreate();
void        StrListDestroy(HSTRLIST hsl);

int         StrListGetCount(HSTRLIST hsl);
const char* StrListGet(HSTRLIST hsl, int i);
BOOL        StrListAdd(HSTRLIST hsl, const char *sz);
void        StrListRemove(HSTRLIST hsl, int i);
void        StrListFreeAll(HSTRLIST hsl);

#ifdef _DEBUG
BOOL        StrListIsValid(HSTRLIST hsl);
#endif /* _DEBUG */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !__STRLIST_H__ */
