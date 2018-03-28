
/***************************************************************************

    GEMUL8R.H

    - build flags for all flavours of Gemulator / Xformer / SoftMac

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008 darekm       Gemulator 9.0 release

***************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

// uncomment to undefine NDEBUG, which will turn on a lot of debug spew
// #undef NDEBUG

//
// There are the builds flags which determines the version of the product
// that is being built (Xformer, Gemulator, SoftMac, etc.)
// and what kind of release if it (demo, debug, beta, release, etc.)
//
// Each flag should either be defined or not, and when defined it
// should be defined to some non-numeric value. This weeds out
// accidental code that uses #if instead of #ifdef!
//
// The build flags that we care about:
//
// XFORMER  build with Atari 8-bit emulation support
// ATARIST  build with Atari ST emulation support
// SOFTMAC  build with Macintosh Plus support
// SOFTMAC2 build with Macintosh Plus / II / Quadra support
// POWERMAC build with PowerPC enabled Macintosh support
// WINXP    build XP/Vista/DirectX compatible version, otherwise Win9x/2000


#define XFORMER

#if defined(_M_AMD64) || defined(_M_ARM)
#define ATARIST_OFF
#else
#define ATARIST
#endif

//
// Force Atari 800 mode only
//

#undef ATARIST
#define ATARIST_OFF

#define SOFTMAC_OFF

#define SOFTMAC2_OFF

#define POWERMAC_OFF

#define WINXP


// POWERMAC build flag implies SOFTMAC2 build flag

#if defined(POWERMAC) && !defined(SOFTMAC2)
#define SOFTMAC2
#endif

// SOFTMAC2 build flag implies SOFTMAC build flag

#if defined(SOFTMAC2) && !defined(SOFTMAC)
#define SOFTMAC
#endif


// Sanity check

#if !defined(XFORMER) && !defined(ATARIST) && !defined(SOFTMAC) && !defined(SOFTMAC2)
#error Must define at least one of XFORMER ATARIST SOFTMAC SOFTMAC2
#endif

