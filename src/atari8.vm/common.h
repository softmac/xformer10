
/***************************************************************************

    COMMON.H

    - This should be the very first file included!!!

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

//
// One of the following host flags must be defined
//
//    HWIN32 - hosted on 32-bit Win32  (Visual C 2.x)
//    HDOS32 - hosted on 32-bit DOSX32 (Watcom C/386)
//    HDOS16 - hosted on 16-bit MS-DOS (Visual C 1.x)
//

#ifdef WIN32
#ifndef HWIN32
#define HWIN32
#endif
#endif

//
// Basic types
//

#ifndef HWIN32
typedef                char CHAR;
typedef          short int  SWRD;
typedef          long  int  LONG;

typedef unsigned       char BYTE;
typedef unsigned short int  WORD;
typedef unsigned long  int  ULONG;

typedef                int  BOOL;

typedef unsigned short int  ADDR;

typedef void           (__cdecl *PFN) (int x, ...);
typedef ULONG          (__cdecl *PFNL)(int x, ...);

#define fTrue 1
#define fFalse 0

#define TRUE  1
#define FALSE 0

#define swab(x) ((WORD)((((WORD)(x & 0xFF00)) >> 8) | (((WORD)(x & 0x00FF)) << 8)))
#endif


//
// Standard include files
//

#ifdef HWIN32
#include "..\gemtypes.h"
#else
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <bios.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <conio.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include "vmdos.h"
#endif


#ifdef HDOS16
#define HDOS16ORDOS32
#else
#ifdef HDOS32
#define HDOS16ORDOS32
#define H32BIT
#else
#ifdef HWIN32
#define H32BIT
#else
#error
#endif // HWIN32
#endif // HDOS32
#endif // HDOS16


#ifdef H32BIT
#define _fmemset memset
#define _fmemcpy memcpy
#endif


//
// Memory model stuff
//

#ifdef HDOS16
#define HUGE __huge
#define FAR  __far
#define NEAR __near
#else
#ifdef HDOS32
#define HUGE
#define FAR
#define NEAR
#define __inline
#else
#ifdef HWIN32
#define HUGE
#else
#error
#endif // HWIN32
#endif // HDOS32
#endif // HDOS16


//
// Debug functions
//

#ifndef HWIN32
#ifndef NDEBUG
#define Assert(f) _assert((f), __FILE__, __LINE__)

__inline void _assert(int f, char *file, int line)
{
    char sz[99];

    if (!f)
        {
        sprintf(sz, "%s: line #%d", file, line);
#ifdef HWIN32
        MessageBox(NULL, sz, "Assert failed", MB_APPLMODAL | MB_OK);
#else
        printf(sz);
        gets(sz);
#endif
        }
}

#define DebugStr printf

#else  // NDEBUG

#define DebugStr
#define Assert(f) (0)

#endif // NDEBUG
#endif // HWIN32
