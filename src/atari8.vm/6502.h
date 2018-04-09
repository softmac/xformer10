
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

__inline void cpuPokeB(int iVM, ADDR addr, BYTE b)
{
    Assert((addr & 0xFFFF0000) == 0);

    rgbMem[addr] = (BYTE) b;
    return;
}

__inline WORD cpuPeekW(int iVM, ADDR addr)
{
    Assert((addr & 0xFFFF0000) == 0);

	return cpuPeekB(iVM, addr) | (cpuPeekB(iVM, addr + 1) << 8);
}

__inline void cpuPokeW(int iVM, ADDR addr, WORD w)
{
    Assert((addr & 0xFFFF0000) == 0);

	rgbMem[addr + 0] = (w & 255);
	rgbMem[addr + 1] = ((w >> 8) & 255);
	return;
}

__inline BOOL cpuInit(PFNB pvmPeekB, PFN pvmPokeB)
{
    extern PFNB pfnPeekB;
	extern PFN pfnPokeB;

	// !!! global - each VM needs to set this every time a VM is Execute'd
	pfnPeekB = pvmPeekB;
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


