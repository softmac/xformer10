
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

void __cdecl Go6502(int);

__inline BOOL cpuExec(int iVM)
{
    Go6502(iVM);
    return TRUE;
}

__inline BOOL cpuDisasm(int iVM, char *pch, ADDR *pPC)
{
    // stub out for now

	iVM;  pch; pPC;

    return TRUE;
}

__inline BYTE cpuPeekB(int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);
    return rgbMem[addr];
}

__inline BOOL cpuPokeB(int iVM, ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr] = (BYTE) b;
    return TRUE;
}

__inline WORD cpuPeekW(int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

    return *(WORD FAR *)&rgbMem[addr];
}

__inline BOOL cpuPokeW(int iVM, ADDR addr, WORD w)
{
    Assert((addr & 0xFFFF0000) == 0);

    *(WORD FAR *)&rgbMem[addr] = (WORD)w;
    return TRUE;
}

__inline BOOL cpuInit(PFNL pvmPokeB)
{
    extern PFN pfnPokeB;

	// !!! global - broken if more than Atari 8 bit uses the 6502, unless each VM knows to set this every time a VM is Execute'd
	pfnPokeB = pvmPokeB;

    return TRUE;
}


__inline BOOL cpuReset(int iVM)
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


