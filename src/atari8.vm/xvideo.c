
/***************************************************************************

    XVIDEO.C

    - Atari 8-bit video emulation

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    09/27/2000  darekm      last update

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER


const WORD mpsizecw[4] = { 1, 2, 1, 4 };

const WORD mppmbw[5] =
    {
    (bfPM0 << 8) | bfPM0,
    (bfPM1 << 8) | bfPM1,
    (bfPM2 << 8) | bfPM2,
    (bfPM3 << 8) | bfPM3,
    (bfPF3 << 8) | bfPF3,
    };

BYTE const * const rgszModes[6] =
    {
    (BYTE const * const )" ATARI 800 (NO CARTRIDGE)",
	(BYTE const * const)" ATARI 800XL (NO CARTRIDGE)",
	(BYTE const * const)" ATARI 130XE (NO CARTRIDGE)",
	(BYTE const * const)" ATARI 800 + CARTRIDGE",
	(BYTE const * const)" ATARI 800XL + CARTRIDGE",
	(BYTE const * const)" ATARI 130XE + CARTRIDGE",
    };

#define vcbScan X8
#define wcScans Y8	// # of valid scan lines
#define NTSCx 512
#define NARROWx 256
#define NORMALx 320
#define WIDEx 384


void ShowCountDownLine(int iVM)
{
    int i = wScan - (wStartScan + wcScans - 12);

    // make sure bottom 8 scan lines

    if (i < 8 && i >= 0)
        {
        char *pch;
        int cch;
        BYTE *qch = vrgvmi[iVM].pvBits;
        BYTE colfg;

		if (cntTick < 2)
			pch = "";
		else if (cntTick < 80)
			pch = "F6 - F10 FOR HELP START SEL OPT RESET";
        else if (cntTick < 156)
            pch = " USE CTRL + ARROWS FOR JOYSTICK";
        else if (cntTick < 206)
            pch = " XFORMER BY DAREK MIHOCKA";
        else
            pch = (char *)rgszModes[mdXLXE + ((ramtop == 0xC000) ? 0 : 3)];

        qch += (wScan - wStartScan) * vcbScan;

        colfg = (BYTE)(((wFrame >> 2) << 4) + i + i);
        colfg = (BYTE)((((wFrame >> 2) + i) << 4) + 8);

        for (cch = 0; cch < X8 >> 3; cch++)
            {
            BYTE b = *pch ? *pch - ' ' : 0;

            b = rgbAtariOSB[0x800 + (b << 3) + i];

            qch[0] = (b & 0x80) ? colfg : 0x00;
            qch[1] = (b & 0x40) ? colfg : 0x00;
            qch[2] = (b & 0x20) ? colfg : 0x00;
            qch[3] = (b & 0x10) ? colfg : 0x00;
            qch[4] = (b & 0x08) ? colfg : 0x00;
            qch[5] = (b & 0x04) ? colfg : 0x00;
            qch[6] = (b & 0x02) ? colfg : 0x00;
            qch[7] = (b & 0x01) ? colfg : 0x00;
            qch += 8;

            if (*pch)
                pch++;
            }

		if ((i == 7) && (cntTick == 1))
		{
			//memset(rgsl, 1, sizeof(rgsl));
		}

        if (i == 7)
            cntTick--;

		// !!! not implemented attempt to see if dirty
        if ((wScan >= wStartScan) && (wScan < (wStartScan+wcScans)))
            {
            wScanMin = min(wScanMin, (wScan - wStartScan));
            wScanMac = max(wScanMac, (wScan - wStartScan) + 1);
            }
        }
}

// Each PM pixel is 2 screen pixels wide, so we treat the screen RAM as an array of WORDS instead of BYTES
//
void DrawPlayers(int iVM, WORD NEAR *pw)
{
	int i;
	int j;
	int cw;
	BYTE b2;
	WORD w;

	// first, fill the bit array with all the players located at each spot
	for (i = 0; i < 4; i++)
	{
		WORD NEAR *qw;

		qw = pw;
		qw += (pmg.hposp[i] - ((NTSCx - X8) >> 2));

		// no PM data in this pixel
		b2 = pmg.grafp[i];
		if (!b2)
			continue;

		cw = mpsizecw[pmg.sizep[i] & 3];

		w = mppmbw[i];

		// look at each bit of Player data
		for (j = 0; j < 8; j++)
		{
			if (b2 & 0x80)
			{
				qw[0] |= w;
				if (cw > 1)
				{
					qw[1] |= w;
					if (cw > 2)
					{
						qw[2] |= w;
						qw[3] |= w;
					}
				}
			}

			qw += cw;
			b2 <<= 1;
		}
	}

	// now do player-to-player and player-to-playfield collisions

	if (pmg.fHitclr)
		return;

	// loop through all players
	for (i = 0; i < 4; i++)
	{
		// GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
		BOOL fHiRes = (sl.modelo == 2 || sl.modelo == 3 || sl.modelo == 15);

        WORD NEAR *qw;
        
		b2 = pmg.grafp[i];
		if (!b2)
            continue;

        qw = pw;
		qw += (pmg.hposp[i] - ((NTSCx - X8) >> 2));

        cw = mpsizecw[ pmg.sizep[i] & 3];

		// loop through all bits of a player, which take up 2 high res playfield bits
        for (j = 0; j < 8; j++)
        {
			// Bit 8-j has this player present... is another player or a PF colour present too?
            if (b2 & 0x80)
                {
                PXPL[i] |= (*qw >> 12);
				PXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;

				// double sized player, check the next bit too
                if (cw > 1)
                    {
                    PXPL[i] |= (*qw >> 12);
                    PXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;

					// quad sized player, check the next 2 bits too
                    if (cw > 2)
                        {
                        PXPL[i] |= (*qw >> 12);
						PXPF[i] |= fHiRes ? (((*qw++) & 0x02)) << 1 : *qw++;
						PXPL[i] |= (*qw >> 12);
						PXPF[i] |= fHiRes ? (((*qw++) & 0x02)) << 1 : *qw++;
					}
                    }
                }
            else
                qw += cw;

            b2 <<= 1;
        }

        // mask out upper 4 bits of each collision register
        // and mask out same player collisions

        *(ULONG *)PXPF &= 0x0F0F0F0F;
        *(ULONG *)PXPL &= 0x070B0D0E;
        }
}

#if _MSC_VER >= 1200
// VC 6.0 optimization bug having to do with (byte >> int)
#pragma optimize("",off)
#endif

void DrawMissiles(int iVM, WORD NEAR *pw, int fFifth)
{
    int i;
    int cw;
    BYTE b2;
    WORD w;

    if (pmg.fHitclr)
        goto Ldm;

    // First do missile-to-player and missile-to-playfield collisions

    for (i = 0; i < 4; i++)
    {
		// GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
		BOOL fHiRes = (sl.modelo == 2 || sl.modelo == 3 || sl.modelo == 15);
		
		WORD NEAR *qw;
        
		b2 = (pmg.grafm >> (i + i)) & 3;
		if (!b2)
            continue;

        qw = (WORD NEAR *)pw;
        qw += (pmg.hposm[i] - ((NTSCx-X8)>>2));

        cw = mpsizecw[((pmg.sizem >> (i+i))) & 3];

        if (b2 & 2)
            {
            MXPL[i] |= (*qw >> 12);
			MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;

            if (cw > 1)
                {
                MXPL[i] |= (*qw >> 12);
				MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;

                if (cw > 2)
                    {
                    MXPL[i] |= (*qw >> 12);
					MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;
					MXPL[i] |= (*qw >> 12);
					MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;
				}
                }
            }
        else
            qw += cw;

        if (b2 & 1)
            {
            MXPL[i] |= (*qw >> 12);
			MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;

            if (cw > 1)
                {
                MXPL[i] |= (*qw >> 12);
				MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;

                if (cw > 2)
                    {
                    MXPL[i] |= (*qw >> 12);
					MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;
					MXPL[i] |= (*qw >> 12);
					MXPF[i] |= fHiRes ? (((*qw++) & 0x02) << 1) : *qw++;
				}
                }
            }
    }

    // mask out upper 4 bits of each collision register

    *(ULONG *)MXPL &= 0x0F0F0F0F;
    *(ULONG *)MXPF &= 0x0F0F0F0F;

Ldm:
    for (i = 0; i < 4; i++)
    {
        WORD NEAR *qw;
        
		b2 = (pmg.grafm >> (i + i)) & 3;
		if (!b2)
            continue;

        qw = (WORD NEAR *)pw;
        qw += (pmg.hposm[i] - ((NTSCx-X8) >> 2));

        cw = mpsizecw[((pmg.sizem >> (i+i))) & 3];

        if (fFifth)
            w  = mppmbw[4];
        else
            w  = mppmbw[i];

        if (b2 & 2)
        {
            if (1 || fFifth)	// !!! 5th player broken
            {
                qw[0] |= w;
                if (cw > 1)
                {
                    qw[1] |= w;
                    if (cw > 2)
                    {
                        qw[2] |= w;
                        qw[3] |= w;
                    }
                }
            }
        }
        qw += cw;

        if (b2 & 1)
        {
            if (1 || fFifth)	// !!! 5th player probably broken!
            {
                qw[0] |= w;
                if (cw > 1)
                {
                    qw[1] |= w;
                    if (cw > 2)
                    {
                        qw[2] |= w;
                        qw[3] |= w;
                    }
                }
            }
        }
    }
}

#pragma optimize("",on)

__inline void IncDLPC(int iVM)
{
    DLPC = (DLPC & 0xFC00) | ((DLPC+1) & 0x03FF);
}

// THEORY OF OPERATION
//
// scans is the number of scan lines this graphics mode has, minus 1. So, 7 for GR.0
// iscan is the current scan line being drawn of this graphics mode line, 0 based
//		iscan == scans means you're on the last scan line (time for DLI?)
// fWait means we're doing a JVB (jump and wait for VB)
// fFetch means we finished an ANTIC instruction and are ready for a new one
// cbDisp is the width of the playfield in bytes (could be narrow, normal or wide)
// cbWidth is boosted to the next size if horiz scrolling
// wAddr is the start of screen memory set by a LMS (load memory scan)

BOOL ProcessScanLine(int iVM)
{
	BYTE rgpix[NTSCx];	// an entire NTSC scan line, including retrace areas

	// scan lines per line of each graphics mode
    static const WORD mpMdScans[19] =
        {
        1, 1, 8, 10, 8, 16, 8, 16, 8, 4, 4, 2, 1, 2, 1, 1, 1, 1, 1
        };

	// BYTES per line of each graphics mode for NARROW playfield
    static const WORD mpMdBytes[19] =
        {
        0, 0, 32, 32, 32, 32, 16, 16,	8, 8, 16, 16, 16, 32, 32, 32, 32, 32, 32
        };

    // table that says how many bits of data to shift for each
    // 2 clock horizontal delay

    static const WORD mpmdbits[19] =
        {
        0, 0, 4, 4, 4, 4, 2, 2,    1, 1, 2, 2, 2, 4, 4, 4, 4, 4, 4
        };

    int i, j;
	
	// !!! used to be static! Not even used, but I'm scared to remove it
    WORD rgfChanged = 0;

	// don't do anything in the invisible top and bottom sections
	if (wScan < wStartScan || wScan >= wStartScan + wcScans)
		return 0;

	// we are entering visible territory (240 lines are visible - usually 24 blank, 192 created by ANTIC, 24 blank)
    if (wScan == wStartScan)
    {
        fWait = 0;	// not doing JVB
        fFetch = 1;	// start by grabbing a DLIST instruction
	}

	// grab a DLIST instruction - not in JVB, done previous instruction, and ANTIC is on.
    if (!fWait && fFetch && (DMACTL & 32))
    {
#ifndef NDEBUG
        extern BOOL  fDumpHW;

        if (fDumpHW)
            printf("Fetching DL byte at scan %d, DLPC=%04X, byte=%02X\n",
                 wScan, DLPC, cpuPeekB(iVM, DLPC));
#endif

        sl.modehi = cpuPeekB(iVM, DLPC);
        sl.modelo = sl.modehi & 0x0F;
        sl.modehi >>= 4;
        IncDLPC(iVM);

		// vertical scroll bit enabled
        if ((sl.modehi & 2) && !sl.fVscrol && (sl.modelo >= 2))
            {
            sl.fVscrol = TRUE;
            sl.vscrol = VSCROL & 15;
			iscan = sl.vscrol;	// start displaying at this scan line. If > # of scan lines per mode line (say, 14)
								// we'll see parts of this mode line twice. We'll count up to 15, then back to 0 and
								// up to sscans. The last scan line is always when iscan == sscans.
								// The actual scan line drawn will be iscan % (number of scan lines in that mode line)
            }
        else
            {
            iscan = 0;
            sl.vscrol = 0;
            }

        switch (sl.modelo)
            {
        case 0:
            // display list blank line(s) instruction

            scans = (sl.modehi & 7);	// scans = 1 less than the # of blank lines wanted
            break;

        case 1:
            // display list jump instruction

            scans = 0;
            fWait = (sl.modehi & 4);	// JVB or JMP?
			
			if (fWait)
				fWait |= (sl.modehi & 8);	// a DLI on a JVB keeps firing every line

            {
            WORD w;

            w = cpuPeekB(iVM, DLPC);
            IncDLPC(iVM);
            w |= (cpuPeekB(iVM, DLPC) << 8);

            DLPC = w;
            }
            break;

        default:
            // normal display list entry

			// how many scan lines a line of this graphics mode takes
            scans = (BYTE)mpMdScans[sl.modelo]-1;

			// LMS (load memory scan) attached to this line to give start of screen memory
            if (sl.modehi & 4)
                {
                wAddr = cpuPeekB(iVM, DLPC);
                IncDLPC(iVM);
                wAddr |= (cpuPeekB(iVM, DLPC) << 8);
                IncDLPC(iVM);
                }
            break;
            }

		// time to stop vscrol, this line doesn't use it
        if (sl.fVscrol && (!(sl.modehi & 2) || (sl.modelo < 2)))
            {
            sl.fVscrol = FALSE;
            scans = VSCROL;	// the first mode line after a VSCROL area is truncated, ending at VSCROL
            }
    }

    // generate a DLI if necessary. Do so on the last scan line of a graphics mode line
	// a JVB instruction with DLI set keeps firing them all the way to the VBI
    if ((sl.modehi & 8) && (iscan == scans) || ((fWait & 0x08) && wScan ))
    {
#ifndef NDEBUG
        extern BOOL  fDumpHW;

        if (fDumpHW)
            printf("DLI interrupt at scan %d\n", wScan);
#endif

		// set DLI, clear VBI leave RST alone - even if we don't take the interrupt
		NMIST = (NMIST & 0x20) | 0x9F;
		if (NMIEN & 0x80)	// DLI enabled
		{
			Interrupt(iVM, FALSE);
			regPC = cpuPeekW(iVM, 0xFFFA);
		}
    }

    // Check playfield width and set cbWidth (number of bytes read by Antic)
    // and the smaller cbDisp (number of bytes actually visible)

    switch (DMACTL & 3)
    {
    default:
        Assert(FALSE);
        break;

	// cbDisp - actual number of bytes per line of this graphics mode visible
	// cbWidth - boosted number of bytes read by ANTIC if horizontal scrolling (narrow->normal, normal->wide)

    case 0:	// playfield off
        sl.modelo = 0;
        cbWidth = 0;
        cbDisp = cbWidth;
        break;
    case 1: // narrow playfield
        cbWidth = mpMdBytes[sl.modelo];		// cbDisp: use NARROW number of bytes per graphics mode line, eg. 32
        cbDisp = cbWidth;
        if ((sl.modehi & 1) && (sl.modelo >= 2)) // hor. scrolling, cbWidth mimics NORMAL
            cbWidth |= (cbWidth >> 2);
        break;
    case 2: // normal playfield
        cbWidth = mpMdBytes[sl.modelo];		// bytes in NARROW mode, eg. 32
        cbDisp = cbWidth | (cbWidth >> 2);	// cbDisp: boost to get bytes per graphics mode line in NORMAL mode, eg. 40
        if ((sl.modehi & 1) && (sl.modelo >= 2)) // hor. scrolling?
            cbWidth |= (cbWidth >> 1);			// boost cbWidth to mimic wide playfield, eg. 48
        else
            cbWidth |= (cbWidth >> 2);			// otherwise same as cbDisp
        break;
    case 3: // wide playfield
        cbWidth = mpMdBytes[sl.modelo];		// NARROW width
        cbDisp = cbWidth | (cbWidth >> 2) | (cbWidth >> 3);	// visible area is half way between NORMAL and WIDE
        cbWidth |= (cbWidth >> 1);			// WIDE width
        break;
    }

	// first scan line of a mode line, use the correct char set for this graphics mode line?
    // !!! it's never too late, but the check will help find bugs by making the whole character bad not just the top row
	//if (iscan == sl.vscrol)
    {
        sl.chbase = CHBASE & ((sl.modelo < 6) ? 0xFC : 0xFE);
        sl.chactl = CHACTL & 7;
    }

    // fill in current sl structure

    sl.scan   = iscan;
    sl.addr   = wAddr;
    sl.dmactl = DMACTL;

    sl.prior  = PRIOR;

	// Horiz scrolling
    if (((sl.modelo) >= 2) && (sl.modehi & 1))
    {
        sl.hscrol = HSCROL & 15;
		// # of bits to shift the screen byte in CHAR modes
		// in MAP modes, every 8 of these means you've shifted an entire screen byte
		// even though you don't do it by shifting the bits in the screen byte
		hshift = (sl.hscrol * mpmdbits[sl.modelo]) >> 1;
	}
    else
    {
        sl.hscrol = 0;
        hshift = 0;
    }

    pmg.hposmX = HPOSMX;
    pmg.hpospX = HPOSPX;
    pmg.sizem  = SIZEM;
    pmg.sizepX = SIZEPX;

    if (sl.dmactl & 0x10)
        pmg.pmbase = PMBASE & 0xF8;
    else
        pmg.pmbase = PMBASE & 0xFC;

    // enable PLAYER DMA and enable players?
    if (sl.dmactl & 0x08 && GRACTL & 2)
    {
		// single line resolution
        if (sl.dmactl & 0x10)
            {
            pmg.grafp0 = cpuPeekB(iVM, (pmg.pmbase << 8) + 1024 + wScan);
            pmg.grafp1 = cpuPeekB(iVM, (pmg.pmbase << 8) + 1280 + wScan);
            pmg.grafp2 = cpuPeekB(iVM, (pmg.pmbase << 8) + 1536 + wScan);
            pmg.grafp3 = cpuPeekB(iVM, (pmg.pmbase << 8) + 1792 + wScan);
            }
		// double line resolution
        else
            {
            pmg.grafp0 = cpuPeekB(iVM, (pmg.pmbase << 8) + 512 + (wScan >>1));
            pmg.grafp1 = cpuPeekB(iVM, (pmg.pmbase << 8) + 640 + (wScan >>1));
            pmg.grafp2 = cpuPeekB(iVM, (pmg.pmbase << 8) + 768 + (wScan >>1));
            pmg.grafp3 = cpuPeekB(iVM, (pmg.pmbase << 8) + 896 + (wScan >>1));
            }
        // !!! GRAFPX = pmg.grafpX;
        }

	// we want players, but a constant value, not memory lookup. GRACTL need not be set.
	else
		pmg.grafpX = GRAFPX;

	// enable MISSILE DMA and enable missiles? (enabling players enables missiles too)
	if ((sl.dmactl & 0x04 || sl.dmactl & 0x08) && GRACTL & 1)
	{
		// single res
		if (sl.dmactl & 0x10)
			pmg.grafm = cpuPeekB(iVM, (pmg.pmbase << 8) + 768 + wScan);
		// double res
		else
			pmg.grafm = cpuPeekB(iVM, (pmg.pmbase << 8) + 384 + (wScan >> 1));

		//GRAFM = pmg.grafm;
	}

	// we want missiles, but a constant value, not memory lookup. GRACTL does NOT have to be set!
	else
		pmg.grafm = GRAFM;
	
    if (pmg.grafpX)
        {
        // Players that are offscreen are treated as invisible
		// the limits are 40 and 216, but a quad sized player could be 32 wide
		// probably not worth looking at size to find actual width?
        if ((pmg.hposp0 < 8) || (pmg.hposp0 >= 216))
            pmg.grafp0 = 0;
        if ((pmg.hposp1 < 8) || (pmg.hposp1 >= 216))
            pmg.grafp1 = 0;
        if ((pmg.hposp2 < 8) || (pmg.hposp2 >= 216))
            pmg.grafp2 = 0;
        if ((pmg.hposp3 < 8) || (pmg.hposp3 >= 216))
            pmg.grafp3 = 0;
        }

    if (pmg.grafm)
        {
        // Missiles that are offsecreen are treated as invisible
		// 47 and 208 are the limits, make sure it doesn't encroach
        if ((pmg.hposm0 < 8) || (pmg.hposm0 >= 216))
            pmg.grafm &= ~0x03;
        if ((pmg.hposm1 < 8) || (pmg.hposm1 >= 216))
            pmg.grafm &= ~0x0C;
        if ((pmg.hposm2 < 8) || (pmg.hposm2 >= 216))
            pmg.grafm &= ~0x30;
        if ((pmg.hposm3 < 8) || (pmg.hposm3 >= 216))
            pmg.grafm &= ~0xC0;
        }

    // set PM/G flag if any player or missile data is non-zero

    sl.fpmg = (pmg.grafpX || pmg.grafm);

    // If GTIA is enabled, only let mode 15 display GTIA, all others blank
	// mode 15 is GTIA mode. prior says whether that will be GR. 9, 10 or 11.
    if (sl.prior & 0xC0)
    {
        if (sl.modelo >= 15)
        {
            sl.modelo = 15 + (sl.prior >> 6); // (we'll call 16, 17 and 18 GR. 9, 10 and 11)
        }
        else
        {
            sl.modelo = 0;
            cbWidth = 0;
        }
    }

// RENDER section - used to be called separately

	BYTE rgbSpecial = 0;	// the byte off the left of the screen that needs to be scrolled on
	//static BYTE sModeLast;	// last ANTIC mode we saw

	// GTIA mode GR.10 uses 704 as background colour, enforced in all scan lines of any antic mode
	sl.colbk = ((PRIOR & 0xC0) == 0x80) ? COLPM0 : COLBK;
	
	sl.colpfX = COLPFX;
    pmg.colpmX = COLPMX;

    if ((wScan < wStartScan) || (wScan >= wStartScan + wcScans))
        goto Lnextscan;

    rgfChanged = 0;

#define fModelo       1
#define fModehi       2
#define fScan       4
#define fAddr       8
#define fDmactl       16
#define fChbase       32
#define fChactl       64
#define fColbk       128
#define fColpf0       256
#define fColpf1       512
#define fColpf2       1024
#define fColpf3       2048
#define fColpfX       (fColpf0|fColpf1|fColpf2|fColpf3)
#define fPrior      4096
#define fFineScroll 8192
#define fPMG        16384
#define fDisp       0x8000
#define fAll        0xFFFF

	// remove the code that attempted to avoid unnecessary rendering when nothing changed, until then, keep this hack
	SL *psl = &sl;

#if 0
	psl = &rgsl[0];	// look at how things were at the top of the screen compared to now?
	
	if (sl.modelo != psl->modelo) rgfChanged |= fModelo;
    if (sl.modehi != psl->modehi) rgfChanged |= fModehi;
    if (sl.scan   != psl->scan  ) rgfChanged |= fScan;    
    if (sl.addr   != psl->addr  ) rgfChanged |= fAddr;    
    if (sl.dmactl != psl->dmactl) rgfChanged |= fDmactl;
    if (sl.chbase != psl->chbase) rgfChanged |= fChbase;
    if (sl.chactl != psl->chactl) rgfChanged |= fChactl;
    if (sl.colbk  != psl->colbk ) rgfChanged |= fColbk;    
#if 1
    if (sl.colpfX != psl->colpfX) rgfChanged |= fColpfX;
#else
    if (sl.colpf0 != psl->colpf0) rgfChanged |= fColpf0;
    if (sl.colpf1 != psl->colpf1) rgfChanged |= fColpf1;
    if (sl.colpf2 != psl->colpf2) rgfChanged |= fColpf2;
    if (sl.colpf3 != psl->colpf3) rgfChanged |= fColpf3;
#endif
    if (sl.prior  != psl->prior)  rgfChanged |= fPrior;
    if (sl.vscrol != psl->vscrol) rgfChanged |= fFineScroll;
    if (sl.hscrol != psl->hscrol) rgfChanged |= fFineScroll;

    if (sl.fpmg   |  psl->fpmg)   rgfChanged |= fPMG;
	
#if 0
    if (rgfChanged & fPMG)
        {
        rgfChanged |= fAll;
        }
#endif
#endif

	// !!! we no longer support trying to save time and skip some rendering if things are the same.
	// It won't save much time, and it's really hard to get right (it never was)
	// This is for variables rgfChanged and fDataChanged
    rgfChanged |= fAll;

	// Old comment:
    // Even if rgfChanged is 0 at this point, we have to check the 10 to 48
    // data bytes associated with this scan line. If it is the first scan line
    // of this mode then we need to blit and compare the memory.
    // If it isn't the first scan line and !fDataChanged then there is no
    // need to blit and compare since nothing changed since the first scan line.

	if (((sl.modelo) >= 2) && (rgfChanged || (iscan == sl.vscrol) || (fDataChanged)))
	{
		j = 0;

		// HSCROL - we read more bytes than we display. View the center portion of what we read, offset cbDisp/10 bytes into the scan line

		if (cbDisp != cbWidth)
		{
			// wide is 48, normal is 40, that's j=4 (split the difference) characters before we begin
			// subtract one for every multiple of 8 hshift is, that's an entire character shift
			j = ((cbWidth - cbDisp) >> 1) - ((hshift) >> 3);
			hshift &= 7;	// now only consider the part < 8
		}

		if (fDataChanged)
		{
			// sl.rgb already has data from previous scan line
		}
		else if (((wAddr + j) & 0xFFF) < 0xFD0)
		{
			_fmemcpy(sl.rgb, &rgbMem[wAddr + j], cbWidth);
			rgbSpecial = rgbMem[wAddr + j - 1];	// the byte just offscreen to the left may be needed if scrolling
		}
		else
		{
			for (i = 0; i < cbWidth; i++)
				sl.rgb[i] = cpuPeekB(iVM, (wAddr & 0xF000) | ((wAddr + i + j) & 0x0FFF));
			rgbSpecial = cpuPeekB(iVM, (wAddr & 0xF000) | ((wAddr + j - 1) & 0x0FFF));	// ditto
		}

		if (fDataChanged || memcmp(&sl.rgb, &psl->rgb[0], cbWidth))
		{
			rgfChanged |= fDisp;
			fDataChanged = 1;
		}
	}

    // check if scan line is visible and if so redraw it
	// THIS IS A LOT OF CODE
	//
    if (rgfChanged && (wScan >= wStartScan) && (wScan < (wStartScan+wcScans)))
    {
        // redraw scan line

        BYTE *qch0 = vrgvmi[iVM].pvBits;
        BYTE FAR *qch = qch0;

        BYTE b1, b2;
        BYTE col0, col1, col2, col3;
        BYTE vpix = iscan;

#ifndef NDEBUG
        // show current stack
        // annoying debug pixels near top of screen: qch[regSP & 0xFF] = (BYTE)(cpuPeekB(regSP | 0x100) ^ wFrame);
#endif

		// more unimplemented code to see if we're dirty
        if ((wScan >= wStartScan) && (wScan < (wStartScan+wcScans)))
            {
            wScanMin = min(wScanMin, (wScan - wStartScan));
            wScanMac = max(wScanMac, (wScan - wStartScan) + 1);
            }

		if (sl.fpmg)
		{
			// When PM/G are present on the scan line is first rendered
			// into rgpix as bytes of bit vectors representing the playfields
			// and PM/G that are present at each pixel. Then later we map
			// rgpix to actual colors on the screen, applying priorities
			// in the process.

			qch = &rgpix[(NTSCx - X8) >> 1];	// first visible screen NTSC pixel
			_fmemset(rgpix, 0, sizeof(rgpix));

			// GTIA modes 9, 10 & 11 don't use the PF registers since they can have 16 colours instead of 4
			// so we CANNOT do a special bitfield mode. See the PRIOR code
			if (sl.modelo <= 15)
			{
				sl.colbk = bfBK;
				sl.colpf0 = bfPF0;
				sl.colpf1 = bfPF1;
				sl.colpf2 = bfPF2;
				sl.colpf3 = bfPF3;
			}
		}
		else
            qch += (wScan - wStartScan) * vcbScan;

		// narrow playfield - if we're in bitfield mode and GTIA GR. 9-11, the background colour has to occupy the lower nibble only.
		// in GR.10, it's the index to the background colour, since the background colour itself is 8 bits and won't fit in a nibble.
        if ((sl.modelo >= 2) && ((sl.dmactl & 3) == 1))
        {
			if (rgfChanged & (fPMG | fDmactl | fModelo | fColbk))
			{
				if (sl.fpmg && sl.modelo == 16)
				{
					memset(qch, sl.colbk & 0x0f, (X8 - NARROWx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NARROWx) >> 1), sl.colbk & 0x0f, (X8 - NARROWx) >> 1);	// background colour after
				}
				else if (sl.fpmg && sl.modelo == 17)
				{
					memset(qch, 0, (X8 - NARROWx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NARROWx) >> 1), 0, (X8 - NARROWx) >> 1);	// background colour after
				}
				else if (sl.fpmg && sl.modelo == 18)
				{
					memset(qch, sl.colbk >> 4, (X8 - NARROWx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NARROWx) >> 1), sl.colbk >> 4, (X8 - NARROWx) >> 1);	// background colour after
				}
				else
				{
					memset(qch, sl.colbk, (X8 - NARROWx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NARROWx) >> 1), sl.colbk, (X8 - NARROWx) >> 1);	// background colour after

				}
			}
            qch += (X8-NARROWx)>>1;
        }
		
		// normal playfield is 320 wide
		if ((sl.modelo >= 2) && ((sl.dmactl & 3) == 2))
		{
			if (rgfChanged & (fPMG | fDmactl | fModelo | fColbk))
			{
				if (sl.fpmg && sl.modelo == 16)
				{
					memset(qch, sl.colbk & 0x0f, (X8 - NORMALx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NORMALx) >> 1), sl.colbk & 0x0f, (X8 - NORMALx) >> 1);	// background colour after
				}
				else if (sl.fpmg && sl.modelo == 17)
				{
					memset(qch, 0, (X8 - NORMALx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NORMALx) >> 1), 0, (X8 - NORMALx) >> 1);	// background colour after
				}
				else if (sl.fpmg && sl.modelo == 18)
				{
					memset(qch, sl.colbk >> 4, (X8 - NORMALx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NORMALx) >> 1), sl.colbk >> 4, (X8 - NORMALx) >> 1);	// background colour after
				}
				else
				{
					memset(qch, sl.colbk, (X8 - NORMALx) >> 1);		// background colour before
					memset(qch + X8 - ((X8 - NORMALx) >> 1), sl.colbk, (X8 - NORMALx) >> 1);	// background colour after
				}
			}
			qch += (X8 - NORMALx) >> 1;
		}
	
		// wide playfield takes up the whole width

        rgfChanged &= ~fDisp;

        if (hshift)
            rgfChanged |= fFineScroll;

        switch (sl.modelo)
        {
        default:
            if (!(rgfChanged & (fPMG | fDmactl | fModelo | fColbk)))
                break;

            // just draw a blank scan line - which in GR.10 w/ PMG (bitfield mode) is just an index
			// no matter what actual ANTIC mode, if GTIA is in GR.10 mode, we use its background colour
            _fmemset(qch, (sl.fpmg && ((PRIOR & 0xC0) == 0x80)) ? 0 : sl.colbk, X8);
            break;

		// GR.0 and descended character GR.0
        case 2:
		case 3:
			BYTE vpixO = vpix % (sl.modelo == 2 ? 8 : 10);
			
			// mimic obscure ANTIC behaviour (why not?) Scans 10-15 duplicate 2-7, not 0-5
			if (sl.modelo == 3 && iscan > 9)
				vpixO = iscan - 8;
			
			vpix = vpixO;
			
			// the artifacting colours
			BYTE red = 0x40 | (sl.colpf1 & 0x0F), green = 0xc0 | (sl.colpf1 & 0x0F);
			BYTE yellow = 0xe0 | (sl.colpf1 & 0x0F);

			// just for fun, don't interlace in B&W
			BOOL fArtifacting = (rgvm[iVM].bfMon == monColrTV);

            col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);
            col2 = sl.colpf2;

			if ((sl.chactl & 4) && sl.modelo == 2)	// vertical reflect bit
				vpix = 7 - (vpix & 7);
			
            for (i = 0 ; i < cbDisp; i++)
            {
                if (((b1 = sl.rgb[i]) == psl->rgb[i]) && !rgfChanged)
                    continue;

				if ((sl.chactl & 4) && sl.modelo == 3)
				{
					if ((sl.rgb[i] & 0x7F) < 0x60)
						vpix = vpixO < 8 ? 7 - vpixO : vpixO;
					else
						// mimic odd ANTIC behaviour - you're just going to have to trust me on this one
						vpix = (vpixO > 7) ? (23 - vpixO) : (vpixO < 2 ? vpixO : (vpixO < 6 ? 7 - vpixO : 15 - vpixO));
				}

				// non-zero horizontal scroll
                if (hshift)
                {
					ULONGLONG u, vv;
					UINT vpix23;

					// we need to look at 1 or 2 of the characters ending in the current character, depending on how much shift.

					// we are not in the blank part of a mode 3 character
					if (sl.modelo == 2 || ((vpix >= 2 && vpix < 8) || (vpix < 2 && (sl.rgb[i] & 0x7F) < 0x60) ||
									(vpix >= 8 && (sl.rgb[i] & 0x7F) >= 0x60)))
					{
						// use the top two rows of pixels for the bottom 2 scan lines for ANTIC mode 3 descended characters
						vpix23 = (vpix >= 8 && (sl.rgb[i] & 0x7f) >= 0x60) ? vpix - 8 : vpix;

						// CHBASE must be on an even page boundary
						vv = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((sl.rgb[i] & 0x7f) << 3) + vpix23);
					}
					// we ARE in the blank part
					else {
						vv = 0;
					}

					// mimic obscure quirky ANTIC behaviour - scans 8 and 9 go missing under these circumstances
					if ((sl.rgb[i] & 0x7f) < 0x60 && (iscan == 8 || iscan == 9))
						vv = 0;
					
					if (sl.rgb[i] & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							vv = 0;
						if (sl.chactl & 2)  // inverse flag
							vv = ~vv & 0xff;
					}
					
					u = vv;

					if (hshift %8)
					{
						// partial shifting means we also need to look at a second character intruding into our space
						// do the same exact thing again
						int index = 1;
						
						// we can't look past the front of the array, we stored that byte in rgbSpecial
						BYTE rgb = rgbSpecial = 0;
						if (i > 0)
							rgb = sl.rgb[i - index];

						if ((sl.chactl & 4) && sl.modelo == 3)
						{
							if ((rgb & 0x7F) < 0x60)
								vpix = vpixO < 8 ? 7 - vpixO : vpixO;
							else
								vpix = (vpixO > 7) ? (23 - vpixO) : (vpixO > 1 ? 7 - vpixO : vpixO);    // mimic odd ANTIC behaviour
						}

						if (sl.modelo == 2 || ((vpix >= 2 && vpix < 8) || (vpix < 2 && (rgb & 0x7F) < 0x60) ||
							(vpix >= 8 && (rgb & 0x7F) >= 0x60)))
						{
							// use the top two rows of pixels for the bottom 2 scan lines for ANTIC mode 3 descended characters
							vpix23 = (vpix >= 8 && (rgb & 0x7f) >= 0x60) ? vpix - 8 : vpix;

							// CHBASE must be on an even page boundary
							vv = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((rgb & 0x7f) << 3) + vpix23);
						}
						// we ARE in the blank part
						else {
							vv = 0;
						}

						// mimic obscure quirky ANTIC behaviour
						if (rgb < 0x60 && (iscan == 8 || iscan == 9))
							vv = 0;
					
						if (rgb & 0x80)
						{
							if (sl.chactl & 1)  // blank (blink) flag
								vv = 0;
							if (sl.chactl & 2)  // inverse flag
								vv = ~vv & 0xff;
						}

						u += (vv << (index << 3));
					}

					// now do the shifting
					b2 = (BYTE)(u >> hshift);

				}
				else
				{
					if (sl.modelo == 2 || ((vpix >= 2) && (vpix < 8)))
						// !!! was 0xFC
						b2 = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((b1 & 0x7F) << 3) + vpix);
					else if ((vpix < 2) && ((b1 & 0x7f) < 0x60))
						b2 = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((b1 & 0x7F) << 3) + vpix);
					else if ((vpix >= 8) && ((b1 & 0x7F) >= 0x60))
						b2 = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((b1 & 0x7F) << 3) + vpix - 8);
					else
						b2 = 0;

					// mimic obscure quirky ANTIC behaviour
					if (b1 < 0x60 && (iscan == 8 || iscan == 9))
						b2 = 0;

					if (b1 & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							b2 = 0;
						if (sl.chactl & 2)  // inverse flag
							b2 = ~b2;
					}
				}

//    col2 ^= cpuPeekB(20);

                // See mode 15 for artifacting theory of operation

				// do a pair of pixels, even then odd
				for (j = 0; j < 4; j++)
				{
					// EVEN

					switch (b2 & 0x80)
					{
					case 0x00:
						*qch++ = col2;
						break;

					case 0x80:
						// don't walk off the beginning of the array
						BYTE last = col2, last2 = col2;

						if (i != 0 || j != 0)
						{
							last = *(qch - 1);
							last2 = *(qch - 2);
						}

						if (last == col2)
						{
							*qch++ = fArtifacting ? red : col1;
							if (last2 == red)
								*(qch - 2) = red; // shouldn't affect a visible pixel if it's out of range
						}
						else
						{
							*qch++ = col1;
							if (last == green)
							{
								*(qch - 2) = col1; // yellow doesn't seem to work
								//*(qch - 1) = yellow;
							}
						}
					}
					
					// ODD

					switch (b2 & 0x40)
					{
					case 0x00:
						*qch++ = col2;
						break;

					case 0x40:
						// don't walk off the beginning of the array later on
						BYTE last, last2 = col2;

						last = *(qch - 1);
						if (i == 0 && j == 0)
							last2 = col2;
						else
							last2 = *(qch - 2);

						if (last == col2)
						{
							*qch++ = fArtifacting ? green : col1;
							if (last2 == green)
								*(qch - 2) = green; // shouldn't affect a visible pixel if it's out of range
						}
						else
						{
							*qch++ = col1;
							if (last == red)
								*(qch - 2) = col1; // shouldn't affect a visible pixel if it's out of range
						}
					}

					b2 <<= 2;
				}

//                _fmemcpy(qch,rgch,8);
            }
            break;

        case 5:
            vpix = iscan >> 1;	// extra thick, use screen data twice for 2 output lines
        case 4:
            if (sl.chactl & 4)
                vpix ^= 7;                // vertical reflect bit
            vpix &= 7;

            col0 = sl.colbk;
            col1 = sl.colpf0;
            col2 = sl.colpf1;
//            col3 = sl.colpf2;

            for (i = 0 ; i < cbDisp; i++)
                {
                if (((b1 = sl.rgb[i]) == psl->rgb[i]) && !rgfChanged)
                    {
                    qch += 8;
                    continue;
                    }

                if (hshift)
                {
                    // non-zero horizontal scroll

					ULONGLONG u, vv;

					// see comments for modes 2 & 3
					int index = 0;

					vv = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((sl.rgb[i - index] & 0x7F) << 3) + vpix);
					u = vv << (index << 3);

					if (hshift % 8)
					{
						index += 1;
						vv = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + (((i == 0 ? rgbSpecial : sl.rgb[i - index]) & 0x7f) << 3) + vpix);
						u |= (vv << (index << 3));
					}

					b2 = (BYTE)(u >> hshift);
                }
                else
                    b2 = cpuPeekB(iVM, ((sl.chbase & 0xFE) <<8) + ((b1 & 0x7F)<<3) + vpix);

                for (j = 0; j < 4; j++)
                {
					// which character was shifted into this position? Pay attention to its high bit to switch colours
					int index = (hshift + 7 - 2 * j) / 8;

					if (((i == 0 && index == 1) ? rgbSpecial : sl.rgb[i - index]) & 0x80)
						col3 = sl.colpf3;
					else
						col3 = sl.colpf2;

					switch (b2 & 0xC0)
                        {
                    default:
                        Assert(FALSE);
                        break;
                
                    case 0x00:
                        *qch++ = col0;
                        *qch++ = col0;
                        break;

                    case 0x40:
                        *qch++ = col1;
                        *qch++ = col1;
                        break;

                    case 0x80:
                        *qch++ = col2;
                        *qch++ = col2;
                        break;

                    case 0xC0:
                        *qch++ = col3;
                        *qch++ = col3;
                        break;
                        }
                    b2 <<= 2;
                }
                }
            break;

        case 7:
            vpix = iscan >> 1;
        case 6:
            if (sl.chactl & 4)
                vpix ^= 7;                // vertical reflect bit
            vpix &= 7;

            col0 = sl.colbk;

            for (i = 0 ; i < cbDisp; i++)
                {
                if (((b1 = sl.rgb[i]) != psl->rgb[i]) || rgfChanged)
                    {
                    col1 = sl.colpf[b1>>6];

					if (hshift)
					{
						// non-zero horizontal scroll

						ULONGLONG u, vv;

						// see comments for modes 2 & 3
						int index = 0;
						
						vv = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + ((sl.rgb[i - index] & 0x3F) << 3) + vpix);
						u = vv << (index << 3);

						if (hshift % 8)
						{
							index += 1;
							vv = cpuPeekB(iVM, ((sl.chbase & 0xFE) << 8) + (((i == 0 ? rgbSpecial : sl.rgb[i - index]) & 0x3f) << 3) + vpix);
							u |= (vv << (index << 3));
						}

						b2 = (BYTE)(u >> hshift);
					
                        for (j = 0; j < 8; j++)
                            {
                            if (j < hshift)	// hshift restricted to 7 or less, so this is sufficient
                                col1 = sl.colpf[(i == 0 ? rgbSpecial : sl.rgb[i-1])>>6];
							else
								col1 = sl.colpf[b1 >> 6];

                            if (b2 & 0x80)
                                {
                                *qch++ = col1;
                                *qch++ = col1;
                                }
                            else
                                {
                                *qch++ = col0;
                                *qch++ = col0;
                                }
                            b2 <<= 1;
                            }
                    }
                    else
                    {
                        b2 = cpuPeekB(iVM, (sl.chbase << 8)
                            + ((b1 & 0x3F) << 3) + vpix);

                        for (j = 0; j < 8; j++)
                            {
                            if (b2 & 0x80)
                                {
                                *qch++ = col1;
                                *qch++ = col1;
                                }
                            else
                                {
                                *qch++ = col0;
                                *qch++ = col0;
                                }
                            b2 <<= 1;
                            }
                        }
                    }
                else
                    {
                    qch += 16;
                    }
                }
            break;

        case 8:
            col0 = sl.colbk;
            col1 = sl.colpf0;
            col2 = sl.colpf1;
            col3 = sl.colpf2;

			for (i = 0; i < cbDisp; i++)
			{
				b2 = sl.rgb[i];

				if (!rgfChanged && (b2 == psl->rgb[i]))
				{
					qch += 32;
					continue;
				}

				WORD u = b2;

				// can't check hshift, because it is half of sl.hscrol, which might be 0 when sl.hscrol == 1
				if (sl.hscrol)
				{
					// non-zero horizontal scroll

					// this shift may involve our byte and the one before it
					u = (sl.rgb[i - 1] << 8) | (BYTE)b2;
				}
				
				// what 1/2-bit position in the WORD u do we start copying from?
				int index = 15 + sl.hscrol;

				// copy 2 screen pixels each iteration, for 32 pixels written per screen byte in this mode
				for (j = 0; j < 16; j++)
				{
					// which 2 bit pair is this 1/2-bit position inside?
					// I really should have drawn a lot of pictures)
					int k = (index - j) >> 2;

					// look at that bit pair
					b2 = (u >> k >> k) & 0x3;

					switch (b2 & 0x03)
					{
					case 0x00:
						*qch++ = col0;
						*qch++ = col0;
						break;

					case 0x01:
						*qch++ = col1;
						*qch++ = col1;
						break;

					case 0x02:
						*qch++ = col2;
						*qch++ = col2;
						break;

					case 0x03:
						*qch++ = col3;
						*qch++ = col3;
						break;
					}
				}
			}
            break;

        case 9:
            col0 = sl.colbk;
            col1 = sl.colpf0;

            for (i = 0 ; i < cbDisp; i++)
            {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 32;
                    continue;
                    }

				WORD u = b2;

				// can't use hshift, it is 1/2 of sl.hscrol, which might be only 1. It's OK, hshift never can get to 8 to reset to 0.
				if (sl.hscrol)
				{
					// non-zero horizontal scroll

					// this shift may involve our byte and the one before it
					u = ((i == 0 ? rgbSpecial : sl.rgb[i - 1]) << 8) | (BYTE)b2;
				}

				// what 1/2-bit position in the WORD u do we start copying from?
				int index = 15 + sl.hscrol;

				// copy 2 screen pixels each iteration, for 32 pixels written per screen byte in this mode
				for (j = 0; j < 16; j++)
				{
					// which bit is this 1/2-bit position inside?
					int k = (index - j) >> 1;

					// look at that bit
					b2 = (u >> k) & 0x1;

					switch (b2 & 0x01)
					{
					case 0x00:
						*qch++ = col0;
						*qch++ = col0;
						break;

					case 0x01:
						*qch++ = col1;
						*qch++ = col1;
						break;
					}
				}
			}
			break;

        case 10:
            col0 = sl.colbk;
            col1 = sl.colpf0;
            col2 = sl.colpf1;
            col3 = sl.colpf2;

            for (i = 0 ; i < cbDisp; i++)
            {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                {
                    qch += 16;
                    continue;
                }

				WORD u = b2;

				// modes 8 and 9 cannot shift more than 1 byte, and hshift could be 0 if sl.hscrol == 1, so we had to test sl.hscrol.
				// This mode can shift > 1 byte, so hshift might be truncated to %8, so it's the opposite...
				// we have to test hshift.
				if (hshift)
				{
					// non-zero horizontal scroll

					// this shift may involve our byte and the one before it
					u = ((i == 0 ? rgbSpecial : sl.rgb[i - 1]) << 8) | (BYTE)b2;
				}

				// what bit position in the WORD u do we start copying from?
				int index = 7 + hshift;

				// copy 2 screen pixels each iteration of a bit, for 16 pixels written per screen byte in this mode
				for (j = 0; j < 8; j++)
				{
					// which 2 bit pair is this bit position inside?
					int k = (index - j) >> 1;

					// look at that bit pair
					b2 = (u >> k >> k) & 0x3;

					switch (b2 & 0x03)
					{
					case 0x00:
						*qch++ = col0;
						*qch++ = col0;
						break;

					case 0x01:
						*qch++ = col1;
						*qch++ = col1;
						break;

					case 0x02:
						*qch++ = col2;
						*qch++ = col2;
						break;

					case 0x03:
						*qch++ = col3;
						*qch++ = col3;
						break;
					}
				}
			}
			break;

		// these only differ by # of scan lines, we'll get called twice for 11 and once for 12
        case 11:
        case 12:
            col0 = sl.colbk;
            col1 = sl.colpf0;

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 16;
                    continue;
                    }

				WORD u = b2;

				// see comments in mode 10, must use hshift, not sl.hscrol for this mode
				if (hshift)
				{
					// non-zero horizontal scroll

					// this shift may involve our byte and the one before it
					u = ((i == 0 ? rgbSpecial : sl.rgb[i - 1]) << 8) | (BYTE)b2;
				}

				// what bit position in the WORD u do we start copying from?
				int index = 7 + hshift;

				// copy 2 screen pixels each iteration, for 16 pixels written per screen byte in this mode
				for (j = 0; j < 8; j++)
				{
					// which bit is this bit position inside?
					int k = (index - j);

					// look at that bit
					b2 = (u >> k) & 0x1;

					switch (b2 & 0x01)
					{
					case 0x00:
						*qch++ = col0;
						*qch++ = col0;
						break;

					case 0x01:
						*qch++ = col1;
						*qch++ = col1;
						break;
					}
				}
			}
			break;

        case 13:
        case 14:
            col0 = sl.colbk;
            col1 = sl.colpf0;
            col2 = sl.colpf1;
            col3 = sl.colpf2;

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

				WORD u = b2;

				// hshift tells us if there's any hscrol needed not accounted for by an even multiple of 8
				if (hshift)
				{
					// non-zero horizontal scroll

					// this shift may involve our byte and the one before it
					u = ((i == 0 ? rgbSpecial : sl.rgb[i - 1]) << 8) | (BYTE)b2;
				}

				// what bit pair position in the WORD u do we start copying from?
				int index = 3 + (hshift >> 1);

				// copy 2 screen pixels each iteration of a bit pair, for total of 8 pixels written per screen byte in this mode
				for (j = 0; j < 4; j++)
				{
					// which 2 bit pair is this bit position inside?
					int k = (index - j);

					// look at that bit pair
					b2 = (u >> k >> k) & 0x3;

					switch (b2 & 0x03)
					{
					case 0x00:
						*qch++ = col0;
						*qch++ = col0;
						break;

					case 0x01:
						*qch++ = col1;
						*qch++ = col1;
						break;

					case 0x02:
						*qch++ = col2;
						*qch++ = col2;
						break;

					case 0x03:
						*qch++ = col3;
						*qch++ = col3;
						break;
					}
				}
			}
			break;

        case 15:
            col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);	// like GR.0, the colour is just a luminence of the background colour
            col2 = sl.colpf2;

			// the artifacting colours
			red = 0x40 | (sl.colpf1 & 0x0F), green = 0xc0 | (sl.colpf1 & 0x0F);
			yellow = 0xe0 | (sl.colpf1 & 0x0F);

			// just for fun, don't interlace in B&W
			fArtifacting = (rgvm[iVM].bfMon == monColrTV);
            
			for (i = 0 ; i < cbDisp; i++)
            {				
				b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                {
                    qch += 8;
                    continue;
                }

				WORD u = b2;

				// do you have the hang of this by now? Use hshift, not sl.hscrol
				if (hshift)
				{
					// non-zero horizontal scroll

					// this shift may involve our byte and the one before it
					u = ((i == 0 ? rgbSpecial : sl.rgb[i - 1]) << 8) | (BYTE)b2;
				}

				// what bit position in the WORD u do we start copying from?
				int index = 7 + hshift;	// ATARI can only shift 2 pixels minimum at this resolution

				// THEORY OF OPERATION - INTERLACING
				// - only even pixels show red, only odd pixels show green (interpolate the empty pixels to be that colour too)
				// - odd and even shows orange. even and odd show white. 3 pixels in a row all show white
				// - a background pixel between white and (red/green) seems to stay background colour

				// copy 2 screen pixel each iteration (odd then even), for 8 pixels written per screen byte in this mode
				for (j = 0; j < 4; j++)
				{
					// don't walk off the beginning of the array
					BYTE last, last2;

					if (i == 0 && j == 0)
					{
						last = col2;
						last2 = col2;
					}
					else
					{
						last = *(qch - 1);
						last2 = *(qch - 2);
					}

					// EVEN - (unwind the loop for speed)

					// which bit is this bit position?
					int k = (index - (j << 1));

					// look at that bit
					b2 = (u >> k) & 0x1;

					// supports artifacting
					switch (b2 & 0x01)
					{
					case 0x00:
						*qch++ = col2;
						break;

					case 0x01:
						if (last == col2)
						{
							*qch++ = fArtifacting ? red : col1;
							if (last2 == red)
								*(qch - 2) = red; // shouldn't affect a visible pixel if it's out of range
						}
						else
						{
							*qch++ = col1;
							if (last == green)
							{
								*(qch - 2) = col1; // yellow doesn't seem to work
								//*(qch - 1) = yellow;
							}
						}
						break;
					}

					// ODD
					
					// don't walk off the beginning of the array later on
					last = *(qch - 1);
					if (i == 0 && j == 0)
						last2 = col2;
					else
						last2 = *(qch - 2);

					// which bit is this bit position?
					k = (index - (j << 1) - 1);

					// look at that bit
					b2 = (u >> k) & 0x1;

					// supports artifacting
					switch (b2 & 0x01)
					{
					case 0x00:
						*qch++ = col2;
						break;

					case 0x01:
						if (last == col2)
						{
							*qch++ = fArtifacting ? green : col1;
							if (last2 == green)
								*(qch - 2) = green; // shouldn't affect a visible pixel if it's out of range
						}
						else
						{
							*qch++ = col1;
							if (last == red)
								*(qch - 2) = col1; // shouldn't affect a visible pixel if it's out of range
						}
						break;
					}
				}
			}
			break;

        case 16:
            // GTIA 16 grey mode

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

				// GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
				// use low nibble of previous byte
				if (hshift & 0x04)
				{
					b2 = b2 >> 4;
					b2 |= (((i == 0 ? rgbSpecial : sl.rgb[i - 1]) & 0x0f) << 4);
				}

				col1 = (b2 >> 4) | (sl.colbk /* & 0xf0 */);	// let the user screw up the colours if they want, like a real 810
				col2 = (b2 & 15) | (sl.colbk /* & 0xf0*/ ); // they should only POKE 712 with multiples of 16

				// we're in BITFIELD mode because PMG are present, so we can only alter the low nibble. We'll put the chroma value back later.
				if (sl.fpmg)
				{
					col1 &= 0x0f;
					col2 &= 0x0f;
				}
                *qch++ = col1;
                *qch++ = col1;
                *qch++ = col1;
                *qch++ = col1;

                *qch++ = col2;
                *qch++ = col2;
                *qch++ = col2;
                *qch++ = col2;
                }
            break;

        case 17:
            // GTIA 9 color mode - GR. 10

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

				// GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
				// use low nibble of previous byte
				if (hshift & 0x04)
				{
					b2 = b2 >> 4;
					b2 |= (((i == 0 ? rgbSpecial : sl.rgb[i - 1]) & 0x0f) << 4);
				}

                col1 = (b2 >> 4);
				col2 = (b2 & 15);

				// if PMG are present on this line, we are asked to make a bitfield in the low nibble of which colours we are 
				// using. That's not possible in GR.10 which has >4 possible colours, so we'll just use our index. If we're not
				// in bitfield mode, we'll use the actual colour

				if (!sl.fpmg)
				{
					if (col1 < 9)
						col1 = *(((BYTE FAR *)&COLPM0) + col1);
					else if (col1 < 12)
						col1 = *(((BYTE FAR *)&COLPM0) + 8);	// col. 9-11 are copies of c.8
					else
						col1 = *(((BYTE FAR *)&COLPM0) + col1 - 8);	// col. 12-15 are copies of c.4-7

					if (col2 < 9)
						col2 = *(((BYTE FAR *)&COLPM0) + col2);
					else if (col2 < 12)
						col2 = *(((BYTE FAR *)&COLPM0) + 8);	// col. 9-11 are copies of c.8
					else
						col2 = *(((BYTE FAR *)&COLPM0) + col2 - 8);	// col. 12-15 are copies of c.4-7
				}
				else
				{
					if (col1 < 9)
						;
					else if (col1 < 12)
						col1 = 8;			// col. 9-11 are copies of c.8
					else
						col1 = col1 - 8;	// col. 12-15 are copies of c.4-7

					if (col2 < 9)
							;
					else if (col2 < 12)
						col2 = 8;			// col. 9-11 are copies of c.8
					else
						col2 = col2 - 8;	// col. 12-15 are copies of c.4-7
				}
                
				*qch++ = col1;
                *qch++ = col1;
                *qch++ = col1;
                *qch++ = col1;

                *qch++ = col2;
                *qch++ = col2;
                *qch++ = col2;
                *qch++ = col2;
                }
            break;

        case 18:
            // GTIA 16 color mode GR. 11

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

				// GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
				// use low nibble of previous byte
				if (hshift & 0x04)
				{
					b2 = b2 >> 4;
					b2 |= (((i == 0 ? rgbSpecial : sl.rgb[i - 1]) & 0x0f) << 4);
				}

				// !!! restrict all drawing in ProcessScanLine of the background to lum=0 in ANTIC mode 18!

				col1 = ((b2 >> 4) << 4) | (sl.colbk /* & 15*/);	// lum comes from 712
				col2 = ((b2 & 15) << 4) | (sl.colbk /* & 15*/); // keep 712 <16, if not, it deliberately screws up like the real hw
				//col1 = ((b2 >> 4) << 4) | (sl.colbk & 15);
                //col2 = ((b2 & 15) << 4) | (sl.colbk & 15);

				// we're in BITFIELD mode because PMG are present, so we can only alter the low nibble.
				// we'll shift it back up and put the luma value back in later
				if (sl.fpmg)
				{
					col1 = col1 >> 4;
					col2 = col2 >> 4;
				}

                *qch++ = col1;
                *qch++ = col1;
                *qch++ = col1;
                *qch++ = col1;

                *qch++ = col2;
                *qch++ = col2;
                *qch++ = col2;
                *qch++ = col2;
                }
            break;

        }
    }

	// may have been altered if we were in BITFIELD mode b/c PMG are active. Put them back to what they were
	if (sl.modelo <= 15)
	{
		sl.colbk = ((PRIOR & 0xC0) == 0x80) ? COLPM0 : COLBK;
		sl.colpfX = COLPFX;
	}

    if (rgfChanged || fDataChanged)
        *psl = sl;

    if ((wScan < wStartScan) || (wScan >= (wStartScan+wcScans)))
        goto Lnextscan;

    if (sl.fpmg)
    {
        BYTE *qch = vrgvmi[iVM].pvBits;

		// now set the bits in rgpix corresponding to players and missiles. Must be in this order for correct collision detection
		DrawPlayers(iVM, (WORD NEAR *)(rgpix + ((NTSCx - X8)>>1)));
		DrawMissiles(iVM, (WORD NEAR *)(rgpix + ((NTSCx - X8)>>1)), sl.prior & 16);	// 5th player?

#ifndef NDEBUG
    if (0)
        if (*(ULONG *)MXPF || *(ULONG *)MXPL || *(ULONG *)PXPF || *(ULONG *)PXPL)
            {
            DebugStr("wScan = %03d, MXPF:%08X MXPL:%08X PXPF:%08X PXPL:%08X, %03d %03d %03d %03d, %d\n", wScan,
                *(ULONG *)MXPF, *(ULONG *)MXPL, *(ULONG *)PXPF, *(ULONG *)PXPL,
                pmg.hposp0, pmg.hposp1, pmg.hposp2, pmg.hposp3, VDELAY);
            }
#endif

        pmg.fHitclr = 0;

        // if in a 2 color mode, set playfield 1 color to some luminence of PF2

        if ((sl.modelo == 2) || (sl.modelo == 3) || (sl.modelo == 15))
            sl.colpf1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);

        if ((wScan < wStartScan) || (wScan >= (wStartScan+wcScans)))
            goto Lnextscan;

        // now map the rgpix array to the screen

        qch += (wScan - wStartScan) * vcbScan;

        // if GTIA, give players and missiles highest priority !!! is that true?

        if (sl.modelo > 15)
            sl.prior = 1;

		// turn the rgpix array from a bitfield of which items are present (player or field colours)
		// into the actual colour that will show up there, based on priorities
		// reminder: b = PM3 PM2 PM1 PM0 | PF3 PF2 PF1 PF0 (all zeroes means BKGND)

		// EXCEPT for GR.9, 10 & 11, which does not have playfield bitfield data. There are 16 colours in those modes and only 4 regsiters.
		// So don't try to look at PF bits. In GR.9 & 11, all playfield is underneath all players.

        for (i = 0; i < X8; i++)
        {
            BYTE b = rgpix[i+((NTSCx - X8)>>1)];

			// Thank you Altirra
			BYTE PRI0 = (sl.prior & 1) ? 0xff : 0;
			BYTE PRI1 = (sl.prior & 2) ? 0xff : 0;
			BYTE PRI2 = (sl.prior & 4) ? 0xff : 0;
			BYTE PRI3 = (sl.prior & 8) ? 0xff : 0;
			BYTE P0 = (b & bfPM0) ? 0xff : 0;
			BYTE P1 = (b & bfPM1) ? 0xff : 0;
			BYTE P2 = (b & bfPM2) ? 0xff : 0;
			BYTE P3 = (b & bfPM3) ? 0xff : 0;

			BYTE MULTI = (sl.prior & 32) ? 0xff : 0;
			
			BYTE PRI01 = PRI0 | PRI1;
			BYTE PRI12 = PRI1 | PRI2;
			BYTE PRI23 = PRI2 | PRI3;
			BYTE PRI03 = PRI0 | PRI3;
			BYTE P01 = P0 | P1;
			BYTE P23 = P2 | P3;

			// the rest of these values will be meaningless in GR. 9-11
			BYTE PF0 = (b & bfPF0) ? 0xff : 0;
			BYTE PF1 = (b & bfPF1) ? 0xff : 0;
			BYTE PF2 = (b & bfPF2) ? 0xff : 0;
			BYTE PF3 = (b & bfPF3) ? 0xff : 0;
			BYTE PF01 = PF0 | PF1;
			BYTE PF23 = PF2 | PF3;

			BYTE SP0 = P0 & ~(PF01 & PRI23) & ~(PRI2 & PF23);
			BYTE SP1 = P1 & ~(PF01 & PRI23) & ~(PRI2 & PF23) & (~P0 | MULTI);
			BYTE SP2 = P2 & ~P01 &  ~(PF23 & PRI12) &  ~(PF01 & ~PRI0);
			BYTE SP3 = P3 & ~P01 & ~(PF23 & PRI12) & ~(PF01 & ~PRI0) & (~P2 | MULTI);
			BYTE SF0 = PF0 & ~(P23 & PRI0) &  ~(P01 & PRI01) /* & ~SF3 meaning P5? */;
			BYTE SF1 = PF1 &  ~(P23 & PRI0) & ~(P01 & PRI01) /* & ~SF3 */;
			BYTE SF2 = PF2 &  ~(P23 & PRI03) & ~(P01 & ~PRI2) /* & ~SF3 */;
			BYTE SF3 = PF3 & ~(P23 & PRI03) & ~(P01 & ~PRI2);
			BYTE SB = ~P01 & ~P23 & ~PF01 & ~PF23;
			
			// !!! 5th PLAYER MODE BROKEN - it does not automatically go overtop all GR. 9-11 playfield (and probably other bugs)

			// GR. 9 - low nibble is the important LUM value. Now add back to chroma value
			if (sl.modelo == 16)
				if (P01 || P23)
					b = (P0 & pmg.colpm0) | (P1 & pmg.colpm1) | (P2 & pmg.colpm2) | (P3 & pmg.colpm3);
				else
					b = (b & 0x0f) | sl.colbk;

			// GR. 10 - low nibble is an index into the colour to use
			else if (sl.modelo == 17)
				if (P01 || P23)
					b = (P0 & pmg.colpm0) | (P1 & pmg.colpm1) | (P2 & pmg.colpm2) | (P3 & pmg.colpm3);
				else
					b = *(&COLPM0 + b);

			// GR. 11 - low nibble is the important CHROM value. We'll shift it back to the high nibble and add the LUM value later
			else if (sl.modelo == 18)
				if (P01 || P23)
					b = (P0 & pmg.colpm0) | (P1 & pmg.colpm1) | (P2 & pmg.colpm2) | (P3 & pmg.colpm3);
				else
					b = (b << 4) | sl.colbk;

			else
			{
				b = (SP0 & pmg.colpm0) | (SP1 & pmg.colpm1) | (SP2 & pmg.colpm2) | (SP3 & pmg.colpm3);
				b = b | (SF0 & sl.colpf0) | (SF1 & sl.colpf1) | (SF2 & sl.colpf2) | (SF3 & sl.colpf3) | (SB & sl.colbk);
			}

            qch[i] = b;
        }
    }

    // Check if mode status countdown is in effect

//    printf("cntTick = %d\n", cntTick);

    if (cntTick)
        {
        ShowCountDownLine(iVM);
        }

#if 0 // rainbow, was #ifndef NDEBUG

    // display the collision registers on each scan line
    //
    // collision registers consist of 8 bytes starting 
    // $D000 in the GTIA chip

    {
    BYTE *qch = vvmi.pvBits;

    int i;

    qch += (wScan - wStartScan) * vcbScan + vcbScan - 81;

    if (wScan == 128)
        {
        qch[-8] = 0xFF;
        qch[-7] = 0xFF;
        qch[-6] = 0xFF;
        qch[-5] = 0xFF;
        qch[-4] = 0xFF;
        }

    qch[-7] = 0;
    qch[-6] = COLBK;
    qch[-5] = COLPF0;
    qch[-4] = COLPF1;
    qch[-3] = COLPF2;
    qch[-2] = COLPF3;
    qch[-1] = 0;

    if (pmg.fHitclr)
        {
        pmg.fHitclr = 0;

        qch[-3] = 0x48;
        qch[-2] = 0x48;
        qch[-1] = 0x48;
        }

    for (i = 0; i < 16; i++)
        {
        int j, k;

        *qch++ = 0x48;

        k = cpuPeekB(readGTIA+i);

        for (j = 0; j < 4; j++)
            {
            *qch++ = (k & 8) ? 0xFF : ((i & ~3)<<4)+2;
            k <<= 1;
            }
        }

    *qch++ = 0x48;
    }
#endif // NDEBUG

Lnextscan:
    // move on to the next scan line

    fFetch = (iscan == scans);
    iscan = (iscan + 1) & 15;

    if (fFetch)
    {
		// ANTIC's PC can't cross a 4K boundary, poor thing
        wAddr = (wAddr & 0xF000) | ((wAddr + cbWidth) & 0x0FFF);
        fDataChanged = 0;
    }

#ifdef HWIN32
    return rgfChanged;
#endif
}

void ForceRedraw(int iVM)
{
	iVM;
    //memset(rgsl,0,sizeof(rgsl));
}

#endif // XFORMER

