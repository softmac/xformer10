/* Precompiled.h */

#pragma once

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP

// #undef  _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
// #define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

// #define WindowsSDKDesktopARM64Support 1

#ifdef __MINGW32__
#define __inline inline __attribute__((always_inline))
#endif

#define STRICT
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOICONS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NONLS
#define NOMETAFILE
#define NOSCROLL
#define NOSERVICE
#define NOTEXTMETRIC
#define NOWH
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "scsidefs.h"
#include "wnaspi32.h"
#include "blockdev.h"


// 0 = use malloc/realloc/free, 1 = use Heap*

#define  USEHEAP 1

// turn on debug tracing code

#define  TRACEDISK 0


//
// Basic Types
//

#define fTrue 1
#define fFalse 0

#define swab(x) ((WORD)((((WORD)(x & 0xFF00)) >> 8) | (((WORD)(x & 0x00FF)) << 8)))

//
// Debug functions
//

#ifndef NDEBUG
#define Assert(f) _blk_assert((f), __FILE__, __LINE__)

static __inline void _blk_assert(int f, char *file, int line)
{
    char sz[99];

    if (!f)
        {
        int ret;

        wsprintf(sz, "%s: line #%d", file, line);
        ret = MessageBox(GetFocus(), sz, "Assert failed", MB_APPLMODAL | MB_ABORTRETRYIGNORE);

        if (ret == 3)
            {
            exit(0);
            }
        else if (ret == 4)
            {
#if (_MSC_VER >= 1300) || defined(__MINGW32__)
            __debugbreak();
#else
            __asm { int 3 };
#endif
            }
        }
}

#define DebugStr printf

#else  // NDEBUG

#define DebugStr 0 &&
#define Assert(f) (0)

#endif // NDEBUG


unsigned long __inline SwapW(unsigned long w)
{
    return (((w) & 0xFF00) >> 8) | (((w) & 0x00FF) << 8);
}

unsigned long __inline SwapL(unsigned long l)
{
    return SwapW(((l) & 0xFFFF0000) >> 16) | (SwapW((l) & 0x0000FFFF) << 16);
}

