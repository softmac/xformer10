
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

#define vcbScan X8
#define wcScans Y8    // # of valid scan lines
#define NTSCx 512
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

                                    // this cycle is for missile DMA? Then all the tables with missile DMA on have it blocked
                                    if (rgDMA[cycle] == DMA_M)
                                    {
                                        if (missile)
                                            rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;

                                    // same for player DMA
                                    }
                                    else if (rgDMA[cycle] == DMA_P)
                                    {
                                        if (player)
                                            rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                    }

                                    // nothing else halts the CPU if Playfield DMA is off

                                    if (pf)
                                    {
                                        // mode fetch will happen on the first scan line of a mode
                                        if (rgDMA[cycle] == DMA_DL)
                                        {
                                            if (first)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // this cycle is blocked by a load memory scan (first scan line only)
                                        else if (rgDMA[cycle] == DMA_LMS)
                                        {
                                            if (lms && first)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all wide playfield modes (>= 2) use this cycle on the first scan line
                                        // (GTIA 9++ is an example of bitmap modes that actually do have subsequent scan lines
                                        //  where DMA should not occur)
                                        else if (rgDMA[cycle] == W8)
                                        {
                                            if (first && mode >= 2 && width == 2)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // wide playfield hi and med res modes use this cycle on the first scan line
                                        else if (rgDMA[cycle] == W4)
                                        {
                                            if (first && mode >= 2 && width == 2 && (mode != 8 && mode != 9))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // wide playfield hi res modes use this cycle on the first scan line
                                        else if (rgDMA[cycle] == W2)
                                        {
                                            if (first && width == 2 && ((mode >= 2 && mode <= 5) || (mode >= 13 && mode <= 18)))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // wide playfield character modes use this cycle on every scan line to get char data
                                        else if (rgDMA[cycle] == WC4)
                                        {
                                            if (width == 2 && (mode >= 2 && mode <= 7))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // wide playfield hi-res character modes use this cycle on every scan line to get char data
                                        else if (rgDMA[cycle] == WC2)
                                        {
                                            if (width == 2 && (mode >= 2 && mode <= 5))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all but narrow playfield modes (>= 2) use this one on the first scan line
                                        else if (rgDMA[cycle] == N8)
                                        {
                                            if (first && mode >= 2 && width > 0)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all but narrow playfield hi and med res modes use this cycle on the first scan line
                                        else if (rgDMA[cycle] == N4)
                                        {
                                            if (first && mode >= 2 && width > 0 && (mode != 8 && mode != 9))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all but narrow playfield hi res modes use this cycle on the first scan line
                                        else if (rgDMA[cycle] == N2)
                                        {
                                            if (first && width > 0 && ((mode >= 2 && mode <= 5) || (mode >= 13 && mode <= 18)))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all but narrow playfield character modes use this cycle on every scan line to get char data
                                        else if (rgDMA[cycle] == NC4)
                                        {
                                            if (width > 0 && (mode >= 2 && mode <= 7))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all but narrow playfield hi-res character modes use this cycle on every scan line to get char data
                                        else if (rgDMA[cycle] == NC2)
                                        {
                                            if (width > 0 && (mode >= 2 && mode <= 5))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all playfield modes (>= 2) use this one on the first scan line
                                        else if (rgDMA[cycle] == A8)
                                        {
                                            if (first && mode >= 2)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all playfield hi and med res modes use this cycle on the first scan line
                                        else if (rgDMA[cycle] == A4)
                                        {
                                            if (first && mode >= 2 && mode != 8 && mode != 9)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all playfield hi res modes use this cycle on the first scan line
                                        else if (rgDMA[cycle] == A2)
                                        {
                                            if (first && ((mode >= 2 && mode <= 5) || (mode >= 13 && mode <= 18)))
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all playfield character modes use this cycle on every scan line to get char data
                                        else if (rgDMA[cycle] == AC4)
                                        {
                                            if (mode >= 2 && mode <= 7)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
                                        }
                                        // all playfield hi-res character modes use this cycle on every scan line to get char data
                                        else if (rgDMA[cycle] == AC2)
                                        {
                                            if (mode >= 2 && mode <= 5)
                                                rgDMAMap[mode][pf][width][first][player][missile][lms][cycle] = 1;
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
                                // !!! correct timing seems to be with 4 refresh cycles?
                                // Now block out 9 RAM refresh cycles, every 4 cycles starting at 25.
                                // They can be bumped up to the next cycle's start point. The last one might be bumped all the way to 102
                                // (If there are no free cycles between 25 and 101, you just one one RAM refresh at 102)
                                for (cycle = 0; cycle < 9; cycle++)
                                {
                                    for (int xx = 0; xx < 4; xx++)
                                    {
                                        if (rgDMAMap[mode][pf][width][first][player][missile][lms][25 + 4 * cycle] == 0)
                                        {
                                            rgDMAMap[mode][pf][width][first][player][missile][lms][25 + 4 * cycle] = 1;
                                            break;
                                        }
                                        if (cycle == 8 && xx == 3)
                                            rgDMAMap[mode][pf][width][first][player][missile][lms][102] = 1; // !!! assumes it's the first free one
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

                                    // we found a free cycle. This is where wLeft == index will execute
                                    array[cycle] = index;

                                    // index 115 - remember what wLeft should jump to on a WSYNC (cycle 105)
                                    if (index == 105)
                                        rgDMAMap[mode][pf][width][first][player][missile][lms][115] = cycle;
                                    // oops, 105 was busy
                                    else if (index < 105 && rgDMAMap[mode][pf][width][first][player][missile][lms][115] == 0)
                                        rgDMAMap[mode][pf][width][first][player][missile][lms][115] = cycle - 1;

                                    // index 116 - remember what wLeft will be when it's time for a DLI/VBI NMI (cycle 10)
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
        if (cycle < 13)
            rgPIXELMap[cycle] = 0;
        else if (cycle >= 101)
            rgPIXELMap[cycle] = X8;
        else
            rgPIXELMap[cycle] = (cycle - 13) << 2;
    }
}


void DrawPlayers(int iVM, BYTE *qb, short start, short stop)
{
    BYTE b2;
    BYTE c;
    int i;

    // first loop, fill the bit array with all the players located at each spot
    for (i = 0; i < 4; i++)
    {
        // no PM data in this pixel
        b2 = pmg.grafp[i];
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hpospPixStart[i] >= stop || pmg.hpospPixStop[i] <= start)
            continue;

        c = mppmbw[i];

        // first loop, fill in where each player is
        for (int z = max(start, pmg.hpospPixStart[i]); z < min(stop, pmg.hpospPixStop[i]); z++)
        {
            BYTE *pq = qb + z;

            if (b2 & (0x80 >> ((z - pmg.hpospPixStart[i]) >> pmg.cwp[i])))
                *pq |= c;
        }
    }

    // don't want collisions
    if (pmg.fHitclr)
        return;

    // second loop, check for collisions once all the players have reported
    for (i = 0; i < 4; i++)
    {
        // no PM data in this pixel
        b2 = pmg.grafp[i];
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hpospPixStart[i] >= stop || pmg.hpospPixStop[i] <= start)
            continue;

        c = mppmbw[i];

        // first loop, fill in where each player is
        for (int z = max(start, pmg.hpospPixStart[i]); z < min(stop, pmg.hpospPixStop[i]); z++)
        {
            BYTE *pq = qb + z;

            // fHiRes - GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
            // fGTIA - no collisions to playfield in GTIA
            if (b2 & (0x80 >> ((z - pmg.hpospPixStart[i]) >> pmg.cwp[i])))
            {
                PXPL[i] |= ((*pq) >> 4);
                PXPF[i] |= pmg.fHiRes ? ((*pq & 0x02) << 1) : (pmg.fGTIA ? 0 : *pq);
            }
        }
    }

    // mask out upper 4 bits of each collision register
    // and mask out same player collisions

    *(ULONG *)PXPF &= 0x0F0F0F0F;
    *(ULONG *)PXPL &= 0x070B0D0E;
}

#if _MSC_VER >= 1200
// VC 6.0 optimization bug having to do with (byte >> int)
#pragma optimize("",off)
#endif

void DrawMissiles(int iVM, BYTE* qb, int fFifth, short start, short stop)
{
    BYTE b2;
    BYTE c;
    int i;

    // no collisions wanted
    if (pmg.fHitclr)
        goto Ldm;

    // first loop, fill the bit array with all the players located at each spot
    for (i = 0; i < 4; i++)
    {
        b2 = (pmg.grafm >> (i + i)) & 3;
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hposmPixStart[i] >= stop || pmg.hposmPixStop[i] <= start)
            continue;

        c = mppmbw[i];

        // first loop, fill in where each player is
        for (int z = max(start, pmg.hposmPixStart[i]); z < min(stop, pmg.hposmPixStop[i]); z++)
        {
            BYTE *pq = qb + z;

            // fHiRes - GR.0 and GR.8 only register collisions with PF1, but they report it as a collision with PF2
            // fGTIA - no collisions to playfield in GTIA
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

Ldm:

    // Now go through and set the bit saying a missile is present wherever it is present. Fifth player is like PF3

    for (i = 0; i < 4; i++)
    {
        b2 = (pmg.grafm >> (i + i)) & 3;
        if (!b2)
            continue;

        // no part of the player is in the region we care about
        if (pmg.hposmPixStart[i] >= stop || pmg.hposmPixStop[i] <= start)
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
        // any free bits to store anything about the fifth player unless we do something big and hacky which I'm not inclined to
        // do right now, so we have 2 choices: 1) make the fifth player invisible or 2) make it visible but the wrong colour
        // Neither is a good option. In games where it's supposed to be invisible, showing it at the wrong colour sticks out
        // like a sore thumb. But in games where it's supposed to be visible, making in invisible loses part of the actual game
        // graphics when it being the wrong colour would not have been a big deal. So the ideal compromise is to figure out
        // if this mode supports the colour they want, and make it that correct colour. That way it will be invisible if they
        // want. If we don't have a way of providing the right colour but we know it's supposed to be visible, make it "close"
        // to the colour they want by matching the chroma or luma at least. This is rather clever, it's too bad only the silly
        // game "Battle Eagles" runs into this issue.

        // !!! The wrong colours may show up here

        if (fFifth && pmg.fGTIA)
        {
            if ((sl.prior & 0xc0) == 0x40)
                c = sl.colpf3 & 0x0f;    // use the correct P5 luma, it may or may not be the right chroma
            else if ((sl.prior & 0xc0) == 0x80)
                c = 7;    // colour PF3 is always an option in Gr. 10! Lucky us! It's index 7
            else if ((sl.prior & 0xc0) == 0xc0)
                c = (sl.colpf3 & 0xf0) >> 4;    // use the correct P5 chroma, but the luminence may be off
        }

        for (int z = max(start, pmg.hposmPixStart[i]); z < min(stop, pmg.hposmPixStop[i]); z++)
        {
            BYTE *pq = qb + z;
            if (b2 & (0x02 >> ((z - pmg.hposmPixStart[i]) >> pmg.cwm[i])))
                *pq |= c;
        }
    }
}

#pragma optimize("",on)

__inline void IncDLPC(int iVM)
{
    DLPC = (DLPC & 0xFC00) | ((DLPC+1) & 0x03FF);
}

// Given our current scan mode, how many bits is it convenient to write at a time?
short BitsAtATime(int iVM)
{
    short it = mpMdBytes[sl.modelo] >> 3;    // how many multiples of 8 bytes per scan line in narrow mode
    short i = 8;        // assume 8
    if (it == 2)
        i <<= 1;    // lo res mode - write 16 bits at a time
    else if (it == 1)
        i <<= 2;    // even lower res mode - writes 32 bits at a time
    return i;
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

////////////////////////////////////////////////////
// 1. INITIAL SET UP WHEN WE FIRST START A SCAN LINE
////////////////////////////////////////////////////

// call before Go6502 to figure out what mode this scan line will be and how many cycles can execute during it

void PSLPrepare(int iVM)
{
    if (PSL == 0 || PSL == X8)    // or just trust the caller and don't check
    {
        PSL = 0;    // allow PSL to do stuff again

        // overscan area - all blank lines and never the first scan line of a mode (so there's no DMA fetch)
        if (wScan < wStartScan || wScan >= wStartScan + wcScans)
        {
            sl.modelo = 0;
            sl.modehi = 0;
            iscan = 8;  // this and sl.vscrol not being equal means we're not the first scan line of a mode, so no PF DMA
            sl.vscrol = 0;
            return;
        }

        // we are entering visible territory (240 lines are visible - usually 24 blank, 192 created by ANTIC, 24 blank)
        if (wScan == wStartScan)
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
                    wScan, DLPC, cpuPeekB(iVM, DLPC));
#endif

            sl.modehi = cpuPeekB(iVM, DLPC);
            //ODS("scan %04x FETCH %02x %02x\n", wScan, sl.modehi, sl.modelo);
            sl.modelo = sl.modehi & 0x0F;
            sl.modehi >>= 4;
            IncDLPC(iVM);

            fFetch = FALSE;

            // vertical scroll bit enabled - you can only start vscrolling on a real mode
            if ((sl.modehi & 2) && !sl.fVscrol && (sl.modelo >= 2))
            {
                sl.fVscrol = TRUE;
                sl.vscrol = VSCROL & 15;    // stores the first value of iscan to see if this is the first scan line of the mode
                iscan = sl.vscrol;    // start displaying at this scan line. If > # of scan lines per mode line (say, 14)
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

                    w = cpuPeekB(iVM, DLPC);
                    IncDLPC(iVM);
                    w |= (cpuPeekB(iVM, DLPC) << 8);

                    DLPC = w;
                    //ODS("Scan %04x JVB\n", wScan);
                }
                break;

            default:
                // normal display list entry

                // how many scan lines a line of this graphics mode takes
                scans = (BYTE)mpMdScans[sl.modelo] - 1;

#if 0    // I fixed cycle counting, and now VSCROL is correctly set to 13
                // GR.9++ hack, if this is a single scan line mode, VSCROL is 3 instead of 13 like it's supposed to be
                if (iscan > scans && scans == 0 && iscan <= 8)
                {
                    iscan = 16 - iscan;
                    sl.vscrol = iscan; // this is the first scan line of a mode line
                }
#endif

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

            // time to stop vscrol, this line doesn't use it.
            // !!! Stop if the mode is different than the mode when we started scrolling? I don't think so...
            // allow blank mode lines to mean duplicates of previous lines (GR.9++)
            if (sl.fVscrol && (!(sl.modehi & 2)))
            {
                sl.fVscrol = FALSE;
                // why is somebody setting the high bits of this sometimes?
                scans = VSCROL & 0x0f;    // the first mode line after a VSCROL area is truncated, ending at VSCROL
                //ODS("%d FINISH: scans=%d\n", wScan, scans);
            }
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

// DLI now happens inside of Go6502 at the proper cycle (10)
#if 0
        // generate a DLI if necessary. Do so on the last scan line of a graphics mode line
        // a JVB instruction with DLI set keeps firing them all the way to the VBI (Race in Space)
        if ((sl.modehi & 8) && (iscan == scans || (fWait & 0x08)))
        {
#ifndef NDEBUG
            extern BOOL  fDumpHW;

            if (fDumpHW)
                printf("DLI interrupt at scan %d\n", wScan);
#endif

            // set DLI, clear VBI leave RST alone - even if we don't take the interrupt
            NMIST = (NMIST & 0x20) | 0x9F;
            if (NMIEN & 0x80)    // DLI enabled
            {
                Interrupt(iVM, FALSE);
                regPC = cpuPeekW(iVM, 0xFFFA);

                // the main code may be waiting for a WSYNC, but in the meantime this DLI should NOT. on RTI set it back.
                // This won't work for nested interrupts, or if DLI finishes either before or after WSYNC point!
                WSYNC_Waiting = FALSE;
                WSYNC_on_RTI = TRUE;

                if (regPC == bp)
                    fHitBP = TRUE;
            }
        }
#endif

        // Check playfield width and set cbWidth (number of bytes read by Antic)
        // and the smaller cbDisp (number of bytes actually visible)

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
            cbWidth = mpMdBytes[sl.modelo];        // cbDisp: use NARROW number of bytes per graphics mode line, eg. 32
            cbDisp = cbWidth;
            if ((sl.modehi & 1) && (sl.modelo >= 2)) // hor. scrolling, cbWidth mimics NORMAL
                cbWidth |= (cbWidth >> 2);
            break;
        case 2: // normal playfield
            cbWidth = mpMdBytes[sl.modelo];        // bytes in NARROW mode, eg. 32
            cbDisp = cbWidth | (cbWidth >> 2);    // cbDisp: boost to get bytes per graphics mode line in NORMAL mode, eg. 40
            if ((sl.modehi & 1) && (sl.modelo >= 2)) // hor. scrolling?
                cbWidth |= (cbWidth >> 1);            // boost cbWidth to mimic wide playfield, eg. 48
            else
                cbWidth |= (cbWidth >> 2);            // otherwise same as cbDisp
            break;
        case 3: // wide playfield
            cbWidth = mpMdBytes[sl.modelo];        // NARROW width
            cbDisp = cbWidth | (cbWidth >> 2) | (cbWidth >> 3);    // visible area is half way between NORMAL and WIDE
            cbWidth |= (cbWidth >> 1);            // WIDE width
            break;
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
        int j = 0;
        if (cbDisp != cbWidth)
        {
            // wide is 48, normal is 40, that's j=4 (split the difference) characters before we begin.
            // subtract one for every multiple of 8 hshift is, that's an entire character shift
            j = ((cbWidth - cbDisp) >> 1) - ((hshift) >> 3);
            hshift &= 7;    // now only consider the part < 8
        }

        if (((wAddr + j) & 0xFFF) < 0xFD0)  // even wide playfield won't wrap a 4K boundary
        {
            // tough to avoid memcpy if we support wrapping on a 4K boundary below
            _fmemcpy(sl.rgb, &rgbMem[wAddr + j], cbWidth);
            rgbSpecial = rgbMem[wAddr + j - 1];    // the byte just offscreen to the left may be needed if scrolling
        }
        else
        {
            for (int i = 0; i < cbWidth; i++)
                sl.rgb[i] = cpuPeekB(iVM, (wAddr & 0xF000) | ((wAddr + i + j) & 0x0FFF));
            rgbSpecial = cpuPeekB(iVM, (wAddr & 0xF000) | ((wAddr + j - 1) & 0x0FFF));    // ditto
        }

        // Other things we only need once per scan line

        sl.chbase = CHBASE & ((sl.modelo < 6) ? 0xFC : 0xFE);
        sl.chactl = CHACTL & 7;
        sl.addr = wAddr;
        sl.dmactl = DMACTL;

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
                pmg.grafp0 = cpuPeekB(iVM, (pmg.pmbase << 8) + 512 + (wScan >> 1));
                pmg.grafp1 = cpuPeekB(iVM, (pmg.pmbase << 8) + 640 + (wScan >> 1));
                pmg.grafp2 = cpuPeekB(iVM, (pmg.pmbase << 8) + 768 + (wScan >> 1));
                pmg.grafp3 = cpuPeekB(iVM, (pmg.pmbase << 8) + 896 + (wScan >> 1));
            }
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
        }

        // we want missiles, but a constant value, not memory lookup. GRACTL does NOT have to be set!
        else
            pmg.grafm = GRAFM;

        // If there is PMG data on this scan line, turn on special bitfield mode to deal with it, and init that buffer
        // Even if they're off screen now, they could be moved on screen at any moment!
        sl.fpmg = (pmg.grafpX || pmg.grafm);

        // If GTIA is enabled, change mode 15 into 16, 17 or 18 for GR. 9, 10 or 11
        // Be brave, and if GTIA is turned off halfway down the screen, turn back!
        if (sl.prior & 0xC0)
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
        pmg.fHiRes = (sl.modelo == 2 || sl.modelo == 3 || sl.modelo == 15);
    }
}

void PSLPostpare(int iVM)
{
    /////////////////////////////////////////////////////////////////////
    // 4. WHEN DONE, MOVE ON TO THE NEXT SCAN LINE NEXT TIME WE'RE CALLED
    /////////////////////////////////////////////////////////////////////

    if (PSL == X8)
    {
        // When we're done with this mode line, fetch again. If we couldn't fetch because DMA was off, keep wanting to fetch
        if (!fFetch)
            fFetch = (iscan == scans);
        iscan = (iscan + 1) & 15;

        if (fFetch)
        {
            // ANTIC's PC can't cross a 4K boundary, poor thing
            wAddr = (wAddr & 0xF000) | ((wAddr + cbWidth) & 0x0FFF);
        }
    }
}

void PSLReadRegs(int iVM)
{
    ///////////////////////////////////////////////////////////////////////////
    // 2. RE-READ ALL THE H/W REGISTERS TO ALLOW THINGS TO CHANGE MID-SCAN LINE
    ///////////////////////////////////////////////////////////////////////////

    // Note: in GTIA mode, ALL scan lines behave somewhat like GTIA, no matter what mode. Mix modes at your peril.

    sl.prior = PRIOR;

    // note if GTIA modes enabled... no collisions to playfield in GTIA
    pmg.fGTIA = sl.prior & 0xc0;

    // check if GRAFPX or GRAFM have changed (with PMG DMA it's only fetched once per scan line, but these can change more often)
    // !!! If it changes from 0 to non-zero mid scan line, we won't do it, because it's too late, we already put ourselves into
    // non-bitfield non-fpmg mode and can't change that mid scan line

    if (!(sl.dmactl & 0x08 && GRACTL & 2))
        pmg.grafpX = GRAFPX;

    if (!((sl.dmactl & 0x04 || sl.dmactl & 0x08) && GRACTL & 1))
            pmg.grafm = GRAFM;

    // update the colour registers

    // GTIA mode GR.10 uses 704 as background colour, enforced in all scan lines of any antic mode
    sl.colbk = ((sl.prior & 0xC0) == 0x80) ? COLPM0 : COLBK;
    sl.colpfX = COLPFX;
    pmg.colpmX = COLPMX;

    // !!! VDELAY NYI

    // Did the H-pos or sizes change?
    if (pmg.hposmX != HPOSMX || pmg.hpospX != HPOSPX || pmg.sizem != SIZEM || pmg.sizepX != SIZEPX)
    {
        pmg.hposmX = HPOSMX;
        pmg.hpospX = HPOSPX;
        pmg.sizem = SIZEM;
        pmg.sizepX = SIZEPX;

        short off[4];
        int i;

        // precompute on what pixel each player and missile start and stop
        for (i = 0; i < 4; i++)
        {
            off[i] = (pmg.hposp[i] - ((NTSCx - X8) >> 2));

            pmg.cwp[i] = (BYTE)mpsizecw[pmg.sizep[i] & 3];    // normal, double or quad size?
            if (pmg.cwp[i] == 4)
                pmg.cwp[i] = 3;    //# of times to shift to divide by (cw *2)

            pmg.hpospPixStart[i] = off[i] << 1;        // first pixel affected by this player, compare to start
            pmg.hpospPixStop[i] = pmg.hpospPixStart[i] + (8 << pmg.cwp[i]);    // the pixel after the last one affected, compare to stop

            // now do missiles

            off[i] = (pmg.hposm[i] - ((NTSCx - X8) >> 2));

            pmg.cwm[i] = (BYTE)mpsizecw[((pmg.sizem >> (i + i))) & 3];
            if (pmg.cwm[i] == 4)
                pmg.cwm[i] = 3;    //# of times to shift to divide by (cw *2)

            pmg.hposmPixStart[i] = off[i] << 1;        // first pixel affected by this player, compare to start
            pmg.hposmPixStop[i] = pmg.hposmPixStart[i] + (2 << pmg.cwm[i]);    // the pixel after the last one affected, compare to stop
        }
    }
}

// We have wLeft CPU cycles left to execute on this scan line.
// Our DMA tables will translate that to which pixel is currently being drawn.
// PSL is the first pixel not drawn last time. Process the appropriate number of
// pixels of this scan line. At the end of the scan line, make sure to call us
// with wLeft <= 0 to finish up.

BOOL ProcessScanLine(int iVM)
{
    // don't do anything in the invisible top and bottom sections
    if (wScan < wStartScan)
        return 0;

    if (wScan >= wStartScan + wcScans)
        return 0;    // uh oh, ANTIC is over - extending itself, but we have no buffer to draw into

    // what pixel is the electron beam drawing at this point (wLeft)? While executing, wLeft will be between 1 and 114
    // so the index into the DMA array, which is 0-based, is (wLeft - 1). That tells us the horizontal clock cycle we're on,
    // which is an index into the PIXEL array to tell us the pixel we're drawing at this moment.
    // Finally, once the whole scan line executes, we will be called with wLeft == 0 to finish things up
    // if wLeft < 0, we're hopefully not in the visible part of the screen (that doesn't happen til at least -5) so it's OK to just use 0.
    short cclock = rgPIXELMap[DMAMAP[wLeft > 0 ? wLeft - 1 : 0]];

    // what part of the scan line were we on last time?
    short cclockPrev = PSL;

    if (cclock <= PSL)
        return TRUE;    // nothing to do

    PSL = cclock;    // next time we're called, start from here

    // now we are responsible for drawing starting from location cclockPrev up to but not including cclock

    PSLReadRegs(iVM);    // read our hardware registers to find brand-new values to use starting this cycle

    ////////////////////////////////////////////////////////////////
    // 3. DRAW THIS PORTION OF THE SCAN LINE - THIS IS A LOT OF CODE
    ////////////////////////////////////////////////////////////////

    // PMG can exist outside the visible boundaries of this line, so we need an extra long line to avoid complicating things
    BYTE rgpix[NTSCx];    // an entire NTSC scan line, including retrace areas, used in PMG bitfield mode

    int i, j;

    // Based on the graphics mode, fill in a scanline's worth of data. It might be the actual data, or if PMG exist on this line,
    // a bitfield simply saying which playfield is at each pixel so the priority of which should be visible can be worked out later
    // GTIA modes are a kind of hybrid approach since it's not so simple as which playfield colour is visible
    //
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

        if (sl.fpmg)
        {
            // When PM/G are present on the scan line is first rendered
            // into rgpix as bytes of bit vectors representing the playfields
            // and PM/G that are present at each pixel. Then later we map
            // rgpix to actual colors on the screen, applying priorities
            // in the process.

            qch = &rgpix[(NTSCx - X8) >> 1];    // first visible screen NTSC pixel

            // GTIA modes 9, 10 & 11 don't use the PF registers since they can have 16 colours instead of 4
            // so we CANNOT do a special bitfield mode. See the PRIOR code
            if (!(sl.prior & 0xc0))
            {
                sl.colbk = bfBK;
                sl.colpf0 = bfPF0;
                sl.colpf1 = bfPF1;
                sl.colpf2 = bfPF2;
                sl.colpf3 = bfPF3;
            }
        }
        else
        {
            // not doing a bitfield, just write into this scan line
            qch += (wScan - wStartScan) * vcbScan;
        }

        // what is the background colour? Normally it's sl.colbk, but in pmg mode we are using an index of 0 to represent it.
        // Unless we're in GR.9 or GR.11, in which case we're still using sl.colbk, but in the case of pmg, a 4-bit version
        // We've already accounted above for GR.10 having a different register for its background colour.
        BYTE bkbk = sl.fpmg ? 0 : sl.colbk;
        if (sl.prior & 0x40)    // GR.9 or 11
        {
            bkbk = sl.colbk;
            if (sl.fpmg && (sl.prior & 0xc0) == 0xc0) // GR.11 the interesting part is the high nibble
                bkbk = sl.colbk >> 4;
            if (sl.fpmg)
                bkbk = bkbk & 0x0f;
        }

        // narrow playfield - if we're in bitfield mode and GTIA GR. 9-11, the background colour has to occupy the lower nibble only.
        // in GR.10, it's the index to the background colour, since the background colour itself is 8 bits and won't fit in a nibble.

        short bbars = 0;
        if (sl.modelo >= 2)
        {
            if ((sl.dmactl & 3) == 1)
                bbars = (X8 - NARROWx) >> 1;    // width of the "black" bar on the side of the visible playfield area
            else if ((sl.dmactl & 3) == 2)
                bbars = (X8 - NORMALx) >> 1;

            if (bbars)
            {
                if (cclockPrev < bbars)
                    memset(qch + cclockPrev, bkbk, min(bbars, cclock) - cclockPrev);        // draw background colour before we start real data
                if (cclock >= X8 - bbars)
                    memset(qch + max(cclockPrev, X8 - bbars), bkbk, cclock - (max(cclockPrev, X8 - bbars)));    // after finished real data
            }
        }

        qch += cclockPrev;

        // i is our loop inside each case statement for each possible mode. Set up where in the loop we need to start
        // skip the portion inside the bar to where we start drawing real data
        i = cclockPrev - bbars;
        if (cclockPrev < bbars)
        {
            qch += (bbars - cclockPrev);
            i = 0;
        }

        // i is a bit index, now make it a segment index, where each scan mode likes to create so many bits at a time per segment
        i /= BitsAtATime(iVM);

        // and where do we stop drawing? Don't go into the right hand bar
        short iTop = cclock - bbars;
        if (iTop <= 0)
            iTop = 0;    // inside the left side bar
        else if (iTop > X8 - bbars - bbars)
            iTop = (X8 - bbars - bbars) / BitsAtATime(iVM);    // inside the right side bar
        else
        {
            iTop -= 1;    // any remainder needs to increase the quotient by 1 (any partial segment required fills the whole segment)
            iTop = iTop / BitsAtATime(iVM) + 1;
        }

        // nothing to draw! Our window is zero
        if (i == iTop)
        {
            PSLPostpare(iVM);
            return TRUE;
        }

        // We kind of have to draw more than asked to, so increment our bounds
        short newTop = iTop * BitsAtATime(iVM) + bbars;
        if (newTop > cclock)
        {
            cclock = newTop;
            PSL = cclock;
        }

#if 0 // !!! This shouldn't be necessary?
        if (sl.fpmg)
            // zero out the section we'll be using, now that we know the proper value of cclock
            _fmemset(&rgpix[(NTSCx - X8) >> 1] + cclockPrev, 0, cclock - cclockPrev);
#endif

        // just fill in whatever we need to indicate background colour for our portion if there is no mode
        if (sl.modelo < 2)
            _fmemset(qch, bkbk, cclock - cclockPrev);

        switch (sl.modelo)
        {

        // GR.0 and descended character GR.0
        case 2:
        case 3:
            BYTE vpixO = vpix % (sl.modelo == 2 ? 8 : 10);

            // mimic obscure ANTIC behaviour (why not?) Scans 10-15 duplicate 2-7, not 0-5
            if (sl.modelo == 3 && iscan > 9)
                vpixO = iscan - 8;

            vpix = vpixO;

            // the artifacting colours - !!! this behaves like NTSC, PAL has somewhat random artifacting
            BYTE red = 0x40 | (sl.colpf1 & 0x0F), green = 0xc0 | (sl.colpf1 & 0x0F);
            BYTE yellow = 0xe0 | (sl.colpf1 & 0x0F);

            // just for fun, don't interlace in B&W
            BOOL fArtifacting = (rgvm[iVM].bfMon == monColrTV);

            col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);
            col2 = sl.colpf2;

            if ((sl.chactl & 4) && sl.modelo == 2)    // vertical reflect bit
                vpix = 7 - (vpix & 7);

            for (; i < iTop; i++)
            {
                b1 = sl.rgb[i];

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
                        // !!! was 0xFC, not 0xFE
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

                // undocumented GR.9 mode based on GR.0 - dereference through a character set to get the bytes to put on the screen,
                // but treat them as GR.9 luminences
                if ((sl.prior & 0xC0) == 0x40)
                {
                    col1 = (b2 >> 4) | (sl.colbk /* & 0xf0 */);    // let the user screw up the colours if they want, like a real 810
                    col2 = (b2 & 15) | (sl.colbk /* & 0xf0*/); // they should only POKE 712 with multiples of 16

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

                // undocumented GR.10 mode based on GR.0 - dereference through a character set to get the bytes to put on the screen,
                // but treat them as GR.10 indexes
                else if ((sl.prior & 0xC0) == 0x80)
                {

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
                            col1 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                        else
                            col1 = *(((BYTE FAR *)&COLPM0) + col1 - 8);    // col. 12-15 are copies of c.4-7

                        if (col2 < 9)
                            col2 = *(((BYTE FAR *)&COLPM0) + col2);
                        else if (col2 < 12)
                            col2 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                        else
                            col2 = *(((BYTE FAR *)&COLPM0) + col2 - 8);    // col. 12-15 are copies of c.4-7
                    }
                    else
                    {
                        if (col1 < 9)
                            ;
                        else if (col1 < 12)
                            col1 = 8;            // col. 9-11 are copies of c.8
                        else
                            col1 = col1 - 8;    // col. 12-15 are copies of c.4-7

                        if (col2 < 9)
                            ;
                        else if (col2 < 12)
                            col2 = 8;            // col. 9-11 are copies of c.8
                        else
                            col2 = col2 - 8;    // col. 12-15 are copies of c.4-7
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

                // undocumented GR.11 mode based on GR.0 - dereference through a character set to get the bytes to put on the screen,
                // but treat them as GR.11 chromas
                else if ((sl.prior & 0xC0) == 0xC0)
                {
                    col1 = ((b2 >> 4) << 4) | (sl.colbk /* & 15*/);    // lum comes from 712
                    col2 = ((b2 & 15) << 4) | (sl.colbk /* & 15*/); // keep 712 <16, if not, it deliberately screws up like the real hw

                    // We're in BITFIELD mode because PMG are present, so we can only alter the low nibble.
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

                // GR.0 See mode 15 for artifacting theory of operation
                else
                {
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
                }
//                _fmemcpy(qch,rgch,8);
            }
            break;

        case 5:
            vpix = iscan >> 1;    // extra thick, use screen data twice for 2 output lines
        case 4:
            if (sl.chactl & 4)
                vpix ^= 7;                // vertical reflect bit
            vpix &= 7;

            col0 = sl.colbk;
            col1 = sl.colpf0;
            col2 = sl.colpf1;
//            col3 = sl.colpf2;

            for (; i < iTop; i++)
            {
                b1 = sl.rgb[i];

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

            for (; i < iTop; i++)
            {
                b1 = sl.rgb[i];

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
                        if (j < hshift)    // hshift restricted to 7 or less, so this is sufficient
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
            break;

        case 8:
            col0 = sl.colbk;
            col1 = sl.colpf0;
            col2 = sl.colpf1;
            col3 = sl.colpf2;

            for (; i < iTop; i++)
            {
                b2 = sl.rgb[i];

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

            for (; i < iTop; i++)
            {
                b2 = sl.rgb[i];

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

            for (; i < iTop; i++)
            {
                b2 = sl.rgb[i];

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

            for (; i < iTop; i++)
                {
                b2 = sl.rgb[i];

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

            for (; i < iTop; i++)
                {
                b2 = sl.rgb[i];

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
            col1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);    // like GR.0, the colour is just a luminence of the background colour
            col2 = sl.colpf2;

            // the artifacting colours - !!! this behaves like NTSC, PAL has somewhat random artifacting
            red = 0x40 | (sl.colpf1 & 0x0F), green = 0xc0 | (sl.colpf1 & 0x0F);
            yellow = 0xe0 | (sl.colpf1 & 0x0F);

            // just for fun, don't interlace in B&W
            fArtifacting = (rgvm[iVM].bfMon == monColrTV);

            for (; i < iTop; i++)
            {
                b2 = sl.rgb[i];

                WORD u = b2;

                // do you have the hang of this by now? Use hshift, not sl.hscrol
                if (hshift)
                {
                    // non-zero horizontal scroll

                    // this shift may involve our byte and the one before it
                    u = ((i == 0 ? rgbSpecial : sl.rgb[i - 1]) << 8) | (BYTE)b2;
                }

                // what bit position in the WORD u do we start copying from?
                int index = 7 + hshift;    // ATARI can only shift 2 pixels minimum at this resolution

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

            for (; i < iTop; i++)
                {
                b2 = sl.rgb[i];

                // GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
                // use low nibble of previous byte
                if (hshift & 0x04)
                {
                    b2 = b2 >> 4;
                    b2 |= (((i == 0 ? rgbSpecial : sl.rgb[i - 1]) & 0x0f) << 4);
                }

                col1 = (b2 >> 4) | (sl.colbk /* & 0xf0 */);    // let the user screw up the colours if they want, like a real 810
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

            for (; i < iTop; i++)
                {
                b2 = sl.rgb[i];

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
                        col1 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                    else
                        col1 = *(((BYTE FAR *)&COLPM0) + col1 - 8);    // col. 12-15 are copies of c.4-7

                    if (col2 < 9)
                        col2 = *(((BYTE FAR *)&COLPM0) + col2);
                    else if (col2 < 12)
                        col2 = *(((BYTE FAR *)&COLPM0) + 8);    // col. 9-11 are copies of c.8
                    else
                        col2 = *(((BYTE FAR *)&COLPM0) + col2 - 8);    // col. 12-15 are copies of c.4-7
                }
                else
                {
                    if (col1 < 9)
                        ;
                    else if (col1 < 12)
                        col1 = 8;            // col. 9-11 are copies of c.8
                    else
                        col1 = col1 - 8;    // col. 12-15 are copies of c.4-7

                    if (col2 < 9)
                            ;
                    else if (col2 < 12)
                        col2 = 8;            // col. 9-11 are copies of c.8
                    else
                        col2 = col2 - 8;    // col. 12-15 are copies of c.4-7
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

            for (; i < iTop; i++)
                {
                b2 = sl.rgb[i];

                // GTIA only allows scrolling on a nibble boundary, so this is the only case we care about
                // use low nibble of previous byte
                if (hshift & 0x04)
                {
                    b2 = b2 >> 4;
                    b2 |= (((i == 0 ? rgbSpecial : sl.rgb[i - 1]) & 0x0f) << 4);
                }

                // !!! restrict all drawing in ProcessScanLine of the background to lum=0 in ANTIC mode 18 (GR.11)!

                col1 = ((b2 >> 4) << 4) | (sl.colbk /* & 15*/);    // lum comes from 712
                col2 = ((b2 & 15) << 4) | (sl.colbk /* & 15*/); // keep 712 <16, if not, it deliberately screws up like the real hw

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
    if (!(sl.prior & 0xc0))
    {
        sl.colbk = ((sl.prior & 0xC0) == 0x80) ? COLPM0 : COLBK;
        sl.colpfX = COLPFX;
    }

   // even with Fetch DMA off, PMG DMA might be on
    if (sl.fpmg)
    {
        BYTE *qch = vrgvmi[iVM].pvBits;

        // !!! VDELAY NYI

        // now set the bits in rgpix corresponding to players and missiles. Must be in this order for correct collision detection
        // tell them what range they are to fill in data for (PSL to cclock)
        DrawPlayers(iVM, (rgpix + ((NTSCx - X8)>>1)), cclockPrev, cclock);
        DrawMissiles(iVM, (rgpix + ((NTSCx - X8)>>1)), sl.prior & 16, cclockPrev, cclock);    // 5th player?

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

        // now map the rgpix array to the screen

        qch += (wScan - wStartScan) * vcbScan;

        // turn the rgpix array from a bitfield of which items are present (player or field colours)
        // into the actual colour that will show up there, based on priorities
        // reminder: b = PM3 PM2 PM1 PM0 | PF3 PF2 PF1 PF0 (all zeroes means BKGND)

        // EXCEPT for GR.9, 10 & 11, which does not have playfield bitfield data. There are 16 colours in those modes and only 4 regsiters.
        // So don't try to look at PF bits. In GR.9 & 11, all playfield is underneath all players.

        for (i = cclockPrev; i < cclock; i++)
        {
            BYTE b = rgpix[i+((NTSCx - X8)>>1)];

            // if in a hi-res 2 color mode, set playfield 1 color to some luminence of PF2
            if (pmg.fHiRes && (b & bfPF1))
                sl.colpf1 = (sl.colpf2 & 0xF0) | (sl.colpf1 & 0x0F);

            // If PF3 and PF1 are present, that can only happen in 5th player mode, so alter PF3's colour to match the luma of PF1
            // (so that text shows up on top of a fifth player using its chroma). Put it back later.
            BYTE colpf3Sav = sl.colpf3;
            if (pmg.fHiRes && (b & bfPF3) && (b & bfPF1))
                sl.colpf3 = (sl.colpf3 & 0xF0) | (sl.colpf1 & 0x0F);

            BYTE colpm0 = pmg.colpm0;
            BYTE colpm1 = pmg.colpm1;
            BYTE colpm2 = pmg.colpm2;
            BYTE colpm3 = pmg.colpm3;

            // in hi-res modes, text is always visible on top of a PMG, because the colour is altered to have PF1's luma
            if (pmg.fHiRes && (b & bfPF1))
            {
                colpm0 = (colpm0 & 0xf0) | (sl.colpf1 & 0x0f);
                colpm1 = (colpm1 & 0xf0) | (sl.colpf1 & 0x0f);
                colpm2 = (colpm2 & 0xf0) | (sl.colpf1 & 0x0f);
                colpm3 = (colpm3 & 0xf0) | (sl.colpf1 & 0x0f);
            }

            // !!! Fifth player colour is altered in GTIA modes (pg. 108) NYI

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

            // OK, based on priority, which of the elements do we show? It could be more than one thing
            // (overlap mode ORs the colours together). Any or all of the players might be in the same spot.
            // Normally only one playfield colour is in any one spot, but a fifth player is like PF3 so there
            // could be two playfields set here. Player 5 (PF3) gets priority unless its a hi-res mode
            // where PF1 text goes on top

            BYTE SP0 = P0 & ~(PF01 & PRI23) & ~(PRI2 & PF23);
            BYTE SP1 = P1 & ~(PF01 & PRI23) & ~(PRI2 & PF23) & (~P0 | MULTI);
            BYTE SP2 = P2 & ~P01 &  ~(PF23 & PRI12) &  ~(PF01 & ~PRI0);
            BYTE SP3 = P3 & ~P01 & ~(PF23 & PRI12) & ~(PF01 & ~PRI0) & (~P2 | MULTI);

            // usually, the fifth player is above all playfields, even with priority 8
            // !!! NYI quirk where fifth player is below players in prior 8 if PF0 and PF1 not present

            BYTE SF3 = PF3 & ~(P23 & PRI03) & ~(P01 & ~PRI2);
            BYTE SF0 = PF0 & ~(P23 & PRI0) &  ~(P01 & PRI01) & ~SF3;
            BYTE SF1 = PF1 &  ~(P23 & PRI0) & ~(P01 & PRI01) & ~SF3;
            BYTE SF2 = PF2 &  ~(P23 & PRI03) & ~(P01 & ~PRI2) & ~SF3;
            BYTE SB = ~P01 & ~P23 & ~PF01 & ~PF23;

            // GR. 9 - low nibble is the important LUM value. Now add back to chroma value
            // Remember, GR.0 can be a secret GTIA mode
            if ((sl.prior & 0xc0) == 0x40)
                if (P01 || P23)    // all players visible over all playfields
                    b = (P0 & pmg.colpm0) | (P1 & pmg.colpm1) | (P2 & pmg.colpm2) | (P3 & pmg.colpm3);
                else
                    b = (b & 0x0f) | sl.colbk;

            // GR. 10 - low nibble is an index into the colour to use
            // Remember, GR.0 can be a secret GTIA mode
            else if ((sl.prior & 0xc0) == 0x80)
                if (P01 || P23)    // !!! not proper, but I'm making all players overtop all playfields
                    b = (P0 & pmg.colpm0) | (P1 & pmg.colpm1) | (P2 & pmg.colpm2) | (P3 & pmg.colpm3);
                else
                    b = *(&COLPM0 + b);

            // GR. 11 - low nibble is the important CHROM value. We'll shift it back to the high nibble and add the LUM value later
            // Remember, GR.0 can be a secret GTIA mode
            else if ((sl.prior & 0xc0) == 0xC0)
                if (P01 || P23)    // all players above all playfields
                    b = (P0 & pmg.colpm0) | (P1 & pmg.colpm1) | (P2 & pmg.colpm2) | (P3 & pmg.colpm3);
                else
                    b = (b << 4) | sl.colbk;

            else
            {
                // OR together all the colours of all the visible things (ovelap mode support)

                // In hi-res modes (GR.0 & 8) each player colour is given the luminence of PF1 whenever they overlap
                // even if the playfields are all behind the players. In other words, all text is readable on top of
                // player missile graphics, and if we don't do this, all text disappears

                b = (SP0 & colpm0) | (SP1 & colpm1) | (SP2 & colpm2) | (SP3 & colpm3);
                b = b | (SF0 & sl.colpf0) | (SF1 & sl.colpf1) | (SF2 & sl.colpf2) | (SF3 & sl.colpf3) | (SB & sl.colbk);
            }

            // put this back if we temporarily changed it to support text visible through the fifth player
            sl.colpf3 = colpf3Sav;

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

    PSLPostpare(iVM);    // see if we're done this scan line and be ready to do the next one

    return TRUE;
}

void ForceRedraw(int iVM)
{
    iVM;
    //memset(rgsl,0,sizeof(rgsl));
}

#endif // XFORMER

