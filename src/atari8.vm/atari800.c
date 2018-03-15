
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
    };


//
// Globals
//

CANDYHW vrgcandy[MAXOSUsable];

CANDYHW *vpcandyCur = &vrgcandy[0];

//BYTE bshiftSav;	// was a local too

WORD fBrakes;        // 0 = run as fast as possible, 1 = slow down
static signed short wLeftMax;

#define INSTR_PER_SCAN_NO_DMA 30

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

// not called when loading from disk, only when creating one from scratch
//
BOOL __cdecl InstallAtari(PVMINFO pvmi, PVM pvm)
{
    // initialize the default Atari 8-bit VM

    pvm->pvmi = pvmi;
    pvm->bfHW = vmAtari48;
    pvm->bfCPU = cpu6502;
    pvm->iOS = 0;
    pvm->bfMon = monColrTV;
    pvm->bfRAM  = ram48K;
    pvm->ivdMac = sizeof(v.rgvm[0].rgvd)/sizeof(VD);	// we only have 8, others have 9

    pvm->rgvd[0].dt = DISK_IMAGE;

    strcpy(pvm->rgvd[0].sz, "DOS25.XFD");	// broken default probably won't get used anyway

    // fake in the Atari 8-bit OS entires

    if (v.cOS < 3)
        {
        v.rgosinfo[v.cOS++].osType  = osAtari48;
        v.rgosinfo[v.cOS++].osType  = osAtariXL;
        v.rgosinfo[v.cOS++].osType  = osAtariXE;
        }

    pvm->fValidVM = fTrue;

 // DumpROMS();

    return TRUE;
}

// Initialize a VM... not necessarily the current one
//
BOOL __cdecl InitAtari(int iVM)
{
    static BOOL fInited;
//	BYTE bshiftSav; // already a global with that name!

#if 0
	// reset the cycle counter now and each cold start (we need to use it now, before the 1st cold start)
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	vi.qpcCold = qpc.QuadPart;	// reset real time
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	vi.qpfCold = qpf.QuadPart;
#endif

	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

    // save shift key status, why? !!! this only happens at boot time now until shutdown
    //bshiftSav = *pbshift & (wNumLock | wCapsLock | wScrlLock);
    //*pbshift &= ~(wNumLock | wCapsLock | wScrlLock);
	//*pbshift |= wScrlLock;
    
	//countInstr = 1;

    if (!fInited) // static, only 1 instance needs to do this
    {
        extern void * jump_tab[512];
        extern void * jump_tab_RO[256];
        unsigned i;

        for (i = 0 ; i < 256; i++)
        {
            jump_tab[i] = jump_tab_RO[i];
        }

        wStartScan = 10;

        fInited = TRUE;
    }

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

    ReadROMs();
	ReadCart(iVM);

    if (!FInitSerialPort(v.rgvm[iVM].iCOM))
		v.rgvm[iVM].iCOM = 0;
    if (!InitPrinter(v.rgvm[iVM].iLPT))
		v.rgvm[iVM].iLPT = 0;

	fSoundOn = TRUE;
    
//  vi.pbRAM[0] = &rgbMem[0xC000-1];
//  vi.eaRAM[0] = 0;
//  vi.cbRAM[0] = 0xC000;
//  vi.eaRAM[1] = 0;
//  vi.cbRAM[1] = 0xC000;
//  vi.pregs = &rgbMem[0];

    return TRUE;
}

BOOL __cdecl UninitAtari(int iVM)
{
    //*pbshift = bshiftSav;

    UninitAtariDisks(iVM);
    return TRUE;
}


BOOL __cdecl MountAtariDisk(int iVM, int i)
{
    PVD pvd = &v.rgvm[iVM].rgvd[i];

    if (pvd->dt == DISK_IMAGE)
        AddDrive(i, pvd->sz);

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
        DeleteDrive(i);

    return TRUE;
}


BOOL __cdecl UninitAtariDisks(int iVM)
{
    int i;

    for (i = 0; i < v.rgvm[iVM].ivdMac; i++)
        UnmountAtariDisk(iVM, i);

    return TRUE;
}

// can only happen to the current instance (I hope)
//
BOOL __cdecl WarmbootAtari()
{
	// POKE 580,1 == Cold Start
	if (rgbMem[0x244])
		ColdbootAtari();

	//OutputDebugString("\n\nWARM START\n\n");
    NMIST = 0x20 | 0x1F;
    regPC = cpuPeekW((mdXLXE != md800) ? 0xFFFC : 0xFFFA);
    cntTick = 50*4;	// delay for banner messages
    QueryTickCtr();
	//countJiffies = 0;
	fBrakes = TRUE;	// back to real time

	InitSound();	// need to reset and queue audio buffers

	ReleaseJoysticks();	// let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	InitJoysticks();
	CaptureJoysticks();

    return TRUE;
}

BOOL __cdecl ColdbootAtari(int iVM)
{
    unsigned addr;
	//OutputDebugString("\n\nCOLD START\n\n");

	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

    // Initialize mode display counter (banner)
	
	// XL and XE have built in BASIC
	ramtop = (v.rgvm[iVM].bfHW > vmAtari48) ? 0xA000 : 0xC000;

	fBrakes = 1;	// global turbo for all instances
	clockMult = 1;	// per instance speed-up

	InitAtariDisks(iVM);

	cntTick = 50*4;
    QueryTickCtr();
    //countJiffies = 0;
	
	fBrakes = TRUE; // go back to real time

    //printf("ColdStart: mdXLXE = %d, ramtop = %04X\n", mdXLXE, ramtop);

    if (mdXLXE == md800)
        v.rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtari48C :*/ vmAtari48;
    else if (mdXLXE == mdXL)
		v.rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtariXLC :*/ vmAtariXL;
    else
		v.rgvm[iVM].bfHW = /* (ramtop == 0xA000) ? vmAtariXEC :*/ vmAtariXE;
    //DisplayStatus(); this might not be the active instance

    // Swap in BASIC and OS ROMs.

    InitBanks();
	InitCart(iVM);	// after the OS and BASIC go in above, so the cartridge overrides built-in BASIC

#if 0 // doesn't the OS do this?
    // initialize memory up to ramtop

    for (addr = 0; addr < ramtop; addr++)
        {
        cpuPokeB(addr,0xAA);
        }
#endif

    // initialize the 6502

    cpuInit(PokeBAtari);

    // initialize hardware
    // NOTE: at $D5FE is ramtop and then the jump table

    for (addr = 0xD000; addr < 0xD5FE; addr++)
        {
        cpuPokeB(addr,0xFF);
        }

    // CTIA/GTIA

	// CONSOL not read yet.
	CONSOL = ((mdXLXE != md800) && (GetKeyState(VK_F9) < 0 || ramtop == 0xC000)) ? 3 : 7; // let an XL detect basic or not
    PAL = 14;
    TRIG0 = 1;
    TRIG1 = 1;
    TRIG2 = 1;
    TRIG3 = 1;
    *(ULONG *)MXPF  = 0;
    *(ULONG *)MXPL  = 0;
    *(ULONG *)PXPF  = 0;
    *(ULONG *)PXPL  = 0;

    // POKEY

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

    // ANTIC

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

    // PIA

	rPADATA = 255;
	rPBDATA = 255;
	wPADATA = 255;
    wPBDATA = (ramtop == 0xC000) ? 255: 253;	// !!!
    PACTL  = 60;
    PBCTL  = 60;

	// reset the cycle counter each cold start
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	vi.qpcCold = qpc.QuadPart;	// reset real time
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	vi.qpfCold = qpf.QuadPart;

	// seed the random number generator so it's different every time, the real ATARI is probably seeded by the orientation
	// of sector 1 on the floppy
	RANDOM17 = qpc.QuadPart & 0x1ffff;

	InitSound();	// Need to reset and queue audio buffers
	ReleaseJoysticks();	// let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
	InitJoysticks();
	CaptureJoysticks();

	return TRUE;
}

BOOL __cdecl DumpHWAtari(char *pch)
{
#ifndef NDEBUG
//    RestoreVideo();

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

#ifdef HDOS16ORDOS32
    _bios_keybrd(_NKEYBRD_READ);
    SaveVideo();
#endif
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


BOOL fDumpHW;

// The big loop! Do a single horizontal scan line (or if it's time, the vertical blank as scan line 251)
// Either continue in a loop, or only do one line at a time for tracing

BOOL __cdecl ExecuteAtari()
{
	vpcandyCur = &vrgcandy[v.iVM];	// make sure we're looking at the proper instance

	fStop = 0;

	do {
		// this should happen every cycle, not every 30 instructions, but we do it here to make sure it happens
		// fairly often on its own, plus just before it is read to make sure it's never the same twice
		RANDOM17 = ((RANDOM17 >> 1) | ((~((~RANDOM17) ^ ~(RANDOM17 >> 5)) & 0x01) << 16)) & 0x1FFFF;
		RANDOM = RANDOM17 & 0xff;

		// when tracing, we only do 1 instruction per call, so there are some left over
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

			// next scan line - should really be 262.5, ugh, sigh, NTSC.
			wScan = wScan + 1;

			// we process the audio after the whole frame is done, but video during the scan line the VBLANK starts at (241)
			if (wScan >= 262)
			{
				SoundDoneCallback(vi.rgwhdr, SAMPLES_PER_VOICE);	// finish this buffer and send it
				wScan = 0;
				wFrame++;

				static ULONGLONG cCYCLES = 0;
				static ULONGLONG cErr = 0;

				// we're emulating its original speed (fBrakes) so slow down to let real time catch up (1/60th sec)
				const ULONGLONG cJif = 29830; // 1789790 / 60
				ULONGLONG cCur = GetCycles() - cCYCLES;
				while (fBrakes && ((cJif - cErr) > cCur * clockMult)) {
					Sleep(1);
					cCur = GetCycles() - cCYCLES;
				}
				cErr = cCur - (cJif - cErr);
				if (cErr > cJif) cErr = cJif;	// don't race forever to catch up if game paused, just carry on (also, it's unsigned)
				cCYCLES = GetCycles();

				// exit each frame to let the next VM run
				if (v.fTiling)
					fStop = fTrue;
			}

			// hack for programs that check $D41B instead of $D40B
			rgbMem[0xD41B] =
				VCOUNT = (BYTE)(wScan >> 1);

			ProcessScanLine(1); // renders the previous scan line
			ProcessScanLine(0);	// prepare for new scan line, and do the DLI

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
			else if (wScan < 250) {
				// business as usual
			}
			else if (wScan == 250) {
				// start the NMI status early so that programs
				// that care can see it
				NMIST = 0x40 | 0x1F;

				wLeft = INSTR_PER_SCAN_NO_DMA;	// DMA should be off
				wLeftMax = wLeft;
			}

			// do the VBI!
			else if (wScan == 251)
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

				// process joysticks before the vertical blank, just because
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

				MSG msg;

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

				// Gem window has a message, stop the loop to process it
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
			else if (wScan > 251)
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

    switch((addr >> 8) & 255)
        {
    default:
        if (mdXLXE == md800)
            break;

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
			int iCurSample = (wScan * 100 + (wLeftMax - wLeft) * 100 / wLeftMax) * SAMPLES_PER_VOICE / 100 / 262;
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

void ReadCart(int iVM)
{
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
		
		_read(h, rgcartData, cb);

		v.rgvm[iVM].rgcart.cbData = cb;
		v.rgvm[iVM].rgcart.fCartIn = TRUE;
	}
	_close(h);
}

void InitCart(int iVM)
{
	// no cartridge
	if (!(v.rgvm[iVM].rgcart.fCartIn))
		return;

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
		_fmemcpy(&rgbMem[0xC000 - (((pcart->cbData + 4095) >> 12) << 12)], rgcartData, (((pcart->cbData + 4095) >> 12) << 12));
		ramtop = 0xA000;	// probably, right?
		return;
	}

	if (cb == 16384)
	{
		// Super cart??

		unsigned int i;

		BYTE FAR *pb = rgcartData;

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


void BankCart(int iVM, int iBank)
{
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

		BYTE FAR *pb = rgcartData;

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

