
//
// ASMPPC.H
//
// PowerPC assembler/disassember definitions
// 
// 08/19/2000 DarekM last update
// 

#ifndef ASMPPC_H
#define ASMPPC_H

#pragma once

#include "asmcom.h"

//
// Disassembly flags
//
// Used by the disassembler to pick apart an instructions for display
//


enum
{
    sf_crfD,            // 3-bit field which identifies the 8 condition regs
    sf_crfS,

    sf_crbD,            // 5-bit field which identifies a specific CR bit
    sf_crbA,
    sf_crbB,

    sf_fD,              // 5-bit FPR fields
    sf_fA,
    sf_fS,
    sf_fB,
    sf_fC,

    sf_rD,              // 5-bit GPR fields
    sf_rA0,             // rA field where 0 means 0
    sf_rA,              // rA field where 0 means r0
    sf_rS,
    sf_rB,
    sf_rC,

    sf_UIMM,            // 16-bit immediate field in lower 16 bits
    sf_SIMM,
    sf_dIMM,

    sf_REL16,           // 16-bit relative offset field
    sf_REL26,           // 24-bit relative offset field

    sf_SH,              // 5-bit shift count, bits 16..20
    sf_MB,              // 5-bit starting bit of bit mask, bits 21..25
    sf_ME,              // 5-bit ending bit of bit mask, bits 26..30

    sf_TO,              // 5-bit trap options, bits 6..10
    sf_BO,              // 5-bit branch options, bits 6..10
    sf_BI,              // 5-bit branch bit index, bits 11..15
    sf_SPR,             // 10-bit SPR field, bits 11..20
    sf_SGR,             // 4-bit SR field, bits 12..15
    sf_CRM,             // 8-bit CRM field, bits 12..19
    sf_FM,              // 8-bit FM field, bits 7..14
    sf_IMM4,            // 4-bit immediate for mtfsfix, bits 16..19

    sfDisLast
};

#if (sfDisLast > 32)
#error
#endif

#define wf_crfD         (1 << sf_crfD)
#define wf_crfS         (1 << sf_crfS)

#define wf_crbD         (1 << sf_crbD)
#define wf_crbA         (1 << sf_crbA)
#define wf_crbB         (1 << sf_crbB)

#define wf_fD           (1 << sf_fD)
#define wf_fA           (1 << sf_fA)
#define wf_fS           (1 << sf_fS)
#define wf_fB           (1 << sf_fB)
#define wf_fC           (1 << sf_fC)

#define wf_rD           (1 << sf_rD)
#define wf_rA0          (1 << sf_rA0)
#define wf_rA           (1 << sf_rA)
#define wf_rS           (1 << sf_rS)
#define wf_rB           (1 << sf_rB)
#define wf_rC           (1 << sf_rC)

#define wf_UIMM         (1 << sf_UIMM)
#define wf_SIMM         (1 << sf_SIMM)
#define wf_dIMM         (1 << sf_dIMM)

#define wf_REL16        (1 << sf_REL16)
#define wf_REL26        (1 << sf_REL26)

#define wf_MB           (1 << sf_MB)
#define wf_ME           (1 << sf_ME)
#define wf_SH           (1 << sf_SH)

#define wf_TO           (1 << sf_TO)
#define wf_BO           (1 << sf_BO)
#define wf_BI           (1 << sf_BI)
#define wf_SPR          (1 << sf_SPR)
#define wf_SGR          (1 << sf_SGR)
#define wf_CRM          (1 << sf_CRM)
#define wf_FM           (1 << sf_FM)
#define wf_IMM4         (1 << sf_IMM4)

#define wfForm_rA_rS_SH \
    (wf_rA | wf_rS | wf_SH)

#define wfForm_rA_rS_rB_MB_ME \
    (wf_rA | wf_rS | wf_rB | wf_MB | wf_ME)

#define wfForm_rA_rS_SH_MB_ME \
    (wf_rA | wf_rS | wf_SH | wf_MB | wf_ME)

#define wfForm_rA_rB \
    (wf_rA | wf_rB)

#define wfForm_rA_rS \
    (wf_rA | wf_rS)

#define wfForm_rA_rS_UIMM \
    (wf_rA | wf_rS | wf_UIMM)

#define wfForm_rS_rA_rB \
    (wf_rS | wf_rA | wf_rB)

#define wfForm_rD_rA_SIMM \
    (wf_rD | wf_rA | wf_SIMM)

#define wfForm_rD_rA0_SIMM \
    (wf_rD | wf_rA0 | wf_SIMM)

#define wfForm_rD_rA0_dIMM \
    (wf_rD | wf_rA0 | wf_dIMM)

#define wfForm_fD_rA0_dIMM \
    (wf_fD | wf_rA0 | wf_dIMM)

#define wfForm_rD_rA \
    (wf_rD | wf_rA)

#define wfForm_rD_rA_rB \
    (wf_rD | wf_rA | wf_rB)

#define wfForm_rA_rS_rB \
    (wf_rA | wf_rS | wf_rB)

#define wfForm_fD_rA_rB \
    (wf_fD | wf_rA | wf_rB)

#define wfForm_fD_fA_fB \
    (wf_fD | wf_fA | wf_fB)

#define wfForm_fD_fA_fB_fC \
    (wf_fD | wf_fA | wf_fB | wf_fC)

#define wfForm_fD_fA_fC \
    (wf_fD | wf_fA | wf_fC)

#define wfForm_fD_fB \
    (wf_fD | wf_fB)

#define wfForm_crfD_rA_rB \
    (wf_crfD | wf_rA | wf_rB)

#define wfForm_crfD_rA_SIMM \
    (wf_crfD | wf_rA | wf_SIMM)

#define wfForm_crfD_rA_UIMM \
    (wf_crfD | wf_rA | wf_UIMM)

#define wfForm_crfD_fA_fB \
    (wf_crfD | wf_fA | wf_fB)

#define wfForm_crbD_crbA_crbB \
    (wf_crbD | wf_crbA | wf_crbB)

//
// Special register enumeration.
//
enum
{
    sprMQ   = 0x000,
    sprXER  = 0x020,
    sprRTCU = 0x080,
    sprRTCL = 0x0A0,
    sprDEC  = 0x0C0,
    sprLR   = 0x100,
    sprCTR  = 0x120,
};

//
// Condition register enumeration
//
enum
{
    CR0_LT = 0,
    CR0_GT,
    CR0_EQ,
    CR0_SO,
    CR1_FX,
    CR1_FEX,
    CR1_VX,
    CR1_OX,
    CR2_LT,
    CR2_GT,
    CR2_EQ,
    CR2_SO,
    CR3_LT,
    CR3_GT,
    CR3_EQ,
    CR3_SO,
    CR4_LT,
    CR4_GT,
    CR4_EQ,
    CR4_SO,
    CR5_LT,
    CR5_GT,
    CR5_EQ,
    CR5_SO,
    CR6_LT,
    CR6_GT,
    CR6_EQ,
    CR6_SO,
    CR7_LT,
    CR7_GT,
    CR7_EQ,
    CR7_SO = 31,
};

//
// Branch control enumeration, these are really only the interesting ones.
//

enum
{
    BO_IF_NOT = 4,
    BO_IF     = 12,
    BO_DNZ    = 16,
};

//
// Micro-ops
//

enum
{
    uopFetch = 0,       // last uop of any instruction
    uopNop,             // placeholder
    uopUnimp,

    // extracting of fields needs to be done in the order listed to
    // avoid trashing a register that contains other fields!!!

#if 0
    // first extract the second source field in EBX into itself

    uopExtract_UIMM,    // zero extend the 16-bit UIMM (this is a nop!)
    uopExtract_SIMM,    // sign extend the 16-bit SIMM
    uopExtract_d,       // sign extend the 16-bit displacement
    uopExtract_REL16,   // sign extend the 16-bit branch offset and mask
    uopExtract_REL26,   // sign extend the 26-bit branch offset and mask
    uopExtract_rB,      // 5-bit rB field
    uopExtract_crfB,    // 5-bit crfB field
    uopExtract_crbB,    // 5-bit crbB field
#endif

    // then extract the source 5-bit fields in EAX into EBP

    uopExtract_rS,      // 5-bit rS field (bits 6..10)
    uopExtract_rA0,     // 5-bit rA0 field (bits 11..15)
    uopExtract_rA,      // 5-bit rA field (bits 11..15)

    // then finally trash EAX by extracting the destination 5-bit field

    uopExtract_rD,      // 5-bit rD field (bits 6..10)

    // generate the effective address into EBX

    uopGenEA_rA,        // generate effective address (rA)
    uopGenEA_rA0,       // generate effective address (rA), r0 is 0
    uopGenEA_rA_d,      // generate effective address d(rA)
    uopGenEA_rA0_d,     // generate effective address d(rA), r0 is 0
    uopGenEA_rA_rB,     // generate effective address rA+rB

    // for update instructions, store the effective address back into rA

    uopUpdate_rA,       // store effective address into rA
    uopUpdate_rA0_load, // store effective address into rA (except if rA == 0)

    // convert the effective address in EBX into a flat offset in EBP

    uopConvEA,          // identical to ConvEA in 68K mode

    // various loads from memory using flat offset in EBP into EBX

    uopLoadByteZero,    // read byte from memory and zero extend
    uopLoadHalfZero,    // read halfword from memory and zero extend
    uopLoadHalfArith,   // read halfword from memory and sign extend
    uopLoadWordZero,    // read 32-bit word from memory and zero extend
    uopLoadSingle,      // read 32-bit float from memory
    uopLoadDouble,      // read 64-bit float from memory

    // various stores to memory using flat offset in EBP and value in EBX

    uopStoreByte,       // write byte to memory
    uopStoreHalf,       // write halfword to memory
    uopStoreWord,       // write halfword to memory
    uopStoreSingle,     // write 32-bit float to memory
    uopStoreDouble,     // write 64-bit float to memory

    // this is a uop looping control, loops back 2 uops

    uopRepeatMultWord,  // keep looping until rD == 32

    // read/write data in EBX using the destination index in EAX

    uopLoad_rD,         // write a GPR using index in EAX
    uopStore_rD,        // write a GPR using index in EAX

    uopLoad_frD,        // write a FPR using index in EAX
    uopStore_frD,       // write a FPR using index in EAX

    uopLoad_crfD,       // write a CR field using index in EAX
    uopStore_crfD,      // write a CR field using index in EAX

    uopLoad_crbD,       // write a CR field using index in EAX
    uopStore_crbD,      // write a CR field using index in EAX

    // read/write data in EBX using the primary source index in EBP

    uopLoad_SPR,        // read a SPR using index in EBP
    uopStore_SPR,       // write a SPR using index in EBP

    uopLoad_SR,         // read a SR using index in EBP
    uopStore_SR,        // write a SR using index in EBP

    uopLoad_GPR,        // read a GPR using index in EBP
    uopStore_GPR,       // write a GPR using index in EBP

    // read data into EBP using the primary source index in EBP

    uopLoad_rA0,        // read register rA or 0 into EBP
    uopLoad_rA,         // read register rA into EBP
    uopLoad_rS,         // read register rS into EBP
    uopLoad_crfA,       // read CR field crfA into EBP
    uopLoad_crfS,       // read CR field crfS into EBP
    uopLoad_crbA,       // read CR bit crfA into EBP

    // read data into EBX using the secondary source index in EBX

    uopLoad_rB,         // read register rB into EBX
    uopLoadfrB,         // read FPR frB
    uopLoad_crfB,       // read CR field crfB into EBX
    uopLoad_crbB,       // read CR bit crbB into EBX

    // read/write a specific register

    uopLoad_LR,         // read LR
    uopStore_LR,        // write LR

    uopLoad_CTR,        // read CTR
    uopStore_CTR,       // write CTR

    uopLoad_CR,         // read CR
    uopStore_CR,        // write CR

    uopLoad_MSR,        // read MSR
    uopStore_MSR,       // write MSR

    uopLoad_FPSCR,      // read FPSCR
    uopStore_FPSCR,     // write FPSCR

    // arithmetic operations

    uopAbs,             // absolute value
    uopAdd,             // add
    uopAddX,            // add extended (add A + B + carry)
    uopAnd,             // AND
    uopDivS,            // divide signed
    uopDivU,            // divide unsigned
    uopMul,             // multiply
    uopNegData,         // negate (note: this is a unary, operates on EBX!)
    uopNotData,         // NOT (note: this is a unary, operates on EBX!)
    uopNotSrc,          // NOT (note: this is a unary, operates on EBP!)
    uopOr,              // OR
    uopSub,             // subtract
    uopSubX,            // subtract extended (B - A + carry)
    uopXor,             // XOR

    uopCmp,             // subtract and update crfD indexed by EAX
    uopCmpU,            // subtract and update crfD indexed by EAX

    uopFabs,            // FP absolute value
    uopFadd,            // FP add
    uopFdiv,            // FP divide
    uopFmul,            // FP multiply
    uopFmuladd,         // FP multiply and add
    uopFmulsub,         // FP multiply and subtract
    uopFneg,            // FP negate
    uopFrnds,           // FP round to single precision

    // other operations

    uopShift16,         // shift left 16 bits (used by addis, etc.)
    uopBswap,           // 32-bit byte swap
    uopSwap,            // swap the two source registers
    uopLdc0,            // load constant 0
    uopLdc1,            // load constant 1
    uopLdcM1,           // load constant -1

    uopBranch,          // unconditional branch
    uopBranchCond,      // conditional branch
    uopRfi,             // return from interrupt

    uopClcs,            // cache line computer size (clcs)
    uopCntlzw,          // count leading zeroes (cntlzw)

    uopExtb,            // sign extend  8 to 32-bits
    uopExth,            // sign extend 16 to 32-bits

    uopMaskg,           // maskg instruction
    uopRlwnm,           // rotate and mask (rlwinm rlwnm)
    uopRlwimi,          // rotate mask and insert (rlwimi)
    uopShl,             // shift left
    uopShr,             // shift right
    uopSar,             // shift arithmetic right

    uopAnd_0x0F,        // AND the accumulator with $0F (mtsrin)
    uopEnd
};

//
// StuffBits(mb,me,l)
//
// Stuff a value l into bits mb through me (PowerPC bit numbering scheme)
//

#define StuffBits(mb,me,l) \
    ((((ULONG)(l)) & ((1 << ((me)-(mb)+1)) - 1)) << (31-(me)))


//
// GetBits(mb,me,l)
//
// Extract bits mb through me (PowerPC bit numbering scheme) from value l
//

#define GetBits(mb,me,l) \
    ((l >> (31-(me))) & ((1 << ((me)-(mb)+1)) - 1))

#define M1 (0xFFFFFFFF)

#define bitOE (StuffBits(21,21,1))
#define bitRc (StuffBits(31,31,1))
#define bitAA (StuffBits(30,30,1))
#define bitLK (StuffBits(31,31,1))

#define StuffOp(op)   (((ULONG)(op) & 63) << 26)
#define StuffXop(xop) (((ULONG)(xop) & 1023) << 1)
#define Stuff_rD(reg) (((ULONG)(reg) & 31) << 21)
#define Stuff_rS(reg) (((ULONG)(reg) & 31) << 21)
#define Stuff_rA(reg) (((ULONG)(reg) & 31) << 16)
#define Stuff_rB(reg) (((ULONG)(reg) & 31) << 11)

int DisasmPPC
(
    unsigned const lPC,      // Program Counter
    unsigned const code,     // Instruction opcode to disassemble
    char         * buffer,   // buffer in which to disassemble
    int            bufLen,   // Length of buffer.        
    char         * symName   // symbol name to use in disassembly.
);

#endif

