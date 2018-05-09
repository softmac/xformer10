
/***************************************************************************

    68K.H

    - 680x0/PowerPC state definition include file

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    03/28/2008  darekm      beta 4 new REGS layout and TLB model
    03/03/2008  darekm      beta 3 uses new N Z flags layout
    08/27/2007  darekm      convert to new memory API for 9.0 beta 2
    04/26/2002  darekm      last update for SoftMac 8.20

***************************************************************************/

#pragma once

#define HERTZ CLOCKS_PER_SEC

// maximum 9 cards with 7 sets of ROMs each

#define MAXOS 63

// maximum number of virtual machines

#define MAX_VM 16

#pragma pack(push, 4)

// keep these defines in sync with 68000.INC

// Data read/write access is absed on a 64-byte block

#define bitsSpan    (6)
#define cbSpan      (1<<bitsSpan)
#define bitsDataTag (11)
#define dwIndexMask ((1<<bitsDataTag)-1)

#define dwBaseMask  (0xFFFFFFFF<<bitsSpan)
#define iSpan(addr) (((addr)>>bitsSpan)&dwIndexMask)

// Code block sizes and code dispatch is based on 64-byte block

#define bitsHash    (6)
#define cbHashSpan  (1<<bitsHash)

#define dwHashMask  (0xFFFFFFFF<<bitsHash)
#define iHash(addr) (((addr)&0xFFFF)>>bitsHash)

//
// 68000 state referenced off the EDX register
//

typedef struct _sregs
    {
    // Byte offset $00

    ULONG res;  // used to cache N flag
    ULONG USP;
    ULONG SSP;
    ULONG VBR;

    // Byte offset $10

    ULONG PC;
    WORD xword;

    union
        {
        WORD SR;
        BYTE CCR;
        struct
            {
            unsigned short bitC:1;
            unsigned short bitV:1;
            unsigned short bitZ:1;
            unsigned short bitN:1;
            unsigned short bitX:1;
            unsigned short unused3:3;
            unsigned short bitsI:3;
            unsigned short unused2:2;
            unsigned short bitSuper:1;
            unsigned short bitT0:1;
            unsigned short bitTrace:1;
            } rgfsr;
        };

    BYTE    fpSrc;
    BYTE    fpDst;
    BYTE    fpOp;
    BYTE    fpMode;

    ULONG FPIAR;    // Floating Point Instruction Address Register

    // Byte offset $20

    union
        {
        ULONG rgregRn[16]; // use index 8..15 for An registers
        ULONG rgregDn[8];  // use index 0..7 for Dn registers

        struct
            {
            ULONG D0;
            ULONG D1;
            ULONG D2;
            ULONG D3;
            ULONG D4;
            ULONG D5;
            ULONG D6;
            ULONG D7;

            ULONG A0;
            ULONG A1;
            ULONG A2;
            ULONG A3;
            ULONG A4;
            ULONG A5;
            ULONG A6;
            ULONG A7;
            };
        };

    // Byte offset $60

    ULONG flagZ;
    ULONG regPCLv;              // live rPC saved during memory access calls
    ULONG regPCSv;              // saved rPC from last flow control
    ULONG Temp32;

    ULONG counter;
    ULONG cpuCur;
    WORD  vecnum;
    BYTE  flagsVC;
    BYTE  flagX;

    // Byte offset $7C

    struct
        {
        // 16-byte Data Micro-Op (DUOP) structure

        PHNDLR  pfn;            // pointer to handler function
        long    guestPC;        // 32-bit guest PC of instruction
        long    ops;            // 32-bit operand or two packed 16-bit operands
        long    disp32;         // 32-bit displacement or immediate constant
        } duops[65536+64];

    ULONG memtlbRW[2048*2];     // memory mapping TLB for writable access
    ULONG memtlbRO[2048*2];     // memory mapping TLB for read-only access
    ULONG memtlbPC[2048*2];     // memory mapping TLB for code access
    ULONG mappedRanges[65536/cbHashSpan]; // map of ranges of PC dispatch

    // Offsets beyond $7C which are not accessed by ASM code

    union
        {
        BYTE    rgb12[12];
        BYTE    rgb10[10];
        float   fp32;
        double  fp64;
        };

    BYTE SFC;
    BYTE DFC;

    ULONG MSP;      // 68020 MSP

    union
        {
        WORD  FPCR;     // Floating Point Condition Register
        struct _fpcr
            {
            unsigned short :4;
            unsigned short rnd:2;
            unsigned short prec:2;
            unsigned short inex1:1;
            unsigned short inex2:1;
            unsigned short dz:1;
            unsigned short unfl:1;
            unsigned short ovfl:1;
            unsigned short operr:1;
            unsigned short snan:1;
            unsigned short bsun:1;
            } rgffpcr;
        };

    union
        {
        ULONG FPSR;     // Floating Point Status Register
        struct _fpsr
            {
            unsigned :3;
            unsigned aeinex:1;
            unsigned aedz:1;
            unsigned aeunfl:1;
            unsigned aeovfl:1;
            unsigned aeiop:1;

            unsigned esinex1:1;
            unsigned esinex2:1;
            unsigned esdz:1;
            unsigned esunfl:1;
            unsigned esovfl:1;
            unsigned esoperr:1;
            unsigned esnan:1;
            unsigned esbsun:1;

            unsigned quotient:7;
            unsigned S:1;

            unsigned ccnan:1;
            unsigned ccI:1;
            unsigned ccZ:1;
            unsigned ccN:1;
            } rgffpsr;
        };

#ifdef POWERMAC
    union
        {
        ULONG rggpr[32];        // PPC general purpose registers
        struct _gpr
            {
            ULONG GPR0;
            ULONG GPR1;
            ULONG GPR2;
            ULONG GPR3;
            };
        };

    union
        {
        double rgfpr[32];    // PPC floating point registers
        struct _fpr
            {
            double FPR0;
            double FPR1;
            double FPR2;
            double FPR3;
            };
        };

    union
        {
        ULONG rgspr[1024];   // PPC special purpose registers
        struct _spr
            {
            ULONG regMQ;
            ULONG regXER;
            ULONG spr2;
            ULONG spr3;
            ULONG regRTCU;
            ULONG regRTCL;
            ULONG spr6;
            ULONG spr7;
            ULONG regLR;
            ULONG regCTR;
            ULONG spr10;
            ULONG spr11;
            ULONG regCR;
            ULONG regFPCR;
            ULONG spr14;
            ULONG spr15;
            ULONG spr16;
            ULONG spr17;
            ULONG regDSISR;
            ULONG regDAR;
            ULONG spr20;
            ULONG spr21;
            ULONG regDEC;
            ULONG spr23;
            ULONG spr24;
            ULONG regSDR1;
            ULONG regSRR0;
            ULONG regSRR1;
            ULONG spr28;
            ULONG spr29;
            ULONG spr30;
            ULONG spr31;
            ULONG spr32;
            };
        };

    union
        {
        ULONG rgsgr[16];     // PPC segment registers
        struct _sgr
            {
            ULONG SR0;
            };
        };

    ULONG MSR;               // Machine State Register

    // These are PPC interpreter state variables
    // which are used by the C interpreter

    double fp0;
    double fp1;
    double fp2;

    ULONG FPSCR;             // Floating point Status and Control Register

    ULONG code;
    ULONG EAX;
    ULONG EBX;
    ULONG ECX;
    ULONG EDX;
    ULONG EDI;
    ULONG ESI;
    ULONG EBP;

    LONG  resppc[1024-128]; // to 4K align the PPC portion
#endif // POWERMAC
    } REGS;

typedef struct _sregs2
    {
    ULONG TCR;              // 68030/68040 MMU Translation Control Register
    ULONG MMUSR;            // 68030/68040 MMU Status Register
    ULONG CACR;             // 68030/68040 Cache Control Register
    ULONG CAAR;             // 68030 Cache Address Register

    union
        {
        // 68030 MMU registers

        struct
            {
            ULONG TT[2];    // 68030 TT0,TT1
            ULONG SRP30[2]; // 68030 Supervisor Root Pointer
            ULONG CRP30[2]; // 68030 CPU Root Pointer
            };

        // 68040 MMU registers

        struct
            {
            ULONG DTT[2];   // 68040 DTT0,DTT1
            ULONG ITT[2];   // 68040 ITT0,ITT1
            ULONG SRP40;
            ULONG URP40;
            };
        };

    ULONG pad4;
    ULONG pad8;

    char rgbFPx[8*16]; // 8 10-byte floating point registers

    } REGS2;



// History buffer size

#define MAX_HIST (16*1024*1024)

