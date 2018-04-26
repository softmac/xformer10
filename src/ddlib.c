
/****************************************************************************

    DDLIB.C

    - DirectX helpers

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    09/07/2001  darekm      update for Windows XP

****************************************************************************/

#if 1
#include "gemtypes.h" // main include file
#else
#include <windows.h>
#include <ddraw.h>
#endif


/**************************************************************************
  DirectDraw Globals
 **************************************************************************/

IDirectDraw            *dd;     // The entire dd library
IDirectDrawSurface     *FrontBuffer;
IDirectDrawPalette     *Palette;
DDSURFACEDESC ddsd;             // The DirectDraw surface description

/**************************************************************************
  BMSDK Globals
 **************************************************************************/
int     fDDEnabled;             // Are we enabled?  This is completely handled by me.
int dwcLock;                    // A lock counter, so lock and unlock can be called enclusive

/**************************************************************************
    The Palette
 **************************************************************************/
PALETTEENTRY ape[256]; // The actual Windows palette structure
BYTE const abRainbow[3][256]= // The Atari800 Rainbow Palette NOTE: MAX=64
{
        0,      0,      0,              8,      8,      8,              16,     16,     16,             24,     24,     24,             31,     31,     31,
        36,     36,     36,             40,     40,     40,             44,     44,     44,             47,     47,     47,             50,     50,     50,
        53,     53,     53,             56,     56,     56,             58,     58,     58,             60,     60,     60,             62,     62,     62,
        63,     63,     63,             6,      5,      0,              12,     10,     0,              24,     15,     1,              30,     20,     2,
        35,     27,     3,              39,     33,     6,              44,     42,     12,             47,     44,     17,             49,     48,     20,
        52,     51,     23,             54,     53,     26,             55,     55,     29,             56,     56,     33,             57,     57,     36,
        58,     58,     39,             59,     59,     41,             9,      5,      0,              14,     9,      0,              20,     13,     0,
        28,     20,     0,              36,     28,     1,              43,     33,     1,              47,     39,     10,             49,     43,     17,
        51,     46,     24,             53,     47,     26,             55,     49,     28,             57,     50,     30,             59,     51,     32,
        60,     53,     36,             61,     55,     39,             62,     56,     40,             11,     3,      1,              18,     5,      2,
        27,     7,      4,              36,     11,     8,              44,     20,     13,             46,     24,     16,             49,     28,     21,
        51,     30,     25,             53,     35,     30,             54,     38,     34,             55,     42,     37,             56,     43,     38,
        57,     44,     39,             57,     46,     40,             58,     48,     42,             59,     49,     44,             11,     1,      3,
        22,     6,      9,              37,     10,     17,             42,     15,     22,             45,     21,     28,             48,     24,     30,
        50,     26,     32,             52,     28,     34,             53,     30,     36,             54,     33,     38,             55,     35,     40,
        56,     37,     42,             57,     39,     44,             58,     41,     45,             59,     42,     46,             60,     43,     47,
        12,     0,      11,             20,     2,      18,             28,     4,      26,             39,     8,      37,             48,     18,     49,
        53,     24,     53,             55,     29,     55,             56,     32,     56,             57,     35,     57,             58,     37,     58,
        59,     39,     59,             59,     41,     59,             59,     42,     59,             59,     43,     59,             59,     44,     59,
        60,     45,     60,             5,      1,      16,             10,     2,      32,             22,     10,     46,             27,     15,     49,
        32,     21,     51,             35,     25,     52,             38,     28,     53,             40,     32,     54,             42,     35,     55,
        44,     37,     56,             46,     38,     57,             47,     40,     57,             48,     41,     58,             49,     43,     58,
        50,     44,     59,             51,     45,     59,             0,      0,      13,             4,      4,      26,             10,     10,     46,
        18,     18,     49,             24,     24,     53,             27,     27,     54,             30,     30,     55,             33,     33,     56,
        36,     36,     57,             39,     39,     57,             41,     41,     58,             43,     43,     58,             44,     44,     59,
        46,     46,     60,             48,     48,     61,             49,     49,     62,             1,      7,      18,             2,      13,     30,
        3,      19,     42,             4,      24,     42,             9,      28,     45,             14,     32,     48,             17,     35,     51,
        20,     37,     53,             24,     39,     55,             28,     41,     56,             31,     44,     57,             34,     46,     57,
        37,     47,     58,             39,     48,     58,             41,     49,     59,             42,     50,     60,             1,      4,      12,
        2,      6,      22,             3,      10,     32,             5,      15,     36,             8,      20,     38,             15,     25,     44,
        21,     30,     47,             24,     34,     49,             27,     38,     52,             29,     42,     54,             31,     44,     55,
        33,     46,     56,             36,     47,     57,             38,     49,     58,             40,     50,     59,             42,     51,     60,
        0,      9,      7,              1,      18,     14,             2,      26,     20,             3,      35,     27,             4,      42,     33,
        6,      47,     38,             14,     51,     44,             18,     53,     46,             22,     55,     49,             25,     56,     51,
        28,     57,     52,             32,     58,     53,             36,     59,     55,             40,     60,     56,             44,     61,     57,
        45,     62,     58,             0,      10,     1,              0,      16,     3,              1,      22,     5,              5,      33,     7,
        9,      44,     16,             14,     48,     21,             19,     51,     24,             22,     52,     28,             24,     53,     31,
        30,     55,     35,             36,     57,     38,             39,     58,     41,             41,     59,     44,             43,     59,     47,
        46,     59,     49,             47,     60,     50,             3,      10,     0,              6,      20,     0,              9,      30,     1,
        14,     37,     4,              18,     44,     7,              22,     46,     12,             26,     48,     17,             29,     50,     22,
        33,     52,     26,             36,     54,     28,             38,     55,     30,             40,     56,     33,             42,     57,     36,
        45,     58,     39,             48,     59,     42,             49,     60,     43,             5,      9,      0,              11,     22,     0,
        17,     35,     1,              23,     42,     2,              29,     48,     8,              34,     50,     12,             38,     51,     17,
        40,     52,     21,             42,     53,     24,             44,     54,     27,             46,     55,     29,             47,     56,     31,
        48,     57,     34,             50,     58,     37,             52,     59,     40,             53,     60,     42,             8,      7,      0,
        19,     16,     0,              28,     24,     4,              33,     31,     6,              48,     38,     8,              52,     44,     12,
        55,     50,     15,             57,     52,     19,             58,     54,     22,             58,     56,     24,             59,     57,     26,
        59,     58,     29,             60,     58,     33,             61,     59,     35,             61,     59,     36,             62,     60,     38,
        8,      5,      0,              13,     9,      0,              22,     14,     1,              32,     21,     3,              42,     29,     5,
        45,     33,     7,              48,     36,     12,             50,     39,     18,             53,     42,     24,             54,     45,     27,
        55,     46,     30,             56,     47,     33,             57,     49,     36,             58,     50,     38,             58,     53,     39,
        59,     54,     40,
};

#ifndef NDEBUG
int CheckDDERR(HRESULT hRet)
{
    switch(hRet)
    {
    case DDERR_EXCEPTION: return hRet;
    case DDERR_GENERIC: return hRet;
    case DDERR_INVALIDOBJECT: return hRet;
    case DDERR_INVALIDPARAMS : return hRet;
    case DDERR_INVALIDRECT  : return hRet;
    case DDERR_NOBLTHW : return hRet;
    case DDERR_SURFACEBUSY:         return hRet;
    case DDERR_SURFACELOST : return hRet;
    case DDERR_UNSUPPORTED: return hRet;
    case DDERR_OUTOFMEMORY: return hRet;
    case DDERR_WASSTILLDRAWING: return hRet;
    case DDERR_NOTLOCKED:   return hRet;
    case DD_OK: break;
    default: return hRet;
    }
}
#else
#define CheckDDERR(hRet) (hRet)
#endif

HRESULT Restore(IDirectDrawSurface *lpdds)
{
    return lpdds->lpVtbl->Restore(lpdds);
}

typedef HRESULT (*DDProc)(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);

#ifdef WINXP
#define CreateProc DirectDrawCreate
#if !defined(_M_ARM)
#pragma comment(linker, "/defaultlib:ddraw")
#endif
#else
DDProc CreateProc;
#endif

BOOL FInitDirectDraw()
{
#ifdef WINXP
#else
    HMODULE HDDraw;

    if (CreateProc == NULL)
        {
        // Run-Time Load the ddraw library
        if((HDDraw=LoadLibrary("ddraw.dll"))==NULL ||
            (CreateProc=(DDProc)GetProcAddress(HDDraw, "DirectDrawCreate"))==NULL)
            return FALSE;
        }
#endif

    return TRUE;
}

BOOL InitDrawing(int *pdx,int *pdy, int *pbpp, HANDLE hwndApp,BOOL fReInit)
{
    fReInit;

//    DDSCAPS caps;
    HRESULT err;
    HDC hdc;

#ifdef WINXP
#else
    if (CreateProc == NULL)
        {
        if (!FInitDirectDraw())
            return FALSE;
        }
#endif

    if (dd == NULL)
        {
#if !defined(_M_ARM)
        if (FAILED(CreateProc(NULL, &dd, NULL)) ||      // Inialize Direct Draw
            FAILED(dd->lpVtbl->SetCooperativeLevel(dd, hwndApp,     // Setup the cooperation with Windows
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX)))
#endif
            return FALSE;
        }

    // Now we can set the resolution

    while (dd->lpVtbl->SetDisplayMode(dd, *pdx, *pdy, *pbpp) != DD_OK)
        {
        if (*pdy==200)
            {
            *pdy=240;
            }
        else if (*pdy==240)
            {
            *pdx=512;
            *pdy=384;
            }
        else if (*pdy==384)
            {
            *pdx=640;
            *pdy=400;
            }
        else if (*pdy==400)
            {
            *pdx=640;
            *pdy=480;
            }
        else
            return FALSE;
        }

    // get rid of any previous surfaces.
    if (FrontBuffer) FrontBuffer->lpVtbl->Release(FrontBuffer),    FrontBuffer = NULL;
    if (Palette)     Palette->lpVtbl->Release(Palette),        Palette = NULL;

    //
    // Create surfaces
    //
    // what we want is a double buffered surface in video memory
    // so we try to create this first.
    //
    // if we cant get a double buffered surface, we try for a double
    // buffered surface not being specific about video memory, we will
    // get back a main-memory surface, that work use HW page flipping
    // but at least we run.
    //
    // NOTE you need to recreate the surfaces for a new display mode
    // they wont work when/if the mode is changed.
    //
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
              DDSCAPS_VIDEOMEMORY;

    // try to get a triple buffered video memory surface.
    err = dd->lpVtbl->CreateSurface(dd, &ddsd, &FrontBuffer, NULL);

    if (err != DD_OK)
    {
    // try to just get video memory surface.
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    err = dd->lpVtbl->CreateSurface(dd, &ddsd, &FrontBuffer, NULL);
    }

    if (err != DD_OK)
    {
    // try to just get video memory surface.
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    err = dd->lpVtbl->CreateSurface(dd, &ddsd, &FrontBuffer, NULL);
    }

    if (err != DD_OK)
    return FALSE;

    // create a palette if we are in a paletized display mode.
    //
    // NOTE because we want to be able to show dialog boxs and
    // use our menu, we leave the windows reserved colors as is
    // so things dont look ugly.
    //
    // palette is setup like so:
    //
    // All 256 grays

    hdc = GetDC(NULL);
    //if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        int i;
        // make a red, grn, and blu wash for our cube.
        for (i=0; i<256; i++)
        {
            ape[i].peRed = abRainbow[0][i]*4;
            ape[i].peGreen = abRainbow[1][i]*4;
            ape[i].peBlue = abRainbow[2][i]*4;
        }
        if (!FAILED(dd->lpVtbl->CreatePalette(dd, DDPCAPS_8BIT, ape, &Palette, NULL)))
            FrontBuffer->lpVtbl->SetPalette(FrontBuffer, Palette);
    }
    SetCursor(NULL);                // Hide the mouse
    ReleaseDC(NULL, hdc);

    fDDEnabled = TRUE;
    ClearSurface();

    return TRUE;
}

void UninitDrawing(BOOL fFinal)
{
    if (FrontBuffer)
        {
        FrontBuffer->lpVtbl->Release(FrontBuffer);
        FrontBuffer = NULL;
        }

    if (Palette)
        {
        Palette->lpVtbl->Release(Palette);
        Palette = NULL;
        }

    if (fFinal && dd)
        {
        dd->lpVtbl->Release(dd);
        dd = NULL;
        }
}

void ClearSurface()
{
    BYTE *lpbSurface;
    int i;
    int j;

    if(!fDDEnabled)
        return;

    if (lpbSurface = LockSurface(&i))
        {
        for (j = 0; j < (int)ddsd.dwHeight; j++)
            {
            memset(lpbSurface, 0, ddsd.dwWidth);
            lpbSurface += ddsd.lPitch;
            }
        UnlockSurface();
        }
}


BYTE *LockSurface(int *pi)
{
    if(fDDEnabled && dwcLock++==0)
        if(CheckDDERR(FrontBuffer->lpVtbl->Lock(FrontBuffer, NULL, &ddsd, DDLOCK_WAIT, NULL))
            == DDERR_SURFACELOST)
        {
            Restore(FrontBuffer);
            dwcLock=0;
            return NULL;
            return LockSurface(pi);
        }
    *pi=ddsd.lPitch;
    return (BYTE*)ddsd.lpSurface;
}

void UnlockSurface()
{
    if(fDDEnabled && --dwcLock==0)
        if(CheckDDERR(FrontBuffer->lpVtbl->Unlock(FrontBuffer, NULL))
            == DDERR_SURFACELOST)
        {
            Restore(FrontBuffer);
            dwcLock=1;
            UnlockSurface();
        }
}

BOOL FCyclePalette(BOOL fForward)
{
    PALETTEENTRY peT;
    LPDIRECTDRAWPALETTE lpDDPalette;

    if( CheckDDERR(FrontBuffer->lpVtbl->GetPalette(FrontBuffer, &lpDDPalette))
        == DDERR_SURFACELOST)
    {
        Restore(FrontBuffer);
        return FCyclePalette(fForward);
    }

    // Cycle the palette
    peT=ape[!fForward*255];
    memcpy(ape+!fForward,ape+fForward,sizeof(PALETTEENTRY)*255);
    ape[fForward*255]=peT;
    CheckDDERR(lpDDPalette->lpVtbl->SetEntries(lpDDPalette, 0, 0, 256, ape));

    return TRUE;
}

BOOL FChangePaletteEntries(BYTE iPalette, int count, RGBQUAD *ppq)
{
    LPDIRECTDRAWPALETTE lpDDPalette;

    if(CheckDDERR(FrontBuffer->lpVtbl->GetPalette(FrontBuffer, &lpDDPalette))
        == DDERR_SURFACELOST)
    {
        Restore(FrontBuffer);
        return FChangePaletteEntries(iPalette, count, ppq);
    }

    while (count--)
        {
        ape[iPalette].peRed = ppq->rgbRed;
        ape[iPalette].peGreen = ppq->rgbGreen;
        ape[iPalette].peBlue = ppq->rgbBlue;
        iPalette++;
        ppq++;
        }
    CheckDDERR(lpDDPalette->lpVtbl->SetEntries(lpDDPalette, 0, 0, 256, ape));

    return TRUE;
}

