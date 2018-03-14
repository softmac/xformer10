
//
// PPCDIS.C
//
// PowerPC disassembler module.
//
// Usage: DisasmPPC
//    unsigned long  PC,       // Program Counter
//    unsigned long  code,     // Instruction opcode to disassemble
//    char         * buffer,   // buffer in which to disassemble
//    int            bufLen,   // Length of buffer.        
//    char         * symName   // symbol name to use in disassembly.
//
// Returns:
//    TRUE on success
//
// 06/18/2001 last update
//

#ifdef TEST
#include <windows.h>
#include <stdio.h>
#endif

#include "asmppc.h"

#ifdef NEWJIT
#define UOPPPC(op)  extern char op[]
#else
#define UOPPPC(op)  enum { op };
#endif

UOPPPC(rguAbs);
UOPPPC(rguAdd);
UOPPPC(rguAddc);
UOPPPC(rguAdde);
UOPPPC(rguAddi);
UOPPPC(rguAddic_);
UOPPPC(rguAddis);
UOPPPC(rguAddme);
UOPPPC(rguAddze);

UOPPPC(rguOr);

UOPPPC(rguLwz);
UOPPPC(rguStw);
UOPPPC(rguStwu);

UOPPPC(rguMfspr);

UOPPPC(rguBranch);
UOPPPC(rguBranchCond);
UOPPPC(rguBranchCondLR);

UOPPPC(rguCmp);

IDEF const rgidef[] =
{
    // CHEATER DEFINITIONS

    {
    "add",
    StuffBits(0,5,31) | StuffBits(22,30,266),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    rguAdd
    },

    {
    "addi",
    StuffBits(0,5,14),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_SIMM,
    rguAddi
    },

    {
    "addis",
    StuffBits(0,5,15),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_SIMM,
    rguAddis
    },

    {
    "b",
    StuffBits(0,5,18),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkAA | wfChkLK,
    wf_REL26,
    rguBranch
    },

    {
    "bc",
    StuffBits(0,5,16),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkAA | wfChkLK,
    wf_BO | wf_BI | wf_REL16,
    rguBranchCond
    },

    {
    "bclr",
    StuffBits(0,5,19) | StuffBits(21,30,16),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt19 | wfChkLK,
    wf_BO | wf_BI,
    rguBranchCondLR
    },

    {
    "cmp",
    StuffBits(0,5,31)                      | StuffBits(21,31,0),
    StuffBits(0,5,M1) | StuffBits(9,9,M1)  | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_crfD_rA_rB,
    rguCmp
    },

    {
    "lwz",
    StuffBits(0,5,32),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    rguLwz
    },

    {
    "mfspr",
    StuffBits(0,5,31) | StuffBits(21,30,339),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_SPR | wf_rD,
    rguMfspr
    },

    {
    "or",
    StuffBits(0,5,31) | StuffBits(21,30,444),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    rguOr
    },

    {
    "stw",
    StuffBits(0,5,36),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    rguStw
    },

    {
    "stwu",
    StuffBits(0,5,37),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    rguStwu
    },

    // END CHEATERS

    {
    "abs",
    StuffBits(0,5,31) | StuffBits(22,30,360),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpu601 | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA,
    rguAbs
    },

    {
    "add",
    StuffBits(0,5,31) | StuffBits(22,30,266),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    rguAdd
    },

    {
    "addc",
    StuffBits(0,5,31) | StuffBits(22,30,10),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA_rB,
    rguAddc
    },

    {
    "adde",
    StuffBits(0,5,31) | StuffBits(22,30,138),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA_rB,
    rguAdde
    },

    {
    "addi",
    StuffBits(0,5,14),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_SIMM,
    rguAddi
    },

    {
    "addic",
    StuffBits(0,5,12),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfSetCA,
    wfForm_rD_rA_SIMM,
    uopLoad_rA, uopAdd, uopStore_rD
    },

    {
    "addic.",
    StuffBits(0,5,13),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfSetCA | wfSetRc,
    wfForm_rD_rA_SIMM,
    uopLoad_rA, uopAdd, uopStore_rD
    },

    {
    "addis",
    StuffBits(0,5,15),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_SIMM,
    rguAddis
    },

    {
    "addme",
    StuffBits(0,5,31) | StuffBits(22,30,234),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA,
    uopLdcM1, uopLoad_rA, uopAddX, uopStore_rD,
    },

    {
    "addze",
    StuffBits(0,5,31) | StuffBits(22,30,202),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA,
    uopLdc0, uopLoad_rA, uopAddX, uopStore_rD,
    },

    {
    "and",
    StuffBits(0,5,31) | StuffBits(21,30,28),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rS, uopAnd, uopStore_rD,
    },

    {
    "andc",
    StuffBits(0,5,31) | StuffBits(21,30,60),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rS, uopNotData, uopAnd, uopStore_rD,
    },

    {
    "andi.",
    StuffBits(0,5,28),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfSetRc,
    wfForm_rA_rS_UIMM,
    uopLoad_rS, uopAnd, uopStore_rD,
    },

    {
    "andis.",
    StuffBits(0,5,29),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfSetRc,
    wfForm_rA_rS_UIMM,
    uopLoad_rS, uopShift16, uopAnd, uopStore_rD,
    },

    {
    "b",
    StuffBits(0,5,18),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkAA | wfChkLK,
    wf_REL26,
    rguBranch
    },

    {
    "bc",
    StuffBits(0,5,16),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkAA | wfChkLK,
    wf_BO | wf_BI | wf_REL16,
    rguBranchCond
    },

    {
    "bcctr",
    StuffBits(0,5,19) | StuffBits(21,30,528),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt19 | wfChkLK,
    wf_BO | wf_BI,
    uopLoad_CTR, uopBranchCond,
    },

    {
    "bclr",
    StuffBits(0,5,19) | StuffBits(21,30,16),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt19 | wfChkLK,
    wf_BO | wf_BI,
    rguBranchCondLR
    },

    {
    "clcs",
    StuffBits(0,5,31) | StuffBits(21,30,531),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rD_rA,
    uopClcs, uopStore_rD,
    },

    {
    "cmp",
    StuffBits(0,5,31)                      | StuffBits(21,31,0),
    StuffBits(0,5,M1) | StuffBits(9,9,M1)  | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_crfD_rA_rB,
    rguCmp
    },

    {
    "cmpi",
    StuffBits(0,5,11),
    StuffBits(0,5,M1) | StuffBits(9,9,M1),
    wfCpuAllPPC,
    wfForm_crfD_rA_SIMM,
    uopLoad_rA, uopCmp,
    },

    {
    "cmpl",
    StuffBits(0,5,31)                      | StuffBits(21,30,32),
    StuffBits(0,5,M1) | StuffBits(9,9,M1)  | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_crfD_rA_rB,
    uopLoad_rA, uopCmpU,
    },

    {
    "cmpli",
    StuffBits(0,5,10),
    StuffBits(0,5,M1) | StuffBits(9,9,M1),
    wfCpuAllPPC,
    wfForm_crfD_rA_UIMM,
    uopLoad_rA, uopCmpU,
    },

    {
    "cntlzw",
    StuffBits(0,5,31) | StuffBits(16,30,26),
    StuffBits(0,5,M1) | StuffBits(16,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS,
    uopLoad_rS, uopCntlzw, uopStore_rD
    },

    {
    "crand",
    StuffBits(0,5,19) | StuffBits(21,30,257),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopAnd, uopStore_crbD,
    },

    {
    "crandc",
    StuffBits(0,5,19) | StuffBits(21,30,129),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopNotData, uopAnd, uopStore_crbD,
    },

    {
    "creqv",
    StuffBits(0,5,19) | StuffBits(21,30,289),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopXor, uopNotData, uopStore_crbD,
    },

    {
    "crnand",
    StuffBits(0,5,19) | StuffBits(21,30,225),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopAnd, uopNotData, uopStore_crbD,
    },

    {
    "crnor",
    StuffBits(0,5,19) | StuffBits(21,30,33),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopOr, uopNotData, uopStore_crbD,
    },

    {
    "cror",
    StuffBits(0,5,19) | StuffBits(21,30,449),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopOr, uopStore_crbD,
    },

    {
    "crorc",
    StuffBits(0,5,19) | StuffBits(21,30,417),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopNotData, uopOr, uopStore_crbD,
    },

    {
    "crxor",
    StuffBits(0,5,19) | StuffBits(21,30,193),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    wfForm_crbD_crbA_crbB,
    uopLoad_crbA, uopXor, uopStore_crbD,
    },

    {
    "dcbf",
    StuffBits(0,5,31)  | StuffBits(21,30,86),
    StuffBits(0,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "dcbi",
    StuffBits(0,5,31)  | StuffBits(21,30,470),
    StuffBits(0,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "dcbst",
    StuffBits(0,5,31)  | StuffBits(21,30,54),
    StuffBits(0,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "dcbt",
    StuffBits(0,5,31)  | StuffBits(21,30,278),
    StuffBits(0,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "dcbtst",
    StuffBits(0,5,31)  | StuffBits(21,30,246),
    StuffBits(0,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "dcbz",
    StuffBits(0,5,31)  | StuffBits(21,30,1014),
    StuffBits(0,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "div",
    StuffBits(0,5,31) | StuffBits(22,30,331),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpu601 | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "divs",
    StuffBits(0,5,31) | StuffBits(22,30,363),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpu601 | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "divw",
    StuffBits(0,5,31) | StuffBits(22,30,491),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopLoad_rA, uopDivS, uopStore_rD,
    },

    {
    "divwu",
    StuffBits(0,5,31) | StuffBits(22,30,459),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopLoad_rA, uopDivU, uopStore_rD,
    },

    {
    "doz",
    StuffBits(0,5,31) | StuffBits(22,30,264),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuPOWER | wfCpu601 | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopNop,
    },

    {
    "dozi",
    StuffBits(0,5,9),
    StuffBits(0,5,M1),
    wfCpuPOWER | wfCpu601,
    wfForm_rD_rA_SIMM,
    uopNop,
    },

    {
    "eciwx",
    StuffBits(0,5,31) | StuffBits(21,30,310),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "ecowx",
    StuffBits(0,5,31) | StuffBits(21,30,438),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "eieio",
    StuffBits(0,5,31)  | StuffBits(21,30,854),
    StuffBits(0,31,M1),
    wfCpuAllPPC | wfExt31,
    0,
    uopNop,
    },

    {
    "eqv",
    StuffBits(0,5,31) | StuffBits(22,30,284),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rS, uopXor, uopNotData, uopStore_rD,
    },

    {
    "extsb",
    StuffBits(0,5,31) | StuffBits(21,30,954),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS,
    uopLoad_rS, uopExtb, uopStore_rD,
    },

    {
    "extsh",
    StuffBits(0,5,31) | StuffBits(21,30,922),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS,
    uopLoad_rS, uopExth, uopStore_rD,
    },

    {
    "fabs",
    StuffBits(0,5,63) | StuffBits(21,30,264),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fadd",
    StuffBits(0,5,63)                       | StuffBits(26,30,21),
    StuffBits(0,5,M1) | StuffBits(21,25,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB,
    uopUnimp,
    },

    {
    "fadds",
    StuffBits(0,5,59)                       | StuffBits(26,30,21),
    StuffBits(0,5,M1) | StuffBits(21,25,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB,
    uopUnimp,
    },

    {
    "fcmpo",
    StuffBits(0,5,63)                      | StuffBits(21,30,32),
    StuffBits(0,5,M1) | StuffBits(9,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63,
    wfForm_crfD_fA_fB,
    uopUnimp,
    },

    {
    "fcmpu",
    StuffBits(0,5,63),
    StuffBits(0,5,M1) | StuffBits(9,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63,
    wfForm_crfD_fA_fB,
    uopUnimp,
    },

    {
    "fctiw",
    StuffBits(0,5,63)                       | StuffBits(21,30,14),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fctiwz",
    StuffBits(0,5,63)                       | StuffBits(21,30,15),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fdiv",
    StuffBits(0,5,63)                       | StuffBits(26,30,18),
    StuffBits(0,5,M1) | StuffBits(21,25,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB,
    uopUnimp,
    },

    {
    "fdivs",
    StuffBits(0,5,59)                       | StuffBits(26,30,18),
    StuffBits(0,5,M1) | StuffBits(21,25,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB,
    uopUnimp,
    },

    {
    "fmadd",
    StuffBits(0,5,63) | StuffBits(26,30,29),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fmadds",
    StuffBits(0,5,59) | StuffBits(26,30,29),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fmr",
    StuffBits(0,5,63)                       | StuffBits(21,30,72),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fmsub",
    StuffBits(0,5,63) | StuffBits(26,30,28),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fmsubs",
    StuffBits(0,5,59) | StuffBits(26,30,28),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fmul",
    StuffBits(0,5,63)                       | StuffBits(26,30,25),
    StuffBits(0,5,M1) | StuffBits(16,20,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fC,
    uopUnimp,
    },

    {
    "fmuls",
    StuffBits(0,5,59)                       | StuffBits(26,30,25),
    StuffBits(0,5,M1) | StuffBits(16,20,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fC,
    uopUnimp,
    },

    {
    "fnabs",
    StuffBits(0,5,63)                       | StuffBits(21,30,136),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fneg",
    StuffBits(0,5,63)                       | StuffBits(21,30,40),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fnmadd",
    StuffBits(0,5,63) | StuffBits(26,30,31),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fnmadds",
    StuffBits(0,5,59) | StuffBits(26,30,31),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fnmsub",
    StuffBits(0,5,63) | StuffBits(26,30,30),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "fnmsubs",
    StuffBits(0,5,59) | StuffBits(26,30,30),
    StuffBits(0,5,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB_fC,
    uopUnimp,
    },

    {
    "frsp",
    StuffBits(0,5,63)                       | StuffBits(21,30,12),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fB,
    uopUnimp,
    },

    {
    "fsub",
    StuffBits(0,5,63)                       | StuffBits(26,30,20),
    StuffBits(0,5,M1) | StuffBits(21,25,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt63 | wfChkRc,
    wfForm_fD_fA_fB,
    uopUnimp,
    },

    {
    "fsubs",
    StuffBits(0,5,59)                       | StuffBits(26,30,20),
    StuffBits(0,5,M1) | StuffBits(21,25,M1) | StuffBits(26,30,M1),
    wfCpuAllPPC | wfFpu601 | wfExt59 | wfChkRc,
    wfForm_fD_fA_fB,
    uopUnimp,
    },

    {
    "icbi",
    StuffBits(0,5,31)                      | StuffBits(21,30,982),
    StuffBits(0,5,M1) | StuffBits(6,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rA_rB,
    uopNop,
    },

    {
    "isync",
    StuffBits(0,5,19)                      | StuffBits(21,30,150),
    StuffBits(0,5,M1) | StuffBits(6,20,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt19,
    0,
    uopNop,
    },

    {
    "lbz",
    StuffBits(0,5,34),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoadByteZero, uopStore_rD
    },

    {
    "lbzu",
    StuffBits(0,5,35),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA0_load, uopConvEA, uopLoadByteZero, uopStore_rD
    },

    {
    "lbzux",
    StuffBits(0,5,31) | StuffBits(21,30,119),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA0_load, uopConvEA, uopLoadByteZero, uopStore_rD
    },

    {
    "lbzx",
    StuffBits(0,5,31) | StuffBits(21,30,87),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadByteZero, uopStore_rD
    },

    {
    "lfd",
    StuffBits(0,5,50),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoadDouble, uopStore_frD,
    },

    {
    "lfdu",
    StuffBits(0,5,51),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA0_load, uopConvEA, uopLoadDouble, uopStore_frD,
    },

    {
    "lfdux",
    StuffBits(0,5,31) | StuffBits(21,30,631),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_fD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA0_load, uopConvEA, uopLoadDouble, uopStore_frD,
    },

    {
    "lfdx",
    StuffBits(0,5,31) | StuffBits(21,30,599),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_fD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadDouble, uopStore_frD,
    },

    {
    "lfs",
    StuffBits(0,5,48),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopUnimp,
    },

    {
    "lfsu",
    StuffBits(0,5,49),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopUnimp,
    },

    {
    "lfsux",
    StuffBits(0,5,31) | StuffBits(21,30,567),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopUnimp,
    },

    {
    "lfsx",
    StuffBits(0,5,31) | StuffBits(21,30,535),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_fD_rA_rB,
    uopUnimp,
    },

    {
    "lha",
    StuffBits(0,5,42),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoadHalfArith, uopStore_rD
    },

    {
    "lhau",
    StuffBits(0,5,43),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA0_load, uopConvEA, uopLoadHalfArith, uopStore_rD
    },

    {
    "lhaux",
    StuffBits(0,5,31) | StuffBits(21,30,375),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA0_load, uopConvEA, uopLoadHalfArith, uopStore_rD
    },

    {
    "lhax",
    StuffBits(0,5,31) | StuffBits(21,30,343),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadHalfArith, uopStore_rD
    },

    {
    "lhbrx",
    StuffBits(0,5,31) | StuffBits(21,30,790),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadHalfZero, uopBswap, uopStore_rD
    },

    {
    "lhz",
    StuffBits(0,5,40),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoadHalfZero, uopStore_rD
    },

    {
    "lhzu",
    StuffBits(0,5,41),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA0_load, uopConvEA, uopLoadHalfZero, uopStore_rD
    },

    {
    "lhzux",
    StuffBits(0,5,31) | StuffBits(21,30,311),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA0_load, uopConvEA, uopLoadHalfZero, uopStore_rD
    },

    {
    "lhzx",
    StuffBits(0,5,31) | StuffBits(21,30,279),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadHalfZero, uopStore_rD
    },

    {
    "lmw",
    StuffBits(0,5,46),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoadWordZero, uopStore_rD, uopRepeatMultWord
    },

    {
    "lscbx",
    StuffBits(0,5,31) | StuffBits(21,30,277),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "lswi",
    StuffBits(0,5,31) | StuffBits(21,30,597),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    uopUnimp,
    },

    {
    "lswx",
    StuffBits(0,5,31) | StuffBits(21,30,533),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "lwarx",
    StuffBits(0,5,31) | StuffBits(21,30,20),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadWordZero, uopStore_rD
    },

    {
    "lwbrx",
    StuffBits(0,5,31) | StuffBits(21,30,534),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadWordZero, uopBswap, uopStore_rD
    },

    {
    "lwz",
    StuffBits(0,5,32),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    rguLwz
    },

    {
    "lwzu",
    StuffBits(0,5,33),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA0_load, uopConvEA, uopLoadWordZero, uopStore_rD
    },

    {
    "lwzux",
    StuffBits(0,5,31) | StuffBits(21,30,55),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA0_load, uopConvEA, uopLoadWordZero, uopStore_rD
    },

    {
    "lwzx",
    StuffBits(0,5,31) | StuffBits(21,30,23),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoadWordZero, uopStore_rD
    },

    // maskg  : POWER instruction not implemented.
    // maskir : POWER instruction not implemented.

    {
    "mcrf",
    StuffBits(0,5,19),
    StuffBits(0,5,M1) | StuffBits(9,10,M1) | StuffBits(14,31,M1),
    wfCpuAllPPC | wfExt19,
    wf_crfD | wf_crfS,
    uopLoad_crfS, uopStore_crfD,
    },

    {
    "mcrfs",
    StuffBits(0,5,63) | StuffBits(21,30,64),
    StuffBits(0,5,M1) | StuffBits(9,10,M1) | StuffBits(14,31,M1),
    wfCpuAllPPC | wfExt63,
    uopUnimp,
    },

    {
    "mcrxr",
    StuffBits(0,5,31) | StuffBits(21,30,512),
    StuffBits(0,5,M1) | StuffBits(9,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_crfD,
    uopLoad_CR, uopStore_crfD,
    },

    {
    "mfcr",
    StuffBits(0,5,31) | StuffBits(21,30,19),
    StuffBits(0,5,M1) | StuffBits(11,31,M1), 
    wfCpuAllPPC | wfExt31,
    wf_rD,
    uopLoad_CR, uopStore_rD,
    },

    {
    "mffs",
    StuffBits(0,5,63) | StuffBits(21,30,583),
    StuffBits(0,5,M1) | StuffBits(11,30,M1),
    wfCpuAllPPC | wfExt63 | wfChkRc,
    wf_fD,
    uopLoad_FPSCR, uopStore_frD,
    },

    {
    "mfmsr",
    StuffBits(0,5,31) | StuffBits(21,30,83),
    StuffBits(0,5,M1) | StuffBits(11,31,M1), 
    wfCpuAllPPC | wfExt31,
    wf_rD,
    uopLoad_MSR, uopStore_rD,
    },

    {
    "mfspr",
    StuffBits(0,5,31) | StuffBits(21,30,339),
    StuffBits(0,5,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_SPR | wf_rD,
    rguMfspr
    },

    {
    "mfsr",
    StuffBits(0,5,31) | StuffBits(21,30,595),
    StuffBits(0,5,M1) | StuffBits(11,11,M1) | StuffBits(16,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_SGR | wf_rD,
    uopLoad_SR, uopStore_rD,
    },

    {
    "mfsrin",
    StuffBits(0,5,31) | StuffBits(21,30,659),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_rD | wf_rB,
    uopAnd_0x0F, uopLoad_SR, uopStore_rD,
    },

    {
    "mtcrf",
    StuffBits(0,5,31) | StuffBits(21,30,144),
    StuffBits(0,5,M1) | StuffBits(11,11,M1) | StuffBits(20,31,M1), 
    wfCpuAllPPC | wfExt31,
    wf_CRM | wf_rD,
    uopLoad_rD, uopStore_CR,
    },

    {
    "mtfsb0",
    StuffBits(0,5,63) | StuffBits(21,30, 70),
    StuffBits(0,5,M1) | StuffBits(11,30, M1),
    wfCpuAllPPC | wfExt63 | wfChkRc,
    0,
    uopUnimp,
    },
    
    {
    "mtfsb1",
    StuffBits(0,5,63) | StuffBits(21,30, 38),
    StuffBits(0,5,M1) | StuffBits(11,30, M1),
    wfCpuAllPPC | wfExt63 | wfChkRc,
    0,
    uopUnimp,
    },
    
    {
    "mtfsf",
    StuffBits(0,5,63) | StuffBits(21,30,711), 
    StuffBits(0,6,M1) | StuffBits(15,15,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt63 | wfChkRc,
    0,
    uopUnimp,
    },

    {
    "mtfsfi",
    StuffBits(0,5,63) | StuffBits(21,30,134),
    StuffBits(0,5,M1) | StuffBits(9,15,M1) | StuffBits(20,30,M1),
    wfCpuAllPPC | wfExt63 | wfChkRc,
    0,
    uopUnimp,
    },

    {
    "mtmsr",
    StuffBits(0,5,31) | StuffBits(21,30,146),
    StuffBits(0,5,M1) | StuffBits(11,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_rD,
    uopLoad_rD, uopStore_MSR,
    },

    {
    "mtspr",
    StuffBits(0,5,31) | StuffBits(21,30,467),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wf_SPR | wf_rD,
    uopLoad_rD, uopStore_SPR,
    },

    {
    "mtsr",
    StuffBits(0,5,31) | StuffBits(21,30,210),
    StuffBits(0,5,M1) | StuffBits(11,11,M1) | StuffBits(16,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_SGR | wf_rD,
    uopLoad_rD, uopStore_SR,
    },

    {
    "mtsrin",
    StuffBits(0,5,31) | StuffBits(21,30,242),
    StuffBits(0,5,M1) | StuffBits(11,15,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    wf_rS | wf_rB,
    uopAnd_0x0F, uopLoad_rD, uopStore_SR,
    },

    {
    "mul",
    StuffBits(0,5,31) | StuffBits(22,30,107),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpu601 | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "mulhw",
    StuffBits(0,5,31) | StuffBits(21,30,75),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "mulhwu",
    StuffBits(0,5,31) | StuffBits(21,30,11),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "mullw",
    StuffBits(0,5,31) | StuffBits(22,30,235),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopLoad_rA, uopMul, uopStore_rD,
    },

    {
    "mulli",
    StuffBits(0,5,7),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA_SIMM,
    uopLoad_rA, uopMul, uopStore_rD,
    },

    {
    "nabs",
    StuffBits(0,5,31) | StuffBits(22,30,488),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpu601 | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA,
    uopLoad_rA, uopAbs, uopNegData, uopStore_rD
    },

    {
    "nand",
    StuffBits(0,5,31) | StuffBits(21,30,476),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rS, uopAnd, uopNotData, uopStore_rD
    },

    {
    "neg",
    StuffBits(0,5,31) | StuffBits(22,30,104),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA,
    uopLoad_rA, uopLdc0, uopSub, uopStore_rD
    },

    {
    "nor",
    StuffBits(0,5,31) | StuffBits(21,30,124),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rA, uopOr, uopNotData, uopStore_rD
    },

    {
    "or",
    StuffBits(0,5,31) | StuffBits(21,30,444),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    rguOr
    },

    {
    "orc",
    StuffBits(0,5,31) | StuffBits(21,30,412),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rA, uopNotData, uopOr, uopStore_rD
    },

    {
    "ori",
    StuffBits(0,5,24),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rA_rS_UIMM,
    uopLoad_rA, uopOr, uopStore_rD,
    },

    {
    "oris",
    StuffBits(0,5,25),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rA_rS_UIMM,
    uopLoad_rA, uopShift16, uopOr, uopStore_rD,
    },

    {
    "rfi",
    StuffBits(0,5,19) | StuffBits(21,30,50),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt19,
    0,
    uopRfi,
    },

    {
    "rlmi",
    StuffBits(0,5,22),
    StuffBits(0,5,M1),
    wfCpu601 | wfChkRc,
    wfForm_rA_rS_rB_MB_ME,
    uopUnimp,
    },

    {
    "rlwimi",
    StuffBits(0,5,20),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkRc,
    wfForm_rA_rS_SH_MB_ME,
    uopLoad_rA, uopRlwimi, uopStore_rD
    },

    {
    "rlwinm",
    StuffBits(0,5,21),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkRc,
    wfForm_rA_rS_SH_MB_ME,
    uopLoad_rA, uopRlwnm, uopStore_rD
    },

    {
    "rlwnm",
    StuffBits(0,5,23),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfChkRc,
    wfForm_rA_rS_rB_MB_ME,
    uopLoad_rA, uopRlwnm, uopStore_rD
    },

    // skipped some instructions (see the manual)

    {
    "slw",
    StuffBits(0,5,31) | StuffBits(21,30,24),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rA, uopSwap, uopShl, uopStore_rD
    },

    // skipped some instructions (see the manual)

    {
    "sraw",
    StuffBits(0,5,31) | StuffBits(21,30,792),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc | wfSetCA,
    wfForm_rA_rS_rB,
    uopLoad_rA, uopSwap, uopSar, uopStore_rD
    },

    {
    "srawi",
    StuffBits(0,5,31) | StuffBits(21,30,824),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc | wfSetCA,
    wfForm_rA_rS_SH,
    uopLoad_rA, uopSwap, uopSar, uopStore_rD
    },

    // skipped some instructions (see the manual)

    {
    "srw",
    StuffBits(0,5,31) | StuffBits(21,30,536),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rA, uopSwap, uopShr, uopStore_rD
    },

    {
    "stb",
    StuffBits(0,5,38),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoad_rD, uopStoreByte,
    },

    {
    "stbu",
    StuffBits(0,5,39),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA, uopConvEA, uopLoad_rD, uopStoreByte,
    },

    {
    "stbux",
    StuffBits(0,5,31) | StuffBits(21,30,247),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA, uopConvEA, uopLoad_rD, uopStoreByte,
    },

    {
    "stbx",
    StuffBits(0,5,31) | StuffBits(21,30,215),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoad_rD, uopStoreByte,
    },

    {
    "stfd",
    StuffBits(0,5,54),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoad_frD, uopStoreDouble,
    },

    {
    "stfdu",
    StuffBits(0,5,55),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_fD_rA0_dIMM,
    uopUnimp,
    },

    {
    "stfdux",
    StuffBits(0,5,31) | StuffBits(21,30,759),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "stfdx",
    StuffBits(0,5,31) | StuffBits(21,30,727),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "stfs",
    StuffBits(0,5,52),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopUnimp,
    },

    {
    "stfsu",
    StuffBits(0,5,53),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopUnimp,
    },

    {
    "stfsux",
    StuffBits(0,5,31) | StuffBits(21,30,695),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "stfsx",
    StuffBits(0,5,31) | StuffBits(21,30,663),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "sth",
    StuffBits(0,5,44),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoad_rD, uopStoreHalf,
    },

    {
    "sthbrx",
    StuffBits(0,5,31) | StuffBits(21,30,918),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopUnimp,
    },

    {
    "sthu",
    StuffBits(0,5,45),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopUpdate_rA, uopConvEA, uopLoad_rD, uopStoreHalf,
    },

    {
    "sthux",
    StuffBits(0,5,31) | StuffBits(21,30,439),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA, uopConvEA, uopLoad_rD, uopStoreHalf,
    },

    {
    "sthx",
    StuffBits(0,5,31) | StuffBits(21,30,407),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoad_rD, uopStoreHalf,
    },

    {
    "stmw",
    StuffBits(0,5,47),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    uopGenEA_rA0_d, uopConvEA, uopLoad_rD, uopStoreWord, uopRepeatMultWord,
    },

    {
    "stswi",
    StuffBits(0,5,31) | StuffBits(21,30,725),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    uopUnimp,
    },

    {
    "stswx",
    StuffBits(0,5,31) | StuffBits(21,30,661),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    uopUnimp,
    },

    {
    "stw",
    StuffBits(0,5,36),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    rguStw
    },

    {
    "stwbrx",
    StuffBits(0,5,31) | StuffBits(21,30,662),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoad_rD, uopBswap, uopStoreWord,
    },

    {
    "stwcx.",
    StuffBits(0,5,31) | StuffBits(21,30,150) | bitRc, // Rc bit is on
    StuffBits(0,5,M1) | StuffBits(21,30,M1)  | bitRc,
    wfCpuAllPPC | wfExt31 | wfSetRc,
    wfForm_rS_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoad_rD, uopStoreWord,
    },

    {
    "stwu",
    StuffBits(0,5,37),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rD_rA0_dIMM,
    rguStwu
    },

    {
    "stwux",
    StuffBits(0,5,31) | StuffBits(21,30,183),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopUpdate_rA, uopConvEA, uopLoad_rD, uopStoreWord,
    },

    {
    "stwx",
    StuffBits(0,5,31) | StuffBits(21,30,151),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wfForm_rD_rA_rB,
    uopGenEA_rA_rB, uopConvEA, uopLoad_rD, uopStoreWord,
    },

    {
    "subf",
    StuffBits(0,5,31) | StuffBits(22,30,40),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc,
    wfForm_rD_rA_rB,
    uopLoad_rA, uopSub, uopStore_rD
    },

    {
    "subfc",
    StuffBits(0,5,31) | StuffBits(22,30,8),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA_rB,
    uopLoad_rA, uopSub, uopStore_rD
    },

    {
    "subfe",
    StuffBits(0,5,31) | StuffBits(22,30,136),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA_rB,
    uopLoad_rA, uopSubX, uopStore_rD
    },

    {
    "subfic",
    StuffBits(0,5,8),
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfSetCA,
    wfForm_rD_rA_SIMM,
    uopLoad_rA, uopSub, uopStore_rD
    },

    {
    "subfme",
    StuffBits(0,5,31) | StuffBits(22,30,232),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA,
    uopLoad_rA, uopLdcM1, uopSub, uopStore_rD
    },

    {
    "subfze",
    StuffBits(0,5,31) | StuffBits(22,30,200),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkOE | wfChkRc | wfSetCA,
    wfForm_rD_rA,
    uopLoad_rA, uopLdc0, uopSub, uopStore_rD
    },

    {
    "sync",
    StuffBits(0,5,31)                      | StuffBits(21,30,598),
    StuffBits(0,5,M1) | StuffBits(6,20,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    0,
    uopNop,
    },

    {
    "tlbia",
    StuffBits(0,5,31) | StuffBits(21,30,370),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    0,
    uopNop,
    },

    {
    "tlbie",
    StuffBits(0,5,31) | StuffBits(21,30,306),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    0,
    uopNop,
    },

    {
    "tw",
    StuffBits(0,5,31) | StuffBits(21,30,4),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31,
    wf_rA | wf_rB | wf_TO,
    uopUnimp,
    },

    {
    "twi",
    StuffBits(0,5,3), 
    StuffBits(0,5,M1),
    wfCpuAllPPC | wfExt31,
    wf_rA | wf_SIMM | wf_TO,
    uopUnimp,
    },

    {
    "xor",
    StuffBits(0,5,31) | StuffBits(21,30,316),
    StuffBits(0,5,M1) | StuffBits(21,30,M1),
    wfCpuAllPPC | wfExt31 | wfChkRc,
    wfForm_rA_rS_rB,
    uopLoad_rA, uopXor, uopStore_rD
    },

    {
    "xori",
    StuffBits(0,5,26),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rA_rS_UIMM,
    uopLoad_rA, uopXor, uopStore_rD,
    },

    {
    "xoris",
    StuffBits(0,5,27),
    StuffBits(0,5,M1),
    wfCpuAllPPC,
    wfForm_rA_rS_UIMM,
    uopLoad_rA, uopShift16, uopXor, uopStore_rD,
    },

#if 0
    // PSEUDO OPS
    //
    // These must come last since they are just equivalent names
    // for existing instructions listed above. During disassembly
    // we want to get the real instruction, not the pseudo op.
    // REVIEW: do we really? Or do we want them first??

    {
    "bctr",                     // equivalent to bcctr 20,0
    StuffBits(0,5,19) | StuffBits(6,10,20) | StuffBits(21,30,528),
    StuffBits(0,5,M1) | StuffBits(6,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    wfCpuAllPPC,
    },

    // Order is significant in the pseudo ops due to the way the table is 
    // searched. By placing 'bnl' before 'bge' in the table we ensure 
    // that a 'bc if_not cr0_lt' instruction will be disassembled as 'bge'
    // which I think is easier to read. But the assembler will understand
    // both 'bnl' and 'bge'
    //

    {
    "bdnz",                        // branch if ctr not zero 
    StuffBits(0,5,16) | StuffBits(6,10, BO_DNZ) | StuffBits(11,15, CR0_LT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "bnl",                        // branch if not less than, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_LT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },
        
    {
    "bng",                        // branch if not greater than, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_GT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "blt",                        // branch if less than, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_LT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "ble",                        // branch if less than or equal, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_GT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "beq",                        // branch if equal, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_EQ),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },
    
    {
    "bge",                        // branch if greater than or equal, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_LT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },
    
    {
    "bgt",                        // branch if greater than, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_GT),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "bso",                        // branch if summary overflow, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_SO),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },
        
    {
    "bns",                        // branch if not summary overflow, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_SO),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "bne",                        // branch if not equal, using cr0
    StuffBits(0,5,16) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_EQ),
    StuffBits(0,5,M1) | StuffBits(6,15, M1)    | bitAA | bitLK,
    wfCpuAllPPC,
    },

    {
    "bgtlr",
    StuffBits(0,5,19) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_GT) | 
    StuffBits(21,30,16),
    StuffBits(0,15,M1) | StuffBits(16,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    0,
    },

    {
    "bltlr",
    StuffBits(0,5,19) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_LT) | 
    StuffBits(21,30,16),
    StuffBits(0,15,M1) | StuffBits(16,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    0,
    },

    {
    "beqlr",
    StuffBits(0,5,19) | StuffBits(6,10, BO_IF) | StuffBits(11,15, CR0_EQ) | 
    StuffBits(21,30,16),
    StuffBits(0,15,M1) | StuffBits(16,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    0,
    },

    {
    "bnelr",
    StuffBits(0,5,19) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_EQ) | 
    StuffBits(21,30,16),
    StuffBits(0,15,M1) | StuffBits(16,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    0,
    },

    {
    "blelr",
    StuffBits(0,5,19) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_GT) | 
    StuffBits(21,30,16),
    StuffBits(0,15,M1) | StuffBits(16,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    0,
    },

    {
    "bgelr",
    StuffBits(0,5,19) | StuffBits(6,10, BO_IF_NOT) | StuffBits(11,15, CR0_LT) | 
    StuffBits(21,30,16),
    StuffBits(0,15,M1) | StuffBits(16,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    0,
    },

    {
    "cmpw",                       // equiv cmp #,0,#,#
    StuffBits(0,5,31) | StuffBits(9,10,0)  | StuffBits(21,30,0),
    StuffBits(0,5,M1) | StuffBits(9,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    },

    {
    "cmpwi",                      // equiv cmpi #,0,#,#
    StuffBits(0,5,11) | StuffBits(9,10,0),
    StuffBits(0,5,M1) | StuffBits(9,10,M1),
    wfCpuAllPPC,
    },

    {
    "cmplw",                      // equiv cmpl #,0,#,#
    StuffBits(0,5,31) | StuffBits(9,10,0)  | StuffBits(21,30,32),
    StuffBits(0,5,M1) | StuffBits(9,10,M1) | StuffBits(21,31,M1),
    wfCpuAllPPC | wfExt31,
    },

    {
    "cmplwi",                     // equiv cmpli #,0,#,#
    StuffBits(0,5,10) | StuffBits(9,10,0),
    StuffBits(0,5,M1) | StuffBits(9,10,M1),
    wfCpuAllPPC,
    },

    {
    "mfctr",                      // equiv to mfspr rD,CTR
    StuffBits(0,5,31) | StuffBits(11,20,sprCTR) | StuffBits(21,30,339),
    StuffBits(0,5,M1) | StuffBits(11,30,M1),
    wfCpuAllPPC | wfExt31,
    },

    {
    "mflr",                      // equiv to mfspr rD, LR
    StuffBits(0,5,31) | StuffBits(11,20,sprLR) | StuffBits(21,30,339),
    StuffBits(0,5,M1) | StuffBits(11,30,M1),
    wfCpuAllPPC | wfExt31,
    },
//
// OLD non standard mnemonics that we accept
//
    {
    "move",                 // equiv to ori ra,rs,0
    StuffBits(0,5,24),
    StuffBits(0,5,M1) | StuffBits(16,31,M1),
    wfCpuAllPPC,
    wf_rA | wf_rS,
    },

    {
    "movei",                // equiv to addi ra,r0,imm
    StuffBits(0,5,14),
    StuffBits(0,5,M1) | StuffBits(11,15,M1),
    wfCpuAllPPC,
    wf_rA | wf_SIMM,
    },
//
// More standard mnemonics.
//  
//    
    {
    "li",                   // equiv to addi ra,r0,imm
    StuffBits(0,5,14),
    StuffBits(0,5,M1) | StuffBits(11,15,M1),
    wfCpuAllPPC,
    wf_rD | wf_SIMM,
    },

    {
    "mtctr",
    StuffBits(0,5,31) | StuffBits(11,20,sprCTR) | StuffBits(21,30,467),
    StuffBits(0,5,M1) | StuffBits(11,30,M1),
    wfCpuAllPPC | wfExt31,
    wf_rD,
    },

    {
    "mtlr",
    StuffBits(0,5,31) | StuffBits(11,20,sprLR) | StuffBits(21,30,467),
    StuffBits(0,5,M1) | StuffBits(11,30,M1),
    wfCpuAllPPC | wfExt31,
    wf_rD,
    },

    {
    "nop",                      // equivalent to ori r0,r0,0
    StuffBits(0,5,24),
    StuffBits(0,5,M1) | StuffBits(6,31,M1),
    wfCpuAllPPC,
    0,
    },

    {
    "ret",                      // equivalent to bclr 20,0
    StuffBits(0,5,19) | StuffBits(6,10,20) | StuffBits(21,30,16),
    StuffBits(0,5,M1) | StuffBits(6,30,M1) | bitLK,
    wfCpuAllPPC | wfExt19,
    },

    {
    "sub",                      // equivalent to subf with registers swapped
    StuffBits(0,5,31) | StuffBits(22,30,40),
    StuffBits(0,5,M1) | StuffBits(22,30,M1),
    wfCpuAllPPC | wfExt31,
    },

#endif // PSEUDO OPS
};

//
// Number of instruction definitions in the rgidef array.
//

ULONG const vcNumInstrs = sizeof(rgidef)/sizeof(IDEF);

//
// Disassembly options
//
#define fUsePseudoOps    0x0001
#define fUseRegNames     0x0002
#define fUseFieldNames   0x0008

#define Options  (fUsePseudoOps | fUseRegNames | fUseFieldNames)

//
// FindInstrDef - Find the instruction definition, 
//                this could be sped up with hashing.
//
// TRUE if instruction match found

int FindInstrDef
(
    ULONG   code,        // opcode to match
    IDEF * pidef        // IDEF structure to be filled in if match found.
)
{
    IDEF  idef;
    ULONG  i;

    for (i = 0; i < vcNumInstrs; i++)
        {
        idef = rgidef[i];

        // Mask out the opcode to check for match.
        //
        if ((code & idef.lMask) == idef.lCode)
            {
            *pidef = idef;
            return TRUE;
            }
    }
    return FALSE;
}

//
// FindPseudoInstrDef - Find the pseudo instruction definition by walking 
//                      the table backwards, this could be sped up with hashing
//
// TRUE if instruction match found

int FindPseudoInstrDef
(
    ULONG   code,        // opcode to match
    IDEF * pidef        // IDEF structure to be filled in if match found.
)
{
    IDEF  idef;
    ULONG  i;

    i = vcNumInstrs;

    while (i--)
        {
        idef = rgidef[i];

        // Mask out the opcode to check for match.
        //
        if ((code & idef.lMask) == idef.lCode)
            {
            *pidef = idef;
            return TRUE;
            }
        }
    return FALSE;
}

#if OLDPPC

ULONG InitOpcodesPPC(ULONG *pl)
{
    ULONG  i, code, code2;

    memset(pl, 0, 65536*sizeof(ULONG));

    for (code = 0; code < 65536; code++)
        {
        switch(code >> 10)
            {
        default:
            code2 = code << 16;
            break;

        case 19:
        case 31:
        case 59:
        case 63:
            code2 = (code >> 1) & 1023;
            }

        for (i = 0; i < vcNumInstrs; i++)
            {
            // Mask out the opcode to check for match.
            //
printf("InitOpcodes: code= %08X, lMask = %08X, lCode = %08X\n",
        code, rgidef[i].lMask, rgidef[i].lCode);

            if ((code2 & rgidef[i].lMask) == rgidef[i].lCode)
                {
                if (*pl)
                    printf("ERROR: two IDEFs match the same opcode\n");

                printf("setting opcodes[%d] to %08X\n", i, &rgidef[i]);

                *pl = (ULONG)&rgidef[i];
                }
            }

        pl++;
        }

    return TRUE;
}

#endif

//
// SetCase - upper case if necessary. Either returns a pointer to the argument
// or copies it to a static buffer, uppercases it and returns a pointer to 
// the buffer.
//
__inline char *SetCase(char *p)
{
#if 0
    if (UseUppercase)
    {
	strcpy(Buf, p);
	_strupr(Buf);
	return Buf;
    }
#endif
    return p;
}

//
// PrintReg - prints register number or verbose name
//

__inline int PrintReg(BYTE *pch, unsigned reg)
{
    int c;

    if ((Options & fUseRegNames) && (reg == 1))
        c = sprintf(pch, "sp");
    else if ((Options & fUseRegNames) && (reg == 2))
        c = sprintf(pch, "rtoc");
    else
        c = sprintf(pch, "r%d", reg);

    return c;
}

//
// DisasmPPC - Disassemble a single instruction. 
//
// Returns TRUE if disassembled ok.

int DisasmPPC
(
    unsigned const lPC,      // Program Counter
    unsigned const code,     // Instruction opcode to disassemble
    char         * buffer,   // buffer in which to disassemble
    int            bufLen,   // Length of buffer.        
    char         * symName   // symbol name to use in disassembly.
)
{
    IDEF   idef;
    char * pch;
    BOOL   ok;

    // pch is as the ptr to the current point being filled in the output buffer
    //
    pch = buffer;

    if (Options & fUsePseudoOps)
        {
        ok = FindPseudoInstrDef(code, &idef);
        }
    else
        {
        ok = FindInstrDef(code, &idef);
        }

    // Not found
    //
    if (!ok)
        {
        pch += sprintf(pch, "????");
        *pch++ = 0;

        return FALSE;
        }

    // Fill in the opname

    {
    BYTE sz[16];

    sprintf(sz, "%s%s%s%s%s",
         SetCase((void *)&idef.szMne),
         SetCase(((idef.wfCPU & wfChkOE) && (code & bitOE)) ? "o" : ""),
         SetCase(((idef.wfCPU & wfChkRc) && (code & bitRc)) ? "." : ""),
         SetCase(((idef.wfCPU & wfChkAA) && (code & bitAA)) ? "a" : ""),
         SetCase(((idef.wfCPU & wfChkLK) && (code & bitLK)) ? "l" : ""));

    pch += sprintf(pch, "%-12s", sz);
    }

    if (idef.wfDis & wf_SGR)
        {
        pch += sprintf(pch, "sr%d", (code >> 16) & 15);
        }

    if (idef.wfDis & wf_crfD)
        {
        pch += sprintf(pch, "cfr%d", (code >> 23) & 7);
        }

    if (idef.wfDis & wf_crfS)
        {
        pch += sprintf(pch, ",cfr%d", (code >> 18) & 7);
        }

    if (idef.wfDis & wf_crbD)
        {
        pch += sprintf(pch, "cfb%d", (code >> 21) & 7);
        }

    if (idef.wfDis & wf_crbA)
        {
        pch += sprintf(pch, ",cfb%d", (code >> 16) & 7);
        }

    if (idef.wfDis & wf_crbB)
        {
        pch += sprintf(pch, ",cfb%d", (code >> 11) & 7);
        }

    if (idef.wfDis & wf_fD)
        {
        pch += sprintf(pch, "fr%d", (code >> 21) & 31);
        }

    if (idef.wfDis & wf_fA)
        {
        pch += sprintf(pch, "fr%d", (code >> 16) & 31);
        }

    if (idef.wfDis & wf_fB)
        {
        pch += sprintf(pch, "fr%d", (code >> 11) & 31);
        }

    if (idef.wfDis & wf_fC)
        {
        pch += sprintf(pch, "fr%d", (code >> 6) & 31);
        }

    if (idef.wfDis & wf_rD)
        {
        pch += PrintReg(pch, (code >> 21) & 31);
        }

    if (idef.wfDis & wf_rA)
        {
        if ((idef.wfDis & wf_rD) || (idef.wfDis & wf_fD))
            pch += sprintf(pch, ",");

        if (idef.wfDis & wf_dIMM)
            {
            pch += sprintf(pch, "0x%04x(", code & 65535);
            pch += PrintReg(pch, (code >> 16) & 31);
            pch += sprintf(pch, ")");
            }
        else
            pch += PrintReg(pch, (code >> 16) & 31);
        }

    if (idef.wfDis & wf_rA0)
        {
        if ((idef.wfDis & wf_rD) || (idef.wfDis & wf_fD))
            pch += sprintf(pch, ",");

        if (idef.wfDis & wf_dIMM)
            {
            if ((code >> 16) & 31)
                {
                pch += sprintf(pch, "0x%04x(", code & 65535);
                pch += PrintReg(pch, (code >> 16) & 31);
                pch += sprintf(pch, ")");
                }
            else
                pch += sprintf(pch, "0x%04x", code & 65535);
            }
        else
            pch += sprintf(pch, "0");
        }

    if (idef.wfDis & wf_rS)
        {
        pch += sprintf(pch, ",");
        pch += PrintReg(pch, (code >> 21) & 31);
        }

    if (idef.wfDis & wf_rB)
        {
        pch += sprintf(pch, ",");
        pch += PrintReg(pch, (code >> 11) & 31);
        }

    if (idef.wfDis & wf_UIMM)
        {
        pch += sprintf(pch, ",0x%04x", code & 65535);
        }

    if (idef.wfDis & wf_SIMM)
        {
        pch += sprintf(pch, ",0x%04x", code & 65535);
        }

    if (idef.wfDis & wf_SPR)
        {
        ULONG l = (code >> 11) & 1023;

        switch(l)
            {
        default:
            pch += sprintf(pch, "%s%d", SetCase("spr"), (WORD)l);
            break;

        case sprMQ:   pch += sprintf(pch, SetCase("mq"));   break;
        case sprXER:  pch += sprintf(pch, SetCase("xer"));  break;
        case sprRTCU: pch += sprintf(pch, SetCase("rtcu")); break;
        case sprRTCL: pch += sprintf(pch, SetCase("rtcl")); break;
        case sprDEC:  pch += sprintf(pch, SetCase("dec"));  break;
        case sprLR:   pch += sprintf(pch, SetCase("lr"));   break;
        case sprCTR:  pch += sprintf(pch, SetCase("ctr"));  break;
            }
        }

    if (idef.wfDis & wf_SH)
        {
        pch += sprintf(pch, ",%d", (code >> 11) & 31);
        }

    if (idef.wfDis & wf_MB)
        {
        pch += sprintf(pch, ",%d", (code >> 6) & 31);
        }

    if (idef.wfDis & wf_ME)
        {
        pch += sprintf(pch, ",%d", (code >> 1) & 31);
        }

    if (idef.wfDis & wf_REL26)
        {
        // 26-bit relative branch addresses

        int l = ((((int)code) << 6) >> 6);
        l &= 0xFFFFFFFC;

        if (idef.wfCPU & wfChkAA)
            pch += sprintf(pch, "0x%08x", l);
        else
            {
            if (l < 0)
                pch += sprintf(pch, "*-0x%x", -l);
            else
                pch += sprintf(pch, "*+0x%x", l);

            pch += sprintf(pch, "   ; 0x%08X", lPC + l);
            }
        }

    if (idef.wfDis & wf_BO)
        {
        // Branch control fields

        ULONG l = (code >>21) & 31;

        if (Options & fUseFieldNames)
            {
            switch (l)
                {
            case  0: pch += sprintf(pch, "dCTR_NZERO_AND_NOT"); break;
            case  1: pch += sprintf(pch, "dCTR_NZERO_AND_NOT_1");break;
            case  2: pch += sprintf(pch, "dCTR_ZERO_AND_NOT");  break;
            case  3: pch += sprintf(pch, "dCTR_ZERO_AND_NOT_1");break;
            case  4: pch += sprintf(pch, "IF_NOT");             break;
            case  5: pch += sprintf(pch, "IF_NOT_1");           break;
            case  6: pch += sprintf(pch, "IF_NOT_2");           break;
            case  7: pch += sprintf(pch, "IF_NOT_3");           break;
            case  8: pch += sprintf(pch, "dCTR_NZERO_AND");     break;
            case  9: pch += sprintf(pch, "dCTR_NZERO_AND_1");   break;
            case 10: pch += sprintf(pch, "dCTR_ZERO_AND");      break;
            case 11: pch += sprintf(pch, "dCTR_ZERO_AND_1");    break;
            case 12: pch += sprintf(pch, "IF_0");               break;
            case 13: pch += sprintf(pch, "IF_1");               break;
            case 14: pch += sprintf(pch, "IF_2");               break;
            case 15: pch += sprintf(pch, "IF_3");               break;
            case 16: pch += sprintf(pch, "dCTR_NZERO");         break;
            case 17: pch += sprintf(pch, "dCTR_NZERO_1");       break;
            case 18: pch += sprintf(pch, "dCTR_ZERO");          break;
            case 19: pch += sprintf(pch, "dCTR_ZERO_1");        break;
            case 20: pch += sprintf(pch, "ALWAYS");             break;
            case 21: pch += sprintf(pch, "ALWAYS_1");           break;
            case 22: pch += sprintf(pch, "ALWAYS_2");           break;
            case 23: pch += sprintf(pch, "ALWAYS_3");           break;
            case 24: pch += sprintf(pch, "dCTR_NZERO_8");       break;
            case 25: pch += sprintf(pch, "dCTR_NZERO_9");       break;
            case 26: pch += sprintf(pch, "dCTR_ZERO_8");        break;
            case 27: pch += sprintf(pch, "dCTR_ZERO_9");        break;
            case 28: pch += sprintf(pch, "ALWAYS_8");           break;
            case 29: pch += sprintf(pch, "ALWAYS_9");           break;
            case 30: pch += sprintf(pch, "ALWAYS_10");          break;
            case 31: pch += sprintf(pch, "ALWAYS_11");          break;
                }
            }                                     
        else
            pch += sprintf(pch, "%u", l);
        }
           
    if (idef.wfDis & wf_BI)
        {
        ULONG l = (code >> 16) & 31;

        if (Options & fUseFieldNames)
            {
            pch += sprintf(pch, "CR%d_", l >> 2);

            switch (l & 3)
                {
            case 0:  pch += sprintf(pch, "LT");  break;
            case 1:  pch += sprintf(pch, "GT");  break;
            case 2:  pch += sprintf(pch, "EQ");  break;
            case 3:  pch += sprintf(pch, "SO");  break;
                }
            }
        else
            pch += sprintf(pch, "%u", l);
        }

    if (idef.wfDis & wf_REL16)
        {
        // 16-bit relative branch addresses

        int l = (short int)code;

        if (idef.wfCPU & wfChkAA)
            pch += sprintf(pch, "0%08x", l);
        else
            {
            if (l < 0)
                pch += sprintf(pch, "*-0%x", -l);
            else
                pch += sprintf(pch, "*+0%x", l);

            pch += sprintf(pch, "   ; 0x%08X", lPC + l);
            }
        }

    // Null terminate the buffer.
    //
    *pch++ = 0;

    if (pch-buffer > bufLen)
    {
#if !defined(NDEBUG) || defined(BETA)
        printf("Disasm buffer overflow\n");
#endif
        return FALSE;
    }
    return ok;
}


int DisPPC(ULONG code)
{
    BYTE rgch[256];

    DisasmPPC(0, code, (void *)&rgch, sizeof(rgch), NULL);
#if PRINTF_ALLOWED
    printf("  %08X:  %-40s | %08X\n", 0, rgch, code);
#endif

    return TRUE;
}


#ifdef TEST

void __cdecl main()
{
    DisPPC(0x60000000);
    DisPPC(0x60050E00);
    DisPPC(0x7C0002D0);
    DisPPC(0x7C0002D1);
    DisPPC(0x7C000214);

    // these next few are from PowerMac G4

    DisPPC(0x13EC1AE0); // vmhaddshs v31,v12,v3,v11
    DisPPC(0x13EC19E0); // vmhaddshs v31,v12,v3,v7
    DisPPC(0x7C3B02A6); // mfsrrl    sp
    DisPPC(0x7C30FAA6); // mfspr     sp,HID0
    DisPPC(0x7C3243A6); // mtsprg    0x02,HID0

    DisPPC(StuffOp(31) | Stuff_rD(1) | Stuff_rA(2)  | StuffXop(360));
    DisPPC(StuffOp(31) | Stuff_rD(3) | Stuff_rA(1)  | StuffXop(360) | bitRc);
    DisPPC(StuffOp(31) | Stuff_rD(5) | Stuff_rA(0)  | StuffXop(360) | bitOE);
    DisPPC(StuffOp(31) | Stuff_rD(7) | Stuff_rA(31) | StuffXop(360) | bitOE | bitRc);

    DisPPC(StuffOp(12) | Stuff_rD(1) | Stuff_rA(2) | 0x1234);
    DisPPC(StuffOp(13) | Stuff_rD(1) | Stuff_rA(2) | 0xFFFF);
    DisPPC(StuffOp(14) | Stuff_rD(1) | Stuff_rA(2) | 0x0000);
    DisPPC(StuffOp(15) | Stuff_rD(1) | Stuff_rA(2) | 0x8001);

    {
    int i;

    for (i = 0; i < 64; i++)
        DisPPC(StuffOp(i));
    }

    printf("sfCpuLast = %d\n", sfCpuLast);
    printf("sfDisLast = %d\n", sfDisLast);
    printf("# of instructions = %d\n", vcNumInstrs);
}

#endif // TEST
