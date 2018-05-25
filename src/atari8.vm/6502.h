
/***************************************************************************

    6502.H

    - Public API from the 6502 interpreter

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

//
// 6502 specific implementation of the CPU API
//

// !!! It shouldn't be ATARI specific, get rid of every reference to CANDY, keep it's own thread-safe state

// !!! don't call this directly
void __cdecl Go6502(const int);

__inline BOOL cpuExec(const int iVM)
{
    Go6502(iVM);
    return TRUE;
}

// !!! The monitor should use this
__inline BOOL cpuDisasm(const int iVM, char *pch, ADDR *pPC)
{
    // stub out for now

    iVM;  pch; pPC;

    return TRUE;
}

__inline BYTE cpuPeekB(const int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return rgbMem[addr];
}

extern BOOL ProcessScanLine(int);

__inline BOOL cpuPokeB(const int iVM, ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    // !!! controversial perf hit - we are writing into the line of screen RAM that we are current displaying
    // handle that with cycle accuracy instead of scan line accuracy or the wrong thing is drawn (Turmoil)
    // most display lists are in himem so do the >= check first, the test most likely to fail and not require additional tests
    if (addr >= wAddr && addr < (WORD)(wAddr + cbWidth) && rgbMem[addr] != b)
        ProcessScanLine(iVM);

    rgbMem[addr] = (BYTE) b;
    return TRUE;
}

__inline WORD cpuPeekW(const int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return cpuPeekB(iVM, addr) | (cpuPeekB(iVM, addr + 1) << 8);
}

__inline BOOL cpuPokeW(const int iVM, ADDR addr, WORD w)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr + 0] = (w & 255);
    rgbMem[addr + 1] = ((w >> 8) & 255);
    return TRUE;
}

__inline BOOL cpuInit(PFNB pvmPeekB, PFNL pvmPokeB)
{
    extern PFNB pfnPeekB;
    extern PFNL pfnPokeB;

    // global - each VM needs to set this every time a VM is Execute'd
    pfnPeekB = pvmPeekB;
    pfnPokeB = pvmPokeB;

    return TRUE;
}

__inline BOOL cpuReset(const int iVM)
{

    // clear all registers

    regA = 0;
    regX = 0;
    regY = 0;
    regP = 0xFF;

    // set initial SP = $FF

    regSP = 0x1FF;
    regPC = cpuPeekW(iVM, 0xFFFC);

    return TRUE;
}


