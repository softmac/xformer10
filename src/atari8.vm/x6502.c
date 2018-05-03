
///////////////////////////////////////////////////////////////////////////////
//
// X6502.C
//
// 6502 interpreter engine for Atari 800 emulator
//
// Copyright (C) 2015-2018 by Darek Mihocka. All Rights Reserved.
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

PFNB pfnPeekB;
PFNL pfnPokeB;

// jump tables
typedef void (__fastcall * PFNOP)(int iVM);
PFNOP jump_tab[256];

#if 0
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
            WriteFile((HANDLE)h, rgch, retval, &i, NULL);
        }

    if (1 && retval && vi.pszOutput)
        strcat(vi.pszOutput, rgch);

    if (1 && retval)
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), rgch, retval, &i, NULL);

    return retval;
}
#endif

// these private macros decide which needs to be called - special peek/poke above ramtop, or not

__inline uint8_t READ_BYTE(int iVM, uint32_t ea)
{
    //printf("READ_BYTE: %04X returning %02X\n", ea, rgbMem[ea]);

    if (ea >= ramtop)
        return (*pfnPeekB)(iVM, ea);
    else
        return cpuPeekB(iVM, ea);
}

__inline uint16_t READ_WORD(int iVM, uint32_t ea)
{
    if (ea >= ramtop)
        return (*pfnPeekB)(iVM, ea) | ((*pfnPeekB)(iVM, ea + 1) << 8);
    else
        return cpuPeekB(iVM, ea) | (cpuPeekB(iVM, ea + 1) << 8);
}

__inline void WRITE_BYTE(int iVM, uint32_t ea, uint8_t val)
{
    //printf("WRITE_BYTE:%04X writing %02X\n", ea, val);

    if (ea >= ramtop)
        (*pfnPokeB)(iVM, ea, val);
    else
        cpuPokeB(iVM, ea, val);
}

// we only call this if we know it's < ramtop
__inline void WRITE_WORD(int iVM, uint32_t ea, uint16_t val)
{
    if (ea >= ramtop)
    {
        (*pfnPokeB)(iVM, ea, val & 255);
        (*pfnPokeB)(iVM, ea + 1, ((val >> 8) & 255));
    }
    else
    {
        cpuPokeB(iVM, ea, val & 255);
        cpuPokeB(iVM, ea + 1, ((val >> 8) & 255));
    }
}

//////////////////////////////////////////////////////////////////
//
// Macro definitions
//
//////////////////////////////////////////////////////////////////

// in debug, stop on a breakpoint or if tracing
#ifndef NDEBUG
#define HANDLER_END() if (regPC != bp && !fTrace && wLeft > wNMI) (*jump_tab[READ_BYTE(iVM, regPC++)])(iVM); }
#else
#define HANDLER_END() if (wLeft > wNMI) (*jump_tab[READ_BYTE(iVM, regPC++)])(iVM); }
#endif

// this used to not let a scan line end until there's an instruction that affects the PC
#define HANDLER_END_FLOW() HANDLER_END()

#define HANDLER(opcode) void __fastcall opcode (int iVM) {

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

#define HELPER(opcode) __forceinline __fastcall opcode (int iVM) {

#define HELPER1(opcode, arg1) __forceinline __fastcall opcode (int iVM, arg1) {

#define HELPER2(opcode, arg1, arg2) __forceinline __fastcall opcode (int iVM, arg1, arg2) {

#define HELPER3(opcode, arg1, arg2, arg3) __forceinline __fastcall opcode ( int iVM, arg1, arg2, arg3) {

// For addressing modes where we know we won't be above ramtop and needing special register values, we can directly do a CPU read.
// Otherwise, our READ_ WRITE_ helper functions will check ramtop, but be one comparison/potential branch slower
// In general, reading through the PC is safe for using cpuPeekB. Reading through EA isn't unless it's ZP, etc.

void HELPER(EA_imm)
{
    regEA = cpuPeekB(iVM, regPC++);    // assume < ramtop, if we're executing code in register space, heaven help you anyway
} }

// the Read versions do the indirection through EA to get the contents, the Write versions don't

// zp
void HELPER(EA_zpR)
{
    regEA = cpuPeekB(iVM, regPC++); regEA = cpuPeekB(iVM, regEA);
} }
void HELPER(EA_zpW)
{
    regEA = cpuPeekB(iVM, regPC++);
} }

// zp,X
void HELPER(EA_zpXR)
{
    regEA = (cpuPeekB(iVM, regPC++) + regX) & 255; regEA = cpuPeekB(iVM, regEA);
} }
void HELPER(EA_zpXW)
{
    regEA = (cpuPeekB(iVM, regPC++) + regX) & 255;
} }

// zp,Y
void HELPER(EA_zpYR)
{
    regEA = (cpuPeekB(iVM, regPC++) + regY) & 255; regEA = cpuPeekB(iVM, regEA);
} }
void HELPER(EA_zpYW)
{
    regEA = (cpuPeekB(iVM, regPC++) + regY) & 255;
} }

// (zp,X) - the indexing and zp parts can safely do cpuPeek*
void HELPER(EA_zpXindR)
{
    regEA = (cpuPeekB(iVM, regPC++) + regX) & 255; regEA = cpuPeekW(iVM, regEA); regEA = READ_BYTE(iVM, regEA);
} }
void HELPER(EA_zpXindW)
{
    regEA = (cpuPeekB(iVM, regPC++) + regX) & 255; regEA = cpuPeekW(iVM, regEA);
} }

// (zp),y
void HELPER(EA_zpYindR)
{
    regEA = cpuPeekB(iVM, regPC++); regEA = cpuPeekW(iVM, regEA) + regY; 
    wLeft -= ((BYTE)regEA < (BYTE)(regEA - regY));  // extra cycle for crossing page boundary
    regEA = READ_BYTE(iVM, regEA);
} }
void HELPER(EA_zpYindW)
{
    regEA = cpuPeekB(iVM, regPC++); regEA = cpuPeekW(iVM, regEA) + regY;
} }

// abs
void HELPER(EA_absR)
{
    regEA = cpuPeekW(iVM, regPC); regPC += 2; regEA = READ_BYTE(iVM, regEA);
} }
void HELPER(EA_absW)
{
    regEA = cpuPeekW(iVM, regPC); regPC += 2;
} }

// abs,X
void HELPER(EA_absXR)
{
    regEA = cpuPeekW(iVM, regPC) + regX; regPC += 2; 
    wLeft -= ((BYTE)regEA < (BYTE)(regEA - regX));  // extra cycle for crossing page boundary
    regEA = READ_BYTE(iVM, regEA);
} }
void HELPER(EA_absXW)
{
    regEA = cpuPeekW(iVM, regPC) + regX; regPC += 2;
} }

// abs,Y
void HELPER(EA_absYR)
{
    regEA = cpuPeekW(iVM, regPC) + regY; regPC += 2; 
    wLeft -= ((BYTE)regEA < (BYTE)(regEA - regY));  // extra cycle for crossing page boundary
    regEA = READ_BYTE(iVM, regEA);
} }
void HELPER(EA_absYW)
{
    regEA = cpuPeekW(iVM, regPC) + regY; regPC += 2;
} }

#define ADD_COUT_VEC(sum, op1, op2) \
  (((op1) & (op2)) | (((op1) | (op2)) & (~(sum))))

#define SUB_COUT_VEC(diff, op1, op2) \
  (((~(op1)) & (op2)) | (((~(op1)) ^ (op2)) & (diff)))

#define ADD_OVFL_VEC(sum, op1, op2) \
  ((~((op1) ^ (op2))) & ((op2) ^ (sum)))

#define SUB_OVFL_VEC(diff, op1, op2) \
  (((op1) ^ (op2)) & ((op1) ^ (diff)))

// !!! BUG - in decimal mode, N is set based on what the answer would have been in binary mode
// POLEPOSITION relies on this
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
    regA &= (BYTE)regEA; update_NZ(iVM, regA);
} }

void HELPER(ORA_com)
{
    regA |= (BYTE)regEA; update_NZ(iVM, regA);
} }

void HELPER(EOR_com)
{
    regA ^= (BYTE)regEA; update_NZ(iVM, regA);
} }

void HELPER(LDA_com)
{
    regA = (BYTE)regEA; update_NZ(iVM, regA);
} }

void HELPER(LDX_com)
{
    regX = (BYTE)regEA; update_NZ(iVM, regX);
} }

void HELPER(LDY_com)
{
    regY = (BYTE)regEA; update_NZ(iVM, regY);
} }

void HELPER1(ST_zp, BYTE x)
{
    cpuPokeB(iVM, regEA, x);    // ZP always < ramtop
} }

void HELPER1(ST_com, BYTE x)
{
    WRITE_BYTE(iVM, regEA, x);
} }

void HELPER1(Jcc_com, int f)
{
    // taking a branch is an extra cycle. Crossing a page boundary from the instruction after the branch is an extra cycle.
    WORD offset = (signed short)(signed char)cpuPeekB(iVM, regPC++); wLeft -= 2;
    if (f)
    {
        wLeft -= ((regPC & 0xff00) == ((regPC + offset) & 0xff00)) ? 0 : 1;
        regPC += offset;
        wLeft--;
    }
} }

void HELPER(BIT_com)
{
    srN = regEA & 0x80;
    srV = regEA & 0x40;
    srZ = regEA & regA;
} }

void HELPER(INC_mem)
{
    BYTE b = READ_BYTE(iVM, regEA) + 1;

    ST_com(iVM, b);
    update_NZ(iVM, b);
} }

void HELPER(INC_zp)
{
    BYTE b = cpuPeekB(iVM, regEA) + 1;

    ST_com(iVM, b);
    update_NZ(iVM, b);
} }

void HELPER(DEC_mem)
{
    BYTE b = READ_BYTE(iVM, regEA) - 1;

    ST_com(iVM, b);
    update_NZ(iVM, b);
} }

void HELPER(DEC_zp)
{
    BYTE b = cpuPeekB(iVM, regEA) - 1;

    ST_com(iVM, b);
    update_NZ(iVM, b);
} }

void HELPER(ASL_mem)
{
    BYTE b = READ_BYTE(iVM, regEA);
    BYTE newC = b & 0x80;

    b += b;
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(ASL_zp)
{
    BYTE b = cpuPeekB(iVM, regEA);
    BYTE newC = b & 0x80;

    b += b;
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(LSR_mem)
{
    BYTE b = READ_BYTE(iVM, regEA);
    BYTE newC = b << 7;

    b = b >> 1;
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(LSR_zp)
{
    BYTE b = cpuPeekB(iVM, regEA);
    BYTE newC = b << 7;

    b = b >> 1;
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(ROL_mem)
{
    BYTE b = READ_BYTE(iVM, regEA);
    BYTE newC = b & 0x80;

    b += b + (srC != 0);
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(ROL_zp)
{
    BYTE b = cpuPeekB(iVM, regEA);
    BYTE newC = b & 0x80;

    b += b + (srC != 0);
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(ROR_mem)
{
    BYTE b = READ_BYTE(iVM, regEA);
    BYTE newC = b << 7;

    b = (b >> 1) | (srC & 0x80);
    ST_com(iVM, b);
    update_NZ(iVM, b);
    srC = newC;
} }

void HELPER(ROR_zp)
{
    BYTE b = cpuPeekB(iVM, regEA);
    BYTE newC = b << 7;

    b = (b >> 1) | (srC & 0x80);
    ST_com(iVM, b);
    update_NZ(iVM, b);
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
    BYTE op2 = (BYTE)regEA;
    BYTE diff = reg - op2;

    update_NZ(iVM, diff);
    update_CC(iVM, diff, reg, op2);
} }

void HELPER(ADC_com)
{
    BYTE op2 = (BYTE)regEA;
    BYTE tCF = (srC != 0);
    BYTE sum = regA + op2 + tCF;

    update_VC(iVM, sum, regA, op2);

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
    update_NZ(iVM, sum);
} }

void HELPER(SBC_com)
{
    BYTE op2 = (BYTE)regEA;
    BYTE tCF = (srC == 0);
    BYTE diff = regA - op2 - tCF;

    update_VCC(iVM, diff, regA, op2);

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
    update_NZ(iVM, diff);
} }

void HELPER1(PushByte, BYTE b)
{
    cpuPokeB(iVM, regSP, b);
    regSP = 0x100 | ((regSP - 1) & 0xFF);
} }

void HELPER1(PushWord, WORD w)
{
    cpuPokeB(iVM, regSP, w >> 8);
    regSP = 0x100 | ((regSP - 1) & 0xFF);
    cpuPokeB(iVM, regSP, w & 0xFF);
    regSP = 0x100 | ((regSP - 1) & 0xFF);
} }

BYTE HELPER(PopByte)
{
    BYTE b;

    regSP = 0x100 | ((regSP + 1) & 0xFF);
    b = READ_BYTE(iVM, regSP);

    return b;
} }

WORD HELPER(PopWord)
{
    WORD w;

    regSP = 0x100 | ((regSP + 1) & 0xFF);
    w = READ_BYTE(iVM, regSP);
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    w |= READ_BYTE(iVM, regSP) << 8;

    return w;
} }

__inline void SIOCheck(int iVM)
{
    // !!! This executes instantly, messing with cycle accuracy
    // !!! You can't set a bp on $e459, $e959 or anything inside SIO
    // Some people jump directly to $e959, where $e459 points to on the 800 only
    // OS must be paged in on XL for this to really be SIO
    if ((regPC == 0xe459 || regPC == 0xe959) && (mdXLXE == md800 || (wPBDATA & 1)))
    {
        // this is our SIO hook!
        PackP(iVM);
        SIOV(iVM);  // if we don't do this now, an interrupt hitting at the same time will screw up the stack
        UnpackP(iVM);
    }
    else if ((mdXLXE != md800) && (regPC >= 0xD700) && (regPC <= 0xD7FF))
    {
        // this is our XE BUS hook!
        PackP(iVM);
        SIOV(iVM);
        UnpackP(iVM);
    }
}


// BRK

HANDLER(op00)
{
    // !!! Does BRK or any other INT set the interrupt flag? It is not maskable itself, right?
    PackP(iVM);    // we'll be pushing it
    Interrupt(iVM, TRUE);    // yes, this is a BRK IRQ
    regPC = cpuPeekW(iVM, 0xFFFE);
    wLeft -= 7;
    HANDLER_END_FLOW();
}

// ORA (zp,X)

HANDLER(op01)
{
    EA_zpXindR(iVM);
    ORA_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// NOP zp

HANDLER(op04)
{
    regPC += 1;
    wLeft -= 3;
    HANDLER_END();
}

// ORA zp

HANDLER(op05)
{
    EA_zpR(iVM);
    ORA_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// ASL zp

HANDLER(op06)
{
    EA_zpW(iVM);
    ASL_zp(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// SLO zp (ASL + ORA)

HANDLER(op07)
{
    EA_zpW(iVM);
    ASL_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// PHP

HANDLER(op08)
{
    PackP(iVM);
    PushByte(iVM, regP);
    wLeft -= 3;
    HANDLER_END();
}

// ORA #

HANDLER(op09)
{
    EA_imm(iVM);
    ORA_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// ASL A

HANDLER(op0A)
{
    BYTE newC = regA & 0x80;

    regA += regA;
    update_NZ(iVM, regA);
    srC = newC;
    wLeft -= 2;
    HANDLER_END();
}

// NOP abs

HANDLER(op0C)
{
    regPC += 2;
    wLeft -= 4;
    HANDLER_END();
}

// ORA abs

HANDLER(op0D)
{
    EA_absR(iVM);
    ORA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ASL abs

HANDLER(op0E)
{
    EA_absW(iVM);
    ASL_mem(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// BPL rel8

HANDLER(op10)
{
    Jcc_com(iVM, (srN & 0x80) == 0);
    HANDLER_END_FLOW();
}

// ORA (zp),Y

HANDLER(op11)
{
    EA_zpYindR(iVM);
    ORA_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// SLO (zp),Y

HANDLER(op13)
{
    EA_zpYindR(iVM);
    ASL_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
    wLeft -= 5;     // best guess
    HANDLER_END();
}

// ORA zp,X

HANDLER(op15)
{
    EA_zpXR(iVM);
    ORA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ASL zp,X

HANDLER(op16)
{
    EA_zpXW(iVM);
    ASL_zp(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// SLO zp,X

HANDLER(op17)
{
    EA_zpXW(iVM);
    ASL_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// CLC

HANDLER(op18)
{
    srC = 0;
    wLeft -= 2;
    HANDLER_END();
}

// ORA abs,Y

HANDLER(op19)
{
    EA_absYR(iVM);
    ORA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// NOP

HANDLER(op1a)
{
    wLeft -= 2;
    HANDLER_END();
}

// ORA abs,X

HANDLER(op1D)
{
    EA_absXR(iVM);
    ORA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ASL abs,X

HANDLER(op1E)
{
    EA_absXW(iVM);
    ASL_mem(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// JSR abs

HANDLER(op20)
{
    EA_absW(iVM);

    PushWord(iVM, regPC - 1);

    regPC = regEA;
    wLeft -= 6;

    SIOCheck(iVM);  // SIO hack

    HANDLER_END_FLOW();
}

// AND (zp,X)

HANDLER(op21)
{
    EA_zpXindR(iVM);
    AND_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// BIT zp

HANDLER(op24)
{
    EA_zpR(iVM);
    BIT_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// AND zp

HANDLER(op25)
{
    EA_zpR(iVM);
    AND_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// ROL zp

HANDLER(op26)
{
    EA_zpW(iVM);
    ROL_zp(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// PLP

HANDLER(op28)
{
    regP = PopByte(iVM);
    regP |= 0x10;    // force srB
    UnpackP(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// AND #

HANDLER(op29)
{
    EA_imm(iVM);
    AND_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// ROL A

HANDLER(op2A)
{
    BYTE newC = regA & 0x80;

    regA += regA + (srC != 0);
    update_NZ(iVM, regA);
    srC = newC;
    wLeft -= 2;
    HANDLER_END();
}

// BIT abs

HANDLER(op2C)
{
    EA_absR(iVM);
    BIT_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// AND abs

HANDLER(op2D)
{
    EA_absR(iVM);
    AND_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ROL abs

HANDLER(op2E)
{
    EA_absW(iVM);
    ROL_mem(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// RLA abs

HANDLER(op2F)
{
    EA_absW(iVM);
    ROL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// BMI rel8

HANDLER(op30)
{
    Jcc_com(iVM, (srN & 0x80) != 0);
    HANDLER_END_FLOW();
}

// AND (zp),Y

HANDLER(op31)
{
    EA_zpYindR(iVM);
    AND_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// AND zp,X

HANDLER(op35)
{
    EA_zpXR(iVM);
    AND_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// ROL zp,X

HANDLER(op36)
{
    EA_zpXW(iVM);
    ROL_zp(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// SEC

HANDLER(op38)
{
    srC = 0x80;
    wLeft -= 2;
    HANDLER_END();
}

// AND abs,Y

HANDLER(op39)
{
    EA_absYR(iVM);
    AND_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// AND abs,X

HANDLER(op3D)
{
    EA_absXR(iVM);
    AND_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ROL abs,X

HANDLER(op3E)
{
    EA_absXW(iVM);
    ROL_mem(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// RTI

HANDLER(op40)
{
    regP = PopByte(iVM);
    regP |= 0x10;    // force srB
    UnpackP(iVM);
    regPC = PopWord(iVM);

    // before our DLI the main code was waiting on WSYNC
    // !!! If we are now past the WSYNC point
    if (WSYNC_on_RTI)
        WSYNC_Waiting = TRUE;
    WSYNC_on_RTI = FALSE;

    wLeft -= 6;
    HANDLER_END_FLOW();
}

// EOR (zp,X)

HANDLER(op41)
{
    EA_zpXindR(iVM);
    EOR_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// EOR zp

HANDLER(op45)
{
    EA_zpR(iVM);
    EOR_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// LSR zp

HANDLER(op46)
{
    EA_zpW(iVM);
    LSR_zp(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// PHA

HANDLER(op48)
{
    PushByte(iVM, regA);
    wLeft -= 3;
    HANDLER_END();
}

// EOR #

HANDLER(op49)
{
    EA_imm(iVM);
    EOR_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// LSR A

HANDLER(op4A)
{
    BYTE newC = regA << 7;

    regA >>= 1;
    update_NZ(iVM, regA);
    srC = newC;
    wLeft -= 2;
    HANDLER_END();
}

// JMP abs

HANDLER(op4C)
{
    EA_absW(iVM);

    regPC = regEA;
    wLeft -= 3;

    SIOCheck(iVM);  // SIO hack

    HANDLER_END_FLOW();
}

// EOR abs

HANDLER(op4D)
{
    EA_absR(iVM);
    EOR_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LSR abs

HANDLER(op4E)
{
    EA_absW(iVM);
    LSR_mem(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// BVC rel8

HANDLER(op50)
{
    Jcc_com(iVM, srV == 0);
    HANDLER_END_FLOW();
}

// EOR (zp),Y

HANDLER(op51)
{
    EA_zpYindR(iVM);
    EOR_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// NOP zp,X

HANDLER(op54)
{
    regPC++;
    wLeft -= 4;
    HANDLER_END();
}

// EOR zp,X

HANDLER(op55)
{
    EA_zpXR(iVM);
    EOR_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LSR zp,X

HANDLER(op56)
{
    EA_zpXW(iVM);
    LSR_zp(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// CLI

HANDLER(op58)
{
    srI = 0;
    wLeft -= 2;
    HANDLER_END();
}

// EOR abs,Y

HANDLER(op59)
{
    EA_absYR(iVM);
    EOR_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// EOR abs,X

HANDLER(op5D)
{
    EA_absXR(iVM);
    EOR_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LSR abs,X

HANDLER(op5E)
{
    EA_absXW(iVM);
    LSR_mem(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// SRE abs,X

HANDLER(op5F)
{
    EA_absXW(iVM);
    LSR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// RTS

HANDLER(op60)
{
    regPC = PopWord(iVM) + 1;
    wLeft -= 6;

    SIOCheck(iVM);  // SIO hack

    HANDLER_END_FLOW();
}

// ADC (zp,X)

HANDLER(op61)
{
    EA_zpXindR(iVM);
    ADC_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// ADC zp

HANDLER(op65)
{
    EA_zpR(iVM);
    ADC_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// ROR zp

HANDLER(op66)
{
    EA_zpW(iVM);
    ROR_zp(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// PLA

HANDLER(op68)
{
    regA = PopByte(iVM);
    update_NZ(iVM, regA);
    wLeft -= 4;
    HANDLER_END();
}

// ADC #

HANDLER(op69)
{
    EA_imm(iVM);
    ADC_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// ROR A

HANDLER(op6A)
{
    BYTE newC = regA << 7;

    regA = (regA >> 1) | (srC & 0x80);
    update_NZ(iVM, regA);
    srC = newC;
    wLeft -= 2;
    HANDLER_END();
}

// JMP (abs)

HANDLER(op6C)
{
    EA_absW(iVM);
    regPC = READ_WORD(iVM, regEA);
    wLeft -= 5;

    SIOCheck(iVM);  // SIO hack

    HANDLER_END_FLOW();
}

// ADC abs

HANDLER(op6D)
{
    EA_absR(iVM);
    ADC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ROR abs

HANDLER(op6E)
{
    EA_absW(iVM);
    ROR_mem(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// RRA abs

HANDLER(op6F)
{
    EA_absW(iVM);
    ROR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// BVS rel8

HANDLER(op70)
{
    Jcc_com(iVM, srV != 0);
    HANDLER_END_FLOW();
}

// ADC (zp),Y

HANDLER(op71)
{
    EA_zpYindR(iVM);
    ADC_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// ADC zp,X

HANDLER(op75)
{
    EA_zpXR(iVM);
    ADC_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// ROR zp,X

HANDLER(op76)
{
    EA_zpXW(iVM);
    ROR_zp(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// RRA zp,x - (ROR + ADC)

HANDLER(op77)
{
    EA_zpXW(iVM);
    ROR_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// SEI

HANDLER(op78)
{
    srI = 0x04;
    wLeft -= 2;
    HANDLER_END();
}

// ADC abs,Y

HANDLER(op79)
{
    EA_absYR(iVM);
    ADC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ADC abs,X

HANDLER(op7D)
{
    EA_absXR(iVM);
    ADC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// ROR abs,X

HANDLER(op7E)
{
    EA_absXW(iVM);
    ROR_mem(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// NOP #imm

HANDLER(op80)
{
    regPC++;
    wLeft -= 2;
    HANDLER_END();
}

// STA (zp,X)

HANDLER(op81)
{
    EA_zpXindW(iVM);
    ST_com(iVM, regA);
    wLeft -= 6;
    HANDLER_END();
}

// SAX (zp,x)

HANDLER(op83)
{

    EA_zpXindW(iVM);
    ST_com(iVM, regA & regX);
    wLeft -= 6;    // best guess
    HANDLER_END();
}

// STY zp

HANDLER(op84)
{

    EA_zpW(iVM);
    ST_zp(iVM, regY);
    wLeft -= 3;
    HANDLER_END();
}

// STA zp

HANDLER(op85)
{
    EA_zpW(iVM);
    ST_zp(iVM, regA);
    wLeft -= 3;
    HANDLER_END();
}

// STX zp

HANDLER(op86)
{
    EA_zpW(iVM);
    ST_zp(iVM, regX);
    wLeft -= 3;
    HANDLER_END();
}

// DEY

HANDLER(op88)
{
    update_NZ(iVM, --regY);
    wLeft -= 2;
    HANDLER_END();
}

// NOP imm

HANDLER(op89)
{
    EA_imm(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// TXA

HANDLER(op8A)
{
    regA = regX;
    update_NZ(iVM, regX);
    wLeft -= 2;
    HANDLER_END();
}

// ANE imm

HANDLER(op8B)
{
    regPC++;    // undefined behaviour, but it is a 2 byte opcode
    wLeft -= 2; // best guess
    HANDLER_END()
}

// STY abs

HANDLER(op8C)
{
    EA_absW(iVM);
    ST_com(iVM, regY);
    wLeft -= 4;
    HANDLER_END()
}

// STA abs

HANDLER(op8D)
{
    EA_absW(iVM);
    ST_com(iVM, regA);
    wLeft -= 4;    // note that the decrementing comes last, after the STA, so STA WSYNC sees the old value
    HANDLER_END();
}

// STX abs

HANDLER(op8E)
{
    EA_absW(iVM);
    ST_com(iVM, regX);
    wLeft -= 4;
    HANDLER_END();
}

// SAX abs

HANDLER(op8F)
{
    EA_absW(iVM);
    ST_com(iVM, regA & regX);
    wLeft -= 4; // best guess
    HANDLER_END();
}

// BCC rel8

HANDLER(op90)
{
    Jcc_com(iVM, (srC & 0x80) == 0);
    HANDLER_END_FLOW();
}

// STA (zp),Y

HANDLER(op91)
{
    EA_zpYindW(iVM);
    ST_com(iVM, regA);
    wLeft -= 6;
    HANDLER_END();
}

// STY zp,X

HANDLER(op94)
{
    EA_zpXW(iVM);
    ST_com(iVM, regY);
    wLeft -= 4;
    HANDLER_END();
}

// STA zp,X

HANDLER(op95)
{
    EA_zpXW(iVM);
    ST_com(iVM, regA);
    wLeft -= 4;
    HANDLER_END();
}

// STX zp,Y

HANDLER(op96)
{
    EA_zpYW(iVM);
    ST_com(iVM, regX);
    wLeft -= 4;
    HANDLER_END();
}

// TYA

HANDLER(op98)
{
    regA = regY;
    update_NZ(iVM, regY);
    wLeft -= 2;
    HANDLER_END();
}

// STA abs,Y

HANDLER(op99)
{
    EA_absYW(iVM);
    ST_com(iVM, regA);
    wLeft -= 5;
    HANDLER_END();
}

// TXS

HANDLER(op9A)
{
    regSP = regX | 0x100;
    wLeft -= 2;
    HANDLER_END();
}

// STA abs,X

HANDLER(op9D)
{
    EA_absXW(iVM);
    ST_com(iVM, regA);
    wLeft -= 5;
    HANDLER_END();
}

// SHA abs,Y

HANDLER(op9F)
{
    regPC += 2;     // undefined implementation, the important thing is to know it's a 3 byte opcode
    wLeft -= 5;     // best guess, probably +1 for crossing a page boundary
    HANDLER_END();
}

// LDY #

HANDLER(opA0)
{
    EA_imm(iVM);
    LDY_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// LDA (zp,X)

HANDLER(opA1)
{
    EA_zpXindR(iVM);
    LDA_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// LDX #

HANDLER(opA2)
{
    EA_imm(iVM);
    LDX_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// LDY zp

HANDLER(opA4)
{
    EA_zpR(iVM);
    LDY_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// LDA zp

HANDLER(opA5)
{
    EA_zpR(iVM);
    LDA_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// LDX zp

HANDLER(opA6)
{
    EA_zpR(iVM);
    LDX_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// TAY

HANDLER(opA8)
{
    regY = regA;
    update_NZ(iVM, regA);
    wLeft -= 2;
    HANDLER_END();
}

// LDA #

HANDLER(opA9)
{
    EA_imm(iVM);
    LDA_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// TAX

HANDLER(opAA)
{
    regX = regA;
    update_NZ(iVM, regA);
    wLeft -= 2;
    HANDLER_END();
}

// LXA imm

HANDLER(opAB)
{
    EA_imm(iVM);
    LDX_com(iVM);
    LDA_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// LDY abs

HANDLER(opAC)
{
    EA_absR(iVM);
    LDY_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LDA abs

HANDLER(opAD)
{
    EA_absR(iVM);
    LDA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LDX abs

HANDLER(opAE)
{
    EA_absR(iVM);
    LDX_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LAX abs

HANDLER(opAF)
{
    EA_absR(iVM);
    LDA_com(iVM);
    LDX_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// BCS rel8

HANDLER(opB0)
{
    Jcc_com(iVM, (srC & 0x80) != 0);
    HANDLER_END_FLOW();
}

// LDA (zp),Y

HANDLER(opB1)
{
    EA_zpYindR(iVM);
    LDA_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// LDY zp,X

HANDLER(opB4)
{
    EA_zpXR(iVM);
    LDY_com(iVM);
    wLeft -= 4;
    HANDLER_END()
}

// LDA zp,X

HANDLER(opB5)
{
    EA_zpXR(iVM);
    LDA_com(iVM);
    wLeft -= 4;
    HANDLER_END()
}

// LDX zp,Y

HANDLER(opB6)
{
    EA_zpYR(iVM);
    LDX_com(iVM);
    wLeft -= 4;
    HANDLER_END()
}

// CLV

HANDLER(opB8)
{
    srV = 0;
    wLeft -= 2;
    HANDLER_END();
}

// LDA abs,Y

HANDLER(opB9)
{
    EA_absYR(iVM);
    LDA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// TSX

HANDLER(opBA)
{
    regX = (BYTE)(regSP & 255);
    update_NZ(iVM, regX);
    wLeft -= 2;
    HANDLER_END();
}

// LDY abs,X

HANDLER(opBC)
{
    EA_absXR(iVM);
    LDY_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LDA abs,X

HANDLER(opBD)
{
    EA_absXR(iVM);
    LDA_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// LDX abs,Y

HANDLER(opBE)
{
    EA_absYR(iVM);
    LDX_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// CPY #

HANDLER(opC0)
{
    EA_imm(iVM);
    CMP_com(iVM, regY);
    wLeft -= 2;
    HANDLER_END();
}

// CMP (zp,X)

HANDLER(opC1)
{
    EA_zpXindR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 6;
    HANDLER_END();
}

// CPY zp

HANDLER(opC4)
{
    EA_zpR(iVM);
    CMP_com(iVM, regY);
    wLeft -= 3;
    HANDLER_END();
}

// CMP zp

HANDLER(opC5)
{
    EA_zpR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 3;
    HANDLER_END();
}

// DEC zp

HANDLER(opC6)
{
    EA_zpW(iVM);
    DEC_zp(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// INY

HANDLER(opC8)
{
    update_NZ(iVM, ++regY);
    wLeft -= 2;
    HANDLER_END();
}

// CMP #

HANDLER(opC9)
{
    EA_imm(iVM);
    CMP_com(iVM, regA);
    wLeft -= 2;
    HANDLER_END();
}

// DEX

HANDLER(opCA)
{
    update_NZ(iVM, --regX);
    wLeft -= 2;
    HANDLER_END();
}

// CPY abs

HANDLER(opCC)
{
    EA_absR(iVM);
    CMP_com(iVM, regY);
    wLeft -= 4;
    HANDLER_END();
}

// CMP abs

HANDLER(opCD)
{
    EA_absR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 4;
    HANDLER_END();
}

// DEC abs

HANDLER(opCE)
{
    EA_absW(iVM);
    DEC_mem(iVM);
    wLeft -= 6;
    HANDLER_END()
}

// BNE rel8

HANDLER(opD0)
{
    Jcc_com(iVM, srZ != 0);
    HANDLER_END_FLOW();
}

// CMP (zp),Y

HANDLER(opD1)
{
    EA_zpYindR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 5;
    HANDLER_END();
}

// DCP (zp),y (decrement and compare)

HANDLER(opD3)
{
    EA_zpYindW(iVM);
    DEC_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
    wLeft -= 5;     // best guess
    HANDLER_END();
}

// NOP zp,x

HANDLER(opD4)
{
    regPC++;
    wLeft -= 4;
    HANDLER_END();
}

// CMP zp,X

HANDLER(opD5)
{
    EA_zpXR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 6;
    HANDLER_END();
}

// DEC zp,X

HANDLER(opD6)
{
    EA_zpXW(iVM);
    DEC_zp(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// CLD

HANDLER(opD8)
{
    srD = 0;
    wLeft -= 2;
    HANDLER_END();
}

// CMP abs,Y

HANDLER(opD9)
{
    EA_absYR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 4;
    HANDLER_END();
}

// CMP abs,X

HANDLER(opDD)
{
    EA_absXR(iVM);
    CMP_com(iVM, regA);
    wLeft -= 4;
    HANDLER_END();
}

// DEC abs,X

HANDLER(opDE)
{
    EA_absXW(iVM);
    DEC_mem(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// DCP abs,x

HANDLER(opDF)
{
    EA_absXW(iVM);
    DEC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
    wLeft -= 7; // best guess
    HANDLER_END();
}

// CPX #

HANDLER(opE0)
{
    EA_imm(iVM);
    CMP_com(iVM, regX);
    wLeft -= 2;
    HANDLER_END();
}

// SBC (zp,X)

HANDLER(opE1)
{
    EA_zpXindR(iVM);
    SBC_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// CPX zp

HANDLER(opE4)
{
    EA_zpR(iVM);
    CMP_com(iVM, regX);
    wLeft -= 3;
    HANDLER_END();
}

// SBC zp

HANDLER(opE5)
{
    EA_zpR(iVM);
    SBC_com(iVM);
    wLeft -= 3;
    HANDLER_END();
}

// INC zp

HANDLER(opE6)
{
    EA_zpW(iVM);
    INC_zp(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// ISB zp - INC + SBC

HANDLER(opE7)
{
    EA_zpW(iVM);
    INC_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// INX

HANDLER(opE8)
{
    update_NZ(iVM, ++regX);
    wLeft -= 2;
    HANDLER_END();
}

// SBC #

HANDLER(opE9)
{
    EA_imm(iVM);
    SBC_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// NOP

HANDLER(opEA)
{
    wLeft -= 2;
    HANDLER_END();
}

// SBC # (again)

HANDLER(opEB)
{
    EA_imm(iVM);
    SBC_com(iVM);
    wLeft -= 2;
    HANDLER_END();
}

// CPX abs

HANDLER(opEC)
{
    EA_absR(iVM);
    CMP_com(iVM, regX);
    wLeft -= 4;
    HANDLER_END();
}

// SBC abs

HANDLER(opED)
{
    EA_absR(iVM);
    SBC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// INC abs

HANDLER(opEE)
{
    EA_absW(iVM);
    INC_mem(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// BEQ rel8

HANDLER(opF0)
{
    Jcc_com(iVM, srZ == 0);
    HANDLER_END_FLOW();
}

// SBC (zp),Y

HANDLER(opF1)
{
    EA_zpYindR(iVM);
    SBC_com(iVM);
    wLeft -= 5;
    HANDLER_END();
}

// ISB (zp),Y

HANDLER(opF3)
{
    EA_zpYindW(iVM);
    INC_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
    wLeft -= 5;     // best guess
    HANDLER_END();
}

// SBC zp,X

HANDLER(opF5)
{
    EA_zpXR(iVM);
    SBC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// INC zp,X

HANDLER(opF6)
{
    EA_zpXW(iVM);
    INC_zp(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// SED

HANDLER(opF8)
{
    srD = 0x08;
    wLeft -= 2;
    HANDLER_END();
}

// SBC abs,Y

HANDLER(opF9)
{
    EA_absYR(iVM);
    SBC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// SBC abs,X

HANDLER(opFD)
{
    EA_absXR(iVM);
    SBC_com(iVM);
    wLeft -= 4;
    HANDLER_END();
}

// INC abs,X

HANDLER(opFE)
{
    EA_absXW(iVM);
    INC_mem(iVM);
    wLeft -= 7;
    HANDLER_END();
}

HANDLER(unused)
{
    regPC--;
    ODS("(%d) UNIMPLEMENTED 6502 OPCODE $%02x USED at $%04x!\n", iVM, READ_BYTE(iVM, regPC), regPC);
    regPC++;
    HANDLER_END();
}

PFNOP jump_tab[256] =
{
    op00,
    op01,
    unused,
    unused,
    op04,
    op05,
    op06,
    op07,
    op08,
    op09,
    op0A,
    unused,
    op0C,
    op0D,
    op0E,
    unused,
    op10,
    op11,
    unused,
    op13,
    unused,
    op15,
    op16,
    op17,
    op18,
    op19,
    op1a,
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
    op2F,
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
    op54,
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
    op5F,
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
    op6F,
    op70,
    op71,
    unused,
    unused,
    unused,
    op75,
    op76,
    op77,
    op78,
    op79,
    unused,
    unused,
    unused,
    op7D,
    op7E,
    unused,
    op80,
    op81,
    unused,
    op83,
    op84,
    op85,
    op86,
    unused,
    op88,
    op89,
    op8A,
    op8B,
    op8C,
    op8D,
    op8E,
    op8F,
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
    op9F,
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
    opAB,
    opAC,
    opAD,
    opAE,
    opAF,
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
    opD3,
    opD4,
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
    opE7,
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
    opF3,
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

// Run for "wLeft" cycles and then return after completing the last instruction (cycles may go negative)
// In debug, tracing and breakpoints can return early.
//
// If "wNMI" is positive, stop at that point to perform an NMI, and then continue down to 0

void __cdecl Go6502(int iVM)
{
    //ODS("Scan %04x : %02x %02x\n", wScan, wLeft, wNMI);
    if (wLeft <= wNMI)    // we're starting past the NMI point, so just do the rest of the line
        wNMI = 0;

    // do not check your breakpoint here, you'll keep hitting it every time you try and execute and never get anywhere

    UnpackP(iVM);

#if 0
        BYTE t, t2;
        PackP();
        t = regP;
        UnpackP();
        PackP();
        t2 = regP;
        if (t != t2) printf("BAD FLAGS STATE\n");
#endif

    do {

        // Start executing
        (*jump_tab[READ_BYTE(iVM, regPC++)])(iVM);

        // if we hit a breakpoint, and an NMI is about to fire, just do the NMI, let the breakpoint get hit after the RTI

        // trigger an NMI - VBI on scan line 248, DLI on others
        if (wNMI && wLeft <= wNMI)
        {
            // VBI takes priority, but there should never be a DLI below scan 247
            if (wScan == STARTSCAN + Y8)
            {
                // clear DLI, set VBI, leave RST alone - even if we're not taking the interrupt
                NMIST = (NMIST & 0x20) | 0x5F;

                // VBI enabled, generate VBI by setting PC to VBI routine. When it's done we'll go back to what we were doing before.
                if (NMIEN & 0x40) {
                    PackP(iVM);    // we're inside Go6502 so we need to pack our flags to push them on the stack
                    //ODS("VBI at %04x %02x\n", wFrame, wLeft);
                    //if (wNMI - wLeft > 4)
                    //    ODS("DELAY!\n");
                    Interrupt(iVM, FALSE);
                    regPC = cpuPeekW(iVM, 0xFFFA);
                    UnpackP(iVM);   // unpack the I bit being set
                }
            }
            else
            {
                // set DLI, clear VBI leave RST alone - even if we don't take the interrupt
                NMIST = (NMIST & 0x20) | 0x9F;
                if (NMIEN & 0x80)    // DLI enabled
                {
                    PackP(iVM);
                    //ODS("DLI at %02x\n", wLeft);
                    Interrupt(iVM, FALSE);
                    regPC = cpuPeekW(iVM, 0xFFFA);
                    UnpackP(iVM);   // unpack the I bit being set
                }
            }
            wNMI = 0;
        }

#ifndef NDEBUG
        if (fTrace || bp == regPC)
            break;
#endif

    } while (wLeft > 0);

    PackP(iVM);
}
