
/***************************************************************************

    ATARI800.C

    - Implements the public API for the Atari 800 VM.

    Copyright (C) 1986-2018 by Darek Mihocka. All Rights Reserved.
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

    X8,        // screen width
    Y8,        // screen height
    8,        // we support 2^8 colours
    rgbRainbow, // our 256 colour palette
    TRUE,    // fUsesCart
    FALSE,    // fUsesMouse
    TRUE,    // fUsesJoystick
    "Xformer/SIO2PC Disks\0*.xfd;*.atr;*.sd;*.dd\0All Files\0*.*\0\0",
    "Xformer Cartridge\0*.bin;*.rom;*.car\0All Files\0*.*\0\0",

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

// the ATARI colour palette, there is no official colour chart, we kind of have to make it up
BYTE    rgbRainbow[256 * 3] =
{
    0,     0,     0,               // $00
    8,     8,     8,
    16,    16,    16,
    24,    24,    24,
    31,    31,    31,
    36,    36,    36,
    40,    40,    40,
    44,    44,    44,
    47,    47,    47,
    50,    50,    50,
    53,    53,    53,
    56,    56,    56,
    58,    58,    58,
    60,    60,    60,
    62,    62,    62,
    63,    63,    63,
    6,     5,     0,               // $10
    12,    10,     0,
    24,    15,     1,
    30,    20,     2,
    35,    27,     3,
    39,    33,     6,
    44,    42,    12,
    47,    44,    17,
    49,    48,    20,
    52,    51,    23,
    54,    53,    26,
    55,    55,    29,
    56,    56,    33,
    57,    57,    36,
    58,    58,    39,
    59,    59,    41,
    9,      5,     0,               // $20
    14,     9,     0,
    20,    13,     0,
    28,    20,     0,
    36,    28,     1,
    43,    33,     1,
    47,    39,    10,
    49,    43,    17,
    51,    46,    24,
    53,    47,    26,
    55,    49,    28,
    57,    50,    30,
    59,    51,    32,
    60,    53,    36,
    61,    55,    39,
    62,    56,    40,
    11,     3,     1,               // $30
    18,     5,     2,
    27,     7,     4,
    36,    11,     8,
    44,    20,    13,
    46,    24,    16,
    49,    28,    21,
    51,    30,    25,
    53,    35,    30,
    54,    38,    34,
    55,    42,    37,
    56,    43,    38,
    57,    44,    39,
    57,    46,    40,
    58,    48,    42,
    59,    49,    44,
    11,     1,     3,               // $40
    22,     6,     9,
    37,    10,    17,
    42,    15,    22,
    45,    21,    28,
    48,    24,    30,
    50,    26,    32,
    52,    28,    34,
    53,    30,    36,
    54,    33,    38,
    55,    35,    40,
    56,    37,    42,
    57,    39,    44,
    58,    41,    45,
    59,    42,    46,
    60,    43,    47,
    12,     0,    11,               // $50
    20,     2,    18,
    28,     4,    26,
    39,     8,    37,
    48,    18,    49,
    53,    24,    53,
    55,    29,    55,
    56,    32,    56,
    57,    35,    57,
    58,    37,    58,
    59,    39,    59,
    59,    41,    59,
    59,    42,    59,
    59,    43,    59,
    59,    44,    59,
    60,    45,    60,
    5,     1,    16,               // $60
    10,     2,    32,
    22,    10,    46,
    27,    15,    49,
    32,    21,    51,
    35,    25,    52,
    38,    28,    53,
    40,    32,    54,
    42,    35,    55,
    44,    37,    56,
    46,    38,    57,
    47,    40,    57,
    48,    41,    58,
    49,    43,    58,
    50,    44,    59,
    51,    45,    59,
    0,     0,    13,               // $70
    4,     4,    26,
    10,    10,    46,
    18,    18,    49,
    24,    24,    53,
    27,    27,    54,
    30,    30,    55,
    33,    33,    56,
    36,    36,    57,
    39,    39,    57,
    41,    41,    58,
    43,    43,    58,
    44,    44,    59,
    46,    46,    60,
    48,    48,    61,
    49,    49,    62,
    1,     7,    18,               // $80
    2,    13,    30,
    3,    19,    42,
    4,    24,    42,
    9,    28,    45,
    14,    32,    48,
    17,    35,    51,
    20,    37,    53,
    24,    39,    55,
    28,    41,    56,
    31,    44,    57,
    34,    46,    57,
    37,    47,    58,
    39,    48,    58,
    41,    49,    59,
    42,    50,    60,
    1,     4,    12,               // $90
    2,     6,    22,
    3,    10,    32,
    5,    15,    36,
    8,    20,    38,
    15,    25,    44,
    21,    30,    47,
    24,    34,    49,
    27,    38,    52,
    29,    42,    54,
    31,    44,    55,
    33,    46,    56,
    36,    47,    57,
    38,    49,    58,
    40,    50,    59,
    42,    51,    60,
    0,     9,     7,               // $A0
    1,    18,    14,
    2,    26,    20,
    3,    35,    27,
    4,    42,    33,
    6,    47,    38,
    14,    51,    44,
    18,    53,    46,
    22,    55,    49,
    25,    56,    51,
    28,    57,    52,
    32,    58,    53,
    36,    59,    55,
    40,    60,    56,
    44,    61,    57,
    45,    62,    58,
    0,    10,     1,               // $B0
    0,    16,     3,
    1,    22,     5,
    5,    33,     7,
    9,    44,    16,
    14,    48,    21,
    19,    51,    24,
    22,    52,    28,
    24,    53,    31,
    30,    55,    35,
    36,    57,    38,
    39,    58,    41,
    41,    59,    44,
    43,    59,    47,
    46,    59,    49,
    47,    60,    50,
    3,    10,     0,               // $C0
    6,    20,     0,
    9,    30,     1,
    14,    37,     4,
    18,    44,     7,
    22,    46,    12,
    26,    48,    17,
    29,    50,    22,
    33,    52,    26,
    36,    54,    28,
    38,    55,    30,
    40,    56,    33,
    42,    57,    36,
    45,    58,    39,
    48,    59,    42,
    49,    60,    43,
    5,     9,     0,               // $D0
    11,    22,     0,
    17,    35,     1,
    23,    42,     2,
    29,    48,     8,
    34,    50,    12,
    38,    51,    17,
    40,    52,    21,
    42,    53,    24,
    44,    54,    27,
    46,    55,    29,
    47,    56,    31,
    48,    57,    34,
    50,    58,    37,
    52,    59,    40,
    53,    60,    42,
    8,     7,     0,               // $E0
    19,    16,     0,
    28,    24,     4,
    33,    31,     6,
    48,    38,     8,
    52,    44,    12,
    55,    50,    15,
    57,    52,    19,
    58,    54,    22,
    58,    56,    24,
    59,    57,    26,
    59,    58,    29,
    60,    58,    33,
    61,    59,    35,
    61,    59,    36,
    62,    60,    38,
    8,     5,     0,               // $F0
    13,     9,     0,
    22,    14,     1,
    32,    21,     3,
    42,    29,     5,
    45,    33,     7,
    48,    36,    12,
    50,    39,    18,
    53,    42,    24,
    54,    45,    27,
    55,    46,    30,
    56,    47,    33,
    57,    49,    36,
    58,    50,    38,
    58,    53,    39,
    59,    54,    40,
};

// our machine specific data for each instance (ATARI specific stuff)
//
CANDYHW *vrgcandy[MAX_VM];

// This map describes what ANTIC is doing (why it might steal the cycle) for every cycle as a horizontal scan line is being drawn
// 0 means it never steals the cycle, but somebody else might (RAM refresh)
//
// THEORY OF OPERATION - there are 456 pixels on an NTSC scan line, 4 pixels can get drawn in the time it takes for 1 CPU cycle
// (and there are 114 CPU cycles per scan line) 114 x 4 = 456. Even though for a wide playfield ANTIC starts fetching data after 9
// cycles, the first are off screen, and it's not until after 13 cycles have elapsed that you start to see data on the screen
// You'll see 4 extra cycles worth of pixels (16) out of the 8 cycles extra that ANTIC fetches for WIDE.
// So WIDE looks like this: 9 cycles. 4 invisible cycles. 4 visible wide cycles (that would be black bars in NORMAL width).
// Then 80 cycles of NORMAL width pixels (320 pixels) then 8 more wide cycles (4 of which are visible).
// That's the WSYNC point. The end of the invisible extra wide pixels. Then 9 cycles after WSYNC to balance out the first 9.
// 
// PMG HPOS = 40 is where you start to be able to see a PMG. Which as explained above is cycle 13. PMG are 1/2 resolution
// of a hi-res playfield, so 40 PMG pixels is like 80 pixels which means it's 20 CPU cycles. Which means PMG HPOS = 0
// is at cycle -7, or 28 pixels before CPU cycle 0. 28 pixels on either side would add 56 pixels to the 456 pixel scan line,
// which is, surprise, surprise, 512. Haven't you always wondered about that?
//
// In keeping with my theory that ANTIC lags behind GTIA, the real ANTIC starts PF DMA late, not at 9, meaning
// that although I am cycle accurate in how many cycles I give to the CPU, I'm supposed to give it one earlier in the scan
// line and take it away later in the scan line. This actually breaks HARDB/CHESS, so I jigger an A8 and A2 cycle from 
// early to late to avoid having to implement the complexity of exactly how it happens
const BYTE rgDMA[HCLOCKS] =
{
    /*  0 - 8  */ DMA_M, DMA_DL, DMA_P, DMA_P, DMA_P, DMA_P, DMA_LMS, DMA_LMS, 0,
    /*  9 - 16 */ W8, WC4, W2, WC2, W4, WC4, W2, WC2,    // ANTIC fetches the overscan early WIDE pixels it never uses :-(
    /* 17 - 24 */ N8, NC4, N2, NC2, N4, NC4, N2, NC2,
//  /* 25 - 88 */ A8, AC4, A2, AC2, A4, AC4, A2, AC2, A8, AC4, A2, AC2, A4, AC4, A2, AC2,
    /* 25 - 88 */  0, AC4,  0, AC2, A4, AC4, A2, AC2, A8, AC4, A2, AC2, A4, AC4, A2, AC2,
                  A8, AC4, A2, AC2, A4, AC4, A2, AC2, A8, AC4, A2, AC2, A4, AC4, A2, AC2,
                  A8, AC4, A2, AC2, A4, AC4, A2, AC2, A8, AC4, A2, AC2, A4, AC4, A2, AC2,
                  A8, AC4, A2, AC2, A4, AC4, A2, AC2, A8, AC4, A2, AC2, A4, AC4, A2, AC2,
    /* 89 - 96 */ N8, NC4, N2, NC2, N4, NC4, N2, NC2,
//  /* 97- 104 */ W8, WC4, W2, WC2, W4,  0, W2,  0,   // ANTIC fetches some useless overscan data, but not all of it.
    /* 97- 104 */ W8, WC4, W2, WC2, W4, A8, W2, A2,   // ANTIC fetches some useless overscan data, but not all of it.
    /* 105-113 */ 0, 0, 0, 0, 0, 0, 0, 0, 0
};

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

    const ULONGLONG s5 = 29830 * 60 * 5;    // 5 seconds
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
    f = LoadStateAtari(iVM, Time[iVM][cTimeTravelPos[iVM]], candysize[iVM]);    // restore our snapshot, and create a new anchor point here

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

    BOOL f = SaveStateAtari(iVM, &pPersist, &cbPersist);    // get our current state

    if (!f || cbPersist != candysize[iVM])
        return FALSE;

    // initialize all our saved states to now, so that going back in time takes us back here.
    for (unsigned i = 0; i < 3; i++)
    {
        if (Time[iVM][i] == NULL)
            return FALSE;

        _fmemcpy(Time[iVM][i], pPersist, candysize[iVM]);

        ullTimeTravelTime[iVM] = GetCycles();    // time stamp it
    }

    return TRUE;
}

// push PC and P on the stack. Is this a BRK interrupt?
//
void Interrupt(int iVM, BOOL b)
{
    // the only time regB is used is to say if an IRQ was a BRK or not
    // is what is pushed on the stack at this time
    if (!b)
        regP &= ~0x10;
    else
        regPC++;    // did you know that BRK is a 2 byte instruction?

    cpuPokeB(iVM, regSP, regPC >> 8);  regSP = (regSP - 1) & 255 | 256;
    cpuPokeB(iVM, regSP, regPC & 255); regSP = (regSP - 1) & 255 | 256;
    cpuPokeB(iVM, regSP, regP);          regSP = (regSP - 1) & 255 | 256;

    if (b)
        regPC--;

    // B bit always reads as 1 by the processor
    regP |= 0x10;    // regB
    regP |= IBIT;    // interrupts always SEI
}

// What happens when it's scan line 241 and it's time to start the VBI
//
void DoVBI(int iVM)
{
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
                    pB = &rPADATA;    // joy 1 & 2
                else
                    pB = &rPBDATA;    // joy 3 & 4

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

    CheckKey(iVM);    // process the ATARI keyboard buffer

    if (fTrace)
        ForceRedraw(iVM);    // it might do this anyway

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

// Read in the cartridge. Are we initializing and using the default bank, or restoring a saved state to the last bank used?
//
BOOL ReadCart(int iVM, BOOL fDefaultBank)
{
    unsigned char *pch = (unsigned char *)rgvm[iVM].rgcart.szName;

    int h;
    int cb = 0, cb2;
    BYTE type = 0, *pb;

    bCartType = 0;

    if (!pch || !rgvm[iVM].rgcart.fCartIn)
    {
        return TRUE;    // nothing to do, all OK
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
            type = pb[7];    // the type of cartridge
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
    //{name: 'XEGS 32 KB cartridge', id : 12 },
    //{name: 'XEGS 64 KB cartridge', id : 13 },
    //{name: 'XEGS 128 KB cartridge', id : 14 },
    //{name: 'XEGS 256 KB cartridge', id : 23 },
    //{name: 'XEGS 512 KB cartridge', id : 24 },
    //{ name: 'XEGS 1024 KB cartridge', id : 25 },
    //{name: 'Bounty Bob Strikes Back 40 KB cartridge', id : 18 },
    //{ name: 'Atarimax 128 KB cartridge', id : 41 },
    //{name: 'Atarimax 1 MB Flash cartridge', id : 42 },

    // !!! I support these, but not the part where they can switch out to RAM yet

    //{ name: 'OSS two chip 16 KB cartridge (034M)', id : 3 },
    //{ name: 'OSS two chip 16 KB cartridge (043M)', id : 45 },
    //{ name: 'OSS one chip 16 KB cartridge', id : 15 },
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
    //{ name: 'MegaCart 128 KB cartridge', id : 29 },
    //{ name: 'SpartaDOS X 128 KB cartridge', id : 43 },
    //{ name: 'SIC! 128 KB cartridge', id : 54 },
    //{ name: 'MegaCart 256 KB cartridge', id : 30 },
    //{ name: 'SIC! 256 KB cartridge', id : 55 },
    //{ name: 'MegaCart 512 KB cartridge', id : 31 },
    //{ name: 'SIC! 512 KB cartridge', id : 56 },
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

    // unique 40K Bounty Bob cartridge - check for last bank being valid?
    else if (type == 18 || (type == 0 && cb == 40960))
    {
        bCartType = CART_BOB;    // unique Bounty Bob cart
    }

    // 128K and 1st bank is the main one - ATARIMAX
    else if (type == 41 || (type == 0 && cb == 131072 && *(pb + 8188) == 0 &&
        ((*(pb + 8191) >= 0x80 && *(pb + 8191) < 0xC0) || (*(pb + 8187) >= 0x80 && *(pb + 8187) < 0xC0))))
    {
        bCartType = CART_ATARIMAX1;

        // tell InitCart to use the default bank
        if (fDefaultBank)
            iSwapCart = 0;
    }

    // 1M and first bank is the main one - ATARIMAX127
    else if (type == 42 || (type == 0 && cb == 1048576 && *(pb + 8188) == 0 &&
        ((*(pb + 8191) >= 0x80 && *(pb + 8191) < 0xC0) || (*(pb + 8187) >= 0x80 && *(pb + 8187) < 0xC0))))
    {
        bCartType = CART_ATARIMAX8;

        // tell InitCart to use the default bank
        if (fDefaultBank)
            iSwapCart = 0;
    }

    // make sure the last bank is a valid ATARI 8-bit cartridge - assume XEGS
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
        rgvm[iVM].rgcart.fCartIn = FALSE;    // unload the cartridge
        rgvm[iVM].rgcart.szName[0] = 0; // erase the name
    }

    return bCartType;    // say if cart looks good or not
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
    // 8K main bank is the last one, and init the $8000 and $9000 banks to their bank 0's
    else if (bCartType == CART_BOB)
    {
        _fmemcpy(&rgbMem[0xA000], pb + cb - 8192, 8192);
        _fmemcpy(&rgbMem[0x8000], pb, 4096);
        _fmemcpy(&rgbMem[0x9000], pb + 16384, 4096);
        ramtop = 0x8000;
    }
    // 8K main bank is the first one (rumours are, in some old 1M carts, it's the last)
    else if (bCartType == CART_ATARIMAX1)
    {
        if (iSwapCart == 0x10)
            ramtop = 0xc000;    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * iSwapCart, 8192); // this bank is in right now
            ramtop = 0xa000;
        }
    }
    else if (bCartType == CART_ATARIMAX8)
    {
        if (iSwapCart == 0x80)
            ramtop = 0xc000;    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * iSwapCart, 8192); // this bank is in right now
            ramtop = 0xa000;
        }

    }
    // 8K main bank is the last one
    else if (bCartType == CART_XEGS)
    {
        _fmemcpy(&rgbMem[0xA000], pb + cb - 8192, 8192);
        // right cartridge present bit must float high if it is not RAM and not part of this bank
        // but if restoring a state, there may be a bank swapped in there, so don't trash memory.
        // It's probably not zero, or cold start would crash.
        if (rgbMem[0x9ffc] == 0)
            rgbMem[0x9ffc] = 0xff;
        ramtop = 0x8000;
    }

    return;
}

// Swap out cartridge banks
//
void BankCart(int iVM, BYTE iBank, BYTE value)
{
    PCART pcart = &(rgvm[iVM].rgcart);
    unsigned int cb = pcart->cbData;
    int i;
    BYTE *pb = rgbSwapCart[iVM];
    BYTE swap[8192];

    // we are not a banking cartridge
    if (!(rgvm[iVM].rgcart.fCartIn) || bCartType <= CART_16K)
        return;

    // banks are 0, 3, 4, main
    if (bCartType == CART_OSSA)
    {
        i = (iBank == 0 ? 0 : (iBank == 3 ? 1 : (iBank == 4 ? 2 : -1)));
        assert(i != -1);
        if (i != -1)
            _fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
    }

    // banks are 0, 4, 3, main
    if (bCartType == CART_OSSAX)
    {
        i = (iBank == 0 ? 0 : (iBank == 3 ? 2 : (iBank == 4 ? 1 : -1)));
        assert(i != -1);
        if (i != -1)
            _fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
    }

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
            i = -1;     //!!! swapping OSS A & B cartridge out to RAM not supported

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

    // Address is bank #, 8K that goes into $A000. Bank top + 1 is RAM
    else if (bCartType == CART_ATARIMAX1 || bCartType == CART_ATARIMAX8)
    {
        int mask;

        if (bCartType == CART_ATARIMAX1)
            mask = 0x0f;
        else
            mask = 0x7f;

        if (iBank == (mask + 1))
        {
            // want RAM - currently ROM - exchange with last bank used
            if (ramtop == 0xa000)
            {
                _fmemcpy(swap, &rgbMem[0xa000], 8192);
                _fmemcpy(&rgbMem[0xA000], pb + iSwapCart * 8192, 8192);
                _fmemcpy(pb + iSwapCart * 8192, swap, 8192);
            }
            ramtop = 0xc000;
            iSwapCart = iBank;
        }
        else if (iBank <= mask)
        {
            // want ROM, currently different ROM? First exchange with last bank used
            if (ramtop == 0xa000 && iBank != iSwapCart)
            {
                _fmemcpy(swap, &rgbMem[0xa000], 8192);
                _fmemcpy(&rgbMem[0xa000], pb + iSwapCart * 8192, 8192);
                _fmemcpy(pb + iSwapCart * 8192, swap, 8192);
            }
            // now exchange with current bank
            if (ramtop == 0xc000 || iBank != iSwapCart)
            {
                _fmemcpy(swap, &rgbMem[0xa000], 8192);
                _fmemcpy(&rgbMem[0xa000], pb + iBank * 8192, 8192);
                _fmemcpy(pb + iBank * 8192, swap, 8192);

                iSwapCart = iBank;    // what bank is in there now
                ramtop = 0xa000;
            }
        }
    }

    // 8k banks, given as contents, not the address
    else if (bCartType == CART_XEGS)
    {
        while (value >= cb >> 13)
            value -= (BYTE)(cb >> 13);

        if (value < (cb << 13))
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
    // Then set now how many cycles have to execute before reaching 0

    // STIMER will call us for all 4 voices in order, so the math will work out.

    if (irq == 0)
    {
        f[0] = AUDF1;
        c[0] = (AUDCTL & 0x40) ? 1789790 : ((AUDCTL & 0x01) ? 15700 : 63921);
        irqPokey[0] = (LONG)((ULONGLONG)f[0] * 1789790 / c[0]);
    }
    else if (irq == 1)
    {
        f[1] = (AUDCTL & 0x10) ? (AUDF2 << 8) + AUDF1 : AUDF2; // when joined, these count down much slower
        c[0] = (AUDCTL & 0x40) ? 1789790 : ((AUDCTL & 0x01) ? 15700 : 63921);
        c[1] = (AUDCTL & 0x10) ? c[0] : ((AUDCTL & 0x01) ? 15700 : 63921);
        irqPokey[1] = (LONG)((ULONGLONG)f[1] * 1789790 / c[1]);
    }
    else if (irq == 3)
    {
        f[3] = (AUDCTL & 0x08) ? (AUDF4 << 8) + AUDF3 : AUDF4;
        c[2] = (AUDCTL & 0x20) ? 1789790 : ((AUDCTL & 0x01) ? 15700 : 63921);    // irq 3 needs to know what this is
        c[3] = (AUDCTL & 0x08) ? c[2] : ((AUDCTL & 0x01) ? 15700 : 63921);
        irqPokey[3] = (LONG)((ULONGLONG)f[3] * 1789790 / c[3]);
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
        CreateDMATables();    // maps of where the electron beam is on each CPU cycle
        CreatePMGTable();     // priorities of PMG

        int j;

        // Initialize the poly counters
        // run through all the poly counters and remember the results in a table
        // so we can look them up and not have to do expensive calculations while emulating

        // poly5's cycle is 31
        poly5[0] = 0;
        int p = 0;
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
        candysize[iVM] += (2048 + 4096 + 10240);    // extended XL/XE RAM
    if (rgvm[iVM].bfHW == vmAtariXE)
        candysize[iVM] += (16384 * 4);            // extended XE RAM

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
    vrgcandy[iVM] = NULL;

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
    rgvm[iVM].ivdMac = sizeof(rgvm[0].rgvd) / sizeof(VD);    // we only have 8, others have 9

    // by default, use XL and XE built in BASIC, but no BASIC for Atari 800.
    // unless Shift-F10 changes it
    ramtop = (rgvm[iVM].bfHW > vmAtari48) ? 0xA000 : 0xC000;
    wStartScan = STARTSCAN;    // this is when ANTIC starts fetching. Usually 3x "blank 8" means the screen starts at 32.

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

    // load the cartridge data
    fCartNeedsSwap = FALSE; // forget that the old cart wasn't swapped in properly so we don't mess up the new cart
    return ReadCart(iVM, TRUE);
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
        rgbMem[0x9ffc] = 1;    // no R cart
        rgbMem[0xbffc] = 1;    // no L cart
        rgbMem[0xbffd] = 0;    // no special R cartridge
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
    UnmountAtariDisk(iVM, i);    // make sure all emulator types know to do this first

    PVD pvd = &rgvm[iVM].rgvd[i];

    if (pvd->dt == DISK_IMAGE)
        return AddDrive(iVM, i, (BYTE *)pvd->sz);
    if (pvd->dt == DISK_NONE)
        return TRUE;

    // I don't recognize this kind of disk... yet
    return FALSE;
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
    // tell the CPU which (done in Execute)
    //cpuInit(PeekBAtari, PokeBAtari);

    //ODS("\n\nWARM START\n\n");
    NMIST = 0x20 | 0x1F;
    regPC = cpuPeekW(iVM, (mdXLXE != md800) ? 0xFFFC : 0xFFFA);
    cntTick = 255;    // delay for banner messages

    fBrakes = TRUE;    // go back to real time
    clockMult = 1;    // NYI
    wFrame = 0;
    wScan = 0;    // start at top of screen again
    wLeft = 0;
    PSL = 0;

    // SIO init
    cSEROUT = 0;
    fSERIN = FALSE;
    isectorPos = 0;
    fWant8 = 0;
    fWant10 = 0;

    InitJoysticks();    // let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
    CaptureJoysticks();

    return TRUE;
}

// Cold Start the machine - the first one is currently done when it first becomes the active instance
//
BOOL __cdecl ColdbootAtari(int iVM)
{
    unsigned addr;
    //ODS("\n\nCOLD START\n\n");

    // first do the warm start stuff
    if (!WarmbootAtari(iVM))
        return FALSE;

    BOOL f = InitAtariDisks(iVM);

    if (!f)
        return FALSE;

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

    // load the proper initial bank back into the cartridge, for cartridges with a RAM bank, make sure it isn't in RAM mode
    // or it will just erase everything and go to memo pad.
    iSwapCart = 0;
    InitCart(iVM);

    // reset the registers
    cpuReset(iVM);

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

    // !!! why?
    // initialize hardware
    // NOTE: at $D5FE is ramtop and then the jump table

    for (addr = 0xD000; addr < 0xD5FE; addr++)
    {
        cpuPokeB(iVM, addr,0xFF);
    }

    NMIST = 0x20 | 0x1F;    // the one in WARMSTART might get undone from the above clearing?

    // CTIA/GTIA reset

    // we want to remove BASIC, on an XL the only way is to fake OPTION being held down.
    // but now it will get stuck down! ExecuteAtari will fix that
    CONSOL = ((mdXLXE != md800) && (GetKeyState(VK_F9) < 0 || ramtop == 0xC000)) ? 3 : 7;
    PAL = 15;   // should be 14, but some apps need to see 15
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

    rPADATA = 255;    // the data PORTA will provide if it is in READ mode
    rPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000) ? 253 : 255) : 255; // XL: HELP out, BASIC = ??, OS IN
    PORTA = rPADATA; // since they default to read mode
    PORTB = rPBDATA;

    wPADATA = 0;    // the data written to PORTA to be provided in WRITE mode (must default to 0 for Caverns of Mars)
    wPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000) ? 253 : 255) : 0; // XL needs to default to OS IN and HELP OUT

    wPADDIR = 0;    // direction - default to read
    wPBDDIR = 0;

    PACTL  = 0x3c;    // I/O mode, not SET DIR mode
    PBCTL  = 0x3c;    // I/O mode, not SET DIR mode

    // seed the random number generator again. The real ATARI is probably seeded by the orientation
    // of sector 1 on the floppy. I wonder how cartridges are unpredictable?
    // don't allow the seed that gets stuck
    do {
        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        random17pos = qpc.QuadPart & 0x1ffff;
    } while (random17pos == 0x1ffff);

    //too slow to do anytime but app startup
    //InitSound();    // Need to reset and queue audio buffers

    // Our two paths to creating a VM, cold start or LoadState, both need to reset time travel to create an anchor point
    return TimeTravelReset(iVM);
}

// SAVE: return a pointer to our data, and how big it is. It will not be harmed, and used before we are Uninit'ed, so don't
// waste time copying anything, just return the right pointer.
// The pointer might be NULL if they just want the size
//
BOOL __cdecl SaveStateAtari(int iVM, char **ppPersist, int *pcbPersist)
{
    // there are some time travel pointers in the structure, we need to be smart enough not to use them
    if (ppPersist)
    {
        // before saving, we need to put the RAM that's swapped into cartridge memory back into RAM so it will be persisted
        // keep iSwapCart to tell us which bank should be swapped back in, so don't do this twice in a row!
        // TimeTravel periodic save state could do this slow memory swap (and swap back) but it's a rare case when
        // cartridges are first executing, they usually run in RAM mode
        if (rgvm[iVM].rgcart.fCartIn && (bCartType == CART_ATARIMAX1 || bCartType == CART_ATARIMAX8) && ramtop == 0xa000 && !fCartNeedsSwap)
        {
            BYTE swap[8192];
            _fmemcpy(swap, &rgbMem[0xa000], 8192);
            _fmemcpy(&rgbMem[0xA000], rgbSwapCart[iVM] + iSwapCart * 8192, 8192);
            _fmemcpy(rgbSwapCart[iVM] + iSwapCart * 8192, swap, 8192);
            ramtop = 0xc000;
            fCartNeedsSwap = TRUE;  // next Execute, swap it back (assumes caller is done with it by then)
        }
        
        *ppPersist = (char *)vrgcandy[iVM];
    }
    
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
    // if we were in the middle of reading a sector through SIO, restore that data
    SIOReadSector(iVM);

    if (!f)
        return f;

    // If our saved state had a cartridge, load it back in
    ReadCart(iVM, FALSE);
    InitCart(iVM);

    // too slow to do anytime but app startup
    //InitSound();    // Need to reset and queue audio buffers

    InitJoysticks(); // let somebody hot plug a joystick in and it will work the next warm/cold start of any instance
    CaptureJoysticks();

    // Our two paths to creating a VM, cold start or LoadState, both need to reset time travel to create an anchor point
    f = TimeTravelReset(iVM); // state is now a valid anchor point

    return f;
}


BOOL __cdecl DumpHWAtari(int iVM)
{
    iVM;
#ifndef NDEBUG
//    RestoreVideo();

    // tell CPU which HW is running
    cpuInit(PeekBAtari, PokeBAtari);

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
// !!! fStep and fCont are ignored for now! Not needed, GEM uses a different tracing system now
BOOL __cdecl ExecuteAtari(int iVM, BOOL fStep, BOOL fCont)
 {
    fCont; fStep;

    // tell the 6502 which HW it is running this time (will be necessary when we support multiple 6502 platforms)
    cpuInit(PeekBAtari, PokeBAtari);

    fStop = 0;    // do not break out of big loop

    // to persist, we had to temporarily put the RAM saved in the cartridge space into memory that is persisted. Put it back.
    if (fCartNeedsSwap && ramtop == 0xc000)
    {
        BYTE swap[8192];
        _fmemcpy(swap, &rgbMem[0xa000], 8192);
        _fmemcpy(&rgbMem[0xa000], rgbSwapCart[iVM] + iSwapCart * 8192, 8192);
        _fmemcpy(rgbSwapCart[iVM] + iSwapCart * 8192, swap, 8192);
        ramtop = 0xa000;
        fCartNeedsSwap = FALSE;
    }

    do {

        // When tracing, we only do 1 instruction per call, so there might be some left over - go straight to doing more instructions.
        // Since we have to finish a 6502 instruction, we might do too many cycles and end up < 0. Don't let errors propagate.
        // Add wLeft back in to the next round.
        if (wLeft <= 0)
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

            // IRQ's, like key presses, SIO or POKEY timers. VBI and DLIST interrupts are NMI's.
            // !!! Only scan line granularity.
            // !!! Serialize when multiple are triggered at once?
            if (!(regP & IBIT))
            {
                if (IRQST != 255)
                {
                    Interrupt(iVM, FALSE);
                    regPC = cpuPeekW(iVM, 0xFFFE);
                    //ODS("IRQ %02x TIME! %d %d\n", (BYTE)~IRQST, wFrame, wScan);

                    // check if the interrupt vector is our breakpoint, otherwise we would never notice and not hit it
                    if (regPC == bp)
                        fHitBP = TRUE;

                    // after $8 (SEROUT FINISHED) for an entire frame comes SERIN, unless we're already transmitting
                    if (!(IRQST & 0x08) && !fSERIN)
                    {
                        if (cSEROUT == 5)
                        {
                            cSEROUT = 0;
                            isectorPos = 0;
                            if (rgSIO[0] == 0x31 && rgSIO[1] == 0x52)
                            {
                                //ODS("DISK READ REQUEST sector %d\n", rgSIO[2] | ((int)rgSIO[3] << 8));
                                bSERIN = 0x41;    // start with ack
                                fSERIN = (wScan + SIO_DELAY);    // waiting less than this hangs apps who aren't ready for the data
                                if (fSERIN >= NTSCY)
                                    fSERIN -= (NTSCY - 1);    // never be 0, that means stop
                            }
                            else if (rgSIO[0] == 0x31 && rgSIO[1] == 0x53)
                            {
                                //ODS("DISK STATUS %02x %02x\n", rgSIO[2], rgSIO[3]);
                                bSERIN = 0x41;    // start with ack
                                fSERIN = (wScan + SIO_DELAY);    // waiting less than this hangs apps who aren't ready for the data
                                if (fSERIN >= NTSCY)
                                    fSERIN -= (NTSCY - 1);    // never be 0, that means stop
                            }
                            else if (rgSIO[0] == 0x31)
                            {
                                ODS("UNSUPPORTED SIO command %02x\n", rgSIO[1]);
                            }
                        }
                    }
                }
            }

            // Prepare ProcessScanLine for a new scan line, and figure out everything we need to know to understand
            // how ANTIC will steal cycles on a scan line like this
            PSLPrepare(iVM);

            // table element HCLOCKS has the starting value of wLeft for this kind of scan line (0-based)
            // subtract any extra cycles we spent last time finishing an instruction
            wLeft += (DMAMAP[HCLOCKS] + 1);

            
            // We delayed a POKE to something that needed to wait until the next scan line, so add the 4 cycles
            // back that we would have lost. We know it's safe to make wLeft bigger than 114 because it will get back to being 
            // <114 as soon as the POKE happens.
            if (fRedoPoke)
            {
                wLeft += 4;
                fRedoPoke = FALSE;
            }

            // Scan line 0-7 are not visible
            // Scan lines 8-247 are 240 visible lines, ANTIC DMA is possible, depending on a lot of things
            // Scan line 248 is the VBLANK
            // Scan lines 249-261 are the rest of the overscan lines

            if (wScan == STARTSCAN + Y8)
            {
                // Joysticks and Keys are looked at just before the VBI, because many games process them there
                DoVBI(iVM);
            }

            // We're supposed to start executing at the WSYNC release point (cycle 105) not the beginning of the scan line
            // That point is stored in index 115 of this array, and + 1 because the index is 0-based
            if (WSYNC_Waiting)
            {
                //ODS("WAITING\n");
                
                // + 1 to make it 1-based
                // +1 is to start early at cycle 104 !!! not always true, but usually?
                wLeft = DMAMAP[115] + 1 +1;
                
                WSYNC_Waiting = FALSE;

                // Uh oh, an NMI should happen in the mean time, so better do that now and delay the wait for WSYNC until the RTI
                // and the main code is executing again

                if (wScan == STARTSCAN + Y8)
                {
                    // clear DLI, set VBI, leave RST alone - even if we're not taking the interrupt
                    NMIST = (NMIST & 0x20) | 0x5F;

                    // VBI enabled, generate VBI by setting PC to VBI routine. When it's done we'll go back to what we were doing before.
                    if (NMIEN & 0x40) {
                        //ODS("special VBI\n");
                        Interrupt(iVM, FALSE);
                        regPC = cpuPeekW(iVM, 0xFFFA);

                        if (regPC == bp)
                            fHitBP = TRUE;
                    }

                    // !!! We let the VBI not start until the WSYNC point because it's so long anyway
                }
                else if ((sl.modehi & 8) && (iscan == scans || (fWait & 0x08)))
                {
                    // set DLI, clear VBI leave RST alone - even if we don't take the interrupt
                    NMIST = (NMIST & 0x20) | 0x9F;
                    if (NMIEN & 0x80)    // DLI enabled
                    {
                        //ODS("special DLI\n");
                        Interrupt(iVM, FALSE);
                        regPC = cpuPeekW(iVM, 0xFFFA);

                        // the main code may be waiting for a WSYNC, but in the meantime this DLI should NOT.
                        // It should start at the regular DLI start point. On RTI note to resume waiting for WSYNC
                        // !!! This won't work for nested interrupts
                        // !!! This doesn't work, whether or not the DLI goes past the WSYNC point. (Most do a STA WSYNC)
                        wLeft = DMAMAP[116] + 1;
                        WSYNC_Waiting = FALSE;
                        WSYNC_on_RTI = TRUE;

                        if (regPC == bp)
                            fHitBP = TRUE;
                    }
                }
            }

        } // if wLeft == 0

        // Normally, executes one horizontal scan line's worth of 6502 code, drawing bits and pieces of it as it goes
        // But tracing or breakpoints can affect that
        // if we just hit a breakpoint at the IRQ vector above, then don't do anything
        if (!fHitBP)
        {
            // If we'll need a DLI or a VBI at cycle 10, tell it the value of wLeft that needs the DLI (+ 1 since it's 0-based)
            wNMI = (((sl.modehi & 8) && (iscan == scans || (fWait & 0x08))) || (wScan == STARTSCAN + Y8)) ? DMAMAP[116] + 1 : 0;
            Go6502(iVM);
        }

        // hit a breakpoint during execution
        if (regPC == bp)
            fStop = TRUE;

        assert(wLeft <= 0 || fTrace == 1 || regPC == bp);

        // we finished the scan line
        if (wLeft <= 0)
        {
            // now finish off any part of the scan line that didn't get drawn during execution
            ProcessScanLine(iVM);

            wScan = wScan + 1;

            // we want the $10 IRQ. Make sure it's enabled, and we waited long enough (dont' decrement past 0)
            if ((IRQEN & 0x10) && fWant10 && (--fWant10 == 0))
            {
                //ODS("TRIGGER - SEROUT NEEDED\n");
                IRQST &= ~0x10;    // it might not be enabled yet
            }

            // the $8 IRQ is needed.
            // !!! apps don't seem to actually be ready for it unless they specificaly enable only this and no other low nibble ones
            // (hardb, Eidolon V2, 221B)
            if ((IRQEN & 0x08) && !(IRQEN & 0x07) && fWant8 && (--fWant8 == 0))
            {
                //ODS("TRIGGER - SEROUT COMPLETE\n");
                IRQST &= ~0x08;    // it might not be enabled yet
                fSERIN = 0;    // abandon any pending transfer from before
            }

            // SIO - periodically send the next data byte, too fast breaks them
            if (fSERIN == wScan && (IRQEN & 0x20))
            {
                fSERIN = (wScan + SIO_DELAY);    // waiting less than this hangs apps who aren't ready for the data (19,200 baud expected)
                if (fSERIN >= NTSCY)
                    fSERIN -= (NTSCY - 1);    // never be 0, that means stop
                //ODS("TRIGGER SERIN\n");
                IRQST &= ~0x20;

#if 0   // this doesn't help any known app, and kind of breaks hardb
                // now pretend 7 jiffies elapsed for apps that time disk sector reads
                BYTE jif = 7;
                // reading the same sector as last time takes twice as long
                short wSector = rgSIO[2] | (short)rgSIO[3] << 8;
                if (wSector == wLastSIOSector)
                    jif = 14;
                BYTE oldjif = rgbMem[20];
                rgbMem[20] = oldjif + jif;
                if (oldjif >= (256 - jif))
                {
                    oldjif = rgbMem[19];
                    rgbMem[19]++;
                    if (oldjif == 255)
                        rgbMem[18]++;
                }
                wLastSIOSector = wSector;
#endif
            }

            // POKEY timers
            // !!! I'm worried about perf if I do this every instruction, so cheat for now and check only every scan line
            // if the counters reach 0, trigger an IRQ. They auto-repeat and auto-fetch new AUDFx values
            // 0 means timer not being used

            for (int irq = 0; irq < 4; irq = ((irq + 1) << 1) - 1)    // 0, 1, 3 (i.e. 1, 2, 4 - timer 3 not supported by the 800)
            {
                if (irqPokey[irq] && irqPokey[irq] <= HCLOCKS)
                {
                    if (IRQEN & (irq + 1))
                    {
                        //ODS("TIMER %d FIRING\n", irq + 1);
                        IRQST &= ~(irq + 1);                // fire away
                    }

                    LONG isav = irqPokey[irq];                // remember

                    ResetPokeyTimer(iVM, irq);            // start it up again, they keep cycling forever

                    if (irqPokey[irq] > HCLOCKS - isav)
                    {
                        irqPokey[irq] -= (HCLOCKS - isav);    // don't let errors propagate
                    }
                    else
                        irqPokey[irq] = 1;                    // uh oh, already time for the next one
                    //ODS("TIMER %d ADJUSTED to %d\n", irq + 1, irqPokey[irq]);
                }
                else if (irqPokey[irq] > HCLOCKS)
                    irqPokey[irq] -= HCLOCKS;
            }

        }

        // we process the audio after the whole frame is done
        if (wScan >= NTSCY)
        {
            TimeTravelPrepare(iVM);        // periodically call this to enable TimeTravel

            SoundDoneCallback(iVM, vi.rgwhdr, SAMPLES_PER_VOICE);    // finish this buffer and send it

            wScan = 0;
            wFrame++;    // count how many frames we've drawn. Does anybody care?

            // exit each frame to let the next VM run (if Tiling) and to update the clock speed on the status bar (always)
            fStop = TRUE;
        }

    } while (!fTrace && !fStop);

    fHitBP = FALSE;
    return (regPC != bp);    // fail if we hit a bp so the monitor will kick in
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


// only called for addr >= ramtop
//
__declspec(noinline)
BYTE __cdecl PeekBAtari(int iVM, ADDR addr)
{
    // This is how Bounty Bob bank selects
    if (bCartType == CART_BOB)
    {
        if (addr >= 0x8ff6 && addr <= 0x8ff9)
            BankCart(iVM, 0, (BYTE)(addr - 0x8ff6));
        if (addr >= 0x9ff6 && addr <= 0x9ff9)
            BankCart(iVM, 1, (BYTE)(addr - 0x9ff6));
    }

    // implement SHADOW or MIRROR registers instantly yet quickly without any memcpy!

    switch ((addr >> 8) & 255)
    {
    case 0xd0:
        addr &= 0xff1f;    // GTIA has 32 registers
        break;
    case 0xd2:
        addr &= 0xff0f;    // POKEY has 16 registers
        break;
    case 0xd3:
        addr &= 0xff03;    // PIA has 4 registers
        break;
    case 0xd4:
        addr &= 0xff0f;    // ANTIC has 16 registers also shadowed to $D5xx
        break;
    case 0xd5:
        BankCart(iVM, addr & 0xff, 0);    // cartridge banking
        break;
    }

    // RANDOM and its shadows
    // we've been asked for a random number. How many times would the poly counter have advanced? (same clock freq as CPU)
    if (addr == 0xD20A) {
        int cur = (wFrame * NTSCY * HCLOCKS + wScan * HCLOCKS + DMAMAP[wLeft - 1]);
        int delta = (int)(cur - random17last);
        random17last = cur;
        random17pos = (random17pos + delta) % 0x1ffff;
        rgbMem[addr] = poly17[random17pos];
    }

    // VCOUNT - by clock 111 VCOUNT increments
    // DMAMAP[115] + 1 is the WSYNC point (cycle 105). VCOUNT increments 6 cycles later. LDA VCOUNT is a 4 cycle instruction.
    else if (addr == 0xD40B)
    {
        // if the last cycle of this 4-cycle instruction ends AT the point where VCOUNT is incremented (111), it still sees the old value
        // - 1 for 0-based. - 3 for the last cycle. + 1 to see what the next cycle is (which might be blocked, so the next instruction
        // could start arbitrarily further than the end of the last instruction).
        if (wLeft >= 4 && DMAMAP[wLeft - 1 - 3] + 1 <= 111)
            return (BYTE)(wScan >> 1);
        // report scan line 262 (131) for only 1 cycle, then start reporting 0 again
        else if (wScan == 261 && wLeft <= 5)
            return 0;
        else
            return (BYTE)((wScan + 1) >> 1);
    }

    else if (addr == 0xd20d)
    {
        if (!fSERIN)
        {
            ODS("UNEXPECTED SERIN\n");
            return 0;
        }

        IRQST |= 0x20;    // don't do another IRQ yet, we have to emulate a slow BAUD rate (VBI will turn back on)

        BYTE rv = bSERIN;

        // remember, the data in the sector can be the same as the ack, complete or checksum
        
        // status responses are 4 bytes long, sector reads are 128
        BYTE iLastSector = (rgSIO[1] == 0x52) ? 128 : 4;
        
        if (isectorPos > 0 && isectorPos < iLastSector)
        {
            //ODS("DATA 0x%02x = 0x%02x\n", isectorPos - 1, bSERIN);
            bSERIN = sectorSIO[iVM][isectorPos];
            isectorPos++;
        }
        else if (isectorPos == iLastSector)
        {
            //ODS("DATA 0x%02x = 0x%02x\n", isectorPos - 1, bSERIN);
            bSERIN = checksum;
            isectorPos++;
        }
        else if (isectorPos > iLastSector)
        {
            //ODS("CHECKSUM = 0x%02x\n", bSERIN);
            fSERIN = FALSE;    // all done
            isectorPos = 0;
        }
        else if (bSERIN == 0x41)    // ack
        {
            bSERIN = 0x43;    // complete
            //ODS("ACK\n");
        }
        else if (bSERIN == 0x43)
        {
            //ODS("COMPLETE\n");
            if (rgSIO[1] == 0x52)
            {
                checksum = SIOReadSector(iVM);
            }
            // this is how I think you properly respond to a status request, I hope
            else if (rgSIO[1] == 0x53)
            {
                checksum = 0xe0;
                sectorSIO[iVM][0] = 0x0;
                sectorSIO[iVM][1] = 0xff;
                sectorSIO[iVM][2] = 0xe0;
                sectorSIO[iVM][3] = 0x00;
            }
            bSERIN = sectorSIO[iVM][0];
            isectorPos = 1;    // next byte will be this one
        }
        return rv;
    }

    return cpuPeekB(iVM, addr);

}

// currently not used
//
WORD __cdecl PeekWAtari(int iVM, ADDR addr)
{
    assert(addr >= ramtop);
    assert(FALSE); // I'm curious when this happens

    return PeekBAtari(iVM, addr) | (PeekBAtari(iVM, addr + 1) << 8);
}

ULONG __cdecl PeekLAtari(int iVM, ADDR addr)
{
    // not used

    return cpuPeekW(iVM, addr);
}

// not used
BOOL __cdecl PokeLAtari(int iVM, ADDR addr, ULONG w)
{
    iVM; addr; w;
    return TRUE;
}

// currently not used
//
BOOL __cdecl PokeWAtari(int iVM, ADDR addr, WORD w)
{
    assert(addr >= ramtop);
    assert(FALSE);    // I'm curious if this ever happens

    PokeBAtari(iVM, addr, w & 255);
    PokeBAtari(iVM, addr + 1, w >> 8);

    return TRUE;
}

// Be efficient! This is one of the most executed functions!
// Only call this is addr >= ramtop
// regPC is probably one too high at this moment
//
__declspec(noinline)
BOOL __cdecl PokeBAtari(int iVM, ADDR addr, BYTE b)
{
    BYTE bOld;
    assert(addr < 65536);

#if 0
    printf("write: addr:%04X, b:%02X, PC:%04X\n", addr, b & 255, regPC - 1); // PC is one too high when we're called
#endif

    // only the monitor uses this, but it seems a safe idea to spend the time to test this
    if (addr < ramtop)
    {
        rgbMem[addr] = b;
        return TRUE;
    }

    // This is how Bounty Bob bank selects
    if (bCartType == CART_BOB)
    {
        if (addr >= 0x8ff6 && addr <= 0x8ff9)
            BankCart(iVM, 0, (BYTE)(addr - 0x8ff6));
        if (addr >= 0x9ff6 && addr <= 0x9ff9)
            BankCart(iVM, 1, (BYTE)(addr - 0x9ff6));
    }

    switch ((addr >> 8) & 255)
    {
    default:

        // above RAMTOP (but not into a special register) does nothing on an 800
        if (mdXLXE == md800)
            break;

        // writing to XL/XE memory !!! what is this?

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

                memcpy(&rgbMem[0xD800], (unsigned char const *)rgbXLXED800, sizeof(rgbSIOR));
            }
        }

        // the OS is swapped out so allow writes to those locations
        if (!(wPBDATA & 1) && (addr >= 0xC000))
        {
            // write to XL/XE RAM under ROM

            if ((addr < 0xD000) || (addr >= 0xD800))
                cpuPokeB(iVM, addr, b);

            break;
        }

        break;

    case 0xD0:      // GTIA

        addr &= 31;

        // CYCLE COUNTING - Writes to GTIA should take effect IMMEDIATELY so process the scan line up to where the electron beam is right now
        // before we change the values. The next time its called will be with the new values.
        if (addr < 0x1d)
        {
            //ODS("GTIA %04x=%02x at VCOUNT=%02x clock=%02x\n", addr, b, PeekBAtari(iVM, 0xd40b), DMAMAP[wLeft - 1]);
            ProcessScanLine(iVM);    // !!! should anything else instantly affect the screen?
        }

        bOld = rgbMem[writeGTIA+addr];
        rgbMem[writeGTIA+addr] = b;

        if (addr == 0x1d)   // GRACTL
        {
            if (!(b & 1))   // turn off missiles needs to clear GRAFM (Decathlon blue bar glitch)
                GRAFM = 0;
            if (!(b & 2))   // turn off players needs to clear GRAFPx
                GRAFPX = 0;
        }
        else if (addr == 30)
        {
            // HITCLR

            *(ULONG *)MXPF = 0;
            *(ULONG *)MXPL = 0;
            *(ULONG *)PXPF = 0;
            *(ULONG *)PXPL = 0;

            pmg.fHitclr = 1;
            //ODS("HITCLR! - %04x %04x %02x\n", wFrame, wScan, wLeft);
        }
        else if (addr == 31)
        {
            // !!! REVIEW: click the speaker

            // printf("CONSOL %02X %02X\n", bOld, b);

                if ((bOld ^ b) & 8)
                {
                // toggle speaker state (mask out lowest bit?)
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
            if (PBCTL == 0x34)
            {
                assert(cSEROUT < 5);
                //ODS("SEROUT #%d = 0x%02x %d %d %d\n", cSEROUT, b, wFrame, wScan, wLeft);
                rgSIO[cSEROUT] = b;
                cSEROUT++;

                // they'll need a scan line at least to prepare for the interrupt
                fWant10 = 2;

                // and one extra scan line to prepare for this interrupt
                fWant8 = 3;
            }
        }
        else if (addr == 14)
        {
            // IRQEN - IRQST is the complement of the enabled IRQ's, and says which interrupts are active

            IRQST |= ~b; // all the bits they poked OFF (to disable an INT) have to show up here as ON
            //ODS("IRQEN: 0x%02x %d %d\n", b, wFrame, wScan);

            // !!! All of the bits they poke ON have to show up instantly OFF in IRQST, but that's not how I do it right now.
            // I don't reset the bit until the interrupt is ready to fire, I treat it as meaning it's triggered not just enabled.

        }
        else if (addr <= 8)
        {
            // AUDFx, AUDCx or AUDCTL have changed - write some sound
            // we're (wScan / 262) of the way through the scan lines and the DMA map tells us our horiz. clock cycle
            int iCurSample = (wScan * 100 + DMAMAP[wLeft - 1] * 100 / HCLOCKS) * SAMPLES_PER_VOICE / 100 / NTSCY;
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

        // Uh-oh, we're changing something with scan line granularity that shouldn't happen until after the next scan line is set up
        // since the STA abs won't have finished by the end of this line (4 cycle instruction) and we execute at the beginning of the
        // instruction instead of the end like we're supposed to. Skip doing it now and back up to do it again.
        // Without this Bump Pong is 2x too tall
        if (addr < 10 && rgbMem[regPC - 1] == 0xd4 && wLeft < 4)
        {
            regPC -= 3; // !!! Support indirect and 2-byte instructions that access ANTIC too
            fRedoPoke = TRUE;   // add 4 cycles to the next scan line so the timing isn't changed
            break;
        }

        rgbMem[writeANTIC+addr] = b;

        // the display list pointer is the only ANTIC register that is R/W
        if (addr == 2 || addr == 3)
            rgbMem[0xd400 + addr] = b;

        if (addr == 14 && b != 0)
            addr = addr;
        
        // Do NOT reload the DL to the top when DMA fetch is turned on, that turned out to be disastrous

        if (addr == 10)
        {
            // WSYNC
            // !!! I assume STA WSYNC. INC WSYNC is 6 cycles, and it triggers the WSYNC on cycle 5, not 4.
            //
            // I wonder if the beam is 5 cycles behind ANTIC. It fetchs the first NARROW non-character byte at 28.
            // It fetches the first character at 26, and the first char lookup at 29. Subsequent lines only do the
            // char lookup starting at 29. So it doesn't have the data it needs to draw at 25 until 30. 5 behind.
            // Also, ANTIC stop fetching at 106. 5 before that is 101, the end of the visibile screen. Coincidence?
            // 5 cycles is 20 pixels.
            //
            // Worm War and the HARDB Chess board are two of the most cycle precise apps I know of.
            // Worm War changes HPOSP3 while it's being drawn. I implement a lag of 6 pixels in ProcessScanLine
            // so that the change happens on a PMG pixel boundary, and that fixes WW for now. Even Altirra shows
            // corruption in the level select text at top that I do not! (At least last I looked).
            // HARDB/CHESS changes HPOSP3 after it's completed drawing in the wrong place! If I lag 6 pixels, this also
            // is corrected.
            // I thought that the solution was to finish every PMG that begins in the range being drawn (drawing with a lag of
            // 20 pixels) so that changes mid-draw will not take effect yet, but that seemed to mess up WW pretty badly. So
            // I do not understand the GTIA lag and when to make a change take effect yet.
            //
            // The snow near the beginning of HARDB is a great cycle precision test, but doesn't use WSYNC/VCOUNT.
            //
            // Decathlon's line 41 DLI spends enough time before its STA WSYNC that it needs to miss the line 41 WSYNC point
            //
            // Pitfall 2 scan line $6d needs to start early (104) on line $6c, and it writes to WSYNC again barely in time
            // to make the deadline.
            //
            // -1 to make it 0 based. Look at last cycle of this 4 cycle instruction (3 fewer). One cycle later is where the
            // store will actually be complete. (If the cycle after it finished is blocked, the next instruction won't start
            // for a while, so where the next instruction starts is irrelevant). The WSYNC is in time if it's complete
            // by the start of 104 (not being complete until the start of 105, the WSYNC point itself, it too late).
            if (wLeft >= 4 && DMAMAP[wLeft - 1 - 3] + 1 <= 104)
                // + 1 to make it 1-based
                // assume we're doing STA abs which is 4 cycles about to be decremented from us when we return
                // + 1 is to start early at cycle 104 !!! not always true, but usually?
                wLeft = DMAMAP[115] + 1 + 4 + 1;
            else
            {
                wLeft = 0;               // stop right now and wait till next line's WSYNC
                WSYNC_Waiting = TRUE;    // next scan line only gets a few cycles
            }
        }
        else if (addr == 15)
        {
            // NMIRES - reset NMI status

            NMIST = 0x1F;
        }
        break;

    case 0xD5:      // Cartridge bank select
//        printf("addr = %04X, b = %02X\n", addr, b);
        BankCart(iVM, addr & 0xff, b);
        break;
    }

    return TRUE;
}

#endif // XFORMER
