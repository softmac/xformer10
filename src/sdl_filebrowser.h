#ifndef SDL_FILEBROWSER_H
#define SDL_FILEBROWSER_H
#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

SDL_Renderer *GetSDLRenderer(void);   /* defined in ddlib_sdl.c */
SDL_Window   *GetSDLWindow(void);     /* defined in ddlib_sdl.c */

/* Returns 1 and fills out[0..sz-1] with the selected path, or 0 on cancel.
   start_path: initial path hint (may be an image file or directory, or NULL). */
int SDL_FileBrowserRun(SDL_Renderer *ren, SDL_Window *win,
                       const char *start_path, char *out, int sz);

/* exts: comma-separated extensions to show (e.g. ".atr,.atx,.xfd"),
         or "" / NULL for all files.
   mode: 0=open file, 1=select folder, 2=save-as (text-input bar at bottom).
   Returns 1 and fills out on success, 0 on cancel. */
int SDL_FileBrowserRunEx(SDL_Renderer *ren, SDL_Window *win,
                         const char *start_path, char *out, int sz,
                         const char *exts, int mode);
#endif /* !_WIN32 */
#endif /* SDL_FILEBROWSER_H */
