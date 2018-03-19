
/***************************************************************************

    ATARI800.H

    - Common include file for all Atari 800 VM components.
    - Defines all of the private variables used by the Atari 800 VM.

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    12/07/1998  darekm      PC Xformer 3.8 release

***************************************************************************/

#pragma once

#include "common.h"

// XE is non-zero when 130XE emulation is wanted
#define XE 1

#define MAX_CART_SIZE 16384
char FAR rgbSwapCart[MAX_VM][MAX_CART_SIZE];	// contents of the cartridges, no need to persist

// poly counters used for disortion and randomization (we only ever look at the low bit or byte at most)
BYTE poly4[(1 << 4) - 1];	// stores the sequence of the poly counter
BYTE poly5[(1 << 5) - 1];
BYTE poly9[(1 << 9) - 1];
BYTE poly17[(1 << 17) - 1];
int poly4pos[4], poly5pos[4], poly9pos[4], poly17pos[4]; 	// each voice keeps track of its own poly position
unsigned int random17pos;	// needs to be unsigned
ULONGLONG random17last;	// instruction count last time a random number was asked for
BOOL fPolyValid = FALSE;

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

    BYTE scan;                  // scan line within mode (0..15)
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

    BYTE grafm;
    BYTE sizem;
    BYTE pmbase;
    BYTE fHitclr;
} PMG;

#pragma pack()

//
// All 6502 address space variables are FAR

#pragma pack(4)

typedef struct
{
    // 6502 register context

    WORD m_regPC, m_regSP;
    BYTE m_regA, m_regY, m_regX, m_regP;

    WORD m_regEA;
    BYTE m_mdEA;
    BYTE pad5[5];

    // 6502 address space

    BYTE m_rgbMem[65536];

    // fTrace:  non-zero for single opcode execution
    // fSIO:    non-zero for SIO call
    // mdXLXE:  0 = Atari 400/800, 1 = 800XL, 2 = 130XE
    // cntTick: mode display countdown timer (18 Hz ticks)

    BYTE m_fTrace, m_fSIO, m_mdXLXE, m_cntTick, m_fDebugger;

    WORD m_wFrame, m_wScan;
    signed short m_wLeft;
	
    WORD m_wJoy0X, m_wJoy0Y, m_wJoy1X, m_wJoy1Y;
    WORD m_wJoy0XCal, m_wJoy1XCal, m_wJoy0YCal, m_wJoy1YCal;
    BYTE m_bJoyBut;

    // size of RAM ($A000 or $C000)

    WORD FAR m_ramtop;

    WORD m_fStop;
    WORD m_wStartScan, m_wScanMin, m_wScanMac;

    WORD m_fJoy, m_fSoundOn, m_fAutoStart;

    // virtual guest vertical blanks ("jiffies")

    ULONG m_countJiffies;

    // virtual guest instructions

    ULONG m_countInstr;

    // clock multiplier 

    ULONG m_clockMult;

    SL m_rgsl[262];     // 256 scan line structures
    PMG m_pmg;          // PMG structure (only 1 needed, updated each scan line)

    SL m_sl;            // current scan line display info
    WORD m_cbWidth, m_cbDisp;
    WORD m_hshift;

    BYTE m_iscan;
    BYTE m_scans;
    WORD m_wAddr;

    BYTE m_fWait;       // wait until next VBI
    BYTE m_fFetch;      // fetch next DL instruction
    BYTE m_fDataChanged;

    WORD m_fNMIPending;
    WORD m_fIRQPending;

    BYTE m_btick;       // current value of 18 Hz timer
    BYTE m_btickByte;   // current value of 18 Hz timer
    BYTE m_bshftByte;   // current value of shift state
    BYTE m_cVBI;        // count of VBIs since last tick

	char FAR m_rgbSwapSelf[2048];	// extended XL memory
	char FAR m_rgbSwapC000[4096];
	char FAR m_rgbSwapD800[10240];

#if XE
	char HUGE m_rgbXEMem[4][16384];	// !!! was 16, not 4
	// !!! the actual memory found at rgbMem $4000 in an XE is a duplicate of either this or one of the banks above
	// I could save 16K of persistance data by keeping this inside the proper bank of rgbXEMem
	char FAR m_rgbSwapXEMem[16384];
#endif // XE

} CANDYHW;

#pragma pack()

extern CANDYHW vrgcandy[MAX_VM], *vpcandyCur;

// #define CANDY_STATE(name) vrgcandy[v.iVM].m_##name
// #define CANDY_STATE(name) vrgcandy[0].m_##name
 #define CANDY_STATE(name) vpcandyCur->m_##name

#define rgbMem        CANDY_STATE(rgbMem)
#define regPC         CANDY_STATE(regPC)
#define regSP         CANDY_STATE(regSP)
#define regA          CANDY_STATE(regA)
#define regY          CANDY_STATE(regY)
#define regX          CANDY_STATE(regX)
#define regP          CANDY_STATE(regP)
#define regEA         CANDY_STATE(regEA)
#define mdEA          CANDY_STATE(mdEA)
#define fTrace        CANDY_STATE(fTrace)
#define fSIO          CANDY_STATE(fSIO)
#define mdXLXE        CANDY_STATE(mdXLXE)
#define cntTick       CANDY_STATE(cntTick)
#define fDebugger     CANDY_STATE(fDebugger)
#define wFrame        CANDY_STATE(wFrame)
#define wScan         CANDY_STATE(wScan)
#define wLeft         CANDY_STATE(wLeft)
#define wJoy0X        CANDY_STATE(wJoy0X)
#define wJoy0Y        CANDY_STATE(wJoy0Y)
#define wJoy1X        CANDY_STATE(wJoy1X)
#define wJoy1Y        CANDY_STATE(wJoy1Y)
#define wJoy0XCal     CANDY_STATE(wJoy0XCal)
#define wJoy0YCal     CANDY_STATE(wJoy0YCal)
#define wJoy1XCal     CANDY_STATE(wJoy1XCal)
#define wJoy1YCal     CANDY_STATE(wJoy1YCal)
#define bJoyBut       CANDY_STATE(bJoyBut)
#define ramtop        CANDY_STATE(ramtop)
#define fStop         CANDY_STATE(fStop)
#define wStartScan    CANDY_STATE(wStartScan)
#define wScanMin      CANDY_STATE(wScanMin)
#define wScanMac      CANDY_STATE(wScanMac)
#define fJoy          CANDY_STATE(fJoy)
#define fSoundOn      CANDY_STATE(fSoundOn)
#define fAutoStart    CANDY_STATE(fAutoStart)
#define countJiffies  CANDY_STATE(countJiffies)
#define countInstr    CANDY_STATE(countInstr)
#define clockMult     CANDY_STATE(clockMult)
#define rgsl          CANDY_STATE(rgsl)
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
#define fDataChanged  CANDY_STATE(fDataChanged)
#define fNMIPendingd  CANDY_STATE(fNMIPending)
#define fIRQPending   CANDY_STATE(fIRQPending)
#define btick         CANDY_STATE(btick)
#define btickByte     CANDY_STATE(btickByte)
#define bshftByte     CANDY_STATE(bshftByte)
#define cVBI          CANDY_STATE(cVBI)
#define rgbSwapSelf  CANDY_STATE(rgbSwapSelf)
#define rgbSwapC000  CANDY_STATE(rgbSwapC000)
#define rgbSwapD800  CANDY_STATE(rgbSwapD800)

#if XE
#define rgbXEMem	  CANDY_STATE(rgbXEMem)
#define rgbSwapXEMem  CANDY_STATE(rgbSwapXEMem)
#endif

#include "6502.h"

//
// Shift key status bits (are returned by PC BIOS)
//

#define wCapsLock 0x40
#define wNumLock  0x20
#define wScrlLock 0x10
#define wAlt      0x08
#define wCtrl     0x04
#define wAnyShift 0x03


//
// Platform specific stuff
//

#define pbtick _pbtick()
__inline BYTE FAR *_pbtick()
{
    btickByte = (BYTE)(GetTickCount() / 50);
    return &btickByte;
}
#define pbshift _pbshift()
__inline BYTE FAR *_pbshift()
{
    return &bshftByte;
}

//
// 6502 condition codes in P register: NV_BDIZC
//

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

void mon(void);

void __cdecl Go6502(void);
void Interrupt(void);
void CheckKey(void);
//void ReadJoy(void);
//void MyReadJoy(void);

//void SaveVideo(void);
//void RestoreVideo(void);
//void WaitForVRetrace(void);
void UpdatePorts(void);

//void InitSIOV(int, char **);
void SIOV();
//void DiskConfig(void);

extern int fXFCable;	// !!! left uninitialized and used
//extern unsigned uBaudClock;

//void _SIO_Init(void);
//void _SIO_Calibrate(void);
int _SIOV(char *qch, int wDev, int wCom, int wStat, int wBytes, int wSector, int wTimeout);

//void InitSoundBlaster();

BOOL ProcessScanLine(BOOL);
void ForceRedraw();

void __cdecl SwapMem(BYTE mask, BYTE flags, WORD pc);
void InitBanks(int);
//void ReadROMs();

void CchDisAsm(unsigned int *puMem);
void CchShowRegs(void);

//
// Various ROM data and swap buffers
//

extern char FAR rgbAtariOSB[]; // Atari 400/800 ROMs

extern char FAR rgbXLXEBAS[], FAR rgbXLXED800[];  // XL/XE ROMs
extern char FAR rgbXLXEC000[], FAR rgbXLXE5000[]; // self test ROMs

//
// The 3 hardware modes
//

#define md800 0
#define mdXL  1
#define mdXE  2


//
// Function prototypes of the Atari 800 VM API
//

BOOL __cdecl InstallAtari(int);
BOOL __cdecl InitAtari(int);
BOOL __cdecl UninitAtari(int);
BOOL __cdecl InitAtariDisks(int);
BOOL __cdecl MountAtariDisk(int);
BOOL __cdecl UninitAtariDisks(int);
BOOL __cdecl UnmountAtariDisk(int);
BOOL __cdecl WarmbootAtari(int);
BOOL __cdecl ColdbootAtari(int);
BOOL __cdecl SaveStateAtari(int, char **, int *);
BOOL __cdecl LoadStateAtari(int, char *, int);
BOOL __cdecl DumpHWAtari(char *pch);
BOOL __cdecl TraceAtari();
BOOL __cdecl ExecuteAtari();
BOOL __cdecl KeyAtari(ULONG l);
BOOL __cdecl DumpRegsAtari(void);
BOOL __cdecl DisasmAtari(char *pch, ADDR *pPC);
BYTE __cdecl PeekBAtari(ADDR addr);
WORD __cdecl PeekWAtari(ADDR addr);
ULONG __cdecl PeekLAtari(ADDR addr);
BOOL __cdecl PokeLAtari(ADDR addr, ULONG l);
BOOL __cdecl PokeWAtari(ADDR addr, WORD w);
BOOL __cdecl PokeBAtari(ADDR addr, BYTE b);


//
// Map C runtime file i/o calls to appropriate 32-bit calls
//

// Map to Win32 counterparts

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

#pragma intrinsic(inp)
#pragma intrinsic(outp)

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
