
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

PFNREAD pfnPeekB;
PFNWRITE pfnPokeB;

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

// these versions of read/write are for when we don't know if we need to do side effects for register read/writes, etc.

#if USE_PEEK_TABLE

// !!! make sure to only use this macro with regEA
// this is for when you don't know if you need to do special tricks for a register or not
#define READ_BYTE read_tab[iVM][regEA >> 8]

#else
// this is for when you don't know if you need to do special tricks for a register or not
__inline uint8_t READ_BYTE(const int iVM, uint32_t ea)
{
    //printf("READ_BYTE: %04X returning %02X\n", ea, rgbMem[ea]);

    Assert(pfnPeekB == PeekBAtari);  // compiler hint

//    if ((0x04000000u >> (ea >> 11)) & 1)
        return (*pfnPeekB)(iVM, ea);
//    else
//        return cpuPeekB(iVM, ea);
}
#endif

#if USE_POKE_TABLE

#define WRITE_BYTE write_tab[iVM][regEA >> 8]

#else
__inline void WRITE_BYTE(const int iVM, uint32_t ea, uint8_t val)
{
    //printf("WRITE_BYTE:%04X writing %02X\n", ea, val);

    Assert(pfnPokeB == PokeBAtari);  // compiler hint

    (*pfnPokeB)(iVM, ea, val);
}
#endif

__inline uint16_t READ_WORD(const int iVM, uint32_t ea)
{
    //Assert(pfnPeekB == PeekBAtari);  // compiler hint

    return READ_BYTE(iVM, ea) | (READ_BYTE(iVM, ea + 1) << 8);
}

__inline void WRITE_WORD(const int iVM, uint32_t ea, uint16_t val)
{
    //Assert(pfnPokeB == PokeBAtari);  // compiler hint

    WRITE_BYTE(iVM, ea, val & 255);
    ea++;   // macro only works with ea, not ea + 1
    WRITE_BYTE(iVM, ea, ((val >> 8) & 255));
}

// dummy opcode handler to force stop emulating 6502

void __fastcall Stop6502(const int iVM)
{
    (void)iVM;
}

//////////////////////////////////////////////////////////////////
//
// Macro definitions
//
//////////////////////////////////////////////////////////////////

// in debug, stop on a breakpoint or if tracing
#ifndef NDEBUG
// it's really handy for debugging to see what cycle of the scan line we're on.
// !!! when wCycle is 0xff, we have passed the end of the scan line, so the next instruction displayed by the monitor
// may NOT be what actually executes, it may be the first instr of an interrupt instead
#define HANDLER_END() { wCycle = wLeft > 0 ? DMAMAP[wLeft - 1] : 0xff; if (regPC != bp && !fTrace && wLeft > wNMI) (*jump_tab[rgbMem[regPC++]])(iVM); } }
#else
#if USE_JUMP_TABLE
#define HANDLER_END() { PFNOP p = Stop6502; if (wLeft > wNMI) p = jump_tab[rgbMem[regPC++]]; (*p)(iVM); } }
#else
// the switch statement only does one instruction at a time (no tail calling) so no need to check wLeft against wNMI
#define HANDLER_END() { } }
#endif
#endif

// this used to not let a scan line end until there's an instruction that affects the PC
#define HANDLER_END_FLOW() HANDLER_END()

#define HANDLER(opcode) void __fastcall opcode (const int iVM) {

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

#define HELPER(opcode) __forceinline __fastcall opcode (const int iVM) {

#define HELPER1(opcode, arg1) __forceinline __fastcall opcode (const int iVM, arg1) {

#define HELPER2(opcode, arg1, arg2) __forceinline __fastcall opcode (const int iVM, arg1, arg2) {

#define HELPER3(opcode, arg1, arg2, arg3) __forceinline __fastcall opcode (const int iVM, arg1, arg2, arg3) {

// For addressing modes where we know we won't be above stack memory and needing special register values, we can directly do a CPU read.
// Otherwise, our READ_ WRITE_ helper functions will check ramtop etc., but be one comparison/potential branch slower
// In general, reading through the PC is safe for using cpuPeekB. Reading through EA isn't unless it's ZP, or stack

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

// !!! 6502 BUG - in decimal mode, N is set based on what the answer would have been in binary mode
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
    WRITE_BYTE(iVM, regEA, x);    // ZP may hold screen RAM, which we have to trap writes to
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

// zp versions of the macros do the quicker direct lookup without a compare to test for special register read
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
    b = cpuPeekB(iVM, regSP);

    return b;
} }

WORD HELPER(PopWord)
{
    WORD w;

    regSP = 0x100 | ((regSP + 1) & 0xFF);
    w = cpuPeekB(iVM, regSP);
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    w |= cpuPeekB(iVM, regSP) << 8;

    return w;
} }

__inline void SIOCheck(const int iVM)
{
    // !!! You can't set a bp on $e459, $e959 or anything inside SIO
    
    // Some people jump directly to $e959, where $e459 points to on the 800 (so that had to be made valid in XL too)
    // OS must be paged in on XL for this to really be SIO. But some people replace the OS with an exact copy, and we can't
    // actually execute that SIO code and work properly, so detect if the swapped in code still says jmp $c933 and do our
    // hack anyway (TITAN)
    // Also, the TRANSLATOR copies the 800 OS back in, so make sure to do our hack if the 800 OS in in XL RAM.
    
    // !!! I'm torn about whether to assume that a jmp to $e459 is always wanting SIO. What are the odds that a custom 
    // program also has an entry point at $e459? Versus the odds that every call to $e459 is some form of SIO that
    // will only work if we trap it?

    if ((regPC == 0xe459 || regPC == 0xe959) &&
            (mdXLXE == md800 || (wPBDATA & 1) ||
                (rgbMem[regPC + 1] == 0x33 && rgbMem[regPC + 2] == 0xc9) ||
                (rgbMem[regPC + 1] == 0x59 && rgbMem[regPC + 2] == 0xe9) ||
                (rgbMem[regPC + 1] == 0x00 && rgbMem[regPC + 2] == 0xce) )) // hack for PAL TRANSLATOR
    {
        // this is our SIO hook!
        PackP(iVM);
        SIOV(iVM);  // if we don't do this now, an interrupt hitting at the same time will screw up the stack
        UnpackP(iVM);

        // With SIO happening instantaneously, you don't see the nice splash screens of many apps, so deliberately take
        // a little bit of time (but not nearly as much as it would really take)

        rgbMem[0xd180] = 0x08;  // php
        rgbMem[0xd181] = 0x48;  // pha
        rgbMem[0xd182] = 0x8a;  // txa
        rgbMem[0xd183] = 0x48;  // pha
        rgbMem[0xd184] = 0x98;  // tya
        rgbMem[0xd185] = 0x48;  // pha
        rgbMem[0xd186] = 0xa0;
        rgbMem[0xd187] = 0x08;  // change this to vary the delay
        rgbMem[0xd188] = 0xa2;
        rgbMem[0xd189] = 0x00;
        rgbMem[0xd18a] = 0xca;
        rgbMem[0xd18b] = 0xd0;
        rgbMem[0xd18c] = 0xfd;
        rgbMem[0xd18d] = 0x88;
        rgbMem[0xd18e] = 0xd0;
        rgbMem[0xd18f] = 0xfa;
        rgbMem[0xd190] = 0x68;  // pla
        rgbMem[0xd191] = 0xa8;  // tay
        rgbMem[0xd192] = 0x68;  // pla
        rgbMem[0xd193] = 0xaa;  // tax
        rgbMem[0xd194] = 0x68;  // pla
        rgbMem[0xd195] = 0x28;  // plp
        rgbMem[0xd196] = 0x60;
        PushWord(iVM, regPC - 1);

        if (vi.fInDebugger && !vi.fExecuting && fTrace)
        {
            fTrace = FALSE; // stop tracing until RTS so we don't get bogged down by this hacky delay code
            wSIORTS = regPC;
        }

        regPC = 0xd180;
    }
    else if ((mdXLXE != md800) && (regPC >= 0xD700) && (regPC <= 0xD7FF))
    {
        // this is our XE BUS hook!
        PackP(iVM);
        SIOV(iVM);
        UnpackP(iVM);
    }
    else if (regPC >= 0xD700 && regPC <= 0xD7FF)
        Assert(FALSE);  // Wrong VM?
}

// we hit a KIL instruction and should hang. Try a different VM type
HANDLER(KIL)
{
    ODS("KIL OPCODE!\n");

    KillMePlease(iVM);

    wLeft -= 2; // Must be non-zero! Or we stack fault in long BRK chains that call this
    HANDLER_END();
}


// BRK

HANDLER(op00)
{
    // !!! Is BRK maskable like I am assuming?

    // we are trying to execute in memory non-existent in an 800, we're probably the wrong VM type
    // hitting a BRK anywhere in OS code probably means we expected a different version of the OS to be there
    if (regPC >= 0xc000 /* && regPC < 0xd000 */ && mdXLXE == md800)
        KIL(iVM);

    PackP(iVM);    // we'll be pushing it
    if (!(regP & IBIT))
    {
        Interrupt(iVM, TRUE);
        regPC = cpuPeekW(iVM, 0xFFFE);
        //ODS("IRQ %02x TIME! %04x %03x\n", (BYTE)~IRQST, wFrame, wScan);
    }
    UnpackP(iVM);

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

// SLO (zp,x)

HANDLER(op03)
{
    EA_zpXindW(iVM);                // the W version of these macros doesn't indirect the second time yet
    ASL_mem(iVM);                   // because this needs to alter the EA (which isn't zero page any longer, so don't use the _zp macro)
    regEA = READ_BYTE(iVM, regEA);  // then this part does the indirection that the R version of the macro would have done
    ORA_com(iVM);                   // so this can act on the new contents
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

// ANC #

HANDLER(op0B)
{
    EA_imm(iVM);
    AND_com(iVM);
    srC = regA & 0x80;
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

// SLO abs

HANDLER(op0F)
{
    EA_absW(iVM);
    ASL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
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
    EA_zpYindW(iVM);
    ASL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
    wLeft -= 5;     // best guess
    HANDLER_END();
}

// NOP zp,X

HANDLER(op14)
{
    regPC++;
    wLeft -= 4;
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

HANDLER(op1A)
{
    wLeft -= 2;
    HANDLER_END();
}

// SLO abs,Y

HANDLER(op1B)
{
    EA_absYW(iVM);
    ASL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// NOP abs,X

HANDLER(op1C)
{
    regPC += 2;
    wLeft -= 4;
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

// SLO abs,X

HANDLER(op1F)
{
    EA_absXW(iVM);
    ASL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ORA_com(iVM);
    wLeft -= 6;
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

// RLA (zp,X)

HANDLER(op23)
{
    EA_zpXindW(iVM);
    ROL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
    wLeft -= 6; // best guess
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

// RLA zp

HANDLER(op27)
{
    EA_zpW(iVM);
    ROL_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
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

// ANC #

HANDLER(op2B)
{
    EA_imm(iVM);
    AND_com(iVM);
    srC = regA & 0x80;
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

// RLA (zp),Y

HANDLER(op33)
{
    EA_zpYindW(iVM);
    ROL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
    wLeft -= 6; // best guess
    HANDLER_END();
}

// NOP zp,X

HANDLER(op34)
{
    regPC++;
    wLeft -= 4;
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

// RLA zp,X

HANDLER(op37)
{
    EA_zpXW(iVM);
    ROL_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
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

// NOP

HANDLER(op3A)
{
    wLeft -= 2;
    HANDLER_END();
}

// RLA abs,Y

HANDLER(op3B)
{
    EA_absYW(iVM);
    ROL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// NOP abs,X

HANDLER(op3C)
{
    regPC += 2;
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

// RLA abs,X

HANDLER(op3F)
{
    EA_absXW(iVM);
    ROL_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    AND_com(iVM);
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
    
    // I'm not sure which one this is, but assume DLI for now
    if (fInVBI && fInDLI)
        fInDLI--;
    else if (fInVBI)
    {
        fInVBI--;
        wScanVBIEnd = wScan;    // remember when the VBI ended
    }
    else if (fInDLI)
        fInDLI--;

    // if we were hiding SIO hack loop code, but an interrupt hit so we where showing code again, now hide it again that it's done
    if (wSIORTS)
        fTrace = FALSE;

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

// SRE (zp,X)

HANDLER(op43)
{
    EA_zpXindW(iVM);
    LSR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
    wLeft -= 6;
    HANDLER_END();
}

// NOP zp

HANDLER(op44)
{
    regPC++;
    wLeft -= 3;
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

// SRE zp

HANDLER(op47)
{
    EA_zpW(iVM);
    LSR_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
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

// SRE abs

HANDLER(op4F)
{
    EA_absW(iVM);
    LSR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
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

// SRE (zp),Y

HANDLER(op53)
{
    EA_zpYindW(iVM);
    LSR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
    wLeft -= 5; // best guess
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

// SRE zp,X

HANDLER(op57)
{
    EA_zpXW(iVM);
    LSR_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
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

// NOP

HANDLER(op5A)
{
    wLeft -= 2;
    HANDLER_END();
}

// SRE abs,Y

HANDLER(op5B)
{
    EA_absYW(iVM);
    LSR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    EOR_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// NOP abs,X

HANDLER(op5C)
{
    regPC += 2;
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

#ifndef NDEBUG
    // resume tracing after skipping our hacky delay code
    if (vi.fInDebugger && !fTrace && regPC == wSIORTS)
    {
        wSIORTS = 0;
        if (!vi.fExecuting) // if we did a 'G' command during SIO delay, don't start tracing now
            fTrace = TRUE;
    }
#endif

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

// RRA (zp,x)

HANDLER(op63)
{
    EA_zpXindW(iVM);
    ROR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
    wLeft -= 6; // best guess
    HANDLER_END();
}

// NOP zp

HANDLER(op64)
{
    regPC++;
    wLeft -= 3;
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

// RRA zp

HANDLER(op67)
{
    EA_zpW(iVM);
    ROR_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
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

    // if a binary forgot to give us a run address, we jump to zero! Hopefully my binary loader has stored the
    // load address of the last segment in $47/48. We can try that.
    if (regEA == 0x2e0)
    {
        // !!! I'm too lazy to fix my hand assembly to make sure critic==0 (Deflektor XE)
        if ((regPC == 0x86c || regPC == 0x5ec) && rgbMem[0x42] != 0)
            rgbMem[0x42] = 0;

        if (!rgbMem[0x2e0] && !rgbMem[0x2e1])
        {
            rgbMem[0x2e0] = rgbMem[0x47];
            rgbMem[0x2e1] = rgbMem[0x48];
        }
        regPC = READ_WORD(iVM, regEA);
    }

    else if (regEA == 0x2e2)
    {
        // !!! I'm too lazy to fix my hand assembly to make sure critic==0
        if ((regPC == 0x87c || regPC == 0x5fc || regPC == 0x7fc || regPC == 0x57c) && rgbMem[0x42] != 0)
            rgbMem[0x42] = 0;

        regPC = READ_WORD(iVM, regEA);

        // this detects the famous autorun.sys (2 versions) that auto-runs a BASIC program, so if we don't have BASIC in, auto-switch it in
        // What else would alter the HATABS table in that particular spot to force feed characters into the buffer?
        if (regPC == 0x0600 && ramtop == 0xc000 &&
            ((rgbMem[regPC + 3] == 0x1a && rgbMem[regPC + 4] == 0x03) ||
            (rgbMem[regPC + 0xa] == 0x21 && rgbMem[regPC + 0xb] == 0x03) ||
            (rgbMem[regPC + 0x2e] == 0x4a && rgbMem[regPC + 0x2f] == 0x03)))
            KIL(iVM);

        // super hack to let us know every section of code that is loaded by our XEX loader.
        // Bin1-Bin3 puts the start addr in (43,44) and the end addr in (45,46), and jumps through ($ffff)
        // We are then responsible for clearing (0x2e2,0x2e3) and jumping to $87c to continue the loader code
        if (regPC == 0xffff)
        {
            WORD ws = READ_WORD(iVM, 0x43);
            WORD we = READ_WORD(iVM, 0x45);
      
            //ODS("Loading segment %04x-%04x\n", ws, we);

            // we are loading code over top of our loader, that will kill us. Try the alternate loader
            // that lives in a different place. You can't do this manually, so you can't turn this behaviour off

            if (!fAltBinLoader && ((ws >= 0x700 && ws < 0x0a80) || (we >= 0x700 && we < 0x0a80)))
            {
                fAltBinLoader = TRUE;   // try the other loaded relocated elsewhere

                KillMeSoftlyPlease(iVM);

                regPC = 0x87c;          // where our handler would like to continue
            }

            else if (fAltBinLoader && ((ws >= 0x400 && ws < 0x600) || (we >= 0x400 && we < 0x0600)))
            {
                fAltBinLoader = FALSE;  // try the other loaded relocated elsewhere

                KillMeSoftlyPlease(iVM);

                regPC = 0x5fc;          // where our handler would like to continue
            }
            else
                regPC = fAltBinLoader ? 0x5fc : 0x87c;              // where our handler would like to continue

            // also, remember the start address of the most recent segment loaded for the $2e0 hack above
            rgbMem[0x47] = rgbMem[0x43];
            rgbMem[0x48] = rgbMem[0x44];

            cpuPokeW(iVM, 0x2e2, 0);    // put this back
        }
    }
    else
    {
        regPC = READ_WORD(iVM, regEA);
    }

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

// RRA (zp),Y

HANDLER(op73)
{
    EA_zpYindW(iVM);
    ROR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
    wLeft -= 5; // best guess
    HANDLER_END();
}

// NOP zp,X

HANDLER(op74)
{
    regPC++;
    wLeft -= 4;
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

// NOP

HANDLER(op7A)
{
    wLeft -= 2;
    HANDLER_END();
}

// RRA abs,Y

HANDLER(op7B)
{
    EA_absYW(iVM);
    ROR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// NOP abs,X

HANDLER(op7C)
{
    regPC += 2;
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

// RRA abs,X

HANDLER(op7F)
{
    EA_absXW(iVM);
    ROR_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    ADC_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// NOP imm

HANDLER(op80)
{
    // BINARY LOADER HACK #2
    // we just ran some INIT code, and our loader assumed $300-$30b weren't touched, but they were (Click)
    // put them back
    if (regPC == 0x724 || regPC == 0x4a4)   // our two versions of the loader
    {
        rgbMem[0x300] = 0x31;
        rgbMem[0x301] = 0x1;
        rgbMem[0x302] = 0x52;
        rgbMem[0x303] = 0x01;
        rgbMem[0x304] = 0x00;
        rgbMem[0x305] = regPC == 0x724 ? 0x0a : 0x04;    // correct buffer for this version
        rgbMem[0x306] = 0x06;
        rgbMem[0x308] = 0x80;
        rgbMem[0x309] = 0x00;
        rgbMem[0x30a] = regPC == 0x724 ? rgbMem[0xa7e] : rgbMem[0x47e]; // correct buffer for this version
        rgbMem[0x30b] = regPC == 0x724 ? rgbMem[0xa7d] : rgbMem[0x47d];
    }
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

// NOP #

HANDLER(op82)
{
    regPC++;
    wLeft -= 2;
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

// SAX zp

HANDLER(op87)
{
    EA_zpW(iVM);
    ST_zp(iVM, regA & regX);
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

// SAX zp,X

HANDLER(op97)
{
    EA_zpXW(iVM);
    ST_com(iVM, regA & regX);
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

// LAX (zp,X)

HANDLER(opA3)
{
    EA_zpXindR(iVM);
    LDA_com(iVM);
    LDX_com(iVM);
    wLeft -= 6;
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

// LAX zp

HANDLER(opA7)
{
    EA_zpR(iVM);
    LDA_com(iVM);
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

// LAX (zp),Y

HANDLER(opB3)
{
    EA_zpYindR(iVM);
    LDA_com(iVM);
    LDX_com(iVM);
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

// LAX zp,Y

HANDLER(opB7)
{
    EA_zpYR(iVM);
    LDA_com(iVM);
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

// LAX abs,Y

HANDLER(opBF)
{
    EA_absYR(iVM);
    LDA_com(iVM);
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

// NOP #

HANDLER(opC2)
{
    regPC++;
    wLeft -= 2;
    HANDLER_END();
}

// DCP (zp,x) (decrement and compare)

HANDLER(opC3)
{
    EA_zpXindW(iVM);
    DEC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
    wLeft -= 6;     // best guess
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

// DCP zp

HANDLER(opC7)
{
    EA_zpW(iVM);
    DEC_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
    wLeft -= 5;
    HANDLER_END()
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

// DCP abs

HANDLER(opCF)
{
    EA_absW(iVM);
    DEC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
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
    DEC_mem(iVM);
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

// DCP zp,X

HANDLER(opD7)
{
    EA_zpXW(iVM);
    DEC_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
    wLeft -= 6;     // best guess
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

// NOP

HANDLER(opDA)
{
    wLeft -= 2;
    HANDLER_END();
}

// DCP abs,Y

HANDLER(opDB)
{
    EA_absYW(iVM);
    DEC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    CMP_com(iVM, regA);
    wLeft -= 7; // best guess
    HANDLER_END();
}

// NOP abs,X

HANDLER(opDC)
{
    regPC += 2;
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
// NOP #

HANDLER(opE2)
{
    regPC++;
    wLeft -= 2;
    HANDLER_END();
}

// ISB (zp,X)

HANDLER(opE3)
{
    EA_zpXindW(iVM);
    INC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
    wLeft -= 6; // best guess
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

// ISB abs

HANDLER(opEF)
{
    EA_absW(iVM);
    INC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
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
    INC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
    wLeft -= 5;     // best guess
    HANDLER_END();
}

// NOP zp,X

HANDLER(opF4)
{
    regPC++;
    wLeft -= 4;
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

// ISB zp,Y

HANDLER(opF7)
{
    EA_zpYW(iVM);
    INC_zp(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
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

// NOP

HANDLER(opFA)
{
    wLeft -= 2;
    HANDLER_END();
}

// ISB abs,Y

HANDLER(opFB)
{
    EA_absYW(iVM);
    INC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// NOP abs,X

HANDLER(opFC)
{
    regPC += 2;
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

// ISB abs,X

HANDLER(opFF)
{
    EA_absXW(iVM);
    INC_mem(iVM);
    regEA = READ_BYTE(iVM, regEA);
    SBC_com(iVM);
    wLeft -= 7;
    HANDLER_END();
}

// !!! Implement the rest of these, already!
HANDLER(unused)
{
    regPC--;
    ODS("(%d) UNIMPLEMENTED 6502 OPCODE $%02x USED at $%04x!\n", iVM, cpuPeekB(iVM, regPC), regPC);
    regPC++;
    wLeft -= 2; // must be non-zero or we could stack fault
    HANDLER_END();
}

PFNOP jump_tab[256] =
{
    op00,
    op01,
    KIL,
    op03,
    op04,
    op05,
    op06,
    op07,
    op08,
    op09,
    op0A,
    op0B,
    op0C,
    op0D,
    op0E,
    op0F,
    op10,
    op11,
    KIL,
    op13,
    op14,
    op15,
    op16,
    op17,
    op18,
    op19,
    op1A,
    op1B,
    op1C,
    op1D,
    op1E,
    op1F,
    op20,
    op21,
    KIL,
    op23,
    op24,
    op25,
    op26,
    op27,
    op28,
    op29,
    op2A,
    op2B,
    op2C,
    op2D,
    op2E,
    op2F,
    op30,
    op31,
    KIL,
    op33,
    op34,
    op35,
    op36,
    op37,
    op38,
    op39,
    op3A,
    op3B,
    op3C,
    op3D,
    op3E,
    op3F,
    op40,
    op41,
    KIL,
    op43,
    op44,
    op45,
    op46,
    op47,
    op48,
    op49,
    op4A,
    unused,
    op4C,
    op4D,
    op4E,
    op4F,
    op50,
    op51,
    KIL,
    op53,
    op54,
    op55,
    op56,
    op57,
    op58,
    op59,
    op5A,
    op5B,
    op5C,
    op5D,
    op5E,
    op5F,
    op60,
    op61,
    KIL,
    op63,
    op64,
    op65,
    op66,
    op67,
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
    KIL,
    op73,
    op74,
    op75,
    op76,
    op77,
    op78,
    op79,
    op7A,
    op7B,
    op7C,
    op7D,
    op7E,
    op7F,
    op80,
    op81,
    op82,
    op83,
    op84,
    op85,
    op86,
    op87,
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
    KIL,
    unused,
    op94,
    op95,
    op96,
    op97,
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
    opA3,
    opA4,
    opA5,
    opA6,
    opA7,
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
    KIL,
    opB3,
    opB4,
    opB5,
    opB6,
    opB7,
    opB8,
    opB9,
    opBA,
    unused,
    opBC,
    opBD,
    opBE,
    opBF,
    opC0,
    opC1,
    opC2,
    opC3,
    opC4,
    opC5,
    opC6,
    opC7,
    opC8,
    opC9,
    opCA,
    unused,
    opCC,
    opCD,
    opCE,
    opCF,
    opD0,
    opD1,
    KIL,
    opD3,
    opD4,
    opD5,
    opD6,
    opD7,
    opD8,
    opD9,
    opDA,
    opDB,
    opDC,
    opDD,
    opDE,
    opDF,
    opE0,
    opE1,
    opE2,
    opE3,
    opE4,
    opE5,
    opE6,
    opE7,
    opE8,
    opE9,
    opEA,
    opEB,
    opEC,
    opED,
    opEE,
    opEF,
    opF0,
    opF1,
    KIL,
    opF3,
    opF4,
    opF5,
    opF6,
    opF7,
    opF8,
    opF9,
    opFA,
    opFB,
    opFC,
    opFD,
    opFE,
    opFF,
};

// Run for "wLeft" cycles and then return after completing the last instruction (cycles may go negative)
// In debug, tracing and breakpoints can return early.
//
// If "wNMI" is positive, stop at that point to perform an NMI, and then continue down to 0

void __cdecl Go6502(const int iVM)
{
    // trying to execute in register space is a bad sign (except our SIO hack lives there) - doesn't appear to help
    //if ((regPC >= 0xd000 && regPC < 0xd180) || (regPC >= 0xd1a0 && regPC < 0xd800))
    //    KIL(iVM);

    //ODS("Scan %04x : %02x %02x\n", wScan, wLeft, wNMI);
    if (wLeft <= wNMI)    // we're starting past the NMI point, so just do the rest of the line
        wNMI = 0;
    
    // we need to set NMIST to what interrupt is theoretically coming up, even if it's not enabled
    // This should happen on cycle 7, but let's do it now, some apps will hang looking for it and need
    // to see it before the interrupt fires (Tank Arkade)
    else if (wNMI > 0)
    {
        if (wScan == STARTSCAN + Y8)
            // clear DLI, set VBI, leave RST alone
            NMIST = (NMIST & 0x20) | 0x5F;
        else
            // set DLI, clear VBI leave RST alone
            NMIST = (NMIST & 0x20) | 0x9F;
    }

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
#if USE_JUMP_TABLE
            (*jump_tab[cpuPeekB(iVM, regPC++)])(iVM);
#else
            do {
                switch (cpuPeekB(iVM, regPC++))
                {
                default:
                    Assert(0);    // hint to the compiler that this is unreachable to optimize away the bounds check
                    break;

                    // There need to be all 256 cases below, 0x00 through 0xFF, since the compiler just optimized away the default case

                case 0x02:
                case 0x12:
                case 0x22:
                case 0x32:
                case 0x42:
                case 0x52:
                case 0x62:
                case 0x72:
                case 0x92:
                case 0xB2:
                case 0xD2:
                case 0xF2:
                    KIL(iVM);
                    break;

                case 0x4B:
                case 0x6B:
                case 0x93:
                case 0x9B:
                case 0x9C:
                case 0x9E:
                case 0xBB:
                case 0xCB:
                    unused(iVM);
                    break;

                case 0x00:   // BRK
                    op00(iVM);
                    break;

                case 0x01:   // ORA (zp,X)
                    op01(iVM);
                    break;

                case 0x03:   // UNDOCUMENTED
                    op03(iVM);
                    break;

                case 0x04:   // UNDOCUMENTED
                    op04(iVM);
                    break;

                case 0x05:   // ORA zp
                    op05(iVM);
                    break;

                case 0x06:   // ASL zp
                    op06(iVM);
                    break;

                case 0x07:   // UNDOCUMENTED
                    op07(iVM);
                    break;

                case 0x08:   // PHP
                    op08(iVM);
                    break;

                case 0x09:   // ORA #
                    op09(iVM);
                    break;

                case 0x0A:   // ASL A
                    op0A(iVM);
                    break;

                case 0x0B:   // ANC #
                    op0B(iVM);
                    break;

                case 0x0C:   // UNDOCUMENTED
                    op0C(iVM);
                    break;

                case 0x0D:   // ORA abs
                    op0D(iVM);
                    break;

                case 0x0E:   // ASL abs
                    op0E(iVM);
                    break;

                case 0x0F:   // SLO abs
                    op0F(iVM);
                    break;

                case 0x10:   // BPL rel8
                    op10(iVM);
                    break;

                case 0x11:   // ORA (zp),Y
                    op11(iVM);
                    break;

                case 0x13:   // UNDOCUMENTED
                    op13(iVM);
                    break;

                case 0x14:   // UNDOCUMENTED
                    op14(iVM);
                    break;

                case 0x15:   // ORA zp,Y
                    op15(iVM);
                    break;

                case 0x16:   // ASL zp,Y
                    op16(iVM);
                    break;

                case 0x17:   // UNDOCUMENTED
                    op17(iVM);
                    break;

                case 0x18:   // CLC
                    op18(iVM);
                    break;

                case 0x19:   // ORA abs,Y
                    op19(iVM);
                    break;

                case 0x1A:   // UNDOCUMENTED
                    op1A(iVM);
                    break;

                case 0x1B:   // SLO abs,Y
                    op1B(iVM);
                    break;

                case 0x1C:   // UNDOCUMENTED
                    op1C(iVM);
                    break;

                case 0x1D:   // ORA abs,X
                    op1D(iVM);
                    break;

                case 0x1E:   // ASL abs,X
                    op1E(iVM);
                    break;

                case 0x1F:   // SLO abs,X
                    op1F(iVM);
                    break;

                case 0x20:   // JSR abs
                    op20(iVM);
                    break;

                case 0x21:   // AND (zp,X)
                    op21(iVM);
                    break;

                case 0x23:   // RLA (zp,X)
                    op23(iVM);
                    break;

                case 0x24:   // BIT zp
                    op24(iVM);
                    break;

                case 0x25:   // AND zp
                    op25(iVM);
                    break;

                case 0x26:   // ROL zp
                    op26(iVM);
                    break;

                case 0x27:   // RLA zp
                    op27(iVM);
                    break;

                case 0x28:   // PLP
                    op28(iVM);
                    break;

                case 0x29:   // AND #
                    op29(iVM);
                    break;

                case 0x2A:   // ROL A
                    op2A(iVM);
                    break;

                case 0x2B:   // ANC #
                    op2B(iVM);
                    break;

                case 0x2C:   // BIT abs
                    op2C(iVM);
                    break;

                case 0x2D:   // AND abs
                    op2D(iVM);
                    break;

                case 0x2E:   // ROL abs
                    op2E(iVM);
                    break;

                case 0x2F:   // UNDOCUMENTED
                    op2F(iVM);
                    break;

                case 0x30:   // BMI rel8
                    op30(iVM);
                    break;

                case 0x31:   // AND (zp),Y
                    op31(iVM);
                    break;

                case 0x33:   // UNDOCUMENTED
                    op33(iVM);
                    break;

                case 0x34:   // UNDOCUMENTED
                    op34(iVM);
                    break;

                case 0x35:   // AND zp,X
                    op35(iVM);
                    break;

                case 0x36:   // ROL zp,X
                    op36(iVM);
                    break;

                case 0x37:   // RLA zp,X
                    op37(iVM);
                    break;

                case 0x38:   // SEC
                    op38(iVM);
                    break;

                case 0x39:   // AND abs,Y
                    op39(iVM);
                    break;

                case 0x3A:   // UNDOCUMENTED
                    op3A(iVM);
                    break;

                case 0x3B:   // RLA abs,Y
                    op3B(iVM);
                    break;

                case 0x3C:   // UNDOCUMENTED
                    op3C(iVM);
                    break;

                case 0x3D:   // AND abs,X
                    op3D(iVM);
                    break;

                case 0x3E:   // ROL abs,X
                    op3E(iVM);
                    break;

                case 0x3F:   // UNDOCUMENTED
                    op3F(iVM);
                    break;

                case 0x40:   // RTI
                    op40(iVM);
                    break;

                case 0x41:   // EOR (zp,X)
                    op41(iVM);
                    break;

                case 0x43:   // SRE (zp,X)
                    op43(iVM);
                    break;
                
                case 0x44:   // UNDOCUMENTED
                    op44(iVM);
                    break;

                case 0x45:   // EOR zp
                    op45(iVM);
                    break;

                case 0x46:   // LSR zp
                    op46(iVM);
                    break;

                case 0x47:   // SRE zp
                    op47(iVM);
                    break;
                
                case 0x48:   // PHA
                    op48(iVM);
                    break;

                case 0x49:   // EOR #
                    op49(iVM);
                    break;

                case 0x4A:   // LSR A
                    op4A(iVM);
                    break;

                case 0x4C:   // JMP abs
                    op4C(iVM);
                    break;

                case 0x4D:   // EOR abs
                    op4D(iVM);
                    break;

                case 0x4E:   // LSR abs
                    op4E(iVM);
                    break;

                case 0x4F:   // SRE abs
                    op4F(iVM);
                    break;

                case 0x50:   // BVC rel8
                    op50(iVM);
                    break;

                case 0x51:   // EOR (zp),Y
                    op51(iVM);
                    break;

                case 0x53:   // SRE (zp),Y
                    op53(iVM);
                    break;
                
                case 0x54:   // UNDOCUMENTED
                    op54(iVM);
                    break;

                case 0x55:   // EOR zp,X
                    op55(iVM);
                    break;

                case 0x56:   // LSR zp,X
                    op56(iVM);
                    break;

                case 0x57:   // UNDOCUMENTED
                    op57(iVM);
                    break;

                case 0x58:   // CLI
                    op58(iVM);
                    break;

                case 0x59:   // EOR abs,Y
                    op59(iVM);
                    break;

                case 0x5A:   // UNDOCUMENTED
                    op5A(iVM);
                    break;

                case 0x5B:   // SRE abs,Y
                    op5B(iVM);
                    break;

                case 0x5C:   // UNDOCUMENTED
                    op5C(iVM);
                    break;

                case 0x5D:   // EOR abs,X
                    op5D(iVM);
                    break;

                case 0x5E:   // LSR abs,X
                    op5E(iVM);
                    break;

                case 0x5F:   // UNDOCUMENTED
                    op5F(iVM);
                    break;

                case 0x60:   // RTS
                    op60(iVM);
                    break;

                case 0x61:   // ADC (zp,X)
                    op61(iVM);
                    break;

                case 0x63:   // UNDOCUMENTED
                    op63(iVM);
                    break;

                case 0x64:   // UNDOCUMENTED
                    op64(iVM);
                    break;

                case 0x65:   // ADC zp
                    op65(iVM);
                    break;

                case 0x66:   // ROR zp
                    op66(iVM);
                    break;

                case 0x67:   // UNDOCUMENTED
                    op67(iVM);
                    break;

                case 0x68:   // PLA
                    op68(iVM);
                    break;

                case 0x69:   // ADC #
                    op69(iVM);
                    break;

                case 0x6A:   // ROR A
                    op6A(iVM);
                    break;

                case 0x6C:   // JMP (abs)
                    op6C(iVM);
                    break;

                case 0x6D:   // ADC abs
                    op6D(iVM);
                    break;

                case 0x6E:   // ROR abs
                    op6E(iVM);
                    break;

                case 0x6F:   // UNDOCUMENTED
                    op6F(iVM);
                    break;

                case 0x70:   // BVS rel8
                    op70(iVM);
                    break;

                case 0x71:   // ADC (zp),Y
                    op71(iVM);
                    break;

                case 0x73:   // RRA (zp),Y
                    op73(iVM);
                    break;

                case 0x74:   // UNDOCUMENTED
                    op74(iVM);
                    break;

                case 0x75:   // ADC zp,X
                    op75(iVM);
                    break;

                case 0x76:   // ROR zp,X
                    op76(iVM);
                    break;

                case 0x77:   // UNDOCUMENTED
                    op77(iVM);
                    break;

                case 0x78:   // SEI
                    op78(iVM);
                    break;

                case 0x79:   // ADC abs,Y
                    op79(iVM);
                    break;

                case 0x7A:   // NOP
                    op7A(iVM);
                    break;

                case 0x7B:   // RRA abs,Y
                    op7B(iVM);
                    break;

                case 0x7C:   // UNDOCUMENTED
                    op7C(iVM);
                    break;

                case 0x7D:   // ADC abs,X
                    op7D(iVM);
                    break;

                case 0x7E:   // ROR abs,X
                    op7E(iVM);
                    break;

                case 0x7F:   // RRA abs,X
                    op7F(iVM);
                    break;

                case 0x80:   // UNDOCUMENTED
                    op80(iVM);
                    break;

                case 0x81:   // STA (zp),Y
                    op81(iVM);
                    break;

                case 0x82:   // NOP #
                    op82(iVM);
                    break;

                case 0x83:   // UNDOCUMENTED
                    op83(iVM);
                    break;

                case 0x84:   // STY zp
                    op84(iVM);
                    break;

                case 0x85:   // STA zp
                    op85(iVM);
                    break;

                case 0x86:   // STX zp
                    op86(iVM);
                    break;

                case 0x87:   // UNDOCUMENTED
                    op87(iVM);
                    break;

                case 0x88:   // DEY
                    op88(iVM);
                    break;

                case 0x89:   // UNDOCUMENTED
                    op89(iVM);
                    break;

                case 0x8A:   // TXA
                    op8A(iVM);
                    break;

                case 0x8B:   // UNDOCUMENTED
                    op8B(iVM);
                    break;

                case 0x8C:   // STY abs
                    op8C(iVM);
                    break;

                case 0x8D:   // STA abs
                    op8D(iVM);
                    break;

                case 0x8E:   // STX abs
                    op8E(iVM);
                    break;

                case 0x8F:   // UNDOCUMENTED
                    op8F(iVM);
                    break;

                case 0x90:   // BCC rel8
                    op90(iVM);
                    break;

                case 0x91:   // STA (zp),Y
                    op91(iVM);
                    break;

                case 0x94:   // STY zp,X
                    op94(iVM);
                    break;

                case 0x95:   // STA zp,X
                    op95(iVM);
                    break;

                case 0x96:   // STX zp,X
                    op96(iVM);
                    break;

                case 0x97:   // UNDOCUMENTED
                    op97(iVM);
                    break;

                case 0x98:   // TYA
                    op98(iVM);
                    break;

                case 0x99:   // STA abs,Y
                    op99(iVM);
                    break;

                case 0x9A:   // TXS
                    op9A(iVM);
                    break;

                case 0x9D:   // STA abs,X
                    op9D(iVM);
                    break;

                case 0x9F:   // UNDOCUMENTED
                    op9F(iVM);
                    break;

                case 0xA0:   // LDY #
                    opA0(iVM);
                    break;

                case 0xA1:   // LDA (zp,X)
                    opA1(iVM);
                    break;

                case 0xA2:   // LDX #
                    opA2(iVM);
                    break;

                case 0xA3:   // LAX (zp,X)
                    opA3(iVM);
                    break;

                case 0xA4:   // LDY zp
                    opA4(iVM);
                    break;

                case 0xA5:   // LDA zp
                    opA5(iVM);
                    break;

                case 0xA6:   // LDX zp
                    opA6(iVM);
                    break;

                case 0xA7:   // LAX zp
                    opA7(iVM);
                    break;

                case 0xA8:   // TAY
                    opA8(iVM);
                    break;

                case 0xA9:   // LDA #
                    opA9(iVM);
                    break;

                case 0xAA:   // TAX
                    opAA(iVM);
                    break;

                case 0xAB:   // UNDOCUMENTED
                    opAB(iVM);
                    break;

                case 0xAC:   // LDY abs
                    opAC(iVM);
                    break;

                case 0xAD:   // LDA abs
                    opAD(iVM);
                    break;

                case 0xAE:   // LDX abs
                    opAE(iVM);
                    break;

                case 0xAF:   // UNDOCUMENTED
                    opAF(iVM);
                    break;

                case 0xB0:   // BCS rel8
                    opB0(iVM);
                    break;

                case 0xB1:   // LDA (zp),Y
                    opB1(iVM);
                    break;

                case 0xB3:   // LAX (zp),Y
                    opB3(iVM);
                    break;

                case 0xB4:   // LDY zp,X
                    opB4(iVM);
                    break;

                case 0xB5:   // LDA zp,X
                    opB5(iVM);
                    break;

                case 0xB6:   // LDX zp,Y
                    opB6(iVM);
                    break;

                case 0xB7:   // LAX zp,Y
                    opB7(iVM);
                    break;

                case 0xB8:   // CLV
                    opB8(iVM);
                    break;

                case 0xB9:   // LDA abs,Y
                    opB9(iVM);
                    break;

                case 0xBA:   // TSX
                    opBA(iVM);
                    break;

                case 0xBC:   // LDY abs,X
                    opBC(iVM);
                    break;

                case 0xBD:   // LDA abs,X
                    opBD(iVM);
                    break;

                case 0xBE:   // LDX abs,Y
                    opBE(iVM);
                    break;

                case 0xBF:   // LAX abs,Y
                    opBF(iVM);
                    break;

                case 0xC0:   // CPY #
                    opC0(iVM);
                    break;

                case 0xC1:   // CMP (zp,X)
                    opC1(iVM);
                    break;

                case 0xC2:   // UNDOCUMENTED
                    opC2(iVM);
                    break;

                case 0xC3:   // UNDOCUMENTED
                    opC3(iVM);
                    break;

                case 0xC4:   // CPY zp
                    opC4(iVM);
                    break;

                case 0xC5:   // CMP zp
                    opC5(iVM);
                    break;

                case 0xC6:   // DEC zp
                    opC6(iVM);
                    break;

                case 0xC7:   // DCP zp
                    opC7(iVM);
                    break;

                case 0xC8:   // INY
                    opC8(iVM);
                    break;

                case 0xC9:   // CMP #
                    opC9(iVM);
                    break;

                case 0xCA:   // DEX
                    opCA(iVM);
                    break;

                case 0xCC:   // CPY abs
                    opCC(iVM);
                    break;

                case 0xCD:   // CMP abs
                    opCD(iVM);
                    break;

                case 0xCE:   // DEC abs
                    opCE(iVM);
                    break;

                case 0xCF:   // DCP abs
                    opCF(iVM);
                    break;

                case 0xD0:   // BNE rel8
                    opD0(iVM);
                    break;

                case 0xD1:   // CMP (zp),Y
                    opD1(iVM);
                    break;

                case 0xD3:   // UNDOCUMENTED
                    opD3(iVM);
                    break;

                case 0xD4:   // UNDOCUMENTED
                    opD4(iVM);
                    break;

                case 0xD5:   // CMP zp,X
                    opD5(iVM);
                    break;

                case 0xD6:   // DEC zp,X
                    opD6(iVM);
                    break;

                case 0xD7:   // DCP zp,X
                    opD7(iVM);
                    break;

                case 0xD8:   // CLD
                    opD8(iVM);
                    break;

                case 0xD9:   // CMP abs,Y
                    opD9(iVM);
                    break;

                case 0xDA:   // NOP
                    opDA(iVM);
                    break;

                case 0xDB:   // DCP abs,Y
                    opDB(iVM);
                    break;

                case 0xDC:   // UNDOCUMENTED
                    opDC(iVM);
                    break;

                case 0xDD:   // CMP abs,X
                    opDD(iVM);
                    break;

                case 0xDE:   // DEC abs,X
                    opDE(iVM);
                    break;

                case 0xDF:   // UNDOCUMENTED
                    opDF(iVM);
                    break;

                case 0xE0:   // CPX #
                    opE0(iVM);
                    break;

                case 0xE1:   // SBC (zp,X)
                    opE1(iVM);
                    break;

                case 0xE2:   // UNDOCUMENTED
                    opE2(iVM);
                    break;

                case 0xE3:   // ISB (zp,X)
                    opE3(iVM);
                    break;

                case 0xE4:   // CPX zp
                    opE4(iVM);
                    break;

                case 0xE5:   // SBC zp
                    opE5(iVM);
                    break;

                case 0xE6:   // INC zp
                    opE6(iVM);
                    break;

                case 0xE7:   // UNDOCUMENTED
                    opE7(iVM);
                    break;

                case 0xE8:   // INX
                    opE8(iVM);
                    break;

                case 0xE9:   // SBC #
                    opE9(iVM);
                    break;

                case 0xEA:   // NOP
                    opEA(iVM);
                    break;

                case 0xEB:   // UNDOCUMENTED
                    opEB(iVM);
                    break;

                case 0xEC:   // CPX abs
                    opEC(iVM);
                    break;

                case 0xED:   // SBC abs
                    opED(iVM);
                    break;

                case 0xEE:   // INC abs
                    opEE(iVM);
                    break;

                case 0xEF:   // ISB abs
                    opEF(iVM);
                    break;

                case 0xF0:   // BEQ rel8
                    opF0(iVM);
                    break;

                case 0xF1:   // SBC (zp),Y
                    opF1(iVM);
                    break;

                case 0xF3:   // ISB (zp),Y
                    opF3(iVM);
                    break;

                case 0xF4:   // NOP zp,X
                    opF4(iVM);
                    break;

                case 0xF5:   // SBC zp,X
                    opF5(iVM);
                    break;

                case 0xF6:   // INC zp,X
                    opF6(iVM);
                    break;

                case 0xF7:   // ISB zp,Y
                    opF7(iVM);
                    break;

                case 0xF8:   // SED
                    opF8(iVM);
                    break;

                case 0xF9:   // SBC abs,Y
                    opF9(iVM);
                    break;

                case 0xFA:   // UNDOCUMENTED
                    opFA(iVM);
                    break;

                case 0xFB:   // ISB abs,Y
                    opFB(iVM);
                    break;

                case 0xFC:   // NOP abs,X
                    opFC(iVM);
                    break;

                case 0xFD:   // SBC abs,X
                    opFD(iVM);
                    break;

                case 0xFE:   // INC abs,X
                    opFE(iVM);
                    break;

                case 0xFF:   // ISB abs,X
                    opFF(iVM);
                    break;
                }
#ifdef NDEBUG
            } while (wLeft > wNMI);
#else
                wCycle = wLeft > 0 ? DMAMAP[wLeft - 1] : 0xff;

            } while (regPC != bp && !fTrace && wLeft > wNMI);
#endif
#endif

        // if we hit a breakpoint, and an NMI is about to fire, just do the NMI, let the breakpoint get hit after the RTI

        // trigger an NMI - VBI on scan line 248, DLI on others
        if (wNMI && wLeft <= wNMI)
        {
            // VBI takes priority, but there should never be a DLI below scan 247
            if (wScan == STARTSCAN + Y8)
            {
                // clear DLI, set VBI, leave RST alone - even if we're not taking the interrupt
                //NMIST = (NMIST & 0x20) | 0x5F;

                // VBI enabled, generate VBI by setting PC to VBI routine. When it's done we'll go back to what we were doing before.
                if (NMIEN & 0x40) {
                    PackP(iVM);    // we're inside Go6502 so we need to pack our flags to push them on the stack
                    //ODS("VBI at %04x %02x\n", wFrame, wLeft);
                    //if (wNMI - wLeft > 4)
                    //    ODS("DELAY!\n");
                    Interrupt(iVM, FALSE);
                    regPC = cpuPeekW(iVM, 0xFFFA);
                    UnpackP(iVM);   // unpack the I bit being set
                    
                    // We're still in the last VBI? Must be a PAL app that's spoiled by how long these can be
                    if (fInVBI)
                        SwitchToPAL(iVM);
                    fInVBI++;;

                    wLeft -= 7; // 7 CPU cycles are wasted internally setting up the interrupt, so it will start @~17, not 10
                    wCycle = wLeft > 0 ? DMAMAP[wLeft - 1] : 0xff;   // wLeft could be 0 if the NMI was delayed due to WSYNC
                }
            }
            else
            {
                // set DLI, clear VBI leave RST alone - even if we don't take the interrupt
                //NMIST = (NMIST & 0x20) | 0x9F;
                
                if (NMIEN & 0x80)    // DLI enabled
                {
                    PackP(iVM);
                    //ODS("DLI at %02x\n", wLeft);
                    Interrupt(iVM, FALSE);
                    regPC = cpuPeekW(iVM, 0xFFFA);
                    UnpackP(iVM);   // unpack the I bit being set

                    // We're still in the last VBI? And we're not one of those post-VBI DLI's?
                    // Must be a PAL app that's spoiled by how long these can be
                    // We're starting to nest deeply in DLIs? Maybe it's a long DLI kernel that needs PAL.
                    if ((fInVBI || fInDLI > 1) && wScan >= STARTSCAN && wScan < STARTSCAN + Y8)
                    {
                        // this happened last frame too, we're PAL. NTSC programs occasionally do this, like MULE every
                        // 4 frames, but never consecutively like a PAL program would.
                        if (fDLIinVBI == wFrame - 1)
                            SwitchToPAL(iVM);
                        fDLIinVBI = wFrame;
                    }
                    fInDLI++;

                    wLeft -= 7; // 7 CPU cycles are wasted internally setting up the interrupt, so it will start @~17, not 10
                    wCycle = wLeft > 0 ? DMAMAP[wLeft - 1] : 0xff;  // wLeft could be 0 if the NMI was delayed due to WSYNC
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
