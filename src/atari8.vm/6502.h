
/***************************************************************************

    6502.H

    - Public API from the 6502 interpreter

    Copyright (C) 1986-2021 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part Xformer project. It is subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

    11/30/2008  darekm      open source release

***************************************************************************/

//
// 6502 specific implementation of the CPU API
//

// !!! It shouldn't be ATARI specific?, get rid of every reference to CANDY, keep it's own thread-safe state

// !!! don't call this directly
void __cdecl Go6502(void *candy);

__inline BOOL cpuExec(void *candy)
{
    Go6502(candy);
    return TRUE;
}

// !!! The monitor should use this
__inline BOOL cpuDisasm(void *candy, char *pch, ADDR *pPC)
{
    // stub out for now

    candy;  pch; pPC;

    return TRUE;
}

extern BOOL ProcessScanLine(void *);

// jump tables
typedef void(__fastcall * PFNOP)(void *candy);
PFNOP jump_tab[256];

typedef BYTE(__fastcall * PFNREAD)(void *pPrivate, ADDR addr);
typedef BOOL(__fastcall * PFNWRITE)(void *pPrivate, ADDR addr, BYTE b);

// call these when you want quick read/writes, with no special case registers or other side effects
// In other words, zero page or stack memory

BYTE __forceinline __fastcall cpuPeekB(void *candy, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return rgbMem[addr];
}

BOOL __forceinline __fastcall cpuPokeB(void *candy, ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr] = (BYTE) b;
    return TRUE;
}

__inline WORD cpuPeekW(void *candy, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return cpuPeekB(candy, addr) | (cpuPeekB(candy, addr + 1) << 8);
}

// not really used, if it ever is, make sure it doesn't need to do any special casing
__inline BOOL cpuPokeW(void *candy, ADDR addr, WORD w)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr + 0] = (w & 255);
    rgbMem[addr + 1] = ((w >> 8) & 255);
    return TRUE;
}

// !!! This can't be used in the jump tables version that uses many possible functions - another reason not to use them
__inline BOOL cpuInit(PFNREAD pvmPeekB, PFNWRITE pvmPokeB)
{
    extern PFNREAD pfnPeekB;
    extern PFNWRITE pfnPokeB;

    // !!! To work with more than just ATARI 800, each VM needs to set this every time its VM is Execute'd
    pfnPeekB = pvmPeekB;
    pfnPokeB = pvmPokeB;

    return TRUE;
}

__inline BOOL cpuReset(void *candy)
{

    // clear all registers

    regA = 0;
    regX = 0;
    regY = 0;
    regP = 0xFF;

    // set initial SP = $FF

    regSP = 0x1FF;
    regPC = cpuPeekW(candy, 0xFFFC);

    return TRUE;
}


