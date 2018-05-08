
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

//
// Standard include files
//

#include "..\gemtypes.h"

#define H32BIT

#if defined(_M_IX86) || defined(_M_AMD64)

#pragma intrinsic(__stosb)
#pragma intrinsic(__movsb)

#define _fmemset(d,s,c) __stosb((unsigned char *)(d),(unsigned char)(s),(size_t)(c))
#define _fmemcpy(d,s,c) __movsb((unsigned char *)(d),(unsigned const char *)(s),(size_t)(c))

#define memset __stosb
#define memcpy __movsb

#else

#define _fmemset memset
#define _fmemcpy memcpy

#endif


//
// Memory model stuff
//

#define HUGE

//
// Debug functions
//
