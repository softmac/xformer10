
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

// !!! It shouldn't be ATARI specific?, get rid of every reference to CANDY, keep it's own thread-safe state

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

BYTE __forceinline __fastcall cpuPeekB(const int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    if (addr == 0xd40a)
        addr = addr;

    return rgbMem[addr];
}

extern BOOL ProcessScanLine(int);
extern void __forceinline __fastcall PackP(const int);
extern void __forceinline __fastcall UnpackP(const int);

// jump tables
typedef void(__fastcall * PFNOP)(const int iVM);
PFNOP jump_tab[256];

typedef BYTE(__fastcall * PFNREAD)(const int iVM, ADDR addr);
PFNREAD read_tab[65536];

__inline BOOL cpuPokeB(const int iVM, ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    // !!! controversial perf hit - we are writing into the line of screen RAM that we are current displaying
    // handle that with cycle accuracy instead of scan line accuracy or the wrong thing is drawn (Turmoil)
    // most display lists are in himem so do the >= check first, the test most likely to fail and not require additional tests
    if (addr >= wAddr && addr < (WORD)(wAddr + cbWidth) && rgbMem[addr] != b)
    {
        PackP(iVM);
        ProcessScanLine(iVM);
        UnpackP(iVM);
    }

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

__inline BOOL cpuInit(PFNPEEK pvmPeekB, PFNL pvmPokeB)
{
    extern PFNPEEK pfnPeekB;
    extern PFNL pfnPokeB;

    // !!! To work with more than just ATARI 800, each VM needs to set this every time its VM is Execute'd
    // but it can't do that now because initializing this table is slow
    pfnPeekB = pvmPeekB;
    pfnPokeB = pvmPokeB;

    for (int i = 0; i < 65536; i++)
    {
        if (i >= 0x8ff6 && i <= 0x8ff9)
            read_tab[i] = pfnPeekB;
        else if (i >= 0x9ff6 && i <= 0x9ff9)
            read_tab[i] = pfnPeekB;
        else if ((i & 0xff00) == 0xd000 && !(i & 0x0010))   // D000-D00F and its shadows D020-D02F, etc (NOT D010)
            read_tab[i] = pfnPeekB;
        else if ((i & 0xff00) == 0xd200 && ((i & 0x000f) == 0x0a || (i & 0x000f) == 0x0d))   // D20A/D and its shadows D21A/D, etc
            read_tab[i] = pfnPeekB;
        else if ((i & 0xff00) == 0xd400 && (i & 0x000f) == 0x0b)   // D40B its shadows D41B etc.
            read_tab[i] = pfnPeekB;
        else if ((i & 0xff00) == 0xd500)    // D5xx cartridge bank select
            read_tab[i] = pfnPeekB;
        else
            read_tab[i] = cpuPeekB;
    }

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


