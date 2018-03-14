
/***************************************************************************

    6502.H

    - Public API from the 6502 interpreter

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

#ifndef HWIN32

BOOL cpuInit(PFNL pvmPokeB);
BOOL cpuExec(void);
BOOL cpuDisasm(char *pch, ADDR *pPC);
BYTE cpuPeekB(ADDR addr);
BOOL cpuPokeB(ADDR addr, BYTE b);
WORD cpuPeekW(ADDR addr);          // performs endian conversion if necessary
BOOL cpuPokeW(ADDR addr, WORD w);  // performs endian conversion if necessary

//
// CPUINFO structure exported by each CPU module
//

typedef struct _cpuinfo
{
    LONG typeMin;           // CPU type, minimum
    LONG typeLim;           // CPU type, maximum
    LONG ver;               // version
    LONG flags;             // flags (TBD)
    LONG *opcodes;          // read-only dispatch table
    LONG *opcodesRW;        // dispatch table modified by VM
    PFNL pfnInit;           // initialization routine
    PFNL pfnGo;             // interpreter entry point for go
    PFNL pfnStep;           // interpreter entry point for step
    LONG *rgfExcptHook;     // pointer to bit vector of exceptions to hook
} CPUINFO, *PCPUINFO;

extern CPUINFO *vpci;

#endif

//
// 6502 specific implementation of the CPU API
//

void __cdecl Go6502(void);

__inline BOOL cpuExec(void)
{
    Go6502();
    return TRUE;
}

__inline BOOL cpuDisasm(char *pch, ADDR *pPC)
{
    // stub out for now

    pch; pPC;

    return TRUE;
}

__inline BYTE cpuPeekB(ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return rgbMem[addr];
    return TRUE;
}

__inline BOOL cpuPokeB(ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr] = (BYTE) b;
    return TRUE;
}

__inline WORD cpuPeekW(ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return *(WORD FAR *)&rgbMem[addr];
}

__inline BOOL cpuPokeW(ADDR addr, WORD w)
{
    Assert((addr & 0xFFFF0000) == 0);

    *(WORD FAR *)&rgbMem[addr] = (WORD)w;
    return TRUE;
}

__inline BOOL cpuInit(PFNL pvmPokeB)
{
    extern PFN pfnPokeB;

    pfnPokeB = pvmPokeB;

    // clear all registers

    regA = 0;
    regX = 0;
    regY = 0;
    regP = 0xFF;

    // set initial SP = $FF

    regSP = 0x1FF;
    regPC = cpuPeekW(0xFFFC);

    return TRUE;
}


