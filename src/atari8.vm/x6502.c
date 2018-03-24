
///////////////////////////////////////////////////////////////////////////////
//
// X6502.C
//
// 6502 interpreter engine for Atari 800 emulator
//
// Copyright (C) 1986-2015 by Darek Mihocka. All Rights Reserved.
// Branch Always Software. http://www.emulators.com/
//
// 09/11/2012   darekm      convert from ASM to C
// 02/05/2015   darekm      convert to handlers and use macros
//
///////////////////////////////////////////////////////////////////////////////

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

#include "atari800.h"

#include "stdint.h"

PFN pfnPokeB;

// the 8 status bits must be together and in the same order as in the 6502

uint8_t    srN;
uint8_t    srV;
uint8_t    srB;
uint8_t    srD;
uint8_t    srI;
uint8_t    srZ;
uint8_t    srC;
uint8_t    pad;

// jump tables (when used)
typedef void (__fastcall * PFNOP)(CANDYHW *vpcandyCur);
PFNOP jump_tab_RO[256];
PFNOP jump_tab[256];

BYTE    rgbRainbow[256*3] =
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

//////////////////////////////////////////////////////////////////
//
// Constants
//
//////////////////////////////////////////////////////////////////

// exit codes from interpreter

enum
{
EXIT_BREAK      =  0,
EXIT_INVALID    =  1,
EXIT_ALTKEY     =  2,
};

// Effective Address calculation modes for mdEA

enum
{
EAimm = 0,
EAzp  = 1,
EAabs = 2,
};


int __cdecl xprintf(const char *format, ...)
{
    va_list arglist;
    char rgch[512];
    int retval;
    DWORD i;
    static HFILE h = HFILE_ERROR;

    va_start(arglist, format);

    retval = wvsprintf(rgch, format, arglist);

//  if (v.fDebugMode)
        {
        OFSTRUCT ofs;

        if (h == HFILE_ERROR)
            h = OpenFile("\\GEMUL8R.LOG", &ofs, OF_CREATE);

        if (1 && retval)
            WriteFile(h, rgch, retval, &i, NULL);
        }

    if (1 && retval && vi.pszOutput)
        strcat(vi.pszOutput, rgch);

    if (1 && retval)
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), rgch, retval, &i, NULL);

    return retval;
}

uint8_t READ_BYTE(uint32_t ea)
{
//printf("READ_BYTE: %04X returning %02X\n", ea, rgbMem[ea]);

	if (ea == 0xD20A) {
		// we've been asked for a random number. How many times would the poly counter have advanced?
		// !!! we don't cycle count, so we are advancing once every instruction, which averages 4 cycles or so
		// !!! this assumes 30 instructions per scanline always (not true) and that we quit when wLeft == 0 (not true) 
		int cur = (wFrame * 262 * 30 + wScan * 30 + (30 - wLeft));
		int delta = cur - random17last;
		random17last = cur;
		random17pos = (random17pos + delta) % 0x1ffff;
		RANDOM = poly17[random17pos];
	}

    return rgbMem[ea];
}

uint16_t READ_WORD(uint32_t ea)
{
    return rgbMem[ea] | (rgbMem[ea+1] << 8);
}


void WRITE_BYTE(uint32_t ea, uint8_t val)
{
//if (ea > 65535) printf("ea too large! %08X\n", ea);
//printf("WRITE_BYTE:%04X writing %02X\n", ea, val);

    rgbMem[ea] = val;

    return; // 0;
}

void WRITE_WORD(uint32_t ea, uint16_t val)
{
    rgbMem[ea + 0] = ((val >> 0) & 255);
    rgbMem[ea + 1] = ((val >> 8) & 255);

    return; // 0;
}


//////////////////////////////////////////////////////////////////
//
// Macro definitions
//
//////////////////////////////////////////////////////////////////

#define HANDLER_END()	if ((--wLeft) > 0) (*jump_tab_RO[READ_BYTE(regPC++)])(pcandy); } 
						//--wLeft; (*jump_tab_RO[READ_BYTE(regPC++)])(pcandy); }
							
// don't let a scan line end until there's an instruction that affects the PC !!! We may do >>30 instructions!
#define HANDLER_END_FLOW()  if ((--wLeft) > 0)      (*jump_tab_RO[READ_BYTE(regPC++)])(pcandy); }

#define HANDLER(opcode) void __fastcall opcode (CANDYHW *pcandy) { \
  countInstr++; \
  __assume(vpcandyCur == pcandy);

#if 0
    xprintf("PC:%04X A:%02X X:%02X Y:%02X SP:%02X  ", \
        regPC-1, regA, regX, regY, regSP); \
    xprintf("%c%c_%c%c%c%c%c  ", \
        (srN & 0x80)  ? 'N' : '.', \
        (srV != 0)    ? 'V' : '.', \
        (regP & BBIT) ? 'B' : '.', \
        (regP & DBIT) ? 'D' : '.', \
        (regP & IBIT) ? 'I' : '.', \
        (srZ == 0)    ? 'Z' : '.', \
        (srC != 0)    ? 'C' : '.'); \
    xprintf("\n");
#endif

#define HELPER(opcode) __forceinline __fastcall opcode (CANDYHW *pcandy) { \
  __assume(vpcandyCur == pcandy);

#define HELPER1(opcode, arg1) __forceinline __fastcall opcode (CANDYHW *pcandy, arg1) { \
  __assume(vpcandyCur == pcandy);

#define HELPER2(opcode, arg1, arg2) __forceinline __fastcall opcode (CANDYHW *pcandy, arg1, arg2) { \
  __assume(vpcandyCur == pcandy);

#define HELPER3(opcode, arg1, arg2, arg3) __forceinline __fastcall opcode (CANDYHW *pcandy, arg1, arg2, arg3) { \
  __assume(vpcandyCur == pcandy);


void HELPER(EA_imm)
{
    mdEA = EAimm;
    regEA = READ_BYTE(regPC++);
} }

// zp
void HELPER(EA_zp)
{
    mdEA = EAzp;  regEA = READ_BYTE(regPC++);
} }

// zp,X
void HELPER(EA_zpX)
{
    mdEA = EAzp;  regEA = (READ_BYTE(regPC++) + regX) & 255;
} }

// zp,Y
void HELPER(EA_zpY)
{
    mdEA = EAzp;  regEA = (READ_BYTE(regPC++) + regY) & 255;
} }

// (zp,X)
void HELPER(EA_zpXind)
{
    mdEA = EAabs; regEA = (READ_BYTE(regPC++) + regX) & 255; regEA = READ_WORD(regEA);
} }

// (zp),y
void HELPER(EA_zpYind)
{
    mdEA = EAabs; regEA = READ_BYTE(regPC++); regEA = READ_WORD(regEA) + regY;
} }

// abs
void HELPER(EA_abs)
{
    mdEA = EAabs; regEA = READ_WORD(regPC); regPC += 2;
} }

// abs,X
void HELPER(EA_absX)
{
    mdEA = EAabs; regEA = READ_WORD(regPC) + regX; regPC += 2;
} }

// abs,Y
void HELPER(EA_absY)
{
    mdEA = EAabs; regEA = READ_WORD(regPC) + regY; regPC += 2;
} }

#define ADD_COUT_VEC(sum, op1, op2) \
  (((op1) & (op2)) | (((op1) | (op2)) & (~(sum))))

#define SUB_COUT_VEC(diff, op1, op2) \
  (((~(op1)) & (op2)) | (((~(op1)) ^ (op2)) & (diff)))

#define ADD_OVFL_VEC(sum, op1, op2) \
  ((~((op1) ^ (op2))) & ((op2) ^ (sum)))

#define SUB_OVFL_VEC(diff, op1, op2) \
  (((op1) ^ (op2)) & ((op1) ^ (diff)))

void HELPER1(update_NZ, BYTE reg)
{
    srZ = srN = reg;
} }

void HELPER3(update_C, BYTE sum, BYTE op1, BYTE op2)    { srC = 0x80 & ADD_COUT_VEC(sum, op1, op2); }
}

void HELPER3(update_CC, BYTE diff, BYTE op1, BYTE op2)  { srC = 0x80 & ~SUB_COUT_VEC(diff, op1, op2); }
}

void HELPER3(update_VC, BYTE sum, BYTE op1, BYTE op2)   { srC = 0x80 & ADD_COUT_VEC(sum, op1, op2); \
                                                 srV = 0x80 & ADD_OVFL_VEC(sum, op1, op2); }
}

void HELPER3(update_VCC, BYTE diff, BYTE op1, BYTE op2) { srC = 0x80 & ~SUB_COUT_VEC(diff, op1, op2); \
                                                 srV = 0x80 &  SUB_OVFL_VEC(diff, op1, op2); }
}

void HELPER(AND_com)
{
    regA &= (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA); update_NZ(pcandy, regA);
} }

void HELPER(ORA_com)
{
    regA |= (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA); update_NZ(pcandy, regA);
} }

void HELPER(EOR_com)
{
    regA ^= (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA); update_NZ(pcandy, regA);
} }

void HELPER(LDA_com)
{
    regA = (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA); update_NZ(pcandy, regA);
} }

void HELPER(LDX_com)
{
    regX = (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA); update_NZ(pcandy, regX);
} }

void HELPER(LDY_com)
{
    regY = (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA); update_NZ(pcandy, regY);
} }

void HELPER1(ST_zp, BYTE x)
{
    WRITE_BYTE(regEA, x);
} }

void HELPER1(ST_com, BYTE x)
{
    if (regEA >= ramtop) { regPC--; (*pfnPokeB)(regEA,x); regPC++; } else WRITE_BYTE(regEA, x);
} }

void HELPER1(Jcc_com, int f)
{
    WORD offset = (signed short)(signed char) READ_BYTE(regPC++); if (f) regPC += offset;
} }

void HELPER(BIT_com)
{
    BYTE b = READ_BYTE(regEA);

    srN = b & 0x80;
    srV = b & 0x40;
    srZ = b & regA;
} }

void HELPER(INC_mem)
{
    BYTE b = READ_BYTE(regEA) + 1;

    ST_com(pcandy, b);
    update_NZ(pcandy, b);
} }

void HELPER(DEC_mem)
{
    BYTE b = READ_BYTE(regEA) - 1;

    ST_com(pcandy, b);
    update_NZ(pcandy, b);
} }

void HELPER(ASL_mem)
{
    BYTE b = READ_BYTE(regEA);
    BYTE newC = b & 0x80;

    b += b;
    ST_com(pcandy, b);
    update_NZ(pcandy, b);
    srC = newC;
} }

void HELPER(LSR_mem)
{
    BYTE b = READ_BYTE(regEA);
    BYTE newC = b << 7;

    b = b >> 1;
    ST_com(pcandy, b);
    update_NZ(pcandy, b);
    srC = newC;
} }

void HELPER(ROL_mem)
{
    BYTE b = READ_BYTE(regEA);
    BYTE newC = b & 0x80;

    b += b + (srC != 0);
    ST_com(pcandy, b);
    update_NZ(pcandy, b);
    srC = newC;
} }

void HELPER(ROR_mem)
{
    BYTE b = READ_BYTE(regEA);
    BYTE newC = b << 7;

    b = (b >> 1) | (srC & 0x80);
    ST_com(pcandy, b);
    update_NZ(pcandy, b);
    srC = newC;
} }

// pack the lazy flags state into proper P register form

void HELPER(PackP)
{
    BYTE p = 0x20 | (srN & 0x80) | (srB & 0x10) | (srD & 0x08) | (srI & 0x04);

    p |= srV ? 0x40 : 0;
    p |= srZ ? 0 : 0x02;
    p |= srC >> 7;

    regP = p;
} }

// unpack the P register into lazy flags state

void HELPER(UnpackP)
{
    srN = (regP & 0x80);
    srV = (regP & 0x40);
    srB = (regP & 0x10);
    srD = (regP & 0x08);
    srI = (regP & 0x04);
    srZ = (regP & 0x02) ^ 0x02;   // srZ = !Z
    srC = (regP & 0x01) << 7;
} }

void HELPER1(CMP_com, BYTE reg)
{
    BYTE op2 = (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA);
    BYTE diff = reg - op2;

    update_NZ(pcandy, diff);
    update_CC(pcandy, diff, reg, op2);
} }

void HELPER(ADC_com)
{
    BYTE op2 = (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA);
    BYTE tCF = (srC != 0);
    BYTE sum = regA + op2 + tCF;

    update_VC(pcandy, sum, regA, op2);

    if (srD)
        {
        if (((regA & 0x0F) + (op2 & 0x0F) + tCF) > 9)
            {
            sum += 6;
            }

        if ((regA + op2 + tCF) > 0x99)
            {
            sum += 0x60;
            srC = 0x80;
            }

// if (sum > 0x99)   printf("ADC error: regA = %02X op2 = %02X C = %d  sum = %02X\n", regA, op2, tCF, sum);
// if ((sum&15) > 9) printf("ADC error: regA = %02X op2 = %02X C = %d  sum = %02X\n", regA, op2, tCF, sum);
        }

    regA = sum;
    update_NZ(pcandy, sum);
} }

void HELPER(SBC_com)
{
    BYTE op2 = (mdEA == EAimm) ? (BYTE)regEA : READ_BYTE(regEA);
    BYTE tCF = (srC == 0);
    BYTE diff = regA - op2 - tCF;

    update_VCC(pcandy, diff, regA, op2);

    if (srD)
        {
        if ((regA & 0x0F) < ((op2 & 0x0F) + tCF))
            {
            diff -= 6;
            }

        if (regA < (op2 + tCF))
            {
            diff -= 0x60;
            srC = 0;
            }

// if (diff > 0x99)   printf("SBC error: regA = %02X op2 = %02X C = %d  diff = %02X\n", regA, op2, tCF, diff);
// if ((diff&15) > 9) printf("SBC error: regA = %02X op2 = %02X C = %d  diff = %02X\n", regA, op2, tCF, diff);
        }

    regA = diff;
    update_NZ(pcandy, diff);
} }

void HELPER1(PushByte, BYTE b)
{
    WRITE_BYTE(regSP, b);
    regSP = 0x100 | ((regSP - 1) & 0xFF);
} }

void HELPER1(PushWord, WORD w)
{
    WRITE_BYTE(regSP, w >> 8);
    regSP = 0x100 | ((regSP - 1) & 0xFF);
    WRITE_BYTE(regSP, w & 0xFF);
    regSP = 0x100 | ((regSP - 1) & 0xFF);
} }

BYTE HELPER(PopByte)
{
    BYTE b;

    regSP = 0x100 | ((regSP + 1) & 0xFF);
    b = READ_BYTE(regSP);

    return b;
} }

WORD HELPER(PopWord)
{
    WORD w;

    regSP = 0x100 | ((regSP + 1) & 0xFF);
    w = READ_BYTE(regSP);
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    w |= READ_BYTE(regSP) << 8;

    return w;
} }


// BRK

HANDLER(op00)
{
    HANDLER_END_FLOW();
}

// ORA (zp,X)

HANDLER(op01)
{
    EA_zpXind(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ORA zp

HANDLER(op05)
{
    EA_zp(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ASL zp

HANDLER(op06)
{
    EA_zp(pcandy);
    ASL_mem(pcandy);
    HANDLER_END();
}

// PHP

HANDLER(op08)
{
    PackP(pcandy);
    PushByte(pcandy, regP);
    HANDLER_END();
}

// ORA #

HANDLER(op09)
{
    EA_imm(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ASL A

HANDLER(op0A)
{
    BYTE newC = regA & 0x80;

    regA += regA;
    update_NZ(pcandy, regA);
    srC = newC;
    HANDLER_END();
}

// ORA abs

HANDLER(op0D)
{
    EA_abs(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ASL abs

HANDLER(op0E)
{
    EA_abs(pcandy);
    ASL_mem(pcandy);
    HANDLER_END();
}

// BPL rel8

HANDLER(op10)
{
    Jcc_com(pcandy, (srN & 0x80) == 0);
    HANDLER_END_FLOW();
}

// ORA (zp),Y

HANDLER(op11)
{
    EA_zpYind(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ORA zp,X

HANDLER(op15)
{
    EA_zpX(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ASL zp,X

HANDLER(op16)
{
    EA_zpX(pcandy);
    ASL_mem(pcandy);
    HANDLER_END();
}

// CLC

HANDLER(op18)
{
    srC = 0;
    HANDLER_END();
}

// ORA abs,Y

HANDLER(op19)
{
    EA_absY(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ORA abs,X

HANDLER(op1D)
{
    EA_absX(pcandy);
    ORA_com(pcandy);
    HANDLER_END();
}

// ASL abs,X

HANDLER(op1E)
{
    EA_absX(pcandy);
    ASL_mem(pcandy);
    HANDLER_END();
}

// JSR abs

HANDLER(op20)
{
    EA_abs(pcandy);

    PushWord(pcandy, regPC - 1);

    regPC = regEA;
	
    if ((regPC == 0xE459) && ((mdXLXE == 0) || (wPBDATA & 1)))
        {
        // this is our SIO hook!

        fSIO = 1;
        wLeft = 1;
        }

    else if ((mdXLXE != 0) && (regPC >= 0xD700) && (regPC <= 0xD7FF))
        {
        // this is our XE BUS hook!

        fSIO = 1;
        wLeft = 1;
        }

    HANDLER_END_FLOW();
}

// AND (zp,X)

HANDLER(op21)
{
    EA_zpXind(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// BIT zp

HANDLER(op24)
{
    EA_zp(pcandy);
    BIT_com(pcandy);
    HANDLER_END();
}

// AND zp

HANDLER(op25)
{
    EA_zp(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// ROL zp

HANDLER(op26)
{
    EA_zp(pcandy);
    ROL_mem(pcandy);
    HANDLER_END();
}

// PLP

HANDLER(op28)
{
    regP = PopByte(pcandy);
    UnpackP(pcandy);
    HANDLER_END();
}

// AND #

HANDLER(op29)
{
    EA_imm(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// ROL A

HANDLER(op2A)
{
    BYTE newC = regA & 0x80;

    regA += regA + (srC != 0);
    update_NZ(pcandy, regA);
    srC = newC;
    HANDLER_END();
}

// BIT abs

HANDLER(op2C)
{
    EA_abs(pcandy);
    BIT_com(pcandy);
    HANDLER_END();
}

// AND abs

HANDLER(op2D)
{
    EA_abs(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// ROL abs

HANDLER(op2E)
{
    EA_abs(pcandy);
    ROL_mem(pcandy);
    HANDLER_END();
}

// BMI rel8

HANDLER(op30)
{
    Jcc_com(pcandy, (srN & 0x80) != 0);
    HANDLER_END_FLOW();
}

// AND (zp),Y

HANDLER(op31)
{
    EA_zpYind(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// AND zp,X

HANDLER(op35)
{
    EA_zpX(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// ROL zp,X

HANDLER(op36)
{
    EA_zpX(pcandy);
    ROL_mem(pcandy);
    HANDLER_END();
}

// SEC

HANDLER(op38)
{
    srC = 0x80;
    HANDLER_END();
}

// AND abs,Y

HANDLER(op39)
{
    EA_absY(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// AND abs,X

HANDLER(op3D)
{
    EA_absX(pcandy);
    AND_com(pcandy);
    HANDLER_END();
}

// ROL abs,X

HANDLER(op3E)
{
    EA_absX(pcandy);
    ROL_mem(pcandy);
    HANDLER_END();
}

// RTI

HANDLER(op40)
{
    regP = PopByte(pcandy);
    UnpackP(pcandy);
    srB = 0;
    regPC = PopWord(pcandy);
    HANDLER_END_FLOW();
}

// EOR (zp,X)

HANDLER(op41)
{
    EA_zpXind(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// EOR zp

HANDLER(op45)
{
    EA_zp(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// LSR zp

HANDLER(op46)
{
    EA_zp(pcandy);
    LSR_mem(pcandy);
    HANDLER_END();
}

// PHA

HANDLER(op48)
{
    PushByte(pcandy, regA);
    HANDLER_END();
}

// EOR #

HANDLER(op49)
{
    EA_imm(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// LSR A

HANDLER(op4A)
{
    BYTE newC = regA << 7;

    regA >>= 1;
    update_NZ(pcandy, regA);
    srC = newC;
    HANDLER_END();
}

// JMP abs

HANDLER(op4C)
{
    EA_abs(pcandy);

    regPC = regEA;

    if ((regPC == 0xE459) && ((mdXLXE == 0) || (wPBDATA & 1)))
        {
        // this is our SIO hook!

        fSIO = 1;
        wLeft = 1;
        }

    else if ((mdXLXE != 0) && (regPC >= 0xD700) && (regPC <= 0xD7FF))
        {
        // this is our XE BUS hook!

        fSIO = 1;
        wLeft = 1;
        }

    HANDLER_END_FLOW();
}

// EOR abs

HANDLER(op4D)
{
    EA_abs(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// LSR abs

HANDLER(op4E)
{
    EA_abs(pcandy);
    LSR_mem(pcandy);
    HANDLER_END();
}

// BVC rel8

HANDLER(op50)
{
    Jcc_com(pcandy, srV == 0);
    HANDLER_END_FLOW();
}

// EOR (zp),Y

HANDLER(op51)
{
    EA_zpYind(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// EOR zp,X

HANDLER(op55)
{
    EA_zpX(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// LSR zp,X

HANDLER(op56)
{
    EA_zpX(pcandy);
    LSR_mem(pcandy);
    HANDLER_END();
}

// CLI

HANDLER(op58)
{
    srI = 0;
    HANDLER_END();
}

// EOR abs,Y

HANDLER(op59)
{
    EA_absY(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// EOR abs,X

HANDLER(op5D)
{
    EA_absX(pcandy);
    EOR_com(pcandy);
    HANDLER_END();
}

// LSR abs,X

HANDLER(op5E)
{
    EA_absX(pcandy);
    LSR_mem(pcandy);
    HANDLER_END();
}

// RTS

HANDLER(op60)
{
    regPC = PopWord(pcandy) + 1;

    if ((mdXLXE != 0) && (regPC >= 0xD700) && (regPC <= 0xD7FF))
        {
        // this is our XE BUS hook!

        fSIO = 1;
        wLeft = 1;
        }

    HANDLER_END_FLOW();
}

// ADC (zp,X)

HANDLER(op61)
{
    EA_zpXind(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ADC zp

HANDLER(op65)
{
    EA_zp(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ROR zp

HANDLER(op66)
{
    EA_zp(pcandy);
    ROR_mem(pcandy);
    HANDLER_END();
}

// PLA

HANDLER(op68)
{
    regA = PopByte(pcandy);
    update_NZ(pcandy, regA);
    HANDLER_END();
}

// ADC #

HANDLER(op69)
{
    EA_imm(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ROR A

HANDLER(op6A)
{
    BYTE newC = regA << 7;

    regA = (regA >> 1) | (srC & 0x80);
    update_NZ(pcandy, regA);
    srC = newC;
    HANDLER_END();
}

// JMP (abs)

HANDLER(op6C)
{
    EA_abs(pcandy);
    regPC = READ_WORD(regEA);

    HANDLER_END_FLOW();
}

// ADC abs

HANDLER(op6D)
{
    EA_abs(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ROR abs

HANDLER(op6E)
{
    EA_abs(pcandy);
    ROR_mem(pcandy);
    HANDLER_END();
}

// BVS rel8

HANDLER(op70)
{
    Jcc_com(pcandy, srV != 0);
    HANDLER_END_FLOW();
}

// ADC (zp),Y

HANDLER(op71)
{
    EA_zpYind(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ADC zp,X

HANDLER(op75)
{
    EA_zpX(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ROR zp,X

HANDLER(op76)
{
    EA_zpX(pcandy);
    ROR_mem(pcandy);
    HANDLER_END();
}

// SEI

HANDLER(op78)
{
    srI = 0x04;
    HANDLER_END();
}

// ADC abs,Y

HANDLER(op79)
{
    EA_absY(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ADC abs,X

HANDLER(op7D)
{
    EA_absX(pcandy);
    ADC_com(pcandy);
    HANDLER_END();
}

// ROR abs,X

HANDLER(op7E)
{
    EA_absX(pcandy);
    ROR_mem(pcandy);
    HANDLER_END();
}

// STA (zp,X)

HANDLER(op81)
{
    EA_zpXind(pcandy);
    ST_com(pcandy, regA);
    HANDLER_END();
}

// STY zp

HANDLER(op84)
{
 
   EA_zp(pcandy);
    WRITE_BYTE(regEA, regY);
    HANDLER_END();
}

// STA zp

HANDLER(op85)
{
    EA_zp(pcandy);
    WRITE_BYTE(regEA, regA);
    HANDLER_END();
}

// STX zp

HANDLER(op86)
{
    EA_zp(pcandy);
    WRITE_BYTE(regEA, regX);
    HANDLER_END();
}

// DEY

HANDLER(op88)
{
    update_NZ(pcandy, --regY);
    HANDLER_END();
}

// TXA

HANDLER(op8A)
{
    regA = regX;
    update_NZ(pcandy, regX);
    HANDLER_END();
}

// STY abs

HANDLER(op8C)
{
    EA_abs(pcandy);
    ST_com(pcandy, regY);
    HANDLER_END()
}

// STA abs

HANDLER(op8D)
{
    EA_abs(pcandy);
    ST_com(pcandy, regA);
    HANDLER_END();
}

// STX abs

HANDLER(op8E)
{
    EA_abs(pcandy);
    ST_com(pcandy, regX);
    HANDLER_END();
}

// BCC rel8

HANDLER(op90)
{
    Jcc_com(pcandy, (srC & 0x80) == 0);
    HANDLER_END_FLOW();
}

// STA (zp),Y

HANDLER(op91)
{
    EA_zpYind(pcandy);
    ST_com(pcandy, regA);
    HANDLER_END();
}

// STY zp,X

HANDLER(op94)
{
    EA_zpX(pcandy);
    ST_com(pcandy, regY);
    HANDLER_END();
}

// STA zp,X

HANDLER(op95)
{
    EA_zpX(pcandy);
    ST_com(pcandy, regA);
    HANDLER_END();
}

// STX zp,Y

HANDLER(op96)
{
    EA_zpY(pcandy);
    ST_com(pcandy, regX);
    HANDLER_END();
}

// TYA

HANDLER(op98)
{
    regA = regY;
    update_NZ(pcandy, regY);
    HANDLER_END();
}

// STA abs,Y

HANDLER(op99)
{
    EA_absY(pcandy);
    ST_com(pcandy, regA);
    HANDLER_END();
}

// TXS

HANDLER(op9A)
{
    regSP = regX | 0x100;
    HANDLER_END();
}

// STA abs,X

HANDLER(op9D)
{
    EA_absX(pcandy);
    ST_com(pcandy, regA);
    HANDLER_END();
}

// LDY #

HANDLER(opA0)
{
    EA_imm(pcandy);
    LDY_com(pcandy);
    HANDLER_END();
}

// LDA (zp,X)

HANDLER(opA1)
{
    EA_zpXind(pcandy);
    LDA_com(pcandy);
    HANDLER_END();
}

// LDX #

HANDLER(opA2)
{
    EA_imm(pcandy);
    LDX_com(pcandy);
    HANDLER_END();
}

// LDY zp

HANDLER(opA4)
{
    EA_zp(pcandy);
    LDY_com(pcandy);
    HANDLER_END();
}

// LDA zp

HANDLER(opA5)
{
    EA_zp(pcandy);
    LDA_com(pcandy);
    HANDLER_END();
}

// LDX zp

HANDLER(opA6)
{
    EA_zp(pcandy);
    LDX_com(pcandy);
    HANDLER_END();
}

// TAY

HANDLER(opA8)
{
    regY = regA;
    update_NZ(pcandy, regA);
    HANDLER_END();
}

// LDA #

HANDLER(opA9)
{
    EA_imm(pcandy);
    LDA_com(pcandy);
    HANDLER_END();
}

// TAX

HANDLER(opAA)
{
    regX = regA;
    update_NZ(pcandy, regA);
    HANDLER_END();
}

// LDY abs

HANDLER(opAC)
{
    EA_abs(pcandy);
    LDY_com(pcandy);
    HANDLER_END();
}

// LDA abs

HANDLER(opAD)
{
    EA_abs(pcandy);
	LDA_com(pcandy);
    HANDLER_END();
}

// LDX abs

HANDLER(opAE)
{
    EA_abs(pcandy);
    LDX_com(pcandy);
    HANDLER_END();
}

// BCS rel8

HANDLER(opB0)
{
    Jcc_com(pcandy, (srC & 0x80) != 0);
    HANDLER_END_FLOW();
}

// LDA (zp),Y

HANDLER(opB1)
{
    EA_zpYind(pcandy);
    LDA_com(pcandy);
    HANDLER_END();
}

// LDY zp,X

HANDLER(opB4)
{
    EA_zpX(pcandy);
    LDY_com(pcandy);
    HANDLER_END()
}

// LDA zp,X

HANDLER(opB5)
{
    EA_zpX(pcandy);
    LDA_com(pcandy);
    HANDLER_END()
}

// LDX zp,Y

HANDLER(opB6)
{
    EA_zpY(pcandy);
    LDX_com(pcandy);
    HANDLER_END()
}

// CLV

HANDLER(opB8)
{
    srV = 0;
    HANDLER_END();
}

// LDA abs,Y

HANDLER(opB9)
{
    EA_absY(pcandy);
    LDA_com(pcandy);
    HANDLER_END();
}

// TSX

HANDLER(opBA)
{
    regX = (BYTE)(regSP & 255);
    update_NZ(pcandy, regX);
    HANDLER_END();
}

// LDY abs,X

HANDLER(opBC)
{
    EA_absX(pcandy);
    LDY_com(pcandy);
    HANDLER_END();
}

// LDA abs,X

HANDLER(opBD)
{
    EA_absX(pcandy);
    LDA_com(pcandy);
    HANDLER_END();
}

// LDX abs,Y

HANDLER(opBE)
{
    EA_absY(pcandy);
    LDX_com(pcandy);
    HANDLER_END();
}

// CPY #

HANDLER(opC0)
{
    EA_imm(pcandy);
    CMP_com(pcandy, regY);
    HANDLER_END();
}

// CMP (zp,X)

HANDLER(opC1)
{
    EA_zpXind(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// CPY zp

HANDLER(opC4)
{
    EA_zp(pcandy);
    CMP_com(pcandy, regY);
    HANDLER_END();
}

// CMP zp

HANDLER(opC5)
{
    EA_zp(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// DEC zp

HANDLER(opC6)
{
    EA_zp(pcandy);
    DEC_mem(pcandy);
    HANDLER_END();
}

// INY

HANDLER(opC8)
{
    update_NZ(pcandy, ++regY);
    HANDLER_END();
}

// CMP #

HANDLER(opC9)
{
    EA_imm(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// DEX

HANDLER(opCA)
{
    update_NZ(pcandy, --regX);
    HANDLER_END();
}

// CPY abs

HANDLER(opCC)
{
    EA_abs(pcandy);
    CMP_com(pcandy, regY);
    HANDLER_END();
}

// CMP abs

HANDLER(opCD)
{
    EA_abs(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// DEC abs

HANDLER(opCE)
{
    EA_abs(pcandy);
    DEC_mem(pcandy);
    HANDLER_END()
}

// BNE rel8

HANDLER(opD0)
{
    Jcc_com(pcandy, srZ != 0);
    HANDLER_END_FLOW();
}

// CMP (zp),Y

HANDLER(opD1)
{
    EA_zpYind(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// CMP zp,X

HANDLER(opD5)
{
    EA_zpX(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// DEC zp,X

HANDLER(opD6)
{
    EA_zpX(pcandy);
    DEC_mem(pcandy);
    HANDLER_END();
}

// CLD

HANDLER(opD8)
{
    srD = 0;
    HANDLER_END();
}

// CMP abs,Y

HANDLER(opD9)
{
    EA_absY(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// CMP abs,X

HANDLER(opDD)
{
    EA_absX(pcandy);
    CMP_com(pcandy, regA);
    HANDLER_END();
}

// DEC abs,X

HANDLER(opDE)
{
    EA_absX(pcandy);
    DEC_mem(pcandy);
    HANDLER_END();
}

HANDLER(opDF)
{
    EA_imm(pcandy);
#if 0
    cmp   al,0DFh
    jne   short @F

    ; this is our SIO hook!
    mov   fSIO,1
    jmp   done
@@:
#endif
    HANDLER_END();
}

// CPX #

HANDLER(opE0)
{
    EA_imm(pcandy);
    CMP_com(pcandy, regX);
    HANDLER_END();
}

// SBC (zp,X)

HANDLER(opE1)
{
    EA_zpXind(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// CPX zp

HANDLER(opE4)
{
    EA_zp(pcandy);
    CMP_com(pcandy, regX);
    HANDLER_END();
}

// SBC zp

HANDLER(opE5)
{
    EA_zp(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// INC zp

HANDLER(opE6)
{
    EA_zp(pcandy);
    INC_mem(pcandy);
    HANDLER_END();
}

// INX

HANDLER(opE8)
{
    update_NZ(pcandy, ++regX);
    HANDLER_END();
}

// SBC #

HANDLER(opE9)
{
    EA_imm(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// NOP

HANDLER(opEA)
{
    HANDLER_END();
}

// CPX abs

HANDLER(opEC)
{
    EA_abs(pcandy);
    CMP_com(pcandy, regX);
    HANDLER_END();
}

// SBC abs

HANDLER(opED)
{
    EA_abs(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// INC abs

HANDLER(opEE)
{
    EA_abs(pcandy);
    INC_mem(pcandy);
    HANDLER_END();
}

// BEQ rel8

HANDLER(opF0)
{
    Jcc_com(pcandy, srZ == 0);
    HANDLER_END_FLOW();
}

// SBC (zp),Y

HANDLER(opF1)
{
    EA_zpYind(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// SBC zp,X

HANDLER(opF5)
{
    EA_zpX(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// INC zp,X

HANDLER(opF6)
{
    EA_zpX(pcandy);
    INC_mem(pcandy);
    HANDLER_END();
}

// SED

HANDLER(opF8)
{
    srD = 0x08;
    HANDLER_END();
}

// SBC abs,Y

HANDLER(opF9)
{
    EA_absY(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// SBC abs,X

HANDLER(opFD)
{
    EA_absX(pcandy);
    SBC_com(pcandy);
    HANDLER_END();
}

// INC abs,X

HANDLER(opFE)
{
    EA_absX(pcandy);
    INC_mem(pcandy);
    HANDLER_END();
}

HANDLER(unused)
{
    HANDLER_END();
}

PFNOP jump_tab_RO[256] =
{
    op00,
    op01,
    unused,
    unused,
    unused,
    op05,
    op06,
    unused,
    op08,
    op09,
    op0A,
    unused,
    unused,
    op0D,
    op0E,
    unused,
    op10,
    op11,
    unused,
    unused,
    unused,
    op15,
    op16,
    unused,
    op18,
    op19,
    unused,
    unused,
    unused,
    op1D,
    op1E,
    unused,
    op20,
    op21,
    unused,
    unused,
    op24,
    op25,
    op26,
    unused,
    op28,
    op29,
    op2A,
    unused,
    op2C,
    op2D,
    op2E,
    unused,
    op30,
    op31,
    unused,
    unused,
    unused,
    op35,
    op36,
    unused,
    op38,
    op39,
    unused,
    unused,
    unused,
    op3D,
    op3E,
    unused,
    op40,
    op41,
    unused,
    unused,
    unused,
    op45,
    op46,
    unused,
    op48,
    op49,
    op4A,
    unused,
    op4C,
    op4D,
    op4E,
    unused,
    op50,
    op51,
    unused,
    unused,
    unused,
    op55,
    op56,
    unused,
    op58,
    op59,
    unused,
    unused,
    unused,
    op5D,
    op5E,
    unused,
    op60,
    op61,
    unused,
    unused,
    unused,
    op65,
    op66,
    unused,
    op68,
    op69,
    op6A,
    unused,
    op6C,
    op6D,
    op6E,
    unused,
    op70,
    op71,
    unused,
    unused,
    unused,
    op75,
    op76,
    unused,
    op78,
    op79,
    unused,
    unused,
    unused,
    op7D,
    op7E,
    unused,
    unused,
    op81,
    unused,
    unused,
    op84,
    op85,
    op86,
    unused,
    op88,
    unused,
    op8A,
    unused,
    op8C,
    op8D,
    op8E,
    unused,
    op90,
    op91,
    unused,
    unused,
    op94,
    op95,
    op96,
    unused,
    op98,
    op99,
    op9A,
    unused,
    unused,
    op9D,
    unused,
    unused,
    opA0,
    opA1,
    opA2,
    unused,
    opA4,
    opA5,
    opA6,
    unused,
    opA8,
    opA9,
    opAA,
    unused,
    opAC,
    opAD,
    opAE,
    unused,
    opB0,
    opB1,
    unused,
    unused,
    opB4,
    opB5,
    opB6,
    unused,
    opB8,
    opB9,
    opBA,
    unused,
    opBC,
    opBD,
    opBE,
    unused,
    opC0,
    opC1,
    unused,
    unused,
    opC4,
    opC5,
    opC6,
    unused,
    opC8,
    opC9,
    opCA,
    unused,
    opCC,
    opCD,
    opCE,
    unused,
    opD0,
    opD1,
    unused,
    unused,
    unused,
    opD5,
    opD6,
    unused,
    opD8,
    opD9,
    unused,
    unused,
    unused,
    opDD,
    opDE,
    opDF,
    opE0,
    opE1,
    unused,
    unused,
    opE4,
    opE5,
    opE6,
    unused,
    opE8,
    opE9,
    opEA,
    unused,
    opEC,
    opED,
    opEE,
    unused,
    opF0,
    opF1,
    unused,
    unused,
    unused,
    opF5,
    opF6,
    unused,
    opF8,
    opF9,
    unused,
    unused,
    unused,
    opFD,
    opFE,
    unused,
};

void __cdecl Go6502()
{
    WORD bias = fTrace ? wLeft - 1 : 0;

    wLeft = fTrace ? 1 : wLeft;

    UnpackP(vpcandyCur);

    do
        {
#if 0
        BYTE t, t2;
        PackP();
        t = regP;
        UnpackP();
        PackP();
        t2 = regP;
        if (t != t2) printf("BAD FLAGS STATE\n");
#endif

	        (*jump_tab_RO[READ_BYTE(regPC++)])(vpcandyCur);
        } while (wLeft > 0);

    PackP(vpcandyCur);

    //if (wLeft < 0) wLeft = 0;

    wLeft += bias;
}

