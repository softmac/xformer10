
/***************************************************************************

    ATARI800.C

    - Implements the public API for the Atari 800 VM.

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/


#include "atari800.h"

//
// Our VM's VMINFO structure
//

#ifdef XFORMER

VMINFO const vmi800 =
    {
    NULL,
    0,
    "Atari 800/800XL/130XE",
    /* vmAtari48C | vmAtariXLC | vmAtariXEC |*/ vmAtari48 | vmAtariXL | vmAtariXE,
    cpu6502,
    ram48K | ram64K | ram128K,
    osAtari48 | osAtariXL | osAtariXE,
    monGreyTV | monColrTV,

	X8,		// screen width
	Y8,		// screen height
	8,		// we support 2^8 colours
	rgbRainbow, // our 256 colour palette
	TRUE,	// fUsesCart
	FALSE,	// fUsesMouse
	TRUE,	// fUsesJoystick
	"Xformer/SIO2PC Disks\0*.xfd;*.atr;*.sd;*.dd\0All Files\0*.*\0\0",
    "Xformer Cartridge\0*.bin;*.rom;*.car\0All Files\0*.*\0\0",	// !!! support CAR files someday?

    InstallAtari,
	UnInstallAtari,
    InitAtari,
    UninitAtari,
    InitAtariDisks,
    MountAtariDisk,
    UninitAtariDisks,
    UnmountAtariDisk,
    ColdbootAtari,
    WarmbootAtari,
    ExecuteAtari,
    TraceAtari,
    KeyAtari,
    DumpRegsAtari,
    DumpHWAtari,
    MonAtari,
    PeekBAtari,
    PeekWAtari,
    PeekLAtari,
    PokeBAtari,
    PokeWAtari,
    PokeLAtari,
	NULL,
	NULL,
	NULL,
	NULL,
	SaveStateAtari,
	LoadStateAtari
    };


//
// Globals
//

// our machine specific data for each instance (ATARI specific stuff)
//
CANDYHW *vrgcandy[MAX_VM];

BOOL fDumpHW;

// get ready for time travel!
BOOL TimeTravelInit(unsigned iVM)
{
	BOOL fI = TRUE;

	// Initialize Time Travel
	for (unsigned i = 0; i < 3; i++)
	{
		if (!Time[iVM][i])
			Time[iVM][i] = malloc(candysize[iVM]);
		if (!Time[iVM][i])
			fI = FALSE;
		_fmemset(Time[iVM][i], 0, candysize[iVM]);
	}

	if (!fI)
		TimeTravelFree(iVM);

	return fI;
}

// all done with time traveling, I'm exhausted
//
void TimeTravelFree(unsigned iVM)
{
	for (unsigned i = 0; i < 3; i++)
	{
		if (Time[iVM][i])
			free(Time[iVM][i]);
		Time[iVM][i] = NULL;
	}
}


// call this constantly every frame, it will save a state every 5 seconds
//
BOOL TimeTravelPrepare(unsigned iVM)
{
	BOOL f = TRUE;

	const ULONGLONG s5 = 29830 * 60 * 5;	// 5 seconds
	char *pPersist;
	int cbPersist;

	// assumes we only get called when we are the current VM

	ULONGLONG cCur = GetCycles();
	ULONGLONG cTest = cCur - ullTimeTravelTime[iVM];
	
	// time to save a snapshot (every 5 seconds)
	if (cTest >= s5)
	{
		f = SaveStateAtari(iVM, &pPersist, &cbPersist);
		
		if (!f || cbPersist != candysize[iVM] || Time[iVM][cTimeTravelPos[iVM]] == NULL)
			return FALSE;

		_fmemcpy(Time[iVM][cTimeTravelPos[iVM]], pPersist, candysize[iVM]);
		
		// Do NOT remember that shift, ctrl or alt were held down.
		// If we cold boot using Ctrl-F10, almost every time travel with think the ctrl key is still down
		((CANDYHW *)Time[iVM][cTimeTravelPos[iVM]])->m_bshftByte = 0;

		ullTimeTravelTime[iVM] = cCur;
		
		cTimeTravelPos[iVM]++;
		if (cTimeTravelPos[iVM] == 3)
			cTimeTravelPos[iVM] = 0;
	}
	return f;
}


// Omega13 - go back in time 13 seconds
//
BOOL TimeTravel(unsigned iVM)
{
	BOOL f = FALSE;
	
	if (Time[iVM][cTimeTravelPos[iVM]] == NULL)
		return FALSE;

	// Two 5-second snapshots ago will be from 10-15 seconds back in time
	if (LoadStateAtari(iVM, Time[iVM][cTimeTravelPos[iVM]], candysize[iVM]))	// restore our snapshot
	{
		// set up for going back to this same point until more time passes
		f = TimeTravelReset(iVM);
	}

	return f;
}

// Call this at a point where you can't go back in time from... Cold Start, Warm Start, LoadState (e.g cartridge may be removed)
//
BOOL TimeTravelReset(unsigned iVM)
{
	char *pPersist;
	int cbPersist;

	// reset our clock to "have not saved any states yet"
	ullTimeTravelTime[iVM] = GetCycles();
	cTimeTravelPos[iVM] = 0;

	// don't remember that we were holding down shift, control or ALT, or going back in time will act like
	// they're still pressed because they won't see our letting go.
	// VERY common if you Ctrl-F10 to cold start, it's pretty much guaranteed to happen.
	// any other key can still stick, but that's less confusing
	// !!! joystick arrows stick too, and that's annoying. Need FLUSHVMINPUT in fn table
	ControlKeyUp8(iVM);

	BOOL f = SaveStateAtari(iVM, &pPersist, &cbPersist);	// get our current state

	if (!f || cbPersist != candysize[iVM])
		return FALSE;

	// initialize all our saved states to now, so that going back in time takes us back here.
	for (unsigned i = 0; i < 3; i++)
	{
		if (Time[iVM][i] == NULL)
			return FALSE;

		_fmemcpy(Time[iVM][i], pPersist, candysize[iVM]);
		
		ullTimeTravelTime[iVM] = GetCycles();	// time stamp it 
	}

	return TRUE;
}

// push PC and P on the stack
void Interrupt(int iVM)
{
	cpuPokeB(iVM, regSP, regPC >> 8);  regSP = (regSP - 1) & 255 | 256;
	cpuPokeB(iVM, regSP, regPC & 255); regSP = (regSP - 1) & 255 | 256;
	cpuPokeB(iVM, regSP, regP);          regSP = (regSP - 1) & 255 | 256;

	regP |= IBIT;
}

// What happens when it's scan line 241 and it's time to start the VBI
//
void DoVBI(int iVM)
{
	wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off
	wLeftMax = wLeft;

#ifndef NDEBUG
	fDumpHW = 0;
#endif
	
	// clear DLI, set VBI, leave RST alone - even if we're not taking the interrupt
	NMIST = (NMIST & 0x20) | 0x5F;
							
	// VBI enabled, generate VBI by setting PC to VBI routine. We'll do a few cycles of it
	// every scan line now until it's done, then resume
	if (NMIEN & 0x40) {
		Interrupt(iVM);
		regPC = cpuPeekW(iVM, 0xFFFA);
	}

	// process joysticks before the vertical blank, just because.
	// When tiling, only the tile in focus gets input
	if ((!v.fTiling || sVM == (int)iVM) && rgvm[iVM].fJoystick && vi.njc > 0)
	{
		// we can handle 4 joysticks on 800, 2 on XL/XE
		for (int joy = 0; joy < min(vi.njc, (mdXLXE == md800) ? 4 : 2); joy++)
		{
			JOYINFO ji;
			MMRESULT mm = joyGetPos(vi.rgjn[joy], &ji);
			if (mm == 0) {

				int dir = (ji.wXpos - (vi.rgjc[vi.rgjn[joy]].wXmax - vi.rgjc[vi.rgjn[joy]].wXmin) / 2);
				dir /= (int)((vi.rgjc[vi.rgjn[joy]].wXmax - vi.rgjc[vi.rgjn[joy]].wXmin) / wJoySens);

				BYTE *pB;
				if (joy < 2)
					pB = &rPADATA;	// joy 1 & 2
				else
					pB = &rPBDATA;	// joy 3 & 4

				// joy 1 & 3 are low nibble, 2 & 4 are high nibble

				(*pB) |= (12 << ((joy & 1) << 4));                  // assume joystick X centered

				if (dir < 0)
					(*pB) &= ~(4 << ((joy & 1) << 2));              // left
				else if (dir > 0)
					(*pB) &= ~(8 << ((joy & 1) << 2));              // right

				dir = (ji.wYpos - (vi.rgjc[vi.rgjn[joy]].wYmax - vi.rgjc[vi.rgjn[joy]].wYmin) / 2);
				dir /= (int)((vi.rgjc[vi.rgjn[joy]].wYmax - vi.rgjc[vi.rgjn[joy]].wYmin) / wJoySens);

				(*pB) |= (3 << ((joy & 1) << 4));                   // assume joystick centered

				if (dir < 0)
					(*pB) &= ~(1 << ((joy & 1) << 2));              // up
				else if (dir > 0)
					(*pB) &= ~(2 << ((joy & 1) << 2));              // down

				UpdatePorts(iVM);
				
				pB = &TRIG0;

				if (ji.wButtons)
					(*(pB + joy)) &= ~1;                // JOY fire button down
				else
					(*(pB + joy)) |= 1;                 // JOY fire button up
			}
		}
	}

	CheckKey(iVM);	// process the ATARI keyboard buffer

	if (fTrace)
		ForceRedraw(iVM);	// it might do this anyway

	// every VBI, shadow the hardware registers
	// to their higher locations

	// !!! Do this quicker in PeekBAtari() as they are read?

	memcpy(&rgbMem[0xD410], &rgbMem[0xD400], 16);
	memcpy(&rgbMem[0xD420], &rgbMem[0xD400], 32);
	memcpy(&rgbMem[0xD440], &rgbMem[0xD400], 64);
	memcpy(&rgbMem[0xD480], &rgbMem[0xD400], 128);

	memcpy(&rgbMem[0xD210], &rgbMem[0xD200], 16);
	memcpy(&rgbMem[0xD220], &rgbMem[0xD200], 32);
	memcpy(&rgbMem[0xD240], &rgbMem[0xD200], 64);
	memcpy(&rgbMem[0xD280], &rgbMem[0xD200], 128);

	memcpy(&rgbMem[0xD020], &rgbMem[0xD000], 32);
	memcpy(&rgbMem[0xD040], &rgbMem[0xD000], 64);
	memcpy(&rgbMem[0xD080], &rgbMem[0xD000], 128);

	// Gem window has a message, stop the loop to process it
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		fStop = TRUE;

	// decrement printer timer

	if (vi.cPrintTimeout && rgvm[iVM].fShare)
	{
		vi.cPrintTimeout--;
		if (vi.cPrintTimeout == 0)
		{
			FlushToPrinter(iVM);
			UnInitPrinter();
		}
	}
	else
		FlushToPrinter(iVM);
}

void UpdatePorts(int iVM)
{
	if (PACTL & 4)
	{
		BYTE b = wPADATA ^ rPADATA;

		PORTA = (b & wPADDIR) ^ rPADATA;
	}
	else
		PORTA = wPADDIR;

	if (PBCTL & 4)
	{
		BYTE b = wPBDATA ^ rPBDATA;

		PORTB = (b & wPBDDIR) ^ rPBDATA;
	}
	else
		PORTB = wPBDDIR;
}

// Read in the cartridge
//
BOOL ReadCart(int iVM)
{
	unsigned char *pch = (unsigned char *)rgvm[iVM].rgcart.szName;

	int h;
	int cb = 0, cb2;
	BYTE type = 0, *pb;

	bCartType = 0;
	
	if (!pch || !rgvm[iVM].rgcart.fCartIn)
	{
		return TRUE;	// nothing to do, all OK
	}

	h = _open((LPCSTR)pch, _O_BINARY | _O_RDONLY);
	if (h != -1)
	{
		//printf("reading %s\n", pch);

		cb = _lseek(h, 0L, SEEK_END);
		
		// valid sizes are 8K - 1024K with or without a 16 byte header
		if (cb > MAX_CART_SIZE || (cb & ~0x1fe010))
		{
			_close(h);
			goto exitCart;
		}

		// must be unsigned to compare the byte values inside
		rgbSwapCart[iVM] = malloc(cb);
		pb = rgbSwapCart[iVM];
		type = 0;

		_lseek(h, 0L, SEEK_SET);

		// read the header
		if (pb && (cb & 0x10))
		{
			cb2 = _read(h, pb, 16);
			if (cb2 != 16)
			{
				_close(h);
				goto exitCart;
			}
			cb -= 16;
			type = pb[7];	// the type of cartridge
		}

		//      printf("size of %s is %d bytes\n", pch, cb);
		//      printf("pb = %04X\n", rgcart[iCartMac].pbData);

		cb = _read(h, pb, cb);

		rgvm[iVM].rgcart.cbData = cb;
		rgvm[iVM].rgcart.fCartIn = TRUE;
	}
	else
		goto exitCart;

	_close(h);

	// what kind of cartridge is it? Here are the possibilites
	//
	//{name: 'Standard 8k cartridge', id : 1 },
	//{name: 'Standard 16k cartridge', id : 2 },
	//{ name: 'OSS two chip 16 KB cartridge (034M)', id : 3 },
	//{ name: 'OSS two chip 16 KB cartridge (043M)', id : 45 },
	//{ name: 'OSS one chip 16 KB cartridge', id : 15 },
	//{name: 'XEGS 32 KB cartridge', id : 12 },
	//{name: 'XEGS 64 KB cartridge', id : 13 },
	//{name: 'XEGS 128 KB cartridge', id : 14 },
	//{name: 'XEGS 256 KB cartridge', id : 23 },
	//{name: 'XEGS 512 KB cartridge', id : 24 },
	//{ name: 'XEGS 1024 KB cartridge', id : 25 },
	//{name: 'Bounty Bob Strikes Back 40 KB cartridge', id : 18 },

	// !!! I support these, but not the part where they can switch out to RAM yet

	//{ name: 'Switchable XEGS 32 KB cartridge', id : 33 },
	//{ name: 'Switchable XEGS 64 KB cartridge', id : 34 },
	//{ name: 'Switchable XEGS 128 KB cartridge', id : 35 },
	//{ name: 'Switchable XEGS 256 KB cartridge', id : 36 },
	//{ name: 'Switchable XEGS 512 KB cartridge', id : 37 },
	//{ name: 'Switchable XEGS 1024 KB cartridge', id : 38 },

	// types I don't support yet, or at least not deliberately

	//{ name: 'OSS 8 KB cartridge', id : 44 },
	//{ name: 'MegaCart 16 KB cartridge', id : 26 },
	//{ name: 'Blizzard 16 KB cartridge', id : 40 },
	//{ name: '32 KB Williams cartridge', id : 22 },
	//{ name: 'MegaCart 32 KB cartridge', id : 27 },
	//{ name: '64 KB Williams cartridge', id : 8 },
	//{ name: 'Express 64 KB cartridge ', id : 9 },
	//{ name: 'Diamond 64 KB cartridge', id : 10 },
	//{ name: 'SpartaDOS X 64 KB cartridge', id : 11 },
	//{ name: 'MegaCart 64 KB cartridge', id : 28 },
	//{ name: 'Atarimax 128 KB cartridge', id : 41 },
	//{ name: 'MegaCart 128 KB cartridge', id : 29 },
	//{ name: 'SpartaDOS X 128 KB cartridge', id : 43 },
	//{ name: 'SIC! 128 KB cartridge', id : 54 },
	//{ name: 'MegaCart 256 KB cartridge', id : 30 },
	//{ name: 'SIC! 256 KB cartridge', id : 55 },
	//{ name: 'MegaCart 512 KB cartridge', id : 31 },
	//{ name: 'SIC! 512 KB cartridge', id : 56 },
	//{name: 'Atarimax 1 MB Flash cartridge', id : 42 },
	//{ name: 'MegaCart 1024 KB cartridge', id : 32 },

	if (cb < 8192)
		goto exitCart;
	
	else if (type == 1 || (!type && cb == 8192))
		bCartType = CART_8K;

	
	else if (cb == 16384)
	{
		// copies of the INIT address and the CART PRESENT byte appear in both cartridge areas - not banked
		if (type == 2 || (pb[16383] >= 0x80 && pb[16383] < 0xC0 && pb[16380] == 0 && pb[8191] >= 0x80 && pb[8191] < 0xC0 && pb[8188] == 0))
			bCartType = CART_16K;

		// INIT area is in the lower half which wouldn't exist yet if we were banked
		else if (type == 0 && pb[16383] >= 0x80 && pb[16383] < 0xA0 && pb[16380] == 0)
			bCartType = CART_16K;

		// last bank is the main bank, other banks are 0, 3, 4.
		else if (type == 3 ||
				(type == 0 && pb[16383] >= 0x80 && pb[16383] < 0xC0 && pb[16380] == 0 && pb[4095] == 0 && pb[8191] == 3 && pb[12287] == 4))
			bCartType = CART_OSSA;

		// last bank is the main bank, other banks are 0, 4, 3.
		else if (type == 45 ||
			(type == 0 && pb[16383] >= 0x80 && pb[16383] < 0xC0 && pb[16380] == 0 && pb[4095] == 0 && pb[8191] == 4 && pb[12287] == 3))
			bCartType = CART_OSSA;

		// first bank is the main bank, other banks are 0, 9 1.
		else if (type == 15 ||
				(type == 0 && pb[4095] >= 0x80 && pb[4095] < 0xC0 && pb[4092] == 0 && pb[8191] == 0 && pb[12287] == 9 && pb[16383] == 1))
			bCartType = CART_OSSB;

		// valid L slot INIT address OR RUN address, and CART PRESENT byte - assume a 16K cartridge
		// Computer War has its INIT procedure inside the OS! Others don't have a run address.
		else if (((pb[16383] >= 0x80 && pb[16383] < 0xC0) || (pb[16379] >= 0x80 && pb[16379] < 0xC0)) && pb[16380] == 0)
			bCartType = CART_16K;

		// valid R slot INIT address OR RUN address, and CART PRESENT byte - assume a 16K cartridge
		else if (((pb[8191] >= 0x80 && pb[8191] < 0xC0) || (pb[8187] >= 0x80 && pb[8187] < 0xC0)) && pb[8188] == 0)
			bCartType = CART_16K;

		// bad cartridge?
		else
			bCartType = 0;
	}

	else if (type == 18 || (type == 0 && cb == 40960))
	{
		bCartType = CART_BOB;	// unique Bounty Bob cart
	}

	// make sure the last bank is a valid ATARI 8-bit cartridge
	else if ( ((type >= 12 && type <= 14) || (type >=23 && type <=25) || (type >= 33 && type <= 38)) || (type == 0 && 
		(((*(pb + cb - 1) >= 0x80 && *(pb + cb - 1) < 0xC0) || (*(pb + cb - 5) >= 0x80 && *(pb + cb - 1) < 0xC0)) && *(pb + cb - 4) == 0)))
	{
		bCartType = CART_XEGS;
	}
	else
		bCartType = 0;


#if 0
	// unknown cartridge, do not attempt to run it
	else
	{
		rgvm[iVM].rgcart.cbData = 0;
		rgvm[iVM].rgcart.fCartIn = FALSE;
	}
#endif

exitCart:

	if (!bCartType)
	{
		rgvm[iVM].rgcart.fCartIn = FALSE;	// unload the cartridge
		rgvm[iVM].rgcart.szName[0] = 0; // erase the name
	}

	return bCartType;	// say if cart looks good or not
}


// SWAP in the cartridge
//
void InitCart(int iVM)
{
	// no cartridge
	if (!(rgvm[iVM].rgcart.fCartIn) || !bCartType)
	{
		// convenience for Atari 800, we can ask for BASIC to be put in
		if (rgvm[iVM].bfHW == vmAtari48 && ramtop <= 0xA000)
		{
			_fmemcpy(&rgbMem[0xA000], rgbXLXEBAS, 8192);
			ramtop = 0xA000;
		}
		return;
	}

	PCART pcart = &(rgvm[iVM].rgcart);
	unsigned int cb = pcart->cbData;
	BYTE *pb = rgbSwapCart[iVM];

	if (bCartType == CART_8K)
	{
		_fmemcpy(&rgbMem[0xC000 - (((cb + 4095) >> 12) << 12)], pb, (((cb + 4095) >> 12) << 12));
		ramtop = 0xA000;
	}
	else if (bCartType == CART_16K)
	{
		_fmemcpy(&rgbMem[0x8000], pb, 16384);
		ramtop = 0x8000;
	}
	// main bank is the last one
	else if (bCartType == CART_OSSA)
	{
		_fmemcpy(&rgbMem[0xB000], pb + 12288, 4096);
		ramtop = 0xA000;
	}
	// main bank is the first one
	else if (bCartType == CART_OSSB)
	{
		_fmemcpy(&rgbMem[0xB000], pb, 4096);
		ramtop = 0xA000;
	}
	// 8K main bank is the last one
	else if (bCartType == CART_BOB)
	{
		_fmemcpy(&rgbMem[0xA000], pb + cb - 8192, 8192);
		rgbMem[0x9ffc] = 0xff;	// right cartridge present bit must float high if it is not RAM
		ramtop = 0x8000;
	}
	// 8K main bank is the last one
	else if (bCartType == CART_XEGS)
	{
		_fmemcpy(&rgbMem[0xA000], pb + cb - 8192, 8192);
		rgbMem[0x9ffc] = 0xff;	// right cartridge present bit must float high if it is not RAM
		ramtop = 0x8000;
	}

	return;
}

// Swap out cartridge banks
//
void BankCart(int iVM, int iBank, int value)
{
	PCART pcart = &(rgvm[iVM].rgcart);
	unsigned int cb = pcart->cbData;
	int i;
	BYTE *pb = rgbSwapCart[iVM];

	// we are not a banking cartridge
	if (ramtop == 0xC000 || !(rgvm[iVM].rgcart.fCartIn) || cb <= 8192 || bCartType <= CART_16K)
		return;

	// banks are 0, 3, 4, main
	if (bCartType == CART_OSSA)
	{
		i = (iBank == 0 ? 0 : (iBank == 3 ? 1 : (iBank == 4 ? 2 : -1)));
		assert(i != -1);
		if (i != -1)
			_fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
	}

#if 0
	// banks are 0, 4, 3, main
	if (bCartType == CART_OSSA_DIFFERENT)
	{
		i = (iBank == 0 ? 0 : (iBank == 3 ? 2 : (iBank == 4 ? 1 : -1)));
		//assert(i != -1);
		if (i != -1)
			_fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
	}
#endif

	// banks are main, 0, 9, 1
	else if (bCartType == CART_OSSB)
	{
		if (!(iBank & 8) && !(iBank & 1))
			i = 1;
		else if (!(iBank & 8) && (iBank & 1))
			i = 3;
		else if ((iBank & 8) && (iBank & 1))
			i = 2;
		else
			i = -1; 	//!!! swapping OSS A & B cartridge out to RAM not supported

		assert(i != -1);
		if (i != -1)
			_fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
	}

	// Bounty Bob - bank 0 is $8000-$9000, 4 choices starting at beginning of file
	// bank 1 is $9000-$a000, 4 choices starting 16K into file
	// value 0-3 selects a 4K ban
	// Also, it is selected by poking $8ff6 + value or $9ff6 + value, NOT the usual way
	// we will get additional calls thinking we're XEGS, so make sure to ignore those
	else if (bCartType == CART_BOB)
	{
		if ((regEA >= 0x8ff6 && regEA <= 0x8ff9) || (regEA >= 0x9ff6 && regEA <= 0x9ff9))
		{
			if (iBank == 0 && value >= 0 && value <= 3)
				_fmemcpy(&rgbMem[0x8000], pb + value * 4096, 4096);
			if (iBank == 1 && value >= 0 && value <= 3)
				_fmemcpy(&rgbMem[0x9000], pb + 16384 + value * 4096, 4096);
		}
	}

	// 8k banks, given as contents, not the address
	else if (bCartType == CART_XEGS)
	{
		while ((unsigned int)value >= cb >> 13)
			value -= (cb >> 13);
		
		if (value < (int)(cb << 13))
			_fmemcpy(&rgbMem[0x8000], pb + value * 8192, 8192);
	}
}

// set the freq of a POKEY timer
void ResetPokeyTimer(int iVM, int irq)
{
	ULONG f[4], c[4];

	if (irq == 2 || irq < 0 || irq >= 4)
		return;

	// f = how many cycles do we have to count down from? (Might be joined to another channel)
	// c = What is the clock frequency? If 2 is joined to 1, 2 gets 1's clock (and 4 gets 3's)
	// Then set now how many instructions have to execute before reaching 0 (447447 instr/s @ 4 cycles/instr)

	// STIMER will call us for all 4 voices in order, so the math will work out.

	if (irq == 0)
	{
		f[0] = AUDF1;
		c[0] = (AUDCTL & 0x40) ? 1789790 : ((AUDCTL & 0x01) ? 15700 : 63921);
		irqPokey[0] = (ULONG)((ULONGLONG)f[0] * 447447 / c[0]);
	}
	else if (irq == 1)
	{
		f[1] = (AUDCTL & 0x10) ? (AUDF2 << 8) + AUDF1 : AUDF2; // when joined, these count down much slower
		c[0] = (AUDCTL & 0x40) ? 1789790 : ((AUDCTL & 0x01) ? 15700 : 63921);
		c[1] = (AUDCTL & 0x10) ? c[0] : ((AUDCTL & 0x01) ? 15700 : 63921);
		irqPokey[1] = (ULONG)((ULONGLONG)f[1] * 447447 / c[1]);
	}
	else if (irq == 3)
	{
		f[3] = (AUDCTL & 0x08) ? (AUDF4 << 8) + AUDF3 : AUDF4;
		c[2] = (AUDCTL & 0x20) ? 1789790 : ((AUDCTL & 0x01) ? 15700 : 63921);	// irq 3 needs to know what this is
		c[3] = (AUDCTL & 0x08) ? c[2] : ((AUDCTL & 0x01) ? 15700 : 63921);
		irqPokey[3] = (ULONG)((ULONGLONG)f[3] * 447447 / c[3]);
	}

	// 0 means not being used, so the minimum value for wanting an interrupt is 1
	// !!! will anything besides an IRQ ever run if we want >1 per scan line?
	if (f[irq] > 0 && irqPokey[irq] == 0)
			irqPokey[irq] = 1;

	//if (irqPokey[irq]) ODS("TIMER %d f=%d c=%d WAIT=%d instr\n", irq + 1, f[irq], c[irq], irqPokey[irq]);
}

#ifndef NDEBUG
void DumpROM(char *filename, char *label, char *rgb, int start, int len)
{
    FILE *fp;
    int i, j;

    fp = fopen(filename, "wt");

    fprintf(fp, "unsigned char %s[%d] =\n{\n", label, len);

    for (i = 0; i < len; i += 8)
      {
      fprintf(fp, "    ");

      for (j = 0; j < 8; j++)
        {
        fprintf(fp, "0x%02X", (unsigned char)rgb[i + j]);

        if (j < 7)
          fprintf(fp, ", ");
        else
          fprintf(fp, ", // $%04X\n", start + i);
        }
      }

    fprintf(fp, "};\n\n");
    fclose(fp);
}

void DumpROMS()
{
    DumpROM("atariosb.c", "rgbAtariOSB", rgbAtariOSB, 0xD800, 0x2800); // 10K
    DumpROM("atarixl5.c", "rgbXLXE5000", rgbXLXE5000, 0x5000, 0x1000); //  4K
    DumpROM("atarixlb.c", "rgbXLXEBAS",  rgbXLXEBAS,  0xA000, 0x2000); //  8K
    DumpROM("atarixlc.c", "rgbXLXEC000", rgbXLXEC000, 0xC000, 0x1000); //  4K
    DumpROM("atarixlo.c", "rgbXLXED800", rgbXLXED800, 0xD800, 0x2800); // 10K
}
#endif

//
// And finally, our entries for the VM function table
//

// call me when first creating the instance. Provide our machine types VMINFO and the type of machine you want (800 vs. XL vs. XE)
//
BOOL __cdecl InstallAtari(int iVM, PVMINFO pvmi, int type)
{
	pvmi;

	// Install an Atari 8-bit VM

	static BOOL fInited;
	if (!fInited) // static, only 1 instance needs to do this
	{
		int j, p;
		extern void * jump_tab[512];
		extern void * jump_tab_RO[256];
		unsigned i;

		for (i = 0; i < 256; i++)
		{
			jump_tab[i] = jump_tab_RO[i];
		}

		// Initialize the poly counters
		// run through all the poly counters and remember the results in a table
		// so we can look them up and not have to do expensive calculations while emulating
		
		// poly5's cycle is 31
		poly5[0] = 0;
		p = 0;
		for (j = 1; j < 0x1f; j++)
		{
			p = ((p >> 1) | ((~(p ^ (p >> 3)) & 0x01) << 4)) & 0x1F;
			poly5[j] = (BYTE)p;
		}

		// poly9's cycle is 511 - we only ever look at the low bit
		poly9[0] = 0;
		p = 0;
		for (j = 1; j < 0x1ff; j++)
		{
			p = ((p >> 1) | ((~((~p) ^ ~(p >> 5)) & 0x01) << 8)) & 0x1FF;
			poly9[j] = p & 0xff;
		}
		// poly17's cycle is 0x1ffff - we only ever look at the low byte
		poly17[0] = 0;
		p = 0;
		for (j = 1; j < 0x1ffff; j++)
		{
			p = ((p >> 1) | ((~((~p) ^ ~(p >> 5)) & 0x01) << 16)) & 0x1FFFF;
			poly17[j] = p & 0xff;
		}

		// poly4's cycle is 15
		poly4[0] = 0;
		p = 0;
		for (j = 1; j < 0x0f; j++)
		{
			p = ((p >> 1) | ((~(p ^ (p >> 1)) & 0x01) << 3)) & 0x0F;
			poly4[j] = (BYTE)p;
		}

		// start the counters at the beginning
		for (int vv = 0; vv < 4; vv++)
		{
			poly4pos[vv] = 0; poly5pos[vv] = 0; poly9pos[vv] = 0; poly17pos[vv] = 0;
		}
		random17pos = 0;
		random17last = 0;

		fInited = TRUE;
	}

	// seed the random number generator again. The real ATARI is probably seeded by the orientation
	// of sector 1 on the floppy. I wonder how cartridges are unpredictable?
	// don't allow the seed that gets stuck
	do {
		LARGE_INTEGER qpc;
		QueryPerformanceCounter(&qpc);
		random17pos = qpc.QuadPart & 0x1ffff;
	} while (random17pos == 0x1ffff);

	// These things change depending on the machine we're emulating

	switch (1 << type)
	{
	case vmAtari48:
		rgvm[iVM].bfHW = vmAtari48;
		rgvm[iVM].iOS = 0;
		rgvm[iVM].bfRAM = ram48K;
		strcpy(rgvm[iVM].szModel, rgszVM[1]);
		break;

	case vmAtariXL:
		rgvm[iVM].bfHW = vmAtariXL;
		rgvm[iVM].iOS = 1;
		rgvm[iVM].bfRAM = ram64K;
		strcpy(rgvm[iVM].szModel, rgszVM[2]);
		break;

	case vmAtariXE:
		rgvm[iVM].bfHW = vmAtariXE;
		rgvm[iVM].iOS = 2;
		rgvm[iVM].bfRAM = ram128K;
		strcpy(rgvm[iVM].szModel, rgszVM[3]);
		break;

	default:
		// oops
		return FALSE;
	}

#if 0
	// fake in the Atari 8-bit OS entires
	if (v.cOS < 3)
        {
        v.rgosinfo[v.cOS++].osType  = osAtari48;
        v.rgosinfo[v.cOS++].osType  = osAtariXL;
        v.rgosinfo[v.cOS++].osType  = osAtariXE;
        }
#endif

	// DumpROMS();

	// Initialize everything

	// how big is our persistable structure?

	candysize[iVM] = sizeof(CANDYHW);
	if (rgvm[iVM].bfHW != vmAtari48)
		candysize[iVM] += (2048 + 4096 + 10240);	// extended XL/XE RAM
	if (rgvm[iVM].bfHW == vmAtariXE)
		candysize[iVM] += (16384 * 4);			// extended XE RAM

	vrgcandy[iVM] = malloc(candysize[iVM]);

	if (vrgcandy[iVM])
		_fmemset(vrgcandy[iVM], 0, candysize[iVM]); // don't think the time travel pointers are valid
	else
		return FALSE;

	// candy must be set up first (above)
	TimeTravelInit(iVM);

	return TRUE;
}

// all done
//
BOOL __cdecl UnInstallAtari(int iVM)
{
	TimeTravelFree(iVM);

	free(vrgcandy[iVM]);

	return TRUE;
}

// Initialize a VM. Either call this to create a default new instance and then ColdBoot it,
//or call LoadState to restore a previous one, not both.
//
BOOL __cdecl InitAtari(int iVM)
{
	// These things are the same for each machine type

	rgvm[iVM].fCPUAuto = TRUE;
	rgvm[iVM].bfCPU = cpu6502;
	rgvm[iVM].bfMon = monColrTV;
	rgvm[iVM].ivdMac = sizeof(rgvm[0].rgvd) / sizeof(VD);	// we only have 8, others have 9

	// by default, use XL and XE built in BASIC, but no BASIC for Atari 800.
	// unless Shift-F10 changes it
	ramtop = (rgvm[iVM].bfHW > vmAtari48) ? 0xA000 : 0xC000;
	wStartScan = STARTSCAN;	// this is when ANTIC starts fetching. Usually 3x "blank 8" means the screen starts at 32.

	switch (rgvm[iVM].bfHW)
	{
	default:
		mdXLXE = md800;
		break;

	case vmAtariXL:
		//case vmAtariXLC:
		mdXLXE = mdXL;
		break;

	case vmAtariXE:
		//case vmAtariXEC:
		mdXLXE = mdXE;
		break;
	}

	rgvm[iVM].bfRAM = BfFromWfI(rgvm[iVM].pvmi->wfRAM, mdXLXE);

	// If our saved state had a cartridge, load it back in
    // !!! I don't report an error, just run the VM without the cart
	ReadCart(iVM);

    if (!FInitSerialPort(rgvm[iVM].iCOM))
		rgvm[iVM].iCOM = 0;
    if (!InitPrinter(rgvm[iVM].iLPT))
		rgvm[iVM].iLPT = 0;

	// We have per instance and overall sound on/off, right now everything is always on.
	fSoundOn = TRUE;
    
//  vi.pbRAM[0] = &rgbMem[0xC000-1];
//  vi.eaRAM[0] = 0;
//  vi.cbRAM[0] = 0xC000;
//  vi.eaRAM[1] = 0;
//  vi.cbRAM[1] = 0xC000;
//  vi.pregs = &rgbMem[0];

    return TRUE;
}

// Call when you are done with the VM, or need to change cartridges
//
BOOL __cdecl UninitAtari(int iVM)
{
	// if there's no cartridge in, make sure the RAM footprint doesn't look like there is
	// (This is faster than clearing all memory). If it's replaced by a built-in BASIC cartridge,
	// ramtop won't change and it won't clear automatically on cold boot.
	if (vrgcandy[iVM])
	{
		rgbMem[0x9ffc] = 1;	// no R cart
		rgbMem[0xbffc] = 1;	// no L cart
		rgbMem[0xbffd] = 0;	// no special R cartridge
	}

	// free the cartridge
	if (rgbSwapCart[iVM])
		free(rgbSwapCart[iVM]);
	rgbSwapCart[iVM] = NULL;

    UninitAtariDisks(iVM);

    return TRUE;
}

BOOL __cdecl MountAtariDisk(int iVM, int i)
{
	UnmountAtariDisk(iVM, i);	// make sure all emulator types know to do this first

    PVD pvd = &rgvm[iVM].rgvd[i];

    if (pvd->dt == DISK_IMAGE)
		// !!! needs to return a value
        AddDrive(iVM, i, (BYTE *)pvd->sz);

    return TRUE;
}

BOOL __cdecl InitAtariDisks(int iVM)
{
    int i;
	BOOL f;

	for (i = 0; i < rgvm[iVM].ivdMac; i++)
	{
		f = MountAtariDisk(iVM, i);
		if (!f)
			return FALSE;
	}
    return TRUE;
}

BOOL __cdecl UnmountAtariDisk(int iVM, int i)
{
    PVD pvd = &rgvm[iVM].rgvd[i];

		if (pvd->dt == DISK_IMAGE)
			DeleteDrive(iVM, i);

    return TRUE;
}

BOOL __cdecl UninitAtariDisks(int iVM)
{
    int i;

    for (i = 0; i < rgvm[iVM].ivdMac; i++)
        UnmountAtariDisk(iVM, i);

    return TRUE;
}

// soft reset
//
BOOL __cdecl WarmbootAtari(int iVM)
{
	// tell the CPU which 
	cpuInit(PokeBAtari);

	//OutputDebugString("\n\nWARM START\n\n");
    NMIST = 0x20 | 0x1F;
    regPC = cpuPeekW(iVM, (mdXLXE != md800) ? 0xFFFC : 0xFFFA);
    cntTick = 255;	// delay for banner messages
    
	fBrakes = TRUE;	// go back to real time
	wScan = 0;	// start at top of screen again

	// too slow to do anytime but startup
	//InitSound();	// need to reset and queue audio buffers

	InitJoysticks();	// let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	CaptureJoysticks();

	return TimeTravelReset(iVM); // state is now a valid anchor point
}

// Cold Start the machine - the first one is currently done when it first becomes the active instance
//
BOOL __cdecl ColdbootAtari(int iVM)
{
    unsigned addr;
	//OutputDebugString("\n\nCOLD START\n\n");

	BOOL f = InitAtariDisks(iVM);

	if (!f)
		return FALSE;

	cntTick = 255;	// start the banner up again
    
	fBrakes = TRUE; // go back to real time
	clockMult = 1;	// per instance speed-up
	wScan = 0;	// start at top of screen again
	wFrame = 0;

    //printf("ColdStart: mdXLXE = %d, ramtop = %04X\n", mdXLXE, ramtop);

#if 0 // doesn't change
    if (mdXLXE == md800)
        rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtari48C :*/ vmAtari48;
    else if (mdXLXE == mdXL)
		rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtariXLC :*/ vmAtariXL;
    else
		rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtariXEC :*/ vmAtariXE;
#endif
    
	//DisplayStatus(); this might not be the active instance

    // Swap in BASIC and OS ROMs.

	// load the OS
    InitBanks(iVM);

	// load the cartridge
	InitCart(iVM);

    // reset the registers, and say which HW it is running
    cpuReset(iVM);
	cpuInit(PokeBAtari);

	// XL's need hardware to cold start, and can only warm start through emulation.
	// We have to invalidate this checksum to force a cold start
	if (mdXLXE != md800)
		rgbMem[0x033d] = 0;

#if 0 // doesn't the OS do this?
	// initialize memory up to ramtop

	for (addr = 0; addr < ramtop; addr++)
	{
		cpuPokeB(addr, 0xAA);
	}
#endif

    // initialize hardware
    // NOTE: at $D5FE is ramtop and then the jump table

    for (addr = 0xD000; addr < 0xD5FE; addr++)
    {
        cpuPokeB(iVM, addr,0xFF);
    }

    // CTIA/GTIA reset

	// we want to remove BASIC, on an XL the only way is to fake OPTION being held down.
	// but now it will get stuck down! ExecuteAtari will fix that
	CONSOL = ((mdXLXE != md800) && (GetKeyState(VK_F9) < 0 || ramtop == 0xC000)) ? 3 : 7;
    PAL = 14;
    TRIG0 = 1;
    TRIG1 = 1;
    TRIG2 = 1;
	// XL cartridge sense line, without this warm starts can try to run an non-existent cartridge
	TRIG3 = (mdXLXE != md800 && !(rgvm[iVM].rgcart.fCartIn)) ? 0 : 1;
    *(ULONG *)MXPF  = 0;
    *(ULONG *)MXPL  = 0;
    *(ULONG *)PXPF  = 0;
    *(ULONG *)PXPL  = 0;
	GRAFP0 = 0;
	GRAFP1 = 0;
	GRAFP2 = 0;
	GRAFP3 = 0;
	GRAFM = 0;

    // POKEY reset

    POT0 = 228;
    POT1 = 228;
    POT2 = 228;
    POT3 = 228;
    POT4 = 228;
    POT5 = 228;
    POT6 = 228;
    POT7 = 228;
    SKSTAT = 0xFF;
    IRQST = 0xFF;
    AUDC1 = 0x00;
    AUDC2 = 0x00;
    AUDC3 = 0x00;
    AUDC4 = 0x00;

    SKCTLS = 0;
    IRQEN = 0;
    SEROUT = 0;

    // ANTIC reset

    VCOUNT = 0;
    NMIST = 0x1F;

    NMIEN = 0;
    CHBASE = 0;
    PMBASE = 0;
    VSCROL = 0;
    HSCROL = 0;
    DLISTH = 0;
    DLISTL = 0;
    CHACTL = 0;
    DMACTL = 0;

    // PIA reset - this is a lot of stuff for only having 4 registers

	rPADATA = 255;	// the data PORTA will provide if it is in READ mode
	rPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000) ? 253 : 255) : 255; // XL: HELP out, BASIC = ??, OS IN
	PORTA = rPADATA; // since they default to read mode
	PORTB = rPBDATA;

	wPADATA = 0;	// the data written to PORTA to be provided in WRITE mode (must default to 0 for Caverns of Mars)
	wPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000) ? 253 : 255) : 0; // XL needs to default to OS IN and HELP OUT
	
	wPADDIR = 0;	// direction - default to read
	wPBDDIR = 0;	
	
	PACTL  = 0x3c;	// I/O mode, not SET DIR mode
    PBCTL  = 0x3c;	// I/O mode, not SET DIR mode

	// seed the random number generator again. The real ATARI is probably seeded by the orientation
	// of sector 1 on the floppy. I wonder how cartridges are unpredictable?
	// don't allow the seed that gets stuck
	do {
		LARGE_INTEGER qpc;
		QueryPerformanceCounter(&qpc);
		random17pos = qpc.QuadPart & 0x1ffff;
	} while (random17pos == 0x1ffff);

	//too slow to do anytime but app startup
	//InitSound();	// Need to reset and queue audio buffers
	
	InitJoysticks(); // let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	CaptureJoysticks();

	return TimeTravelReset(iVM); // state is now a valid anchor point
}

// SAVE: return a pointer to our data, and how big it is. It will not be harmed, and used before we are Uninit'ed, so don't
// waste time copying anything, just return the right pointer.
// The pointer might be NULL if they just want the size
//
BOOL __cdecl SaveStateAtari(int iVM, char **ppPersist, int *pcbPersist)
{
	// there are some time travel pointers in the structure, we need to be smart enough not to use them
	if (ppPersist)
		*ppPersist = (char *)vrgcandy[iVM];
	if (pcbPersist)
		*pcbPersist = candysize[iVM];
	return TRUE;
}

// LOAD: copy from the data we've been given. Make sure it's the right size
// Either INIT is called followed by ColdBoot, or this, not both, so do what we need to do to restore our state
// without resetting anything
//
BOOL __cdecl LoadStateAtari(int iVM, char *pPersist, int cbPersist)
{
	if (cbPersist != candysize[iVM] || pPersist == NULL)
		return FALSE;

	// load our state
	_fmemcpy(vrgcandy[iVM], pPersist, cbPersist);

	if (!FInitSerialPort(rgvm[iVM].iCOM))
		rgvm[iVM].iCOM = 0;
	if (!InitPrinter(rgvm[iVM].iLPT))
		rgvm[iVM].iLPT = 0;

	BOOL f = InitAtariDisks(iVM);

	if (!f)
		return f;

	// If our saved state had a cartridge, load it back in
	ReadCart(iVM);
	InitCart(iVM);

	// too slow to do anytime but app startup
	//InitSound();	// Need to reset and queue audio buffers

	InitJoysticks(); // let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	CaptureJoysticks();

	f = TimeTravelReset(iVM); // state is now a valid anchor point

	return f;
}


BOOL __cdecl DumpHWAtari(int iVM)
{
	iVM;
#ifndef NDEBUG
//    RestoreVideo();

	// tell CPU which HW is running
	cpuInit(PokeBAtari);

    printf("NMIEN  %02X IRQEN  %02X SKCTLS %02X SEROUT %02X\n",
        NMIEN, IRQEN, SKCTLS, SEROUT);
    printf("DLISTH %02X DLISTL %02X VSCROL %02X HSCROL %02X\n",
        DLISTH, DLISTL, VSCROL, HSCROL);
    printf("PMBASE %02X DMACTL %02X CHBASE %02X CHACTL %02X\n",
        PMBASE, DMACTL, CHBASE, CHACTL);
    printf("HPOSP0 %02X HPOSP1 %02X HPOSP2 %02X HPOSP3 %02X\n",
        HPOSP0, HPOSP1, HPOSP2, HPOSP3);
    printf("HPOSM0 %02X HPOSM1 %02X HPOSM2 %02X HPOSM3 %02X\n",
        HPOSM0, HPOSM1, HPOSM2, HPOSM3);
    printf("SIZEP0 %02X SIZEP1 %02X SIZEP2 %02X SIZEP3 %02X\n",
        SIZEP0, SIZEP1, SIZEP2, SIZEP3);
    printf("COLPM0 %02X COLPM1 %02X COLPM2 %02X COLPM3 %02X\n",
        COLPM0, COLPM1, COLPM2, COLPM3);
    printf("COLPF0 %02X COLPF1 %02X COLPF2 %02X COLPF3 %02X\n",
        COLPF0, COLPF1, COLPF2, COLPF3);
    printf("GRAFP0 %02X GRAFP1 %02X GRAFP2 %02X GRAFP3 %02X\n",
        GRAFP0, GRAFP1, GRAFP2, GRAFP3);
    printf("PRIOR  %02X COLBK  %02X GRAFM  %02X SIZEM  %02X\n",
        PRIOR, COLBK, GRAFM, SIZEM);
    printf("AUDF1  %02X AUDC1  %02X AUDF2  %02X AUDC2  %02X\n",
        AUDF1, AUDC1, AUDF2, AUDC2);
    printf("AUDF3  %02X AUDC3  %02X AUDF4  %02X AUDC4  %02X\n",
        AUDF3, AUDC3, AUDF4, AUDC4);
    printf("AUDCTL %02X  $87   %02X   $88  %02X  $A2  %02X\n",
       AUDCTL, cpuPeekB(iVM, 0x87), cpuPeekB(iVM, 0x88), cpuPeekB(iVM, 0xA2));

//    ForceRedraw();
#endif // DEBUG
    return TRUE;
}

BOOL __cdecl TraceAtari(int iVM, BOOL fStep, BOOL fCont)
{
    fTrace = TRUE;
    ExecuteAtari(iVM, fStep, fCont);
    fTrace = FALSE;

    return TRUE;
}

// The big loop! Process an entire frame of execution and build the screen buffer, or, if fTrace, only do a single instruction.
//
// !!! fStep and fCont are ignored for now!
BOOL __cdecl ExecuteAtari(int iVM, BOOL fStep, BOOL fCont)
{
	fCont; fStep;

	// tell the 6502 which HW it is running this time
	cpuInit(PokeBAtari);

	fStop = 0;

	do {

		// when tracing, we only do 1 instruction per call, so there might be some left over - go straight to doing more instructions
		if (wLeft == 0)
		{

#ifndef NDEBUG
			// Display scan line here
			if (fDumpHW) {
				UINT PCt = cpuPeekW(iVM, 0x200);
				extern void CchDisAsm(int, unsigned int *);
				int i;

				printf("\n\nscan = %d, DLPC = %04X, %02X\n", wScan, DLPC,
					cpuPeekB(iVM, DLPC));
				for (i = 0; i < 60; i++)
				{
					CchDisAsm(iVM, &PCt); printf("\n");
				}
				PCt = regPC;
				for (i = 0; i < 60; i++)
				{
					CchDisAsm(iVM, &PCt); printf("\n");
				}
				FDumpHWVM(iVM);
			}
#endif // DEBUG

			// if we faked the OPTION key being held down so OSXL would remove BASIC, now it's time to lift it up
			if (wFrame > 20 && !(CONSOL & 4) && GetKeyState(VK_F9) >= 0)
				CONSOL |= 4;

			// Reinitialize clock cycle counter - number of instructions to run per call to Go6502 (one scanline or so)
			// 21 should be right for Gr.0 but for some reason 18 is?
			// 30 instr/line * 262 lines/frame * 60 frames/sec * 4 cycles/instr = 1789790 cycles/sec
			// !!! DMA cycle stealing is more complex than this! ANTIC could report how many cycles it stole, since it goes first,
			// and then Go6502 could cycle count how many are left for it to do (it would not be precise, it would have to end
			// its current instruction).
			// Allowing 30 instructions on ANTIC blank lines really helped app compat
			// Last I timed it, for z=1 to 1000 in GR.0 took 125 jiffies (DMA on) or 88-89 (DMA off).
			// Using 21 and 30, Gem took 120 (on) and 96 (off).
			wLeft = (fBrakes && (DMACTL & 3) && sl.modelo >= 2) ? 21 : INSTR_PER_SCAN_NO_DMA;	// runs faster with ANTIC turned off (all approximate)
			wLeftMax = wLeft;	// remember what it started at
			//wLeft *= clockMult;	// any speed up will happen after a frame by sleeping less

			// Scan line 0-7 are not visible, 8 lines without DMA
			// Scan lines 8-247 are 240 visible lines
			// Scan line 248 is the VBLANK, without DMA
			// Scan lines 249-261 are the rest of the scan lines, without DMA

			if (wScan < wStartScan) {
				wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off for the first 10 lines
				wLeftMax = wLeft;
			}
			else if (wScan < STARTSCAN + Y8) {	// after all the valid lines have been drawn
				// business as usual
			}

			// the CPU will have seen VCOUNT == 124 executing scan line 247, so it should be safe to VBLANK now
			// VBLANKS are long and the main CPU code won't execute for a while and will never see scan line 248
			// so that's why we cheat and send Go6502 one scanline higher than ProcessScanLine saw
			else if (wScan == STARTSCAN + Y8)
			{
				wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off
				DoVBI(iVM);	// it's huge, it bloats this function to inline it.
			}

			else if (wScan > STARTSCAN + Y8)
			{
				wLeft = INSTR_PER_SCAN_NO_DMA;	// for this retrace section, there will be no DMA
				wLeftMax = wLeft;
			}
		
			// this is, for instance, the key pressed interrupt. VBI and DLIST interrupts are NMI's.
			if (!(regP & IBIT))
			{
				if (IRQST != 255)
				{
					Interrupt(iVM);
					regPC = cpuPeekW(iVM, 0xFFFE);
					//ODS("IRQ TIME!\n");
				}
			}

			ProcessScanLine(iVM);	// do the DLI, and fill the bitmap
		  
			// VCOUNT should increment in the middle of Go6502, but we're not that fancy yet.
			// Let's use the higher value so that VCOUNT reaches 124 (248 / 2) on line 247 and apps can
			// see it get that high (breaks MULE)
		
			// some programs check "mirrors" instead of $D40B
			rgbMem[0xD47B] = rgbMem[0xD41B] = VCOUNT = (BYTE)((wScan + 1) >> 1);

			WSYNC_Seen = FALSE;

		} // if wLeft == 0

		// Normally, executes about one horizontal scan line's worth of 6502 code
		Go6502(iVM);

		// hit a breakpoint
		if (regPC == bp)
			fStop = TRUE;

		assert(wLeft == 0 || fTrace == 1);

		// Code tried to somehow jump to the SIO routine, which talks to the serial bus - we emulate that separately
		// and skip running the OS code
		if (fSIO) {
			fSIO = 0;
			SIOV(iVM);
		}

		// next scan line
		if (wLeft == 0)
		{
			wScan = wScan + 1;

			if (!WSYNC_Seen)
				WSYNC_Waited = FALSE;

			// I'm worried about perf if I do this every instruction, so cheat for now and check only every scan line
			// if the counters reach 0, trigger an IRQ. They auto-repeat and auto-fetch new AUDFx values
			// 0 means timer not being used

			for (int irq = 0; irq < 4; irq = ((irq + 1) << 1) - 1)	// 0, 1, 3 (i.e. 1, 2, 4 - timer 3 not supported)
			{
				if (irqPokey[irq] && irqPokey[irq] <= wLeftMax)
				{
					if (IRQEN & (irq + 1))
					{
						//ODS("TIMER %d FIRING\n", irq + 1);
						IRQST &= ~(irq + 1);				// fire away
					}

					UINT isav = irqPokey[irq];				// remember

					ResetPokeyTimer(iVM, irq);			// start it up again, they keep cycling forever

					if (irqPokey[irq] > wLeftMax - isav)
					{
						irqPokey[irq] -= (wLeftMax - isav);	// don't let errors propagate
					}
					else
						irqPokey[irq] = 1;					// uh oh, already time for the next one
					//ODS("TIMER %d ADJUSTED to %d\n", irq + 1, irqPokey[irq]);
				}
				else if (irqPokey[irq] > wLeftMax)
					irqPokey[irq] -= wLeftMax;
			}

		}

		// we process the audio after the whole frame is done
		if (wScan >= NTSCY)
		{
			TimeTravelPrepare(iVM);

			SoundDoneCallback(iVM, vi.rgwhdr, SAMPLES_PER_VOICE);	// finish this buffer and send it

			wScan = 0;
			wFrame++;	// count how many frames we've drawn. Does anybody care?

			// exit each frame to let the next VM run (if Tiling) and to update the clock speed on the status bar (always)
			fStop = TRUE;
		}

    } while (!fTrace && !fStop);

    return (regPC != bp);
}

//
// Stubs
//

BOOL __cdecl DumpRegsAtari(int iVM)
{
	iVM;

    // later

    return TRUE;
}


BOOL __cdecl DisasmAtari(int iVM, char *pch, ADDR *pPC)
{
	pPC; pch; iVM;
	
	// later

    return TRUE;
}


BYTE __cdecl PeekBAtari(int iVM, ADDR addr)
{
    // reads always read directly - and we can't change that, as the 6502 emulator assumes so

    return cpuPeekB(iVM, addr);
}


WORD __cdecl PeekWAtari(int iVM, ADDR addr)
{
    // reads always read directly

    return cpuPeekW(iVM, addr);
}


ULONG __cdecl PeekLAtari(int iVM, ADDR addr)
{
    // reads always read directly

    return cpuPeekW(iVM, addr);
}


BOOL __cdecl PokeWAtari(int iVM, ADDR addr, WORD w)
{
    PokeBAtari(iVM, addr, w & 255);
    PokeBAtari(iVM, addr+1, w >> 8);
    return TRUE;
}


BOOL __cdecl PokeLAtari(int iVM, ADDR addr, ULONG w)
{
	iVM; addr; w;
    return TRUE;
}

BOOL __cdecl PokeBAtari(int iVM, ADDR addr, BYTE b)
{
    BYTE bOld;
    addr &= 65535;
    b &= 255; // just to be sure

#if 0
    printf("write: addr:%04X, b:%02X, PC:%04X\n", addr, b & 255, regPC);
#endif

    if (addr < ramtop)
    {
#if 0
        printf("poking into RAM, addr = %04X, ramtop = %04X\n", addr, ramtop);
        flushall();
#endif

        cpuPokeB(iVM, addr, b);
        return TRUE;
    }

	// This is how Bounty Bob bank selects
	if (addr >= 0x8ff6 && addr <= 0x8ff9 && bCartType == CART_BOB)
		BankCart(iVM, 0, addr - 0x8ff6);
	if (addr >= 0x9ff6 && addr <= 0x9ff9 && bCartType == CART_BOB)
		BankCart(iVM, 1, addr - 0x9ff6);

	switch ((addr >> 8) & 255)
	{
	default:

		// don't allow writing to cartridge memory, but do allow writing to special extended XL/XE RAM
		if (mdXLXE == md800 ||
			(rgvm[iVM].rgcart.fCartIn && addr < 0xC000u && addr >= 0xc000u - rgvm[iVM].rgcart.cbData))
		{
			break;
		}

        // writing to XL/XE memory

        if (addr == 0xD1FF)
            {
            static const unsigned char rgbSIOR[] =
                {
                0x00, 0x00, 0x04, 0x80, 0x00, 0x4C, 0x70, 0xD7,
                0x4C, 0x80, 0xD7, 0x91, 0x00, 0x00, 0xD7, 0x10,
                0xD7, 0x20, 0xD7, 0x30, 0xD7, 0x40, 0xD7, 0x50,
                0xD7, 0x4C, 0x60, 0xD7
                };

            if (b & 1)
                {
                // swap in XE BUS device 1 (R: handler)

                memcpy(&rgbMem[0xD800], rgbSIOR, sizeof(rgbSIOR));
                }
            else
                {
                // swap out XE BUS device 1 (R: handler)

                memcpy(&rgbMem[0xD800], rgbXLXED800, sizeof(rgbSIOR));
                }
            }

        if (!(PORTB & 1) && (addr >= 0xC000))
            {
            // write to XL/XE RAM under ROM

            if ((addr < 0xD000) || (addr >= 0xD800))
                cpuPokeB(iVM, addr, b);

            break;
            }
        break;

    case 0xD0:      // GTIA
        addr &= 31;
        bOld = rgbMem[writeGTIA+addr];
        rgbMem[writeGTIA+addr] = b;
	
        if (addr == 30)
            {
            // HITCLR

            *(ULONG *)MXPF = 0;
            *(ULONG *)MXPL = 0;
            *(ULONG *)PXPF = 0;
            *(ULONG *)PXPL = 0;

            pmg.fHitclr = 1;

            //DebugStr("HITCLR!!\n");
            }
        else if (addr == 31)
            {
            // 


            // REVIEW: click the speaker

//            printf("CONSOL %02X %02X\n", bOld, b);

            if (!vi.fWinNT)
	            if ((bOld ^ b) & 8)
                {
                // toggle speaker state (mask out lowest bit?)

#if !defined(_M_ARM) && !defined(_M_ARM64)
                outp(0x61, (inp(0x61) & 0xFE) ^ 2);
#endif
                }
            }
        break;

    case 0xD2:      // POKEY
        addr &= 15;
		
		rgbMem[writePOKEY+addr] = b;

		if (addr == 9)
		{
			// STIMER - grab the new frequency of all the POKEY timers, even if they're disabled. They might be enabled by time 0
			// maybe not necessary, but I believe, technically, poking here should reset any timer that is partly counted down.
			for (int irq = 0; irq < 4; irq++)
				ResetPokeyTimer(iVM, irq);
		}
		if (addr == 10)
        {
            // SKRES

            SKSTAT |=0xE0;
        }
        else if (addr == 13)
        {
            // SEROUT - !!! unsupported

			// doesn't seem to help - if they want serial ouptut data needed or data ready interrupts, then
			// pretend we're done with their data right away and need more, even though we don't actually do anything with it
			//IRQST &= ~(IRQEN & 0x18);
        }
        else if (addr == 14)
        {
            // IRQEN - IRQST is the complement of the enabled IRQ's, and says which interrupts are active

            IRQST |= ~b; // all the bits they poked OFF (to disable an INT) have to show up here as ON

			// !!! UNSUPPORTED. If I fire the interrupts they want, an no main code ever gets a chance to run again,
			// or they hang waiting for the interrupt if I don't fire it, or they hang because I can't give them valid data.

			// !!! If they enable interrupts for when serial input data is ready, just fire it and tell them it's ready.
			// The data will be garbage, so say the data is bad, so they don't keep reading forever
			if (IRQEN & 0x20)
			{
				//SKSTAT &= 0x3f;	// both serial input frame overrun and frame error. Bad luck!
				//IRQST &= ~0x20;	// fire the INT that data is ready
			}

			// !!! If they want to be told when serial output needs more data, just tell them it needs it now!
			//if (IRQEN & 0x10)
			//	IRQST &= ~10;

			// as soon as they give us data (addr == 13) we'll tell them we're done with it right away and need more

        }
		else if (addr <= 8)
		{
			// AUDFx, AUDCx or AUDCTL have changed - write some sound
			// we're (wScan / 262) of the way through the scan lines and (wLeftMax - wLeft) of the way through this scan line
			int iCurSample = (wScan * 100 + (wLeftMax - wLeft) * 100 / wLeftMax) * SAMPLES_PER_VOICE / 100 / NTSCY;
			if (iCurSample < SAMPLES_PER_VOICE)
				SoundDoneCallback(iVM, vi.rgwhdr, iCurSample);

			// reset a timer that had its frequency changed
			for (int irq = 0; irq < 4; irq++)
			{
				if (addr == 8 || addr == (ADDR)(irq << 1))
					ResetPokeyTimer(iVM, irq);
			}
		}
        break;

    case 0xD3:      // PIA
        addr &= 3;
        rgbMem[writePIA+addr] = b;

        // The PIA is funky in that it has 8 registers in a window of 4:
        //
        // D300 - PORTA data register (read OR write) OR data direction register
        //      - the data register reads the first two joysticks
        //      - the direction register just behaves as RAM
        //        for each of the 8 bits: 1 = write, 0 = read
        //      - when reading PORTA, write bits behave as RAM
        //
        // D301 - PACTL, controls PORTA
        //      - bits 7 and 6 are ignored, always read as 0
        //      - bits 5 4 3 1 and 0 are ignored and just behave as RAM
        //      - bit 2 selects: 1 = data register, 0 = direction register
        //
        // D302 - PORTB, similar to PORTB, on the XL/XE it controls memory banks
        //
        // D303 - PBCTL, controls PORTB
        //
        // When we get a write to PACTL or PBCTL, strip off the upper two bits and
        // stuff the result into the appropriate read location. The only bit we care
        // about is bit 2 (data/direction select). When this bit toggles we need to
        // update the PORTA or PORTB read location appropriately.
        //
        // When we get a write to PORTA or PORTB, if the appropriate select bit
        // selects the data direction register, write to wPADDIR or wPBDDIR.
        // Otherwise, write to wPADATA or wPBDATA using ddir as a mask.
        //
        // When reading PORTA or PORTB, return (writereg ^ ddir) | (readreg ^ ~ddir)
        //

        if (addr & 2)
        {
            // it is a control register

            cpuPokeB(iVM, PACTLea + (addr & 1), b & 0x3C);
        }
        else
        {
            // it is either a data or ddir register. Check the control bit

            if (cpuPeekB(iVM, PACTLea + addr) & 4)
            {
                // it is a data register. Update only bits that are marked as write.

                BYTE bMask = cpuPeekB(iVM, wPADDIRea + addr);
                bOld  = cpuPeekB(iVM, wPADATAea + addr);
                BYTE bNew  = (b & bMask) | (bOld & ~bMask);

                if (bOld != bNew)
                {
                    cpuPokeB(iVM, wPADATAea + addr, bNew);

                    if ((addr == 1) && (mdXLXE != md800))
                    {
                        
						// Writing a new value to PORT B on the XL/XE.

                        if (mdXLXE != mdXE)
                        {
                            // in 800XL mode, mask bank select bits
                            bNew |= 0x7C;
                            wPBDATA |= 0x7C;
                        }

                        SwapMem(iVM, bOld ^ bNew, bNew);
                    }
                }
            }
            else
            {
                // it is a data direction register.

                cpuPokeB(iVM, wPADDIRea + addr, b);
            }
        }

        // update PORTA and PORTB read registers

        UpdatePorts(iVM);
        break;

    case 0xD4:      // ANTIC
        addr &= 15;
		
		// !!! using shadows like this could break an app that uses a functioning mirror of the registers! Does anybody?
        rgbMem[writeANTIC+addr] = b;

		// the display list pointer is the only ANTIC register that is R/W
		if (addr == 2 || addr == 3)
			rgbMem[0xd400 + addr] = b;

        if (addr == 10)
            {
            // WSYNC - our code executes after the scan line is drawn, so ignore the first one, it's already safe to draw
			// if we see a second one on this same scan line, or we saw one last time that we actually waited on,
			// we're in a kernel, and we can't ignore it
			
			// !!! Demos2/Swan needs to wait the first time, which makes no sense
			if (WSYNC_Seen || WSYNC_Waited)
			{
				wLeft = 1;
				WSYNC_Waited = TRUE;
			}
			WSYNC_Seen = TRUE;

			// (we already start out with VCOUNT incremented so this is no longer necessary)
			// if we're not going to wait, we need to make sure VCOUNT is updated like it would have been had we waited
			//rgbMem[0xD47B] = rgbMem[0xD41B] = VCOUNT = (BYTE)((wScan + 1) >> 1);
            }
        else if (addr == 15)
            {
            // NMIRES - reset NMI status

            NMIST = 0x1F;
            }
        break;

    case 0xD5:      // Cartridge bank select
//        printf("addr = %04X, b = %02X\n", addr, b);
        BankCart(iVM, addr & 255, b);
        break;
    }

    return TRUE;
}

#endif // XFORMER