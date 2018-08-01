
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

// size of PMG
const WORD mpsizecw[4] = { 1, 2, 1, 4 };

// bits each player (incl. 5th player) use to indicate their presence in the bitfield
const BYTE mppmbw[5] =
    {
    bfPM0,
    bfPM1,
    bfPM2,
    bfPM3,
    bfPF3,
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

#define vcbScan X8      // NTSC and PAL widths are the same
#define wcScans Y8      // # of valid scan lines, NTSC or PAL is the same
#define NTSCx 512       // TRIVIA: only 456 pixels take time to draw, the other 56 are instantaneous and no CPU time elapses.
                        // see ATARI800.c rgDMAx[] comment for explanation. Horizontal width is same for PAL
#define NARROWx 256
#define NORMALx 320
#define WIDEx 384

// scan lines per line of each graphics mode
static const WORD mpMdScans[19] =
{
    1, 1, 8, 10, 8, 16, 8, 16, 8, 4, 4, 2, 1, 2, 1, 1, 1, 1, 1
};

// BYTES per line of each graphics mode for NARROW playfield
static const WORD mpMdBytes[19] =
{
    0, 0, 32, 32, 32, 32, 16, 16,    8, 8, 16, 16, 16, 32, 32, 32, 32, 32, 32
};

// table that says how many bits of data to shift for each
// 2 clock horizontal delay

static const WORD mpmdbits[19] =
{
    0, 0, 4, 4, 4, 4, 2, 2,    1, 1, 2, 2, 2, 4, 4, 4, 4, 4, 4
};

static const ULONG BitsToArrayMask[16] =
{
    // The lookup table to map to a little-endian bit mask to little-endian byte mask

    0x00000000,
    0x000000FF,
    0x0000FF00,
    0x0000FFFF,
    0x00FF0000,
    0x00FF00FF,
    0x00FFFF00,
    0x00FFFFFF,
    0xFF000000,
    0xFF0000FF,
    0xFF00FF00,
    0xFF00FFFF,
    0xFFFF0000,
    0xFFFF00FF,
    0xFFFFFF00,
    0xFFFFFFFF,
};

static const ULONG BitsToByteMask[16] =
{
    // 6502 is big endian so it renders pixels in a left-to-right bit ordering.
    // The lookup table to map to a little-endian mask for x86/ARM is therefore byte swapped!

    0x00000000,
    0xFF000000,
    0x00FF0000,
    0xFFFF0000,
    0x0000FF00,
    0xFF00FF00,
    0x00FFFF00,
    0xFFFFFF00,
    0x000000FF,
    0xFF0000FF,
    0x00FF00FF,
    0xFFFF00FF,
    0x0000FFFF,
    0xFF00FFFF,
    0x00FFFFFF,
    0xFFFFFFFF,
};

void ShowCountDownLine(void *candy)
{
    int i = wScan - (STARTSCAN + wcScans - 12);

    // make sure bottom 8 scan lines

    if (i < 8 && i >= 0)
    {
        char *pch;
        int cch;
        BYTE *qch;

        if (v.fTiling && !v.fMyVideoCardSucks)
        {
            qch = vvmhw.pTiledBits;
            qch += pvmin->sRectTile.top * sStride + pvmin->sRectTile.left;
        }
        else
            // User screen buffer # = to which visible tile we are!
            qch = vvmhw.pbmTile[pvmin->iVisibleTile].pvBits;
        
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

        if (v.fTiling && !v.fMyVideoCardSucks)
        {
            qch += ((wScan - STARTSCAN) * sStride);
            if (qch < (BYTE *)(vvmhw.pTiledBits) || qch >= (BYTE *)(vvmhw.pTiledBits) + sStride * sRectC.bottom)
                return;
        }
        else
            qch += ((wScan - STARTSCAN) * vcbScan);

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

    }
}

// make the tables of where the electron beam is for each CPU cycle. Lookup table is the only acceptable fast way of figuring this out.

void CreateDMATables()
{
    // First, simply note whether the CPU is blocked on each cycle of each type of scan line... 1 means CPU blocked, a 0 means CPU free

    BYTE mode, pf, width, first, player, missile, lms, cycle;

    for (mode = 0; mode < 20; mode++)
    {
        for (pf = 0; pf < 2; pf++)
            for (width = 0; width < 3; width++)
                for (first = 0; first < 2; first++)
                    for (player = 0; player < 2; player++)
                        for (missile = 0; missile < 2; missile++)
                            for (lms = 0; lms < 2; lms++)
                                for (cycle = 0; cycle < HCLOCKS; cycle++)
                                {
                                    // assume free
                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 0;

                                    // CHARACTER MODE
                                    if (mode < 8)
                                    {
                                        // this cycle is for missile DMA? Then all the tables with missile DMA on have it blocked
                                        // in double line resolution, there is still a DMA fetch every line, stupid ANTIC
                                        if (rgDMAC[cycle] == DMA_M)
                                        {
                                            if (missile)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;

                                        // same for player DMA
                                        }
                                        else if (rgDMAC[cycle] == DMA_P)
                                        {
                                            if (player)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }

                                        // nothing else halts the CPU if Playfield DMA is off

                                        if (pf)
                                        {
                                            // mode fetch will happen on the first scan line of a mode
                                            if (rgDMAC[cycle] == DMA_DL)
                                            {
                                                if (first)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // this cycle is blocked by a load memory scan (first scan line only)
                                            else if (rgDMAC[cycle] == DMA_LMS)
                                            {
                                                if (lms && first)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // wide playfield hi and med res modes use this cycle on the first scan line
                                            else if (rgDMAC[cycle] == W4)
                                            {
                                                if (first && mode >= 2 && width == 2)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // wide playfield hi res modes use this cycle on the first scan line
                                            else if (rgDMAC[cycle] == W2)
                                            {
                                                if (first && width == 2 && mode >= 2 && mode <= 5)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // wide playfield character modes use this cycle on every scan line to get char data
                                            else if (rgDMAC[cycle] == WC4)
                                            {
                                                if (width == 2 && mode >= 2)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // wide playfield hi-res character modes use this cycle on every scan line to get char data
                                            else if (rgDMAC[cycle] == WC2)
                                            {
                                                if (width == 2 && mode >= 2 && mode <= 5)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // NORMAL - all but narrow playfield hi and med res modes use this cycle on the first scan line
                                            else if (rgDMAC[cycle] == N4)
                                            {
                                                if (first && mode >= 2 && width > 0)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all but narrow playfield hi res modes use this cycle on the first scan line
                                            else if (rgDMAC[cycle] == N2)
                                            {
                                                if (first && width > 0 && mode >= 2 && mode <= 5)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all but narrow playfield character modes use this cycle on every scan line to get char data
                                            else if (rgDMAC[cycle] == NC4)
                                            {
                                                if (width > 0 && mode >= 2)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all but wide playfield hi-res character modes use this cycle on every scan line to get char data
                                            else if (rgDMAC[cycle] == NC2)
                                            {
                                                if (width > 0 && mode >= 2 && mode <= 5)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield hi and med res modes use this cycle on the first scan line
                                            else if (rgDMAC[cycle] == A4)
                                            {
                                                if (first && mode >= 2)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield hi res modes use this cycle on the first scan line
                                            else if (rgDMAC[cycle] == A2)
                                            {
                                                if (first && mode >= 2 && mode <= 5)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield character modes use this cycle on every scan line to get char data
                                            else if (rgDMAC[cycle] == AC4)
                                            {
                                                if (mode >= 2)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield hi-res character modes use this cycle on every scan line to get char data
                                            else if (rgDMAC[cycle] == AC2)
                                            {
                                                if (mode >= 2 && mode <= 5)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                        }
                                    }

                                    // NON CHARACTER MODE
                                    else
                                    {
                                        // this cycle is for missile DMA? Then all the tables with missile DMA on have it blocked
                                        // in double line resolution, there is still a DMA fetch every line
                                        if (rgDMANC[cycle] == DMA_M)
                                        {
                                            if (missile)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;

                                            // same for player DMA
                                        }
                                        else if (rgDMANC[cycle] == DMA_P)
                                        {
                                            if (player)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }

                                        // nothing else halts the CPU if Playfield DMA is off

                                        if (pf)
                                        {
                                            // mode fetch will happen on the first scan line of a mode
                                            if (rgDMANC[cycle] == DMA_DL)
                                            {
                                                if (first)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // this cycle is blocked by a load memory scan (first scan line only)
                                            else if (rgDMANC[cycle] == DMA_LMS)
                                            {
                                                if (lms && first)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all wide playfield modes (>= 2) use this cycle on the first scan line
                                            // (GTIA 9++ is an example of bitmap modes that actually do have subsequent scan lines
                                            //  where DMA should not occur)
                                            else if (rgDMANC[cycle] == W8)
                                            {
                                                if (first && width == 2)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // wide playfield hi and med res modes use this cycle on the first scan line
                                            else if (rgDMANC[cycle] == W4)
                                            {
                                                if (first && width == 2 && mode != 8 && mode != 9)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // wide playfield hi res modes use this cycle on the first scan line
                                            else if (rgDMANC[cycle] == W2)
                                            {
                                                if (first && width == 2 && mode >= 13 && mode <= 18)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all but wide playfield modes (>= 2) use this one on the first scan line
                                            else if (rgDMANC[cycle] == N8)
                                            {
                                                if (first && width > 0)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all but wide playfield hi and med res modes use this cycle on the first scan line
                                            else if (rgDMANC[cycle] == N4)
                                            {
                                                if (first && width > 0 && mode != 8 && mode != 9)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all but wide playfield hi res modes use this cycle on the first scan line
                                            else if (rgDMANC[cycle] == N2)
                                            {
                                                if (first && width > 0 && mode >= 13 && mode <= 18)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield modes (>= 2) use this one on the first scan line
                                            else if (rgDMANC[cycle] == A8)
                                            {
                                                if (first)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield hi and med res modes use this cycle on the first scan line
                                            else if (rgDMANC[cycle] == A4)
                                            {
                                                if (first && mode != 8 && mode != 9)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                            // all playfield hi res modes use this cycle on the first scan line
                                            else if (rgDMANC[cycle] == A2)
                                            {
                                                if (first && mode >= 13 && mode <= 18)
                                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                            }
                                        }
                                    }
                                }
    }

    // Build an array that says given how many CPU cycles we have left to execute, what clock cycle we will be be on (0-113).
    // rgDMAMap will look like this, for instance:
    // 113 110 105 ... 2 0(at index 60)
    // meaning that when there are 60 cpu cycles left to execute on this scan line, we are at clock 0 of the scan line.
    // When there is 1 cpu cycle left to execute, we will be at clock 110.
    //
    // We will have a table to convert horizontal clock cycle to screen pixels
    //
    // rgDMAMap index 114 will tell you how many CPU cycles can execute on this scan line so you can set the initial wLeft
    // Index 115 will tell you what wLeft should be set to after WSYNC is released (cycle 105)
    // Index 116 will tell you what wLeft will be on cycle 10, when it's time for an NMI (DLI/VBI)
    //

    for (mode = 0; mode < 20; mode++)
    {
        for (pf = 0; pf < 2; pf++)
            for (width = 0; width < 3; width++)
                for (first = 0; first < 2; first++)
                    for (player = 0; player < 2; player++)
                        for (missile = 0; missile < 2; missile++)
                            for (lms = 0; lms < 2; lms++)
                            {
                                // Now block out 9 RAM refresh cycles, every 4 cycles starting at 25.
                                // They can be bumped up to the next cycle's start point. The last one might be bumped all the way to 106
                                // (If there are no free cycles between 25 and 60, you just get one RAM refresh at the end)
                                for (cycle = 0; cycle < 9; cycle++)
                                {
                                    for (int xx = 0; xx < 4; xx++)
                                    {
                                        if (rgDMAMap[mode][pf][width][first][player][missile][lms][25 + xx + 4 * cycle] == 0)
                                        {
                                            rgDMAMap[mode][pf][width][first][player][missile][lms][25 + xx + 4 * cycle] = 1;
                                            break;
                                        }
                                        if (cycle == 8 && xx == 3)
                                        {
                                            // a few modes are so contended there's only 1 RAM refresh
                                            if (width == 0)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][90] = 1;
                                            else if (width == 1)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][98] = 1;
                                            else
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][106] = 1;
                                        }
                                    }
                                }

                                rgDMAMap[mode][pf][width][first][player][missile][lms][115] = 0;    // clear WSYNC point
                                rgDMAMap[mode][pf][width][first][player][missile][lms][116] = 0;    // clear DLI/VBI point

                                char array[HCLOCKS];
                                char index = HCLOCKS;

                                // Calculate the CPU cycle that will be executing at each clock cycle of the scan line.
                                // There are 0 CPU cycles left to execute at cycle 113, the last cycle.
                                // Back up from there to the next free cycle that the CPU can use, and
                                // that will be where there is 1 CPU cycle left, then 2, etc.
                                for (cycle = 0; cycle < HCLOCKS; cycle++)
                                {
                                    // go backwards, find a free cycle
                                    do {
                                        index--;
                                        if (index < 0)
                                            break;
                                    } while (rgDMAMap[mode][pf][width][first][player][missile][lms][index] == 1);

                                    if (index < 0)
                                        break;

                                    // we found a free cycle.
                                    array[cycle] = index;

                                    // index 115 - remember what wLeft should jump to on a WSYNC (cycle 105)
                                    if (index == 105)
                                        rgDMAMap[mode][pf][width][first][player][missile][lms][115] = cycle;
                                    // oops, 105 was busy
                                    else if (index < 105 && rgDMAMap[mode][pf][width][first][player][missile][lms][115] == 0)
                                        rgDMAMap[mode][pf][width][first][player][missile][lms][115] = cycle - 1;

                                    // index 116 - remember what wLeft will be when it's time for a DLI/VBI NMI (cycle 10)
                                    // (it takes 7 cycles for the CPU to process the NMI and start executing code, so
                                    // we won't run until cycle 17, see x6502.c. Decathalon glitches if we start the DLI earlier).
                                    if (index == 10)
                                        rgDMAMap[mode][pf][width][first][player][missile][lms][116] = cycle;
                                    // oops, 10 was busy
                                    else if (index < 10 && rgDMAMap[mode][pf][width][first][player][missile][lms][116] == 0)
                                        rgDMAMap[mode][pf][width][first][player][missile][lms][116] = cycle - 1;
                                }

                                // This is what wLeft should start at for this kind of line (plus one, since wLeft is one-based)
                                rgDMAMap[mode][pf][width][first][player][missile][lms][HCLOCKS] = cycle - 1;

                                // copy the temp array over to the permanent array
                                for (cycle = 0; cycle < HCLOCKS; cycle++)
                                {
                                    rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = array[cycle];
                                }
                            }
    }

    // now make a conversion table of clock cycles to pixels... We start drawing at cycle 13 through 100 and there are 4 pixels per cycle
    for (cycle = 0; cycle < HCLOCKS; cycle++)
    {
        if (cycle < 14)
            rgPIXELMap[cycle] = 0;
        else if (cycle >= 101)
            rgPIXELMap[cycle] = X8;
        else
            rgPIXELMap[cycle] = (cycle - 13) << 2;
    }
}


// make the tables of what is visible based on PMG priorities

void CreatePMGTable()
{
    for (int z = 0; z < 65536; z++)
    {
        // cycle through all possible bitfields of which PM/PF's are present (low byte) and what the priority is set to (high byte)
        BYTE b = z & 0xff;
        BYTE prior = (BYTE)(z >> 8);

        // This table will effectively remove all the PM/PF's from the bitfield that will not be visible, based on priority

        BYTE PRI0 = (prior & 1) ? 0xff : 0;
        BYTE PRI1 = (prior & 2) ? 0xff : 0;
        BYTE PRI2 = (prior & 4) ? 0xff : 0;
        BYTE PRI3 = (prior & 8) ? 0xff : 0;
        BYTE PRI01 = PRI0 | PRI1;
        BYTE PRI12 = PRI1 | PRI2;
        BYTE PRI23 = PRI2 | PRI3;
        BYTE PRI03 = PRI0 | PRI3;
        BYTE MULTI = (prior & 32) ? 0xff : 0;
        
        BYTE P0 = (b & bfPM0);
        BYTE P1 = (b & bfPM1);
        BYTE P2 = (b & bfPM2);
        BYTE P3 = (b & bfPM3);
        BYTE P01 = (b & (bfPM0 | bfPM1)) ? 0xff : 0;
        BYTE P23 = (b & (bfPM2 | bfPM3)) ? 0xff : 0;
    
        // no worrying about playfield priorities in GR. 9-11
        if (!(prior & 0xc0))
        {
            BYTE NOTP0 = (b & bfPM0) ? 0 : 0xff;
            BYTE NOTP2 = (b & bfPM2) ? 0 : 0xff;

            BYTE PF0 = (b & bfPF0);
            BYTE PF1 = (b & bfPF1);
            BYTE PF2 = (b & bfPF2);
            BYTE PF3 = (b & bfPF3);

            BYTE PF01 = (b & (bfPF0 | bfPF1)) ? 0xff : 0;
            BYTE PF23 = (b & (bfPF2 | bfPF3)) ? 0xff : 0;

            BYTE NOTPF01PRI23 = (PF01 & PRI23) ? 0 : 0xff;
            BYTE NOTPF23PRI2 = (PF23 & PRI2) ? 0 : 0xff;
            BYTE NOTPF23PRI12 = (PF23 & PRI12) ? 0 : 0xff;
            BYTE NOTPF01NOTPRI0 = (PF01 & ~PRI0) ? 0 : 0xff;
            BYTE NOTP23PRI03 = (P23 & PRI03) ? 0 : 0xff;
            BYTE NOTP01NOTPRI2 = (P01 & ~PRI2) ? 0 : 0xff;
            BYTE NOTP23PRI0 = (P23 & PRI0) ? 0 : 0xff;
            BYTE NOTP01PRI01 = (P01 & PRI01) ? 0 : 0xff;

            // OK, based on priority, which of the elements do we show? It could be more than one thing
            // (overlap mode ORs the colours together). Any or all of the players might be in the same spot.
            // Normally only one playfield colour is in any one spot, but a fifth player is like PF3 so there
            // could be two playfields set here. Player 5 (PF3) gets priority unless its a hi-res mode
            // where PF1 text goes on top

            BYTE SP0 = P0 & NOTPF01PRI23 & NOTPF23PRI2;
            BYTE SP1 = P1 & NOTPF01PRI23 & NOTPF23PRI2 & (NOTP0 | MULTI);
            BYTE SP2 = P2 & ~P01 & NOTPF23PRI12 & NOTPF01NOTPRI0;
            BYTE SP3 = P3 & ~P01 & NOTPF23PRI12 & NOTPF01NOTPRI0 & (NOTP2 | MULTI);

            // usually, the fifth player is above all playfields, even with priority 8
            // !!! NYI quirk where fifth player is below players in prior 8 if PF0 and PF1 not present

            BYTE SF3 = PF3 & NOTP23PRI03 & NOTP01NOTPRI2;
            BYTE NOTSF3 = (b & bfPF3) ? 0 : 0xff;
            BYTE SF0 = PF0 & NOTP23PRI0 & NOTP01PRI01 & NOTSF3;
            BYTE SF1 = PF1 & NOTP23PRI0 & NOTP01PRI01 & NOTSF3;
            BYTE SF2 = PF2 & NOTP23PRI03 & NOTP01NOTPRI2 & NOTSF3;
            //BYTE SB = 0xff & ~P01 & ~P23 & ~PF01 & ~PF23;

            // OR together all the colours of all the visible things (ovelap mode support)
            // to create a bitfield of just what is visible instead of all the things that were present at that pixel
            rgPMGMap[z] = SP0 | SP1 | SP2 | SP3 | SF0 | SF1 | SF2 | SF3;
        }

        else
        {
            BYTE NOTP0 = (b & bfPM0) ? 0 : 0xff;
            BYTE NOTP2 = (b & bfPM2) ? 0 : 0xff;

            BYTE SP1 = P1 & (NOTP0 | MULTI);
            BYTE SP2 = P2 & ~P01;
            BYTE SP3 = P3 & ~P01 & (NOTP2 | MULTI);

            // GR. 9 - low nibble is the important LUM value.
            // Remember, GR.0 & 8 can be secret GTIA modes
            if ((prior & 0xc0) == 0x40)
            {
                if (P01 || P23)    // all players visible over all playfields
                    rgPMGMap[z] = P0 | SP1 | SP2 | SP3;
                else
                    rgPMGMap[z] = (b & 0x0f); // | sl.colbk will come later once we know it to get chroma
            }

            // GR. 10 - low nibble is an index into the colour to use
            else if ((prior & 0xc0) == 0x80)
            {
                if (P01 || P23)    // !!! not proper in GR.10, but I'm making all players overtop all playfields
                    rgPMGMap[z] = P0 | SP1 | SP2 | SP3;
                else
                    rgPMGMap[z] = b; // = *(&COLPM0 + b) will come later once we know them
            }

            // GR. 11 - low nibble is the important CHROM value.
            else if ((prior & 0xc0) == 0xC0)
            {
                if (P01 || P23)    // all players above all playfields
                    rgPMGMap[z] = P0 | SP1 | SP2 | SP3;
                else
                    rgPMGMap[z] = (b << 4); // | sl.colbk or COLBK will come later when we know it to get luminence
            }
        }
    }
}


void DrawPlayers(void *candy, BYTE *qb, unsigned start, unsigned stop)
{
    BYTE b2;
    BYTE c;
    int i;
    short new[4] = { NTSCx, NTSCx,NTSCx,NTSCx };       // the new location we're moving a player to
    short newstop[4];   // the stop position of the new location

    // first loop, fill the bit array with all the players located at each spot
    for (i = 0; i < 4; i++)
    {
        // no PM data in this pixel
        b2 = pmg.grafp[i];
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hpospPixStart[i] >= (short)stop || pmg.hpospPixStop[i] <= (short)start)
            continue;

        c = mppmbw[i];

        // take note of where the player is
        WORD z;
        for (z = max((short)start, pmg.hpospPixStart[i]); z < (unsigned)min((short)stop, pmg.hpospPixStop[i]); z++)
        {
            BYTE *pq = qb + z;

            if (b2 & (0x80 >> ((z - pmg.hpospPixStart[i]) >> pmg.cwp[i])))
                *pq |= c;
        }

        // If we finished drawing a PMG (fully, or we stopped early because we're done the line), see if there's
        // a new location or GRAF data for for it. We can't leave the new position unprocessed even if it won't be drawn
        if ((short)z == pmg.hpospPixStop[i] || z >= wSLEnd)
        {
            // one a PMG finishes, we update the pending new data to use next time
            if (pmg.newGRAFp[i])
            {
                pmg.grafp[i] = rgbMem[GRAFP0A + i];
                pmg.newGRAFp[i] = FALSE;
                //ODS("Draw [%02x] done @ %04x Update GRAF to %02x\n", i, z, pmg.grafp[i]);
            }

            if (pmg.hpospPixNewStart[i] < NTSCx)
            {
                new[i] = pmg.hpospPixNewStart[i];
                //ODS("Draw [%02x] done @ %04x, new HPOS is %04x\n", i, z, new[i]);

                newstop[i] = new[i] + (8 << pmg.cwp[i]);

                // data for new location is empty (may differ from data at old location, b2)
                if (!pmg.grafp[i])
                    continue;

                // no part of the player is in the region we care about
                if (new[i] >= (short)stop || newstop[i] <= (short)start)
                    continue;

                // now note all the locations of the new player
                for (z = max((short)start, new[i]); z < (unsigned)min((short)stop, newstop[i]); z++)
                {
                    BYTE *pq = qb + z;

                    if (pmg.grafp[i] & (0x80 >> ((z - new[i]) >> pmg.cwp[i])))
                        *pq |= c;
                }
            }
        }
    }

    // don't want collisions - we always do
    //if (pmg.fHitclr)
    //    return;

    // second loop, check for collisions once all the players have reported
    // !!! I can only detect collisions from hpos 40 (visible) but I'm supposed detect collisions
    // in the overscan area back to hpos 34! (and similarly on the right)
    for (i = 0; i < 4; i++)
    {
        // no PM data in this pixel (old data) - we didn't do anything above in this case, so no need to check for collisions
        b2 = pmg.grafp[i];
        if (!b2)
            continue;

        // no part of the player is in the region we care about.
        if (pmg.hpospPixStart[i] >= (short)stop || pmg.hpospPixStop[i] <= (short)start)
            continue;

        c = mppmbw[i];

        // look for collisions
        for (unsigned z = max((short)start, pmg.hpospPixStart[i]); z < (unsigned)min((short)stop, pmg.hpospPixStop[i]); z++)
        {
            BYTE *pq = qb + z;

            // fHiRes - GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
            // fGTIA - no collisions to playfield in GTIA
            // !!! GR.10 is supposed to have collisions!
            if (b2 & (0x80 >> ((z - pmg.hpospPixStart[i]) >> pmg.cwp[i])))
            {
                PXPL[i] |= ((*pq) >> 4);
                PXPF[i] |= pmg.fHiRes ? ((*pq & 0x02) << 1) : (pmg.fGTIA ? 0 : *pq);
            }
        }

        // this player may be in two places, check the second place for collisions too,
        // with its data, pmg.grafp[i], which may be different than b2
        if (new[i] < NTSCx) // even if hpospPixNewStart < NTSCx, this may not be valid if we didn't finish drawing the old one
        {
            // no part of the player is in the region we care about. Can't continue because we have to execute the code below it
            if (!(new[i] >= (short)stop || newstop[i] <= (short)start))
            {

                for (unsigned z = max((short)start, new[i]); z < (unsigned)min((short)stop, newstop[i]); z++)
                {
                    BYTE *pq = qb + z;

                    // fHiRes - GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
                    // fGTIA - no collisions to playfield in GTIA
                    // !!! GR.10 is supposed to have collisions!
                    if (pmg.grafp[i] & (0x80 >> ((z - pmg.hpospPixNewStart[i]) >> pmg.cwp[i])))
                    {
                        PXPL[i] |= ((*pq) >> 4);
                        PXPF[i] |= pmg.fHiRes ? ((*pq & 0x02) << 1) : (pmg.fGTIA ? 0 : *pq);
                    }
                }
            }
            
            // from now on, there's just one place this PMG lives
            pmg.hpospPixStart[i] = new[i];
            pmg.hpospPixStop[i] = newstop[i];
            pmg.hpospPixNewStart[i] = NTSCx;
        }
    }

    // mask out upper 4 bits of each collision register
    // and mask out same player collisions

    *(ULONG *)PXPF &= 0x0F0F0F0F;
    *(ULONG *)PXPL &= 0x070B0D0E;
}

void DrawMissiles(void *candy, BYTE* qb, int fFifth, unsigned start, unsigned stop, BYTE *rgFifth)
{
    BYTE b2;
    BYTE c;
    int i;

    // no collisions wanted
    //if (pmg.fHitclr)
    //    goto Ldm;

    // !!! We do not support moving missiles in the middle of drawing the same missile, like we do players. They are much
    // narrower, and I don't know of any app that counts on it.

    // first loop, do missile collisions with playfield and with players (after we add missile data to the bitfield we'll no longer
    // be able to tell a player apart from its missile)
    // !!! I can only detect collisions from hpos 40 (visible) but I'm supposed detect collisions
    // in the overscan area back to hpos 34! (and similarly on the right)
    for (i = 0; i < 4; i++)
    {
        b2 = (pmg.grafm >> (i + i)) & 3;
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hposmPixStart[i] >= (short)stop || pmg.hposmPixStop[i] <= (short)start)
            continue;

        c = mppmbw[i];

        for (unsigned z = max((short)start, pmg.hposmPixStart[i]); z < (unsigned)min((short)stop, pmg.hposmPixStop[i]); z++)
        {
            BYTE *pq = qb + z;

            // fHiRes - GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
            // fGTIA - no collisions to playfield in GTIA
            // !!! GR.10 should have collisions
            if (b2 & (0x02 >> ((z - pmg.hposmPixStart[i]) >> pmg.cwm[i])))
            {
                MXPL[i] |= ((*pq) >> 4);
                MXPF[i] |= pmg.fHiRes ? (((*pq) & 0x02) << 1) : (pmg.fGTIA ? 0 : *pq);
            }
        }
    }

    // mask out upper 4 bits of each collision register

    *(ULONG *)MXPL &= 0x0F0F0F0F;
    *(ULONG *)MXPF &= 0x0F0F0F0F;

//Ldm:

    // Now go through and set the bit saying a missile is present wherever it is present. Fifth player is like PF3

    for (i = 0; i < 4; i++)
    {
        b2 = (pmg.grafm >> (i + i)) & 3;
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hposmPixStart[i] >= (short)stop || pmg.hposmPixStop[i] <= (short)start)
            continue;

        // treat fifth player like PF3 being present instead of its corresponding player. This is the only case
        // where more than one playfield colour could be present at one time. That's how we can tell a fifth player
        // is in use, and do the right thing later.

        if (fFifth)
            c = mppmbw[4];
        else
            c = mppmbw[i];

        // In GTIA modes, however, we aren't using a bitmask to show which playfields are present, we're storing a luminence,
        // so remembering the fact that the fifth player is here corrupts the data and gives the wrong colour. We don't have
        // any free bits to store anything about the fifth player, so we'll have to store that somewhere else.
        //
        for (unsigned z = max((short)start, pmg.hposmPixStart[i]); z < (unsigned)min((short)stop, pmg.hposmPixStop[i]); z++)
        {
            if (b2 & (0x02 >> ((z - pmg.hposmPixStart[i]) >> pmg.cwm[i])))
            {
                if (fFifth && pmg.fGTIA)
                    rgFifth[z] = TRUE;
                else
                    *(qb + z) |= c;
            }
        }
    }
}

__inline void IncDLPC(void *candy)
{
    DLPC = (DLPC & 0xFC00) | ((DLPC+1) & 0x03FF);
}

// Given our current scan mode, how many bits is it convenient to write at a time?
short BitsAtATime(void *candy)
{
    short it = mpMdBytes[sl.modelo] >> 3;    // how many multiples of 8 bytes per scan line in narrow mode
    short i = 8;        // assume 8
    if (it == 2)
        i <<= 1;    // med res mode - write 16 bits at a time
    else if (it == 1)
        i <<= 2;    // lo res mode - writes 32 bits at a time
    return i;
}

// Same question, but how much do we shift by to mult/divide by that number?
short ShiftBitsAtATime(void *candy)
{
    short it = mpMdBytes[sl.modelo] >> 3;    // how many multiples of 8 bytes per scan line in narrow mode
    short i = 3;        // assume 8 (3 shifts)
    if (it == 2)
        i += 1;    // med res mode - write 16 bits at a time (4 shifts)
    else if (it == 1)
        i += 2;    // lo res mode - writes 32 bits at a time (5 shifts)
    return i;
}

// !!! Why can't this live in atari800.c ?
__declspec(noinline)
BOOL __fastcall PokeBAtariDL(void *candy, ADDR addr, BYTE b)
{
    // we are writing into the line of screen RAM that we are current displaying
    // handle that with cycle accuracy instead of scan line accuracy or the wrong thing is drawn (Turmoil)
    // most display lists are in himem so do the >= check first, the test most likely to fail and not require additional tests
    Assert(addr < ramtop);

    // !!! Does not handle ANTIC bank != CPU bank
    if (addr >= wAddr && addr < (WORD)(wAddr + cbWidth) && rgbMem[addr] != b)
        ProcessScanLine(candy);

    return cpuPokeB(candy, addr, b);
}


// THEORY OF OPERATION
//
// scans is the number of scan lines this graphics mode has, minus 1. So, 7 for GR.0
// iscan is the current scan line being drawn of this graphics mode line, 0 based
//        iscan == scans means you're on the last scan line (time for DLI?)
// VSCROL set iscan to some higher initial number than 0, which will count to 16 and then back to 0
//    Whenever iscan == scans is when the vscrol is done
//
// fWait means we're doing a JVB (jump and wait for VB)
// fFetch means we finished an ANTIC instruction and are ready for a new one
// cbDisp is the width of the playfield in bytes (could be narrow, normal or wide)
// cbWidth is boosted to the next size if horiz scrolling, it's how many bytes we fetch even though some aren't shown (ready to scroll on)
// wAddr is the start of screen memory set by a LMS (load memory scan)

// INITIAL SET UP WHEN WE FIRST START A SCAN LINE
// call before Go6502 to learn all about this scan line to know how many cycles can execute during it
//
void PSLPrepare(void *candy)
{
    if (PSL == 0 || PSL >= wSLEnd)    // or just trust the caller and don't check
    {
        PSL = 0;    // allow PSL to do stuff again

        // overscan area - all blank lines and never the first scan line of a mode (so there's no DMA fetch)
        if (wScan < STARTSCAN || wScan >= STARTSCAN + wcScans)
        {
            sl.modelo = 0;
            sl.modehi = 0;
            iscan = 8;  // this and sl.vscrol not being equal means we're not the first scan line of a mode, so no PF DMA
            sl.vscrol = 0;
            return;
        }

        // we are entering visible territory (240 lines are visible - usually 24 blank, 192 created by ANTIC, 24 blank)
        if (wScan == STARTSCAN)
        {
            //ODS("BEGIN scan 8\n");
            fWait = 0;    // not doing JVB anymore, we reached the top
            fFetch = 1;    // start by grabbing a DLIST instruction
        }

        // grab a DLIST instruction - if we're not in JVB, we're done the previous instruction, and ANTIC is on.
        if (!fWait && fFetch && (DMACTL & 32))
        {
#ifndef NDEBUG
            extern BOOL  fDumpHW;

            if (fDumpHW)
                printf("Fetching DL byte at scan %d, DLPC=%04X, byte=%02X\n",
                    wScan, DLPC, cpuPeekB(candy, DLPC));
#endif

#if ANTICBANK
            // if ANTIC is reading from a different XE bank than the CPU, that bank # + 1 is stored for us
            if (mdXLXE == mdXE && ANTICBankDifferent && DLPC >= 0x4000 && DLPC < 0x8000)
                sl.modehi = rgbXEMem[(ANTICBankDifferent - 1) * XE_SIZE + DLPC - 0x4000];
            else
#endif
                // yes, the DL might be in ROM or even accidentally in register memory (Ghost Chaser) so don't use cpuPeekB
                sl.modehi = PeekBAtari(candy, DLPC);

            sl.modelo = sl.modehi & 0x0F;
            sl.modehi >>= 4;
            //sl.modehi |= (sl.modelo << 4);   // hide original mode up here (technically correct?)
            //ODS("%04x: scan %03x FETCH %01x %01x\n", DLPC, wScan, sl.modehi, sl.modelo);
            IncDLPC(candy);

            fFetch = FALSE;

            // vertical scroll bit enabled - you can only start vscrolling on a real mode
            if ((sl.modehi & 2) && !sl.fVscrol && (sl.modelo >= 2))
            {
                sl.fVscrol = TRUE;
                sl.vscrol = VSCROL & 15;// stores the first value of iscan to see if this is the first scan line of the mode
                iscan = sl.vscrol;      // start displaying at this scan line. If > # of scan lines per mode line (say, 14)
                                        // we'll see parts of this mode line twice. We'll count up to 15, then back to 0 and
                                        // up to sscans. The last scan line is always when iscan == sscans.
                                        // The actual scan line drawn will be iscan % (number of scan lines in that mode line)
                //ODS("%d iscan = %d\n", wScan, iscan);
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

                scans = (sl.modehi & 7);    // scans = 1 less than the # of blank lines wanted
                break;

            case 1:
                // display list jump instruction

                scans = 0;
                fWait = (sl.modehi & 4);    // JVB or JMP?

                if (fWait)
                    fWait |= (sl.modehi & 8);    // a DLI on a JVB keeps firing every line

                {
                    WORD w;

#if ANTICBANK
                    // if ANTIC is reading from a different XE bank than the CPU, that bank # + 1 is stored for us
                    if (mdXLXE == mdXE && ANTICBankDifferent && DLPC >= 0x4000 && DLPC < 0x8000)
                        w = rgbXEMem[(ANTICBankDifferent - 1) * XE_SIZE + DLPC - 0x4000];
                    else
#endif
                        w = PeekBAtari(candy, DLPC);

                    IncDLPC(candy);
      
#if ANTICBANK
                    // if ANTIC is reading from a different XE bank than the CPU, that bank # + 1 is stored for us
                    if (mdXLXE == mdXE && ANTICBankDifferent && DLPC >= 0x4000 && DLPC < 0x8000)
                        w |= rgbXEMem[(ANTICBankDifferent - 1) * XE_SIZE + DLPC - 0x4000];
                    else
#endif
                        w |= (PeekBAtari(candy, DLPC) << 8);

                    DLPC = w;
                    //ODS("Scan %04x JVB\n", wScan);
                }
                break;

            default:
                // normal display list entry

                // how many scan lines a line of this graphics mode takes
                scans = (BYTE)mpMdScans[sl.modelo] - 1;

                // LMS (load memory scan) attached to this line to give start of screen memory
                if (sl.modehi & 4)
                {
#if USE_POKE_TABLE
                    // stop catching writes to old screen RAM
                    BYTE b1 = (wAddr & 0xff00) >> 8;
                    BYTE b2 = ((wAddr + cbWidth - 1) & 0xff00) >> 8;
                    if ((ramtop >> 8) > b1)
                        write_tab[iVM][b1] = cpuPokeB;
                    if ((ramtop >> 8) > b2)
                        write_tab[iVM][b2] = cpuPokeB;
#endif

#if ANTICBANK
                    // if ANTIC is reading from a different XE bank than the CPU, that bank # + 1 is stored for us
                    if (mdXLXE == mdXE && ANTICBankDifferent && DLPC >= 0x4000 && DLPC < 0x8000)
                        wAddr = rgbXEMem[(ANTICBankDifferent - 1) * XE_SIZE + DLPC - 0x4000];
                    else
#endif
                        wAddr = PeekBAtari(candy, DLPC);
                    
                    IncDLPC(candy);

#if ANTICBANK
                    // if ANTIC is reading from a different XE bank than the CPU, that bank # + 1 is stored for us
                    if (mdXLXE == mdXE && ANTICBankDifferent && DLPC >= 0x4000 && DLPC < 0x8000)
                        wAddr |= rgbXEMem[(ANTICBankDifferent - 1) * XE_SIZE + DLPC - 0x4000];
                    else
#endif
                        wAddr |= (PeekBAtari(candy, DLPC) << 8);
                    
                    IncDLPC(candy);
                }
                break;
            }
        }
         
        // Check playfield width and set cbWidth (number of bytes read by Antic)
        // and the smaller cbDisp (number of bytes actually visible)
        // This happens every scan line (frogger)
        switch (DMACTL & 3)
        {
        default:
            Assert(FALSE);
            break;

            // cbDisp - actual number of bytes per line of this graphics mode visible
            // cbWidth - boosted number of bytes read by ANTIC if horizontal scrolling (narrow->normal, normal->wide)

        case 0:    // playfield off
            sl.modelo = 0;
            cbWidth = 0;
            cbDisp = cbWidth;
            break;
        case 1: // narrow playfield
            //sl.modelo = sl.modehi >> 4; // restore the mode (technically correct?)
            cbWidth = mpMdBytes[sl.modelo];        // cbDisp: use NARROW number of bytes per graphics mode line, eg. 32
            cbDisp = cbWidth;
            if ((sl.modehi & 1) && (sl.modelo >= 2)) // hor. scrolling, cbWidth mimics NORMAL
                cbWidth |= (cbWidth >> 2);
            break;
        case 2: // normal playfield
            //sl.modelo = sl.modehi >> 4; // restore the mode
            cbWidth = mpMdBytes[sl.modelo];        // bytes in NARROW mode, eg. 32
            cbDisp = cbWidth | (cbWidth >> 2);    // cbDisp: boost to get bytes per graphics mode line in NORMAL mode, eg. 40
            if ((sl.modehi & 1) && (sl.modelo >= 2)) // hor. scrolling?
                cbWidth |= (cbWidth >> 1);            // boost cbWidth to mimic wide playfield, eg. 48
            else
                cbWidth |= (cbWidth >> 2);            // otherwise same as cbDisp
            break;
        case 3: // wide playfield
            //sl.modelo = sl.modehi >> 4; // restore the mode
            cbWidth = mpMdBytes[sl.modelo];        // NARROW width
            cbDisp = cbWidth | (cbWidth >> 2) | (cbWidth >> 3);    // visible area is half way between NORMAL and WIDE
            cbWidth |= (cbWidth >> 1);            // WIDE width
            break;
        }

#if USE_POKE_TABLE
        // catch writes to screen RAM
        BYTE b1 = (wAddr & 0xff00) >> 8;
        BYTE b2 = ((wAddr + cbWidth - 1) & 0xff00) >> 8;
        if ((ramtop >> 8) > b1)
            write_tab[iVM][b1] = PokeBAtariDL;
        if ((ramtop >> 8) > b2)
            write_tab[iVM][b2] = PokeBAtariDL;
#endif

        // time to stop vscrol, this line doesn't use it.
        // !!! Stop if the mode is different than the mode when we started scrolling? I don't think so...
        // allow blank mode lines to mean duplicates of previous lines (GR.9++)
        if (sl.fVscrol && (!(sl.modehi & 2) || sl.modelo < 2))
        {
            sl.fVscrol = FALSE;
            // why is somebody setting the high bits of this sometimes?
            scans = VSCROL & 0x0f;    // the first mode line after a VSCROL area is truncated, ending at VSCROL
            //ODS("%d FINISH: scans=%d\n", wScan, scans);
        }

        // DMA is off, and we're wanting to fetch because we finished the last thing it was doing when it was turned off
        // so just do blank mode lines until DMA comes back on
        if (fFetch && !(DMACTL & 0x20))
        {
            sl.modelo = 0;
            sl.modehi = 0;
            iscan = 1;        // keep this from running free, make it look like it's just finished
            sl.vscrol = 0;    // note that this is not the first scan line of a new mode line (do not do PF DMA fetch)
            scans = 0;        // pretend it's a 1-line blank mode
        }

        // we do not support changing HSCROL in the middle of a scan line. I doubt it's necessary, it would slow things down,
        // and it's complicated anyway

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

        // HSCROL - we read more bytes than we display. View the center portion of what we read, offset cbDisp/10 bytes into the scan line
        wAddrOff = 0;
        if (cbDisp != cbWidth)
        {
            // wide is 48, normal is 40, that's j=4 (split the difference) characters before we begin.
            // subtract one for every multiple of 8 hshift is, that's an entire character shift
            wAddrOff = ((cbWidth - cbDisp) >> 1) - ((hshift) >> 3);
            hshift &= 7;    // now only consider the part < 8
        }

        // Other things we only need once per scan line

        sl.chactl = CHACTL & 7; // !!! cycle granularity?
        sl.addr = wAddr;        // !!! not used
        sl.dmactl = DMACTL;

        if (sl.dmactl & 0x10)
            pmg.pmbase = PMBASE & 0xF8;
        else
            pmg.pmbase = PMBASE & 0xFC;

        // enable PLAYER DMA and enable players?
        // GRACTL ON but DMA OFF seems to produce randomly changing PMG data, so I'll just use the value of GRAFPx
        if (sl.dmactl & 0x08 && GRACTL & 2)
        {
            // single line resolution
            if (sl.dmactl & 0x10)
            {
                // !!! VDELAY affects this too, but in an odd way such that nobody is likely to be using it
                pmg.grafp0 = cpuPeekB(candy, (pmg.pmbase << 8) + 1024 + wScan);
                pmg.grafp1 = cpuPeekB(candy, (pmg.pmbase << 8) + 1280 + wScan);
                pmg.grafp2 = cpuPeekB(candy, (pmg.pmbase << 8) + 1536 + wScan);
                pmg.grafp3 = cpuPeekB(candy, (pmg.pmbase << 8) + 1792 + wScan);
            }
            // double line resolution
            else
            {
                pmg.grafp0 = cpuPeekB(candy, (pmg.pmbase << 8) + 512 + ((wScan - ((VDELAY >> 4) & 1)) >> 1));
                pmg.grafp1 = cpuPeekB(candy, (pmg.pmbase << 8) + 640 + ((wScan - ((VDELAY >> 5) & 1)) >> 1));
                pmg.grafp2 = cpuPeekB(candy, (pmg.pmbase << 8) + 768 + ((wScan - ((VDELAY >> 6) & 1)) >> 1));
                pmg.grafp3 = cpuPeekB(candy, (pmg.pmbase << 8) + 896 + ((wScan - ((VDELAY >> 7) & 1)) >> 1));
            }
        }

        // enable MISSILE DMA and enable missiles? (enabling players enables missiles too)
        if ((sl.dmactl & 0x04 || sl.dmactl & 0x08) && GRACTL & 1)
        {
            // single res - !!! VDELAY ignored as well
            if (sl.dmactl & 0x10)
                pmg.grafm = cpuPeekB(candy, (pmg.pmbase << 8) + 768 + wScan);
            // double res
            else
            {
                pmg.grafm = 0;
                pmg.grafm |= (cpuPeekB(candy, (pmg.pmbase << 8) + 384 + ((wScan - ((VDELAY >> 0) & 1)) >> 1)) & 0x3);    //M0
                pmg.grafm |= (cpuPeekB(candy, (pmg.pmbase << 8) + 384 + ((wScan - ((VDELAY >> 1) & 1)) >> 1)) & 0xc);    //M1
                pmg.grafm |= (cpuPeekB(candy, (pmg.pmbase << 8) + 384 + ((wScan - ((VDELAY >> 2) & 1)) >> 1)) & 0x30);   //M2
                pmg.grafm |= (cpuPeekB(candy, (pmg.pmbase << 8) + 384 + ((wScan - ((VDELAY >> 3) & 1)) >> 1)) & 0xc0);   //M3
            }
        }

        // the extent of visible PM data on this scan line
        pmg.hposPixEarliest = X8;
        pmg.hposPixLatest = 0;

        // PM DMA might have changed which players are visible - maybe a PL is empty and wasn't before or v.v. 
        // Is there visible PMG data on this scan line? Figure out the extents
        // This code is also in ReadRegs but only executes when something changes requiring recalculation during the scan line
        if (pmg.grafpX || pmg.grafm)
        {

            // precompute on what pixel each player and missile start and stop (assume its data is $ff, although...)
            // also keep track of the overall range of VISIBLE PMs on this scan line
            for (int i = 0; i < 4; i++)
            {
                // a pending change in data for a player should have resolved itself by the end of the last scan line
                Assert(!pmg.newGRAFp[i]);

                if (pmg.grafp[i]) // player visible
                {
                    // keep track of left-most P pixel
                    if (pmg.hpospPixStart[i] < pmg.hposPixEarliest)
                        pmg.hposPixEarliest = pmg.hpospPixStart[i];

                    // keep track of right-most P pixel
                    if (pmg.hpospPixStop[i] > pmg.hposPixLatest)
                        pmg.hposPixLatest = pmg.hpospPixStop[i];

                    // We don't have to check hpospPixNewStart since we are definitely not in the middle of drawing a player
                    // so it's not active
                    Assert(pmg.hpospPixNewStart[i] == NTSCx);
                }

                if (pmg.grafm & (0x03 << (i << 1))) // missile visible
                {
                    // keep track of left-most M pixel
                    if (pmg.hposmPixStart[i] < pmg.hposPixEarliest)
                        pmg.hposPixEarliest = pmg.hposmPixStart[i];

                    // keep track of right-most M pixel
                    if (pmg.hposmPixStop[i] > pmg.hposPixLatest)
                        pmg.hposPixLatest = pmg.hposmPixStop[i];
                }
            }

            if (pmg.hposPixEarliest < 0)
                pmg.hposPixEarliest = 0;
            if (pmg.hposPixLatest > X8)
                pmg.hposPixLatest = X8;
        }
    }
}

// WHEN DONE, MOVE ON TO THE NEXT SCAN LINE NEXT TIME WE'RE CALLED
//
void PSLPostpare(void *candy)
{
    if (PSL >= wSLEnd)      // we are at least at the last pixel to do (may be < X8 if we're a tile partially off the right side of the screen)
    {
        // When we're done with this mode line, fetch again. If we couldn't fetch because DMA was off, keep wanting to fetch
        if (!fFetch)
            fFetch = (iscan == scans);
        iscan = (iscan + 1) & 15;
        
        // !!! This is wrong, but doesn't seem to hurt anything but acid test - ANTIC line buffering
        // We should only advance ANTIC's PC during a fetch if playfield is on (DMACTL & 3) and the fetched mode >= 2
        // However, that is difficult as we won't remember the correct amount to advance it if we move this code
        // (save cbWidth from the last valid mode drawn?)
        if (fFetch)
        {

#if USE_POKE_TABLE
            // stop catching writes to old screen RAM
            BYTE b1 = (wAddr & 0xff00) >> 8;
            BYTE b2 = ((wAddr + cbWidth - 1) & 0xff00) >> 8;
            if ((ramtop >> 8) > b1)
                write_tab[iVM][b1] = cpuPokeB;
            if ((ramtop >> 8) > b2)
                write_tab[iVM][b2] = cpuPokeB;
#endif

            // ANTIC's PC can't cross a 4K boundary, poor thing
            wAddr = (wAddr & 0xF000) | ((wAddr + cbWidth) & 0x0FFF);
        }
    }
}

// we call this from a couple places, so make sure we do the same thing every time
// fGTIA and fHiRes must be set first
//
void UpdateColourRegisters(void *candy)
{
    // update the colour registers

    // GTIA mode GR.10 uses 704 as background colour, enforced in all scan lines of any antic mode
    sl.colbk = (pmg.fGTIA == 0x80) ? COLPM0 : COLBK;
    
    // GTIA mode GR.11 uses the low nibble of the background colour for the luminence for all colours,
    // but forces the actual background colour to dark.
    if (pmg.fGTIA == 0xc0)
        sl.colbk = COLBK & 0xf0;
    
    sl.colpfX = COLPFX;
    pmg.colpmX = COLPMX;

    // if in a hi-res 2 color mode, set playfield 1 color to some luminence of PF2
    if (pmg.fHiRes)
        sl.colpf1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);

    // uncomment to make all PMG orange and hopefully stand out
    //pmg.colpm0 = pmg.colpm1 = pmg.colpm2 = pmg.colpm3 = 245;

    // in greyscale mode, just use luminences
    if (FGreyFromBf(pvm->bfMon))
    {
        sl.colpfX &= 0x0f0f0f0f;
        pmg.colpmX &= 0x0f0f0f0f;
        sl.colbk &= 0x0f;
    }
}


// RE-READ ALL THE H/W REGISTERS TO ALLOW THINGS TO CHANGE MID-SCAN LINE
//
void PSLReadRegs(void *candy, short start, short stop)
{
    int i;

    // Note: in GTIA mode, ALL scan lines behave somewhat like GTIA, no matter what mode. Mix modes at your peril.
    sl.prior = PRIOR;

    sl.chbase = CHBASE & ((sl.modelo < 6) ? 0xFC : 0xFE);

    // note if GTIA modes enabled... no collisions to playfield in GTIA
    pmg.fGTIA = sl.prior & 0xc0;

    // If GTIA is enabled, change mode 15 into 16, 17 or 18 for GR. 9, 10 or 11
    // Be brave, and if GTIA is turned off halfway down the screen, turn back!
    if (sl.prior & 0xC0)    // pmg.fGTIA is not set up yet
    {
        if (sl.modelo == 15)
        {
            sl.modelo = 15 + (sl.prior >> 6); // (we'll call 16, 17 and 18 GR. 9, 10 and 11)
        }
        else if (sl.modelo == 2 || sl.modelo == 3 || sl.modelo > 15)
            ;    // little known fact, you can go into GTIA modes based on GR.0, dereferencing a char set to get the bytes to draw
        else
        {
            // !!! ANTIC does something strange, but I don't emulate it right now!
        }
    }
    else
    {
        if (sl.modelo > 15)
            sl.modelo = 15;
    }

    // are we in a hi-res mono mode that has special collision detection rules?
    // !!! apparently this should be forced off if fGTIA? (pseudo mode E)
    pmg.fHiRes = (sl.modelo == 2 || sl.modelo == 3 || sl.modelo == 15);

    // fGTIA and fHiRes must be set first
    UpdateColourRegisters(candy);

    // check if GRAFPX or GRAFM are being used (PMG DMA is only fetched once per scan line, but these can change more often)
    if (!((sl.dmactl & 0x08) && (GRACTL & 2)))
    {
        // note that we're trying to change player data
        if (GRAFP0 != pmg.grafp[0])
            pmg.newGRAFp[0] = TRUE;
        if (GRAFP1 != pmg.grafp[1])
            pmg.newGRAFp[1] = TRUE;
        if (GRAFP2 != pmg.grafp[2])
            pmg.newGRAFp[2] = TRUE;
        if (GRAFP3 != pmg.grafp[3])
            pmg.newGRAFp[3] = TRUE;

        // but it doesn't take effect yet (if we're in the middle of drawing it now)
        //pmg.grafpX = GRAFPX;  
    }

    BOOL newGRAFm = FALSE;
    if (!(((sl.dmactl & 0x04) || (sl.dmactl & 0x08)) && (GRACTL & 1)))
    {
        if (GRAFM != pmg.grafm)
        {
            // !!! I do not properly wait until the current missile is finished drawing before allowing its data to change,
            // since they're so narrow and it's complex and no known app needs it.
            newGRAFm = TRUE;
            pmg.grafm = GRAFM;
        }
    }

    // Did the H-pos or sizes or data (possibly affecting visibility) change?
    if (pmg.newGRAFp[0] || pmg.newGRAFp[1] || pmg.newGRAFp[2] || pmg.newGRAFp[3] ||
                    newGRAFm || pmg.hposmX != HPOSMX || pmg.hpospX != HPOSPX || pmg.sizem != SIZEM || pmg.sizepX != SIZEPX)
    {
        short off;

        // we do update the HPOS, but we won't map that to a pixel number until we're ready to draw it
        // (if we're still in the middle of drawing it in an old location)
        pmg.hposmX = HPOSMX;
        pmg.hpospX = HPOSPX;
        pmg.sizem = SIZEM;
        pmg.sizepX = SIZEPX;

        for (i = 0; i < 4; i++)
        {
            // at what pixel is the new hpos?
            off = (pmg.hposp[i] - ((NTSCx - X8) >> 2)) << 1;

            pmg.cwp[i] = (BYTE)mpsizecw[pmg.sizep[i] & 3];    // normal, double or quad size?
            if (pmg.cwp[i] == 4)
                pmg.cwp[i] = 3;    //# of times to shift to divide by (cw *2)

            // We are in the middle of drawing this player, and its old data is non-zero
            // (if we're already past the (clipped?) visible area, wSLEnd, there will be no further drawing, so updates can happen now)
            if (pmg.hpospPixStart[i] < start && pmg.hpospPixStop[i] > start && pmg.grafp[i] && start < wSLEnd)
            {
                // it's position has moved
                if (off != pmg.hpospPixStart[i])
                {
                    //ODS("%04x: HPOS CHANGE (delay both) during [%02x] G=%02x @ %04x, new pos %04x nG=%02x\n", wScan, i, pmg.grafp[i], start, off, rgbMem[GRAFP0A + i]);
                    
                    pmg.hpospPixNewStart[i] = off;  // remember the new position after we finish drawing the old position
                    
                    // do not update its GRAF data yet

                    // if the new pos is overlapping, stop the old player where the new one begins
                    if (off > start && off < pmg.hpospPixStop[i])
                    {
                        ODS("ReadRegs: truncate\n");
                        pmg.hpospPixStop[i] = off;
                    }
                }
                else
                {
                    // if it hasn't moved, keep drawing in old pos with old GRAF data.
                    // When it finishes drawing, it will grab the new GRAF data.
                    
                    //if (rgbMem[GRAFP0A + i] != pmg.grafp[i])
                    //    ODS("%04x: GRAF CHANGE (delay GRAF) during draw [%02x] G=%02x to %02x @ %04x\n", wScan, i, pmg.grafp[i], rgbMem[GRAFP0A +i], start);
                }
            }

            // OK to update HPOS and GRAF
            // !!! No it's not. If we're in the middle of the new pos, we should wait until the beginning of the next time it's drawn,
            // but we can only do that if the old place is drawing GRAF != 0
            // !!! No it's not #2. If we're in the middle of drawing GRAF=0, there's no way to delay GRAF <= non-0
            // until it's finished drawing, but hopefully no app does this
            else
            {
                short offstop = off + (8 << pmg.cwp[i]);

#ifndef NDEBUG
                // as explained above, we may prematurely alter the HPOS and GRAF, but that's better than
                // ignoring the change.
                if (off != pmg.hpospPixStart[i])    // hpos is moving
                {
                    if (off < start && offstop > start && (pmg.grafp[i] || (pmg.newGRAFp[i] && rgbMem[GRAFP0A + i])))
                        ODS("%04x: CRAP! HPOS[%02x] moved to %04x to straddle current pos %04x while GRAF != 0!\n", wScan, i, off, start);

                    if (pmg.hpospPixStart[i] < start && pmg.hpospPixStop[i] > start && !pmg.grafp[i] && rgbMem[GRAFP0A + i])
                        ODS("%04x: CRAP! HPOS[%02x] moved to %04x while old pos %04x was drawing 0 and new GRAF != 0!\n", wScan, i, i, start);
                }
#endif

                // update GRAF
                if (pmg.newGRAFp[i])
                {
                    pmg.grafp[i] = rgbMem[GRAFP0A + i];
                    pmg.newGRAFp[i] = FALSE;
                    //ODS("...Update GRAFP%02x to %02x\n", i, pmg.grafp[i]);
                }

                //ODS("ReadRegs: new %d can replace old\n", i);
                pmg.hpospPixStart[i] = off;
                pmg.hpospPixStop[i] = offstop;
                pmg.hpospPixNewStart[i] = NTSCx;
            }
        }

        pmg.hposPixEarliest = X8;
        pmg.hposPixLatest = 0;

        // precompute on what pixel each player and missile start and stop (assume its data is $ff, although...)
        // also keep track of the overall range of VISIBLE PMs on this scan line
        for (i = 0; i < 4; i++)
        {
            if (pmg.grafp[i])   // it's visible!
            {
                // keep track of left-most PMG pixel
                if (pmg.hpospPixStart[i] < pmg.hposPixEarliest)
                    pmg.hposPixEarliest = pmg.hpospPixStart[i];

                // keep track of right-most PMG pixel
                if (pmg.hpospPixStop[i] > pmg.hposPixLatest)
                    pmg.hposPixLatest = pmg.hpospPixStop[i];

                // the new pending start location for a recently moved PMG, valid if < NTSCx
                if (pmg.hpospPixNewStart[i] < NTSCx && pmg.hpospPixNewStart[i] < pmg.hposPixEarliest)
                {
                    //ODS("ReadRegs: include new %i in early\n", i);
                    pmg.hposPixEarliest = pmg.hpospPixNewStart[i];
                }

                // the new pending end location for a recently moved PMG
                if (pmg.hpospPixNewStart[i] < NTSCx && pmg.hpospPixNewStart[i] + (8 << pmg.cwp[i]) > pmg.hposPixLatest)
                {
                    //ODS("ReadRegs: include new %i in late\n", i);
                    pmg.hposPixLatest = pmg.hpospPixNewStart[i] + (8 << pmg.cwp[i]);
                }
            }
     
            // now do missile #i

            off = (pmg.hposm[i] - ((NTSCx - X8) >> 2));

            pmg.cwm[i] = (BYTE)mpsizecw[((pmg.sizem >> (i + i))) & 3];
            if (pmg.cwm[i] == 4)
                pmg.cwm[i] = 3;    //# of times to shift to divide by (cw *2)

            pmg.hposmPixStart[i] = off << 1;                         // first pixel affected by this missile
            pmg.hposmPixStop[i] = (off << 1) + (2 << pmg.cwm[i]);    // the pixel after the last one affected

            if (pmg.grafm & (0x03 << (i << 1))) // it's visible
            {
                // keep track of left-most PMG pixel
                if (pmg.hposmPixStart[i] < pmg.hposPixEarliest)
                    pmg.hposPixEarliest = pmg.hposmPixStart[i];

                // keep track of right-most PMG pixel
                if (pmg.hposmPixStop[i] > pmg.hposPixLatest)
                    pmg.hposPixLatest = pmg.hposmPixStop[i];
            }
        }

        if (pmg.hposPixEarliest < 0)
            pmg.hposPixEarliest = 0;
        if (pmg.hposPixLatest > X8)
            pmg.hposPixLatest = X8;
    }

    // If there is visible PMG data on this scan line, turn on special bitfield mode to deal with it
    sl.fpmg = (pmg.grafpX || pmg.grafm);

    // Make sure a PMG is really showing in this section we're drawing because going into PMG mode is SLOW!
    if (sl.fpmg && !(pmg.hposPixEarliest < stop && pmg.hposPixLatest > start))
        sl.fpmg = FALSE;
}


// This will draw a portion of a scan line from start to stop (in pixels)
// It will look at sl.fpmg to see if PMG are somewhere in this area, if so it does a special slower bitmask algorithm.
// i and iTop are the byte numbers of the visible part of the scan line we are drawing, inside the black bars (each bar is bbars wide)
// For instance, if there are 16 pixels of black bars and we're drawing the first 32 pixels of a hi-res scan mode (8 bits per byte)
// start = 0, stop = 32, i = 0, iTop = 2, bbars = 16 (We are drawing the black bar plus 16 pixels, which is 2 ATARI screen bytes in hi-res).
// start and stop are already on a BitsAtATime boundary for whatever graphics mode we are doing
// (a multiple of 8, 16 or 32 from the end of the left black bar, depending on if we're in a high, medium or lo-res mode).

void PSLInternal(void *candy, unsigned start, unsigned stop, unsigned i, unsigned iTop, unsigned bbars, RECT *prectTile)
{
    if (start >= stop)
        return;
    
    // PMG can exist outside the visible boundaries of this line, so we need an extra long line to avoid complicating things
    BYTE rgpix[NTSCx];    // an entire NTSC scan line, including retrace areas, used in PMG bitfield mode
    
    // rgpix doesn't have enough room to store everything about a pixel, so we need some extra storage
    BYTE rgFifth[X8];    // is fifth player colour present in this pixel in GTIA modes?
    BYTE rgArtifact[X8]; // is artifacting being done on this pixel?
    BYTE colpf1Save = 0;     // if so, we'll need to remember this

    // Based on the graphics mode, fill in a scanline's worth of data. It might be the actual data, or if PMG exist on this line,
    // a bitfield simply saying which playfield is at each pixel so the priority of which should be visible can be worked out later
    // GTIA modes are a kind of hybrid approach since it's not so simple as which playfield colour is visible
    //

    int j;
    BYTE *qch0;
    
    if (v.fTiling && !v.fMyVideoCardSucks)
    {
        qch0 = vvmhw.pTiledBits;
        qch0 += prectTile->top * sStride + prectTile->left;
    }
    else
        qch0 = vvmhw.pbmTile[pvmin->iVisibleTile].pvBits;

    BYTE * __restrict qch = qch0;

    BYTE b1, b2;
    BYTE vpix = iscan;

    union
    {
        struct
        {
            BYTE col0, col1, col2, col3;
        };

        BYTE col[4];
        DWORD colX;
    } Col;
    
#ifndef NDEBUG
    // show current stack
    // annoying debug pixels near top of screen: qch[regSP & 0xFF] = (BYTE)(cpuPeekB(regSP | 0x100) ^ wFrame);
#endif
    
    BYTE *qchStart = 0;
    if (sl.fpmg)
    {
        // We didn't waste time initializing the array if we weren't going to use it
        memset(rgFifth, 0, sizeof(rgFifth));
        memset(rgArtifact, 0, sizeof(rgArtifact));
        
        // When PM/G are present on the scan line is first rendered
        // into rgpix as bytes of bit vectors representing the playfields
        // and PM/G that are present at each pixel. Then later we map
        // rgpix to actual colors on the screen, applying priorities
        // in the process.
        qchStart = &rgpix[(NTSCx - X8) >> 1];     // first visible screen NTSC pixel
        qch = qchStart;

        // GTIA modes 9, 10 & 11 don't use the PF registers since they can have 16 colours instead of 4
        // so we CANNOT do a special bitfield mode. See the PRIOR code
        if (!pmg.fGTIA)
        {
            sl.colbk = bfBK;
            sl.colpf0 = bfPF0;
            colpf1Save = sl.colpf1; // remember what this used to be for artifacting
            sl.colpf1 = bfPF1;
            sl.colpf2 = bfPF2;
            sl.colpf3 = bfPF3;
        }
    }
    else
    {
        // not doing a bitfield, just write into this scan line
        if (v.fTiling && !v.fMyVideoCardSucks)
        {
            qch += ((wScan - STARTSCAN) * sStride);
            if (qch < (BYTE *)(vvmhw.pTiledBits) || qch >= (BYTE *)(vvmhw.pTiledBits) + sStride * sRectC.bottom)
                return;
        }
        else
            qch += ((wScan - STARTSCAN) * vcbScan);
    }

    // what is the background colour? Normally it's sl.colbk, but in pmg mode we are using an index of 0 to represent it.
    // Unless we're in GR.9 or GR.11, in which case we're still using sl.colbk, but in the case of pmg, a 4-bit version
    // We've already accounted above for GR.10 having a different register for its background colour.
    BYTE bkbk = sl.fpmg ? 0 : sl.colbk;
    if (pmg.fGTIA & 0x40)    // GR.9 or 11
    {
        bkbk = sl.colbk;
        if (sl.fpmg && (pmg.fGTIA == 0xc0)) // GR.11 the interesting part is the high nibble
            bkbk = sl.colbk >> 4;
        if (sl.fpmg)
            bkbk = bkbk & 0x0f;
    }

    if (bbars)
    {
        if (start < bbars)
            memset(qch + start, bkbk, min(bbars, stop) - start);        // draw background colour before we start real data
        if (stop >= X8 - bbars)
            memset(qch + max(start, X8 - bbars), bkbk, stop - (max(start, X8 - bbars)));    // after finished real data
    }

    // bring our buffer up to where we left off, but at least to the end of the bars
    qch += max(start, bbars);

#if 0 // This shouldn't be necessary, the graphics mode touches every pixel
    if (sl.fpmg)
        // zero out the section we'll be using, now that we know the proper value of stop
        _fmemset(&rgpix[(NTSCx - X8) >> 1] + start, 0, stop - start);
#endif

// nothing to do otherwise
if (sl.modelo < 2 || iTop > i)
    switch (sl.modelo)
    {
    default:
        Assert(FALSE);
        break;

    case 0:
    case 1:
        // just fill in whatever we need to indicate background colour for our portion if there is no mode
        // the one twist is that in PMG GR.11 we need to know if C.0 is being used to force it to be the darkest lum,
        // so we write 0 as background instead of the actual colour
        _fmemset(qch, (sl.fpmg && pmg.fGTIA == 0xc0) ? 0 : bkbk, stop - start);
        break;

        // GR.0 and descended character GR.0
    case 2:
    case 3:
    {
        BYTE vpixO = vpix % (sl.modelo == 2 ? 8 : 10);

        // mimic obscure ANTIC behaviour (why not?) Scans 10-15 duplicate 2-7, not 0-5
        if (sl.modelo == 3 && iscan > 9)
            vpixO = iscan - 8;

        vpix = vpixO;

        Col.col1 = sl.colpf1;
        Col.col2 = sl.colpf2;

        // just for fun, don't interlace in B&W.
        const BOOL fArtifacting = (pvm->bfMon == monColrTV) && !pmg.fGTIA && !sl.fpmg;
        const BOOL fPMGA = (pvm->bfMon == monColrTV) && !pmg.fGTIA && sl.fpmg; // fill in special array

        // the artifacting colours - !!! this behaves like NTSC, PAL has somewhat random artifacting
        const BYTE red = fArtifacting ? (0x40 | (Col.col1 & 0x0F)) : Col.col1;
        const BYTE green = fArtifacting ? (0xc0 | (Col.col1 & 0x0F)) : Col.col1;
        const BYTE yellow = fArtifacting ? (0xe0 | (Col.col1 & 0x0F)) : Col.col1;
        yellow; // NYI

        // the real artifact colours, not the bitfield versions
        const BYTE redPMG = 0x40 | (colpf1Save & 0x0F);
        const BYTE greenPMG = 0xc0 | (colpf1Save & 0x0F);
        const BYTE yellowPMG = 0xe0 | (colpf1Save & 0x0F);
        yellowPMG; // NYI

        // generate a 4 pixel wide pattern of col1 and col2 and the artifact pattern

        const ULONG FillA = 0x00010001 * (red | (green << 8));
        const ULONG FillAPMG = 0x00010001 * (redPMG | (greenPMG << 8));
        const ULONG Fill1 = 0x01010101 * Col.col1;
        const ULONG Fill1PMG = 0x01010101 * colpf1Save;
        const ULONG Fill2 = 0x01010101 * Col.col2;

        if ((sl.chactl & 4) && sl.modelo == 2)    // vertical reflect bit
            vpix = 7 - (vpix & 7);

        for (; i < iTop; i++)
        {
            b1 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            if ((sl.chactl & 4) && sl.modelo == 3)
            {
                if ((b1 & 0x7F) < 0x60)
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
                if (sl.modelo == 2 || ((vpix >= 2 && vpix < 8) || (vpix < 2 && (b1 & 0x7F) < 0x60) ||
                    (vpix >= 8 && (b1 & 0x7F) >= 0x60)))
                {
                    // use the top two rows of pixels for the bottom 2 scan lines for ANTIC mode 3 descended characters
                    vpix23 = (vpix >= 8 && (b1 & 0x7f) >= 0x60) ? vpix - 8 : vpix;

                    // sl.chbase is already on a proper page boundary
                    vv = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x7f) << 3) + vpix23);
                }
                // we ARE in the blank part
                else {
                    vv = 0;
                }

                // mimic obscure quirky ANTIC behaviour - scans 8 and 9 go missing under these circumstances
                if ((b1 & 0x7f) < 0x60 && (iscan == 8 || iscan == 9))
                    vv = 0;

                if (b1 & 0x80)
                {
                    if (sl.chactl & 1)  // blank (blink) flag
                        vv = 0;
                    if (sl.chactl & 2)  // inverse flag
                        vv = ~vv & 0xff;
                }

                u = vv;

                if (hshift % 8)
                {
                    // partial shifting means we also need to look at a second character intruding into our space
                    // do the same exact thing again
                    int index = 1;
                    BYTE rgb = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - index) & 0x0FFF));  // the char offscreen to the left

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

                        // sl.chbase is on a proper page boundary
                        vv = cpuPeekB(candy, (sl.chbase << 8) + ((rgb & 0x7f) << 3) + vpix23);
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
                    // use sl.chbase not CHBASE because its already anded with $fc or $fe
                    b2 = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix);
                else if ((vpix < 2) && ((b1 & 0x7f) < 0x60))
                    b2 = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix);
                else if ((vpix >= 8) && ((b1 & 0x7F) >= 0x60))
                    b2 = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix - 8);
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

            // undocumented GR.9 mode based on GR.0 - dereference through a character set to get the bytes to put on the screen,
            // but treat them as GR.9 luminences
            if (pmg.fGTIA == 0x40)
            {
                Col.col1 = (b2 >> 4) | (sl.colbk /* & 0xf0 */);    // let the user screw up the colours if they want, like a real 810
                Col.col2 = (b2 & 15) | (sl.colbk /* & 0xf0*/); // they should only POKE 712 with multiples of 16

                                                           // we're in BITFIELD mode because PMG are present, so we can only alter the low nibble. We'll put the chroma value back later.
                if (sl.fpmg)
                {
                    Col.col1 &= 0x0f;
                    Col.col2 &= 0x0f;
                }

                *qch++ = Col.col1;
                *qch++ = Col.col1;
                *qch++ = Col.col1;
                *qch++ = Col.col1;

                *qch++ = Col.col2;
                *qch++ = Col.col2;
                *qch++ = Col.col2;
                *qch++ = Col.col2;
            }

            // undocumented GR.10 mode based on GR.0 - dereference through a character set to get the bytes to put on the screen,
            // but treat them as GR.10 indexes
            else if (pmg.fGTIA == 0x80)
            {
                Col.col1 = (b2 >> 4);
                Col.col2 = (b2 & 15);

                // if PMG are present on this line, we are asked to make a bitfield in the low nibble of which colours we are
                // using. That's not possible in GR.10 which has >4 possible colours, so we'll just use our index. If we're not
                // in bitfield mode, we'll use the actual colour

                if (!sl.fpmg)
                {
                    if (Col.col1 < 9)
                        Col.col1 = *(((BYTE FAR *)&COLPM0) + Col.col1);
                    else if (Col.col1 < 12)
                        Col.col1 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                    else
                        Col.col1 = *(((BYTE FAR *)&COLPM0) + Col.col1 - 8);    // col. 12-15 are copies of c.4-7

                    if (Col.col2 < 9)
                        Col.col2 = *(((BYTE FAR *)&COLPM0) + Col.col2);
                    else if (Col.col2 < 12)
                        Col.col2 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                    else
                        Col.col2 = *(((BYTE FAR *)&COLPM0) + Col.col2 - 8);    // col. 12-15 are copies of c.4-7
                }
                else
                {
                    if (Col.col1 < 9)
                        ;
                    else if (Col.col1 < 12)
                        Col.col1 = 8;            // col. 9-11 are copies of c.8
                    else
                        Col.col1 = Col.col1 - 8;    // col. 12-15 are copies of c.4-7

                    if (Col.col2 < 9)
                        ;
                    else if (Col.col2 < 12)
                        Col.col2 = 8;            // col. 9-11 are copies of c.8
                    else
                        Col.col2 = Col.col2 - 8;    // col. 12-15 are copies of c.4-7
                }

                *qch++ = Col.col1;
                *qch++ = Col.col1;
                *qch++ = Col.col1;
                *qch++ = Col.col1;

                *qch++ = Col.col2;
                *qch++ = Col.col2;
                *qch++ = Col.col2;
                *qch++ = Col.col2;
            }

            // undocumented GR.11 mode based on GR.0 - dereference through a character set to get the bytes to put on the screen,
            // but treat them as GR.11 chromas
            else if (pmg.fGTIA == 0xC0)
            {
                Col.col1 = ((b2 >> 4) << 4) | (sl.colbk /* & 15*/);    // lum comes from 712
                Col.col2 = ((b2 & 15) << 4) | (sl.colbk /* & 15*/); // keep 712 <16, if not, it deliberately screws up like the real hw

                                                                // We're in BITFIELD mode because PMG are present, so we can only alter the low nibble.
                                                                // we'll shift it back up and put the luma value back in later
                if (sl.fpmg)
                {
                    Col.col1 = Col.col1 >> 4;
                    Col.col2 = Col.col2 >> 4;
                }

                *qch++ = Col.col1;
                *qch++ = Col.col1;
                *qch++ = Col.col1;
                *qch++ = Col.col1;

                *qch++ = Col.col2;
                *qch++ = Col.col2;
                *qch++ = Col.col2;
                *qch++ = Col.col2;
            }

            // GR.0 See mode 15 for artifacting theory of operation
            else
            {

// !!! TODO - make this a monitor type you can select
// !!! We use different strategies for GR.0 and GR.8
#if 1
                // This is the "I have sharp display with minimum pixel bleeding" version

                ULONG BlendMask = BitsToByteMask[(b2 >> 4) & 0xF];
                ULONG ColorMask = BitsToByteMask[(b2 >> 5) & 0xF] | BitsToByteMask[(b2 >> 3) & 0xF];

                *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                // make note of the artificting colour for this pixel
                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                qch += sizeof(ULONG);

                BlendMask = BitsToByteMask[((b2 >> 0) & 0xF)];
                ColorMask = BitsToByteMask[((b2 >> 1) & 0xF)] | BitsToByteMask[((b2 << 1) & 0xF) | 1];

                *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                qch += sizeof(ULONG);
#elif 0
                // This is the "my cheap TV really sucks" version - "but does not reflect any read hardware" (-Danny)

                ULONG BlendMask = BitsToByteMask[(b2 >> 4) & 0xF];
                ULONG ColorMask = BitsToByteMask[(b2 >> 5) & 0xF];

                *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                // make note of the artificting colour for this pixel
                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                qch += sizeof(ULONG);

                BlendMask = BitsToByteMask[((b2 >> 0) & 0xF)];
                ColorMask = BitsToByteMask[((b2 >> 1) & 0xF)];

                *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                qch += sizeof(ULONG);
#endif
            }
        }
        break;
    }

    case 5:
        vpix = iscan >> 1;    // extra thick, use screen data twice for 2 output lines
    case 4:
        if (sl.chactl & 4)
            vpix ^= 7;                // vertical reflect bit
        vpix &= 7;

        Col.col0 = sl.colbk;
        Col.col1 = sl.colpf0;
        Col.col2 = sl.colpf1;
        //            col3 = sl.colpf2;

        for (; i < iTop; i++)
        {
            b1 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            if (hshift)
            {
                // non-zero horizontal scroll

                ULONGLONG u, vv;

                // see comments for modes 2 & 3

                vv = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix);
                u = vv;

                if (hshift % 8)
                {
                    int index = 1;
                    vv = cpuPeekB(candy, (sl.chbase << 8) +
                                ((cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - index) & 0x0FFF)) & 0x7f) << 3) + vpix);
                    u |= (vv << (index << 3));
                }

                b2 = (BYTE)(u >> hshift);
            }
            else
                b2 = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x7F) << 3) + vpix);

            for (j = 0; j < 4; j++)
            {
                switch (b2 & 0xC0)
                {
                default:
                    Assert(FALSE);
                    break;

                case 0x00:
                    qch[0] = Col.col0;
                    qch[1] = Col.col0;
                    break;

                case 0x40:
                    qch[0] = Col.col1;
                    qch[1] = Col.col1;
                    break;

                case 0x80:
                    qch[0] = Col.col2;
                    qch[1] = Col.col2;
                    break;

                case 0xC0:
                    {
                    // which character was shifted into this position? Pay attention to its high bit to switch colours
                    int index = (hshift + 7 - 2 * j) / 8;

                    if (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - index) & 0x0FFF)) & 0x80)
                        Col.col3 = sl.colpf3;
                    else
                        Col.col3 = sl.colpf2;

                        qch[0] = Col.col3;
                        qch[1] = Col.col3;
                    }
                    break;
                }

                qch += 2;
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

        Col.col0 = sl.colbk;
        const ULONG Fill0 = 0x01010101 * Col.col0;

        for (; i < iTop; i++)
        {
            b1 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            Col.col1 = sl.colpf[b1 >> 6];

            if (hshift)
            {
                // non-zero horizontal scroll

                ULONGLONG u, vv;

                // see comments for modes 2 & 3
               
                vv = cpuPeekB(candy, (sl.chbase << 8) + ((b1 & 0x3F) << 3) + vpix);
                u = vv;

                if (hshift % 8)
                {
                    int index = 1;
                    vv = cpuPeekB(candy, (sl.chbase << 8) +
                                ((cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - index) & 0x0FFF)) & 0x3f) << 3) + vpix);
                    u |= (vv << (index << 3));
                }

                b2 = (BYTE)(u >> hshift);

                for (j = 0; j < 8; j++)
                {
                    if (b2 & 0x80)
                    {
                        if (j < hshift)    // hshift restricted to 7 or less, so this is sufficient
                            Col.col1 = sl.colpf[cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) >> 6];
                        else
                            Col.col1 = sl.colpf[b1 >> 6];

                        qch[0] = Col.col1;
                        qch[1] = Col.col1;
                    }
                    else
                    {
                        qch[0] = Col.col0;
                        qch[1] = Col.col0;
                    }

                    qch += 2;
                    b2 <<= 1;
                }
            }
            else
            {
                b2 = cpuPeekB(candy, (sl.chbase << 8)
                    + ((b1 & 0x3F) << 3) + vpix);

                const ULONG Fill1 = 0x01010101 * Col.col1;

                ULONG BlendMask = BitsToByteMask[(b2 >> 4) & 0xF];
                ULONG PixelsHi = (Fill1 & BlendMask) | (Fill0 & ~BlendMask);
                BlendMask = BitsToByteMask[(b2 >> 0) & 0xF];
                ULONG PixelsLo = (Fill1 & BlendMask) | (Fill0 & ~BlendMask);

                *qch++ = PixelsHi & 0xff;
                *qch++ = PixelsHi & 0xff;
                PixelsHi >>= 8;
                *qch++ = PixelsHi & 0xff;
                *qch++ = PixelsHi & 0xff;
                PixelsHi >>= 8;
                *qch++ = PixelsHi & 0xff;
                *qch++ = PixelsHi & 0xff;
                PixelsHi >>= 8;
                *qch++ = PixelsHi & 0xff;
                *qch++ = PixelsHi & 0xff;
                PixelsHi >>= 8;

                *qch++ = PixelsLo & 0xff;
                *qch++ = PixelsLo & 0xff;
                PixelsLo >>= 8;
                *qch++ = PixelsLo & 0xff;
                *qch++ = PixelsLo & 0xff;
                PixelsLo >>= 8;
                *qch++ = PixelsLo & 0xff;
                *qch++ = PixelsLo & 0xff;
                PixelsLo >>= 8;
                *qch++ = PixelsLo & 0xff;
                *qch++ = PixelsLo & 0xff;
                PixelsLo >>= 8;
            }
        }
        break;

    case 8:
        Col.col0 = sl.colbk;
        Col.col1 = sl.colpf0;
        Col.col2 = sl.colpf1;
        Col.col3 = sl.colpf2;

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            WORD u = b2;

            // can't check hshift, because it is half of sl.hscrol, which might be 0 when sl.hscrol == 1
            if (sl.hscrol)
            {
                // non-zero horizontal scroll

                // this shift may involve our byte and the one before it
                u = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) << 8) | (BYTE)b2;
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

                *qch++ = Col.col[b2];
                *qch++ = Col.col[b2];
            }
        }
        break;

    case 9:
        Col.col0 = sl.colbk;
        Col.col1 = sl.colpf0;

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            WORD u = b2;

            // can't use hshift, it is 1/2 of sl.hscrol, which might be only 1. It's OK, hshift never can get to 8 to reset to 0.
            if (sl.hscrol)
            {
                // non-zero horizontal scroll

                // this shift may involve our byte and the one before it
                u = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) << 8) | (BYTE)b2;
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

                *qch++ = Col.col[b2];
                *qch++ = Col.col[b2];
            }
        }
        break;

    case 10:
        Col.col0 = sl.colbk;
        Col.col1 = sl.colpf0;
        Col.col2 = sl.colpf1;
        Col.col3 = sl.colpf2;

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            WORD u = b2;

            // modes 8 and 9 cannot shift more than 1 byte, and hshift could be 0 if sl.hscrol == 1, so we had to test sl.hscrol.
            // This mode can shift > 1 byte, so hshift might be truncated to %8, so it's the opposite...
            // we have to test hshift.
            if (hshift)
            {
                // non-zero horizontal scroll

                // this shift may involve our byte and the one before it
                u = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) << 8) | (BYTE)b2;
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

                *qch++ = Col.col[b2];
                *qch++ = Col.col[b2];
            }
        }
        break;

        // these only differ by # of scan lines, we'll get called twice for 11 and once for 12
    case 11:
    case 12:
        Col.col0 = sl.colbk;
        Col.col1 = sl.colpf0;

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            WORD u = b2;

            // see comments in mode 10, must use hshift, not sl.hscrol for this mode
            if (hshift)
            {
                // non-zero horizontal scroll

                // this shift may involve our byte and the one before it
                u = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) << 8) | (BYTE)b2;
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

                *qch++ = Col.col[b2];
                *qch++ = Col.col[b2];
            }
        }
        break;

    case 13:
    case 14:
        Col.col0 = sl.colbk;
        Col.col1 = sl.colpf0;
        Col.col2 = sl.colpf1;
        Col.col3 = sl.colpf2;

        for (; i < iTop; i++)
        {
            WORD wb2 = (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF);
#if ANTICBANK
// !!! This needs to go in every case statement to implement ANTIC bank different than CPU XE bank
            if (mdXLXE == mdXE && ANTICBankDifferent && wAddr >= 0x4000 && wAddr < 0x8000)
                b2 = rgbXEMem[(ANTICBankDifferent - 1) * XE_SIZE + wb2 - 0x4000];
            else
#endif
                b2 = cpuPeekB(candy, wb2);

            WORD u = b2;

            // hshift tells us if there's any hscrol needed not accounted for by an even multiple of 8
            if (hshift)
            {
                // non-zero horizontal scroll

                // this shift may involve our byte and the one before it
                u = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) << 8) | (BYTE)b2;
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

                *qch++ = Col.col[b2];
                *qch++ = Col.col[b2];
            }
        }
        break;

    case 15:
        {
        Col.col1 = sl.colpf1;
        Col.col2 = sl.colpf2;

        // just for fun, don't artifact in B&W
        const BOOL fArtifacting = (pvm->bfMon == monColrTV) && !pmg.fGTIA && !sl.fpmg; // artifact the normal screen memory
        const BOOL fPMGA = (pvm->bfMon == monColrTV) && !pmg.fGTIA && sl.fpmg; // fill in special array

        // the artifacting colours - !!! this behaves like NTSC, PAL has somewhat random artifacting
        const BYTE red = fArtifacting ? (0x40 | (Col.col1 & 0x0F)) : Col.col1;
        const BYTE green = fArtifacting ? (0xc0 | (Col.col1 & 0x0F)) : Col.col1;
        const BYTE yellow = fArtifacting ? (0xe0 | (Col.col1 & 0x0F)) : Col.col1;
        yellow; // NYI

        // the real artifact colours, not the bitfield versions
        const BYTE redPMG = 0x40 | (colpf1Save & 0x0F);
        const BYTE greenPMG = 0xc0 | (colpf1Save & 0x0F);
        const BYTE yellowPMG = 0xe0 | (colpf1Save & 0x0F);
        yellowPMG; // NYI

        // generate a 4 pixel wide pattern of col1 and col2 and the artifact pattern

        const ULONG FillA = 0x00010001 * (red | (green << 8));
        const ULONG FillAPMG = 0x00010001 * (redPMG | (greenPMG << 8));
        const ULONG Fill1 = 0x01010101 * Col.col1;
        const ULONG Fill1PMG = 0x01010101 * colpf1Save;
        const ULONG Fill2 = 0x01010101 * Col.col2;

        WORD u = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF));

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            u = (u << 8) | (BYTE)b2;

            // do you have the hang of this by now? Use hshift, not sl.hscrol
            // what bit position in the WORD u do we start copying from?
            int index = 7 + hshift;

            // ATARI can only shift 2 pixels minimum at this resolution

            // THEORY OF OPERATION - INTERLACING
            // - only even pixels show red, only odd pixels show green (interpolate the empty pixels to be that colour too)
            // - odd and even shows orange. even and odd show white. 3 pixels in a row all show white
            // - a background pixel between white and (red/green) seems to stay background colour
            // copy 2 screen pixel each iteration (odd then even), for 8 pixels written per screen byte in this mode

            u = 0x3FF & (u >> (index - 7));  // 10-bit mask includes the two previous pixels (only uses one for now)

// !!! TODO - make this a monitor type you can select
// !!! We use different strategies for GR.0 and GR.8
            if (!fArtifacting && !fPMGA)
            {
                // This is the "I have sharp display with minimum pixel bleeding" version
                // It is the ONLY version that works in black and white mode when artifacting is supposedly off

                ULONG BlendMask = BitsToByteMask[(u >> 4) & 0xF];
                ULONG ColorMask = BitsToByteMask[(u >> 5) & 0xF] | BitsToByteMask[(u >> 3) & 0xF];

                *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                // make note of the artificting colour for this pixel
                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                qch += sizeof(ULONG);

                BYTE bPeekAhead = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i + 1) & 0x0FFF)) & 0x80) >> 7;

                BlendMask = BitsToByteMask[((u >> 0) & 0xF)];
                ColorMask = BitsToByteMask[((u >> 1) & 0xF)] | BitsToByteMask[((u << 1) & 0xF) | bPeekAhead];

                *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

                qch += sizeof(ULONG);
            }
#if 0
            // This is the "my cheap TV really sucks" version - "but does not reflect any real hardware" (- Danny)

            ULONG BlendMask = BitsToByteMask[(u >> 4) & 0xF];
            ULONG ColorMask = BitsToByteMask[(u >> 5) & 0xF];

            *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

            // make note of the artificting colour for this pixel
            if (fPMGA)
            *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

            qch += sizeof(ULONG);

            BlendMask = BitsToByteMask[((u >> 0) & 0xF)];
            ColorMask = BitsToByteMask[((u >> 1) & 0xF)];

            *(ULONG *)qch = (((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

            // make note of the artificting colour for this pixel
            if (fPMGA)
            *(ULONG *)(&rgArtifact[qch - qchStart]) = (((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | (Fill2 & ~BlendMask);

            qch += sizeof(ULONG);
#endif
            else  // this algorithm does NOT work when fArtifacting is FALSE or for B&W monitors
            {
                // This is the "make the bricks solid in Lode Runner" mode ("Apple II users would disagree!" - Darek)
                // ("Apple II users were on a hi-res monitor, solid bricks are correct for the TV type we say we emulate!" - Danny)

                ULONG BlendMask = BitsToByteMask[(u >> 4) & 0xF];
                ULONG ColorMask = BitsToByteMask[(u >> 5) & 0xF] | BitsToByteMask[(u >> 3) & 0xF];
                ULONG SolidMask = BitsToByteMask[(u >> 5) & 0xF] & BitsToByteMask[(u >> 3) & 0xF];

                *(ULONG *)qch = ((((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | \
                    ((((0x80808080 ^ FillA) & SolidMask) | (Fill2 & ~SolidMask)) & ~BlendMask));

                // make note of the artificting colour for this pixel
                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = ((((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | \
                    ((((0x80808080 ^ FillAPMG) & SolidMask) | (Fill2 & ~SolidMask)) & ~BlendMask));

                qch += sizeof(ULONG);

                BYTE bPeekAhead = (cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i + 1) & 0x0FFF)) & 0x80) >> 7;
                BlendMask = BitsToByteMask[((u >> 0) & 0xF)];
                ColorMask = BitsToByteMask[((u >> 1) & 0xF)] | BitsToByteMask[((u << 1) & 0xF) | bPeekAhead];
                SolidMask = BitsToByteMask[((u >> 1) & 0xF)] & BitsToByteMask[((u << 1) & 0xF) | bPeekAhead];

                *(ULONG *)qch = ((((FillA & ~ColorMask) | (Fill1 & ColorMask)) & BlendMask) | \
                    ((((0x80808080 ^ FillA) & SolidMask) | (Fill2 & ~SolidMask)) & ~BlendMask));

                if (fPMGA)
                    *(ULONG *)(&rgArtifact[qch - qchStart]) = ((((FillAPMG & ~ColorMask) | (Fill1PMG & ColorMask)) & BlendMask) | \
                    ((((0x80808080 ^ FillAPMG) & SolidMask) | (Fill2 & ~SolidMask)) & ~BlendMask));

                qch += sizeof(ULONG);
            }
        }
        break;
        }
    case 16:
        // GTIA 16 grey mode

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            // GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
            // use low nibble of previous byte
            if (hshift & 0x04)
            {
                b2 = b2 >> 4;
                b2 |= ((cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) & 0x0f) << 4);
            }

            Col.col1 = (b2 >> 4) | (sl.colbk /* & 0xf0 */);    // let the user screw up the colours if they want, like a real 810
            Col.col2 = (b2 & 15) | (sl.colbk /* & 0xf0*/); // they should only POKE 712 with multiples of 16

                                                       // we're in BITFIELD mode because PMG are present, so we can only alter the low nibble. We'll put the chroma value back later.
            if (sl.fpmg)
            {
                Col.col1 &= 0x0f;
                Col.col2 &= 0x0f;
            }
            *qch++ = Col.col1;
            *qch++ = Col.col1;
            *qch++ = Col.col1;
            *qch++ = Col.col1;

            *qch++ = Col.col2;
            *qch++ = Col.col2;
            *qch++ = Col.col2;
            *qch++ = Col.col2;
        }
        break;

    case 17:
        // GTIA 9 color mode - GR. 10

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            // GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
            // use low nibble of previous byte
            if (hshift & 0x04)
            {
                b2 = b2 >> 4;
                b2 |= ((cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) & 0x0f) << 4);
            }

            Col.col1 = (b2 >> 4);
            Col.col2 = (b2 & 15);

            // if PMG are present on this line, we are asked to make a bitfield in the low nibble of which colours we are
            // using. That's not possible in GR.10 which has >4 possible colours, so we'll just use our index. If we're not
            // in bitfield mode, we'll use the actual colour

            if (!sl.fpmg)
            {
                if (Col.col1 < 9)
                    Col.col1 = *(((BYTE FAR *)&COLPM0) + Col.col1);
                else if (Col.col1 < 12)
                    Col.col1 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                else
                    Col.col1 = *(((BYTE FAR *)&COLPM0) + Col.col1 - 8);    // col. 12-15 are copies of c.4-7

                if (Col.col2 < 9)
                    Col.col2 = *(((BYTE FAR *)&COLPM0) + Col.col2);
                else if (Col.col2 < 12)
                    Col.col2 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                else
                    Col.col2 = *(((BYTE FAR *)&COLPM0) + Col.col2 - 8);    // col. 12-15 are copies of c.4-7
            }
            else
            {
                if (Col.col1 < 9)
                    ;
                else if (Col.col1 < 12)
                    Col.col1 = 8;            // col. 9-11 are copies of c.8
                else
                    Col.col1 = Col.col1 - 8;    // col. 12-15 are copies of c.4-7

                if (Col.col2 < 9)
                    ;
                else if (Col.col2 < 12)
                    Col.col2 = 8;            // col. 9-11 are copies of c.8
                else
                    Col.col2 = Col.col2 - 8;    // col. 12-15 are copies of c.4-7
            }

            *qch++ = Col.col1;
            *qch++ = Col.col1;
            *qch++ = Col.col1;
            *qch++ = Col.col1;

            *qch++ = Col.col2;
            *qch++ = Col.col2;
            *qch++ = Col.col2;
            *qch++ = Col.col2;
        }
        break;

    case 18:
        // GTIA 16 color mode GR. 11

        for (; i < iTop; i++)
        {
            b2 = cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i) & 0x0FFF));

            // GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
            // use low nibble of previous byte
            if (hshift & 0x04)
            {
                b2 = b2 >> 4;
                b2 |= ((cpuPeekB(candy, (wAddr & 0xF000) | ((wAddr + wAddrOff + i - 1) & 0x0FFF)) & 0x0f) << 4);
            }

            Col.col1 = ((b2 >> 4) << 4);
            Col.col2 = ((b2 & 15) << 4);

            // we're in BITFIELD mode because PMG are present, so we can only alter the low nibble.
            // we'll shift it back up and put the luma value back in later
            if (sl.fpmg)
            {
                Col.col1 = Col.col1 >> 4;
                Col.col2 = Col.col2 >> 4;
            }
            else
            {
                if (Col.col1)
                    Col.col1 |= COLBK; /* & 15*/    // C.1-15 lum comes from COLBK, sl.colbk has luminence stripped out of it
                else
                    Col.col1 |= sl.colbk;           // C.0 is forced dark, strip out lum
                if (Col.col2)
                    Col.col2 |= COLBK; /* & 15*/    // you better keep COLBK <16, if not, it deliberately screws up like the real hw
                else
                    Col.col2 |= sl.colbk;
            }

            *qch++ = Col.col1;
            *qch++ = Col.col1;
            *qch++ = Col.col1;
            *qch++ = Col.col1;

            *qch++ = Col.col2;
            *qch++ = Col.col2;
            *qch++ = Col.col2;
            *qch++ = Col.col2;
        }
        break;
    }

    // may have been altered if we were in BITFIELD mode b/c PMG are active. Put them back to what they were
    if (sl.fpmg && !pmg.fGTIA)
    {
        UpdateColourRegisters(candy);
    }

    // even with Fetch DMA off, PMG DMA might be on
    if (sl.fpmg)
    {
        if (v.fTiling && !v.fMyVideoCardSucks)
        {
            qch = vvmhw.pTiledBits;
            // We are being told what piece of the big bitmap we are writing into (sRectTile) and the size of the entire bitmap (sRectC)
            // as well as its stride (sStride).
            // The rect was made 32 bytes too wide so add those, and round that number up to the nearest 4 bytes (stride)
            qch += pvmin->sRectTile.top * sStride + pvmin->sRectTile.left;
        }
        else
            qch = vvmhw.pbmTile[pvmin->iVisibleTile].pvBits;

        // now set the bits in rgpix corresponding to players and missiles. Must be in this order for correct collision detection
        // tell them what range they are to fill in data for (start to stop)
        DrawPlayers(candy, (rgpix + ((NTSCx - X8) >> 1)), start, stop);
        DrawMissiles(candy, (rgpix + ((NTSCx - X8) >> 1)), sl.prior & 16, start, stop, rgFifth);    // 5th player?

    #ifndef NDEBUG
        if (0)
            if (*(ULONG *)MXPF || *(ULONG *)MXPL || *(ULONG *)PXPF || *(ULONG *)PXPL)
            {
                DebugStr("wScan = %03d, MXPF:%08X MXPL:%08X PXPF:%08X PXPL:%08X, %03d %03d %03d %03d, %d\n", wScan,
                    *(ULONG *)MXPF, *(ULONG *)MXPL, *(ULONG *)PXPF, *(ULONG *)PXPL,
                    pmg.hposp0, pmg.hposp1, pmg.hposp2, pmg.hposp3, VDELAY);
            }
    #endif

        //pmg.fHitclr = 0;

        // now map the rgpix array to the screen
        if (v.fTiling && !v.fMyVideoCardSucks)
        {
            qch += ((wScan - STARTSCAN) * sStride);
            if (qch < (BYTE *)(vvmhw.pTiledBits) || qch >= (BYTE *)(vvmhw.pTiledBits) + sStride * sRectC.bottom)
                return;
        }
        else
            qch += ((wScan - STARTSCAN) * vcbScan);

        // turn the rgpix array from a bitfield of which items are present (player or field colours)
        // into the actual colour that will show up there, based on priorities
        // reminder: b = PM3 PM2 PM1 PM0 | PF3 PF2 PF1 PF0 (all zeroes means BKGND)

        // EXCEPT for GR.9, 10 & 11, which does not have playfield bitfield data. There are 16 colours in those modes and only 4 regsiters.
        // So don't try to look at PF bits. In GR.9 & 11, all playfield is underneath all players.

        // precompute some things so our loop can be fast. It was the slowest part of the code

        // In fifth player mode, use PF3 unless PF1 also present in which case use the luma of PF1 (special)
        // Both pf3 and pfX have special versions, depending on which we need to use
        const DWORD colpfXNorm = sl.colpfX;
        const BYTE  colpf3Norm = sl.colpf3;
        const BYTE  colpf3Spec = (sl.colpf3 & 0xF0) | (sl.colpf1 & 0x0F);
        sl.colpf3 = colpf3Spec;
        const DWORD colpfXSpec = sl.colpfX;
        sl.colpfX = colpfXNorm;

        // In hi-res mode PMG colours use the luma of PF1 when PF1 present (special)
        const DWORD colpmXNorm = pmg.colpmX;
        //const DWORD colpmXNorm = 0xf5f5f5f5;    // testing - make all PMG bright orange
        const DWORD colpmXSpec = (pmg.colpmX & 0xF0F0F0F0) | (0x01010101 * (sl.colpf1 & 0x0f));

        const BYTE * __restrict pb = &rgpix[((NTSCx - X8) >> 1)];

        // we did the PMG collisions. Priorities and actual render can wait, we're not even going to render this frame!
        if (!fRenderThisTime)
            start = stop;

        for (i = start; i < stop; i++)
        {
            BYTE b = pb[i];

            // assume we're using the normal versions
            DWORD colpfX = colpfXNorm;
            BYTE  colpf3 = colpf3Norm;
            DWORD colpmX = colpmXNorm;

            // use the special version in hires modes with PF1 present
            // !!! avoiding this if made perf worse but so did obvious improvements. Caching must be hiding the truth?
            if (pmg.fHiRes && (b & bfPF1))
            {
                // If PF3 and PF1 are present, that can only happen in 5th player mode, so alter PF3's colour to match the luma of PF1
                colpfX = colpfXSpec;
                colpf3 = colpf3Spec;

                // we have an artifacting colour we want to use instead of the regular colour for PF1
                // and that colour is not just the normal pf1 colour, it's an artifact colour (red or green)
                // so let it through and make sure artifacts show through PMGs.
                if (rgArtifact[i] && rgArtifact[i] != sl.colpf1)
                {
                    colpfX = colpfX & 0xffff00ff | (rgArtifact[i] << 8);
                    colpmX = 0x01010101 * rgArtifact[i];    // use artifact colour for PMG colours
                }
                else
                {
                    // in hi-res modes, text is always visible on top of a PMG, because the colour is altered to have PF1's luma
                    colpmX = colpmXSpec;
                }
            }

            if (!pmg.fGTIA)
            {
                if (b)
                {
                    // convert bitfield of what is present to bitfield of what wins and is visible
                    b = rgPMGMap[(sl.prior << 8) + b];

                    // for PRIOR=0, both one of (P0,P1) and PF1 might be visible, and OR-d together. But in hi-res mode,
                    // we need to remove the chroma from PF1 so that the chroma from P0/P1 is used instead
                    if (pmg.fHiRes && (b & (bfPM1 | bfPM0)) && (b & bfPF1))
                        colpfX &= 0xffff0fff;

                    // OR the colours in parallel
                    DWORD bX = (colpmX & BitsToArrayMask[(b / bfPM0) & 0x0f]) | (colpfX & BitsToArrayMask[(b / bfPF0) & 0x0f]);
                    bX = bX | (bX >> 16);
                    b = (bX | (bX >> 8)) & 0xff;
                }
                else
                {
                    // nothing present in this bit, so use background colour
                    b = sl.colbk;
                }
            }

            else
            {
                // Remember, GR.0 & 8 can be secret GTIA modes
                // GR. 9 - use the background colour for the chroma and the pixel value as luma
                if (pmg.fGTIA == 0x40)
                {
                    if (b & (bfPM3 | bfPM2 | bfPM1 | bfPM0))    // all players visible over all playfields
                    {
                        b = rgPMGMap[(sl.prior << 8) + b];  // lookup who wins, get colours quickly from sparse array

                        // OR the colours in parallel
                        DWORD bX = (colpmX & BitsToArrayMask[(b / bfPM0) & 0x0f]);
                        bX = bX | (bX >> 16);
                        b = (bX | (bX >> 8)) & 0xff;
                    }
                    else if (rgFifth[i]) // we want the fifth player but there was no other way to indicate that
                    {
                        b = colpf3;
                        rgFifth[i] = 0;
                    }
                    else
                    {
                        // no PMG present, do normal thing of using background as CHROMA and pixel value as LUMA
                        b = rgPMGMap[(sl.prior << 8) + b];
                        b |= sl.colbk;
                    }
                }
            // GR. 10 - low nibble is an index into the colour to use
            else if (pmg.fGTIA == 0x80)
                {
                    if (b & (bfPM3 | bfPM2 | bfPM1 | bfPM0))    // all players visible over all playfields
                    {
                        b = rgPMGMap[(sl.prior << 8) + b];  // lookup who wins, get colours quickly from sparse array

                        // OR the colours in parallel
                        DWORD bX = (colpmX & BitsToArrayMask[(b / bfPM0) & 0x0f]);
                        bX = bX | (bX >> 16);
                        b = (bX | (bX >> 8)) & 0xff;
                    }
                    else if (rgFifth[i]) // we want the fifth player but there was no other way to indicate that
                    {
                        b = colpf3;
                        rgFifth[i] = 0;
                    }
                    else
                    {
                        // no PMG present, so do the normal GR.10 thing of using an index to 9 colours
                        b = *(&COLPM0 + b);
                    }
                }
                
                // GR. 11 - use COLBK for the lum and the pixel value (which has already been shifted up in the table) as chroma
                else
                {
                    if (b & (bfPM3 | bfPM2 | bfPM1 | bfPM0))    // all players visible over all playfields
                    {
                        b = rgPMGMap[(sl.prior << 8) + b];  // lookup who wins, get colours quickly from sparse array

                        // OR the colours in parallel
                        DWORD bX = (colpmX & BitsToArrayMask[(b / bfPM0) & 0x0f]);
                        bX = bX | (bX >> 16);
                        b = (bX | (bX >> 8)) & 0xff;
                    }
                    else if (rgFifth[i]) // we want the fifth player but there was no other way to indicate that
                    {
                        b = colpf3;
                        rgFifth[i] = 0;
                    }
                    else
                    {
                        // no PMG present, do normal thing of using COLBK as LUMA and pixel value as chroma
                        // sl.colbk has had the luminence stripped, so use COLBK
                        b = rgPMGMap[(sl.prior << 8) + b];
                        if (b)
                            b |= (COLBK /* & 15*/);    // C. 1-15 use luminence of background, which is only found in COLBK
                        else
                            b |= (sl.colbk /* & 15*/);  // C. 0 is always forced dark, use sl.colbk which has lum stripped out
                    }
                }
            }

            qch[i] = b;
        }
    }

    // Check if mode status countdown is in effect

    //    printf("cntTick = %d\n", cntTick);

    if (cntTick)
    {
        ShowCountDownLine(candy);
    }

#if 0 // rainbow, was #ifndef NDEBUG

    // display the collision registers on each scan line
    //
    // collision registers consist of 8 bytes starting
    // $D000 in the GTIA chip

    BYTE *qch = vvmi.pvBits;

    int i;

    qch += (wScan - STARTSCAN) * vcbScan + vcbScan - 81;

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

        k = cpuPeekB(readGTIA + i);

        for (j = 0; j < 4; j++)
        {
            *qch++ = (k & 8) ? 0xFF : ((i & ~3) << 4) + 2;
            k <<= 1;
        }
    }

    *qch++ = 0x48;
#endif // NDEBUG
}


// We have wLeft CPU cycles left to execute on this scan line.
// Our DMA tables will translate that to which pixel is currently being drawn.
// PSL is the first pixel not drawn last time. Process the appropriate number of
// pixels of this scan line. At the end of the scan line, make sure to call us
// with wLeft <= 0 to finish up.
// Returns the last pixel processed
//
BOOL ProcessScanLine(void *candy)
{
    // don't do anything in the invisible top and bottom sections
    if (wScan < STARTSCAN || wScan >= STARTSCAN + wcScans)
        return TRUE;

    // what pixel is the electron beam drawing at this point (wLeft)? While executing, wLeft will be between 1 and 114
    // so the index into the DMA array, which is 0-based, is (wLeft - 1). That tells us the horizontal clock cycle we're on,
    // which is an index into the PIXEL array to tell us the pixel we're drawing at this moment.
    // Finally, once the whole scan line executes, we will be called with wLeft <= 0 to finish things up
    // (When wLeft <= 4 we'll wish we had started the next scan line already for things that have scan line granularity,
    // like VSCROL (bump pong), so we have code elsewhere to delay the execution until the next scan line).
    // Technically, I execute at the start of the STA GTIA, but it shouldn't happen until after the 4 cycle instruction.
    // We may have been blocked after the 4th cycle of the store until the next instruction could begin, so take
    // the position of the beginning of the 4th cycle, and add 1 to it when indexing rgPIXELMap to see when the 4th cycle ended.
    // DMAMAP[0] will be the maximum we can index rgPIXELMap, so don't go lower than 1, so we can +1 and still be in a valid
    // index for rgPIXELMap
    // The beam is 20 pixels behind ANTIC fetching (see WSYNC).
    short cclock = rgPIXELMap[DMAMAP[wLeft > 4 ? (wLeft - 1 - 3) : 1] + 1] - 20;
    if (cclock < 0)
        cclock = 0;
    if (cclock == 352 - 20)
        cclock = 352;

    // Don't write off the right side of the bitmap if we are a partially visible tile, why waste time?
    // We have a wide enough bitmap to finish even the lowest resolution atari pixel (32 Windows pixels wider than the client rect
    // as a safety buffer we can grow into).

    wSLEnd = X8;    // assume we're doing the whole scan line

    if (v.fTiling)
    {
        // these variables tell us what piece of the big bitmap we're drawing (sRectTile) and how big the entire bitmap is (sRectC)
        // this is how many pixels are visible, and our stop point for this scan line instead of X8 (352)
        if (pvmin->sRectTile.right > sRectC.right)
        {
            wSLEnd = (WORD)(sRectC.right - pvmin->sRectTile.left);

            if ((short)wSLEnd < cclock)
                cclock = (short)wSLEnd;
        }
    }

    // what part of the scan line were we on last time?
    short cclockPrev = PSL;

    PSL = cclock;    // next time we're called, start from here

    // We may need to draw more than asked for, to draw an integer # of bytes of a scan mode at a time (!!! current limitation)
    // Figure out i & iTop, the beginning and ending pixel we have to draw to (iTop may be > cclock which will have to grow)

    WORD bbars = 0, i = 0, iTop = 0;
    if (sl.modelo >= 2)
    {
        if ((sl.dmactl & 3) == 1)
            bbars = (X8 - NARROWx) >> 1;    // width of the "black" bar on the side of the visible playfield area
        else if ((sl.dmactl & 3) == 2)
            bbars = (X8 - NORMALx) >> 1;

        // i is our loop inside each case statement for each possible mode. Set up where in the loop we need to start
        // skip the portion inside the bar to where we start drawing real data
        i = cclockPrev - bbars;
        if (cclockPrev < bbars)
            i = 0;

        // i is a bit index, now make it a segment index, where each scan mode likes to create so many bits at a time per segment
        i /= BitsAtATime(candy);

        // and where do we stop drawing? Don't go into the right hand bar
        iTop = cclock - bbars;
        if (cclock <= bbars)
            iTop = 0;    // inside the left side bar
        else if (iTop > X8 - bbars - bbars)
            iTop = (X8 - bbars - bbars) / BitsAtATime(candy);    // inside the right side bar
        else
        {
            iTop -= 1;    // any remainder needs to increase the quotient by 1 (any partial segment required fills the whole segment)
            iTop = iTop / BitsAtATime(candy) + 1;
        }

        // We kind of have to draw more than asked to, so increment our bounds
        if (iTop > 0)
        {
            short newTop = iTop * BitsAtATime(candy) + bbars;
            if (newTop > cclock)
            {
                cclock = newTop;
                PSL = cclock;
            }
        }
    }

    // now we are responsible for drawing starting from location cclockPrev up to but not including cclock

    PSLReadRegs(candy, cclockPrev, cclock);    // read our hardware registers to find brand-new values to use starting this cycle

    if (cclock <= cclockPrev)
        return TRUE;    // nothing to do except read new values into the registers

    // Be efficient. Split this section into 3 pieces... before PMG appear, with PMG, and after PMG so the slow algorithm
    // runs as little as possible
    if (sl.fpmg)
    {
        short iEarly, iLate;

        // back up the beginning of this PM data to an ATARI byte boundary
        iEarly = 0;
        if (sl.modelo >= 2 && pmg.hposPixEarliest >= bbars)
        {
            if (pmg.hposPixEarliest >= (short)(X8 - bbars))
                iEarly = iTop;
            else
            {
                pmg.hposPixEarliest -= bbars;
                pmg.hposPixEarliest >>= ShiftBitsAtATime(candy);
                iEarly = pmg.hposPixEarliest;
                pmg.hposPixEarliest <<= ShiftBitsAtATime(candy);
                pmg.hposPixEarliest += bbars;
            }
        }

        // move the end of this PM data forward to an ATARI byte boundary
        iLate = 0;
        if (sl.modelo >= 2 && pmg.hposPixLatest >= bbars)
        {
            if (pmg.hposPixLatest >= (short)(X8 - bbars))
                iLate = iTop;
            else
            {
                pmg.hposPixLatest -= bbars;
                pmg.hposPixLatest = ((pmg.hposPixLatest - 1) >> ShiftBitsAtATime(candy)) + 1;
                iLate = pmg.hposPixLatest;
                pmg.hposPixLatest <<= ShiftBitsAtATime(candy);
                pmg.hposPixLatest += bbars;
            }
        }

        //ODS("%d %d-%d (%d-%d)\n", wScan, cclockPrev, cclock, i, iTop);
        //ODS("          %d-%d (%d-%d)\n", cclockPrev, pmg.hposPixEarliest, i, iEarly);
        //ODS("          %d-%d (%d-%d)\n", max(cclockPrev, pmg.hposPixEarliest), min(cclock, pmg.hposPixLatest), max(i, iEarly), min(iTop, iLate));
        //ODS("          %d-%d (%d-%d)\n", pmg.hposPixLatest, cclock, iLate, iTop);

        sl.fpmg = FALSE;
        PSLInternal(candy, cclockPrev, pmg.hposPixEarliest, i, iEarly, bbars, &pvmin->sRectTile);
  
        sl.fpmg = TRUE;
        PSLInternal(candy, max(cclockPrev, pmg.hposPixEarliest), min(cclock, pmg.hposPixLatest),
                                max(i, iEarly), min(iTop, iLate), bbars, &pvmin->sRectTile);

        sl.fpmg = FALSE;
        PSLInternal(candy, pmg.hposPixLatest, cclock, iLate, iTop, bbars, &pvmin->sRectTile);
    }
    else
    {
        //ODS("%d %d-%d (%d-%d)\n", wScan, cclockPrev, cclock, i, iTop);
        PSLInternal(candy, cclockPrev, cclock, i, iTop, bbars, &pvmin->sRectTile);
    }

    PSLPostpare(candy);    // see if we're done this scan line and be ready to do the next one

    return TRUE;
}

#endif