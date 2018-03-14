
/***************************************************************************

    ASM68K.H

    - 680x0 assembler/disassember definitions

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    05/15/2001  darekm      last update

***************************************************************************/

#ifndef ASM68K_H
#define ASM68K_H

#pragma once

#include "asmcom.h"

//
// Disassembly flags
//
// Used by the disassembler to pick apart an instructions for display
//


typedef enum
{
    SIZE_B = 0,
    SIZE_W,
    SIZE_L,
    SIZE_X,
    SIZE_S,
    SIZE_D,
    SIZE_E,
    SIZE_P,
    SIZE_BAD
} ESIZE;


//
// 68K instruction operand size
// Any other value must be treated as an error
//

typedef enum
{
    eNoSize = ' ',  // instruction has no operands or sizes are implied
    eSizeB  = 'B',  // operands are of size BYTE
    eSizeW  = 'W',  // operands are of size WORD
    eSizeL  = 'L',  // operands are of size LONG
    eSizeBI = 'b',  // operands are of size BYTE (implied, size not displayed)
    eSizeWI = 'w',  // operands are of size WORD (implied, size not displayed)
    eSizeLI = 'l',  // operands are of size LONG (implied, size not displayed)
    eSize76 = '3',  // operand sizes are encoded in bits 7 and 6 (B W L -)
    eSize6  = '6',  // operand size is encoded in bit 6 (W L)
    eSize7  = '7',  // operand size is encoded in bit 7 (W L)
    eSize8  = '8',  // operand size is encoded in bit 8 (W L)
    eSize90 = '9',  // operand sizes are encoded in bits 10 and 9 (B W L -)
    eSizeFP = 'F',  // operand size is encoded in xword 14/12..10 (FPU only)
} ESIZE68K;


//
// 68K field types
// Any other value must be treated as an error
//

typedef enum
{
    eNone   = ' ',  // no operand

    eCCR    = 'C',  // implied CCR
    eSR     = 'S',  // implied SR
    eUSP    = 'U',  // implied USP
    eSP     = '7',  // implied A7 (not displayed)

    eImm3   = 'Q',  // 3-bit field op[11..9]    immediate value 1-8
    eImm8   = 'B',  // 8-bit field xop[7..0]    immediate 8-bit value
    eImm16  = 'W',  // 8-bit field xop[15..0]   immediate 16-bit value
    eImm32  = 'L',  // 8-bit field xop2[31..0]  immediate 32-bit value
    eImm    = '#',  // 8, 16, or 32 bit immediate value (as in ADDI)
    eQ8     = 'q',  // 8-bit field op[7..0]     8-bit constant for MOVEQ
    eD8     = 'b',  // 8-bit field op[7..0]     8-bit displacement
    eD16    = 'w',  // 16-bit field xop[15..0]  16-bit displacement
    eD32    = 'l',  // 32-bit field xop2[31..0] 32-bit displacement
    eS16    = 's',  // 16-bit field xop[15..0]  16-bit displacement

    eVec3   = '3',  // 3-bit field op[2..0]     immediate value 0-7
    eVec4   = '4',  // 4-bit field op[3..0]     immediate value 0-15

    eDy     = 'y',  // 3-bit field op[2..0]     Dy as in ADDX Dy,Dx
    eDx     = 'x',  // 3-bit field op[11..9]    Dx as in ADD Dx,ea
    eAx     = 'A',  // 3-bit field op[11..9]    Ax as in ADDA ea,Ax
    eAy     = 'a',  // 3-bit field op[2..0]     Ay as in LINK
    eAxi    = 'X',  // 3-bit field op[11..9]    Axi as in CMPM (Ay)+,(Ax)+
    eAyi    = 'Y',  // 3-bit field op[0..2]     Ayi as in CMPM (Ay)+,(Ax)+
    edAx    = 'z',  // 3-bit field op[11..9]    dAx as in ADDX -(Ay),-(Ax)
    edAy    = 'Z',  // 3-bit field op[0..2]     dAy as in ADDX -(Ay),-(Ax)
    eDn     = 'N',  // 3-bit field xop[14..12]  Dn in bitfield ops
    eRn     = 'k',  // 4-bit field xop[15..12]  Rn as in CHK2 CAS2
    eRn2    = '2',  // 4-bit field xop2[15..12] Rn2 as in CAS2

    eDc     = 'o',  // 3-bit field xop[2..0]    CAS
    eDu     = 'p',  // 3-bit field xop[8..6]    CAS

    eDc1Dc2 = 'O',  // 6-bit field xop[2..0]xop2[2..0]      CAS2
    eDu1Du2 = 'P',  // 6-bit field xop[8..6]xop2[8..6]      CAS2
    eRn1Rn2 = ':',  // 6-bit field xop[14..12]xop2[14..12]  CAS2

    eDhDl   = '*',  // 6-bit field xop[2..0]xop[14..12]    MUL*.L
    eDrDq   = '/',  // 6-bit field xop[2..0]xop[14..12]    DIV*.L

    eRegsS  = '<',  // 16-bit field xop[15..0]  source reg list for MOVEM
    eRegsD  = '>',  // 16-bit field xop[15..0]  dest reg list for MOVEM

    eSPR68  = 'r',  // 12-bit field xop[11..0]  special registers for MOVEC

    eEaAll  = 'E',  // 6-bit field op[5..0]     Dn An mem An+ -An imm PC
    eEaAllFP= 'F',  // 6-bit field op[5..0]     Dn    mem An+ -An imm PC
    eEaData = 'd',  // 6-bit field op[5..0]     Dn    mem An+ -An imm PC
    eEaCntl = 'n',  // 6-bit field op[5..0]           mem             PC
    eEaBitf = 'I',  // 6-bit field op[5..0]     Dn    mem             PC
    eEaBitfA= 'i',  // 6-bit field op[5..0]     Dn    mem
    eEaPred = '-',  // 6-bit field op[5..0]           mem     -An
    eEaPost = '+',  // 6-bit field op[5..0]           mem An+         PC
    eEaAlt  = 'V',  // 6-bit field op[5..0]     Dn An mem An+ -An
    eEaDAlt = 'v',  // 6-bit field op[5..0]     Dn    mem An+ -An
    eEaMAlt = 'M',  // 6-bit field op[5..0]           mem An+ -An

    eEaDest = 'D',  // 6-bit field op[11..6]    Dn    mem An+ -An

    eCC     = 'c',  // 4-bit field op[11..8]    condition (Bcc DBcc TRAPcc)
    eFCC    = 'f',  // 6-bit field xop[5..0]    FP condition
    eBF     = 't',  // 3-bit field op[10..8]    bitfield operation
    eSH     = 'h',  // 3-bit field op[4..3/8]   shift type and direction (reg)
    eSHM    = 'H',  // 3-bit field op[10..8]    shift type and direction (mem)

} EOP68K;

// pack the operand size, instruction length, and operands into a ULONG wfDis

#define _68kOp(size,opn1,opn2,opn3) \
    (((size) << 24) | ((opn1) << 16) | ((opn2) << 8) | (opn3))

// extract the various fields from wfDis

#define _68kSiz(op)   (((op) >> 24) & 255)
#define _68kOp1(op)   (((op) >> 16) & 255)
#define _68kOp2(op)   (((op) >>  8) & 255)
#define _68kOp3(op)   (((op) >>  0) & 255)
#define _68kOpN(op,n) (((op) >> (16-((n)<<3))) & 255)


//
// FAny/FAlt/FControl/FMem/FData mode, reg, size
//
// Return 1 if the addressing mode is valid, or 0 if it is not
//

BOOL __inline FAny(ULONG mode, ULONG reg, ULONG size)
{
	if ((mode == 1) && (size == 0))
		return 0;

	if ((mode >= 7) && (reg >= 5))
		return 0;

	return 1;
}

BOOL __inline FAlt(ULONG mode, ULONG reg, ULONG size)
{
	if ((mode == 1) && (size == 0))
		return 0;

	if ((mode >= 7) && (reg >= 2))
		return 0;

	return 1;
}

BOOL __inline FData(ULONG mode, ULONG reg, ULONG size)
{
	if (mode == 1)
		return 0;

	if ((mode >= 7) && (reg >= 4))
		return 0;

	return 1;
}

BOOL __inline FDataAlt(ULONG mode, ULONG reg, ULONG size)
{
	if (mode == 1)
		return 0;

	if ((mode >= 7) && (reg >= 2))
		return 0;

	return 1;
}

BOOL __inline FMem(ULONG mode, ULONG reg, ULONG size)
{
	if (mode < 2)
		return 0;

	if ((mode >= 7) && (reg >= 5))
		return 0;

	return 1;
}

BOOL __inline FControl(ULONG mode, ULONG reg, ULONG size)
{
	if ((mode < 2) || (mode == 3) || (mode == 4))
		return 0;

	if ((mode >= 7) && (reg >= 5))
		return 0;

	return 1;
}


//
// Special register enumeration.
//

typedef enum
{
    sprSFC  = 0x000,
    sprDFC  = 0x001,
    sprCACR = 0x002,
    sprTC   = 0x003,
    sprITT0 = 0x004,
    sprITT1 = 0x005,
    sprDTT0 = 0x006,
    sprDTT1 = 0x007,

    sprUSP  = 0x800,
    sprVBR  = 0x801,
    sprCAAR = 0x802,
    sprMSP  = 0x803,
    sprISP  = 0x804,
    sprMMSR = 0x805,
    sprURP  = 0x806,
    sprSRP  = 0x807,
} ESPR68K;

//
// Condition register enumeration
//

typedef enum
{
    CCR_C  = 0,
    CCR_V,
    CCR_Z,
    CCR_N,
    CCR_X,
} ECCR;

//
// Branch control enumeration
//

typedef enum
{
    Bcc_F,
    Bcc_T,
    Bcc_NE,
    Bcc_EQ,
    Bcc_GT,
    Bcc_LT,
    Bcc_GE,
    Bcc_LE,
    Bcc_HI,
    Bcc_LS,
    Bcc_PL,
    Bcc_MI,
    Bcc_VC,
    Bcc_VS,
    Bcc_CC,
    Bcc_CS,
} EBcc;


//
// The 68020 full format extension word and 68000 short format word
//

#pragma pack(push, 1)

typedef union FFEW
{
    struct
        {
        WORD   IIS:3;   // I/IS
        WORD   :1;
        WORD   BDSIZ:2; // base displacement size
        WORD   IS:1;    // index suppress
        WORD   BS:1;    // base register suppress
        WORD   fFull:1;
        WORD   SCALE:2; // 00 = 1, 01 = 2, 10 = 4, 11 = 8
        WORD   WL:1;    // 0 = short, 1 = long index
        WORD   ireg:4;  // index register
        };

    short  d8:8;        // 8-bit displacemt for short format
    short  w;
} FFEW;

#pragma pack(pop)

//
// the possible values of the 6-bit mod/reg field
//

typedef enum
{
mr_D0   = 0,            // Dn
mr_D7   = 7,            // Dn
mr_A0   = 8,            // An
mr_A7   = 15,           // An
mr_A0i  = 16,           // (An)
mr_A7i  = 23,           // (An)
mr_A0pi = 24,           // (An)+
mr_A7pi = 31,           // (An)+
mr_pdA0 = 32,           // -(An)
mr_pdA7 = 39,           // -(An)
mr_dA0  = 40,           // d(An)
mr_dA7  = 47,           // d(An)
mr_A0ix = 48,           // d(An,Rn)
mr_A7ix = 55,           // d(An,Rn)
mr_absw = 56,           // abs.w
mr_absl = 57,           // abs.l
mr_dPC  = 58,           // d(PC)
mr_PCix = 59,           // d(PC,Rn)
mr_imm  = 60,           // immediate
} EMODREG;

//
// the decoded operand
//

#pragma pack(push, 1)

typedef struct OPN
{
    BYTE    reg:4;      // register index, 0..15 corresponds to D0..D7/A0..A7
    BYTE    reg2:4;     // second register index in a register pair
    BYTE    type;       // operand type (this is a field type byte)
    BYTE    size;       // size (SIZE_B, etc)
    BYTE    res;
    WORD    instr;      // instruction word (same for all instruction operands)

    union
        {
        char rgb[10];   // 10 byte float immediates
        double d;       // 8-byte float immediates
        float  f;       // 4-byte float immediates
        int    disp;    // 32-bit signed displacement
        ULONG  addr;    // 32-bit absolute address
        ULONG  l;       // 32-bit unsigned immediate
        WORD   xword;   // 16-bit extension word

        struct
            {
            int    bd;  // 32-bit signed base displacement
            int    od;  // 32-bit signed outer displacement
            FFEW   ew;  // full format extension word
            };
        };
} OPN, *POPN;

#pragma pack(pop)


// max number of 68K instruction operands (comma separated, not unique fields)

#define MAX_OPN68   3

#define M1 (0xFFFFFFFF)

int Disasm68K
(
    unsigned const int    lPC,      // Program Counter
    unsigned const char * pbCode,   // byte stream to disassemble
    char                * buffer,   // buffer in which to disassemble
    int                   bufLen,   // Length of buffer.
    char                * symName   // symbol name to use in disassembly.
);

#endif

