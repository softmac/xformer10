
/***************************************************************************

    ATARI800.H

    - Common include file for all Atari 800 VM components.
    - Defines all of the private variables used by the Atari 800 VM.

    Copyright (C) 1986-2021 by Darek Mihocka and Danny Miller. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

    11/30/2008  darekm      open source release
    12/07/1998  darekm      PC Xformer 3.8 release

***************************************************************************/

#pragma once

#include "common.h"

// !!! DO NOT DEFINE THESE! THE JUMP TABLES ARE BROKEN SINCE THE VM NO LONGER IS PASSED AN INDEX TO A SPARSE ARRAY
//
//#if defined(_M_ARM) || defined(_M_ARM64)
//// Use the switch table based interpreter for ARM/ARM64 builds as that utilizes the extra registers better
//#define USE_JUMP_TABLE (0)
//#else
//// Sorry, never use the jump table dispatch interpreter. X86 refuses to tail call. X64 is slower than no jump tables
//#define USE_JUMP_TABLE (0)
//#endif
//#define USE_PEEK_TABLE (0)
//#define USE_POKE_TABLE (0)
//
// !!! DO NOT DEFINE THESE! THE JUMP TABLES ARE BROKEN SINCE THE VM NO LONGER IS PASSED AN INDEX TO A SPARSE ARRAY

#define STARTSCAN 8    // scan line ANTIC starts drawing at

// ANTIC does 262 line non-interlaced NTSC video at true ~60fps, not 262.5 standard NTSC interlaced video.
// the TV is prevented from going in between scan lines on the next pass, but always overwrites the previous frame.
// I wonder if that ever created weird burn-in patterns.

extern BYTE rgbRainbow[];    // the ATARI colour palette

// all the reasons ANTIC might do DMA and block the CPU

#define DMA_M 1     // grab missile data if missile DMA is on
#define DMA_DL 2    // grab DList mode if Playfield DMA is on
#define DMA_P 3     // grab player data if player DMA is on
#define DMA_LMS 4   // do the load memory scan

#define W8 5        // wide playfield hi, med or lo res mode grab (on first or only line)
#define WC4 6       // grab char data, all lines of a character mode, hi or med res
#define W4 7        // wide playfield hi or med res modes
#define WC2 8       // character mode, hi res only
#define W2 9        // wide playfield, hi res mode only

#define N8 10       // same for wide or normal playfield (present in all but narrow)
#define NC4 11
#define N4 12
#define NC2 13
#define N2 14

#define A8 15       // same for wide, normal or narrow playfield
#define AC4 16
#define A4 17
#define AC2 18
#define A2 19

// Describes what ANTIC does for each cycle, see comment in atari800.c
extern const BYTE rgDMAC[HCLOCKS];
extern const BYTE rgDMANC[HCLOCKS];

extern const BYTE Bin1[128];
extern const BYTE Bin2[128];
extern const BYTE Bin3[128];

// all the possible variables affecting which cycles ANTIC will have the CPU blocked
//
// 19 modes
// Playfield DMA on or off?
// 3 playfield widths
// first scan line of a mode or not?
// player DMA on?
// missile DMA on?
// LMS (load memory scan) happening?
// # of CPU cycles left to execute on this scan line (wLeft)
//
// cycle number is 0-113 (0 based, representing what pixel is being drawn when wLeft is from 1-114).
// index 114 holds how many CPU cycles can execute this scan line (wLeft's initial value, 1-based, from 1-114)
// index 115 holds the WSYNC point (set wLeft to this + 1 when you want to jump to cycle 105)
// index 116 holds the DLI point (cycle 10), do an NMI when wLeft decrements to this
BYTE rgDMAMap[20][2][3][2][2][2][2][HCLOCKS + 3 + 11];

// for clock cycle number 0-113 (0 based), what pixel is about to be drawn at the start of that cycle #?
WORD rgPIXELMap[HCLOCKS];

// precompute PMG priorities
BYTE rgPMGMap[65536];

// this mess is how we properly index all of those arrays
// [mode number], which we've set to 0 for before and after visible scan area (<8 or >= 248)
// [PF DMA on?], plus must be in a valid range of scan lines or ANTIC won't do it anyway
// [0=narrow 1=normal 2=wide]
// [first scan line of this mode?], which we indicate by setting iScan == sl.vscrol
// [P DMA on?]
// [M DMA on?] which is automatically enabled if P DMA is enabled
// [LMS?] which happens if the right bit is set on valid modes 

#define DMAMAP rgDMAMap[sl.modelo][((DMACTL & 0x20) >> 5) && wScan >= STARTSCAN && wScan < STARTSCAN + Y8][(DMACTL & 0x03) ? ((DMACTL & 3) - 1) : 0][iscan == sl.vscrol][((DMACTL & 0x8) >> 3) && wScan >= STARTSCAN && wScan < STARTSCAN + Y8][(((DMACTL & 4) >> 2) | ((DMACTL & 8) >> 3)) && wScan >= STARTSCAN && wScan < STARTSCAN + Y8][((sl.modehi & 4) && sl.modelo >= 2) ? 1 : 0]

// !!! I ignore the fact that HSCROL delays the PF DMA by a variable number of clocks

// for testing, # of jiffies it takes a real 800 to do FOR Z=1 TO 1000 in these graphics modes (+16 to eliminate mode 2 parts):
//   88-89      125                 102 101     86  87  89      92          100         121     121
//   0          2 GR.0              6 GR.1/2    8               11 GR.6     13 GR.7     15      GTIA

// XE is non-zero when 130XE emulation is wanted
#define XE 1

// keep screen pixel co-ordinates unsigned, both to reduce compiler warnings about signed/unsigned comparisons
// and also to reduce code size when using a pixel offset to index an array
#define X8 352u       // screen width
#define Y8 240u       // screen height

#define CART_4K      1  // 0 for invalid
#define CART_8K      2
#define CART_16K     3
    // numbers above 3 are bank select types
#define CART_OSSA    4  // 034M
#define CART_OSSAX   5  // 043M version
#define CART_OSSB    6  // M091
#define CART_XEGS    7
#define CART_BOB     8
#define CART_ATARIMAX1  9
#define CART_ATARIMAX8  10
#define CART_ATRAX      11
#define CART_ATARIMAX1_OR_ATRAX 12
#define CART_ATARIMAX1L 13
#define CART_ATARIMAX8L 14
#define CART_XEGS_OR_ATARIMAX8L  15
#define CART_XEGS_OR_ATARIMAX1L  16
#define CART_MEGACART   17  // 16K through 1MB
#define CART_OSSBX      18  // 091M
#define CART_OSSBY      19  // 019M
#define CART_DIAMOND    20
#define CART_SPARTA     21

#define MAX_CART_SIZE 1048576 + 16 // 1MB cart with 16 byte header

#if USE_PEEK_TABLE
// quickly peek and poke to the right page w/o branching using jump tables
PFNREAD read_tab[MAX_VM][256];
#endif
#if USE_POKE_TABLE
PFNWRITE write_tab[MAX_VM][256];
#endif

// xsio.c stuff
typedef struct
{
    WORD mode;
    int  h;
    WORD fWP;
    WORD wSectorMac;
    WORD ofs;
    //    char *pbRAMdisk;
    char path[MAX_PATH];
    char name[12];
    ULONG cb;   // size of file when mode == MD_FILE*
} DRIVE;

#define MAX_DRIVES 2

// !!! This delay between byte reads seems to make beeps at the proper rate. The initial delay
// for the first byte is going to be twice as long (for spin-up). Spy vs Spy Arctic hangs if the doubled
// delay is < 13, and Flight Simulator II hangs if it's < 20
#define SIO_DELAY 13

// poly counters used for disortion and randomization (we only ever look at the low bit or byte at most)
// globals are OK as only 1 thread does sound at a time
// RANDOM will hopefully be helped by not being thread safe, as it will become even more random. :-)
BYTE poly4[(1 << 4) - 1];    // stores the sequence of the poly counter
BYTE poly5[(1 << 5) - 1];
BYTE poly9[(1 << 9) - 1];
BYTE poly17[(1 << 17) - 1];
int poly4pos[4], poly5pos[4], poly9pos[4], poly17pos[4];     // each voice keeps track of its own poly position
unsigned int random17pos;    // needs to be unsigned
ULONGLONG random17last;    // cycle count last time a random number was asked for

//
// Scan line structure
//

#pragma pack(4)

typedef struct
{
    //BYTE rgb[48];               // values of scanline data (now we're cycle granular so we don't pre-cache the screen RAM)

    BYTE modehi;                // display list byte (hi nibble)
    BYTE modelo;                // display list byte (lo nibble)
    WORD addr;                  // starting address of scan line

    BYTE pad;                  // scan line within mode (0..15)
    BYTE dmactl;                // dmactl value
    BYTE chbase;                // starting page of text font
    BYTE chactl;                // character control

    union
        {
        struct
            {
            BYTE colpf0;
            BYTE colpf1;
            BYTE colpf2;
            BYTE colpf3;
            };
        ULONG colpfX;
        BYTE colpf[4];
        };

    BYTE hscrol;
    BYTE vscrol;
    BYTE prior;                 // priority / GTIA select
    BYTE colbk;

    BYTE fpmg;                  // PMG are present on scan line
    BYTE fVscrol;               // line is vertically scrolled
} SL;

// PMG Player Missile Graphics structure

typedef struct
{
    union
        {
        struct
            {
            BYTE colpm0;
            BYTE colpm1;
            BYTE colpm2;
            BYTE colpm3;
            };
        ULONG colpmX;
        BYTE colpm[4];
        };

    union
        {
        struct
            {
            BYTE grafp0;
            BYTE grafp1;
            BYTE grafp2;
            BYTE grafp3;
            };
        ULONG grafpX;
        BYTE grafp[4];
        };

    union
        {
        struct
            {
            BYTE hposm0;
            BYTE hposm1;
            BYTE hposm2;
            BYTE hposm3;
            };
        ULONG hposmX;
        BYTE hposm[4];
        };

    union
        {
        struct
            {
            BYTE hposp0;
            BYTE hposp1;
            BYTE hposp2;
            BYTE hposp3;
            };
        ULONG hpospX;
        BYTE hposp[4];
        };

    union
        {
        struct
            {
            BYTE sizep0;
            BYTE sizep1;
            BYTE sizep2;
            BYTE sizep3;
            };
        ULONG sizepX;
        BYTE sizep[4];
        };

    // which pixel the players and missiles start at stop at, given their current hpos's and sizes
    // (Sorry, Darek, these have to be signed. A PMG that straddles the left edge of the screen only
    // has its right half showing and my math only works if these are signed)
    short hpospPixStart[4];
    short hpospPixStop[4];
    short hposmPixStart[4];
    short hposmPixStop[4];

    // when we change the location of a PMG, this is the location we're trying to change it to
    // It doesn't take effect right away, since we have to finish it in the old position, if we're halfway through drawing it
    short hpospPixNewStart[4];
    BYTE newGRAFp[4];       // delay changing the data in the player until it completes drawing the current one
    
    // of all the PMs, old and new positions, what is the earliest and latest pixel they can be found at?
    // (optimize areas we know to be empty of PMG). Also must be signed.
    short hposPixEarliest;
    short hposPixLatest;

    BYTE cwp[4];    // size, translated into how much to shift by (1, 2 or 3 for single, double, quad)
    BYTE cwm[4];

    // is the current mode hi-res mono mode 2, 3 or 15? Is GTIA enabled in PRIOR?
    BOOL fHiRes, fGTIA;

    BYTE grafm;
    BYTE sizem;
    BYTE pmbase;
} PMG;

#pragma pack()

//
// All 6502 address space variables are FAR

#pragma pack(4)

// Each VM has data that is never persisted. By giving them their own structure, most indexes will all be near the top 
// of the structure and can be dereferenced quickly (vs hiding them at the end of the CANDYHW structure).
// So put the shortest stuff first

typedef struct
{
    ULONGLONG m_ullTimeTravelTime;    // the time stamp of a snapshot
    char *m_Time[3];       // 3 time travel saved snapshots, 5 seconds apart, for going back ~13 seconds
    char m_cTimeTravelPos; // which is the current snapshot?

    DRIVE m_rgDrives[MAX_DRIVES];

    // Like disk and cartridge images, this is not persisted because of its size. We simply re-fill it when we are loaded back in.
    BYTE m_sectorSIO[256];    // disk sector, not persisted but reloaded

    BYTE *m_rgbSwapCart;      // Contents of the cartridges, not persisted but reloaded

    PVM m_pvm;                // pointer to GEM's persisted info about our VM that we may need to access
    PVMINST m_pvmi;           // pointer to GEM's non-persisted info about our VM that we may need to access
} CANDY_UNPERSISTED;

#define ullTimeTravelTime        ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_ullTimeTravelTime
#define cTimeTravelPos           ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_cTimeTravelPos
#define Time                     ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_Time
#define rgDrives                 ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_rgDrives
#define sectorSIO                ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_sectorSIO
#define rgbSwapCart              ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_rgbSwapCart
#define pvm                      ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_pvm
#define pvmin                    ((CANDY_UNPERSISTED *)((BYTE *)candy + ((CANDYHW *)candy)->m_dwCandySize))->m_pvmi

// CANDY - our persistable state, including SL (scan line) and PMG (player-missile graphics)

typedef struct
{
    // most ofen accessed variables go first!

    WORD m_ramtop;
    int m_dwCandySize;

    // 6502 register context - BELONGS IN CPU NOT HERE !!!
    WORD m_regPC, m_regSP;
    BYTE m_regA, m_regY, m_regX, m_regP;
    WORD m_regEA;

    // the 8 status bits must be together and in the same order as in the 6502
    unsigned char    m_srN;
    unsigned char    m_srV;
    unsigned char    m_srB;
    unsigned char    m_srD;
    unsigned char    m_srI;
    unsigned char    m_srZ;
    unsigned char    m_srC;
    unsigned char    m_cntTickLo;

    short m_wLeft;          // signed, cycles to go can go <0 finishing the last 6502 instruction
    short m_wNMI;    // keep track of wLeft when debugging, needs to be thread safe, but not persisted, and signed like wLeft
    BYTE m_fTrace;
    WORD m_bp;             // breakpoint
    WORD m_wCycle;      // the cycle of the scan line we're about to execute (0 - 113)

    // 6502 address space
    BYTE m_rgbMem[65536];

    // !!! For efficiency, the above variables used in the tight 6502 loop need to be at the front of this structure

    WORD m_fKeyPressed;     // xkey.c
    WORD m_wLastScanCh;     // xkey.c, last scan codes seen
    BOOL m_wShiftChanged;   // xkey.c

    BOOL m_fAlreadyTriedBASIC;  // have we auto-swapped in BASIC
    BYTE m_fCartNeedsSwap;  // we just un-banked the cartridge for persisting. The next Execute needs to re-swap it

    BYTE m_ANTICBankDifferent;  // is ANTIC not using the same XE bank as the CPU?

    // mdXLXE:  0 = Atari 400/800, 1 = 800XL, 2 = 130XE
    // cntTick: mode display countdown timer (18 Hz ticks)
    BYTE m_mdXLXE, m_cntTick;

    BYTE m_iSwapCart;   // which bank is currently swapped in

    WORD m_wFrame, m_wScan;
    short m_wLastSIOSector; // which sector we read last time
    WORD m_wLastSectorFrame;// when we last read a sector, to know when the spin down the motor
    BYTE m_WSYNC_Waiting;   // do we need to limit the next scan line to the part after WSYNC is released?
    BYTE m_fAltBinLoader;   // use the alternate binary loader

    short m_PSL;        // the value of wLeft last time ProcessScanLine was called

    // bare bones SIO support
    BYTE m_rgSIO[5];    // holds the SIO command frame
    BYTE m_cSEROUT;     // how many bytes we've gotten so far of the 5
    WORD m_fSERIN;      // we're executing a disk read command
    WORD m_isectorPos;  // where in the buffer are we?
    BYTE m_checksum;    // buffer checksum
    BYTE m_fWant8;      // we'd like the SEROUT DONE IRQ8
    BYTE m_fWant10;     // we'd like the SEROUT NEEDED IRQ10

    BYTE m_fHitBP;      // anybody changing the PC outside of Go6502 needs to check and set this

    BOOL m_fPAL;            // emulating PAL?
    BOOL m_fSwitchingToPAL; // are we in the middle of switching to PAL?
    BYTE m_fRedoPoke;       // we tried to change a register after a scan line is over. Wait until the next scan line to do it.
    BOOL m_fInVBI;          // are we inside the VBI?
    BOOL m_fInDLI;          // are we inside the DLI?
    WORD m_fDLIinVBI;       // what scan line did we last see a DLI start inside a VBI?
    WORD m_wScanVBIEnd;     // what scan line did the VBI end on?
    WORD m_wPALFrame;       // what frame did somthing suspicously PAL-like last happen on?

    BYTE m_POT;         // current paddle potentiometer reading, counts from 0 to 228

    WORD m_wLiveShift;  // do we look at the shift key live as we process keys, or are we pasting and we want a specific value?

    WORD m_wSLEnd;      // last visible pixel of a scan line (some tiles may be partially off the right hand side)

    WORD m_wSIORTS;     // return value of the SIO routine, used in monitor
    BOOL m_fDrivesNeedMounting; // we delay this so app startup is fast, since it doesn't alloc additional memory and won't fail

    // ANTIC stuff (xvideo.c)
    PMG m_pmg;          // PMG structure (only 1 needed, updated each scan line)
    SL m_sl;            // current scan line display info
    WORD m_cbWidth, m_cbDisp;
    WORD m_hshift;
    BYTE m_iscan;
    BYTE m_scans;
    WORD m_wAddr;
    BYTE m_fWait;       // wait until next VBI
    BYTE m_fFetch;      // fetch next DL instruction
    WORD m_wAddrOff;    // because of HSCROL, how many bytes forward to actually start the scan line

    // things we precompute for artifacting
    ULONG m_FillA;
    ULONG m_Fill1;
    ULONG m_Fill1bf;
    ULONG m_Fill2;
    ULONG m_Fill2bf;

    BYTE m_bCartType;   // type of cartridge
    BYTE m_fRAMCart;    // does this cartridge type have a RAM bank?
    WORD m_iBankSize;   // how big banks are on this cartridge
    BYTE m_iNumBanks;   // how many banks in this cartridge?
    BYTE m_bshftByte;   // current value of shift state

    LONG m_irqPokey[4]; // POKEY h/w timers, how many cycles to go
    BYTE m_AUDCTLSave;
    BYTE m_AUDFxSave[4];
    LONG m_irqPokeySave[4];

    int m_iXESwap;      // which 16K chunk is saving something swapped out from regular RAM? This saves needing 16K more

	BOOL m_fJoyCONSOL;	// joystick is the reason for the CONSOL key being pressed
    BOOL m_fJoyFire;	// joystick is the reason for the FIRE button being pressed
    BOOL m_fJoyMove;    // joystick is the reason for moving (not the keyboard)

    BYTE m_chka00;      // checksum of page $a

    #define TEMP_SIZE 1024
    BYTE m_temp[TEMP_SIZE];  // some temporary storage for swapping banks

    // !!! Not used anymore, candidates for removal
    WORD m_fJoy, m_fSoundOn, m_fAutoStart;
    ULONG m_clockMult;
    BYTE m_btickByte;   // current value of 18 Hz timer
    //

    BYTE m_rgbXLExtMem; // beginning of XL extended memory

    // which, if present, will look like this:
    //
    //BYTE m_rgbSwapSelf[2048];    // extended XL memory
    //BYTE m_rgbSwapC000[4096];
    //BYTE m_rgbSwapD800[10240];
    //BYTE m_rgbSwapBASIC[8192];
    //
    //BYTE m_rgbXEMem[4][16384];
    //

} CANDYHW;

#pragma pack()

#define CANDY_STATE(name) ((CANDYHW *)candy)->m_##name

#define dwCandySize   CANDY_STATE(dwCandySize)
#define rgbMem        CANDY_STATE(rgbMem)
#define regPC         CANDY_STATE(regPC)
#define regSP         CANDY_STATE(regSP)
#define regA          CANDY_STATE(regA)
#define regY          CANDY_STATE(regY)
#define regX          CANDY_STATE(regX)
#define regP          CANDY_STATE(regP)
#define regEA         CANDY_STATE(regEA)
#define srN           CANDY_STATE(srN)
#define srV           CANDY_STATE(srV)
#define srB           CANDY_STATE(srB)
#define srD           CANDY_STATE(srD)
#define srI           CANDY_STATE(srI)
#define srZ           CANDY_STATE(srZ)
#define srC           CANDY_STATE(srC)
#define WSYNC_Seen    CANDY_STATE(WSYNC_Seen)
#define WSYNC_Waiting CANDY_STATE(WSYNC_Waiting)
#define PSL           CANDY_STATE(PSL)
#define wSLEnd        CANDY_STATE(wSLEnd)
#define wNMI          CANDY_STATE(wNMI)
#define wCycle        CANDY_STATE(wCycle)
#define wSIORTS       CANDY_STATE(wSIORTS)
#define fAltBinLoader CANDY_STATE(fAltBinLoader)
#define rgSIO         CANDY_STATE(rgSIO)
#define cSEROUT       CANDY_STATE(cSEROUT)
#define fSERIN        CANDY_STATE(fSERIN)
#define fDrivesNeedMounting CANDY_STATE(fDrivesNeedMounting)
#define isectorPos    CANDY_STATE(isectorPos)
#define checksum      CANDY_STATE(checksum)
#define fWant8        CANDY_STATE(fWant8)
#define fWant10       CANDY_STATE(fWant10)
#define fHitBP        CANDY_STATE(fHitBP)
#define bias          CANDY_STATE(bias)
#define iSwapCart     CANDY_STATE(iSwapCart)
#define fCartNeedsSwap CANDY_STATE(fCartNeedsSwap)
#define fRedoPoke     CANDY_STATE(fRedoPoke)
#define wLastSIOSector CANDY_STATE(wLastSIOSector)
#define wLastSectorFrame CANDY_STATE(wLastSectorFrame)
#define fAlreadyTriedBASIC CANDY_STATE(fAlreadyTriedBASIC)
#define fKeyPressed   CANDY_STATE(fKeyPressed)
#define wLastScanCh   CANDY_STATE(wLastScanCh)
#define bp            CANDY_STATE(bp)
#define wShiftChanged CANDY_STATE(wShiftChanged)
#define wLiveShift    CANDY_STATE(wLiveShift)
#define fTrace        CANDY_STATE(fTrace)
#define mdXLXE        CANDY_STATE(mdXLXE)
#define cntTick       CANDY_STATE(cntTick)
#define cntTickLo     CANDY_STATE(cntTickLo)
#define wFrame        CANDY_STATE(wFrame)
#define wScan         CANDY_STATE(wScan)
#define wLeft         CANDY_STATE(wLeft)
#define ramtop        CANDY_STATE(ramtop)
#define fPAL          CANDY_STATE(fPAL)
#define fSwitchingToPAL CANDY_STATE(fSwitchingToPAL)
#define fInDLI        CANDY_STATE(fInDLI)
#define fInVBI        CANDY_STATE(fInVBI)
#define fDLIinVBI     CANDY_STATE(fDLIinVBI)
#define wScanVBIEnd   CANDY_STATE(wScanVBIEnd)
#define wPALFrame     CANDY_STATE(wPALFrame)
#define fJoy          CANDY_STATE(fJoy)
#define fSoundOn      CANDY_STATE(fSoundOn)
#define fAutoStart    CANDY_STATE(fAutoStart)
#define clockMult     CANDY_STATE(clockMult)
#define pmg           CANDY_STATE(pmg)
#define sl            CANDY_STATE(sl)
#define cbWidth       CANDY_STATE(cbWidth)
#define cbDisp        CANDY_STATE(cbDisp)
#define hshift        CANDY_STATE(hshift)
#define iscan         CANDY_STATE(iscan)
#define scans         CANDY_STATE(scans)
#define wAddr         CANDY_STATE(wAddr)
#define wAddrOff      CANDY_STATE(wAddrOff)
#define fWait         CANDY_STATE(fWait)
#define fFetch        CANDY_STATE(fFetch)
#define POT           CANDY_STATE(POT)
#define bCartType     CANDY_STATE(bCartType)
#define fRAMCart      CANDY_STATE(fRAMCart)
#define iBankSize     CANDY_STATE(iBankSize)
#define iNumBanks     CANDY_STATE(iNumBanks)
#define btickByte     CANDY_STATE(btickByte)
#define bshftByte     CANDY_STATE(bshftByte)
#define irqPokey      CANDY_STATE(irqPokey)
#define irqPokeySave  CANDY_STATE(irqPokeySave)
#define AUDCTLSave    CANDY_STATE(AUDCTLSave)
#define AUDFxSave     CANDY_STATE(AUDFxSave)
#define iXESwap       CANDY_STATE(iXESwap)
#define fJoyCONSOL    CANDY_STATE(fJoyCONSOL)
#define fJoyFire      CANDY_STATE(fJoyFire)
#define fJoyMove      CANDY_STATE(fJoyMove)
#define chka00        CANDY_STATE(chka00)
#define rgbXLExtMem   CANDY_STATE(rgbXLExtMem)
#define ANTICBankDifferent   CANDY_STATE(ANTICBankDifferent)
#define temp          CANDY_STATE(temp)
#define FillA         CANDY_STATE(FillA)
#define Fill1bf       CANDY_STATE(Fill1bf)
#define Fill1         CANDY_STATE(Fill1)
#define Fill2bf       CANDY_STATE(Fill2bf)
#define Fill2         CANDY_STATE(Fill2)

#define SELF_SIZE 2048
#define C000_SIZE 4096
#define BASIC_SIZE 8192
#define D800_SIZE 10240
#define XE_SIZE 16384

// if present, here is where these would be
#define rgbSwapSelf   &CANDY_STATE(rgbXLExtMem)
#define rgbSwapC000   (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE)
#define rgbSwapD800   (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE + C000_SIZE)
#define rgbSwapBASIC  (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE + C000_SIZE + D800_SIZE)
#define rgbXEMem      (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE + C000_SIZE + D800_SIZE + BASIC_SIZE)

// Time Travel stuff

BOOL TimeTravel(void *);
BOOL TimeTravelPrepare(void *, BOOL);
BOOL TimeTravelReset(void *);
BOOL TimeTravelInit(void *);
void TimeTravelFree(void *);
BOOL TimeTravelFixPoint(void *);


#include "6502.h"

//
// Shift key status bits (are returned by PC BIOS)
//

#define wRCtrl    0x80    // specifically, right control
#define wCapsLock 0x40
#define wNumLock  0x20
#define wScrlLock 0x10
#define wAlt      0x08
#define wCtrl     0x04
#define wAnyShift 0x03

//
// Platform specific stuff
//

#define pbshift &bshftByte;

#if 0
_pbshift(void *candy)
__inline BYTE *_pbshift(void *candy)
{
    return &bshftByte;
}
#endif

//
// 6502 condition codes in P register: NV_BDIZC
// we could just use the 6502.h defines, you know

#define NBIT 0x80
#define VBIT 0x40
#define BBIT 0x10
#define DBIT 0x08
#define IBIT 0x04
#define ZBIT 0x02
#define CBIT 0x01

//
// Atari 800 hardware read registers
//

// GTIA

#define readGTIA 0xD000

#define MXPF    ((&rgbMem[0xD000]))
#define M0PF    rgbMem[0xD000]
#define M1PF    rgbMem[0xD001]
#define M2PF    rgbMem[0xD002]
#define M3PF    rgbMem[0xD003]

#define PXPF    ((&rgbMem[0xD004]))
#define P0PF    rgbMem[0xD004]
#define P1PF    rgbMem[0xD005]
#define P2PF    rgbMem[0xD006]
#define P3PF    rgbMem[0xD007]

#define MXPL    ((&rgbMem[0xD008]))
#define M0PL    rgbMem[0xD008]
#define M1PL    rgbMem[0xD009]
#define M2PL    rgbMem[0xD00A]
#define M3PL    rgbMem[0xD00B]

#define PXPL    ((&rgbMem[0xD00C]))
#define P0PL    rgbMem[0xD00C]
#define P1PL    rgbMem[0xD00D]
#define P2PL    rgbMem[0xD00E]
#define P3PL    rgbMem[0xD00F]

#define TRIG0   rgbMem[0xD010]
#define TRIG1   rgbMem[0xD011]
#define TRIG2   rgbMem[0xD012]
#define TRIG3   rgbMem[0xD013]

#define PAL     rgbMem[0xD014]

#define CONSOL  rgbMem[0xD01F]

// POKEY

#define readPOKEY 0xD200

#define POT0    0xD200
#define POT1    0xD201
#define POT2    0xD202
#define POT3    0xD203
#define POT4    0xD204
#define POT5    0xD205
#define POT6    0xD206
#define POT7    0xD207

#define ALLPOT  rgbMem[0xD208]
#define KBCODE  rgbMem[0xD209]
#define RANDOM  rgbMem[0xD20A]
#define SERIN   rgbMem[0xD20D]
#define IRQST   rgbMem[0xD20E]
#define SKSTAT  rgbMem[0xD20F]

// PIA

#define readPIA 0xD300

#define PORTAea 0xD300
#define PORTA   rgbMem[0xD300]
#define PORTB   rgbMem[0xD301]

#define PACTLea 0xD302
#define PACTL   rgbMem[0xD302]
#define PBCTL   rgbMem[0xD303]

// ANTIC

#define readANTIC 0xD400

#define VCOUNT  rgbMem[0xD40B]
#define PENH    rgbMem[0xD40C]
#define PENV    rgbMem[0xD40D]
#define NMIST   rgbMem[0xD40F]


//
// Atari 800 hardware write registers
//

// GTIA

#define writeGTIA 0xD110

#define HPOSPX  (*(ULONG *)&rgbMem[0xD110])
#define HPOSP0  rgbMem[0xD110]
#define HPOSP1  rgbMem[0xD111]
#define HPOSP2  rgbMem[0xD112]
#define HPOSP3  rgbMem[0xD113]

#define HPOSMX  (*(ULONG *)&rgbMem[0xD114])
#define HPOSM0  rgbMem[0xD114]
#define HPOSM1  rgbMem[0xD115]
#define HPOSM2  rgbMem[0xD116]
#define HPOSM3  rgbMem[0xD117]

#define SIZEPX  (*(ULONG *)&rgbMem[0xD118])
#define SIZEP0  rgbMem[0xD118]
#define SIZEP1  rgbMem[0xD119]
#define SIZEP2  rgbMem[0xD11A]
#define SIZEP3  rgbMem[0xD11B]
#define SIZEM   rgbMem[0xD11C]

#define GRAFPX  (*(ULONG *)&rgbMem[0xD11D])
#define GRAFP0A 0xD11D
#define GRAFP0  rgbMem[0xD11D]
#define GRAFP1  rgbMem[0xD11E]
#define GRAFP2  rgbMem[0xD11F]
#define GRAFP3  rgbMem[0xD120]
#define GRAFM   rgbMem[0xD121]

#define COLPMX  (*(ULONG *)&rgbMem[0xD122])
#define COLPM0  rgbMem[0xD122]
#define COLPM1  rgbMem[0xD123]
#define COLPM2  rgbMem[0xD124]
#define COLPM3  rgbMem[0xD125]

#define COLPFX  (*(ULONG *)&rgbMem[0xD126])
#define COLPF0  rgbMem[0xD126]
#define COLPF1  rgbMem[0xD127]
#define COLPF2  rgbMem[0xD128]
#define COLPF3  rgbMem[0xD129]

#define COLBK   rgbMem[0xD12A]
#define PRIOR   rgbMem[0xD12B]
#define VDELAY  rgbMem[0xD12C]
#define GRACTL  rgbMem[0xD12D]
#define HITCLR  rgbMem[0xD12E]
#define wCONSOL rgbMem[0xD12F]

// POKEY

#define writePOKEY 0xD130

#define AUDF1   rgbMem[0xD130]
#define AUDC1   rgbMem[0xD131]
#define AUDF2   rgbMem[0xD132]
#define AUDC2   rgbMem[0xD133]
#define AUDF3   rgbMem[0xD134]
#define AUDC3   rgbMem[0xD135]
#define AUDF4   rgbMem[0xD136]
#define AUDC4   rgbMem[0xD137]
#define AUDCTL  rgbMem[0xD138]

#define STIMER  rgbMem[0xD139]
#define SKRES   rgbMem[0xD13A]
#define POTGO   rgbMem[0xD13B]

#define SEROUT  rgbMem[0xD13D]
#define IRQEN   rgbMem[0xD13E]
#define SKCTLS  rgbMem[0xD13F]

// PIA

#define writePIA 0xD140

#define wPORTAea 0xD140
#define wPORTA  rgbMem[0xD140]
#define wPORTB  rgbMem[0xD141]

#define wPACTLea 0xD142
#define wPACTL  rgbMem[0xD142]
#define wPBCTL  rgbMem[0xD143]

#define wPADDIRea 0xD144
#define wPADDIR rgbMem[0xD144]
#define wPBDDIR rgbMem[0xD145]

#define wPADATAea 0xD146
#define wPADATA rgbMem[0xD146]
#define wPBDATA rgbMem[0xD147]

#define rPADATAea 0xD148
#define rPADATA rgbMem[0xD148]
#define rPBDATA rgbMem[0xD149]

// ANTIC

#define writeANTIC 0xD150

#define DMACTL  rgbMem[0xD150]
#define CHACTL  rgbMem[0xD151]

#define DLPC    (*(WORD FAR *)&rgbMem[0xD152])
#define DLISTL  rgbMem[0xD152]
#define DLISTH  rgbMem[0xD153]

#define HSCROL  rgbMem[0xD154]
#define VSCROL  rgbMem[0xD155]

#define PMBASE  rgbMem[0xD157]

#define CHBASE  rgbMem[0xD159]
#define WSYNC   rgbMem[0xD15A]

#define NMIEN   rgbMem[0xD15E]
#define NMIRES  rgbMem[0xD15F]

// POKEY again - actual paddle values that the POT registers are counting up to

#define PADDLE0 0xD160
#define PADDLE1 0xD161
#define PADDLE2 0xD162
#define PADDLE3 0xD163
#define PADDLE4 0xD164
#define PADDLE5 0xD165
#define PADDLE6 0xD166
#define PADDLE7 0xD167

// D180 is where we put the SIO hack code

//
// Function prototypes
//

void KillMePlease(void *);
void KillMePleaseXE(void *);
void KillMePleaseBASIC(void *);
void PrecomputeArtifacting(void *);
void KillMeSoftlyPlease(void *);
void SwitchToPAL(void *);
void Interrupt(void *, BOOL);
void CheckKey(void *, BOOL, WORD);
void UpdatePorts(void *);
void SIOV(void *);
BOOL SIOReadSector(void *, int, BYTE *);
void SIOGetInfo(void *, int, BOOL *, BOOL *, BOOL *, BOOL *, BOOL *);
BOOL GetWriteProtectDrive(void *, int);
BOOL SetWriteProtectDrive(void *, int, BOOL);
void DeleteDrive(void *, int);
BOOL AddDrive(void *, int, BYTE *);
void CloseDrive(void *, int);
void CreateDMATables();
void CreatePMGTable();
void PSLPrepare(void *);    // call at the beginning of the scan line to get the mode
BOOL ProcessScanLine(void *);
BOOL SwapMem(void *, BYTE mask, BYTE flags);
void InitBanks(void *);
void CchDisAsm(void *, WORD *puMem);
void CchShowRegs(void *);
void ControlKeyUp8(void *);
void AddToPacket(void *, ULONG);
void SwapHelper(void *, BYTE *, BYTE *, int);

//extern int fXFCable;    // appears to be unused
//int _SIOV(char *qch, int wDev, int wCom, int wStat, int wBytes, int wSector, int wTimeout);
//void ReadROMs();
//void ReadJoy(void);
//void MyReadJoy(void);
//void SaveVideo(void);
//void RestoreVideo(void);
//void WaitForVRetrace(void);
//void InitSIOV(int, char **);
//void DiskConfig(void);
//extern unsigned uBaudClock;
//void _SIO_Init(void);
//void _SIO_Calibrate(void);
//void InitSoundBlaster();

//
// Various ROM data and swap buffers
//

extern char FAR rgbAtariOSB[]; // Atari 400/800 ROMs
extern char FAR rgbXLXEBAS[], FAR rgbXLXED800[];  // XL/XE ROMs
extern char FAR rgbXLXEC000[], FAR rgbXLXE5000[]; // self test ROMs

//
// The 3 hardware modes our VM can have
//

#define md800 0
#define mdXL  1
#define mdXE  2

//
// Function prototypes of the Atari 800 VM API
//

BOOL __cdecl InstallAtari(void **, int *, PVM, PVMINFO, int);
BOOL __cdecl UnInstallAtari(void *);
BOOL __cdecl InitAtari(void *);
BOOL __cdecl UninitAtari(void *);
BOOL __cdecl InitAtariDisks(void *);
BOOL __cdecl MountAtariDisk(void *, int);
BOOL __cdecl UninitAtariDisks(void *);
BOOL __cdecl UnmountAtariDisk(void *, int);
BOOL __cdecl WriteProtectAtariDisk(void *, int, BOOL, BOOL);
BOOL __cdecl WarmbootAtari(void *);
BOOL __cdecl ColdbootAtari(void *);
BOOL __cdecl SaveStateAtari(void *);
BOOL __cdecl LoadStateAtari(void *, void *, int);
BOOL __cdecl DumpHWAtari(void *);
BOOL __cdecl TraceAtari(void *, BOOL, BOOL);
BOOL __cdecl ExecuteAtari(void *, BOOL, BOOL);
BOOL __cdecl KeyAtari(void *, HWND, UINT, WPARAM, LPARAM);
BOOL __cdecl DumpRegsAtari(void *);
BOOL __cdecl MonAtari(void *);

WORD __cdecl PeekWAtari(void *, ADDR addr);
ULONG __cdecl PeekLAtari(void *, ADDR addr);
BOOL  __cdecl PokeLAtari(void *, ADDR addr, ULONG l);
BOOL  __cdecl PokeWAtari(void *, ADDR addr, WORD w);

// all the possible PEEK routines, based on address, for the jump table version
BYTE __forceinline __fastcall PeekBAtariHW(void *, ADDR addr); // d0, d2, d3, d4
BYTE __forceinline __fastcall PeekBAtariBB(void *, ADDR addr); // 8f, 9f for BountyBob bank select
BYTE __forceinline __fastcall PeekBAtariBS(void *, ADDR addr); // d5 for other cartridge bank select

BYTE __forceinline __fastcall PeekBAtari(void *, ADDR addr);   // non-jump table single entry point

BYTE PeekBAtariMON(void *, ADDR addr);   // something the monitor is allowed to call

// all the possible POKE routines, based on address, for the jump table version
BOOL  __forceinline __fastcall PokeBAtariDL(void *, ADDR, BYTE);   // screen RAM
BOOL  __forceinline __fastcall PokeBAtariBB(void *, ADDR, BYTE);   // $8fxx or $9fxx bank select for BountyBob cartridge
BOOL  __forceinline __fastcall PokeBAtariNULL(void *, ADDR, BYTE); // above ramtop until $c000, $d500 to $d7ff
BOOL  __forceinline __fastcall PokeBAtariOS(void *, ADDR, BYTE);   // $c000 to $cfff and $d800 and above
BOOL  __forceinline __fastcall PokeBAtariHW(void *, ADDR, BYTE);   // d000-d4ff
BOOL  __forceinline __fastcall PokeBAtariBS(void *, ADDR, BYTE);   // d500-d5ff

BOOL  __forceinline __fastcall PokeBAtari(void *, ADDR, BYTE);     // non-jump table single entry point

BOOL  PokeBAtariMON(void *, ADDR, BYTE); // something the monitor is allowed to call

//
// Map C runtime file i/o calls to appropriate 32-bit calls
//

// Map to Win32 counterparts

// !!! This is very dangerous!
#define _open    _lopen
#define _read    _lread
#define _write   _lwrite
#define _lseek   _llseek
#define _close   _lclose
#undef  _O_BINARY
#define _O_BINARY 0
#undef  _O_RDWR
#define _O_RDWR   OF_READWRITE
#undef  _O_RDONLY
#define _O_RDONLY OF_READ
#undef  SEEK_END
#define SEEK_END  FILE_END
#undef  SEEK_SET
#define SEEK_SET  FILE_BEGIN

// for xvideo.c

#define bfBK  0x00
#define bfPF0 0x01
#define bfPF1 0x02
#define bfPF2 0x04
#define bfPF3 0x08
#define bfPM0 0x10
#define bfPM1 0x20
#define bfPM2 0x40
#define bfPM3 0x80

#ifndef NDEBUG
#undef Assert
#define Assert(f) _800_assert((f), __FILE__, __LINE__, candy)
extern __inline void _800_assert(int f, char *file, int line, void *);
#endif
