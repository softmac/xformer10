
/****************************************************************************

    DDLIB_SDL.C

    - SDL2 window + streaming texture replacing DirectDraw on Linux/Pi

    Copyright (C) 1991-2021 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

****************************************************************************/

#ifndef _WIN32

/* SDL2/SDL.h must come before gemtypes.h to avoid __inline redefinition conflict with arm_neon.h */
#include <stdio.h>
#include <SDL2/SDL.h>
#include "gemtypes.h"
#include "atari800.h"
#include "menu_sdl.h"

static SDL_Window   *gSDLWin;
static SDL_Renderer *gSDLRen;
static SDL_Texture  *gSDLTex;
static int gTexW, gTexH;

/* Per-tile textures for tiling mode — avoids single-texture update race */
#define MAX_TILE_TEX 64
static SDL_Texture *gTileTex[MAX_TILE_TEX];

/* Lospec GTIA palette — 8-bit R,G,B per entry, ordered by Atari color register value.
   Replaces the original hand-crafted 6-bit rgbRainbow for accurate NTSC colors.
   Source: lospec.com/palette-list/atari-8-bit-family-gtia (Retrospecs App reference) */
static const BYTE sAtariPal[256 * 3] = {
      0,  0,  0,  /* $00 hue 0 */
     17, 17, 17,
     34, 34, 34,
     51, 51, 51,
     68, 68, 68,
     85, 85, 85,
    102,102,102,
    119,119,119,
    136,136,136,
    153,153,153,
    170,170,170,
    187,187,187,
    204,204,204,
    221,221,221,
    238,238,238,
    255,255,255,
     25,  7,  0,  /* $10 hue 1 */
     42, 24,  0,
     59, 41,  0,
     76, 58,  0,
     93, 75,  0,
    110, 92,  0,
    127,109,  0,
    144,126,  9,
    161,143, 26,
    179,160, 43,
    195,177, 60,
    212,194, 77,
    229,211, 94,
    247,228,111,
    255,245,130,
    255,255,150,
     49,  0,  0,  /* $20 hue 2 */
     63,  0,  0,
     83, 23,  0,
    100, 40,  0,
    117, 57,  0,
    134, 74,  0,
    151, 91, 10,
    168,108, 27,
    185,125, 44,
    202,142, 61,
    219,159, 78,
    236,176, 95,
    253,193,112,
    255,210,133,
    255,227,156,
    255,244,178,
     66,  4,  4,  /* $30 hue 3 */
     79,  0,  0,
     96,  8,  0,
    113, 25,  0,
    130, 42, 13,
    147, 59, 30,
    164, 76, 47,
    181, 93, 64,
    198,110, 81,
    215,127, 98,
    232,144,115,
    249,161,131,
    255,178,152,
    255,195,174,
    255,212,196,
    255,229,218,
     65,  1,  3,  /* $40 hue 4 */
     80,  0, 15,
     97,  0, 27,
    114, 15, 43,
    131, 32, 60,
    148, 49, 77,
    165, 66, 94,
    182, 83,111,
    199,100,128,
    216,117,145,
    233,134,162,
    250,151,179,
    255,168,200,
    255,185,222,
    255,202,239,
    251,220,246,
     51,  0, 53,  /* $50 hue 5 */
     68,  0, 65,
     85,  0, 76,
    102, 12, 92,
    119, 29,109,
    136, 46,126,
    153, 63,143,
    170, 80,160,
    187, 97,177,
    204,114,194,
    221,131,211,
    238,148,228,
    255,165,228,
    255,182,233,
    255,199,238,
    255,216,243,
     29,  0, 92,  /* $60 hue 6 */
     46,  0,104,
     64,  0,116,
     81, 16,132,
     98, 33,149,
    115, 50,166,
    132, 67,183,
    149, 84,200,
    166,101,217,
    183,118,234,
    200,135,235,
    217,152,235,
    233,169,236,
    251,186,235,
    255,203,239,
    255,223,249,
      2,  0,113,  /* $70 hue 7 */
     19,  0,125,
     36, 11,140,
     53, 28,157,
     70, 45,174,
     87, 62,191,
    104, 79,208,
    121, 96,225,
    138,113,242,
    155,130,247,
    172,147,247,
    189,164,247,
    206,181,247,
    223,198,247,
    240,215,247,
    255,232,248,
      0,  0,104,  /* $80 hue 8 */
      0, 10,124,
      8, 27,144,
     25, 44,161,
     42, 61,178,
     59, 78,195,
     76, 95,212,
     93,112,229,
    110,129,246,
    127,146,255,
    144,163,255,
    161,180,255,
    178,197,255,
    195,214,255,
    212,231,255,
    229,248,255,
      0, 10, 77,  /* $90 hue 9 */
      0, 27, 99,
      0, 44,121,
      2, 61,143,
     19, 78,160,
     36, 95,177,
     53,112,194,
     70,129,211,
     87,146,228,
    104,163,245,
    121,180,255,
    138,197,255,
    155,214,255,
    172,231,255,
    189,248,255,
    206,255,255,
      0, 26, 38,  /* $A0 hue A */
      0, 43, 60,
      0, 60, 82,
      0, 77,104,
      6, 94,124,
     23,111,141,
     40,128,158,
     57,145,175,
     74,162,192,
     91,179,209,
    108,196,226,
    125,213,243,
    142,230,255,
    159,247,255,
    176,255,255,
    193,255,255,
      1, 37, 10,  /* $B0 hue B */
      2, 54, 16,
      0, 70, 34,
      0, 87, 56,
      5,104, 77,
     22,121, 94,
     39,138,111,
     56,155,128,
     73,172,145,
     90,189,162,
    107,206,179,
    124,223,196,
    141,240,213,
    158,255,229,
    175,255,241,
    192,255,253,
      4, 38, 13,  /* $C0 hue C */
      4, 56, 17,
      5, 71, 19,
      0, 90, 27,
     16,107, 27,
     33,124, 44,
     50,141, 61,
     67,158, 78,
     84,175, 95,
    101,192,112,
    118,209,129,
    135,226,146,
    152,243,163,
    169,255,179,
    186,255,191,
    203,255,203,
      0, 35, 10,  /* $D0 hue D */
      0, 53, 16,
      4, 70, 19,
     21, 86, 19,
     38,103, 19,
     55,120, 19,
     72,137, 20,
     89,154, 37,
    106,171, 54,
    123,188, 71,
    140,205, 88,
    157,222,105,
    174,239,122,
    191,255,139,
    208,255,151,
    225,255,163,
      0, 23,  7,  /* $E0 hue E */
     14, 40,  8,
     31, 57,  8,
     48, 74,  8,
     65, 91,  8,
     82,108,  8,
     99,125,  8,
    116,142, 13,
    133,159, 30,
    150,176, 47,
    167,193, 64,
    184,210, 81,
    201,227, 98,
    218,244,115,
    235,255,130,
    252,255,142,
     27,  7,  1,  /* $F0 hue F */
     44, 24,  1,
     60, 41,  0,
     77, 59,  0,
     95, 76,  0,
    112, 94,  0,
    129,111,  0,
    147,128,  9,
    164,146, 26,
    178,160, 43,
    199,180, 61,
    216,198, 78,
    234,215, 96,
    246,228,111,
    255,250,132,
    255,255,153,
};

void linux_set_window_title(const char *s)
{
    if (gSDLWin) SDL_SetWindowTitle(gSDLWin, s);
}

void linux_get_client_rect(RECT *r)
{
    int w = 0, h = 0;
    if (gSDLWin) SDL_GetWindowSize(gSDLWin, &w, &h);
    r->left = 0; r->top = 0;
    r->right = w; r->bottom = (h > MENU_H) ? h - MENU_H : 0;
}

BOOL InitDrawing(int dx, int dy, int bpp, HANDLE hwndApp, BOOL fReInit)
{
    (void)bpp; (void)hwndApp; (void)fReInit;
    gSDLWin = SDL_CreateWindow("Xformer 10",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               dx * 3, dy * 3 + MENU_H,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!gSDLWin) return FALSE;
    gSDLRen = SDL_CreateRenderer(gSDLWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gSDLRen) return FALSE;
    gSDLTex = SDL_CreateTexture(gSDLRen,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                dx, dy);
    if (!gSDLTex) return FALSE;
    gTexW = dx;
    gTexH = dy;
    MenuInit(gSDLRen);
    return TRUE;
}

BYTE *LockSurface(int *pStride)
{
    void *pixels;
    int pitch;
    if (SDL_LockTexture(gSDLTex, NULL, &pixels, &pitch) != 0)
        return NULL;
    *pStride = pitch / 4;
    return (BYTE *)pixels;
}

void UnlockSurface(void)
{
    SDL_UnlockTexture(gSDLTex);
}

void ClearSurface(void)
{
    if (gSDLRen)
        SDL_RenderClear(gSDLRen);
}

void UninitDrawing(BOOL fFinal)
{
    if (fFinal)
    {
        MenuQuit();
        for (int i = 0; i < MAX_TILE_TEX; i++) {
            if (gTileTex[i]) { SDL_DestroyTexture(gTileTex[i]); gTileTex[i] = NULL; }
        }
        if (gSDLTex) { SDL_DestroyTexture(gSDLTex);   gSDLTex = NULL; }
        if (gSDLRen) { SDL_DestroyRenderer(gSDLRen);  gSDLRen = NULL; }
        if (gSDLWin) { SDL_DestroyWindow(gSDLWin);    gSDLWin = NULL; }
    }
}

void RenderBitmap_SDL(void)
{
    if (gSDLTex == NULL || gSDLRen == NULL) return;

    static Uint32 argbBuf[X8 * Y8];

    SDL_RenderClear(gSDLRen);

    if (v.fTiling && cThreads > 0) {
        for (int t = 0; t < cThreads; t++) {
            if (t >= vvmhw.numTiles) break;
            BYTE *src = (BYTE *)vvmhw.pbmTile[t].pvBits;
            if (!src) continue;

            /* Ensure a per-tile texture exists */
            if (t < MAX_TILE_TEX && !gTileTex[t]) {
                gTileTex[t] = SDL_CreateTexture(gSDLRen, SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING, gTexW, gTexH);
            }
            SDL_Texture *tex = (t < MAX_TILE_TEX && gTileTex[t]) ? gTileTex[t] : gSDLTex;

            for (int i = 0; i < gTexW * gTexH; i++) {
                BYTE p = src[i];
                argbBuf[i] = (Uint32)0xFF000000
                    | (Uint32)sAtariPal[p * 3    ] << 16
                    | (Uint32)sAtariPal[p * 3 + 1] <<  8
                    | (Uint32)sAtariPal[p * 3 + 2];
            }
            SDL_UpdateTexture(tex, NULL, argbBuf, gTexW * 4);

            int slot = nFirstVisibleTile + t;
            int col  = (sTilesPerRow > 0) ? slot % sTilesPerRow : 0;
            int row  = (sTilesPerRow > 0) ? slot / sTilesPerRow : t;
            SDL_Rect dest = {col * gTexW,
                             MENU_H + v.sWheelOffset + row * gTexH,
                             gTexW, gTexH};
            SDL_RenderCopy(gSDLRen, tex, NULL, &dest);

            if (sVM >= 0 && slot == sVM) {
                SDL_SetRenderDrawColor(gSDLRen, 255, 255, 255, 255);
                SDL_RenderDrawRect(gSDLRen, &dest);
                SDL_SetRenderDrawColor(gSDLRen, 0, 0, 0, 255);
            }
        }
    } else {
        void *src = vvmhw.pbmTile[0].pvBits;

        if (src == NULL) goto done;

        for (int i = 0; i < gTexW * gTexH; i++) {
            BYTE p = ((BYTE *)src)[i];
            argbBuf[i] = (Uint32)0xFF000000
                | (Uint32)sAtariPal[p * 3    ] << 16
                | (Uint32)sAtariPal[p * 3 + 1] <<  8
                | (Uint32)sAtariPal[p * 3 + 2];
        }
        SDL_UpdateTexture(gSDLTex, NULL, argbBuf, gTexW * 4);

        SDL_Rect dest;
        if (v.fZoomColor || v.fFullScreen) {
            int winW, winH;
            SDL_GetWindowSize(gSDLWin, &winW, &winH);
            int availW = winW;
            int availH = winH - MENU_H;
            if (availH < 1) availH = 1;
            if (v.fZoomColor) {
                dest = (SDL_Rect){0, MENU_H, availW, availH};
            } else {
                int scaledW = availH * gTexW / gTexH;
                if (scaledW <= availW)
                    dest = (SDL_Rect){(availW - scaledW) / 2, MENU_H, scaledW, availH};
                else {
                    int scaledH = availW * gTexH / gTexW;
                    dest = (SDL_Rect){0, MENU_H + (availH - scaledH) / 2, availW, scaledH};
                }
            }
        } else {
            dest = (SDL_Rect){0, MENU_H, gTexW * 3, gTexH * 3};
        }
        SDL_RenderCopy(gSDLRen, gSDLTex, NULL, &dest);
    }
done:
    MenuRender(gSDLRen);
    SDL_RenderPresent(gSDLRen);
}

SDL_Renderer *GetSDLRenderer(void) { return gSDLRen; }
SDL_Window   *GetSDLWindow(void)   { return gSDLWin; }

#endif /* !_WIN32 */
