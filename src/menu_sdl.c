/****************************************************************************

    MENU_SDL.C

    - SDL2 menu bar for xformer10 on Linux/Pi

    Copyright (C) 1991-2021 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

****************************************************************************/

#ifndef _WIN32

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "gemtypes.h"
#include "menu_sdl.h"
#include "ddlib_sdl.h"

extern void LinuxDoCommand(int idm);

#define FONT_PATH         "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#define FONT_SIZE         14
#define NUM_TOPS          5
#define DISK_MENU_IDX     3
#define NUM_SUBMENUS      2
#define MAX_SUB_ITEMS     8
#define IDM_SUB_D1        (-10)
#define IDM_SUB_D2        (-11)
#define MAX_ITEMS         16
#define MENU_ITEM_H       20
#define MENU_SEP_H        8
#define MENU_PAD          8
#define MENU_SHORTCUT_GAP 16
#define MENU_DROP_MIN_W   160
#define MENU_CHECK_AREA   16  /* reserved width for checkmark left of label */

static const char *gTopLabels[NUM_TOPS] = { "File", "VM", "Window", "Disk/Cartridge", "Help" };
static int         gTopX[NUM_TOPS];

static TTF_Font    *gFont         = NULL;
static SDL_Texture *gTopTex[NUM_TOPS];
static int          gTopW[NUM_TOPS];
static int          gTopH[NUM_TOPS];
static SDL_Texture *gCheckTex     = NULL;
static int          gCheckW       = 0;
static int          gCheckH       = 0;
static SDL_Texture *gArrowTex     = NULL;
static int          gArrowW       = 0;
static int          gArrowH       = 0;
static int          gMenuReady    = 0;
static int          gMenuOpen     = -1;
static int          gMenuHover    = -1;
static int          gSubmenuOpen  = -1;
static int          gSubmenuHover = -1;

typedef struct {
    const char  *label;
    const char  *shortcut;
    int          idm;       /* 0=separator, <0=special/submenu-parent, else IDM */
    int          needsVM;   /* gray when v.iVM < 0 */
    SDL_Texture *labelTex;
    int          labelW, labelH;
    SDL_Texture *shortTex;
    int          shortW, shortH;
} MenuItem;

static MenuItem gItems[NUM_TOPS][MAX_ITEMS];
static int      gItemCount[NUM_TOPS];
static int      gDropW[NUM_TOPS];

static MenuItem gSubItems[NUM_SUBMENUS][MAX_SUB_ITEMS];
static int      gSubItemCount[NUM_SUBMENUS];
static int      gSubDropW[NUM_SUBMENUS];

static const struct { const char *lbl; const char *sc; int idm; int needsVM; }
kDef[NUM_TOPS][MAX_ITEMS] = {
    /* File */
    {
        {"Open Folder...",             NULL, IDM_OPENFOLDER, 0},
        {"Add Atari 800",              NULL, IDM_ADDVM1,     0},
        {"Delete VM",                  NULL, IDM_DELVM,      1},
        {NULL,                         NULL, 0,              0},
        {"New Session",                NULL, IDM_NEW,        0},
        {"Load Session...",            NULL, IDM_LOAD,       0},
        {"Save Session As...",         NULL, IDM_SAVEAS,     0},
        {NULL,                         NULL, 0,              0},
        {"Restore Last Session",       NULL, IDM_AUTOLOAD,   0},
        {NULL,                         NULL, 0,              0},
        {"Next VM",                    NULL, IDM_NEXTVM,     1},
        {"Previous VM",                NULL, IDM_PREVVM,     1},
        {NULL,                         NULL, 0,              0},
        {"Save Settings",              NULL, IDM_SAVEINI,    0},
        {NULL,                         NULL, 0,              0},
        {"Exit",                       NULL, IDM_EXIT,       0},
    },
    /* VM */
    {
        {"Warm Start",     "F10",       IDM_WARMSTART,   1},
        {"Cold Start",     "Ctrl+F10",  IDM_COLDSTART,   1},
        {"Toggle BASIC",   "Shift+F10", IDM_TOGGLEBASIC, 1},
        {"Change Type",    "Alt+F10",   IDM_CHANGEVM,    1},
        {NULL,             NULL,        0,               0},
        {"Emulate PAL",    "Alt+F12",   IDM_NTSCPAL,     1},
        {"Switch Monitor", "Shift+F12", IDM_COLORMONO,   1},
        {NULL,             NULL,        0,               0},
        {"Paste as ASCII",   NULL,      IDM_PASTEASCII,  1},
        {"Paste as ATASCII", NULL,      IDM_PASTEATASCII,1},
        {NULL,               NULL,      0,               0},
        {"Time Travel",      NULL,      IDM_TIMETRAVEL,            1},
        {"Set Fix Point",    NULL,      IDM_TIMETRAVELFIXPOINT,    1},
        {"Use Fix Point",    NULL,      IDM_USETIMETRAVELFIXPOINT, 1},
    },
    /* Window */
    {
        {"Full Screen",                "Alt+Enter", IDM_FULLSCREEN,       0},
        {"Stretch Mode",               "F12",       IDM_STRETCH,          0},
        {"Tile All VMs",               "F5",        IDM_TILE,             0},
        {NULL,                         NULL,        0,                    0},
        {"Turbo Mode",                 "Alt+F1",    IDM_TURBO,            1},
        {NULL,                         NULL,        0,                    0},
        {"Auto-detect VM Type",        NULL,        IDM_AUTOKILL,         0},
        {NULL,                         NULL,        0,                    0},
        {"Disable L-CTRL as FIRE",     NULL,        IDM_LCTRLFIRE,        0},
        {NULL,                         NULL,        0,                    0},
        {"Mouse Wheel Sensitivity High", NULL,      IDM_WHEELSENS,        0},
        {NULL,                         NULL,        0,                    0},
        {"My Video Cards Sucks",       NULL,        IDM_MYVIDEOCARDSUCKS, 0},
        {NULL,                         NULL,        0,                    0},
        {"Enable Sound",               "Alt+S",     IDM_TOGGLESOUND,      0},
    },
    /* Disk/Cartridge */
    {
        {"D1:",              NULL, IDM_SUB_D1, 1},
        {"D2:",              NULL, IDM_SUB_D2, 1},
        {NULL,               NULL, 0,          0},
        {"Cartridge...",     NULL, IDM_CART,   1},
        {"Remove Cartridge", NULL, IDM_NOCART, 1},
    },
    /* Help */
    {
        {"Emulators.com Home Page", NULL, IDM_WEB_EMULATORS, 0},
        {"SoftMac/XFormer10 Repo",  NULL, IDM_WEB_FREE,      0},
        {"Online Documentation",    NULL, IDM_WEB_HELP,      0},
        {NULL,                      NULL, 0,                 0},
        {"About",                   NULL, IDM_ABOUT,         0},
    },
};

static const int kItemCount[NUM_TOPS] = {16, 14, 15, 5, 5};

static const struct { const char *lbl; const char *sc; int idm; int needsVM; }
kSubDef[NUM_SUBMENUS][MAX_SUB_ITEMS] = {
    /* [0] D1 */
    {
        {"Mount D1",          NULL, IDM_D1,         1},
        {"Unmount D1",        NULL, IDM_D1U,        1},
        {NULL,                NULL, 0,              0},
        {"Write Protect",     NULL, IDM_WP1,        1},
        {"Create Blank",      NULL, IDM_D1BLANKSD,  1},
        {"Extract DOS Files", NULL, IDM_IMPORTDOS1, 1},
    },
    /* [1] D2 */
    {
        {"Mount D2",          NULL, IDM_D2,         1},
        {"Unmount D2",        NULL, IDM_D2U,        1},
        {NULL,                NULL, 0,              0},
        {"Write Protect",     NULL, IDM_WP2,        1},
        {"Create Blank",      NULL, IDM_D2BLANKSD,  1},
        {"Extract DOS Files", NULL, IDM_IMPORTDOS2, 1},
    },
};
static const int kSubItemCount[NUM_SUBMENUS] = {6, 6};

/* Returns y offset of item j within dropdown m (relative to MENU_H) */
static int ItemYOffset(int m, int j)
{
    int yoff = 0;
    for (int k = 0; k < j; k++)
        yoff += (gItems[m][k].idm == 0) ? MENU_SEP_H : MENU_ITEM_H;
    return yoff;
}

/* Returns total pixel height of dropdown m */
static int DropHeight(int m)
{
    return ItemYOffset(m, gItemCount[m]);
}

/* Returns bounding rect of open dropdown for menu m */
static SDL_Rect DropRect(int m)
{
    return (SDL_Rect){ gTopX[m], MENU_H, gDropW[m], DropHeight(m) };
}

/* Returns total pixel height of submenu s */
static int SubDropHeight(int s)
{
    int h = 0;
    for (int j = 0; j < gSubItemCount[s]; j++)
        h += (gSubItems[s][j].idm == 0) ? MENU_SEP_H : MENU_ITEM_H;
    return h;
}

/* Returns bounding rect of submenu s flyout panel */
static SDL_Rect SubRect(int s)
{
    int parentRow = s;  /* D1 is row 0, D2 is row 1 in Disk/Cartridge dropdown */
    return (SDL_Rect){
        gTopX[DISK_MENU_IDX] + gDropW[DISK_MENU_IDX],
        MENU_H + ItemYOffset(DISK_MENU_IDX, parentRow),
        gSubDropW[s],
        SubDropHeight(s)
    };
}

/* Returns index of the top-level label hit at x, or -1 */
static int HitTopLabel(int x)
{
    for (int i = 0; i < NUM_TOPS; i++) {
        if (x >= gTopX[i] - 2 && x < gTopX[i] + gTopW[i] + 2)
            return i;
    }
    return -1;
}

/* Returns item index within dropdown m at pixel (x,y), or -1 */
static int HitItem(int m, int x, int y)
{
    SDL_Rect r = DropRect(m);
    if (x < r.x || x >= r.x + r.w || y < r.y || y >= r.y + r.h)
        return -1;
    int yrel = y - MENU_H;
    int yacc = 0;
    for (int j = 0; j < gItemCount[m]; j++) {
        int h = (gItems[m][j].idm == 0) ? MENU_SEP_H : MENU_ITEM_H;
        if (yrel < yacc + h)
            return j;
        yacc += h;
    }
    return -1;
}

/* Returns item index within submenu s at pixel (x,y), or -1 */
static int HitSubItem(int s, int x, int y)
{
    SDL_Rect sr = SubRect(s);
    if (x < sr.x || x >= sr.x + sr.w || y < sr.y || y >= sr.y + sr.h)
        return -1;
    int yrel = y - sr.y;
    int yacc = 0;
    for (int j = 0; j < gSubItemCount[s]; j++) {
        int h = (gSubItems[s][j].idm == 0) ? MENU_SEP_H : MENU_ITEM_H;
        if (yrel < yacc + h) return j;
        yacc += h;
    }
    return -1;
}

/* Returns 1 if submenu s item j should be grayed */
static int IsSubItemGrayed(int s, int j)
{
    MenuItem *it = &gSubItems[s][j];
    if (it->idm <= 0) return 1;
    if (it->needsVM && v.iVM < 0) return 1;
    if (v.iVM >= 0) {
        if (s == 0) {
            if (it->idm == IDM_D1U || it->idm == IDM_WP1 || it->idm == IDM_IMPORTDOS1)
                return !rgpvm[v.iVM]->rgvd[0].sz[0];
        } else {
            if (it->idm == IDM_D2U || it->idm == IDM_WP2 || it->idm == IDM_IMPORTDOS2)
                return !rgpvm[v.iVM]->rgvd[1].sz[0];
        }
    }
    return 0;
}

static void DispatchMenuCmd(int idm)
{
    switch (idm) {
    case IDM_EXIT:      vi.fQuitting = TRUE;      break;
    case IDM_WARMSTART: FWarmbootVM(v.iVM);        break;
    case IDM_COLDSTART: ColdStart(v.iVM);          break;
    case IDM_D1:
    case IDM_D2:
        LinuxDoCommand(idm);
        break;
    default:            LinuxDoCommand(idm);       break;
    }
}

void MenuInit(SDL_Renderer *ren)
{
    if (TTF_Init() < 0) {
        fprintf(stderr, "MenuInit: TTF_Init failed: %s\n", TTF_GetError());
        return;
    }
    gFont = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (!gFont) {
        fprintf(stderr, "MenuInit: TTF_OpenFont failed: %s\n", TTF_GetError());
        TTF_Quit();
        return;
    }

    /* Top-level label textures */
    SDL_Color fg = {230, 230, 230, 255};
    for (int i = 0; i < NUM_TOPS; i++) {
        SDL_Surface *surf = TTF_RenderUTF8_Blended(gFont, gTopLabels[i], fg);
        if (!surf) { gTopTex[i] = NULL; continue; }
        gTopTex[i] = SDL_CreateTextureFromSurface(ren, surf);
        gTopW[i]   = surf->w;
        gTopH[i]   = surf->h;
        SDL_FreeSurface(surf);
    }

    /* Compute gTopX dynamically from rendered label widths */
    {
        int xpos = 8;
        for (int i = 0; i < NUM_TOPS; i++) {
            gTopX[i] = xpos;
            xpos += gTopW[i] + 20;
        }
    }

    /* Checkmark texture (U+2713 ✓) */
    SDL_Color white = {255, 255, 255, 255};
    {
        SDL_Surface *surf = TTF_RenderUTF8_Blended(gFont, "\xe2\x9c\x93", white);
        if (surf) {
            gCheckTex = SDL_CreateTextureFromSurface(ren, surf);
            gCheckW   = surf->w;
            gCheckH   = surf->h;
            SDL_FreeSurface(surf);
        }
    }

    /* Arrow texture (U+25B6 ▶) for submenu parents */
    {
        SDL_Surface *surf = TTF_RenderUTF8_Blended(gFont, "\xe2\x96\xb6", white);
        if (surf) {
            gArrowTex = SDL_CreateTextureFromSurface(ren, surf);
            gArrowW   = surf->w;
            gArrowH   = surf->h;
            SDL_FreeSurface(surf);
        }
    }

    /* Item textures — white so SDL_SetTextureColorMod can dim them */
    for (int m = 0; m < NUM_TOPS; m++) {
        gItemCount[m] = kItemCount[m];
        int maxW = 0;
        for (int j = 0; j < gItemCount[m]; j++) {
            MenuItem *it = &gItems[m][j];
            it->label    = kDef[m][j].lbl;
            it->shortcut = kDef[m][j].sc;
            it->idm      = kDef[m][j].idm;
            it->needsVM  = kDef[m][j].needsVM;
            if (it->idm == 0 || !it->label) continue;

            SDL_Surface *surf = TTF_RenderUTF8_Blended(gFont, it->label, white);
            if (surf) {
                it->labelTex = SDL_CreateTextureFromSurface(ren, surf);
                it->labelW   = surf->w;
                it->labelH   = surf->h;
                SDL_FreeSurface(surf);
            }
            if (it->shortcut) {
                surf = TTF_RenderUTF8_Blended(gFont, it->shortcut, white);
                if (surf) {
                    it->shortTex = SDL_CreateTextureFromSurface(ren, surf);
                    it->shortW   = surf->w;
                    it->shortH   = surf->h;
                    SDL_FreeSurface(surf);
                }
            }
            int needed = MENU_PAD + MENU_CHECK_AREA + it->labelW
                       + (it->shortcut ? MENU_SHORTCUT_GAP + it->shortW : 0)
                       + MENU_PAD;
            /* submenu parent rows reserve space for the arrow */
            if (it->idm == IDM_SUB_D1 || it->idm == IDM_SUB_D2)
                needed += gArrowW + MENU_PAD;
            if (needed > maxW) maxW = needed;
        }
        gDropW[m] = (maxW > MENU_DROP_MIN_W) ? maxW : MENU_DROP_MIN_W;
    }

    /* Submenu item textures */
    for (int s = 0; s < NUM_SUBMENUS; s++) {
        gSubItemCount[s] = kSubItemCount[s];
        int maxW = 0;
        for (int j = 0; j < gSubItemCount[s]; j++) {
            MenuItem *it = &gSubItems[s][j];
            it->label    = kSubDef[s][j].lbl;
            it->shortcut = kSubDef[s][j].sc;
            it->idm      = kSubDef[s][j].idm;
            it->needsVM  = kSubDef[s][j].needsVM;
            if (it->idm == 0 || !it->label) continue;

            SDL_Surface *surf = TTF_RenderUTF8_Blended(gFont, it->label, white);
            if (surf) {
                it->labelTex = SDL_CreateTextureFromSurface(ren, surf);
                it->labelW   = surf->w;
                it->labelH   = surf->h;
                SDL_FreeSurface(surf);
            }
            int needed = MENU_PAD + MENU_CHECK_AREA + it->labelW + MENU_PAD;
            if (needed > maxW) maxW = needed;
        }
        gSubDropW[s] = (maxW > MENU_DROP_MIN_W) ? maxW : MENU_DROP_MIN_W;
    }

    gMenuReady = 1;
}

void MenuQuit(void)
{
    if (!gMenuReady) return;
    for (int i = 0; i < NUM_TOPS; i++) {
        if (gTopTex[i]) { SDL_DestroyTexture(gTopTex[i]); gTopTex[i] = NULL; }
    }
    for (int m = 0; m < NUM_TOPS; m++) {
        for (int j = 0; j < gItemCount[m]; j++) {
            if (gItems[m][j].labelTex) {
                SDL_DestroyTexture(gItems[m][j].labelTex);
                gItems[m][j].labelTex = NULL;
            }
            if (gItems[m][j].shortTex) {
                SDL_DestroyTexture(gItems[m][j].shortTex);
                gItems[m][j].shortTex = NULL;
            }
        }
    }
    for (int s = 0; s < NUM_SUBMENUS; s++) {
        for (int j = 0; j < gSubItemCount[s]; j++) {
            if (gSubItems[s][j].labelTex) {
                SDL_DestroyTexture(gSubItems[s][j].labelTex);
                gSubItems[s][j].labelTex = NULL;
            }
        }
    }
    if (gCheckTex) { SDL_DestroyTexture(gCheckTex); gCheckTex = NULL; }
    if (gArrowTex) { SDL_DestroyTexture(gArrowTex); gArrowTex = NULL; }
    if (gFont) { TTF_CloseFont(gFont); gFont = NULL; }
    TTF_Quit();
    gMenuReady = 0;
}

static void MenuSyncCursor(void)
{
    if (gMenuOpen >= 0)
        SDL_ShowCursor(SDL_ENABLE);
    else if (SDL_GetWindowFlags(GetSDLWindow()) & SDL_WINDOW_INPUT_FOCUS)
        SDL_ShowCursor(SDL_DISABLE);
}

void MenuRender(SDL_Renderer *ren)
{
    MenuSyncCursor();
    if (!gMenuReady) return;

    int winW, winH;
    SDL_GetRendererOutputSize(ren, &winW, &winH);
    (void)winH;

    /* bar background */
    SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
    SDL_Rect bar = {0, 0, winW, MENU_H};
    SDL_RenderFillRect(ren, &bar);

    /* top-level labels */
    for (int i = 0; i < NUM_TOPS; i++) {
        if (!gTopTex[i]) continue;
        int y = (MENU_H - gTopH[i]) / 2;
        SDL_Rect dst = { gTopX[i], y, gTopW[i], gTopH[i] };
        SDL_RenderCopy(ren, gTopTex[i], NULL, &dst);
    }

    /* separator line at bottom of bar */
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
    SDL_RenderDrawLine(ren, 0, MENU_H - 1, winW - 1, MENU_H - 1);

    if (gMenuOpen < 0) return;

    /* dropdown panel */
    int m = gMenuOpen;
    SDL_Rect dr = DropRect(m);

    SDL_SetRenderDrawColor(ren, 55, 55, 55, 255);
    SDL_RenderFillRect(ren, &dr);
    SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
    SDL_RenderDrawRect(ren, &dr);

    int yacc = MENU_H;
    for (int j = 0; j < gItemCount[m]; j++) {
        MenuItem *it = &gItems[m][j];
        int itemH = (it->idm == 0) ? MENU_SEP_H : MENU_ITEM_H;

        if (it->idm == 0) {
            SDL_SetRenderDrawColor(ren, 90, 90, 90, 255);
            SDL_RenderDrawLine(ren,
                dr.x + MENU_PAD,          yacc + MENU_SEP_H / 2,
                dr.x + dr.w - MENU_PAD,   yacc + MENU_SEP_H / 2);
        } else {
            int isSubParent = (it->idm == IDM_SUB_D1 || it->idm == IDM_SUB_D2);
            int isGrayed = (!isSubParent && it->idm < 0) || (it->needsVM && v.iVM < 0);
            if (it->idm == IDM_NOCART && v.iVM >= 0) isGrayed |= !rgpvm[v.iVM]->rgcart.fCartIn;
            if (it->idm == IDM_DELVM)  isGrayed |= (v.cVM <= 1);
            if (it->idm == IDM_NEXTVM) isGrayed |= (v.cVM <= 1);
            if (it->idm == IDM_PREVVM) isGrayed |= (v.cVM <= 1);
            int isChecked = 0;
            if (it->idm == IDM_TURBO)            isChecked = !fBrakes;
            if (it->idm == IDM_NTSCPAL)          isChecked = (v.iVM >= 0 && rgpvm[v.iVM]->fEmuPAL);
            if (it->idm == IDM_AUTOLOAD)         isChecked = v.fSaveOnExit;
            if (it->idm == IDM_FULLSCREEN)       isChecked = v.fFullScreen;
            if (it->idm == IDM_STRETCH)        { isChecked = v.fZoomColor; isGrayed |= v.fTiling; }
            if (it->idm == IDM_TILE)             isChecked = v.fTiling;
            if (it->idm == IDM_AUTOKILL)         isChecked = v.fAutoKill;
            if (it->idm == IDM_LCTRLFIRE)        isChecked = v.fDisableLCTRLFire;
            if (it->idm == IDM_WHEELSENS)        isChecked = v.fWheelSensitive;
            if (it->idm == IDM_MYVIDEOCARDSUCKS) isChecked = v.fMyVideoCardSucks;
            if (it->idm == IDM_TOGGLESOUND)      isChecked = !v.fSilentMode;
            if (it->idm == IDM_TIMETRAVEL && v.iVM >= 0)
                isGrayed |= !rgpvm[v.iVM]->fTimeTravelEnabled;
            if (it->idm == IDM_USETIMETRAVELFIXPOINT)
                isChecked = (v.iVM >= 0 && rgpvm[v.iVM]->fTimeTravelFixed);

            /* submenu parent rows stay highlighted while their flyout is open */
            int subIdx = (it->idm == IDM_SUB_D1) ? 0 : (it->idm == IDM_SUB_D2) ? 1 : -1;
            int isHovered = (gMenuHover == j) || (subIdx >= 0 && gSubmenuOpen == subIdx);
            if (isHovered && !isGrayed) {
                SDL_SetRenderDrawColor(ren, 80, 110, 160, 255);
                SDL_Rect hlr = { dr.x + 1, yacc + 1, dr.w - 2, itemH - 2 };
                SDL_RenderFillRect(ren, &hlr);
            }

            Uint8 c = isGrayed ? 100 : 230;

            if (isChecked && gCheckTex) {
                SDL_SetTextureColorMod(gCheckTex, c, c, c);
                int ty = yacc + (itemH - gCheckH) / 2;
                SDL_Rect dst = { dr.x + MENU_PAD, ty, gCheckW, gCheckH };
                SDL_RenderCopy(ren, gCheckTex, NULL, &dst);
            }
            if (it->labelTex) {
                SDL_SetTextureColorMod(it->labelTex, c, c, c);
                int ty = yacc + (itemH - it->labelH) / 2;
                SDL_Rect dst = { dr.x + MENU_PAD + MENU_CHECK_AREA, ty,
                                 it->labelW, it->labelH };
                SDL_RenderCopy(ren, it->labelTex, NULL, &dst);
            }
            if (isSubParent && gArrowTex) {
                SDL_SetTextureColorMod(gArrowTex, c, c, c);
                int ty = yacc + (itemH - gArrowH) / 2;
                SDL_Rect dst = { dr.x + dr.w - gArrowW - MENU_PAD, ty,
                                 gArrowW, gArrowH };
                SDL_RenderCopy(ren, gArrowTex, NULL, &dst);
            }
            if (it->shortTex) {
                SDL_SetTextureColorMod(it->shortTex, c, c, c);
                int ty = yacc + (itemH - it->shortH) / 2;
                SDL_Rect dst = { dr.x + dr.w - it->shortW - MENU_PAD, ty,
                                 it->shortW, it->shortH };
                SDL_RenderCopy(ren, it->shortTex, NULL, &dst);
            }
        }

        yacc += itemH;
    }

    /* submenu flyout panel */
    if (m == DISK_MENU_IDX && gSubmenuOpen >= 0) {
        int s = gSubmenuOpen;
        SDL_Rect sr = SubRect(s);

        SDL_SetRenderDrawColor(ren, 55, 55, 55, 255);
        SDL_RenderFillRect(ren, &sr);
        SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
        SDL_RenderDrawRect(ren, &sr);

        int sy = sr.y;
        for (int j = 0; j < gSubItemCount[s]; j++) {
            MenuItem *it = &gSubItems[s][j];
            int itemH = (it->idm == 0) ? MENU_SEP_H : MENU_ITEM_H;

            if (it->idm == 0) {
                SDL_SetRenderDrawColor(ren, 90, 90, 90, 255);
                SDL_RenderDrawLine(ren,
                    sr.x + MENU_PAD,        sy + MENU_SEP_H / 2,
                    sr.x + sr.w - MENU_PAD, sy + MENU_SEP_H / 2);
            } else {
                int isGrayed  = IsSubItemGrayed(s, j);
                int isChecked = 0;
                if (it->idm == IDM_WP1 && v.iVM >= 0 && rgpvm[v.iVM]->rgvd[0].sz[0])
                    isChecked = FWriteProtectDiskVM(v.iVM, 0, FALSE, FALSE);
                if (it->idm == IDM_WP2 && v.iVM >= 0 && rgpvm[v.iVM]->rgvd[1].sz[0])
                    isChecked = FWriteProtectDiskVM(v.iVM, 1, FALSE, FALSE);

                if (gSubmenuHover == j && !isGrayed) {
                    SDL_SetRenderDrawColor(ren, 80, 110, 160, 255);
                    SDL_Rect hlr = { sr.x + 1, sy + 1, sr.w - 2, itemH - 2 };
                    SDL_RenderFillRect(ren, &hlr);
                }

                Uint8 c = isGrayed ? 100 : 230;

                if (isChecked && gCheckTex) {
                    SDL_SetTextureColorMod(gCheckTex, c, c, c);
                    int ty = sy + (itemH - gCheckH) / 2;
                    SDL_Rect dst = { sr.x + MENU_PAD, ty, gCheckW, gCheckH };
                    SDL_RenderCopy(ren, gCheckTex, NULL, &dst);
                }
                if (it->labelTex) {
                    SDL_SetTextureColorMod(it->labelTex, c, c, c);
                    int ty = sy + (itemH - it->labelH) / 2;
                    SDL_Rect dst = { sr.x + MENU_PAD + MENU_CHECK_AREA, ty,
                                     it->labelW, it->labelH };
                    SDL_RenderCopy(ren, it->labelTex, NULL, &dst);
                }
            }
            sy += itemH;
        }
    }
}

int MenuHandleEvent(SDL_Event *e)
{
    if (!gMenuReady) return 0;

    switch (e->type) {
    case SDL_KEYDOWN:
        if (e->key.keysym.sym == SDLK_ESCAPE && gMenuOpen >= 0) {
            gMenuOpen     = -1;
            gMenuHover    = -1;
            gSubmenuOpen  = -1;
            gSubmenuHover = -1;
            return 1;
        }
        return 0;

    case SDL_MOUSEMOTION:
        if (gMenuOpen >= 0) {
            int x = e->motion.x, y = e->motion.y;
            /* if a submenu is open, check its area before processing the main dropdown */
            if (gMenuOpen == DISK_MENU_IDX && gSubmenuOpen >= 0) {
                SDL_Rect sr = SubRect(gSubmenuOpen);
                if (x >= sr.x && x < sr.x + sr.w &&
                    y >= sr.y && y < sr.y + sr.h) {
                    gSubmenuHover = HitSubItem(gSubmenuOpen, x, y);
                    /* keep gMenuHover on the parent row so it stays highlighted */
                    return 0;
                }
            }
            int j = HitItem(gMenuOpen, x, y);
            gMenuHover = j;
            if (gMenuOpen == DISK_MENU_IDX) {
                if (j >= 0 && gItems[DISK_MENU_IDX][j].idm == IDM_SUB_D1) {
                    gSubmenuOpen = 0; gSubmenuHover = -1;
                } else if (j >= 0 && gItems[DISK_MENU_IDX][j].idm == IDM_SUB_D2) {
                    gSubmenuOpen = 1; gSubmenuHover = -1;
                } else {
                    gSubmenuOpen = -1; gSubmenuHover = -1;
                }
            }
        }
        return 0;

    case SDL_MOUSEBUTTONDOWN:
        if (e->button.button != SDL_BUTTON_LEFT) return 0;
        {
            int x = e->button.x, y = e->button.y;

            if (y < MENU_H) {
                int hit = HitTopLabel(x);
                gSubmenuOpen  = -1;
                gSubmenuHover = -1;
                if (hit < 0) {
                    gMenuOpen  = -1;
                    gMenuHover = -1;
                    return 0;
                }
                gMenuOpen  = (gMenuOpen == hit) ? -1 : hit;
                gMenuHover = -1;
                return 1;
            }

            if (gMenuOpen >= 0) {
                /* check submenu flyout first */
                if (gMenuOpen == DISK_MENU_IDX && gSubmenuOpen >= 0) {
                    SDL_Rect sr = SubRect(gSubmenuOpen);
                    if (x >= sr.x && x < sr.x + sr.w &&
                        y >= sr.y && y < sr.y + sr.h) {
                        int j = HitSubItem(gSubmenuOpen, x, y);
                        int s = gSubmenuOpen;
                        gMenuOpen = -1; gMenuHover = -1;
                        gSubmenuOpen = -1; gSubmenuHover = -1;
                        if (j >= 0 && !IsSubItemGrayed(s, j))
                            DispatchMenuCmd(gSubItems[s][j].idm);
                        return 1;
                    }
                }

                SDL_Rect dr = DropRect(gMenuOpen);
                if (x >= dr.x && x < dr.x + dr.w &&
                    y >= dr.y && y < dr.y + dr.h) {
                    int j = HitItem(gMenuOpen, x, y);
                    int m = gMenuOpen;

                    /* clicking a submenu parent opens its flyout without closing the menu */
                    if (j >= 0 && (gItems[m][j].idm == IDM_SUB_D1 ||
                                   gItems[m][j].idm == IDM_SUB_D2)) {
                        gSubmenuOpen  = (gItems[m][j].idm == IDM_SUB_D1) ? 0 : 1;
                        gMenuHover    = j;
                        gSubmenuHover = -1;
                        return 1;
                    }

                    gMenuOpen = -1; gMenuHover = -1;
                    gSubmenuOpen = -1; gSubmenuHover = -1;
                    if (j >= 0) {
                        MenuItem *it = &gItems[m][j];
                        int grayed = (it->idm <= 0) || (it->needsVM && v.iVM < 0);
                        if (it->idm == IDM_NOCART && v.iVM >= 0) grayed |= !rgpvm[v.iVM]->rgcart.fCartIn;
                        if (it->idm == IDM_DELVM)  grayed |= (v.cVM <= 1);
                        if (it->idm == IDM_NEXTVM) grayed |= (v.cVM <= 1);
                        if (it->idm == IDM_PREVVM) grayed |= (v.cVM <= 1);
                        if (it->idm == IDM_STRETCH) grayed |= v.fTiling;
                        if (!grayed)
                            DispatchMenuCmd(it->idm);
                    }
                    return 1;
                }
                gMenuOpen = -1; gMenuHover = -1;
                gSubmenuOpen = -1; gSubmenuHover = -1;
                return 0;
            }
        }
        return 0;

    default:
        return 0;
    }
}

#endif /* !_WIN32 */
