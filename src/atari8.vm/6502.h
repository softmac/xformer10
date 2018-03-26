
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

	// !!! broken if more than Atari 8 bit uses the 6502, this needs to be set every time a machine is Execute()'d
	// independently of clearing the registers
	pfnPokeB = pvmPokeB;

    return TRUE;
}


__inline BOOL cpuReset()
{
	
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


