
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

extern BOOL ProcessScanLine(int);

// jump tables
typedef void(__fastcall * PFNOP)(const int iVM);
PFNOP jump_tab[256];

typedef BYTE(__fastcall * PFNREAD)(const int iVM, ADDR addr);
typedef BOOL(__fastcall * PFNWRITE)(const int iVM, ADDR addr, BYTE b);

PFNREAD read_tab[65536];
PFNWRITE write_tab[256];

// call these when you want quick read/writes, with no special case registers or other side effects
// In other words, zero page or stack memory

BYTE __forceinline __fastcall cpuPeekB(const int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return rgbMem[addr];
}

BOOL __forceinline __fastcall cpuPokeB(const int iVM, ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr] = (BYTE) b;
    return TRUE;
}

__inline WORD cpuPeekW(const int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return cpuPeekB(iVM, addr) | (cpuPeekB(iVM, addr + 1) << 8);
}

// not really used, if it ever is, make sure it doesn't need to do any special casing
__inline BOOL cpuPokeW(const int iVM, ADDR addr, WORD w)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr + 0] = (w & 255);
    rgbMem[addr + 1] = ((w >> 8) & 255);
    return TRUE;
}

__inline BOOL cpuInit(PFNREAD pvmPeekB, PFNWRITE pvmPokeB)
{
    extern PFNREAD pfnPeekB;
    extern PFNWRITE pfnPokeB;

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
        else if (i >= 0xd000 && i < 0xd600)
            read_tab[i] = pfnPeekB;
        else
            read_tab[i] = cpuPeekB;

#if 0 // these are the registers I special case, but I need to use the pfnPeekB function for EVERY hw reg to shadow properly
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
#endif
    }

    for (int i = 0; i < 256; i++)
    {
        if (i >= 0x80)
            write_tab[i] = pfnPokeB;    // !!! for now, assume ramtop might be as low as $8000
        else
            write_tab[i] = cpuPokeB;
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


