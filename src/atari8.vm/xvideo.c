
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
    " ATARI 800 (NO CARTRIDGE)",
    " ATARI 800XL (NO CARTRIDGE)",
    " ATARI 130XE (NO CARTRIDGE)",
    " ATARI 800 + CARTRIDGE",
    " ATARI 800XL + CARTRIDGE",
    " ATARI 130XE + CARTRIDGE",
    };

#define vcbScan X8
#define wcScans Y8
#define NTSCx 512
#define NARROWx 256
#define NORMALx 320
#define WIDEx 384

BYTE rgpix[NTSCx];	// an entire NTSC scan line, including retrace areas

void ShowCountDownLine()
{
    WORD i = wScan - (wStartScan + wcScans - 12);

    // make sure bottom 8 scan lines

    if (i < 8)
        {
        BYTE *pch;
        int cch;
        BYTE *qch = vvmi.pvBits;
        BYTE colfg;

        if (cntTick < 2)
            {
            pch = "";
            }
        else if (cntTick < 60)
            {
            pch = " PRESS ALT+ENTER FOR FULLSCREEN";
            }
        else if (cntTick < 120)
            {
            pch = " XFORMER BY DAREK MIHOCKA";
            }
        else
            pch = (char *)rgszModes[mdXLXE + ((ramtop == 0xC000) ? 0 : 3)];
        qch += (wScan - wStartScan) * vcbScan;

        colfg = ((wFrame >> 2) << 4) + i + i;
        colfg = (((wFrame >> 2) + i) << 4) + 8;

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
            memset(rgsl,1,sizeof(rgsl));
            }

        if (i == 7)
            cntTick--;

#ifdef HWIN32
        if ((wScan >= wStartScan) && (wScan < (wStartScan+wcScans)))
            {
            wScanMin = min(wScanMin, (wScan - wStartScan));
            wScanMac = max(wScanMac, (wScan - wStartScan) + 1);
            }
#endif
        }
}

void DrawPlayers(WORD NEAR *pw)
{
    int i;
    int j;
    int cw;
    BYTE b2;
    WORD w;

    for (i = 0; i < 4; i++)
        {
        WORD NEAR *qw;
        
        if (!(b2 = pmg.grafp[i]))
            continue;

        qw = pw;
        qw += (pmg.hposp[i] - ((NTSCx-X8)>>2));

        cw = mpsizecw[ pmg.sizep[i] & 3];

        w  = mppmbw[i];

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

    for (i = 0; i < 4; i++)
        {
        WORD NEAR *qw;
        
        if (!(b2 = pmg.grafp[i]))
            continue;

        qw = pw;
		qw += (pmg.hposp[i] - ((NTSCx - X8) >> 2));

        cw = mpsizecw[ pmg.sizep[i] & 3];

        for (j = 0; j < 8; j++)
            {
            if (b2 & 0x80)
                {
                PXPL[i] |= (*qw >> 12);
                PXPF[i] |= *qw++;

                if (cw > 1)
                    {
                    PXPL[i] |= (*qw >> 12);
                    PXPF[i] |= *qw++;

                    if (cw > 2)
                        {
                        PXPL[i] |= (*qw >> 12);
                        PXPF[i] |= *qw++;
                        PXPL[i] |= (*qw >> 12);
                        PXPF[i] |= *qw++;
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

void DrawMissiles(WORD NEAR *pw, int fFifth)
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
        WORD NEAR *qw;
        
        if (!(b2 = ((pmg.grafm >> (i+i)) & 3)))
            continue;

        qw = (WORD NEAR *)pw;
        qw += (pmg.hposm[i] - ((NTSCx-X8)>>2));

        cw = mpsizecw[((pmg.sizem >> (i+i))) & 3];

        if (b2 & 2)
            {
            MXPL[i] |= (*qw >> 12);
            MXPF[i] |= *qw++;

            if (cw > 1)
                {
                MXPL[i] |= (*qw >> 12);
                MXPF[i] |= *qw++;

                if (cw > 2)
                    {
                    MXPL[i] |= (*qw >> 12);
                    MXPF[i] |= *qw++;
                    MXPL[i] |= (*qw >> 12);
                    MXPF[i] |= *qw++;
                    }
                }
            }
        else
            qw += cw;

        if (b2 & 1)
            {
            MXPL[i] |= (*qw >> 12);
            MXPF[i] |= *qw++;

            if (cw > 1)
                {
                MXPL[i] |= (*qw >> 12);
                MXPF[i] |= *qw++;

                if (cw > 2)
                    {
                    MXPL[i] |= (*qw >> 12);
                    MXPF[i] |= *qw++;
                    MXPL[i] |= (*qw >> 12);
                    MXPF[i] |= *qw;
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
        
        if (!(b2 = ((pmg.grafm >> (i+i)) & 3)))
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
            if (0 && fFifth)
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
            else
                {
                qw[0] = w;
                if (cw > 1)
                    {
                    qw[1] = w;
                    if (cw > 2)
                        {
                        qw[2] = w;
                        qw[3] = w;
                        }
                    }
                }
            }
        qw += cw;

        if (b2 & 1)
            {
            if (0 && fFifth)
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
            else
                {
                qw[0] = w;
                if (cw > 1)
                    {
                    qw[1] = w;
                    if (cw > 2)
                        {
                        qw[2] = w;
                        qw[3] = w;
                        }
                    }
                }
            }
        }
}

#pragma optimize("",on)

__inline void IncDLPC()
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

#ifdef HWIN32
BOOL ProcessScanLine(BOOL fRender)
#else
void ProcessScanLine(BOOL fRender)
#endif
{
	// scan lines per line of each graphics mode
    static const WORD mpMdScans[16] =
        {
        1, 1, 8, 10, 8, 16, 8, 16, 8, 4, 4, 2, 1, 2, 1, 1
        };

	// BYTES per line of each graphics mode for NARROW playfield
    static const WORD mpMdBytes[16] =
        {
        0, 0, 32, 32, 32, 32, 16, 16,	8, 8, 16, 16, 16, 32, 32, 32
        };

    // table that says how many bits of data to shift for each
    // 2 clock horizontal delay

    static const WORD mpmdbits[16] =
        {
        0, 0, 4, 4, 4, 4, 2, 2,    1, 1, 2, 2, 2, 2, 2, 2
        };

    int i, j;
    static WORD rgfChanged = 0;

    SL *psl = &rgsl[0];

    if (fRender)
        goto Lrender;

	// don't do anything in the invisible retrace sections
	if (wScan < wStartScan || wScan >= wStartScan + wcScans)
		return 0;

	// we are entering visible territory (240 lines are visible - usually 24 blank, 192 created by ANTIC, 24 blank)
    if (wScan == wStartScan)
    {
        fWait = 0;	// not doing JVB
        fFetch = 1;	// start by grabbing a DLIST instruction
	}

	// current scan line structure
	psl = &rgsl[wScan];

	// grab a DLIST instruction - not in JVB, done previous instruction, and ANTIC is on.
    if (!fWait && fFetch && (DMACTL & 32))
        {
#ifndef NDEBUG
        extern BOOL  fDumpHW;

        if (fDumpHW)
            printf("Fetching DL byte at scan %d, DLPC=%04X, byte=%02X\n",
                 wScan, DLPC, cpuPeekB(DLPC));
#endif

        sl.modehi = cpuPeekB(DLPC);
        sl.modelo = sl.modehi & 0x0F;
        sl.modehi >>= 4;
        IncDLPC();

		// vertical scroll bit enabled
        if ((sl.modehi & 2) && !sl.fVscrol && (sl.modelo >= 2))
            {
            sl.fVscrol = TRUE;
            sl.vscrol = VSCROL & 15;
			iscan = sl.vscrol;
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

            {
            WORD w;

            w = cpuPeekB(DLPC);
            IncDLPC();
            w |= (cpuPeekB(DLPC) << 8);

            DLPC = w;
            }
            break;

        default:
            // normal display list entry

			// how many scan lines a line of this graphics mode takes
            scans = mpMdScans[sl.modelo]-1;

			// LMS (load memory scan) attached to this line to give start of screen memory
            if (sl.modehi & 4)
                {
                wAddr = cpuPeekB(DLPC);
                IncDLPC();
                wAddr |= (cpuPeekB(DLPC) << 8);
                IncDLPC();
                }
            break;
            }

		// time to stop vscrol, this line doesn't use it
        if (sl.fVscrol && (!(sl.modehi & 2) || (sl.modelo < 2)))
            {
            sl.fVscrol = FALSE;
            scans = VSCROL;	// don't do this line like normal, finish the previous scroll?
            }
        }

    // generate a DLI if necessary. Do so on the last scan line of a graphics mode line
    if ((sl.modehi & 8) && (iscan == scans) && (NMIEN & 0x80))
        {
#ifndef NDEBUG
        extern BOOL  fDumpHW;

        if (fDumpHW)
            printf("DLI interrupt at scan %d\n", wScan);
#endif
        Interrupt();
          NMIST = 0x80 | 0x1F;
        regPC = cpuPeekW(0xFFFA);
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
        cbWidth = mpMdBytes[sl.modelo];		// can't boost any more, we're already WIDE
        cbDisp = cbWidth | (cbWidth >> 1);	// use WIDE numbers
        cbWidth |= (cbWidth >> 1);
        break;
        }

	// first scan line of a mode line, use the correct char set for this graphics mode line?
    if (iscan == sl.vscrol)
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
        sl.hscrol = HSCROL & 15;  // HACK: can't handle odd hscrol
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

    // enable PLAYER DMA and enable players
    if (sl.dmactl & 0x08 && GRACTL & 2)
        {
		// single line resolution
        if (sl.dmactl & 0x10)
            {
            pmg.grafp0 = cpuPeekB((pmg.pmbase << 8) + 1024 + wScan);
            pmg.grafp1 = cpuPeekB((pmg.pmbase << 8) + 1280 + wScan);
            pmg.grafp2 = cpuPeekB((pmg.pmbase << 8) + 1536 + wScan);
            pmg.grafp3 = cpuPeekB((pmg.pmbase << 8) + 1792 + wScan);
            }
		// double line resolution
        else
            {
            pmg.grafp0 = cpuPeekB((pmg.pmbase << 8) + 512 + (wScan>>1));
            pmg.grafp1 = cpuPeekB((pmg.pmbase << 8) + 640 + (wScan>>1));
            pmg.grafp2 = cpuPeekB((pmg.pmbase << 8) + 768 + (wScan>>1));
            pmg.grafp3 = cpuPeekB((pmg.pmbase << 8) + 896 + (wScan>>1));
            }
        GRAFPX = pmg.grafpX;
        }
    else
        {
        pmg.grafpX = GRAFPX;
#if 0
        pmg.pmbase = 0;
#endif
        }
	
	// enable MISSILE DMA and enable missiles
    if (sl.dmactl & 0x04 && GRACTL & 1)
        {
		// single res
        if (sl.dmactl & 0x10)
            pmg.grafm = cpuPeekB((pmg.pmbase << 8) + 768 + wScan);
        // double res
		else
            pmg.grafm = cpuPeekB((pmg.pmbase << 8) + 384 + (wScan>>1));

        GRAFM = pmg.grafm;
        }
    else
        pmg.grafm = GRAFM;

    if (pmg.grafpX)
        {
        // Players that are offscreen are treated as invisible
		// !!! 47 and 208 are the limits, make sure it doesn't encroach
        if ((pmg.hposp0 < 32) || (pmg.hposp0 >= 216))
            pmg.grafp0 = 0;
        if ((pmg.hposp1 < 32) || (pmg.hposp1 >= 216))
            pmg.grafp1 = 0;
        if ((pmg.hposp2 < 32) || (pmg.hposp2 >= 216))
            pmg.grafp2 = 0;
        if ((pmg.hposp3 < 32) || (pmg.hposp3 >= 216))
            pmg.grafp3 = 0;
        }

    if (pmg.grafm)
        {
        // Missiles that are offsecreen are treated as invisible
		// !!! 47 and 208 are the limits, make sure it doesn't encroach
        if ((pmg.hposm0 < 32) || (pmg.hposm0 >= 216))
            pmg.grafm &= ~0x03;
        if ((pmg.hposm1 < 32) || (pmg.hposm1 >= 216))
            pmg.grafm &= ~0x0C;
        if ((pmg.hposm2 < 32) || (pmg.hposm2 >= 216))
            pmg.grafm &= ~0x30;
        if ((pmg.hposm3 < 32) || (pmg.hposm3 >= 216))
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

    return 0;

Lrender:

    sl.colbk  = COLBK;
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

    // HACK! HACK! render everything all the time until the screen
    // corruption is figured out
    // !!! This makes it OK to not check CHBASE all the time in ExecuteAtari()
	// only do this if CHBASE changed for that scan line, we don't cache CHBASE per scan line
	rgfChanged |= fAll;

    // Even if rgfChanged is 0 at this point, we have to check the 10 to 48
    // data bytes associated with this scan line. If it is the first scan line
    // of this mode then we need to blit and compare the memory.
    // If it isn't the first scan line and !fDataChanged then there is no
    // need to blit and compare since nothing changed since the first scan line.

    if (((sl.modelo) >= 2) && (rgfChanged || (iscan == sl.vscrol) || (fDataChanged)))
        {
        WORD j = 0;

        // HSCROL - we read more bytes than we display. View the center portion of what we read, offset cbDisp/10 bytes into the scan line

        if (cbDisp != cbWidth)
            {
			// wide is 48, normal is 40, that's j=4 (split the difference) characters before we begin
			// subtract one for every multiple of 8 hshift is, that's an entire character shift
            j = (cbDisp>>3) - (cbDisp>>5) - ((hshift) >> 3);
            hshift &= 7;	// now only consider the part < 8
            }

        if (fDataChanged)
            {
            // sl.rgb already has data from previous scan line
            }
        else if (((wAddr+j) & 0xFFF) < 0xFD0)
            _fmemcpy(sl.rgb,&rgbMem[wAddr+j],cbWidth);
        else
            {
            for (i = 0; i < cbWidth; i++)
                sl.rgb[i] = cpuPeekB((wAddr & 0xF000) | ((wAddr+i+j) & 0x0FFF));
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

        BYTE *qch0 = vvmi.pvBits;
        BYTE FAR *qch = qch0;

        BYTE b1, b2;
        BYTE col0, col1, col2, col3;
        BYTE vpix = iscan;

#ifndef NDEBUG
        // show current stack
        // !!! annoying debug pixels near top of screen qch[regSP & 0xFF] = (BYTE)(cpuPeekB(regSP | 0x100) ^ wFrame);
#endif

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

            qch = &rgpix[(NTSCx-X8)>>1];	// first visible screen NTSC pixel
            _fmemset(rgpix, 0, sizeof(rgpix));

            sl.colbk  = bfBK;
            sl.colpf0 = bfPF0;
            sl.colpf1 = bfPF1;
            sl.colpf2 = bfPF2;
            sl.colpf3 = bfPF3;
            }
        else
            qch += (wScan - wStartScan) * vcbScan;

		// narrow playfield
        if ((sl.modelo >= 2) && ((sl.dmactl & 3) == 1))
        {
            if (rgfChanged & (fPMG|fDmactl|fModelo|fColbk))
                {
                memset(qch,sl.colbk,(X8-NARROWx)>>1);		// background colour before
                memset(qch+X8-((X8-NARROWx)>>1),sl.colbk,(X8-NARROWx)>>1);	// background colour after
                }
            qch += (X8-NARROWx)>>1;
        }
		
		// normal playfield is 320 wide
		if ((sl.modelo >= 2) && ((sl.dmactl & 3) == 2))
		{
			if (rgfChanged & (fPMG | fDmactl | fModelo | fColbk))
			{
				memset(qch, sl.colbk, (X8 - NORMALx) >> 1);		// background colour before
				memset(qch + X8-((X8-NORMALx)>>1), sl.colbk, (X8 - NORMALx) >> 1);	// background colour after
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

            // just draw a blank scan line

            _fmemset(qch, sl.colbk, X8);
            break;

        case 2:
            col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);
            col2 = sl.colpf2;

            if (sl.chactl & 4)
                vpix = vpix ^ 7;    // vertical reflect bit
            vpix &= 7;

            for (i = 0 ; i < cbDisp; i++, qch += 8)
                {
                BYTE rgch[8];

                if (((b1 = sl.rgb[i]) == psl->rgb[i]) && !rgfChanged)
                    continue;

                if (hshift)
                {
                    // non-zero horizontal scroll

					ULONGLONG u, v;

					v = cpuPeekB((sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix);
					if (b1 & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							v = 0;
						if (sl.chactl & 2)  // inverse flag
							v = ~v & 0xff;
					}
					u = v;
					
					v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 1] & 0x7f) << 3) + vpix);
					if (sl.rgb[i-1] & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							v = 0;
						if (sl.chactl & 2)  // inverse flag
							v = ~v & 0xff;
					}
					u |= (v << 8);

					v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 2] & 0x7f) << 3) + vpix);
					if (sl.rgb[i - 2] & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							v = 0;
						if (sl.chactl & 2)  // inverse flag
							v = ~v & 0xff;
					}
					u |= (v << 16);

					v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 3] & 0x7f) << 3) + vpix);
					if (sl.rgb[i - 3] & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							v = 0;
						if (sl.chactl & 2)  // inverse flag
							v = ~v & 0xff;
					}
					u |= (v << 24);

					v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 4] & 0x7f) << 3) + vpix);
					if (sl.rgb[i - 4] & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							v = 0;
						if (sl.chactl & 2)  // inverse flag
							v = ~v & 0xff;
					}
					u |= (v << 32);

                    b2 = (BYTE)(u >> hshift);
                }
				else
				{
					b2 = cpuPeekB((sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix);
					if (b1 & 0x80)
					{
						if (sl.chactl & 1)  // blank (blink) flag
							b2 = 0;
						if (sl.chactl & 2)  // inverse flag
							b2 = ~b2;
					}
				}

//    col2 ^= cpuPeekB(20);

                if (b2 == 0)
                    {
                    memset(qch,col2,8);
                    continue;
                    }

                if (b2 == 255)
                    {
                    memset(qch,col1,8);
                    continue;
                    }

                memset(rgch,col2,8);

                if (b2 & 0x80)
                    rgch[0] = col1;

                if (b2 & 0x40)
                    rgch[1] = col1;

                if (b2 & 0x20)
                    rgch[2] = col1;

                if (b2 & 0x10)
                    rgch[3] = col1;

                if (b2 & 0x08)
                    rgch[4] = col1;

                if (b2 & 0x04)
                    rgch[5] = col1;

                if (b2 & 0x02)
                    rgch[6] = col1;

                if (b2 & 0x01)
                    rgch[7] = col1;

                _fmemcpy(qch,rgch,8);
                }
            break;

        case 3:
            if (sl.chactl & 4)
                vpix = 9 - vpix;    // vertical reflect bit
            vpix %= 10;

            for (i = 0 ; i < cbDisp; i++)
                {
                if (((b1 = sl.rgb[i]) != psl->rgb[i]) || rgfChanged)
                    {
                    if (b1 & 0x80)
                        {
                        if (sl.chactl & 2)  // inverse flag
                            col2 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);
                        else
                            col2 = sl.colpf2;

                        if (sl.chactl & 1)  // blank (blink) flag
                            col2 = sl.colpf2;

                        col1 = sl.colpf2;
                        b1 &= 0x7F;
                        }
                    else
                        {
                        col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);
                        col2 = sl.colpf2;
                        }

                    if (hshift)
                        {
                        // non-zero horizontal scroll

                        WORD u;

                        u = (cpuPeekB((sl.chbase<<8) + (b1<<3) + vpix) << 8) |
                             cpuPeekB((sl.chbase<<8) + (sl.rgb[i+1]<<3) + vpix);

                         // BUG: not checking inverse bit of sl.rgb[i+1]
                         // BUG: not checking alignment

                        b2 = (BYTE)(u >> hshift);
                        }
                    else if ((vpix >= 2) && (vpix < 8))
                        b2 = cpuPeekB(((sl.chbase & 0xFC) << 8) + (b1 << 3) + vpix);
                    else if ((vpix < 2) && (b1 < 0x60))
                        b2 = cpuPeekB(((sl.chbase & 0xFC) << 8) + (b1 << 3) + vpix);
                    else if ((vpix >= 8) && (b1 >= 0x60))
                        b2 = cpuPeekB(((sl.chbase & 0xFC) << 8) + (b1 << 3) + vpix - 8);
                    else
                        b2 = 0;

                    for (j = 0; j < 8; j++)
                        {
                        if (b2 & 0x80)
                            *qch++ = col1;
                        else
                            *qch++ = col2;
                        b2 <<= 1;
                        }
                    }
                else
                    {
                    qch += 8;
                    }
                }
            break;

        case 5:
            vpix = iscan >> 1;
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

                    WORD u;

                    u = (cpuPeekB((sl.chbase<<8) + ((b1 & 0x7F)<<3) + vpix) << 8) |
                         cpuPeekB((sl.chbase<<8) + ((sl.rgb[i+1] & 0x7F)<<3) + vpix);

                    b2 = (BYTE)(u >> hshift);
                    }
                else
                    b2 = cpuPeekB((sl.chbase<<8) + ((b1 & 0x7F)<<3) + vpix);

                if (b1 & 0x80)
                    col3 = sl.colpf3;
                else
                    col3 = sl.colpf2;

                for (j = 0; j < 4; j++)
                    {
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

						ULONGLONG u, v;

						v = cpuPeekB((sl.chbase << 8) + ((b1 & 0x3F) << 3) + vpix);
						u = v;

						v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 1] & 0x3f) << 3) + vpix);
						u |= (v << 8);

						v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 2] & 0x3f) << 3) + vpix);
						u |= (v << 16);

						v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 3] & 0x3f) << 3) + vpix);
						u |= (v << 24);

						v = cpuPeekB((sl.chbase << 8) + ((sl.rgb[i - 4] & 0x3f) << 3) + vpix);
						u |= (v << 32);

						b2 = (BYTE)(u >> hshift);
					
                        for (j = 0; j < 8; j++)
                            {
                            if (j <= hshift)
                                col1 = sl.colpf[sl.rgb[i-1]>>6];
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
                        b2 = cpuPeekB((sl.chbase << 8)
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

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 32;
                    continue;
                    }

                if (hshift)
                    {
                    // non-zero horizontal scroll

                    WORD u = (b2 << 8) | sl.rgb[i+1];
                    b2 = (BYTE)(u >> hshift);
                    }

                for (j = 0; j < 4; j++)
                    {
                    switch (b2 & 0xC0)
                        {
                    default:
                        Assert(FALSE);
                        break;
                
                    case 0x00:
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        break;

                    case 0x40:
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        break;

                    case 0x80:
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        break;

                    case 0xC0:
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        break;
                        }
                    b2 <<= 2;
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

                if (hshift)
                    {
                    // non-zero horizontal scroll

                    WORD u = (b2 << 8) | sl.rgb[i+1];
                    b2 = (BYTE)(u >> hshift);
                    }

                for (j = 0; j < 8; j++)
                    {
                    if (b2 & 0x80)
                        {
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        }
                    else
                        {
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        }
                    b2 <<= 1;
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

                if (hshift)
                    {
                    // non-zero horizontal scroll

                    WORD u = (b2 << 8) | sl.rgb[i+1];
                    b2 = (BYTE)(u >> hshift);
                    }

                for (j = 0; j < 4; j++)
                    {
                    switch (b2 & 0xC0)
                        {
                    default:
                        Assert(FALSE);
                        break;
                
                    case 0x00:
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        *qch++ = col0;
                        break;

                    case 0x40:
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        *qch++ = col1;
                        break;

                    case 0x80:
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        *qch++ = col2;
                        break;

                    case 0xC0:
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        *qch++ = col3;
                        break;
                        }
                    b2 <<= 2;
                    }
                }
            break;

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

                if (hshift)
                    {
                    // non-zero horizontal scroll

                    WORD u = (b2 << 8) | sl.rgb[i+1];
                    b2 = (BYTE)(u >> hshift);
                    }

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

                if (hshift)
                    {
                    // non-zero horizontal scroll

                    WORD u = (b2 << 8) | sl.rgb[i+1];
                    b2 = (BYTE)(u >> hshift);
                    }

                for (j = 0; j < 4; j++)
                    {
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

        case 15:
            col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);
            col2 = sl.colpf2;

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

                if (hshift)
                    {
                    // non-zero horizontal scroll

                    WORD u = (b2 << 8) | sl.rgb[i+1];
                    b2 = (BYTE)(u >> hshift);
                    }

                if (b2 == 0)
                    {
                    memset(qch,col2,8);
                    qch += 8;
                    continue;
                    }

                if (b2 == 255)
                    {
                    memset(qch,col1,8);
                    qch += 8;
                    continue;
                    }

                for (j = 0; j < 8; j++)
                    {
                    if (b2 & 0x80)
                        *qch++ = col1;
                    else
                        *qch++ = col2;
                    b2 <<= 1;
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

                col1 = (b2 >> 4) | ((sl.colbk & 15) << 4);
                col2 = (b2 & 15) | ((sl.colbk & 15) << 4);

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
            // GTIA 9 color mode

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

                col1 = (b2 >> 4);

                if (col1 < 9)
                    {
                    col1 = *(((BYTE FAR *)&COLPM0) + col1);
                    }
                else
                    col1 = 0;

                col2 = (b2 & 15);

                if (col2 < 9)
                    {
                    col2 = *(((BYTE FAR *)&COLPM0) + col2);
                    }
                else
                    col2 = 0;

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
            // GTIA 16 color mode

            for (i = 0 ; i < cbDisp; i++)
                {
                b2 = sl.rgb[i];

                if (!rgfChanged && (b2 == psl->rgb[i]))
                    {
                    qch += 8;
                    continue;
                    }

                col1 = ((b2 >> 4) << 4) | (sl.colbk & 15);
                col2 = ((b2 & 15) << 4) | (sl.colbk & 15);

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

    sl.colbk  = COLBK;
    sl.colpfX = COLPFX;

    if (rgfChanged || fDataChanged)
        *psl = sl;

    if ((wScan < wStartScan) || (wScan >= (wStartScan+wcScans)))
        goto Lnextscan;

    if (sl.fpmg)
        {
        BYTE *qch = vvmi.pvBits;

		// now set the bits in rgpix corresponding to players and missiles
		// missiles go underneath the players, at least in Ms. Pacman. !!! RIVERAID breaks if missiles drawn first, no missile collision
		DrawPlayers((WORD NEAR *)(rgpix + ((NTSCx - X8)>>1)));
		DrawMissiles((WORD NEAR *)(rgpix + ((NTSCx - X8)>>1)), sl.prior & 16);

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

        // if in a 2 color mode, set playfield 1 color

        if ((sl.modelo == 2) || (sl.modelo == 3) || (sl.modelo == 15))
            sl.colpf1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);

        if ((wScan < wStartScan) || (wScan >= (wStartScan+wcScans)))
            goto Lnextscan;

        // now map the rgpix array to the screen

        qch += (wScan - wStartScan) * vcbScan;

        // if GTIA, give players and missiles highest priority

        if (sl.modelo > 15)
            sl.prior = 1;

        for (i = 0; i < X8; i++)
            {
            BYTE b = rgpix[i+((NTSCx - X8)>>1)];

            if (b == 0)
                {
                // just background

                if (sl.modelo <= 15)
                    b = sl.colbk;
                }
            else if ((b & 0xF0) == 0)
                {
                // just playfield, no missiles

                if (sl.modelo > 15)
                    ;
                else if (b & bfPF0)
                    b = sl.colpf0;
                else if (b & bfPF1)
                    b = sl.colpf1;
                else if (b & bfPF2)
                    b = sl.colpf2;
                else
                    b = sl.colpf3;
                }
            else
                {
                switch (sl.prior & 15)
                    {
                case 0:
                case 1:   // P0 P1 P2 P3 PF0 PF1 PF2 PF3
LdrawPM01:
                    if ((sl.prior & 32) &&
                           ((b & (bfPM0|bfPM1)) == (bfPM0|bfPM1)))
                        b = pmg.colpm0 | pmg.colpm1;
                    else if (b & bfPM0)
                        b = pmg.colpm0;
                    else if (b & bfPM1)
                        b = pmg.colpm1;
                    else
                        {
LdrawPM23:
                        if ((sl.prior & 32) &&
                               ((b & (bfPM2|bfPM3)) == (bfPM2|bfPM3)))
                            b = pmg.colpm2 | pmg.colpm3;
                        else if (b & bfPM2)
                            b = pmg.colpm2;
                        else if (b & bfPM3)
                            b = pmg.colpm3;
                        }
                    break;

                case 2:
                    if ((sl.prior & 32) &&
                           ((b & (bfPM0|bfPM1)) == (bfPM0|bfPM1)))
                        b = pmg.colpm0 | pmg.colpm1;
                    else if (b & bfPM0)
                        b = pmg.colpm0;
                    else if (b & bfPM1)
                        b = pmg.colpm1;
                    else if (b & bfPF0)
                        b = sl.colpf0;
                    else if (b & bfPF1)
                        b = sl.colpf1;
                    else if (b & bfPF2)
                        b = sl.colpf2;
                    else if (b & bfPF3)
                        b = sl.colpf3;
                    else
                        goto LdrawPM23;
                    break;

                case 3:
                case 4:
                    if (b & bfPF0)
                        b = sl.colpf0;
                    else if (b & bfPF1)
                        b = sl.colpf1;
                    else if (b & bfPF2)
                        b = sl.colpf2;
                    else if (b & bfPF3)
                        b = sl.colpf3;
                    else
                        goto LdrawPM01;
                    break;

                default:
                case 8:
                    if (b & bfPF0)
                        b = sl.colpf0;
                    else if (b & bfPF1)
                        b = sl.colpf1;
                    else if ((sl.prior & 32) &&
                           ((b & (bfPM0|bfPM1)) == (bfPM0|bfPM1)))
                        b = pmg.colpm0 | pmg.colpm1;
                    else if (b & bfPM0)
                        b = pmg.colpm0;
                    else if (b & bfPM1)
                        b = pmg.colpm1;
                    else if ((sl.prior & 32) &&
                           ((b & (bfPM2|bfPM3)) == (bfPM2|bfPM3)))
                        b = pmg.colpm2 | pmg.colpm3;
                    else if (b & bfPM2)
                        b = pmg.colpm2;
                    else if (b & bfPM3)
                        b = pmg.colpm3;
                    else if (b & bfPF2)
                        b = sl.colpf2;
                    else if (b & bfPF3)
                        b = sl.colpf3;
                    break;
                    }
                }

            qch[i] = b;
            }
        }

    // Check if mode status countdown is in effect

//    printf("cntTick = %d\n", cntTick);

    if (cntTick)
        {
        ShowCountDownLine();
        }

#if 0 // !!! rainbow #ifndef NDEBUG

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

void ForceRedraw()
{
    memset(rgsl,0,sizeof(rgsl));
}

#endif // XFORMER

