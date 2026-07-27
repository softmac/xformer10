#ifndef MENU_SDL_H
#define MENU_SDL_H
#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MENU_H 22   /* pixel height of the menu bar strip */

void MenuInit(SDL_Renderer *ren);
void MenuQuit(void);
void MenuRender(SDL_Renderer *ren);
int  MenuHandleEvent(SDL_Event *e);  /* returns 1 if event consumed */
#endif /* !_WIN32 */
#endif /* MENU_SDL_H */
