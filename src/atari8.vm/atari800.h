
/***************************************************************************

    ATARI800.H

    - Common include file for all Atari 800 VM components.
    - Defines all of the private variables used by the Atari 800 VM.

    Copyright (C) 1986-2018 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    12/07/1998  darekm      PC Xformer 3.8 release

***************************************************************************/

#pragma once

#include "common.h"

#define STARTSCAN 8    // scan line ANTIC starts drawing at
#define NTSCY 262    // ANTIC does 262 line non-interlaced NTSC video at true ~60fps, not 262.5 standard NTSC interlaced video.
                    // the TV is prevented from going in between scan lines on the next pass, but always overwrites the previous frame.
                    // I wonder if that ever created weird burn-in patterns. We do not emulate PAL.
#define HCLOCKS 114 // number of clock cycles per horizontal scan line

extern BYTE rgbRainbow[];    // the ATARI colour palette

// all the reasons ANTIC might do DMA and block the CPU

#define DMA_M 1        // grab missile data if missile DMA is on
#define DMA_DL 2    // grab DList mode if Playfield DMA is on
#define DMA_P 3        // grab player data if player DMA is on
#define DMA_LMS 4    // do the load memory scan

#define W8 5        // wide playfield hi, med or lo res modes
#define WC4 6        // 1st scan line of a character mode, hi or med res
#define W4 7        // wide playfield hi or med res modes
#define WC2 8        // 1st scan line of a character mode, hi res
#define W2 9        // wide playfield hi res mode

#define N8 10        // same for wide or normal playfield (present in all but narrow)
#define NC4 11
#define N4 12
#define NC2 13
#define N2 14

#define A8 15        // same for wide, normal or narrow playfield
#define AC4 16
#define A4 17
#define AC2 18
#define A2 19

// Describes what ANTIC does for each cycle, see comment in atari800.c
extern const BYTE rgDMA[HCLOCKS];

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
char rgDMAMap[19][2][3][2][2][2][2][HCLOCKS + 3];

// for clock cycle number 0-113 (0 based), what pixel is being drawn when wLeft is from 1-114).
short rgPIXELMap[HCLOCKS];

// precompute PMG priorities
char rgPMGMap[65536];

// this mess is how we properly index all of those arrays
#define DMAMAP rgDMAMap[sl.modelo][(DMACTL & 0x20) >> 5][(DMACTL & 0x03) ? ((DMACTL & 3) >> 1) : 0][iscan == sl.vscrol][(DMACTL & 0x8) >> 3][((DMACTL & 4) >> 2) | ((DMACTL & 8) >> 3)][((sl.modehi & 4) && sl.modelo >= 2) ? 1 : 0]

// !!! I ignore the fact that HSCROL delays the PF DMA by a variable number of clocks

// !!! Why do I take 92/130 cycles in mode 0/2 with 9 RAM refresh cycles?

// for testing, # of jiffies it takes a real 800 to do FOR Z=1 TO 1000 in these graphics modes (+16 to eliminate mode 2 parts):
//   88-89      125                 102 101     86  87  89      92          100         121     121
//   0            2 GR.0              6 GR.1/2    8               11 GR.6     13 GR.7     15      GTIA

// XE is non-zero when 130XE emulation is wanted
#define XE 1

#define X8 352        // screen width
#define Y8 240        // screen height

#define CART_8K      1    // 0 for invalid
#define CART_16K     2
#define CART_OSSA    3  // Action
#define CART_OSSAX   4  // 0 4 3 version
#define CART_OSSB    5  // Mac65
#define CART_XEGS     6
#define CART_BOB     7
#define CART_ATARIMAX1    8
#define CART_ATARIMAX8 9

#define MAX_CART_SIZE 1048576 + 16 // 1MB cart with 16 byte header
BYTE *rgbSwapCart[MAX_VM];    // Contents of the cartridges, not persisted but reloaded
int candysize[MAX_VM];        // how big our persistable data is (bigger for XL/XE than 800), set at Install

// bare wire SIO stuff to support apps too stupid to know the OS has a routine to do this for you

#define SIO_DELAY 6                // wait fewer scan lines than this and you're faster than 19,200 BAUD and apps hang not expecting it so soon
// Like disk and cartridge images, this is not persisted because of its size. We simply re-fill it when we are loaded back in.
BYTE sectorSIO[MAX_VM][128];    // disk sector, not persisted but reloaded

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

// Time Travel stuff

BOOL TimeTravel(unsigned);
BOOL TimeTravelPrepare(unsigned);
BOOL TimeTravelReset(unsigned);
BOOL TimeTravelInit(unsigned);
void TimeTravelFree(unsigned);

ULONGLONG ullTimeTravelTime[MAX_VM];    // the time stamp of a snapshot
char cTimeTravelPos[MAX_VM];    // which is the current snapshot?
char *Time[MAX_VM][3];        // 3 time travel saved snapshots, 5 seconds apart, for going back ~13 seconds

//
// Scan line structure
//

#pragma pack(4)

typedef struct
{
    BYTE rgb[48];               // values of scanline data

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
    short hpospPixStart[4];
    short hpospPixStop[4];
    short hposmPixStart[4];
    short hposmPixStop[4];
    
    // of all the PMs, what is the earliest and latest pixel they can be found at? (optimize areas we know to be empty of PMG)
    short hposPixEarliest;
    short hposPixLatest;

    BYTE cwp[4];    // size, translated into how much to shift by (1, 2 or 3 for single, double, quad)
    BYTE cwm[4];

    // is the current mode hi-res mono mode 2, 3 or 15? Is GTIA enabled in PRIOR?
    BOOL fHiRes, fGTIA;

    BYTE grafm;
    BYTE sizem;
    BYTE pmbase;
    BYTE fHitclr;
} PMG;

#pragma pack()

//
// All 6502 address space variables are FAR

#pragma pack(4)

// CANDY - our persistable state, including SL (scan line) and PMG (player-missile graphics)

typedef struct
{
    // 6502 register context - BELONGS IN CPU NOT HERE !!!

    WORD m_regPC, m_regSP;
    BYTE m_regA, m_regY, m_regX, m_regP;
    WORD m_regEA;
    BYTE m_mdEA;

    WORD m_fKeyPressed;     // xkey.c
    WORD m_bp;             // breakpoint
    BOOL m_wShiftChanged;// xkey.c

    // 6502 address space
    BYTE m_rgbMem[65536];

    // fTrace:  non-zero for single opcode execution
    // mdXLXE:  0 = Atari 400/800, 1 = 800XL, 2 = 130XE
    // cntTick: mode display countdown timer (18 Hz ticks)

    BYTE m_fTrace;
    BYTE m_fCartNeedsSwap;  // we just un-banked the cartridge for persisting. The next Execute needs to re-swap it
    BYTE m_mdXLXE, m_cntTick;

    BYTE m_iSwapCart;   // which bank is currently swapped in

    WORD m_wFrame, m_wScan;
    short m_wLeft;        // signed, cycles to go can go <0 finishing the last 6502 instruction
    short m_wLastSIOSector; // which sector we read last time
    BYTE m_WSYNC_Waiting; // do we need to limit the next scan line to the part after WSYNC is released?
    BYTE m_WSYNC_on_RTI;  // restore the state of m_WSYNC_Waiting after a DLI

    short m_PSL;        // the value of wLeft last time ProcessScanLine was called

    short m_wNMI;    // keep track of wLeft when debugging, needs to be thread safe, but not persisted, and signed like wLeft

    BYTE m_rgSIO[5];    // holds the SIO command frame
    BYTE m_cSEROUT;        // how many bytes we've gotten so far of the 5
    WORD m_fSERIN;        // we're executing a disk read command
    BYTE m_bSERIN;        // byte to return in SERIN
    BYTE m_isectorPos;    // where in the buffer are we?
    BYTE m_checksum;    // buffer checksum
    BYTE m_fWant8;        // we'd like the SEROUT DONE IRQ8
    BYTE m_fWant10;        // we'd like the SEROUT NEEDED IRQ10

    BYTE m_fHitBP;        // anybody changing the PC outside of Go6502 needs to check and set this

    // size of RAM ($A000 or $C000)

    WORD FAR m_ramtop;

    WORD m_fStop;
    WORD m_wStartScan;
    BYTE m_fRedoPoke;
    BYTE pad3B;
    WORD pad4W;

    WORD m_fJoy, m_fSoundOn, m_fAutoStart;

    ULONG pad5UL;
    ULONG pad6UL;

    // clock multiplier
    ULONG m_clockMult;

    PMG m_pmg;          // PMG structure (only 1 needed, updated each scan line)
    SL m_sl;            // current scan line display info
    WORD m_cbWidth, m_cbDisp;
    WORD m_hshift;

    BYTE m_iscan;
    BYTE m_scans;
    WORD m_wAddr;

    BYTE m_fWait;       // wait until next VBI
    BYTE m_fFetch;      // fetch next DL instruction
    BYTE m_rgbSpecial;    // the offscreen character being scrolled on

    WORD pad7W;
    WORD pad8W;

    BYTE m_bCartType;   // type of cartridge
    BYTE m_btickByte;   // current value of 18 Hz timer
    BYTE m_bshftByte;   // current value of shift state

    BYTE pad9B;

    // the 8 status bits must be together and in the same order as in the 6502
    unsigned char     m_srN;
    unsigned char    m_srV;
    unsigned char    m_srB;
    unsigned char    m_srD;
    unsigned char    m_srI;
    unsigned char    m_srZ;
    unsigned char    m_srC;
    unsigned char    m_pad;

    LONG m_irqPokey[4];    // POKEY h/w timers, how many cycles to go

    int m_iXESwap;            // which 16K chunk is saving something swapped out from regular RAM? This saves needing 16K more

    char m_rgbXLExtMem;        // beginning of XL extended memory

    // which, if present, will look like this:
    //
    //char m_rgbSwapSelf[2048];    // extended XL memory
    //char m_rgbSwapC000[4096];
    //char m_rgbSwapD800[10240];
    //
    //char HUGE m_rgbXEMem[4][16384];    // !!! doesn't need to be HUGE anymore?
    //

} CANDYHW;

#pragma pack()

extern CANDYHW *vrgcandy[MAX_VM];

// make sure your local variable iVM is set to the instance you are before using
#define CANDY_STATE(name) vrgcandy[iVM]->m_##name

#define rgbMem        CANDY_STATE(rgbMem)
#define regPC         CANDY_STATE(regPC)
#define regSP         CANDY_STATE(regSP)
#define regA          CANDY_STATE(regA)
#define regY          CANDY_STATE(regY)
#define regX          CANDY_STATE(regX)
#define regP          CANDY_STATE(regP)
#define regEA         CANDY_STATE(regEA)
#define mdEA          CANDY_STATE(mdEA)
#define srN           CANDY_STATE(srN)
#define srV           CANDY_STATE(srV)
#define srB           CANDY_STATE(srB)
#define srD           CANDY_STATE(srD)
#define srI           CANDY_STATE(srI)
#define srZ           CANDY_STATE(srZ)
#define srC           CANDY_STATE(srC)
#define WSYNC_Seen    CANDY_STATE(WSYNC_Seen)
#define WSYNC_Waiting CANDY_STATE(WSYNC_Waiting)
#define WSYNC_on_RTI  CANDY_STATE(WSYNC_on_RTI)
#define PSL           CANDY_STATE(PSL)
#define wNMI          CANDY_STATE(wNMI)
#define rgbSpecial    CANDY_STATE(rgbSpecial)
#define rgSIO         CANDY_STATE(rgSIO)
#define cSEROUT       CANDY_STATE(cSEROUT)
#define fSERIN        CANDY_STATE(fSERIN)
#define bSERIN        CANDY_STATE(bSERIN)
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
#define fKeyPressed   CANDY_STATE(fKeyPressed)
#define bp            CANDY_STATE(bp)
#define wShiftChanged CANDY_STATE(wShiftChanged)
#define fTrace        CANDY_STATE(fTrace)
#define mdXLXE        CANDY_STATE(mdXLXE)
#define cntTick       CANDY_STATE(cntTick)
#define wFrame        CANDY_STATE(wFrame)
#define wScan         CANDY_STATE(wScan)
#define wLeft         CANDY_STATE(wLeft)
#define ramtop        CANDY_STATE(ramtop)
#define fStop         CANDY_STATE(fStop)
#define wStartScan    CANDY_STATE(wStartScan)
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
#define fWait         CANDY_STATE(fWait)
#define fFetch        CANDY_STATE(fFetch)
#define bCartType     CANDY_STATE(bCartType)
#define btickByte     CANDY_STATE(btickByte)
#define bshftByte     CANDY_STATE(bshftByte)
#define irqPokey      CANDY_STATE(irqPokey)
#define iXESwap       CANDY_STATE(iXESwap)
#define rgbXLExtMem   CANDY_STATE(rgbXLExtMem)

// if present, here is where these would be
#define SELF_SIZE 2048
#define C000_SIZE 4096
#define D800_SIZE 10240
#define XE_SIZE 16384

#define rgbSwapSelf   &CANDY_STATE(rgbXLExtMem)
#define rgbSwapC000   (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE)
#define rgbSwapD800   (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE + C000_SIZE)

#define rgbXEMem      (&CANDY_STATE(rgbXLExtMem) + SELF_SIZE + C000_SIZE + D800_SIZE)

#include "6502.h"

//
// Shift key status bits (are returned by PC BIOS)
//

#define wRCtrl      0x80    // specifically, right control
#define wCapsLock 0x40
#define wNumLock  0x20
#define wScrlLock 0x10
#define wAlt      0x08
#define wCtrl     0x04
#define wAnyShift 0x03


//
// Platform specific stuff
//

#define pbtick _pbtick(iVM)
__inline BYTE *_pbtick(int iVM)
{
    btickByte = (BYTE)(GetTickCount() / 50);
    return &btickByte;
}
#define pbshift _pbshift(iVM)
__inline BYTE *_pbshift(int iVM)
{
    return &bshftByte;
}

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

#define POT0    rgbMem[0xD200]
#define POT1    rgbMem[0xD201]
#define POT2    rgbMem[0xD202]
#define POT3    rgbMem[0xD203]
#define POT4    rgbMem[0xD204]
#define POT5    rgbMem[0xD205]
#define POT6    rgbMem[0xD206]
#define POT7    rgbMem[0xD207]

#define ALLPOT  rgbMem[0xD208]
#define KBCODE  rgbMem[0xD209]
#define RANDOM  rgbMem[0xD20A]

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


//
// Function prototypes
//

void Interrupt(int, BOOL);
void CheckKey(int);
void UpdatePorts(int);
void SIOV(int);
BYTE SIOReadSector(int);
void DeleteDrive(int, int);
BOOL AddDrive(int, int, BYTE *);
void CreateDMATables();
void CreatePMGTable();
void PSLPrepare(int);    // call at the beginning of the scan line to get the mode
BOOL ProcessScanLine(int);
void ForceRedraw(int);
BOOL __cdecl SwapMem(int, BYTE mask, BYTE flags);
void InitBanks(int);
void CchDisAsm(int, unsigned int *puMem);
void CchShowRegs(int);
void ControlKeyUp8(int);

extern int fXFCable;    // !!! left uninitialized and used

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

BOOL __cdecl InstallAtari(int, PVMINFO, int);
BOOL __cdecl UnInstallAtari(int);
BOOL __cdecl InitAtari(int);
BOOL __cdecl UninitAtari(int);
BOOL __cdecl InitAtariDisks(int);
BOOL __cdecl MountAtariDisk(int, int);
BOOL __cdecl UninitAtariDisks(int);
BOOL __cdecl UnmountAtariDisk(int, int);
BOOL __cdecl WarmbootAtari(int);
BOOL __cdecl ColdbootAtari(int);
BOOL __cdecl SaveStateAtari(int, char **, int *);
BOOL __cdecl LoadStateAtari(int, char *, int);
BOOL __cdecl DumpHWAtari(int);
BOOL __cdecl TraceAtari(int, BOOL, BOOL);
BOOL __cdecl ExecuteAtari(int, BOOL, BOOL);
BOOL __cdecl KeyAtari(int, HWND, UINT, WPARAM, LPARAM);
BOOL __cdecl DumpRegsAtari(int);
BOOL __cdecl MonAtari(int);
BYTE __cdecl PeekBAtari(int, ADDR addr);
WORD __cdecl PeekWAtari(int, ADDR addr);
ULONG __cdecl PeekLAtari(int, ADDR addr);
BOOL  __cdecl PokeLAtari(int, ADDR addr, ULONG l);
BOOL  __cdecl PokeWAtari(int, ADDR addr, WORD w);
BOOL  __cdecl PokeBAtari(int, ADDR addr, BYTE b);


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
