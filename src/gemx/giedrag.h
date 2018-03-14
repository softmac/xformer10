/* giedrag.h */


#ifndef __GIEDRAG_H__
#define __GIEDRAG_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- drop target --- */
typedef DWORD HFILEDROPTGT;

#define WM_GIE_CANDROPFILES           (WM_USER + 201) /* wParam = HFILEDROPTGT, lParam = HSTRLIST (return TRUE if files are acceptable) */
#define WM_GIE_FILESDROPPED           (WM_USER + 202) /* wParam = HFILEDROPTGT, lParam = HSTRLIST */

HFILEDROPTGT RegisterFileDropTarget(HWND hwndProcessDrop, HWND hwndTarget);
void         UnregisterFileDropTarget(HFILEDROPTGT hfdt);


/* --- drop source --- */
#define WM_GIE_GETFILEGROUPDESCRIPTOR (WM_USER + 203) /* (return HGLOBAL containing FILEGROUPDESCRIPTOR) */
#define WM_GIE_GETFILECONTENTS        (WM_USER + 204) /* lParam = index (return HGLOBAL containing file contents) */

/* drop source functions */
BOOL DoDrag(HWND hwnd);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !__GIEDRAG_H__ */
