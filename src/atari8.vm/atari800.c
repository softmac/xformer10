
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
    // ATR must come first to be the default extension when creating a new disk image
    "Xformer/SIO2PC Disks\0*.atr;*.xfd;*.sd;*.dd;*.xex;*.exe;*.com;*.bas\0All Files\0*.*\0\0",
    "Xformer Cartridge\0*.bin;*.rom;*.car;*.x28;*.x64;*.x32\0All Files\0*.*\0\0",

    InstallAtari,
    UnInstallAtari,
    InitAtari,
    UninitAtari,
    InitAtariDisks,
    WriteProtectAtariDisk,
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
    
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    SaveStateAtari,
    LoadStateAtari
    };

#ifndef NDEBUG

/*static */ __inline void _800_assert(int f, char *file, int line, void *candy)
{
    char sz[99];

    if (!f)
    {
        int ret;

        wsprintf(sz, "%s: line #%d", file, line);
        ret = MessageBox(GetFocus(), sz, "Assert failed", MB_APPLMODAL | MB_ABORTRETRYIGNORE);

        if (ret == 3)
        {
            // ABORT - break into the debugger! (if we can)
            if (v.iVM >= 0 && candy == rgpvmi(v.iVM)->pPrivate)
            {
                bp = regPC; // makes Exec fail so the debugger knows which VM to use !!! won't work if we're not in Exec
                vi.fWantDebugBreak = TRUE; // tell the main thread to break into debugger. We can't do it from a VM thread which we might be.
                PostMessage(vi.hWnd, WM_COMMAND, IDM_DEBUGGER, 0);  // Send will hang, we're a VM thread and the window thread is blocked on us
                                                                    // so it's not wise to block on them.
            }
            else
                ret = 4;    // we'll crash, since the tile that asserted is not in focus, so let's retry instead to break into VS
        }

        if (ret == 4)
        {
            __debugbreak();
        }
    }
}

#endif

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

// These map describes what ANTIC is doing (why it might steal the cycle) for every cycle as a horizontal scan line is being drawn
// 0 means ANTIC never steals the cycle, but somebody else might (RAM refresh)
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
// of a hi-res playfield, so 40 PMG pixels is like 80 pixels which means it should be @ 20 CPU cycles not 13. Which means PMG HPOS = 0
// is at cycle -7, or 28 pixels before CPU cycle 0. 28 pixels on either side would add 56 pixels to the 456 pixel scan line,
// which is, surprise, surprise, 512. Haven't you always wondered about that?
//
// Let's say you do STA WSYNC STA COLPF0. The STA wakes up at the earliest at cycle 114 115 116 117. GTIA is 5 cycles behind ANTIC
// so the new colour takes effect at the beginning of cycle 113. But cycles 111 -114 are the invisible overscan pixels, so 
// you won't see the new colour, as desired. Therefore ATARI even accounts for more overscan than I'm showing where only 2 cycles are
// offscreen instead of the 4 that I make offscreen. I'm supposed to do collisions in this extra overscan area too, but I don't

//           < WIDE ><NORMAL><NARROW><NORMAL>< WIDE > 
//  0  3     9   13  17      25  ... 89      97  101 105   111.114 (CYCLE)
//  -  -     -   0   16          ...         336 352 -     -  .   (MY VISIBLE PIXEL # - 0 starts at clock 13, 352 ends at cycle 101)
//  14 20    32  40  48      64      192     208 216 224   236.242 (PMG HPOS - extends 14 more on either side would make 512 pixels
//                   ^  VISIBLE AREA (MY OVERSCAN)   ^

// char mode map - 1st scan line mode grab starts at 10. char data itself is on every line of that mode and starts at 13
const BYTE rgDMAC[HCLOCKS] =
{
    /*  0 - 8  */ DMA_M, DMA_DL, DMA_P, DMA_P, DMA_P, DMA_P, DMA_LMS, DMA_LMS, 0,
    /*  9 - 16 */ 0,  W4, 0,  W2, WC4, W4, WC2, W2,
    /* 17 - 24 */ WC4, N4, WC2, N2, NC4, N4, NC2, N2,
    /* 25 - 88 */ NC4, A4, NC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2,
                  AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2,
                  AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2,
                  AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2, AC4, A4, AC2, A2,
    /* 89 - 96 */ AC4, N4, AC2, N2, NC4, N4, NC2, N2,
    /* 97- 104 */ NC4, W4, NC2, W2, WC4, W4, WC2, W2,
    /* 105-113 */ WC4, 0, 0, 0, 0, 0, 0, 0, 0   // WC2 overscan and not needed
};

// non-char mode map - scan line data grab starts at 12, first line only (usually there's only 1 line of non-char mode but GR.9++)
const BYTE rgDMANC[HCLOCKS] =
{
    /*  0 - 11  */ DMA_M, DMA_DL, DMA_P, DMA_P, DMA_P, DMA_P, DMA_LMS, DMA_LMS, 0, 0, 0, 0,
    /* 12 - 19  */ W8, 0, W2, 0, W4, 0, W2, 0,
    /* 20 - 27 */ N8, 0, N2, 0, N4, 0, N2, 0,
    /* 28 - 91 */ A8, 0, A2, 0, A4, 0, A2, 0, A8, 0, A2, 0, A4, 0, A2, 0,   // 28-43
                  A8, 0, A2, 0, A4, 0, A2, 0, A8, 0, A2, 0, A4, 0, A2, 0,   // 44-59
                  A8, 0, A2, 0, A4, 0, A2, 0, A8, 0, A2, 0, A4, 0, A2, 0,   // 60-75
                  A8, 0, A2, 0, A4, 0, A2, 0, A8, 0, A2, 0, A4, 0, A2, 0,   // 76-91
    /* 92 - 99 */ N8, 0, N2, 0, N4, 0, N2, 0,
    /* 100- 107 */ W8, 0, W2, 0, W4, 0, 0, 0, // W2 overscan and not needed
    /* 108-113 */ 0, 0, 0, 0, 0, 0
};

BOOL fDumpHW;

// get ready for time travel!
BOOL TimeTravelInit(void *candy)
{
    // Initialize Time Travel

    if (pvm->fTimeTravelEnabled && !Time[0])
    {
        for (unsigned i = 0; i < 3; i++)
        {
            if (!Time[i])
                Time[i] = malloc(dwCandySize);
            if (!Time[i])
            {
                TimeTravelFree(candy);
                return FALSE;
            }
            _fmemset(Time[i], 0, dwCandySize);
        }
    }
    return TRUE;
}

// all done with time traveling, I'm exhausted
//
void TimeTravelFree(void *candy)
{
    for (unsigned i = 0; i < 3; i++)
    {
        if (Time[i])
            free(Time[i]);
        Time[i] = NULL;
    }
}

// use this specific spot to time travel to from now on
BOOL TimeTravelFixPoint(void *candy)
{
    // Turn on this feature if not on. This is not part of the VM save state, it's a GEM global, so it doesn't matter when we do it
    pvm->fTimeTravelEnabled = TRUE;
    pvm->fTimeTravelFixed = TRUE;

    FixAllMenus(FALSE); // GEM might not know this happened

    // we go back 3 save points, so save 3 times and then we will go back to this moment.
    if (TimeTravelPrepare(candy, TRUE))   // force save state now
        if (TimeTravelPrepare(candy, TRUE))
            if (TimeTravelPrepare(candy, TRUE))
                return TRUE;

    return FALSE;
}

// call this constantly every frame, it will save a state every 5 seconds, unless forced to now
//
BOOL TimeTravelPrepare(void *candy, BOOL fForce)
{
    // we aren't saving periodically for the time being, we have a fixed point
    if (pvm->fTimeTravelFixed && !fForce)
        return TRUE;

    // we apparently have just been turned on. So init and set an anchor point to now.
    if (pvm->fTimeTravelEnabled && !Time[0])
    {
        if (!TimeTravelInit(candy) || !TimeTravelReset(candy))
            return FALSE;
    }

    // nothing to do
    if (!pvm->fTimeTravelEnabled)
        return TRUE;

    BOOL f = TRUE;
    const ULONGLONG s5 = NTSC_CLK * 5;    // 5 seconds

    ULONGLONG cCur = GetCyclesN();  // always use the same NTSC clock, it doesn't matter
    ULONGLONG cTest = cCur - ullTimeTravelTime;

    // time to save a snapshot (every 5 seconds, or maybe we're forcing it)
    if (fForce || cTest >= s5)
    {
        f = SaveStateAtari(candy);

        if (!f)
            return FALSE;

        _fmemcpy(Time[cTimeTravelPos], candy, dwCandySize);

        // Do NOT remember that shift, ctrl or alt were held down.
        // If we cold boot using Ctrl-F10, almost every time travel with think the ctrl key is still down
        ((CANDYHW *)Time[cTimeTravelPos])->m_bshftByte = 0;

        ullTimeTravelTime = cCur;

        cTimeTravelPos++;
        if (cTimeTravelPos == 3)
            cTimeTravelPos = 0;
    }
    return f;
}

// Omega13 - go back in time 13 seconds. If TT is not on, turn it on for next time
//
BOOL TimeTravel(void *candy)
{
    pvm->fTimeTravelEnabled = TRUE;

    FixAllMenus(FALSE); // GEM might not know this happened

    // not on... turn it on for next time
    if (!Time[0])
    {
        if (!TimeTravelInit(candy) || !TimeTravelReset(candy))
            return FALSE;
        return FALSE;   // we didn't go back in time, did we?
    }

    BOOL f = FALSE;

    // Two 5-second snapshots ago will be from 10-15 seconds back in time

    f = LoadStateAtari(Time[cTimeTravelPos], candy, dwCandySize);    // restore our snapshot, and create a new anchor point here

    // lift up the control and ALT keys, do NOT remember their state from before (or we start continuous firing, etc.)
    ControlKeyUp8(candy);

    // center the joysticks and release the triggers
    rPADATA |= 255;
    TRIG0 = 1;
    TRIG1 = 1;
    if (mdXLXE == md800)
    {
        rPBDATA |= 255;
        TRIG2 = 1;
        TRIG3 = 1;
    }
    UpdatePorts(candy);

    return f;
}

// Call this at a point where you can't go back in time from... Cold Start, Warm Start, LoadState (e.g cartridge may be removed)
//
BOOL TimeTravelReset(void *candy)
{
    if (!pvm->fTimeTravelEnabled)
        return TRUE;    // don't fail, we'll quit the VM thinking something's wrong!

    // reset our clock to "have not saved any states yet"
    ullTimeTravelTime = GetCyclesN();
    cTimeTravelPos = 0;

    // don't remember that we were holding down shift, control or ALT, or going back in time will act like
    // they're still pressed because they won't see our letting go.
    // VERY common if you Ctrl-F10 to cold start, it's pretty much guaranteed to happen.
    // If we closed using ALT-F4 it will have saved the state that ALT is down
    // any other key can still stick, but that's less confusing
    // !!! Doing this instead of poking the saved shift byte off like TTPrepare() interferes a bit more with the VM
    // !!! joystick arrows stick too, and that's annoying. Need FLUSHVMINPUT in fn table
    ControlKeyUp8(candy);

    BOOL f = SaveStateAtari(candy);    // prepare our current state for persisting

    if (!f)
        return FALSE;

    // initialize all our saved states to now, so that going back in time takes us back here.
    for (unsigned i = 0; i < 3; i++)
    {
        if (Time[i] == NULL)
            return FALSE;

        _fmemcpy(Time[i], candy, dwCandySize);

        ullTimeTravelTime = GetCyclesN();    // time stamp it
    }

    return TRUE;
}

// push PC and P on the stack. Is this a BRK interrupt?
//
void Interrupt(void *candy, BOOL b)
{
    // if we were hit by an interrupt while hiding loop code, we have to show the code again
    if (wSIORTS)
        fTrace = TRUE;

    // the only time regB is used is to say if an IRQ was a BRK or not
    // is what is pushed on the stack at this time
    if (!b)
        regP &= ~0x10;
    else
        regPC++;    // did you know that BRK is a 2 byte instruction?

    cpuPokeB(candy, regSP, regPC >> 8);  regSP = (regSP - 1) & 255 | 256;
    cpuPokeB(candy, regSP, regPC & 255); regSP = (regSP - 1) & 255 | 256;
    cpuPokeB(candy, regSP, regP);          regSP = (regSP - 1) & 255 | 256;

    if (b)
        regPC--;

    // B bit always reads as 1 by the processor
    regP |= 0x10;    // regB
    regP |= IBIT;    // interrupts always SEI
}

// we just realized we're running an app that can only work properly on PAL
void SwitchToPAL(void *candy)
{
    if (v.fAutoKill && !fPAL && !fSwitchingToPAL)    // only if the option to auto-detect VM type is on
    {
        fSwitchingToPAL = TRUE;     // don't try again and accidentally switch back
        pvm->fEmuPAL = TRUE;   // GEM's flag that we want to be in PAL, to trigger the real switch when it's safe
        PostMessage(vi.hWnd, WM_COMMAND, IDM_FIXALLMENUS, 0); // Send will hang.
    }
}

// What happens when it's scan line 241 and it's time to start the VBI
// Joysticks, light pen, pasting, etc.
//
void DoVBI(void *candy)
{
    // process joysticks before the vertical blank, just because.
    // When tiling, only the tile in focus gets input
    if ((!v.fTiling || (sVM != -1 && candy == rgpvmi(sVM)->pPrivate)) && pvm->fJoystick)
    {
        // we can handle 4 joysticks on 800, 2 on XL/XE
        for (int joy = 0; joy < ((mdXLXE == md800) ? 4 : 2); joy++)
        {
            if (joy >= vi.njc)
            {
                // make paddles read neutral for any that aren't plugged in
                rgbMem[PADDLE0 + (joy << 1)] = 228;
                rgbMem[PADDLE0 + (joy << 1) + 1] = 228;
                continue;
            }
            
            JOYINFOEX ji;
            ji.dwSize = sizeof(JOYINFOEX);
            ji.dwFlags = JOY_RETURNALL;
            JOYCAPS jc = vi.rgjc[joy];

            MMRESULT mm = joyGetPosEx(vi.rgjn[joy], &ji);
            //ODS("Joy %d: x=%04x y=%04x u=%04x b=%04x\n", joy, ji.dwXpos, ji.dwYpos, ji.dwUpos, ji.dwButtons);
            
            if (mm == 0) {

                // if this axis ever goes non-zero, it's an XBOX joystick and we can use the right joystick safely as a paddle!
                if (ji.dwUpos)
                {
                    Assert(vi.rgjt[joy] & JT_JOYSTICK);
                    vi.rgjt[joy] |= JT_XBOXPADDLE;
                }

                // only the keypad has this many buttons, not even an XBOX joystick.
                // !!! Keypad isn't recognized until one of the last two buttons are pushed (0, #)
                if (ji.dwButtons & 0xc00)
                {
                    vi.rgjt[joy] = JT_KEYPAD;
                }

                if (vi.rgjt[joy] & (JT_JOYSTICK | JT_DRIVING))
                {
                    int dir = (ji.dwXpos - (jc.wXmax - jc.wXmin) / 2);
                    dir /= (int)((jc.wXmax - jc.wXmin) / wJoySens);

                    BYTE *pB;
                    if (joy < 2)
                        pB = &rPADATA;    // joy 1 & 2
                    else
                        pB = &rPBDATA;    // joy 3 & 4

                    // joy 1 & 3 are low nibble, 2 & 4 are high nibble

                    (*pB) |= (12 << ((joy & 1) << 2));                  // assume joystick X centered

                    if (dir < 0)
                        (*pB) &= ~(4 << ((joy & 1) << 2));              // left
                    else if (dir > 0)
                        (*pB) &= ~(8 << ((joy & 1) << 2));              // right

                    // driving controller special value reports $bfef or so
                    ULONG uz = abs((jc.wYmax - jc.wYmin) * 3 / 4 - (ji.dwYpos - jc.wYmin));

                    // we don't know which it is yet, figure it out
                    if ((vi.rgjt[joy] & (JT_JOYSTICK | JT_DRIVING)) == (JT_JOYSTICK | JT_DRIVING))
                    {
                        ULONG ux = abs((jc.wXmax - jc.wXmin) / 2 - (ji.dwXpos - jc.wXmin));
                        ULONG uy = abs((jc.wYmax - jc.wYmin) / 2 - (ji.dwYpos - jc.wYmin));

                        // This is a value a driving controller can't return, so we must be an XBox joystick or similar
                        if (!(ux < 0x20 && (uz < 0x20 || (ji.dwYpos - jc.wYmin) < 0x20 || uy < 0x20 || (jc.wYmax - ji.dwYpos) < 0x20)))
                            vi.rgjt[joy] &= ~JT_DRIVING;

                        // If we reached the special driving controller value without any invalid values first, we must be driving!
                        if (uz < 0x20)
                            vi.rgjt[joy] &= ~JT_JOYSTICK;
                    }

                    dir = (ji.dwYpos - (jc.wYmax - jc.wYmin) / 2);
                    dir /= (int)((jc.wYmax - jc.wYmin) / wJoySens);

                    (*pB) |= (3 << ((joy & 1) << 2));                   // assume joystick centered

                    if (vi.rgjt[joy] == JT_DRIVING && uz < 0x20)
                    {
                        (*pB) &= ~(1 << ((joy & 1) << 2));              // both up and down
                        (*pB) &= ~(2 << ((joy & 1) << 2));              //
                    }
                    else
                    {
                        if (dir < 0)
                            (*pB) &= ~(1 << ((joy & 1) << 2));          // up
                        else if (dir > 0)
                            (*pB) &= ~(2 << ((joy & 1) << 2));          // down
                    }

                    UpdatePorts(candy);

                    pB = &TRIG0;

                    if (ji.dwButtons == 1)
                        (*(pB + joy)) &= ~1;                // JOY fire button down
                    else
                        (*(pB + joy)) |= 1;                 // JOY fire button up

                    // extra buttons become START SELECT OPTION for tablet mode
                    if (!fJoyCONSOL && ji.dwButtons >= 2 && ji.dwButtons <= 8)
                    {
                        CONSOL &= ~(ji.dwButtons >> 1);	// button goes down
                        fJoyCONSOL = TRUE;				// we were the cause
                    }
                    else if (fJoyCONSOL && ji.dwButtons == 0)
                    {
                        CONSOL |= 7;						// don't lift up buttons we didn't press
                        fJoyCONSOL = FALSE;
                    }
                }

                // an XBOX right joystick will also serve as a paddle! That is the U axis. Only if we've ever seen the U axis
                // go non-zero should we do this. Moving the left joystick left will be the paddle trigger.
                // Also, a keypad returns some of its data in the paddles

                if (vi.rgjt[joy] == JT_PADDLE || (vi.rgjt[joy] & JT_XBOXPADDLE) || vi.rgjt[joy] == JT_KEYPAD)
                {
                    int x, y;

                    // x or u value is left paddle, y value is right paddle, scaled to 0-228
                    if (vi.rgjt[joy] == JT_PADDLE)
                    {
                        x = (ji.dwXpos - jc.wXmin) * 229 / (jc.wXmax - jc.wXmin);
                        y = (ji.dwYpos - jc.wYmin) * 229 / (jc.wYmax - jc.wYmin);
                    }
                    
                    else if (vi.rgjt[joy] == JT_KEYPAD)
                    {
                        // They write to a port to say which row they are interested in retrieving data for.
                        // The keypad/USB converter simply sets a button bit from 1 to $800.
                        int row = rgbMem[wPADATAea + (joy >> 1)] >> ((joy & 1) << 2);   // will be 1, 2, 4 or 8 for rows 0-3
                        int r = 0;
                        // convert to 0-3
                        while ((row & 1) && r < 4)
                        {
                            r++;
                            row >>= 1;
                        }
                        if (r < 4)
                        {
                            ji.dwButtons >>= (r * 3);   // bottom 3 bits are now the button states we want to return

                            y = (ji.dwButtons & 0x1) ? 228 : 0; // left button in the row is right paddle
                            x = (ji.dwButtons & 0x2) ? 228 : 0; // middle button is left paddle
                            *(&TRIG0 + joy) &= 0xfe;
                            *(&TRIG0 + joy) |= (ji.dwButtons & 0x4) ? 0 : 1;    // right button is stick trigger
                        }
                        else
                        {
                            x = y = 228;            // they don't want any keypad buttons read, return neutral
                            *(&TRIG0 + joy) |= 1;
                        }
                    }

                    // XBOX as left paddle using U axis
                    else
                    {
                        x = (ji.dwUpos - jc.wXmin) * 229 / (jc.wUmax - jc.wXmin);   // assume the same max and min
                        y = 228;                                                    // there is no second paddle
                    }

                    if (x == 229) x = 228;  // give 228 a reasonable chance of being chosen
                    if (y == 229) y = 228;

                    rgbMem[PADDLE0 + (joy << 1)] = (BYTE)x;
                    rgbMem[PADDLE0 + (joy << 1) + 1] = (BYTE)y;

                    // left paddle button is same as joystick left
                    // right paddle button is joystick right

                    BYTE *pB;
                    if (joy < 2)
                        pB = &rPADATA;    // paddles 0-3
                    else
                        pB = &rPBDATA;    // paddles 4-7

                    // only process paddle buttons for a real paddle - otherwise we move the joystick they are using on them!
                    if (vi.rgjt[joy] == JT_PADDLE)
                    {
                        // paddles 0-1 & 4-5 are low nibble, 2-3 & 6-7 are high nibble

                        (*pB) |= (12 << ((joy & 1) << 2));                  // assume buttons not pressed

                        if (ji.dwButtons == 1)
                            (*pB) &= ~(4 << ((joy & 1) << 2));              // left
                        else if (ji.dwButtons == 2)
                            (*pB) &= ~(8 << ((joy & 1) << 2));              // right

                        UpdatePorts(candy);
                    }
                }

                // if this device is not a paddle, use the values that mean a paddle is not plugged in
                else
                {
                    rgbMem[PADDLE0 + (joy << 1)] = 228;
                    rgbMem[PADDLE0 + (joy << 1) + 1] = 228;
                }
            }

            // a joystick isn't working that was before. Time to re-query the joystick situation
            // DO NOT POLL for new joysticks being added while some joysticks are working! That breaks the working
            // joysticks and re-centres them!
            else
            {
                vi.fJoyNeedsInit = 5;   // check every this many seconds
                vi.njc = 0;             // don't keep trying the broken joysticks and resetting to 5 all the time
            }
        }
    }

    // Process LightPen - we are told which pixel we are on, translate that to colour clock and VCOUNT
    if (candy == LightPenVM)
    {
        // For some reason, although the left side of the screen is colour clock 27, you need to report it as 67 + (27/2)
        // !!! X could use calibrating - different lights pens and apps behaved differently
        PENH = (BYTE)(LightPenX / 2 + 67 + 27 / 2);
        PENV = (BYTE)((LightPenY + 8) / 2);

        if (PENH < 8)
            PENH = 255;    // it can wrap
        if (PENV < 4)
            PENV = 4;
        if (PENV > 124)
            PENV = 124;
    }
    else
    {
        PENH = 0;
        PENV = 0;
    }

    // are we pasting into the keyboard buffer?
    if (cPasteBuffer && v.iVM >= 0 && candy == rgpvmi(v.iVM)->pPrivate)
    {
        fBrakes = FALSE;    // this is SLOW so we definitely need turbo mode for this
        
        if (!(iPasteBuffer & 1))    // send both bytes on the EVEN count
        {
            BYTE b = rgPasteBuffer[iPasteBuffer];

            if (b == 0x2a)    // SHIFT
            {
                wLiveShift = wAnyShift;
                iPasteBuffer += 2;
            }
            else if (b == 0x1d)   // CTRL
            {
                wLiveShift = wCtrl;
                iPasteBuffer += 2;
            }
            else
            {
                wLiveShift = 0;
            }

            extern BYTE rgbMapScans[1024];

            //ODS("%02x (%04x)\n", rgPasteBuffer[iPasteBuffer], wFrame);
            AddToPacket(candy, rgPasteBuffer[iPasteBuffer]);
            AddToPacket(candy, rgPasteBuffer[iPasteBuffer + 1]);
            iPasteBuffer++;
        }
        else
        {
            iPasteBuffer++; // do nothing on the odd count to make sure the key is noticed, one key per jiffy sometimes misses keys
        }
    }
    else if (cPasteBuffer == 0)
    {
        fBrakesSave = fBrakes;  // remember last state of fBrakes
    }

    if (cPasteBuffer && iPasteBuffer >= cPasteBuffer)
    {
        Assert(iPasteBuffer == cPasteBuffer);
        cPasteBuffer = 0;   // all done!
        if (rgPasteBuffer)
            free(rgPasteBuffer);
        rgPasteBuffer = NULL;
        fBrakes = fBrakesSave;  // go back to old turbo mode
    }

    // decrement printer timer

    if (vi.cPrintTimeout && pvm->fShare)
    {
        vi.cPrintTimeout--;
        if (vi.cPrintTimeout == 0)
        {
            FlushToPrinter(0);  // !!! not thread safe, can't pass VM#
            UnInitPrinter();
        }
    }
    else
        FlushToPrinter(0);  // !!! not thread safe, can't pass VM#
}

void UpdatePorts(void *candy)
{
    if (PACTL & 4)
    {
        // return the read or write status of each bit, depending on if it's a read or write bit

        BYTE b = wPADATA ^ rPADATA;         // bits that don't agree

        PORTA = (b & wPADDIR) ^ rPADATA;    // return read value, except for bits that are write bits and different from the read value
    }
    else
        PORTA = wPADDIR;                    // return the direction of each bit

    if (PBCTL & 4)
    {
        BYTE b = wPBDATA ^ rPBDATA;

        PORTB = (b & wPBDDIR) ^ rPBDATA;
    }
    else
        PORTB = wPBDDIR;
}

// let the jump table know which POKE routine to use based on where RAMTOP is
void AlterRamtop(void *candy, WORD addr)
{
    ramtop = addr;

#if USE_POKE_TABLE || USE_PEEK_TABLE
    int i;
    for (i = 0x80; i < (ramtop >> 8); i++)
    {
#if USE_POKE_TABLE
        write_tab[i] = cpuPokeB;
#endif
#if USE_PEEK_TABLE
        read_tab[i] = cpuPeekB;
#endif
    }
    for (; i < 0xc0; i++)
    {
#if USE_POKE_TABLE
        write_tab[i] = PokeBAtariNULL;
#endif
#if USE_PEEK_TABLE
        read_tab[i] = cpuPeekB;    // unless BountyBob changes this to do bank selecting, plus this changes it back
#endif
    }
#endif
}

// Read in the cartridge. Are we initializing and using the default bank, or restoring a saved state to the last bank used?
//
BOOL ReadCart(void *candy, BOOL fDefaultBank)
{
    unsigned char *pch = (unsigned char *)pvm->rgcart.szName;

    int h;
    int cb = 0, cb2;
    BYTE type = 0, *pb;

    if (!pch || !pvm->rgcart.fCartIn)
    {
        bCartType = 0;
        return TRUE;    // nothing to do, all OK
    }

    h = _open((LPCSTR)pch, _O_BINARY | _O_RDONLY);
    if (h != -1)
    {
        //printf("reading %s\n", pch);

        cb = _lseek(h, 0L, SEEK_END);

        // valid sizes are 4K or 8-1024K in multiples of 8K, with or without a 16 byte header
        if (cb > MAX_CART_SIZE || ((cb & ~0x1fe010) && cb & ~0x1010))
        {
            _close(h);
            bCartType = 0;
            goto exitCart;
        }

        // must be unsigned to compare the byte values inside
        rgbSwapCart = malloc(cb);
        pb = rgbSwapCart;
        type = 0;

        _lseek(h, 0L, SEEK_SET);

        // read the header
        if (pb && (cb & 0x10))
        {
            cb2 = _read(h, pb, 16);
            if (cb2 != 16)
            {
                _close(h);
                bCartType = 0;
                goto exitCart;
            }
            cb -= 16;
            type = pb[7];    // the type of cartridge
        }

        //      printf("size of %s is %d bytes\n", pch, cb);
        //      printf("pb = %04X\n", rgcart[iCartMac].pbData);

        if (pb)
        {
            cb = _read(h, pb, cb);

            pvm->rgcart.cbData = cb;
            pvm->rgcart.fCartIn = TRUE;
        }
        else
        {
            bCartType = 0;
            goto exitCart;
        }
    }
    else
    {
        bCartType = 0;
        goto exitCart;
    }

    _close(h);

    // we're restoring - we already know the cart type! And we might not be able to figure it out again if we tried
    if (!fDefaultBank)
        goto exitCart;

    // figure out the cart type
    bCartType = 0;

    // every type that is a RAM cart must also set its default bank #, and tell us the bank size and # of banks

    // what kind of cartridge is it? Here are the possibilites
    //
    //{name: 'Standard 8k cartridge', id : 1 },
    //{name: 'Standard 16k cartridge', id : 2 },
    //{ name: 'Diamond 64 KB cartridge', id : 10 },     // !!! buggy
    //{ name: 'SpartaDOS X 64 KB cartridge', id : 11 }, // !!! buggy
    //{name: 'XEGS 32 KB cartridge', id : 12 },
    //{name: 'XEGS 64 KB cartridge', id : 13 },
    //{name: 'XEGS 128 KB cartridge', id : 14 },
    // Atrax 128K, id : 17
    //{name: 'Bounty Bob Strikes Back 40 KB cartridge', id : 18 },
    //{name: 'XEGS 256 KB cartridge', id : 23 },
    //{name: 'XEGS 512 KB cartridge', id : 24 },
    //{ name: 'XEGS 1024 KB cartridge', id : 25 },
    //{ name: 'MegaCart 16 KB cartridge', id : 26 },
    //{ name: 'MegaCart 32 KB cartridge', id : 27 },
    //{ name: 'MegaCart 64 KB cartridge', id : 28 },
    //{ name: 'MegaCart 128 KB cartridge', id : 29 },
    //{ name: 'MegaCart 256 KB cartridge', id : 30 },
    //{ name: 'MegaCart 512 KB cartridge', id : 31 },
    //{ name: 'MegaCart 1024 KB cartridge', id : 32 },
    //{ name: 'Atarimax 128 KB cartridge', id : 41 },
    //{name: 'Atarimax 1 MB Flash cartridge', id : 42 },
    // 4K cartridge, id : 58

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
    //{ name: 'Blizzard 16 KB cartridge', id : 40 },
    //{ name: '32 KB Williams cartridge', id : 22 },
    //{ name: '64 KB Williams cartridge', id : 8 },
    //{ name: 'Express 64 KB cartridge ', id : 9 },
    //{ name: 'SpartaDOS X 128 KB cartridge', id : 43 },
    //{ name: 'SIC! 128 KB cartridge', id : 54 },
    //{ name: 'SIC! 256 KB cartridge', id : 55 },
    //{ name: 'SIC! 512 KB cartridge', id : 56 },

    if ((type == 58 || !type) && cb == 4096)
        bCartType = CART_4K;

    else if (type == 1 || (!type && cb == 8192))
        bCartType = CART_8K;

    else if (cb == 16384)
    {
        // allows RAM to be swapped in instead
        if (type == 26)
        {
            bCartType = CART_MEGACART;
            fRAMCart = TRUE;
            iSwapCart = 0;
            iBankSize = 0x4000;
            iNumBanks = 1;
        }

        // copies of the INIT address and the CART PRESENT byte appear in both cartridge areas - not banked
        else if (type == 2 || (pb[16383] >= 0x80 && pb[16383] < 0xC0 && pb[16380] == 0 && pb[8191] >= 0x80 && pb[8191] < 0xC0 && pb[8188] == 0))
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
            bCartType = CART_OSSAX;

        // first bank is the main bank, other banks are 0, 9, 1.
        else if (type == 15 ||
                (type == 0 && pb[4095] >= 0x80 && pb[4095] < 0xC0 && pb[4092] == 0 && pb[8191] == 0 && pb[12287] == 9 && pb[16383] == 1))
            bCartType = CART_OSSB;

        // last bank is the main bank, other banks are 0, 9, 1. (Action alt 2 has a 0 instead of 9)
        else if (type == 0 && pb[16383] >= 0x80 && pb[16383] < 0xC0 && !pb[16380] &&
                    !pb[4095] && (pb[8191] == 9 || !pb[8191]) && pb[12287] == 1)
            bCartType = CART_OSSBX;

        // last bank is the main bank, other banks are 0, 1, 9. (Action A has a 0 instead of 9)
        else if (type == 0 && pb[16383] >= 0x80 && pb[16383] < 0xC0 && !pb[16380] &&
                !pb[4095] && pb[8191] == 1 && (pb[12287] == 9 || !pb[12287]))
            bCartType = CART_OSSBY;

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

    // unique DIAMOND cartridge - all 8 8K segments are valid with 5 as the boot byte, 1st segment is the main one
    else if (type == 10 || (type == 0 && cb == 0x10000 && !pb[0x1ffc] && pb[0x1fff] >= 0xa0 && pb[0x1fff] < 0xc0 && pb[0x1ffd] == 5 && 
                !pb[0x3ffc] && pb[0x3ffd] == 5 && !pb[0xfffc] && pb[0xfffd] == 5))
    {
        bCartType = CART_DIAMOND;    // unique DIAMOND cart
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x2000;
        iNumBanks = 8;
    }
    
    // unique SPARTA cartridge - every 8K segment is valid with 1 as the boot byte
    else if (type == 11 || (type == 0 && cb == 0x10000 && !pb[0x1ffc] && pb[0x1fff] >= 0xa0 && pb[0x1fff] < 0xc0 && pb[0x1ffd] == 1 &&
        !pb[0x3ffc] && pb[0x3ffd] == 1 && !pb[0xfffc] && pb[0xfffd] == 1))
    {
        bCartType = CART_SPARTA;    // unique SPARTA cart
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x2000;
        iNumBanks = 8;
    }

    // unique 40K Bounty Bob cartridge - check for last bank being valid?
    else if (type == 18 || (type == 0 && cb == 40960))
    {
        bCartType = CART_BOB;    // unique Bounty Bob cart
    }

    // ATARIMAX1 - 128K and 1st 8K bank is the main one
    else if (type == 41 && *(pb + 8188) == 0 &&
        ((*(pb + 8191) >= 0x80 && *(pb + 8191) < 0xC0) || (*(pb + 8187) >= 0x80 && *(pb + 8187) < 0xC0)))
    {
        bCartType = CART_ATARIMAX1;
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x2000;
        iNumBanks = 16;
    }

    // I assume this is the variant where the last bank is the default one
    else if (type == 41)
    {
        bCartType = CART_ATARIMAX1L;
        fRAMCart = TRUE;
        iSwapCart = 0xf;
        iBankSize = 0x2000;
        iNumBanks = 16;
    }

    // 1M, 8K banks and first bank is the main one - ATARIMAX8
    else if ((type == 42 || type == 0) && cb == 1048576 && *(pb + 8188) == 0 &&
        ((*(pb + 8191) >= 0x80 && *(pb + 8191) < 0xC0) || (*(pb + 8187) >= 0x80 && *(pb + 8187) < 0xC0)))
    {
        bCartType = CART_ATARIMAX8;
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x2000;
        iNumBanks = 128;
    }

    // I assume this is the variation where the last bank is the default
    else if (type == 42)
    {
        bCartType = CART_ATARIMAX8L;
        fRAMCart = TRUE;
        iSwapCart = 127;
        iBankSize = 0x2000;
        iNumBanks = 128;
    }
    
    // ATRAX - 128K and 1st 8K bank is the main one
    else if (type == 17)
    {
        bCartType = CART_ATRAX;
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x2000;
        iNumBanks = 16;
    }
    
    // 128K, 8K banks, 1st bank is the main one, last bank isn't valid - either CART_ATRAX or CART_ATARIMAX1.
    // (I assume it can't be ATRAX if last bank is valid but it might be XEGS)
    else if (type == 0 && cb == 131072 && !(*(pb + 8188)) && (*(pb + 131068) || *(pb + 131071) < 0xa0 || *(pb + 131071) >= 0xc0) && 
        ((*(pb + 8191) >= 0x80 && *(pb + 8191) < 0xC0) || (*(pb + 8187) >= 0x80 && *(pb + 8187) < 0xC0)))
    {
        bCartType = CART_ATARIMAX1_OR_ATRAX;
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x2000;
        iNumBanks = 16;
    }

    // 32K-1MB, 16K banks and 1st bank is the main one and last bank NOT valid 8K segment like XEGS (Star Raiders II) - MEGACART
    else if ((type >= 27 && type <= 32) || (type == 0 && 
            (cb == 0x8000 || cb == 0x10000 || cb == 0x20000 || cb == 0x40000 || cb == 0x80000 || cb == 0x100000) &&
            !(*(pb + 0x3ffc)) && (*(pb + cb - 4) || *(pb + cb - 1) < 0xa0 || *(pb + cb - 1) >= 0xc0) &&
            ((*(pb + 0x3fff) >= 0x80 && *(pb + 0x3fff) < 0xC0) || (*(pb + 0x3ffb) >= 0x80 && *(pb + 0x3ffb) < 0xC0))))
    {
        bCartType = CART_MEGACART;
        fRAMCart = TRUE;
        iSwapCart = 0;
        iBankSize = 0x4000;
        iNumBanks = (BYTE)(cb / 0x4000);
    }

    // both first and last 8K of 128K are valid... could be ATARIMAX1L or XEGS (main bank is the last one in each)
    else if (type == 0 && cb == 0x20000 && !(*(pb + 0x1ffc)) && !(*(pb + 0x1fffc)))
    {
        bCartType = CART_XEGS_OR_ATARIMAX1L;
        //fRAMCart = TRUE; don't know yet
        iSwapCart = 15;
        iBankSize = 0x2000;
        iNumBanks = 16;
    }

    // make sure the last bank is a valid ATARI 8-bit cartridge - assume XEGS
    // last 8K bank goes from $a000-$c000 and other 8K banks get swapped into $8000-$a000
    else if ( ((type >= 12 && type <= 14) || (type >=23 && type <=25) || (type >= 33 && type <= 38)) || (type == 0 &&
        (((*(pb + cb - 1) >= 0x80 && *(pb + cb - 1) < 0xC0) || (*(pb + cb - 5) >= 0x80 && *(pb + cb - 1) < 0xC0)) && *(pb + cb - 4) == 0)))
    {
        if (cb == 0x100000)
            bCartType = CART_XEGS_OR_ATARIMAX8L; // actually, it could be either of these
        else
            bCartType = CART_XEGS;
        //fRAMCart = TRUE; not for XEGS
        iNumBanks = (BYTE)(cb / 0x2000);
        iSwapCart = iNumBanks - 1;
        iBankSize = 0x2000;
    }

    // I give up, never heard of it
    else
        bCartType = 0;

#if 0
    // unknown cartridge, do not attempt to run it
    else
    {
        pvm->rgcart.cbData = 0;
        pvm->rgcart.fCartIn = FALSE;
    }
#endif

exitCart:

    if (!bCartType)
    {
        pvm->rgcart.fCartIn = FALSE;    // unload the cartridge
        pvm->rgcart.szName[0] = 0; // erase the name
    }

    return bCartType;    // say if cart looks good or not
}

// helper function to swap either a ROM bank or RAM in (8K or 16K banks)
// AlterRamtop should always be called, and if it looks like we're swapping in the same bank that's already there,
// do it anyway because that's what happens when we restore a saved state
void SwapRAMCart(void *candy, WORD size, BYTE *pb, BYTE iBank)
{
    Assert(size <= 0x4000);

    BYTE swap[0x4000];  // big enough for 16K !!! large stack, but global isn't thread safe (total must be kept < 64K thread stack space)

    // !!! Sorry, if you swapped BASIC in when your cartridge was out, you're stuck that way until you swap BASIC out
    if (iSwapCart == iNumBanks && ramtop < 0xc000)
        return;

    // this is the RAM bank
    if (iBank >= iNumBanks)
    {
        // want RAM - currently ROM - swap out that ROM and put the RAM stored there back in
        if (ramtop == 0xc000 - size)
        {
            _fmemcpy(swap, &rgbMem[0xc000 - size], size);
            _fmemcpy(&rgbMem[0xc000 - size], pb + iSwapCart * size, size);
            _fmemcpy(pb + iSwapCart * size, swap, size);
        }
        AlterRamtop(candy, 0xc000);
        iSwapCart = iNumBanks;

        // XL/XE uses TRIG3 to tell programs if a cartridge is in
        if (mdXLXE != md800)
            TRIG3 = 0;
    }
    else
    {
        // want ROM, currently different ROM? First swap the old ROM out and put RAM in so it doesn't get lost
        if (ramtop == (0xc000 - size) && iBank != iSwapCart)
        {
            _fmemcpy(swap, &rgbMem[0xc000 - size], size);
            _fmemcpy(&rgbMem[0xc000 - size], pb + iSwapCart * size, size);
            _fmemcpy(pb + iSwapCart * size, swap, size);
        }

        // do NOT swap if that's the bank already in... you'll swap it out and swap RAM in!
        if (ramtop == 0xc000 || iBank != iSwapCart)
        {
            // now swap in the desired bank
            _fmemcpy(swap, &rgbMem[0xc000 - size], size);
            _fmemcpy(&rgbMem[0xc000 - size], pb + iBank * size, size);
            _fmemcpy(pb + iBank * size, swap, size);

            iSwapCart = iBank;    // what bank is in there now
            AlterRamtop(candy, 0xc000 - size);
        }

        // XL/XE uses TRIG3 to tell programs if a cartridge is in
        if (mdXLXE != md800)
            TRIG3 = 1;
    }
}

// SWAP in the cartridge
//
void InitCart(void *candy)
{
    // every code path must call AlterRamtop, to set the jump tables correctly for that address space
    // (possibly un-doing the BountyBob bank select)

    // no cartridge
    if (!(pvm->rgcart.fCartIn) || !bCartType)
    {
        // convenience for Atari 800, we can ask for BASIC to be put in
        if (pvm->bfHW == vmAtari48 && ramtop <= 0xA000)
        {
            _fmemcpy(&rgbMem[0xA000], rgbXLXEBAS, 8192);
            AlterRamtop(candy, 0xa000);
        }
        else
            AlterRamtop(candy, ramtop);   // every code path must call this
        return;
    }

    PCART pcart = &(pvm->rgcart);
    unsigned int cb = pcart->cbData;
    BYTE *pb = rgbSwapCart;

    if (bCartType == CART_4K)
    {
        _fmemcpy(&rgbMem[0xb000], pb, 4096);
        AlterRamtop(candy, 0xa000);   // even though cart starts at 0xb000
    }
    
    else if (bCartType == CART_8K)
    {
        _fmemcpy(&rgbMem[0xC000 - (((cb + 4095) >> 12) << 12)], pb, (((cb + 4095) >> 12) << 12));
        AlterRamtop(candy, 0xa000);
    }
    
    else if (bCartType == CART_16K)
    {
        _fmemcpy(&rgbMem[0x8000], pb, 16384);
        AlterRamtop(candy, 0x8000);
    }
    
    // main bank is the last one
    else if (bCartType == CART_OSSA || bCartType == CART_OSSAX || bCartType == CART_OSSBX || bCartType == CART_OSSBY)
    {
        _fmemcpy(&rgbMem[0xB000], pb + 12288, 4096);
        AlterRamtop(candy, 0xa000);
    }
    
    // main bank is the first one
    else if (bCartType == CART_OSSB)
    {
        _fmemcpy(&rgbMem[0xB000], pb, 4096);
        AlterRamtop(candy, 0xa000);
    }

    // 8K main bank is the last one, and init the $8000 and $9000 banks to their bank 0's
    else if (bCartType == CART_BOB)
    {
        _fmemcpy(&rgbMem[0xA000], pb + cb - 8192, 8192);
        _fmemcpy(&rgbMem[0x8000], pb, 4096);
        _fmemcpy(&rgbMem[0x9000], pb + 16384, 4096);
        AlterRamtop(candy, 0x8000);

#if USE_POKE_TABLE
        // default ramtop jump table setting is not right... set up jump table to handle bank selecting
        write_tab[0x8f] = PokeBAtariBB;
        write_tab[0x9f] = PokeBAtariBB;
#endif
#if USE_PEEK_TABLE
        // and the read jump table too
        read_tab[0x8f] = PeekBAtariBB;
        read_tab[0x9f] = PeekBAtariBB;
#endif
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
        AlterRamtop(candy, 0x8000);
    }

    // one of the many cartridges that also can be swapped out to RAM, or perhaps we're not sure yet
    // put the correct bank in, but also save the RAM that may have been restored on load.
    else if (fRAMCart || bCartType == CART_XEGS_OR_ATARIMAX1L || bCartType == CART_XEGS_OR_ATARIMAX8L)
    {
        // Swap won't do anything if iSwapCart is already the bank that's in, and shouldn't, since that would swap the thing
        // we want back out! Set RAMTOP high to force Swap to actually put bank #iSwapCart in
        ramtop = 0xc000;
        SwapRAMCart(candy, iBankSize, pb, iSwapCart);
        fCartNeedsSwap = FALSE; // we just took care of that
    }

#if 0
    // 8K main bank is the first one
    else if (bCartType == CART_DIAMOND || bCartType == CART_SPARTA)
    {
        if (iSwapCart == 8)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xA000], pb, 8192);
            AlterRamtop(candy, 0xa000);
        }
    }
    
    // 8K main bank is the first one for ATARIMAX1
    // if we're not sure yet, then iSwapCart won't be the special value
    else if (bCartType == CART_ATARIMAX1 || bCartType == CART_ATARIMAX1_OR_ATRAX)
    {
        if (iSwapCart == 0x10)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * iSwapCart, 8192);
            AlterRamtop(candy, 0xa000);
        }
    }

    // old version where the initial bank is the last one
    else if (bCartType == CART_ATARIMAX1L || bCartType == CART_XEGS_OR_ATARIMAX1L)
    {
        if (iSwapCart == 0x80)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * 0x0f, 8192); // last 8K bank of 128K is bank 0x0f
            AlterRamtop(candy, 0xa000);
        }
    }

    // old version where the initial bank is the last one
    // or we're not sure, so let $8000-$a000 be RAM for now in case it's ATARIMAX8L
    else if (bCartType == CART_ATARIMAX8L || bCartType == CART_XEGS_OR_ATARIMAX8L)
    {
        if (iSwapCart == 0x80)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * 0x7f, 8192); // last 8K bank of 1MB is bank 0x7f
            AlterRamtop(candy, 0xa000);
        }
    }

    // initial bank is the first one
    else if (bCartType == CART_ATARIMAX8)
    {
        if (iSwapCart == 0x80)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * 0, 8192); // start with bank 0
            AlterRamtop(candy, 0xa000);
        }
    }
    // old version where the initial bank is the last one
    // or we're not sure, so let $8000-$a000 be RAM for now in case it's ATARIMAX8L
    else if (bCartType == CART_ATARIMAX8L || bCartType == CART_XEGS_OR_ATARIMAX8L)
    {
        if (iSwapCart == 0x80)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * 0x7f, 8192); // last 8K bank of 1MB is bank 0x7f
            AlterRamtop(candy, 0xa000);
        }
    }
    // 8K main bank is the first one
    else if (bCartType == CART_ATRAX)
    {
        if (iSwapCart >= 0x80)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0xa000], pb + 8192 * 0, 8192); // start with bank 0
            AlterRamtop(candy, 0xa000);
        }
    }
    // 16K main bank is the first one
    else if (bCartType == CART_MEGACART)
    {
        if (iSwapCart >= 0x80)
            AlterRamtop(candy, 0xc000);    // RAM is in right now
        else
        {
            _fmemcpy(&rgbMem[0x8000], pb + 0x4000 * 0, 0x4000); // start with bank 0
            AlterRamtop(candy, 0x8000);
        }
    }
#endif

    else
        Assert(0);  // oops, what did I forget?

    return;
}

// Swap out cartridge banks
//
void BankCart(CANDYHW *candy, BYTE iBank, BYTE value)
{
    PCART pcart = &(pvm->rgcart);
    unsigned int cb = pcart->cbData;
    int i;
    BYTE *pb = rgbSwapCart;

    // we are not a banking cartridge
    if (!(pvm->rgcart.fCartIn) || bCartType <= CART_16K)
        return;

    // we aren't sure yet, figure it out based on type of banking attempted
    if (bCartType == CART_ATARIMAX1_OR_ATRAX)
    {
        if (value > 0x10 && value < 0x80)
            bCartType = CART_ATARIMAX1; // ATRAX bank# is in value, and would never be 17-127
        else if (iBank > 0)
            bCartType = CART_ATARIMAX1; // ATRAX would never use an address != 0xd500 (I hope)
        // 0xfe is hack for Loderunner 2010 TURBOSOFT which is identical behaviour to ATARIMAX1
        else if (iBank == 0 && value > 0 && (value <= 0x0f || value >= 0x80) && value != 0xfe)
            bCartType = CART_ATRAX;     // ATRAX asks for non-zero bank#, better respond to it and hope for the best

        // otherwise, we're just asking for bank 0 which we already have, so delay our decision
    }

    // we aren't sure between these
    else if (bCartType == CART_XEGS_OR_ATARIMAX8L)
    {
        if (iBank > 0)
        {
            bCartType = CART_ATARIMAX8L; // XEGS would never use an address != 0xd500 (I hope)
            fRAMCart = TRUE;             // unlike XEGS, this is a RAM cart
        }
        else if (iBank == 0 && value > 0)
        {
            bCartType = CART_XEGS;      // XEGS asks for non-zero bank#, better respond to it and hope for the best

            AlterRamtop(candy, 0x8000);   // XEGS uses 16K not 8K

            // right cartridge present bit must float high if it is not RAM and not part of this bank
            // but if restoring a state, there may be a bank swapped in there, so don't trash memory.
            // It's probably not zero, or cold start would crash.
            if (rgbMem[0x9ffc] == 0)
                rgbMem[0x9ffc] = 0xff;
        }
    }

    // we aren't sure between these
    else if (bCartType == CART_XEGS_OR_ATARIMAX1L)
    {
        if (iBank > 0)
        {
            bCartType = CART_ATARIMAX1L; // XEGS would never use an address != 0xd500 (I hope)
            fRAMCart = TRUE;             // unlike XEGS, this is a RAM cart
        }
        else if (iBank == 0 && value > 0 && value < 0xf)
        {
            bCartType = CART_XEGS;      // XEGS asks for non-zero bank#, better respond to it and hope for the best

            AlterRamtop(candy, 0x8000);   // XEGS uses 16K not 8K

            // right cartridge present bit must float high if it is not RAM and not part of this bank
            // but if restoring a state, there may be a bank swapped in there, so don't trash memory.
            // It's probably not zero, or cold start would crash.
            if (rgbMem[0x9ffc] == 0)
                rgbMem[0x9ffc] = 0xff;
        }
        // actually, we appear to really want bank 0, which means we're neither, but probably ATARIMAX1
        else if (iBank == 0)
        {
            bCartType = CART_ATARIMAX1L;
            fRAMCart = TRUE;
            // fall through to code that will swap bank 0 in, luckily it doesn't seem to be too late!
        }
    }

    // Bank based on CART TYPE

    // banks are 0, 3, 4, main
    if (bCartType == CART_OSSA)
    {
        i = (iBank == 0 ? 0 : (iBank == 3 ? 1 : (iBank == 4 ? 2 : -1)));
        Assert(i != -1);
        if (i != -1)
            _fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
    }

    // banks are 0, 4, 3, main
    else if (bCartType == CART_OSSAX)
    {
        i = (iBank == 0 ? 0 : (iBank == 3 ? 2 : (iBank == 4 ? 1 : -1)));
        Assert(i != -1);
        if (i != -1)
            _fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
    }

    // banks are main, 0, 9, 1
    else if (bCartType == CART_OSSB)
    {
        if (!(iBank & 8) && !(iBank & 1))
            i = 1;
        else if (!(iBank & 8) && (iBank & 1))   // may ask for bank 1 with iBank == 3
            i = 3;
        else if ((iBank & 8) && (iBank & 1))
            i = 2;
        else
            i = -1;     //!!! swapping OSS A & B cartridge out to RAM not supported

        Assert(i != -1);
        if (i != -1)
            _fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
    }

    // banks are 0, 9, 1, main
    else if (bCartType == CART_OSSBX)
    {
        if (!(iBank & 8) && !(iBank & 1))
            i = 0;
        else if (!(iBank & 8) && (iBank & 1))   // may ask for bank 1 with iBank == 3
            i = 2;
        else if ((iBank & 8) && (iBank & 1))
            i = 1;
        else
            i = -1;     //!!! swapping OSS A & B cartridge out to RAM not supported

        Assert(i != -1);
        if (i != -1)
            _fmemcpy(&rgbMem[0xA000], pb + i * 4096, 4096);
    }

    // banks are 0, 1, 9, main
    else if (bCartType == CART_OSSBY)
    {
        if (!(iBank & 8) && !(iBank & 1))
            i = 0;
        else if (!(iBank & 8) && (iBank & 1))   // may ask for bank 1 with iBank == 3
            i = 1;
        else if ((iBank & 8) && (iBank & 1))
            i = 2;
        else
            i = -1;     //!!! swapping OSS A & B cartridge out to RAM not supported

        Assert(i != -1);
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
        while (value >= cb >> 13)
            value -= (BYTE)(cb >> 13);

        if (value < (cb << 13))
            _fmemcpy(&rgbMem[0x8000], pb + value * 8192, 8192);
    }

    // $d5d7 through $d5d0 is bank 0-7. $d5d8 through $d5df is RAM
    else if (bCartType == CART_DIAMOND)
    {
        if (iBank >= 0xd0 && iBank <= 0xd7)
        {
            iBank = 0xd7 - iBank;
            SwapRAMCart(candy, iBankSize, pb, iBank);
        }
        else if (iBank >= 0xd8 && iBank <= 0xdf)
            SwapRAMCart(candy, iBankSize, pb, iNumBanks); // RAM
    }

    // $d5e7 through $d5e0 is bank 0-7. $d5e8 through $d5ef is RAM, I hope
    // !!! 2nd cartridge not supported
    else if (bCartType == CART_SPARTA)
    {
        if (iBank >= 0xe0 && iBank <= 0xe7)
        {
            iBank = 0xe7 - iBank;
            SwapRAMCart(candy, iBankSize, pb, iBank);
        }
        else if (iBank >= 0xe8 && iBank <= 0xef)
            SwapRAMCart(candy, iBankSize, pb, iNumBanks);
    }

    // Byte is bank #, 8K/16K that goes into $A000/$8000. Any bank >=0x80 means RAM
    else if (bCartType == CART_ATRAX || bCartType == CART_MEGACART)
    {
        if (value >= 0x80)
            SwapRAMCart(candy, iBankSize, pb, iNumBanks); // RAM
        else
            SwapRAMCart(candy, iBankSize, pb, value % iNumBanks); // ATRAX uses bank $10 to mean 0, strip off the high bits
    }

    // The typical RAM cart behaviour - Address is bank #, 8K that goes into $A000. Bank == top + 1 is RAM
    else if (fRAMCart)
    {
        SwapRAMCart(candy, iBankSize, pb, iBank);
    }

    else
        Assert(0);  // what did I miss?

}

// set the freq of a POKEY timer
void ResetPokeyTimer(void *candy, int irq)
{
    ULONG f[4], c[4];
    int pCLK, pCLK28, pCLK114;

    if (irq == 2)   // not implemented in 800
        return;

    Assert(irq == 0 || irq == 1 || irq == 3);

    pCLK = fPAL ? PAL_CLK : NTSC_CLK;
    pCLK28 = pCLK / 28;
    pCLK114 = pCLK / 114;
    
    // f = how many cycles do we have to count down from? (Might be joined to another channel)
    // c = What is the clock frequency? If 2 is joined to 1, 2 gets 1's clock (and 4 gets 3's)
    // Then set now how many cycles have to execute before reaching 0

    // STIMER will call us for all 4 voices in order, so the math will work out.

    // 64K and 15K clocks have an extra count (N+1) before firing, just like the audio frequency will actually be divided by N+1

    if (irq == 0)
    {
        f[0] = AUDF1 + ((AUDCTL & 0x40) ? 0 : 1);
        c[0] = (AUDCTL & 0x40) ? pCLK : ((AUDCTL & 0x01) ? pCLK114 : pCLK28);
        irqPokey[0] = (LONG)((ULONGLONG)f[0] * pCLK / c[0]);
    }
    else if (irq == 1)
    {
        f[1] = ((AUDCTL & 0x10) ? (AUDF2 << 8) + AUDF1 : AUDF2) + ((AUDCTL & 0x40) ? 0 : 1);
        c[0] = (AUDCTL & 0x40) ? pCLK : ((AUDCTL & 0x01) ? pCLK114 : pCLK28);
        c[1] = (AUDCTL & 0x10) ? c[0] : ((AUDCTL & 0x01) ? pCLK114 : pCLK28);
        irqPokey[1] = (LONG)((ULONGLONG)f[1] * pCLK / c[1]);
    }
    else if (irq == 3)
    {
        f[3] = ((AUDCTL & 0x08) ? (AUDF4 << 8) + AUDF3 : AUDF4) + ((AUDCTL & 0x20) ? 0 : 1);
        c[2] = (AUDCTL & 0x20) ? pCLK : ((AUDCTL & 0x01) ? pCLK114 : pCLK28);    // irq 3 needs to know what this is
        c[3] = (AUDCTL & 0x08) ? c[2] : ((AUDCTL & 0x01) ? pCLK114 : pCLK28);
        irqPokey[3] = (LONG)((ULONGLONG)f[3] * pCLK / c[3]);
    }

    // 0 means not being used, so the minimum value for wanting an interrupt is 1
    // !!! will anything besides an IRQ ever run if we want >1 per scan line?
    if (f[irq] > 0 && irqPokey[irq] == 0)
            irqPokey[irq] = 1;

    //if (irqPokey[irq]) ODS("TIMER %d f=%d c=%d WAIT=%d instr\n", irq + 1, f[irq], c[irq], irqPokey[irq]);

}

// Add a byte to the IKBD input buffer
void AddToPacket(void *candy, ULONG b)
{
    //ODS("%02x ", b);
    pvmin->rgbKeybuf[pvmin->keyhead++] = (BYTE)b;
    pvmin->keyhead &= 1023;

#if defined(ATARIST) || defined(SOFTMAC)
    vsthw[v.iVM].gpip &= 0xEF; // to indicate characters available
#endif // ATARIST
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
BOOL __cdecl InstallAtari(void **ppPrivate, int *pPrivateSize, PVM pGem, PVMINFO pvmi, int type)
{
    if (!ppPrivate || !pPrivateSize || !pGem || !pvmi)
        return FALSE;

#if USE_PEEK_TABLE

    // set up the function tables
    for (int i = 0; i < 256; i++)
    {
        // This is the initial setup. BountyBob cart will modify this to bank select

        if (i == 0xd0 || i == 0xd2 || i == 0xd3 || i == 0xd4)
            read_tab[i] = PeekBAtariHW;    // hw registers
        else if (i == 0xd5)
            read_tab[i] = PeekBAtariBS;    // cartridge bank select
        else
            read_tab[i] = cpuPeekB;
    }
#endif

#if USE_POKE_TABLE

    for (int i = 0; i < 256; i++)
    {
        // This is the initial setup. Ramtop being set will set up $8000-$c000
        // So will Bounty Bob cartridge for bank selecting.
        // So will ANTIC to look for modifying the current screen RAM

        if (i >= 0xc0 && i < 0xd0)
            write_tab[i] = PokeBAtariOS;   // OS ROM may be banked out
        else if (i >= 0xd0 && i < 0xd5)
            write_tab[i] = PokeBAtariHW;   // HW registers
        else if (i == 0xd5)
            write_tab[i] = PokeBAtariBS;   // cartridge line for bank select
        else if (i >= 0xd6 && i < 0xd8)
            write_tab[i] = PokeBAtariHW; // PokeBAtariNULL; // !!! $d680 binary loader hack
        else if (i >= 0xd8)
            write_tab[i] = PokeBAtariOS;   // OS ROM may be banked out
        else if (i < 0x80)
            write_tab[i] = cpuPokeB;       // always RAM
    }
#endif

#if !USE_PEEK_TABLE || !USE_POKE_TABLE

    // tell the 6502 which HW it is running this time (will be necessary when we support multiple 6502 platforms)
    // !!! Once we mix VMs we'll have to do this more often to switch between them
    cpuInit(PeekBAtari, PokeBAtari);

#endif

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

    WORD initramtop = 0;

    if (pvmi == (PVMINFO)VM_CRASHED)
    {
        // Drag/Drop initially tries 800 w/o BASIC. If 800 breaks, try XL w/o BASIC. If XL breaks, try XE w/ BASIC.
        //
        // Some apps (BountyBob v3) don't run on XE, only XL, and usually an XE only app prompts you that you need 128K
        // and doesn't crash. Plus an XE is twice the memory footprint, you can fit twice almsot as many XL VMs

        if (((PVMINST)(pGem + 1))->fKillMePlease == 3) // our special kill code for "need XE w/o BASIC"
        {
            type = mdXE;
            initramtop = 0xc000;
        }
        else if (((PVMINST)(pGem + 1))->fKillMePlease == 4) // our special kill code for "need BASIC"
        {
            // keep the same type
            initramtop = 0xa000;
        }
        else if (type == md800)
        {
            type = mdXL;
            initramtop = 0xc000;
        }
        else if (type == mdXL)
        {
            type = mdXE;
            initramtop = 0xa000;
        }
        // otherwise, if you manually put an 800 only app into an XL or XE VM it reboots infinitely never finding an 800 VM
        else
        {
            type = md800;
            initramtop = 0xc000;
        }
    }

    // These things change depending on the machine we're emulating

    switch (1 << type)
    {
    case vmAtari48:
        pGem->bfHW = vmAtari48;
        pGem->iOS = 0;
        pGem->bfRAM = ram48K;
        strcpy(pGem->szModel, rgszVM[1]);
        break;

    case vmAtariXL:
        pGem->bfHW = vmAtariXL;
        pGem->iOS = 1;
        pGem->bfRAM = ram64K;
        strcpy(pGem->szModel, rgszVM[2]);
        break;

    case vmAtariXE:
        pGem->bfHW = vmAtariXE;
        pGem->iOS = 2;
        pGem->bfRAM = ram128K;
        strcpy(pGem->szModel, rgszVM[3]);
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

    int candysize = sizeof(CANDYHW);
    if (pGem->bfHW != vmAtari48)
        candysize += (SELF_SIZE + C000_SIZE + BASIC_SIZE + D800_SIZE);    // extended XL/XE RAM
    if (pGem->bfHW == vmAtariXE)
        candysize += (16384 * 4);            // extended XE RAM

    // Quicker than malloc and this is in a loop in slow load-up code, the fewer calls the better
    // Return our private data to GEM
    *ppPrivate = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, candysize + sizeof(CANDY_UNPERSISTED));

    if (*ppPrivate)
        ((CANDYHW *)(*ppPrivate))->m_dwCandySize = candysize;
    else
        return FALSE;

    CANDYHW *candy = *ppPrivate;

    // return our persistable size to GEM
    *pPrivateSize = candysize;
    
    // Remember GEM's persisted and unpersisted data about us, we'll need it
    pvm = pGem;
    pvmin = (PVMINST)(pGem + 1);

    AlterRamtop(*ppPrivate, initramtop);    // if we needed to set this to something in particular

    // init breakpoint. JMP $0 happens, so the only safe address that won't ever execute is $FFFF
    bp = 0xffff;

    // candy must be set up first (above)
    return TimeTravelInit(*ppPrivate);
}

// all done
//
BOOL __cdecl UnInstallAtari(void *candy)
{
    if (candy)
    {
        TimeTravelFree(candy);

        // GEM is responsible for knowing not to use it anymore
    
        HeapFree(GetProcessHeap(), 0, candy);
    }

    return TRUE;
}

BOOL __cdecl UnmountAtariDisks(void *candy)
{
    int i;

    for (i = 0; i < pvm->ivdMac; i++)
        UnmountAtariDisk(candy, i);

    return TRUE;
}

// Initialize a VM. Either call this to create a default new instance and then ColdBoot it,
//or call LoadState to restore a previous one, not both.
//
BOOL __cdecl InitAtari(void *candy)
{
    // These things are the same for each machine type

    pvm->fCPUAuto = TRUE;
    pvm->bfCPU = cpu6502;
    pvm->ivdMac = MAX_DRIVES; // sizeof(rgvm[0].rgvd) / sizeof(VD);    // although we have room for 8, and others have 9

    pvm->bfMon = monColrTV;

    // by default, use XL and XE built in BASIC, but no BASIC for Atari 800 unless Shift-F10 changes it
    // Install may have a preference and set this already, in which case, don't change it
    if (!ramtop)
        AlterRamtop(candy, (pvm->bfHW > vmAtari48) ? 0xA000 : 0xC000);
    
    switch (pvm->bfHW)
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

    pvm->bfRAM = BfFromWfI(pvm->pvmi->wfRAM, mdXLXE);

    if (!FInitSerialPort(pvm->iCOM))
        pvm->iCOM = 0;
    if (!InitPrinter(pvm->iLPT))
        pvm->iLPT = 0;

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
BOOL __cdecl UninitAtari(void *candy)
{
    // if there's no cartridge in, make sure the RAM footprint doesn't look like there is
    // (This is faster than clearing all memory). If it's replaced by a built-in BASIC cartridge,
    // ramtop won't change and it won't clear automatically on cold boot.
    if (candy)
    {
        rgbMem[0x9ffc] = 1;    // no R cart
        rgbMem[0xbffc] = 1;    // no L cart
        rgbMem[0xbffd] = 0;    // no special R cartridge

        // free the cartridge
        if (rgbSwapCart)
            free(rgbSwapCart);
        rgbSwapCart = NULL;

        UnmountAtariDisks(candy);
    }

    if (rgPasteBuffer)
        free(rgPasteBuffer);
    rgPasteBuffer = NULL;

    return TRUE;
}

// Get or Set the write protect status
//
BOOL __cdecl WriteProtectAtariDisk(void *candy, int i, BOOL fSet, BOOL fWP)
{

    if (!fSet)
        pvm->rgvd[i].mdWP = (BYTE)GetWriteProtectDrive(candy, i);
    else
        pvm->rgvd[i].mdWP = (BYTE)SetWriteProtectDrive(candy, i, fWP); // may or may not actually change

    return pvm->rgvd[i].mdWP;
}

BOOL __cdecl MountAtariDisk(void *candy, int i)
{
    UnmountAtariDisk(candy, i);    // make sure all emulator types know to do this first

    PVD pvd = &pvm->rgvd[i];
    BOOL f;
    if (pvd->dt == DISK_IMAGE)
    {
        f = AddDrive(candy, i, (BYTE *)pvd->sz);
        SetWriteProtectDrive(candy, i, pvm->rgvd[i].mdWP);   // VM remembers state of write protect
    }
    else if (pvd->dt == DISK_NONE)
        f = TRUE;
    else
    // I don't recognize this kind of disk... yet
        f =  FALSE;
    
    return f;
}

// spin the motors back up, our VMs will be needed soon
BOOL __cdecl InitAtariDisks(void *candy)
{
    candy;
    // let the motors spin themselves up if we actually use the drive, otherwise save lots of time doing nothing yet!
    return TRUE;
}

// spin the motors back down, our VM won't be needed for a bit
BOOL __cdecl UninitAtariDisks(void *candy)
{
    for (int i = 0; i < pvm->ivdMac; i++)
        CloseDrive(candy, i);
        
    return TRUE;
}

BOOL __cdecl MountAtariDisks(void *candy)
{
    int i;
    BOOL f;

    for (i = 0; i < pvm->ivdMac; i++)
    {
        f = MountAtariDisk(candy, i);
        if (!f)
            return FALSE;
    }
    return TRUE;
}

BOOL __cdecl UnmountAtariDisk(void *candy, int i)
{
    PVD pvd = &pvm->rgvd[i];

    if (pvd->dt == DISK_IMAGE)
        DeleteDrive(candy, i);

    return TRUE;
}

// PIA reset - this is a lot of stuff for only having 4 registers
//
void ResetPIA(void *candy)
{
    // XL/XE - swap banks back to default, this might be a warm start so don't lose data
    if (mdXLXE != md800)
    {
        // this will trigger the swap code to safely swap back to the default banks, keeping the data in the other banks
        PBCTL = 0;  // PORTB registers are DIRECTIONS 
        PokeBAtari(candy, PORTAea + 1, 0);    // all bits INPUT
    }

    // but it also might be a coldstart, and ramtop might have changed, so 
    rPADATA = 255;    // the data PORTA will provide if it is in READ mode - defaults to no joysticks moved
    rPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000 && !pvm->rgcart.fCartIn) ? 253 : 255) : 255; // XL: HELP out, BASIC = ??, OS IN
    
    wPADATA = 0;    // the data written to PORTA to be provided in WRITE mode (must default to 0 for Caverns of Mars)
    wPBDATA = (mdXLXE != md800) ? ((ramtop == 0xA000 && !pvm->rgcart.fCartIn) ? 253 : 255) : 0; // XL defaults to OS IN and HELP OUT

    wPADDIR = 0;    // direction - default to read
    wPBDDIR = 0;

    PORTA = rPADATA; // since they default to read mode
    PORTB = rPBDATA;

    PACTL = 0x0; // was 0x3c;    // I/O mode, not SET DIR mode
    PBCTL = 0x0; // was 0x3c;    // I/O mode, not SET DIR mode    
}

// soft reset
//
BOOL __cdecl WarmbootAtari(void *candy)
{
    //ODS("\n\nWARM START\n\n");

    // warm/cold start is the only way to stop a long pasting operation
    if (cPasteBuffer)
    {
        cPasteBuffer = 0;
        if (rgPasteBuffer)
            free(rgPasteBuffer);
        rgPasteBuffer = NULL;
        fBrakes = fBrakesSave;  // go back to old turbo mode
    }

    // notice NTSC/PAL switch
    fPAL = pvm->fEmuPAL;
    PAL = fPAL ? 1 : 15;    // set GTIA register
    fInVBI = 0;
    fInDLI = 0;

    // on XL/XE, PIA is reset on warm start too (eg, to swap the OS back in)
    if (mdXLXE != md800)
        ResetPIA(candy);

    // run the warm start interrupt
    NMIST = 0x20 | 0x1F;
    regPC = cpuPeekW(candy, (mdXLXE != md800) ? 0xFFFC : 0xFFFA);
    cntTick = 255;    // delay for banner messages

    //fBrakes = TRUE;    // do not go back to real time, auto-switching tiles keep disrupting turbo mode
    clockMult = 1;    // NYI
    wFrame = 0;
    wScan = 0;    // start at top of screen again
    wLeft = 0;
    wCycle = 0;
    PSL = 0;
    wSIORTS = 0;    // stop avoiding printing in the monitor
    pvm->fTimeTravelFixed = FALSE;   // start periodically saving again
    fCartNeedsSwap = FALSE; // or else we'll fault trying to swap a bogus bank in

    // SIO init
    cSEROUT = 0;
    fSERIN = FALSE;
    isectorPos = 0;
    fWant8 = 0;
    fWant10 = 0;

    // unused new starting positions of PMGs must be huge, since 0 and some negative numbers are valid locations
    for (BYTE i = 0; i < 4; i++)
        pmg.hpospPixNewStart[i] = 512;  // !!! should be NTSCx, we can't access that constant

    // A self-rebooting tile that isn't ours must not init the joysticks while we are moving a joystick
    // or we'll think it's a paddle. 

    if (v.iVM >= 0 && candy == rgpvmi(v.iVM)->pPrivate)
        InitJoysticks();    // let somebody hot plug a joystick in and it will work the next warm boot!

    return TRUE;
}

// Cold Start the machine - the first one is currently done when it first becomes the active instance
//
BOOL __cdecl ColdbootAtari(void *candy)
{
    //ODS("\n\nCOLD START\n\n");

    // !!! why is this needed for some apps? (PAL: Joe Blade #1, Kick off, One Man Droid, Uridium, etc.)
    // initialize hardware
    // !!! old NOTE: at $D5FE is ramtop and then the jump table, not still true?
    // do it first before PIA registers are set to non-$ff in Warmboot
    WORD addr;
    for (addr = 0xD000; addr < 0xD5FE; addr++)
    {
        cpuPokeB(candy, addr, 0xFF);
    }

    // first do the warm start stuff
    if (!WarmbootAtari(candy))
        return FALSE;

    // 800 resets PIA on cold start only
    if (mdXLXE == md800)
        ResetPIA(candy);

    // Do NOT mount the drives yet, that opens files and takes forever and slows app startup
    fDrivesNeedMounting = TRUE;

    //printf("ColdStart: mdXLXE = %d, ramtop = %04X\n", mdXLXE, ramtop);

#if 0 // doesn't change
    if (mdXLXE == md800)
        pvm->bfHW = /* (ramtop == 0xA000) ? vmAtari48C :*/ vmAtari48;
    else if (mdXLXE == mdXL)
        pvm->bfHW = /* (ramtop == 0xA000) ? vmAtariXLC :*/ vmAtariXL;
    else
        pvm->bfHW = /* (ramtop == 0xA000) ? vmAtariXEC :*/ vmAtariXE;
#endif

    //DisplayStatus(); this might not be the active instance

    // load the OS, erase all extra RAM
    InitBanks(candy);

    // Load the proper initial bank back into the cartridge, for cartridges with a RAM bank, make sure it isn't in RAM mode
    // or it will just erase everything and go to memo pad.
    // Cartridges must be read into memory now, not delayed, to secure the memory they need, it will fail and kill the VM
    // if we do it later when memory is full
    ReadCart(candy, TRUE);
    InitCart(candy);

    // reset the registers
    cpuReset(candy);

    // XL's use hardware assist to cold start, pure emulation can only warm start.
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

    NMIST = 0x20 | 0x1F;    // the one in WARMSTART might get undone from the above clearing?

    // CTIA/GTIA reset

    // we want to remove BASIC, on an XL the only way is to fake OPTION being held down.
    // but now it will get stuck down! ExecuteAtari will fix that
    CONSOL = ((mdXLXE != md800) && (GetKeyState(VK_F9) < 0 || ramtop == 0xC000)) ? 3 : 7;
    PAL = fPAL ? 1 : 15;   // perhaps should be 0 and 14, but some apps need to see 1 and 15
    TRIG0 = 1;
    TRIG1 = 1;
    TRIG2 = 1;
    // XL cartridge sense line, without this warm starts can try to run an non-existent cartridge
    TRIG3 = (mdXLXE != md800 && !(pvm->rgcart.fCartIn)) ? 0 : 1;
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

    //POT0 = 228;   // now we actually scan and do it for real!
    //POT1 = 228;
    //POT2 = 228;
    //POT3 = 228;
    //POT4 = 228;
    //POT5 = 228;
    //POT6 = 228;
    //POT7 = 228;
    
    SKSTAT = 0xFF;
    IRQST = 0xFF;   // !!! shouldn't warm start do some of this?
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
    return TimeTravelReset(candy);
}

// SAVE: our data is being persisted. Do anything you need to do to make it persistable. In our case, that's putting
// RAM back in that was swapped out to non-persistable cartridge memory, so its contents don't get lost
//
BOOL __cdecl SaveStateAtari(void *candy)
{

    // !!! Time Travel periodically does this and it is SLOW! (when a RAM cart is being used)
    // Before saving, we need to put the RAM that's swapped into cartridge memory back into RAM so it will be persisted
    // keep iSwapCart to tell us which bank should be swapped back in, so don't do this twice in a row!
    // if BASIC is swapped in to a RAM cart, then ramtop is low, but that still means that no swapping is needed
    if (pvm->rgcart.fCartIn && fRAMCart && iSwapCart < iNumBanks && !fCartNeedsSwap)
    {
        BYTE swap[8192];
        _fmemcpy(swap, &rgbMem[0xa000], 8192);
        _fmemcpy(&rgbMem[0xA000], rgbSwapCart + iSwapCart * 8192, 8192);
        _fmemcpy(rgbSwapCart + iSwapCart * 8192, swap, 8192);
        AlterRamtop(candy, 0xc000);
        fCartNeedsSwap = TRUE;  // next Execute, swap it back (assumes caller is done with it by then)
    }
        
    return TRUE;
}

// LOAD: copy from the data we've been given. Make sure it's the right size
// Either INIT is called followed by ColdBoot, or this, not both, so do what we need to do to restore our state
// without resetting anything
//
BOOL __cdecl LoadStateAtari(void *pPersist, void *candy, int cbPersist)
{
    if (cbPersist != dwCandySize || candy == NULL || pPersist == NULL)
        return FALSE;

    // copy the new persistable state over, leaving the unpersisted stuff after it untouched
    _fmemcpy(candy, pPersist, cbPersist);

    // Now fix our non-persistable data based on new persistable data

    // 1. rgDrives info about the attached drives, now delayed for speed savings
    //BOOL f = MountAtariDisks(candy);
    fDrivesNeedMounting = TRUE;

    // 2. If we were in the middle of reading a sector through SIO, restore that data
    if (rgSIO[0] >= 0x31 && rgSIO[0] <= 0x34 && rgSIO[1] == 0x52)
        SIOReadSector(candy, rgSIO[0] - 0x31);

    // 3. If our saved state had a cartridge, load it back in, but do not reset to original bank
    ReadCart(candy, FALSE);
    InitCart(candy);

#if 0
    // legacy stuff that doesn't matter for now
    if (!FInitSerialPort(pvm->iCOM))
        pvm->iCOM = 0;
    if (!InitPrinter(pvm->iLPT))
        pvm->iLPT = 0;
#endif

    // 4. Our two paths to creating a VM, cold start or LoadState, both need to reset time travel to create an anchor point
    return TimeTravelReset(candy); // state is now a valid anchor point
}


BOOL __cdecl DumpHWAtari(void *candy)
{
    candy;
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
       AUDCTL, cpuPeekB(candy, 0x87), cpuPeekB(candy, 0x88), cpuPeekB(candy, 0xA2));

#endif // DEBUG
    return TRUE;
}

BOOL __cdecl TraceAtari(void *candy, BOOL fStep, BOOL fCont)
{
    fTrace = TRUE;
    ExecuteAtari(candy, fStep, fCont);
    fTrace = FALSE;

    return TRUE;
}

// The big loop! Process an entire frame of execution and build the screen buffer, or, if fTrace, only do a single instruction.
//
// !!! fStep and fCont are ignored for now! Not needed, GEM uses a different tracing system now
BOOL __cdecl ExecuteAtari(void *candy, BOOL fStep, BOOL fCont)
 {
    fCont; fStep;

    WORD MAXY = fPAL ? PAL_LPF : NTSC_LPF; // 312 or 262 lines per frame?

    BOOL fStop = 0;    // do not break out of big loop

    // we delayed this so the app could start up quickly, since it doesn't alloc additional memory, we *can* delay it
    if (fDrivesNeedMounting)
    {
        int rt = ramtop;
        MountAtariDisks(candy); // failure is no big deal, just means the file can't be opened, not even in R/O
        if (ramtop != rt)
        {
            // In the case where we load a .BAS file, we try to set ramtop to 0xa000, which can't be done anymore
            // since we delayed the mounting, so we just need a reboot with BASIC.
            Assert(ramtop == 0xa000);
            KillMePleaseBASIC(candy);
        }
        fDrivesNeedMounting = FALSE;
    }

    // to persist, we had to temporarily put the RAM saved in the cartridge space into memory that is persisted. Put it back.
    if (fCartNeedsSwap && ramtop == 0xc000)
    {
        BYTE swap[8192];
        _fmemcpy(swap, &rgbMem[0xa000], 8192);
        _fmemcpy(&rgbMem[0xa000], rgbSwapCart + iSwapCart * 8192, 8192);
        _fmemcpy(rgbSwapCart + iSwapCart * 8192, swap, 8192);
        AlterRamtop(candy, 0xa000);
        fCartNeedsSwap = FALSE;
    }
    else
        fCartNeedsSwap = FALSE; // somebody forgot to reset this!

    do {

        // When tracing, we only do 1 instruction per call, so there might be some left over - go straight to doing more instructions.
        // Since we have to finish a 6502 instruction, we might do too many cycles and end up < 0. Don't let errors propagate.
        // Add wLeft back in to the next round.
        if (wLeft <= 0)
        {

#ifndef NDEBUG
            // Display scan line here
            if (fDumpHW) {
                WORD PCt = cpuPeekW(candy, 0x200);
                extern void CchDisAsm(void *, WORD *);
                int i;

                printf("\n\nscan = %d, DLPC = %04X, %02X\n", wScan, DLPC, cpuPeekB(candy, DLPC));
                for (i = 0; i < 60; i++)
                {
                    CchDisAsm(candy, &PCt);
                    printf("\n");
                }
                PCt = regPC;
                for (i = 0; i < 60; i++)
                {
                    CchDisAsm(candy, &PCt);
                    printf("\n");
                }
                FDumpHWVM(0);   // !!! Don't have VM# we need
            }
#endif // DEBUG

            // if we faked the OPTION key being held down so OSXL would remove BASIC, now it's time to lift it up
			// (unless the joystick wants it pressed right now)
            if (wFrame > 20 && !(CONSOL & 4) && GetKeyState(VK_F9) >= 0 && !fJoyCONSOL)
                CONSOL |= 4;

            // IRQ's, like key presses, SIO or POKEY timers. VBI and DLIST interrupts are NMI's.
            // !!! Only scan line granularity.
            // We clear bits in IRQST when we want the interrupt to happen and the interrupt is enabled. If I is set,
            // they won't happen, but some apps poll IRQST with an IRQ enabled in IRQEN but with SEI disabled to see
            // when the events happen. When I is cleared, if they haven't reset the IRQ by POKE IRQEN, the interrupt will happen, delayed.
            // !!! I need to reset IRQST $8 for apps to see (DEATHCHASE XE hangs) even if the IRQ is not enabled, so that
            // one needs to have a check for being enabled
            if (!(regP & IBIT)) // registers are packed right now since we are not inside a call to Go6502
            {
                // a low bit in IRQST means an interrupt wants to trigger and we already checked that it was enabled in IRQEN,
                // except for $8.
                if (((IRQST & (BYTE)~0x08) != (BYTE)~0x08) || (!(IRQST & (BYTE)0x08) && (IRQEN & (BYTE)0x08)))
                {
                    Interrupt(candy, FALSE);
                    regPC = cpuPeekW(candy, 0xFFFE);
                    
                    //ODS("IRQ %02x TIME! @%03x\n", (BYTE)~(IRQST), wScan);

                    // check if the interrupt vector is our breakpoint, otherwise we would never notice and not hit it
                    if (regPC == bp)
                        fHitBP = TRUE;

                }
            }

            // Prepare ProcessScanLine for a new scan line, and figure out everything we need to know to understand
            // how ANTIC will steal cycles on a scan line like this
            PSLPrepare(candy);

            // table element HCLOCKS has the starting value of wLeft for this kind of scan line (0-based)
            // subtract any extra cycles we spent last time finishing an instruction
            wLeft += (DMAMAP[HCLOCKS] + 1);
            wCycle = DMAMAP[wLeft - 1];
            
            // If we'll need a DLI or a VBI at cycle 10, tell it the value of wLeft that needs the DLI (+ 1 since it's 0-based)
            wNMI = (((sl.modehi & 8) && (iscan == scans || (fWait & 0x08))) || (wScan == STARTSCAN + Y8)) ? DMAMAP[116] + 1 : 0;

            // We delayed a POKE to something that needed to wait until the next scan line, so add the 4 cycles
            // back that we would have lost. We know it's safe to make wLeft bigger than 114 because it will get back to being 
            // <114 as soon as the POKE happens.
            if (fRedoPoke)
            {
                wLeft += 4;
                wCycle = DMAMAP[wLeft - 1];
                fRedoPoke = FALSE;
            }

            // Scan line 0-7 are not visible
            // Scan lines 8-247 are 240 visible lines, ANTIC DMA is possible, depending on a lot of things
            // Scan line 248 is the VBLANK
            // Scan lines 249-261 are the rest of the overscan lines

            if (wScan == STARTSCAN)
            {
                // process the ATARI keyboard buffer. Don't look at the shift key live if we're pasting, we're handling it
                // we can't process keys only during a VB because that nests interrupts and breaks MULE pause
                CheckKey(candy, !(cPasteBuffer), wLiveShift);
            }

            if (wScan == STARTSCAN + Y8)
            {
                // Joysticks, light pen and pasting are processed just before the VBI, many games process joysticks in a VBI
                DoVBI(candy);
            }

            // We're supposed to start executing at the WSYNC release point (cycle 105) not the beginning of the scan line
            // That point is stored in index 115 of this array, and + 1 because the index is 0-based
            // OR we want an NMI on this scan line, and it's already past the point for it! (The last instruction on the previous
            // scan line went over and then DMA happened and there you go). If so, Go6502 won't trigger the NMI, so we have to do it now

            if (WSYNC_Waiting || wLeft <= wNMI)
            {
                //ODS("WAITING\n");

                // Is there an NMI on this upcoming scan line that will be skipped by a WSYNC?
                BOOL fNMI = WSYNC_Waiting && wNMI && wNMI < wLeft;

                // we need to delay the next thing that executes (regular or NMI code) until the WSYNC point
                if (WSYNC_Waiting)
                {
                    
                    // + 1 to make it 1-based
                    // + 1 is to restart early at cycle 104
                    // !!! That's not always true, but usually is, especially if the WSYNC happened at the end of the previous scan line,
                    // so I'm pretty sure in this case we should resume at 104, unless the previous instruction was INC WSYNC in which
                    // case after the WSYNC is touched at cycle 5, the instruction still has another cycle left to do, so the next
                    // instruction can't start early at 104
                    if (rgbMem[regPC - 3] == 0xee)
                        wLeft = DMAMAP[115] + 1;
                    else
                        wLeft = DMAMAP[115] + 1 + 1;

                    wCycle = DMAMAP[wLeft - 1];

                    WSYNC_Waiting = FALSE;

                    // the NMI would trigger right away since we are jumping past it. But in real life,
                    // the next instruction after the STA WSYNC is allowed to execute first, so move the NMI
                    // to one cycle from now so one more instruction will execute and then the NMI will fire
                    if (fNMI)
                        wNMI = wLeft - 1;
                }
                
                // Uh oh, an NMI was to happen at cycle 10, and we're already there or past it, so trigger it now
                // unless we've arranged to trigger it soon because now is too soon (fNMI)
                if (!fNMI && wScan == STARTSCAN + Y8)
                {
                    // clear DLI, set VBI, leave RST alone - even if we're not taking the interrupt
                    NMIST = (NMIST & 0x20) | 0x5F;

                    // VBI enabled, generate VBI by setting PC to VBI routine. When it's done we'll go back to what we were doing before.
                    if (NMIEN & 0x40) {
                        //ODS("special VBI\n");
                        Interrupt(candy, FALSE);
                        regPC = cpuPeekW(candy, 0xFFFA);
                        
                        // We're still in the last VBI? Must be a PAL app that's spoiled by how long these can be
                        if (fInVBI)
                            SwitchToPAL(candy);
                        fInVBI++;

                        wLeft -= 7; // 7 CPU cycles are wasted internally setting up the interrupt, so it will start @~17, not 10
                        wCycle = wLeft > 0 ? DMAMAP[wLeft - 1] : 0xff;   // wLeft could be 0 if the NMI was delayed due to WSYNC

                        if (regPC == bp)
                            fHitBP = TRUE;
                    }
                }

                else if (!fNMI && (sl.modehi & 8) && (iscan == scans || (fWait & 0x08)))
                {
                    // set DLI, clear VBI leave RST alone - even if we don't take the interrupt
                    NMIST = (NMIST & 0x20) | 0x9F;
                    if (NMIEN & 0x80)    // DLI enabled
                    {
                        //ODS("special DLI\n");
                        Interrupt(candy, FALSE);
                        regPC = cpuPeekW(candy, 0xFFFA);

                        // We're still in the last VBI? And we're not one of those post-VBI DLI's?
                        // Must be a PAL app that's spoiled by how long these can be
                        if (fInVBI && wScan >= STARTSCAN && wScan < STARTSCAN + Y8)
                        {
                            // this happened last frame too, we're PAL. NTSC programs occasionally do this, like MULE every
                            // 4 frames, but never consecutively like a PAL program would.
                            if (fDLIinVBI == wFrame - 1)
                                SwitchToPAL(candy);
                            fDLIinVBI = wFrame;
                        }
                        fInDLI++;

                        wLeft -= 7; // 7 CPU cycles are wasted internally setting up the interrupt, so it will start @~17, not 10
                        wCycle = wLeft > 0 ? DMAMAP[wLeft - 1] : 0xff;   // wLeft could be 0 if the NMI was delayed due to WSYNC

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
            Go6502(candy);
        }

        // hit a breakpoint during execution
        if (regPC == bp)
            fStop = TRUE;

        Assert(wLeft <= 0 || fTrace == 1 || regPC == bp);

        // we finished the scan line
        if (wLeft <= 0)
        {
            // now finish off any part of the scan line that didn't get drawn during execution
            ProcessScanLine(candy);

            wScan = wScan + 1;

            // increment the POT counters once per scan line until they hit the actual paddle value, in which case say we are done
            POT++;
            if (POT > 228) POT = 228;
            for (int i = 0; i < 8; i++)
            {
                if (POT < rgbMem[PADDLE0 + i])
                    rgbMem[POT0 + i] = POT;
                else
                {
                    rgbMem[POT0 + i] = rgbMem[PADDLE0 + i];
                    ALLPOT &= ~(1 << i);    // say this pot is done
                }
            }

            // A data byte was written, so we want to assert SEROUT has been freed up after the approximate amount of time
            // has passed. It's now OK to write to SEROUT again. Don't decrement past 0.
            // If the IRQ is not enabled at the moment it happens, it is missed
            if (fWant10 && (--fWant10 == 0) && (IRQEN & 0x10))
            {
                //ODS("TRIGGER - SEROUT NEEDED @%03x\n", wScan);
                IRQST &= ~0x10;
            }

            // If another byte is not written within another few scan lines, we are starving.
            // They either completed a frame and stopped sending, or are just slow.
            // This means not only is SEROUT finished being read into the output shift register, but the shift register is empty too.
            // If we're starving because they finished sending a frame, let's process that frame. We'll trigger the interrupt next.
            if (fWant8 && (--fWant8 == 0) && cSEROUT == 5)  // 5 bytes sent, and suitable time has passed
            {
                // we have a complete frame, that's why we're starving, let's give them some data back!
                cSEROUT = 0;

                // !!! Lots of bare bones SIO stuff is NYI
                // Anything except read D1 sector # and D1 status (D2-D4, format, write, etc.) - see SIOV hack
                // Technically, SKSTAT bit 1 should go low 90% of the time to signal the input shift register is busy.
                // SKSTAT bit 7 framing error never happens in emulation, except possibily protected disks?
                // SKSTAT bits 5-6 serial and keyboard overrun errors NYI
                // SKSTAT bit 4 raw data read NYI (for detecting cassette motor speed)

                isectorPos = 0;

                // !!! 2-drive support only, the drive in question must be mounted, or we ignore the request and let it time out
                if (rgSIO[0] >= 0x31 && rgSIO[0] <= 0x32 && pvm->rgvd[rgSIO[0] - 0x31].sz[0])
                {
                    if (rgSIO[1] == 0x52)
                    {
                        //ODS("DISK READ REQUEST sector %d @%03x\n", rgSIO[2] | ((int)rgSIO[3] << 8), wScan);
                        SERIN = 0xff;    // flag to say we haven't reported any data back yet
                        // First delay is extra long for spin-up, Flight Simulator II hangs waiting < 20
                        fSERIN = (wScan + SIO_DELAY * 2);    // waiting less than this hangs apps who aren't ready for the data
                        if (fSERIN >= MAXY)
                            fSERIN -= (MAXY - 1);    // never be 0, that means stop
                    }

                    else if (rgSIO[1] == 0x53)
                    {
                        //ODS("DISK STATUS %02x %02x @%03x\n", rgSIO[2], rgSIO[3], wScan);
                        SERIN = 0xff;    // flag to say we haven't reported any data back yet
                        fSERIN = (wScan + SIO_DELAY);    // waiting less than this hangs apps who aren't ready for the data
                        if (fSERIN >= MAXY)
                            fSERIN -= (MAXY - 1);    // never be 0, that means stop
                    }

                    // since there is a drive to respond, it should respond with NAK
                    else
                    {
                        ODS("UNSUPPORTED SIO command %02x %02x\n", rgSIO[0], rgSIO[1]);
                        SERIN = 0xff;    // flag to say we haven't reported any data back yet
                        fSERIN = (wScan + SIO_DELAY);    // waiting less than this hangs apps who aren't ready for the data
                        if (fSERIN >= MAXY)
                            fSERIN -= (MAXY - 1);    // never be 0, that means stop
                    }
                }
                
                else
                    ODS("UNSUPPORTED SIO command %02x %02x\n", rgSIO[0], rgSIO[1]);
                
            }
        
            // Unlike other interrupts, this interrupt triggers whenever the output shift register is idle and the interrupt is enabled,
            // you don't miss it if you didn't have the interrupt enabled at the exact right moment.
            // !!! Apps need to see the IRQST bit reset even if the IRQ is not enabled, so my trigger an IRQ code, above, is hacked.
            if (!fWant8) // DEATHCHASE XE hangs && (IRQEN & 0x08))  // the output shift register is not busy
            {
                //ODS("MAYBE TRIGGER - SEROUT COMPLETE @%03x\n", wScan);
                IRQST &= ~0x08;
            }

            // SIO - periodically send the next data byte, too fast breaks them
            if (wScan == fSERIN)    
            {
                Assert(wScan);  // wScan should not be able to be 0 at this point

                fSERIN = (wScan + SIO_DELAY);    // you have this long to read it before the next bit goes in its place
                if (fSERIN >= MAXY)
                    fSERIN -= (MAXY - 1);    // never be 0, that means stop
                
                // They want the interrupt for this, let's give it to them
                if (IRQEN & 0x20)
                {
                    //ODS("TRIGGER SERIN @%03x\n", wScan);
                    IRQST &= ~0x20;
                }

                // !!! Technically, SKSTAT bit 5 (overrun) should be set if the $20 interrupt is enabled and was already low
                // in IRQST before we set it low above 

                // Now set SERIN to the next input byte. They can read it as many times as they want, but it has to be inside
                // the correct time window or they'll miss it and another byte will replace it

                // remember, the data in the sector can be the same as the ack, complete or checksum

                // status responses are 4 bytes long, sector reads are 128
                BYTE iLastSector = (rgSIO[1] == 0x52) ? 128 : 4;

                if (isectorPos > 0 && isectorPos < iLastSector)
                {
                    SERIN = sectorSIO[isectorPos];
                    //ODS("SERIN: DATA 0x%02x = 0x%02x\n", isectorPos, SERIN);
                    isectorPos++;
                }

                else if (isectorPos == iLastSector)
                {
                    SERIN = checksum;
                    //ODS("SERIN: CHECKSUM = 0x%02x\n", SERIN);
                    fSERIN = FALSE;
                    isectorPos = 0;
                }

                else if (SERIN == 0xff && (rgSIO[1] == 0x52 || rgSIO[1] == 0x53))    // nothing sent yet
                {
                    SERIN = 0x41;    // ack
                    //ODS("SERIN: ACK @%03x\n", wScan);
                }

                // we don't support this yet, so send NAK
                else if (SERIN == 0xff)
                {
                    SERIN = 0x4e;    // NAK
                    fSERIN = FALSE;  // all done
                }

                else if (SERIN == 0x41)    // ack
                {
                    SERIN = 0x43;    // complete
                    //ODS("SERIN: COMPLETE\n");
                }

                else if (SERIN == 0x43)
                {
                    if (rgSIO[1] == 0x52)
                    {
                        checksum = SIOReadSector(candy, rgSIO[0] - 0x31);
                    }
                    
                    // this is how I think you properly respond to a status request, I hope
                    else if (rgSIO[1] == 0x53)
                    {
                        WORD dv = rgSIO[0] - 0x31;
                        BOOL sd, ed, dd, wp;
                        SIOGetInfo(candy, dv, &sd, &ed, &dd, &wp, NULL);
                        
                        /* b7 = enhanced   b5 = DD/SD  b4 = motor on   b3 = write prot */
                        sectorSIO[0] = ((ed ? 128 : 0) + (dd ? 32 : 0) + (wp ? 8 : 0));;

                        sectorSIO[1] = 0xff;
                        sectorSIO[2] = 0xe0;
                        sectorSIO[3] = 0x00;

                        checksum = 0xe0 + sectorSIO[0];
                        if (sectorSIO[0] > 0x1f)
                            checksum++; // add the carry
                    }

                    SERIN = sectorSIO[0];
                    isectorPos = 1;    // next byte will be this one
                    //ODS("SERIN: DATA 0x%02x = 0x%02x\n", 0, SERIN);
                }

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
                        //ODS("TIMER %d FIRING: %03x %02x\n", irq + 1, wScan, wCycle);
                        IRQST &= ~(irq + 1);                // fire away
                    }

                    LONG isav = irqPokey[irq];                // remember

                    ResetPokeyTimer(candy, irq);            // start it up again, they keep cycling forever

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
        if (wScan >= MAXY)
        {
            TimeTravelPrepare(candy, FALSE);        // periodically call this to enable TimeTravel

            // we can only do actual 50Hz if we're not tiling
            SoundDoneCallback(candy, (fPAL && !v.fTiling) ? SAMPLES_PAL : SAMPLES_NTSC);    // finish this buffer and send it

            // notice NTSC/PAL switch on a frame boundary only
            fPAL = pvm->fEmuPAL;
            PAL = fPAL ? 1 : 15;    // set GTIA register
            if (fPAL)
                fSwitchingToPAL = FALSE;

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

BOOL __cdecl DumpRegsAtari(void *candy)
{
    candy;

    // later

    return TRUE;
}


BOOL __cdecl DisasmAtari(void *candy, char *pch, ADDR *pPC)
{
    candy;  pPC; pch;

    // later

    return TRUE;
}

// we are running in the wrong type of VM, schedule a reboot of a different type
void KillMePlease(void *candy)
{
    if (v.fAutoKill)
    {
        vi.fExecuting = FALSE;  // WRONG VM! alert the main thread something is up

        pvmin->fKillMePlease = TRUE;   // say which thread died

        // quit the thread as early as possible
        wLeft = 0;  // exit the Go6502 loop
        bp = regPC; // don't do additional scan lines                    }
    }
}

// we need to switch binary loaders, so a reboot is required.
void KillMeSoftlyPlease(void *candy)
{
    // don't just call KIL, cuz it does nothing if we're not auto-killing bad VMs
    vi.fExecuting = FALSE;  // alert the main thread something is up
 
    // quit the thread as early as possible
    wLeft = 0;  // exit the Go6502 loop
    bp = regPC; // don't do additional scan lines

    pvmin->fKillMePlease = 2;   // say which thread died, with special code meaning "kill me softly" (coldboot only)
}

// we specifically notice we need to be an XE, probably without BASIC (not normally in our cycle (800 w/o --> XL w/o --> XE w/BASIC)
void KillMePleaseXE(void *candy)
{
    if (v.fAutoKill)
    {
        vi.fExecuting = FALSE;  // WRONG VM! alert the main thread something is up

        pvmin->fKillMePlease = 3;   // say which thread died, special code for XE w/o BASIC

        // quit the thread as early as possible
        wLeft = 0;  // exit the Go6502 loop
        bp = regPC; // don't do additional scan lines                    }
    }
}

// we specifically want BASIC swapped into the same VM type we are now
void KillMePleaseBASIC(void *candy)
{
    if (v.fAutoKill)
    {
        vi.fExecuting = FALSE;  // WRONG VM! alert the main thread something is up

        pvmin->fKillMePlease = 4;   // say which thread died, special code for BASIC

        // quit the thread as early as possible
        wLeft = 0;  // exit the Go6502 loop
        bp = regPC; // don't do additional scan lines                    }
    }
}

//
// here are our various PEEK routines, based on address
//

// This is how Bounty Bob bank selects
BYTE __forceinline __fastcall PeekBAtariBB(void *candy, ADDR addr)
{
    Assert(bCartType == CART_BOB);

    if (addr >= 0x8ff6 && addr <= 0x8ff9)
        BankCart(candy, 0, (BYTE)(addr - 0x8ff6));
    if (addr >= 0x9ff6 && addr <= 0x9ff9)
        BankCart(candy, 1, (BYTE)(addr - 0x9ff6));

    return cpuPeekB(candy, addr);
}

// This is how other cartridges bank select
BYTE __forceinline __fastcall PeekBAtariBS(void *candy, ADDR addr)
{
    Assert((addr & 0xff00) == 0xd500);

    // some don't bank when reading, only writing !!! Which ones?
    if (bCartType != CART_ATRAX)
        BankCart(candy, addr & 0xff, 0);    // cartridge banking
    
    return TRUE;
}

// this is for shadowing the HW registers at $d0, $d2, $d3 and $d4
// !!! Why all the noinline? Let's be consistent
__declspec(noinline)
BYTE __forceinline __fastcall PeekBAtariHW(void *candy, ADDR addr)
{
    // implement SHADOW or MIRROR registers instantly yet quickly without any memcpy!

    switch ((addr >> 8) & 255)
    {
    default:
        Assert(FALSE);  // help the compiler
        break;

        // floating data bus... on the 800, you're supposed to get the last effective address put on the bus in the $cxxx range,
        // which I at least fake up by returning a random number for apps that hang if it's always the same # (Highway Duel)
    case 0xc0:
        if (mdXLXE == md800)
            return PeekBAtari(candy, 0xd20a);
        break;

    case 0xd0:
        addr &= 0xff1f;    // GTIA has 32 registers
        
        if (addr < 0xd010)
            ProcessScanLine(candy);   // reading collision registers needs cycle accuracy
        
        // (XL) TRIG3 == 1 for cartridge present, which makes a lot of paranoid disk games deliberately hang
        // on 800, TRIG3 is always 1 unless P4 presses the trigger, and these apps always hang (TurboBasic, Barroom Brawl)
        // User code reading this within 1sec or so of boot is suspiciously not actually looking for a joystick reading (OS reads it during boot)
        else if (addr == 0xd013)
        {
            if (mdXLXE == md800 && wFrame < 100 && regPC < ramtop)
                KillMePlease(candy);
        }
        
        // NTSC/PAL GTIA flag
        else if (addr == 0xd014)
        {
            // deliberately hang on NTSC machines? Not only switch to PAL, but lie and say that we already are!
            // LDA PAL, AND #$e, BEQ $fe
            // LDA PAL, EOR #$ff (Bounty Bob)
            if ((rgbMem[regPC + 2] == 0xd0 && rgbMem[regPC + 3] == 0xfe) || (rgbMem[regPC] == 0x49 && rgbMem[regPC + 1] == 0xff))
            {
                SwitchToPAL(candy);
                return 0x0;
            }
        }
        break;
    
    case 0xd2:
        addr &= 0xff0f;    // POKEY has 16 registers

        // RANDOM and its shadows
        // we've been asked for a random number. How many times would the poly counter have advanced? (same clock freq as CPU)
        if (addr == 0xD20A) {
            int cur = (wFrame * (fPAL ? PAL_LPF : NTSC_LPF) * HCLOCKS + wScan * HCLOCKS + DMAMAP[wLeft - 1]);
            int delta = (int)(cur - random17last);
            random17last = cur;
            random17pos = (random17pos + delta) % 0x1ffff;
            rgbMem[addr] = poly17[random17pos];
        }

        //else if (addr == 0xd20d)
            //ODS("SERIN READ @%03x\n", wScan);

        //else if (addr == 0xd20e && regPC < 0xa000)
        //    ODS("IRQST READ @%03x\n", wScan);

        break;

    case 0xd3:
        addr &= 0xff03;    // PIA has 4 registers
        break;

    case 0xd4:
        addr &= 0xff0f;    // ANTIC has 16 registers (some say shadowed to $D5 too in a way?)

        // VCOUNT - by clock 111 VCOUNT increments
        // DMAMAP[115] + 1 is the WSYNC point (cycle 105). VCOUNT increments 6 cycles later. LDA VCOUNT is a 4 cycle instruction.
        if (addr == 0xD40B)
        {
            // uh oh, somebody is comparing VCOUNT to a really big number, we're supposed to be a PAL machine
            // CMP VCOUNT (A = big)
            if (rgbMem[regPC - 3] == 0xcd && regA > 0x83)
                SwitchToPAL(candy);

            // uh oh #2... LDA VCOUNT CMP #big
            else if (rgbMem[regPC] == 0xc9 && rgbMem[regPC + 1] > 0x83)
                SwitchToPAL(candy);

            // some XL apps (Operation Blood) wait for VCOUNT == $7f because that's when the XL VBI exits,
            // but the 800 VBI is longer and exits at $80 and hangs. Help it out.
            else if (mdXLXE == md800 && rgbMem[regPC + 1] == 0x7f)
                rgbMem[regPC + 1] = 0x80;

            // if the last cycle of this 4-cycle instruction ends AT the point where VCOUNT is incremented (111), it still sees the old value
            // - 1 for 0-based. - 3 for the last cycle. + 1 to see what the next cycle is (which might be blocked, so the next instruction
            // could start arbitrarily further than the end of the last instruction).
            if (wLeft >= 4 && DMAMAP[wLeft - 1 - 3] + 1 <= 111)
                return (BYTE)(wScan >> 1);
            // report scan line NTSC: 262 (131) or PAL: 312 (156) for only 1 cycle, then start reporting 0 again
            else if (wScan == (fPAL ? PAL_LPF - 1 : NTSC_LPF - 1) && wLeft <= 5)
                return 0;
            else
                return (BYTE)((wScan + 1) >> 1);
        }

        break;
    }

    // return the actual register no matter what shadow they accessed
    return cpuPeekB(candy, addr);
}

// the non-jump table version of the code just switches the proper routine
BYTE __forceinline __fastcall PeekBAtari(void *candy, ADDR addr)
{
    switch (addr >> 8)
    {
    default:
        return cpuPeekB(candy, addr);

    case 0xc0:
    case 0xd0:
    case 0xd2:
    case 0xd3:
    case 0xd4:
        return PeekBAtariHW(candy, addr);

    case 0xd5:
        return PeekBAtariBS(candy, addr);

    case 0x8f:
    case 0x9f:
        if (bCartType == CART_BOB)
            return PeekBAtariBB(candy, addr);
        return cpuPeekB(candy, addr);
    }
}

// for some reason the monitor can't call the __fastcall __forceinline version
BYTE PeekBAtariMON(void *candy, ADDR addr)
{
    return PeekBAtari(candy, addr);
}

// currently not used, and can't be
//
WORD __cdecl PeekWAtari(void *candy, ADDR addr)
{
    candy; addr;
    return 0;
}

// not used
ULONG __cdecl PeekLAtari(void *candy, ADDR addr)
{
    candy; addr;
    return 0;
}

// not used
BOOL __cdecl PokeLAtari(void *candy, ADDR addr, ULONG w)
{
    candy; addr; w;
    return TRUE;
}

// not used, and can never be
//
BOOL __cdecl PokeWAtari(void *candy, ADDR addr, WORD w)
{
    candy; addr; w;
    return TRUE;
}

//
// Here are the various POKE functions depending on the address space, that we directly jump to to avoid branching tests on the address
//

#if 0   // !!! Why can't this live here?
__declspec(noinline)
BOOL __fastcall PokeBAtariDL(void *candy, ADDR addr, BYTE b)
{
    // we are writing into the line of screen RAM that we are current displaying
    // handle that with cycle accuracy instead of scan line accuracy or the wrong thing is drawn (Turmoil)
    // most display lists are in himem so do the >= check first, the test most likely to fail and not require additional tests
    Assert(addr < ramtop);
    if (addr < ramtop && addr >= wAddr && addr < (WORD)(wAddr + cbWidth) && rgbMem[addr] != b)
        ProcessScanLine(candy);

    cpuPokeB(candy, addr, b);
}
#endif

BOOL __fastcall PokeBAtariBB(void *candy, ADDR addr, BYTE b)
{
    Assert(bCartType == CART_BOB);
    b;

    // This is how Bounty Bob bank selects
    if (addr >= 0x8ff6 && addr <= 0x8ff9)
        BankCart(candy, 0, (BYTE)(addr - 0x8ff6));
    if (addr >= 0x9ff6 && addr <= 0x9ff9)
        BankCart(candy, 1, (BYTE)(addr - 0x9ff6));
    
    return TRUE;
}

BOOL __fastcall PokeBAtariOS(void *candy, ADDR addr, BYTE b)
{
    // the OS is swapped out so allow writes to those locations (the OS is swapped out when the OS bit is in write direction mode and
    // the last thing written to it was 0 to swap it out)
    // !!! make this one test for perf
    if (mdXLXE != md800 && !(wPBDATA & 1) && (wPBDDIR & 1))
    {
        // write to XL/XE RAM under ROM
        Assert((addr >= 0xc000 && addr < 0xD000) || addr >= 0xD800);

        return cpuPokeB(candy, addr, b);
    }
    return TRUE;
}

// dead cartridge space before $c000
BOOL __fastcall PokeBAtariNULL(void *candy, ADDR addr, BYTE b)
{
    candy; addr; b;
    return TRUE;
}

// cartridge line for bank select
BOOL __fastcall PokeBAtariBS(void *candy, ADDR addr, BYTE b)
{
    //printf("addr = %04X, b = %02X\n", addr, b);
    BankCart(candy, addr & 0xff, b);
    
    return TRUE;
}

// used for $d0 - $d4
__declspec(noinline)
BOOL __forceinline __fastcall PokeBAtariHW(void *candy, ADDR addr, BYTE b)
{
    BYTE bOld;
    Assert(addr < 65536);

    //printf("write: addr:%04X, b:%02X, PC:%04X\n", addr, b & 255, regPC - 1); // PC is one too high when we're called

    switch (addr >> 8)
    {
    default:
        Assert(FALSE);  // help the compiler
        break;

    case 0xD0:      // GTIA

        addr &= 31;
        bOld = rgbMem[writeGTIA + addr];

        // CYCLE COUNTING - Writes to GTIA should take effect IMMEDIATELY so process the scan line up to where the electron beam is right now
        // before we change the values. The next time its called will be with the new values.
        // HITCLR needs cycle accuracy too, to get all collisions in the past processed before clearing
        // Don't waste time if this isn't changing the value
        if ((addr < 0x1d && bOld !=b) || addr == 30)
        {
            //ODS("GTIA %04x=%02x at VCOUNT=%02x clock=%02x\n", addr, b, PeekBAtari(candy, 0xd40b), DMAMAP[wLeft - 1]);
            ProcessScanLine(candy);    // !!! should anything else instantly affect the screen?
        }

        rgbMem[writeGTIA+addr] = b;

        // When you turn GRACTL off, you continue to use the most recent data
        if (addr == 0x1d)   // GRACTL
        {
            // should be no need to ProcessScanLine since we are specifically preventing something from changing

            if ((bOld & 1) && !(b & 1))   // (blue bar glitches - Decathlon needs to continue to use 0, Pitfall needs to continue with $c0)
                GRAFM = pmg.grafm;
            if ((bOld & 2) && !(b & 2))
                GRAFPX = pmg.grafpX;
            if (b & 4)
                ODS("ATTEMPT TO LATCH TRIGGERS!\n");    // !!! NYI
        }
        else if (addr == 30)
        {
            // HITCLR

            *(ULONG *)MXPF = 0;
            *(ULONG *)MXPL = 0;
            *(ULONG *)PXPF = 0;
            *(ULONG *)PXPL = 0;

            //We are supposed to start detecting collisions again right away
            //pmg.fHitclr = 1;
            //ODS("HITCLR! - %04x %04x %02x\n", wFrame, wScan, wLeft);
        }
        else if (addr == 31)
        {
            // !!! Supposed to RESET when 8 bit is set, to ~(bits 4,2,1)
            // Therefore poking with 8 resets to no buttons pressed

            // !!! REVIEW: click the speaker

            // printf("CONSOL %02X %02X\n", bOld, b);

                if ((bOld ^ b) & 8)
                {
                // toggle speaker state (mask out lowest bit?)
                }
        }
        break;

    case 0xD1:  // XL CIO R: hack, Darek ???
#if 0   // this breaks apps like Arena who need to see the C: handler, not the R: swapped in

        if (mdXLXE != md800 && addr == 0xD1FF)
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
#endif
        break;

    case 0xD2:      // POKEY
        addr &= 15;

        rgbMem[writePOKEY+addr] = b;

        // only non-zero values start the timers, OS init code does not
        if (addr == 9)
        {
            // STIMER - grab the new frequency of all the POKEY timers, even if they're disabled. They might be enabled by time 0.
            // I believe, technically, poking here should reset any timer that is partly counted down.
            for (int irq = 0; irq < 4; irq++)
            {
                // Halftime Battlin' Bands hangs unless the initial OS pokes of 0 do trigger timer interrupts when they're enabled
                // so clearly the docs that say this should only start the timers for non-zero values are wrong.
                ResetPokeyTimer(candy, irq);
            }
        }
        if (addr == 10)
        {
            // SKRES

            SKSTAT |=0xE0;
        }
        else if (addr == 11)
        {
            // POTGO - start counting once per scan line from 0 to 228, which will be the paddle values
            // ALLPOT bits get reset when the counting is done
            POT = 0;
            ALLPOT = 0xff;
        }
        else if (addr == 13)
        {
            // most known apps call this with PBCTL == 0x34, Astromeda uses 0x30
            // ACID test calls us with PBCTL == $3c. I need to avoid OS boot code that writes 0 or FF being interpreted as valid.
            // But I can't only let data I recognize through. 221B sends device #$c3 status and hangs if the interrupts don't 
            // fire that show we sent that command. Other apps try to talk to the printer, etc.
            // !!! The right thing to do is probably a timeout of some sort? This can hang apps who write here with PBCTL == $3c
            if (PBCTL == 0x34 || PBCTL == 0x30)
            {
                Assert(cSEROUT < 5);
                //ODS("SEROUT #%d = 0x%02x @%03x\n", cSEROUT, b, wScan);
                rgSIO[cSEROUT] = b;
                cSEROUT++;

                // we pretend that sending one byte takes more than 1 full scan line. Any faster is a faster baud rate than
                // some apps can handle and they hang not expected it to finish so quickly
                fWant10 = 2;    // !!! I wait 2, and my beep rate sounds about right

                // Add three extra scan lines until we are starving. This is the shift register clear interrupt
                // so if they don't send another byte in time after the $10 interrupt is ready for data, we are "starving"
                fWant8 = 5;     // !!! I wait 3 additional
                IRQST |= 0x08;  // we're not starving anymore
            }
        }

        else if (addr == 14)
        {
            // IRQEN - IRQST is the complement of the enabled IRQ's, and says which interrupts are active

            // all the bits they poked OFF (to disable an INT) have to show up here as ON, except for $8
            IRQST |= (~b & (BYTE)~0x08);
            
            //ODS("IRQEN: 0x%02x @%03x\n", b, wScan);

            // !!! All of the bits they poke ON have to show up instantly OFF in IRQST, but that's not how I do it right now.
            // I don't reset the bit until the interrupt is ready to fire, I treat it as meaning it's triggered not just enabled.

        }

        // SKCTL !!!
        // SKCTL bit 7 force serial to output 0 NYI
        // SKCTL bits 5 & 6 serial clock NYI
        // SKCTL bit 4 asynch receive NYI fully
        // SKCTL bit 3 two tone mode NYI
        // SKCTL bit 2 fast pot scan NYI
        // SKCTL bits 0 & 1 always assumed on to enable keyboard
        // SKCTL resetting the poly counters to a known value also NYI

        else if (addr == 15)
        {
            // asynch receive mode. Timers 3 and 4 are suspended until a start bit is received
            // !!! I don't resume them when SIO receives data or when this bit is cleared, should I?
            if (b & 0x10)
            {
                // Cosmic Balance hangs if entering async receive mode doesn't kill timer 4
                irqPokey[2] = 0;
                irqPokey[3] = 0;
            }
        }

        else if (addr <= 8)
        {
            if (addr == 8 && (b & 6))
                ODS("HIGH PASS FILTER NYI!\n");

            // AUDFx, AUDCx or AUDCTL have changed - write some sound
            // we're (wScan / 262) of the way through the scan lines and the DMA map tells us our horiz. clock cycle
            // we can only do 50Hz if we're not tiling, otherwise use NTSC timing
            int SAMPLES_PER_VOICE = (fPAL && !v.fTiling) ? SAMPLES_PAL : SAMPLES_NTSC;
            int iCurSample = (wScan * 100 + DMAMAP[wLeft - 1] * 100 / HCLOCKS) * SAMPLES_PER_VOICE / 100 /
                        ((fPAL && !v.fTiling) ? PAL_LPF : NTSC_LPF);
            if (iCurSample < SAMPLES_PER_VOICE)
                SoundDoneCallback(candy, iCurSample);

            // reset an active timer (irqPokey will be non-zero) that had its frequency changed
            for (int irq = 0; irq < 4; irq++)
            {
                if (irqPokey[irq] && addr == 8 || addr == (ADDR)(irq << 1))
                    ResetPokeyTimer(candy, irq);
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
        // Apparently, if we write something to a PORT while it's in input mode,
        // it is of course ignored, but the next time we put it into output mode
        // it is supposed to remember what we previously tried to write to it (ACID PIA_BASIC)

        if (addr & 2)
        {
            // it is a control register

            cpuPokeB(candy, PACTLea + (addr & 1), b & 0x3C);
        }
        else
        {
            // it is either a data or ddir register. Check the control bit

            if (cpuPeekB(candy, PACTLea + addr) & 4)
            {
                // it is a data register. Update only bits that are marked as write.

                BYTE bMask = cpuPeekB(candy, wPADDIRea + addr);   // which bits are in write mode
        
                // what is the banking state right now? read bits have the default bank, and write bits have the last data written
                bOld = (rPBDATA & ~wPBDDIR) | (wPBDATA & wPBDDIR);

                BYTE bNew  = (b & bMask) | (bOld & ~bMask);     // use new data for those in write mode, keep old data for those that aren't

                // We do want to remember what was poked here, even in read mode, so that when we switch to write mode, it will
                // automatically put this data in. (Acid test PIA Basic)
                cpuPokeB(candy, wPADATAea + addr, b);

                if (bOld != bNew)
                {
                    if (addr == 1)
                    {
                        if (mdXLXE != md800)
                        {

                            // Writing a new value to PORT B on the XL/XE.

                            if (mdXLXE != mdXE)
                            {
                                // This is an XE bank switch to a non-main bank, we're in the wrong VM!
                                // !!! Not ideal, we'll switch to 130XE w/BASIC, hopefully the app is smart enough to swap that out
                                // (Fortress Underground is smart enough)
                                // 8f/8e swaps out bit 6, which is undefined, so doesn't sound like a real XE bank request
                                // Some Bounty Bob hang in XE so we can't go into XE mode by mistake if they don't mean it
                                if ((bNew & 0x30) != 0x30 && bNew != 0x8f && bNew != 0x8e)
                                    KillMePleaseXE(candy);

                                // in 800XL mode, mask bank select bits
                                bNew |= 0x7C;
                                wPBDATA |= 0x7C;
                            }

                            SwapMem(candy, bOld ^ bNew, bNew);
                        }
                        else if (b && !(b & 1)) // hopefully poking with 0 is init code and not meant to swap out the OS
                        {
                            KillMePlease(candy);
                        }
                    }
                }

                // PORT B bit 0 being used to attempt to swap out the OS on an 800? Or swap in an extended XE bank?
                // Unfortunately, the XL OS boot sequence leaves PORTB in write mode, and the 800 OS obviously doesn't,
                // so I can't actually tell if somebody is trying to swap out the OS, I have to assume.
                // (They won't always put PORTB in write mode explicitly)
                // I at least protect against clearing memory with zeros thinking that's an attempt to swap out the OS
                // by insisting b be non-zero
                // 
                else if (addr == 1 && mdXLXE == md800 && b && (!(b & 1) || ((b & 0x30) != 0x30)))
                {
                    // 8f/8e swaps out bit 6, which is undefined, so doesn't sound like a real XE bank request
                    // Some Bounty Bob hang in XE so we can't go into XE mode by mistake if they don't mean it
                    if ((b & 0x30) != 0x30 && b != 0x8f && b != 0x8e)    
                        KillMePleaseXE(candy);    // need XE w/o BASIC
                    else
                        KillMePlease(candy);      // need XL w/o BASIC
                }
            }
            else
            {
                // it is a data direction register.

                // Apparently, putting PORTB bits into read mode swaps those banks back to default, so that clearing PIA with 0s
                // won't swap out the OS accidentally. First POKE PORTB,0 does swap out the OS. Then POKE PBCTL,0 puts the
                // ports into direction control mode. Then the POKE to the next PORTB shadow sets PORTB to read mode, and
                // that needs to swap the OS back in (Great American Road Race, some versions)
                
                // The default banks are found in rPBDATA, set at power up and never changed - HELP out, OS IN, BASIC ???)
                // The bits marked for writing get whatever bank they were last told to use (wPBDATA)
                // Therefore, putting all PORTB bits into read mode swaps in the default banks, and putting them back into write mode
                // restores whatever was there before by seeing if wPBDATA is different than rPBDATA for that bit
                // 
                if (addr == 1 && mdXLXE != md800)
                {
                    // b contains which bits are in write mode
                    // what is the banking state right now? read bits have the default bank, and write bits have the last data written
                    bOld = (rPBDATA & ~wPBDDIR) | (wPBDATA & wPBDDIR);

                    // use the default for bits in read mode; the last write value for those in write mode.
                    BYTE bNew = (rPBDATA & ~b) | (wPBDATA & b); // (MPT24SPL.XEX)
                    if (bNew != bOld)
                        SwapMem(candy, bOld ^ bNew, bNew);      // this crashes if we tell it to swap in something already there
                }

                // now it's safe to poke in the new value
                cpuPokeB(candy, wPADDIRea + addr, b);

            }
        }

        // update PORTA and PORTB read registers

        UpdatePorts(candy);
        break;

    case 0xD4:      // ANTIC
        addr &= 15;

        // CYCLE COUNTING - Writes to CHBASE should take effect IMMEDIATELY (BD BeefDrop) so process the scan line up to where
        // the electron beam is right now before we change the values. The next time its called will be with the new values.
        // !!! CHACTL, etc, too?
        if (addr == 9)
        {
            //ODS("GTIA %04x=%02x at VCOUNT=%02x clock=%02x\n", addr, b, PeekBAtari(candy, 0xd40b), DMAMAP[wLeft - 1]);
            ProcessScanLine(candy);    // !!! should anything else instantly affect the screen?
        }

        // Uh-oh, we're changing something with scan line granularity that shouldn't happen until after the next scan line is set up
        // since the STA abs won't have finished by the end of this line (4 cycle instruction) and we execute at the beginning of the
        // instruction instead of the end like we're supposed to. Skip doing it now and back up to do it again.
        // Without this Bump Pong is 2x too tall
        else if (addr < 9 && rgbMem[regPC - 1] == 0xd4 && wLeft < 4)
        {
            regPC -= 3; // !!! Support indirect and 2-byte instructions that access ANTIC too
            fRedoPoke = TRUE;   // add 4 cycles to the next scan line so the timing isn't changed
            break;
        }

        // now it's safe to alter the values

        rgbMem[writeANTIC+addr] = b;

        // the display list pointer is the only ANTIC register that is R/W
        if (addr == 2 || addr == 3)
        {
            rgbMem[0xd400 + addr] = b;

            // #1 problem in PAL apps... they think they have forever in the VBI (50 extra scan lines, with no DMA stealing)
            // and they don't get around to letting the OS copy the DLIST shadows until it's too late, past scan 8 of the next line
            // which resets back to the top of the DLIST and jitters.
            // Sometimes, the first thing they do after a VBI is set this, so we might not be in the VBI, but we just left it.
            if ((fInVBI || wScan == wScanVBIEnd || wScan == wScanVBIEnd + 1) && wScan >= STARTSCAN && wScan < STARTSCAN + Y8)
                SwitchToPAL(candy);

            //ODS("DLPC = %04x @ %d\n", DLPC, wScan);
        }

        // Do NOT reload the DL to the top when DMA fetch is turned on, that turned out to be disastrous

        if (addr == 10)
        {
            // WSYNC
            // The beam is 5 cycles behind ANTIC. It fetchs the first NARROW non-character byte at cycle 28.
            // It fetches the first character at 26, and the first char lookup at 29. Subsequent lines only do the
            // char lookup starting at 29. So it doesn't have the data it needs to draw at 25 until 30. 5 behind.
            // Also, ANTIC stop fetching at 106. 5 before that is 101, the end of the visibile screen. Coincidence?
            // 5 cycles is 20 pixels.
            //
            // Worm War and the HARDB Chess board are two of the most cycle precise apps I know of.
            // Worm War changes HPOSP3 while it's being drawn and it must continue with the old parameters.
            // HARDB/CHESS seems to change HPOSP3 after it's completed drawing in the wrong place! If I lag 20 pixels, this
            // is corrected.
            //
            // The snow near the beginning of HARDB is a great cycle precision test, but doesn't use WSYNC/VCOUNT.
            //
            // Decathlon's line 41 DLI spends enough time before its STA WSYNC that it needs to miss the line 41 WSYNC point
            //
            // Pitfall 2 scan line $6d needs to start early (104) on line $6c, and it writes to WSYNC again barely in time
            // to make the deadline.
            // Annoyingly alternatively, Tarzan needs to resume at 105 from WSYNC, to see VCOUNT increment in time or the
            // splash screen is garbled. So I can't just pick one or the other
            //
            // -1 to make it 0 based. Look at the cycle the write happens on (4 or 5) of this 4 or 6 cycle instruction.
            // When that cycle finishes is where the store will actually be complete.
            // (If the cycle after it finished is blocked, the next instruction won't start
            // for a while, so where the next instruction starts is irrelevant). The WSYNC is in time if it's complete
            // by the start of 104 (not being complete until the start of 105, the WSYNC point itself, it too late).
            //
            BYTE cT = 4;    // assumed STA WSYNC is 4 cycles
            BYTE cW = 4;    // cycle that the write happens on
            if (rgbMem[regPC - 3] == 0xee)
            {
                cT = 6;     // INC WSYNC is 6 cycles
                cW = 5;     // and the write happens on cycle 5
            }

            // Sometimes WSYNC resumes at cycle 105 instead of 104, if the cycle after STA WSYNC is blocked,
            // or if playfield DMA or RAM refresh DMA uses cycle 104. It's difficult to know for sure, but
            // if we're drawing in the middle (narrow) section of a character mode right now, chances are ANTIC is busy and
            // a STA WSYNC won't resume until cycle 105. At least it works for Tarzan.
            short dma = DMAMAP[wLeft > 4 ? (wLeft - 1 - 3) : 1];
            short cclock = rgPIXELMap[dma + 1] - 20;
            short wo = 1;
            if (cclock > 48 && cclock < 352 - 48 && sl.modelo > 1 && sl.modelo < 8)
                wo = 0;
            // INC WSYNC touches WSYNC on cycle 5 of a 6 cycle instruction and cycle 6 executes next for free instead
            // of the first cycle of the next instruction, so we never start early at 104 after an INC WSYNC
            if (cT > cW)
                wo = 0;

            // -1 to make it 0 based, cW - 1 to get the beginning of the write cycle. Plus one to get the end of that cycle
            if (wLeft >= cT && DMAMAP[wLeft - 1 - (cW - 1)] + 1 <= 104)
            {
                // Is there an NMI on this scan line that hasn't happened yet?
                BOOL fNMI = wNMI && wNMI < wLeft;

                // + 1 to make it 1-based
                // +cT is how many cycles are about to be decremented the moment we return
                // + wo is to start 1 cycle early at cycle 104 if wo == 1.
                // 
                wLeft = DMAMAP[115] + 1 + cT + wo;
                wCycle = DMAMAP[wLeft - 1];

                // the NMI would trigger right away since we are jumping past it. But in real life,
                // the next instruction after the STA WSYNC is allowed to execute first, so move the NMI
                // to one cycle from now so one more instruction will execute and then the NMI will fire
                // (cT cycles are about to be decremented)
                if (fNMI)
                    wNMI = wLeft - cT - 1;
            }
            else
            {
                wLeft = 0;               // stop right now and wait till next line's WSYNC
                wCycle = 0x0;
                WSYNC_Waiting = TRUE;    // next scan line only gets a few cycles
            }
        }
        else if (addr == 15)
        {
            // NMIRES - reset NMI status

            NMIST = 0x1F;
        }
        else if (addr == 5)
        {
            //ODS("VSCROL=%02x: %d %d\n", b, wScan, wCycle);

            // we're changing VSCROL in a vertical blank, we probably mean for it to happen before the next frame is drawn,
            // but it is too late since NTSC has far fewer overscan lines
            // Sometimes, the first thing they do after a VBI is set this, so we might not be in the VBI, but we just left it.
            if ((fInVBI || wScan == wScanVBIEnd) && wScan >= STARTSCAN && wScan < STARTSCAN + Y8)
                SwitchToPAL(candy);
        }
        break;

        case 0xd6:
        case 0xd7:
            // !!! self modifying binary loader hack at $d680
            // self modifying instruction, 800 OS loader, XL OS loader
            if (fAltBinLoader && ((addr == 0xd77d && regPC == 0xd7c6) || regPC == 0xf334 || regPC == 0xc5ef))
                rgbMem[addr] = b;
            break;
    }

    return TRUE;
}

// the non-jump table version just switches to the right function
BOOL __forceinline __fastcall PokeBAtari(void *candy, ADDR addr, BYTE b)
{
    BYTE ba = (BYTE)(addr >> 8);
    switch (ba)
    {
    default:
        if (addr < ramtop)
        {
            // writes to screen memory being drawn right now need to be trapped (Turmoil)
            if (addr >= wAddr && addr < (WORD)(wAddr + cbWidth) && rgbMem[addr] != b)
                ProcessScanLine(candy);
            return cpuPokeB(candy, addr, b);
        }
        else if (bCartType == CART_BOB && (ba == 0x8f || ba == 0x9f))
            return PokeBAtariBB(candy, addr, b);
        else
            return TRUE;

    case 0xc0:
    case 0xc1:
    case 0xc2:
    case 0xc3:
    case 0xc4:
    case 0xc5:
    case 0xc6:
    case 0xc7:
    case 0xc8:
    case 0xc9:
    case 0xca:
    case 0xcb:
    case 0xcc:
    case 0xcd:
    case 0xce:
    case 0xcf:
        return PokeBAtariOS(candy, addr, b);

    case 0xd0:
    case 0xd1:
    case 0xd2:
    case 0xd3:
    case 0xd4:
        return PokeBAtariHW(candy, addr, b);

    case 0xd5:
        return PokeBAtariBS(candy, addr, b);

    case 0xd6:
    case 0xd7:
        return PokeBAtariHW(candy, addr, b);  // was TRUE; $d680 binary loader hack

    case 0xd8:
    case 0xd9:
    case 0xda:
    case 0xdb:
    case 0xdc:
    case 0xdd:
    case 0xde:
    case 0xdf:
    case 0xe0:
    case 0xe1:
    case 0xe2:
    case 0xe3:
    case 0xe4:
    case 0xe5:
    case 0xe6:
    case 0xe7:
    case 0xe8:
    case 0xe9:
    case 0xea:
    case 0xeb:
    case 0xec:
    case 0xed:
    case 0xee:
    case 0xef:
    case 0xf0:
    case 0xf1:
    case 0xf2:
    case 0xf3:
    case 0xf4:
    case 0xf5:
    case 0xf6:
    case 0xf7:
    case 0xf8:
    case 0xf9:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfd:
    case 0xfe:
    case 0xff:
        return PokeBAtariOS(candy, addr, b);
    }
}

// for some reason the monitor can't call the __fastcall __forceinline version
BOOL PokeBAtariMON(void *candy, ADDR addr, BYTE b)
{
    return PokeBAtari(candy, addr, b);
}

#endif // XFORMER
