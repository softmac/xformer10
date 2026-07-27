#ifndef DDLIB_SDL_H
#define DDLIB_SDL_H
#ifndef _WIN32
#include <SDL2/SDL.h>
SDL_Renderer *GetSDLRenderer(void);
SDL_Window   *GetSDLWindow(void);
void linux_get_client_rect(RECT *r);
#endif /* !_WIN32 */
#endif /* DDLIB_SDL_H */
