
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
    InstallAtari,
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
    DisasmAtari,
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
CANDYHW vrgcandy[MAX_VM];
CANDYHW *vpcandyCur = &vrgcandy[0]; // default to first instance

//BYTE bshiftSav;	// was a local too
BOOL fDumpHW;

static signed short wLeftMax;	// keeps track of how many 6502 instructions we're trying to execute this scan line
#define INSTR_PER_SCAN_NO_DMA 30	// when DMA is off, we can do about 30. Unfortunately, with DMA on, it's variable

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
    FILE *fp;
    int i, j;

    DumpROM("atariosb.c", "rgbAtariOSB", rgbAtariOSB, 0xC800, 0x2800); // 10K
    DumpROM("atarixl5.c", "rgbXLXE5000", rgbXLXE5000, 0x5000, 0x1000); //  4K
    DumpROM("atarixlb.c", "rgbXLXEBAS",  rgbXLXEBAS,  0xA000, 0x2000); //  8K
    DumpROM("atarixlc.c", "rgbXLXEC000", rgbXLXEC000, 0xC000, 0x1000); //  4K
    DumpROM("atarixlo.c", "rgbXLXED800", rgbXLXED800, 0xD800, 0x2800); // 10K
}

// call me when first creating the instance. Provide our machine types VMINFO and the type of machine you want (800 vs. XL vs. XE)
//
BOOL __cdecl InstallAtari(int iVM, PVMINFO pvmi, int type)
{
	// Install an Atari 8-bit VM

	static BOOL fInited;
	if (!fInited) // static, only 1 instance needs to do this
	{
		extern void * jump_tab[512];
		extern void * jump_tab_RO[256];
		unsigned i;

		for (i = 0; i < 256; i++)
		{
			jump_tab[i] = jump_tab_RO[i];
		}

		fInited = TRUE;
	}

	// Initialize the poly counters
	if (fPolyValid == FALSE)
	{
		int j, p;

		// run through all the poly counters and remember the results in a table
		// so we can look them up and not have to do expensive calculations while emulating
		
		// poly5's cycle is 31
		poly5[0] = 0;
		p = 0;
		for (j = 1; j < 0x1f; j++)
		{
			p = ((p >> 1) | ((~(p ^ (p >> 3)) & 0x01) << 4)) & 0x1F;
			poly5[j] = p;
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
			poly4[j] = p;
		}

		// start the counters at the beginning
		for (int v = 0; v < 4; v++)
		{
			poly4pos[v] = 0; poly5pos[v] = 0; poly9pos[v] = 0; poly17pos[v] = 0;
		}
		random17pos = 0;
		random17last = 0;
		fPolyValid = TRUE;	// no need to ever do this again
	}

	// reset the cycle counter now and each cold start (we may not get a cold start)
	// !!! dangerous globals, could glitch the other VMs
	LARGE_INTEGER qpc;
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	vi.qpfCold = qpf.QuadPart;

	// seed the random number generator again. The real ATARI is probably seeded by the orientation
	// of sector 1 on the floppy. I wonder how cartridges are unpredictable?
	// don't allow the seed that gets stuck
	do {
		QueryPerformanceCounter(&qpc);
		vi.qpcCold = qpc.QuadPart;	// reset real time
		random17pos = qpc.QuadPart & 0x1ffff;
	} while (random17pos == 0x1ffff);

	// These things change depending on the machine we're emulating

	switch (type)
	{
	case vmAtari48:
		v.rgvm[iVM].bfHW = vmAtari48;
		v.rgvm[iVM].iOS = 0;
		v.rgvm[iVM].bfRAM = ram48K;
		strcpy(v.rgvm[iVM].szModel, rgszVM[1]);
		break;

	case vmAtariXL:
		v.rgvm[iVM].bfHW = vmAtariXL;
		v.rgvm[iVM].iOS = 1;
		v.rgvm[iVM].bfRAM = ram64K;
		strcpy(v.rgvm[iVM].szModel, rgszVM[2]);
		break;

	case vmAtariXE:
		v.rgvm[iVM].bfHW = vmAtariXE;
		v.rgvm[iVM].iOS = 2;
		v.rgvm[iVM].bfRAM = ram128K;
		strcpy(v.rgvm[iVM].szModel, rgszVM[3]);
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

    return TRUE;
}

// Initialize a VM. Either call this to create a default new instance and then ColdBoot it,
//or call LoadState to restore a previous one, not both.
//
BOOL __cdecl InitAtari(int iVM)
{
//	BYTE bshiftSav; // already a global with that name!

	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

	// initialize our state (doesn't seem to help)
	//_fmemset(vpcandyCur, 0, sizeof(CANDYHW));

	// These things are the same for each machine type

	v.rgvm[iVM].fCPUAuto = TRUE;
	v.rgvm[iVM].bfCPU = cpu6502;
	v.rgvm[iVM].bfMon = monColrTV;
	v.rgvm[iVM].ivdMac = sizeof(v.rgvm[0].rgvd) / sizeof(VD);	// we only have 8, others have 9

	// default to a DOS disk image for a newly created instance
	// loading an empty instance from a file calls LoadState, not Init and won't get this default disk image
	if (v.rgvm[iVM].rgvd[0].dt == DISK_NONE)
	{
		v.rgvm[iVM].rgvd[0].dt = DISK_IMAGE;
		strcpy(v.rgvm[iVM].rgvd[0].sz, "DOS25.XFD");
	}

	// by default, use XL and XE built in BASIC, but no BASIC for Atari 800.
	// unless Shift-F10 changes it
	ramtop = (v.rgvm[iVM].bfHW > vmAtari48) ? 0xA000 : 0xC000;
	wStartScan = STARTSCAN;	// this is when ANTIC starts fetching. Usually 3x "blank 8" means the screen starts at 32.

    // save shift key status, why? !!! this only happens at boot time now until shutdown
    //bshiftSav = *pbshift & (wNumLock | wCapsLock | wScrlLock);
    //*pbshift &= ~(wNumLock | wCapsLock | wScrlLock);
	//*pbshift |= wScrlLock;
	//countInstr = 1;

	switch (v.rgvm[iVM].bfHW)
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

	v.rgvm[iVM].bfRAM = BfFromWfI(v.rgvm[iVM].pvmi->wfRAM, mdXLXE);

	// If our saved state had a cartridge, load it back in
    ReadCart(iVM);

    if (!FInitSerialPort(v.rgvm[iVM].iCOM))
		v.rgvm[iVM].iCOM = 0;
    if (!InitPrinter(v.rgvm[iVM].iLPT))
		v.rgvm[iVM].iLPT = 0;

	// !!! We have at least 3 variables for whether or not to use sound
	fSoundOn = TRUE;
    
//  vi.pbRAM[0] = &rgbMem[0xC000-1];
//  vi.eaRAM[0] = 0;
//  vi.cbRAM[0] = 0xC000;
//  vi.eaRAM[1] = 0;
//  vi.cbRAM[1] = 0xC000;
//  vi.pregs = &rgbMem[0];

    return TRUE;
}

// Call when you destroy the instance. There is no UnInstall
//
BOOL __cdecl UninitAtari(int iVM)
{
    //*pbshift = bshiftSav;

    UninitAtariDisks(iVM);
    return TRUE;
}

BOOL __cdecl MountAtariDisk(int iVM, int i)
{
	UnmountAtariDisk(iVM, i);	// make sure all emulator types know to do this first

    PVD pvd = &v.rgvm[iVM].rgvd[i];

    if (pvd->dt == DISK_IMAGE)
        AddDrive(iVM, i, pvd->sz);

    return TRUE;
}

BOOL __cdecl InitAtariDisks(int iVM)
{
    int i;

    for (i = 0; i < v.rgvm[iVM].ivdMac; i++)
        MountAtariDisk(iVM, i);

    return TRUE;
}

BOOL __cdecl UnmountAtariDisk(int iVM, int i)
{
    PVD pvd = &v.rgvm[iVM].rgvd[i];

		//we can't just unmount if we've asked for a disk image, the whole point is to unmount if we didn't ask for one.
		//if (pvd->dt == DISK_IMAGE)
        DeleteDrive(iVM, i);

    return TRUE;
}

BOOL __cdecl UninitAtariDisks(int iVM)
{
    int i;

    for (i = 0; i < v.rgvm[iVM].ivdMac; i++)
        UnmountAtariDisk(iVM, i);

    return TRUE;
}

// soft reset
//
BOOL __cdecl WarmbootAtari(int iVM)
{
	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

	// tell the CPU which 
	cpuInit(PokeBAtari);

	//OutputDebugString("\n\nWARM START\n\n");
    NMIST = 0x20 | 0x1F;
    regPC = cpuPeekW((mdXLXE != md800) ? 0xFFFC : 0xFFFA);
    cntTick = 50*4;	// delay for banner messages
    QueryTickCtr();
	//countJiffies = 0;

	fBrakes = TRUE;	// go back to real time
	wScan = NTSCY;	// start at top of screen again

	// too slow to do anytime but startup
	//InitSound();	// need to reset and queue audio buffers

	InitJoysticks();	// let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	CaptureJoysticks();

    return TRUE;
}

// Cold Start the machine - the first one is currently done when it first becomes the active instance
//
BOOL __cdecl ColdbootAtari(int iVM)
{
    unsigned addr;
	//OutputDebugString("\n\nCOLD START\n\n");

	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

	InitAtariDisks(iVM);

	cntTick = 50*4;
    QueryTickCtr();
    //countJiffies = 0;
	
	fBrakes = TRUE; // go back to real time
	clockMult = 1;	// per instance speed-up
	wScan = NTSCY;	// start at top of screen again
	wFrame = 0;

    //printf("ColdStart: mdXLXE = %d, ramtop = %04X\n", mdXLXE, ramtop);

#if 0 // doesn't change
    if (mdXLXE == md800)
        v.rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtari48C :*/ vmAtari48;
    else if (mdXLXE == mdXL)
		v.rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtariXLC :*/ vmAtariXL;
    else
		v.rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtariXEC :*/ vmAtariXE;
#endif
    
	//DisplayStatus(); this might not be the active instance

    // Swap in BASIC and OS ROMs.

	// load the OS
    InitBanks(iVM);

	// load the cartridge
	InitCart(iVM);

    // reset the registers, and say which HW it is running
    cpuReset();
	cpuInit(PokeBAtari);

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
        cpuPokeB(addr,0xFF);
    }

    // CTIA/GTIA reset

	// CONSOL won't be valid during cold start. Pass the F9 to an XL as option being held down to remove BASIC
	CONSOL = ((mdXLXE != md800) && (GetKeyState(VK_F9) < 0 || ramtop == 0xC000)) ? 3 : 7;
    PAL = 14;
    TRIG0 = 1;
    TRIG1 = 1;
    TRIG2 = 1;
    TRIG3 = 1;
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

    // PIA reset

	rPADATA = 255;	// the data PORTA will provide if it is in READ mode
	wPADATA = 0;	// the data written to PORTA to be provided in WRITE mode (must default to 0 for Caverns of Mars)
	rPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000) ? 253 : 255) : 0; // XL bit to indicate if BASIC is swapped in
	wPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000) ? 253 : 255) : 0; // XL bit to indicate if BASIC is swapped in
	PACTL  = 60;
    PBCTL  = 60;

	// reset the cycle counter each cold start
	LARGE_INTEGER qpc;
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	vi.qpfCold = qpf.QuadPart;
	
	// seed the random number generator again. The real ATARI is probably seeded by the orientation
	// of sector 1 on the floppy. I wonder how cartridges are unpredictable?
	// don't allow the seed that gets stuck
	do {
		QueryPerformanceCounter(&qpc);
		vi.qpcCold = qpc.QuadPart;	// reset real time
		random17pos = qpc.QuadPart & 0x1ffff;
	} while (random17pos == 0x1ffff);

	//too slow to do anytime but app startup
	//InitSound();	// Need to reset and queue audio buffers
	
	InitJoysticks(); // let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	CaptureJoysticks();

	return TRUE;
}

// SAVE: return a pointer to our data, and how big it is
//
BOOL __cdecl SaveStateAtari(int iVM, char **ppPersist, int *pcbPersist)
{
	*ppPersist = &vrgcandy[iVM];
	*pcbPersist = sizeof(vrgcandy[iVM]);
	return TRUE;
}

// LOAD: copy from the data we've been given. Make sure it's the right size
// Either INIT is called followed by ColdBoot, or this, not both, so do what we need to do to restore our state
// without resetting anything
//
BOOL __cdecl LoadStateAtari(int iVM, char *pPersist, int cbPersist)
{
	if (cbPersist != sizeof(vrgcandy[iVM]))
		return FALSE;

	_fmemcpy(&vrgcandy[iVM], pPersist, cbPersist);
	
	if (!FInitSerialPort(v.rgvm[iVM].iCOM))
		v.rgvm[iVM].iCOM = 0;
	if (!InitPrinter(v.rgvm[iVM].iLPT))
		v.rgvm[iVM].iLPT = 0;

	InitAtariDisks(iVM);

	// If our saved state had a cartridge, load it back in
	ReadCart(iVM);
	InitCart(iVM);

	// too slow to do anytime but app startup
	//InitSound();	// Need to reset and queue audio buffers

	InitJoysticks(); // let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	CaptureJoysticks();

	return TRUE;
}


BOOL __cdecl DumpHWAtari(char *pch)
{
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
       AUDCTL, cpuPeekB(0x87), cpuPeekB(0x88), cpuPeekB(0xA2));

    return TRUE;

    ForceRedraw();
#endif // DEBUG
    return TRUE;
}

// push PC and P on the stack
void Interrupt()
{
    cpuPokeB(regSP, regPC >> 8);  regSP = (regSP-1) & 255 | 256;
    cpuPokeB(regSP, regPC & 255); regSP = (regSP-1) & 255 | 256;
    cpuPokeB(regSP, regP);          regSP = (regSP-1) & 255 | 256;

    regP |= IBIT;
}

BOOL __cdecl TraceAtari()
{
    fTrace = TRUE;
    ExecuteAtari();
    fTrace = FALSE;

    return TRUE;
}

// What happens when it's scan line 241 and it's time to start the VBI
//
DoVBI()
{
	wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off
	wLeftMax = wLeft;

#ifndef NDEBUG
	fDumpHW = 0;
#endif
	if (NMIEN & 0x40) {
		// VBI enabled, generate VBI by setting PC to VBI routine. We'll do a few cycles of it
		// every scan line now until it's done, then resume
		Interrupt();
		NMIST = 0x40 | 0x1F;
		regPC = cpuPeekW(0xFFFA);
	}

	// process joysticks before the vertical blank, just because.
	// Very slow if joysticks not installed, so skip the code
	if (vmCur.fJoystick && vi.rgjc[0].wNumButtons > 0) {
		JOYINFO ji;
		MMRESULT mm = joyGetPos(0, &ji);
		if (mm == 0) {

			int dir = (ji.wXpos - (vi.rgjc[0].wXmax - vi.rgjc[0].wXmin) / 2);
			dir /= (int)((vi.rgjc[0].wXmax - vi.rgjc[0].wXmin) / wJoySens);

			rPADATA |= 12;                  // assume joystick centered

			if (dir < 0)
				rPADATA &= ~4;              // left
			else if (dir > 0)
				rPADATA &= ~8;              // right

			dir = (ji.wYpos - (vi.rgjc[0].wYmax - vi.rgjc[0].wYmin) / 2);
			dir /= (int)((vi.rgjc[0].wYmax - vi.rgjc[0].wYmin) / wJoySens);

			rPADATA |= 3;                   // assume joystick centered

			if (dir < 0)
				rPADATA &= ~1;              // up
			else if (dir > 0)
				rPADATA &= ~2;              // down

			UpdatePorts();

			if (ji.wButtons)
				TRIG0 &= ~1;                // JOY 0 fire button down
			else
				TRIG0 |= 1;                 // JOY 0 fire button up
		}
		if (vi.rgjc[1].wNumButtons > 0)
		{
			mm = joyGetPos(1, &ji);
			if (mm == 0) {

				int dir = (ji.wXpos - (vi.rgjc[1].wXmax - vi.rgjc[1].wXmin) / 2);
				dir /= (int)((vi.rgjc[1].wXmax - vi.rgjc[1].wXmin) / wJoySens);

				rPBDATA |= 12;                  // assume joystick centered

				if (dir < 0)
					rPBDATA &= ~4;              // left
				else if (dir > 0)
					rPBDATA &= ~8;              // right

				dir = (ji.wYpos - (vi.rgjc[1].wYmax - vi.rgjc[1].wYmin) / 2);
				dir /= (int)((vi.rgjc[1].wYmax - vi.rgjc[1].wYmin) / wJoySens);

				rPBDATA |= 3;                   // assume joystick centered

				if (dir < 0)
					rPBDATA &= ~1;              // up
				else if (dir > 0)
					rPBDATA &= ~2;              // down

				UpdatePorts();

				if (ji.wButtons)
					TRIG1 &= ~1;                // JOY 0 fire button down
				else
					TRIG1 |= 1;                 // JOY 0 fire button up
			}
		}
	}

	CheckKey();	// process the ATARI keyboard buffer

	if (fTrace)
		ForceRedraw();	// it might do this anyway

	// every VBI, shadow the hardware registers
	// to their higher locations

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

#if 0 // !!! now we try drawing at scan line 262, not at the start of the VBI and hope it didn't break anything
	if (wScanMin > wScanMac)
	{
		assert(0);
		// screen is not dirty for some reason, so don't render (these variables updated in ProcessScanLine)
	}
	else
	{
		extern void RenderBitmap();
		RenderBitmap();	// tell Gemulator to actually draw the window
	}
	wScanMin = 9999;	// screen not dirty anymore !!! remove these variables?
	wScanMac = 0;
#endif

	// Gem window has a message, stop the loop to process it
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	fStop = fTrue;

	// decrement printer timer

	if (vi.cPrintTimeout && vmCur.fShare)
	{
		vi.cPrintTimeout--;
		if (vi.cPrintTimeout == 0)
		{
			FlushToPrinter();
			UnInitPrinter();
		}
	}
	else
	FlushToPrinter();
}

// The big loop! Do a single horizontal scan line (or if it's time, the vertical blank as scan line 251)
// Either continue until there's a message for our window, or cycle through all instances every frame,
// or if tracing, only do one scan line
//
// !!! fStep and fCont are ignored for now!
BOOL __cdecl ExecuteAtari(BOOL fStep, BOOL fCont)
{
	vpcandyCur = &vrgcandy[v.iVM];	// make sure we're looking at the proper instance

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
				WORD PCt = cpuPeekW(0x200);
				extern void CchDisAsm(WORD *);
				int i;

				printf("\n\nscan = %d, DLPC = %04X, %02X\n", wScan, DLPC,
					cpuPeekB(DLPC));
				for (i = 0; i < 60; i++)
				{
					CchDisAsm(&PCt); printf("\n");
				}
				PCt = regPC;
				for (i = 0; i < 60; i++)
				{
					CchDisAsm(&PCt); printf("\n");
				}
				FDumpHWVM();
			}
#endif // DEBUG

			// next scan line
			wScan = wScan + 1;

			// we process the audio after the whole frame is done, but the VBLANK starts at 241
			if (wScan >= NTSCY)
			{			
				SoundDoneCallback(vi.rgwhdr, SAMPLES_PER_VOICE);	// finish this buffer and send it

				wScan = 0;
				wFrame++;	// count how many frames we've drawn. Does anybody care?

				static ULONGLONG cCYCLES = 0;
				static ULONGLONG cErr = 0;
				static unsigned lastVM = 0;

				// in tiling mode, run all of the instances, and then wait for time to catch up
				if (!v.fTiling || v.iVM <= lastVM) // = if there's only 1 VM
				{
					RenderBitmap();	// !!! used to render in VBI, if compat bugs appear. Try it only when dirty. Use DDraw.
									
					// we're emulating its original speed (fBrakes) so slow down to let real time catch up (1/60th sec)
					// don't let errors propogate
					const ULONGLONG cJif = 29830; // 1789790 / 60
					ULONGLONG cCur = GetCycles() - cCYCLES;

					// report back how long it took
					cEmulationSpeed = cCur * 1000000 / cJif;

					while (fBrakes && ((cJif - cErr) > cCur * clockMult)) {
						Sleep(1);
						cCur = GetCycles() - cCYCLES;
					}
					cErr = cCur - (cJif - cErr);
					if (cErr > cJif) cErr = cJif;	// don't race forever to catch up if game paused, just carry on (also, it's unsigned)
					cCYCLES = GetCycles();
				}
				lastVM = v.iVM;

				// exit each frame to let the next VM run (if Tiling) and to update the clock speed on the status bar (always)
				//if (v.fTiling)
					fStop = fTrue;
			}

			// hack for programs that check $D41B instead of $D40B
			rgbMem[0xD41B] =
				VCOUNT = (BYTE)(wScan >> 1);

			ProcessScanLine();	// do the DLI, and fill the bitmap
			
			// Reinitialize clock cycle counter - number of instructions to run per call to Go6502 (one scanline or so)
			// !!! 30 is right, but DMA on is random from 10-50% slower, so 20 is bogus.
			// 21 should be right for Gr.0 but for some reason 18 is.
			// 30 instr/line * 262 lines/frame * 60 frames/sec * 4 cycles/instr = 1789790 cycles/sec
			wLeft = (fBrakes && (DMACTL & 3)) ? 20 : INSTR_PER_SCAN_NO_DMA;	// runs faster with ANTIC turned off (all approximate)
			wLeftMax = wLeft;	// remember what it started at
			//wLeft *= clockMult;	// any speed up will happen after a frame by sleeping less

			// !!! Everything breaks if I move the VBI away from 251

			// Scan line 0-9 are not visible, 10 lines without DMA
			// Scan lines 10-249 are 240 visible lines
			// Scan line 250 preps NMI, 251 is the VBLANK, without DMA
			// Scan lines 252-261 are the rest of the scan lines, without DMA

			if (wScan < wStartScan) {
				wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off for the first 10 lines
				wLeftMax = wLeft;
			}
			else if (wScan < STARTSCAN + Y8) {	// after all the valid lines have been drawn
				// business as usual
			}
			else if (wScan == STARTSCAN + Y8) {
				// start the NMI status early so that programs
				// that care can see it
				NMIST = 0x40 | 0x1F;

				wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off
				wLeftMax = wLeft;
			}

			// **************************************************************
			// ******************** do the VBI! *****************************
			// **************************************************************
			else if (wScan == STARTSCAN + Y8 + 1)	// was 251
			{
				DoVBI();	// it's huge, it bloats this function to inline it.
			}

			else if (wScan > STARTSCAN + Y8 + 1)
			{
				wLeft = 30;	// for this retrace section, there will be no DMA
				wLeftMax = wLeft;
			}
		} // if wLeft == 0

		// this is, for instance, the key pressed interrupt. VBI and DLIST interrupts are NMI's.
		if (!(regP & IBIT))
		{
			if (IRQST != 255)
			{
				//        printf("IRQ interrupt\n");
				Interrupt();
				regPC = cpuPeekW(0xFFFE);
			}
		}

		// Execute about one horizontal scan line's worth of 6502 code
		Go6502();
		//assert(wLeft == 0 || fTrace == 1);

		if (fSIO) {
			// REVIEW: need to check if PC == $E459 and if so make sure
			// XL/XE ROMs are swapped in, otherwise ignore
			fSIO = 0;
			SIOV();
		}

    } while (!fTrace && !fStop);

    return FALSE;
}


//
// Stubs
//

BOOL __cdecl DumpRegsAtari()
{
    // later

    return TRUE;
}


BOOL __cdecl DisasmAtari(char *pch, ADDR *pPC)
{
    // later

    return TRUE;
}


BYTE __cdecl PeekBAtari(ADDR addr)
{
    // reads always read directly

    return cpuPeekB(addr);
}


WORD __cdecl PeekWAtari(ADDR addr)
{
    // reads always read directly

    return cpuPeekW(addr);
}


ULONG __cdecl PeekLAtari(ADDR addr)
{
    // reads always read directly

    return cpuPeekW(addr);
}


BOOL __cdecl PokeWAtari(ADDR addr, WORD w)
{
    PokeBAtari(addr, w & 255);
    PokeBAtari(addr+1, w >> 8);
    return TRUE;
}


BOOL __cdecl PokeLAtari(ADDR addr, ULONG w)
{
    return TRUE;
}


BOOL __cdecl PokeBAtari(ADDR addr, BYTE b)
{
    BYTE bOld;
    addr &= 65535;
    b &= 255; // just to be sure

    // this is an ugly hack to keep wLeft from getting stuck 
    //if (wLeft > 1)
    //    wLeft--;

#if 0
    printf("write: addr:%04X, b:%02X, PC:%04X\n", addr, b & 255, regPC);
#endif

    if (addr < ramtop)
    {
#if 0
        printf("poking into RAM, addr = %04X, ramtop = %04X\n", addr, ramtop);
        flushall();
#endif

        cpuPokeB(addr, b);
        return TRUE;
    }

	switch ((addr >> 8) & 255)
	{
	default:

		// don't allow writing to cartridge memory, but do allow writing to special extended XL/XE RAM
		if (mdXLXE == md800 ||
			(v.rgvm[v.iVM].rgcart.fCartIn && addr < 0xC000 && addr >= 0xc000 - v.rgvm[v.iVM].rgcart.cbData))
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
                cpuPokeB(addr, b);

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

            DebugStr("HITCLR!!\n");
            }
        else if (addr == 31)
            {
            // CONSOL

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

        if (addr == 10)
        {
            // SKRES

            SKSTAT |=0xE0;
        }
        else if (addr == 13)
        {
            // SEROUT

            IRQST &= ~(IRQEN & 0x18);
        }
        else if (addr == 14)
        {
            // IRQEN

            IRQST |= ~b;
        }
		else if (addr <= 8)
		{
			// AUDFx, AUDCx or AUDCTL have changed - write some sound
			// we're (wScan / 262) of the way through the scan lines and (wLeftmax - wLeft) of the way through this scan line
			int iCurSample = (wScan * 100 + (wLeftMax - wLeft) * 100 / wLeftMax) * SAMPLES_PER_VOICE / 100 / NTSCY;
			if (iCurSample < SAMPLES_PER_VOICE)	// !!! remove once wLeft can't go < 0
				SoundDoneCallback(vi.rgwhdr, iCurSample);
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

            cpuPokeB(PACTLea + (addr & 1), b & 0x3C);
            }
        else
            {
            // it is either a data or ddir register. Check the control bit

            if (cpuPeekB(PACTLea + addr) & 4)
                {
                // it is a data register. Update only bits that are marked as write.

                BYTE bMask = cpuPeekB(wPADDIRea + addr);
                BYTE bOld  = cpuPeekB(wPADATAea + addr);
                BYTE bNew  = (b & bMask) | (bOld & ~bMask);

                if (bOld != bNew)
                    {
                    cpuPokeB(wPADATAea + addr, bNew);

                    if ((addr == 1) && (mdXLXE != md800))
                        {
                        // Writing a new value to PORT B on the XL/XE.

                        if (mdXLXE != mdXE)
                            {
                            // in 800XL mode, mask bank select bits
                            bNew |= 0x7C;
                            wPBDATA |= 0x7C;
                            }

                        SwapMem(bOld ^ bNew, bNew, 0);
                        }
                    }
                }
            else
                {
                // it is a data direction register.

                cpuPokeB(wPADDIRea + addr, b);
                }
            }

        // update PORTA and PORTB read registers

        UpdatePorts();
        break;

    case 0xD4:      // ANTIC
        addr &= 15;
        rgbMem[writeANTIC+addr] = b;

        if (addr == 10)
            {
            // WSYNC

            wLeft = 1;
            }
        else if (addr == 15)
            {
            // NMIRES - reset NMI status

            NMIST = 0x1F;
            }
        break;

    case 0xD5:      // Cartridge bank select
//        printf("addr = %04X, b = %02X\n", addr, b);
        BankCart(v.iVM, addr & 255);
        break;
    }

    return TRUE;
}

void UpdatePorts()
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
void ReadCart(int iVM)
{
	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

	char *pch = v.rgvm[iVM].rgcart.szName;

	int h;
	int cb = 0;
	long l;

	h = _open(pch, _O_BINARY | _O_RDONLY);
	if (h != -1)
	{
#ifndef NDEBUG
		printf("reading %s\n", pch);
#endif

		l = _lseek(h, 0L, SEEK_END);
		cb = min(l, MAX_CART_SIZE);
		// !!! assert or something if the cartridge is too big
		l = _lseek(h, 0L, SEEK_SET);

		//      printf("size of %s is %d bytes\n", pch, cb);
		//      printf("pb = %04X\n", rgcart[iCartMac].pbData);
		
		_read(h, rgbSwapCart[iVM], cb);

		v.rgvm[iVM].rgcart.cbData = cb;
		v.rgvm[iVM].rgcart.fCartIn = TRUE;
	}
	_close(h);
}

// SWAP in the cartridge
//
void InitCart(int iVM)
{
	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

	// no cartridge
	if (!(v.rgvm[iVM].rgcart.fCartIn))
	{
		// convenience for Atari 800, we can ask for BASIC to be put in
		if (v.rgvm[iVM].bfHW == vmAtari48 && ramtop == 0xA000)
		{
			_fmemcpy(&rgbMem[0xA000], rgbXLXEBAS, 8192);
			ramtop = 0xA000;
		}
		else
		{
			// otherwise, the OS may try to run code for a cartridge that isn't there
		// (zeroing all of memory would also work, but be slower)
			rgbMem[0x9FFC] = 0x01;	// tell the OS there is no regular cartridge B inserted
			rgbMem[0xBFFD] = 0x00;	// tell the OS there is no special cartridge A inserted
			rgbMem[0xBFFC] = 0x01;	// tell the OS there is no regular cartridge A inserted
		}
		return;
	}

	PCART pcart = &(v.rgvm[iVM].rgcart);
	unsigned int cb = pcart->cbData;

	//printf("pcart = %08X\n", pcart);
	//printf("pb    = %08X\n", pcart->pbData);

//	if (ramtop == 0xC000)
//		return;

	if (cb <= 8192)
	{
		// simple 4K or 8K case
		// copy entire pages at a time
		_fmemcpy(&rgbMem[0xC000 - (((pcart->cbData + 4095) >> 12) << 12)], rgbSwapCart[iVM], (((pcart->cbData + 4095) >> 12) << 12));
		ramtop = 0xA000;
		return;
	}

	if (cb == 16384)
	{
		// Super cart?? !!! This is broken right now

		unsigned int i;

		BYTE FAR *pb = rgbSwapCart[iVM];

		for (i = 0; i != cb; i += 4096)
		{
			if ((pb[i + 4095] >= 0xA0) && (pb[i + 4095] <= 0xBF))
			{
				// This is the 'fixed' bank

				_fmemcpy(&rgbMem[0xB000], pb + i, 4096);
				//printf("fixed bank = %04X\n", i);
				//gets("  ");
			}

			if (pb[i + 4095] == 0x00)
			{
				// This is the default bank

				_fmemcpy(&rgbMem[0xA000], pb + i, 4096);
				//printf("default bank = %04X\n", i);
				//gets("  ");
			}
		}
		ramtop = 0xA000;
	}
}

// !!! This is broken. Work with unbanked and banked 16K carts
//
void BankCart(int iVM, int iBank)
{
	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

	PCART pcart = &(v.rgvm[iVM].rgcart);
	unsigned int cb = pcart->cbData;

	//    printf("bank select = %d\n", iBank);

	if (ramtop == 0xC000)
		return;

	if (!(v.rgvm[iVM].rgcart.fCartIn))
		return;

	if (cb <= 8192)
	{
		return;
	}

	if (cb == 16384)
	{
		// Super cart??

		unsigned int i;

		BYTE FAR *pb = rgbSwapCart[iVM];

		for (i = 0; i != cb; i += 4096)
		{
			if (pb[i + 4095] == i)
			{
				// This is the selected bank

				_fmemcpy(&rgbMem[0xA000], pb + i, 4096);
			}
			else
			{
			}
		}
	}
}
#endif // XFORMER