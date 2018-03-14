
/***************************************************************************

    68KOPS.C

    - This file defines the 68K instruction set.
    - Not to be built standalone, included by 68KDIS.C

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    06/06/2001  darekm      update long branches for SoftMac XP

***************************************************************************/

//
// Operand size queries
//
// Determines size based on the passed in first 32 bits of the instruction
//

ESIZE CheckBad(ESIZE e)     { return (e == SIZE_X) ? SIZE_BAD : e; }

ESIZE SizeNone(POPN p)      { return SIZE_X; } 
ESIZE SizeB   (POPN p)      { return SIZE_B; } 
ESIZE SizeW   (POPN p)      { return SIZE_W; } 
ESIZE SizeL   (POPN p)      { return SIZE_L; } 
ESIZE SizeBI  (POPN p)      { return SIZE_B; } 
ESIZE SizeWI  (POPN p)      { return SIZE_W; } 
ESIZE SizeLI  (POPN p)      { return SIZE_L; } 
ESIZE Size76  (POPN p)      { return CheckBad(SIZE_B + (p->xword >> 6) & 3); } 
ESIZE Size6   (POPN p)      { return CheckBad(SIZE_W + (p->xword >> 6) & 1); } 
ESIZE Size7   (POPN p)      { return CheckBad(SIZE_W + (p->xword >> 7) & 1); } 
ESIZE Size8   (POPN p)      { return CheckBad(SIZE_W + (p->xword >> 8) & 1); } 
ESIZE Size90  (POPN p)      { return CheckBad(SIZE_B + (p->xword >> 9) & 3); } 
ESIZE SizeFP  (POPN p)
{
    static const BYTE mpFPUSize[8] =
        {
        SIZE_L, SIZE_S, SIZE_E, SIZE_P, SIZE_W, SIZE_D, SIZE_B, SIZE_X
        };

    if (p->xword & 0x4000)  // check R/M bit
        return SIZE_E;

    return CheckBad(mpFPUSize[(p->xword >> 10) & 7]);
}

//
// Operand value validation
//

BOOL ValEaAny(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return FAny(mode, reg, size);
}

BOOL ValEaAlt(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return FAlt(mode, reg, size);
}

BOOL ValEaData(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return FData(mode, reg, size);
}

BOOL ValEaDAlt(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return FDataAlt(mode, reg, size);
}

BOOL ValEaMem(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return FMem(mode, reg, size);
}

BOOL ValEaCtl(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return FControl(mode, reg, size);
}


//
// Operand value extractions
//

BOOL ExtEa(POPN p)
{
    ULONG mode = (p->instr >> 3) & 7;
    ULONG reg  = (p->instr >> 0) & 7;
    ULONG size = p->size;

    return TRUE;
}



//
// Operand value display
//

ULONG CbDispDn(POPN p, BYTE *pch)     { return 0; }


#ifdef LATER

//
// Operand Descriptor objects
//

// OD const odInstrSize90[] = { NULL };
    

// CMP2,CHK2 (B,W,L)   <ea>,Rn

// OLIST const olC2[] = {
//    odInstrSize90,  odEaCntl,       odEaRn,         NULL,           NULL };


// ORI/ANDI/EORI (B)   #imm,CCR

// OLIST const olImmToCCR[] = {
//    odInstrSizeB,   odImmB,         odCCR,          NULL,           NULL };

#endif


#define BinOp(op)   NULL  // uRead0ToA,  uRead1ToB,  u##op,      uWriteBTo2
#define BitOp(op)   NULL  // uRead0ToA,  uRead1ToB,  uB##op,     uWriteBTo2

#define uExg        NULL // uRead0ToA,  uRead1ToB,  uExg,       uWriteBTo0, uWriteATo1

#define rguIllegal  NULL
#define rguReset    NULL
#define rguRtd      NULL
#define rguRte      NULL
#define rguRtr      NULL
#define rguStop     NULL
#define rguSwap     NULL
#define rguTrap     NULL
#define rguTrapV    NULL

#if defined(NEWJIT) && (zNEWJIT > 1)
#define UOP68K(op)  extern char op[]
#else
#define UOP68K(op)  enum { op };
#endif

UOP68K(rguMoveB);
UOP68K(rguMoveaW);
UOP68K(rguMoveW);
UOP68K(rguMoveaL);
UOP68K(rguMoveL);

UOP68K(rguMoveQ);
UOP68K(rguAddq);
UOP68K(rguAddqA);
UOP68K(rguSubq);
UOP68K(rguSubqA);

UOP68K(rguNop);
UOP68K(rguClr);
UOP68K(rguExtB);
UOP68K(rguExtW);
UOP68K(rguExtL);
UOP68K(rguNeg);
UOP68K(rguNegx);
UOP68K(rguNot);
UOP68K(rguTst);

UOP68K(rguJmp);
UOP68K(rguJsr);
UOP68K(rguLea);
UOP68K(rguPea);
UOP68K(rguRts);

UOP68K(rguBra8);
UOP68K(rguBra16);
UOP68K(rguBsr8);
UOP68K(rguBsr16);
UOP68K(rguBcc8);
UOP68K(rguBcc16);

UOP68K(rguLink16);
UOP68K(rguLink32);
UOP68K(rguUnlk);

UOP68K(rguBinOpImm);    // ORI   #imm,ea
UOP68K(rguBinOpM2R);    // OR    ea,Dn
UOP68K(rguBinOpM2A);    // ADDA  ea,An
UOP68K(rguBinOpR2M);    // OR    Dn,ea
UOP68K(rguShiftMem);    // shift.w #,ea

// use ST as shorthand for wfNoDecode, indicates a sub table entry

#define ST wfNoDecode

IDEF const line0[] =
{
    { "CHK2",   0x00C00000, 0xF9C00FFF, wfCpu68K32,     '9nk ', NULL },
    { "CMP2",   0x00C00800, 0xF9C00FFF, wfCpu68K32,     '9nk ', NULL },

    { "ORI",    0x003C0000, 0xFFFF0000, wfCpuAll68K,    '3#C ', BinOp(Or) },
    { "ANDI",   0x023C0000, 0xFFFF0000, wfCpuAll68K,    '3#C ', BinOp(And) },
    { "EORI",   0x0A3C0000, 0xFFFF0000, wfCpuAll68K,    '3#C ', BinOp(Xor) },

    { "ORI",    0x007C0000, 0xFFFF0000, wfCpuAll68K,    '3#S ', BinOp(Or) },
    { "ANDI",   0x027C0000, 0xFFFF0000, wfCpuAll68K,    '3#S ', BinOp(And) },
    { "EORI",   0x0A7C0000, 0xFFFF0000, wfCpuAll68K,    '3#S ', BinOp(Xor) },

    { "BTST",   0x01000000, 0xF1C00000, wfCpuAll68K,    'lxd ', BitOp(tst) },
    { "BTST",   0x08000000, 0xFFC0FF00, wfCpuAll68K,    'bBd ', BitOp(tst) },
    { "BCHG",   0x01400000, 0xF1C00000, wfCpuAll68K,    'lxv ', BitOp(chg) },
    { "BCHG",   0x08400000, 0xFFC0FF00, wfCpuAll68K,    'bBv ', BitOp(chg) },
    { "BCLR",   0x01800000, 0xF1C00000, wfCpuAll68K,    'lxv ', BitOp(clr) },
    { "BCLR",   0x08800000, 0xFFC0FF00, wfCpuAll68K,    'bBv ', BitOp(clr) },
    { "BSET",   0x01C00000, 0xF1C00000, wfCpuAll68K,    'lxv ', BitOp(set) },
    { "BSET",   0x08C00000, 0xFFC0FF00, wfCpuAll68K,    'bBv ', BitOp(set) },

    { "MOVEP",  0x01080000, 0xF1B80000, wfCpuAll68K,    '6sx ', NULL },
    { "MOVEP",  0x01880000, 0xF1B80000, wfCpuAll68K,    '6xs ', NULL },

    { "ORI",    0x00000000, 0xFF000000, wfCpuAll68K,    '3#v ', rguBinOpImm },
    { "ANDI",   0x02000000, 0xFF000000, wfCpuAll68K,    '3#v ', rguBinOpImm },
    { "SUBI",   0x04000000, 0xFF000000, wfCpuAll68K,    '3#v ', rguBinOpImm },
    { "ADDI",   0x06000000, 0xFF000000, wfCpuAll68K,    '3#v ', rguBinOpImm },
    { "EORI",   0x0A000000, 0xFF000000, wfCpuAll68K,    '3#v ', rguBinOpImm },
    { "CMPI",   0x0C000000, 0xFF000000, wfCpuAll68K,    '3#d ', rguBinOpImm },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const line4[] =
{
    // fixed instructions

    { "ILLEGAL",0x4AFC0000, 0xFFFF0000, wfCpuAll68K,    '    ', rguIllegal },
    { "RESET",  0x4E700000, 0xFFFF0000, wfCpuAll68K,    '    ', rguReset },
    { "NOP",    0x4E710000, 0xFFFF0000, wfCpuAll68K,    '    ', rguNop },
    { "STOP",   0x4E710000, 0xFFFF0000, wfCpuAll68K,    '    ', rguStop },
    { "RTE",    0x4E730000, 0xFFFF0000, wfCpuAll68K,    '    ', rguRte },
    { "RTD",    0x4E740000, 0xFFFF0000, wfCpuAll68K,    'w#  ', rguRtd },
    { "RTS",    0x4E750000, 0xFFFF0000, wfCpuAll68K,    '    ', rguRts },
    { "TRAPV",  0x4E760000, 0xFFFF0000, wfCpuAll68K,    '    ', rguTrapV },
    { "RTR",    0x4E770000, 0xFFFF0000, wfCpuAll68K,    '    ', rguRtr },
    { "MOVEC",  0x4E7A0000, 0xFFFF0400, wfCpuAll68K,    'lrk ', NULL },
    { "MOVEC",  0x4E7B0000, 0xFFFF0400, wfCpuAll68K,    'lkr ', NULL },

    { "SWAP",   0x48400000, 0xFFF80000, wfCpuAll68K,    ' y  ', rguSwap },
    { "EXT",    0x48800000, 0xFFF80000, wfCpuAll68K,    'Wy  ', rguExtW },
    { "EXT",    0x48C00000, 0xFFF80000, wfCpuAll68K,    'Ly  ', rguExtL },
    { "EXTB",   0x49C00000, 0xFFF80000, wfCpu68K32,     'Ly  ', rguExtB },

    { "LINK",   0x4E500000, 0xFFF80000, wfCpuAll68K,    'Wa#7', rguLink16 },
    { "LINK",   0x48080000, 0xFFF80000, wfCpu68K32,     'La#7', rguLink32 },
    { "UNLK",   0x4E580000, 0xFFF80000, wfCpuAll68K,    ' a7 ', rguUnlk },
    { "MOVE",   0x4E600000, 0xFFF80000, wfCpuAll68K,    'laU ', NULL },
    { "MOVE",   0x4E680000, 0xFFF80000, wfCpuAll68K,    'lUa ', NULL },

    { "TRAP",   0x4E400000, 0xFFF00000, wfCpuAll68K,    ' 4  ', rguTrap },

    { "MULU",   0x4C000000, 0xFFC08FF8, wfCpuAll68K,    'LdN ', BinOp(Mulu) }, 
    { "MULU",   0x4C000400, 0xFFC08FF8, wfCpuAll68K,    'Ld* ', BinOp(Mulu) }, 
    { "MULS",   0x4C000800, 0xFFC08FF8, wfCpuAll68K,    'LdN ', BinOp(Muls) }, 
    { "MULS",   0x4C000C00, 0xFFC08FF8, wfCpuAll68K,    'Ld* ', BinOp(Muls) }, 

    { "DIVU",   0x4C400000, 0xFFC08FF8, wfCpuAll68K,    'LdN ', BinOp(Divu) }, 
    { "DIVUL",  0x4C400400, 0xFFC08FF8, wfCpuAll68K,    'Ld/ ', BinOp(Divu) }, 
    { "DIVS",   0x4C400800, 0xFFC08FF8, wfCpuAll68K,    'LdN ', BinOp(Divs) }, 
    { "DIVSL",  0x4C400C00, 0xFFC08FF8, wfCpuAll68K,    'Ld/ ', BinOp(Divs) }, 

    { "JMP",    0x4EC00000, 0xFFC00000, wfCpuAll68K,    ' n  ', rguJmp },
    { "JSR",    0x4E800000, 0xFFC00000, wfCpuAll68K,    ' n  ', rguJsr },
    { "LEA",    0x41C00000, 0xF1C00000, wfCpuAll68K,    ' nA ', rguLea },
    { "PEA",    0x48400000, 0xFFC00000, wfCpuAll68K,    ' n  ', rguPea },

    { "CHK",    0x41000000, 0xF1400000, wfCpuAll68K,    '7dx ', NULL },

    { "MOVE",   0x40C00000, 0xFFC00000, wfCpuAll68K,    'wSv ', NULL },
    { "MOVE",   0x42C00000, 0xFFC00000, wfCpuAll68K,    'wCv ', NULL },
    { "MOVE",   0x44C00000, 0xFFC00000, wfCpuAll68K,    'wdC ', NULL },
    { "MOVE",   0x46C00000, 0xFFC00000, wfCpuAll68K,    'wdS ', NULL },
    { "NBCD",   0x48000000, 0xFFC00000, wfCpuAll68K,    'bv  ', NULL },
    { "TAS",    0x4AC00000, 0xFFC00000, wfCpuAll68K,    ' v  ', NULL },

    // duplicate entires with dummy low bit to force 4 byte instruction size

    { "MOVEM",  0x48800000, 0xFF800001, wfCpuAll68K,    '6<- ', NULL },
    { "MOVEM",  0x4C800000, 0xFF800001, wfCpuAll68K,    '6+> ', NULL },
    { "MOVEM",  0x48800001, 0xFF800001, wfCpuAll68K,    '6<- ', NULL },
    { "MOVEM",  0x4C800001, 0xFF800001, wfCpuAll68K,    '6+> ', NULL },

    { "NEGX",   0x40000000, 0xFF000000, wfCpuAll68K,    '3v  ', rguNegx },
    { "CLR",    0x42000000, 0xFF000000, wfCpuAll68K,    '3v  ', rguClr },
    { "NEG",    0x44000000, 0xFF000000, wfCpuAll68K,    '3v  ', rguNeg },
    { "NOT",    0x46000000, 0xFF000000, wfCpuAll68K,    '3v  ', rguNot },
    { "TST",    0x4A000000, 0xFF000000, wfCpuAll68K,    '3E  ', rguTst },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const line5[] =
{
    // Scc must come before TRAPcc because TRAPcc uses unused Scc modes

    { "S",      0x50C00000, 0xF0C00000, wfCpuAll68K,    ' cv ', NULL },

    { "TRAP",   0x50FC0000, 0xF0FF0000, wfCpuAll68K,    ' c  ', NULL },
    { "TRAP",   0x50F90000, 0xF0FF0000, wfCpuAll68K,    'Wc# ', NULL },
    { "TRAP",   0x50FA0000, 0xF0FF0000, wfCpuAll68K,    'Lc# ', NULL },

    { "DB",     0x50C80000, 0xF0F80000, wfCpuAll68K,    ' cyw', NULL },

    { "ADDQ",   0x50080000, 0xF1380000, wfCpuAll68K,    '3QV ', rguAddqA },
    { "SUBQ",   0x51080000, 0xF1380000, wfCpuAll68K,    '3QV ', rguSubqA },
    { "ADDQ",   0x50000000, 0xF1000000, wfCpuAll68K,    '3QV ', rguAddq },
    { "SUBQ",   0x51000000, 0xF1000000, wfCpuAll68K,    '3QV ', rguSubq },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const line6[] =
{
    // branches

    { "BRA",    0x60FF0000, 0xFFFF0000, wfCpu68K32,     'Ll  ', NULL },
    { "BRA",    0x60000000, 0xFFFF0000, wfCpuAll68K,    'Ww  ', rguBra16 },
    { "BRA",    0x60000000, 0xFF000000, wfCpuAll68K,    'Bb  ', rguBra8 },

    { "BSR",    0x61FF0000, 0xFFFF0000, wfCpu68K32,     'Ll  ', NULL },
    { "BSR",    0x61000000, 0xFFFF0000, wfCpuAll68K,    'Ww  ', rguBsr16 },
    { "BSR",    0x61000000, 0xFF000000, wfCpuAll68K,    'Bb  ', rguBsr8 },

    { "B",      0x60FF0000, 0xF0FF0000, wfCpu68K32,     'Lcl ', NULL },
    { "B",      0x60000000, 0xF0FF0000, wfCpuAll68K,    'Wcw ', rguBcc16 },
    { "B",      0x60000000, 0xF0000000, wfCpuAll68K,    'Bcb ', rguBcc8 },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const line8[] =
{
    { "DIVU",   0x80C00000, 0xF1C00000, wfCpuAll68K,    'Wdx ', NULL }, 
    { "DIVS",   0x81C00000, 0xF1C00000, wfCpuAll68K,    'Wdx ', NULL }, 

    { "OR",     0x80000000, 0xF1000000, wfCpuAll68K,    '3Ex ', rguBinOpM2R },
    { "OR",     0x81000000, 0xF1000000, wfCpuAll68K,    '3xM ', rguBinOpR2M },

    { "SBCD",   0x81000000, 0xF1F80000, wfCpuAll68K,    'byx ', NULL },
    { "SBCD",   0x81080000, 0xF1F80000, wfCpuAll68K,    'byx ', NULL },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const line9[] =
{
    { "SUBA",   0x90C00000, 0xF0C00000, wfCpuAll68K,    '8EA ', rguBinOpM2A },

    { "SUBX",   0x91000000, 0xF1380000, wfCpuAll68K,    '3yx ', NULL },
    { "SUBX",   0x91080000, 0xF1380000, wfCpuAll68K,    '3Zz ', NULL },

    { "SUB",    0x90000000, 0xF1000000, wfCpuAll68K,    '3Ex ', rguBinOpM2R },
    { "SUB",    0x91000000, 0xF1000000, wfCpuAll68K,    '3xM ', rguBinOpR2M },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const lineB[] =
{
    { "CMPA",   0xB0C00000, 0xF0C00000, wfCpuAll68K,    '8EA ', rguBinOpM2A },

    { "CMPM",   0xB1080000, 0xF1380000, wfCpuAll68K,    '3YX ', NULL },

    { "CMP",    0xB0000000, 0xF1000000, wfCpuAll68K,    '3Ex ', rguBinOpM2R },
    { "EOR",    0xB1000000, 0xF1000000, wfCpuAll68K,    '3xv ', rguBinOpR2M }, 

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const lineC[] =
{
    { "EXG",    0xC1400000, 0xF1F80000, wfCpuAll68K,    ' xy ', uExg }, 
    { "EXG",    0xC1480000, 0xF1F80000, wfCpuAll68K,    ' Aa ', uExg }, 
    { "EXG",    0xC1880000, 0xF1F80000, wfCpuAll68K,    ' xa ', uExg }, 

    { "MULU",   0xC0C00000, 0xF1C00000, wfCpuAll68K,    'Wdx ', BinOp(Mulu) }, 
    { "MULS",   0xC1C00000, 0xF1C00000, wfCpuAll68K,    'Wdx ', BinOp(Muls) }, 

    { "AND",    0xC0000000, 0xF1000000, wfCpuAll68K,    '3Ex ', rguBinOpM2R }, 
    { "AND",    0xC1000000, 0xF1000000, wfCpuAll68K,    '3xM ', rguBinOpR2M }, 

    { "ABCD",   0xC1000000, 0xF1F80000, wfCpuAll68K,    'byx ', NULL },
    { "ABCD",   0xC1080000, 0xF1F80000, wfCpuAll68K,    'bZz ', NULL },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const lineD[] =
{
    { "ADDA",   0xD0C00000, 0xF0C00000, wfCpuAll68K,    '8EA ', rguBinOpM2A },

    { "ADDX",   0xD1000000, 0xF1380000, wfCpuAll68K,    '3yx ', NULL },
    { "ADDX",   0xD1080000, 0xF1380000, wfCpuAll68K,    '3Zz ', NULL },

    { "ADD",    0xD0000000, 0xF1000000, wfCpuAll68K,    '3Ex ', rguBinOpM2R },
    { "ADD",    0xD1000000, 0xF1000000, wfCpuAll68K,    '3xM ', rguBinOpR2M },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const lineE[] =
{
    { "BFTST",  0xE8C00000, 0xFFC00000, wfCpu68K32,     ' I  ', NULL },
    { "BFEXTU", 0xE9C00000, 0xFFC00000, wfCpu68K32,     ' I  ', NULL },
    { "BFCHG",  0xEAC00000, 0xFFC00000, wfCpu68K32,     ' i  ', NULL },
    { "BFEXTS", 0xEBC00000, 0xFFC00000, wfCpu68K32,     ' I  ', NULL },
    { "BFCLR",  0xECC00000, 0xFFC00000, wfCpu68K32,     ' i  ', NULL },
    { "BFFFO",  0xEDC00000, 0xFFC00000, wfCpu68K32,     ' I  ', NULL },
    { "BFSET",  0xEEC00000, 0xFFC00000, wfCpu68K32,     ' i  ', NULL },
    { "BFINS",  0xEFC00000, 0xFFC00000, wfCpu68K32,     ' i  ', NULL },

    { "",       0xE0C00000, 0xF8C00000, wfCpuAll68K,    'WHM ', NULL /* rguShiftMem */ },
    { "",       0xE0000000, 0xF0200000, wfCpuAll68K,    '3hQy', NULL },
    { "",       0xE0200000, 0xF0200000, wfCpuAll68K,    '3hxy', NULL },

    { "",       0,          0,          0,              0,      NULL },
};

IDEF const lineF[] =
{
    { "FNOP",   0xF2800000, 0xFFFFFFFF, wfCpu68K32,     '    ', NULL },
    { "FMOVECR",0xF2005C00, 0xFFFFFC00, wfCpu68K32,     '    ', NULL },
    { "F",      0xF2000000, 0xFFC0A000, wfCpu68K32,     '    ', NULL },

    { "FScc",   0xF2400000, 0xFFC0FFC0, wfCpu68K32,     '    ', NULL },
    { "FDBcc",  0xF2480000, 0xFFF8FFC0, wfCpu68K32,     '    ', NULL },
    { "FTRAPcc",0xF2780000, 0xFFF8FFC0, wfCpu68K32,     '    ', NULL },

    { "FMOVE",  0xF2006000, 0xFFC0E000, wfCpu68K32,     '    ', NULL },
    { "FMOVE",  0xF2008000, 0xFFC0C3FF, wfCpu68K32,     '    ', NULL },

    { "FSAVE",  0xF3000000, 0xFFC00000, wfCpu68K32,     ' -  ', NULL },
    {"FRESTORE",0xF3400000, 0xFFC00000, wfCpu68K32,     ' +  ', NULL },

    { "MOVE16", 0xF6000000, 0xFFE00000, wfCpu68K32,     '    ', NULL },
    { "MOVE16", 0xF6200000, 0xFFF80000, wfCpu68K32,     '    ', NULL },

    { "Line F", 0xF0000000, 0xF0000000, wfCpuAll68K,    '    ', NULL },
    { "",       0,          0,          0,              0,      NULL },
};

//
// Main 68K table, branches off to sub-tables by marking the CPU flags with
// the no decode flag. We do this to help speed up search time
//

IDEF const rgidef68[] =
{
    { "",       0x00000000, 0xF0000000, wfCpuAll68K|ST, &line0, NULL },

    // MOVE instructions (watch the order, it's B L W)

    { "MOVE",   0x10000000, 0xF0000000, wfCpuAll68K,    'BED ', rguMoveB },
    { "MOVEA",  0x20400000, 0xF1C00000, wfCpuAll68K,    'LED ', rguMoveaL },
    { "MOVE",   0x20000000, 0xF0000000, wfCpuAll68K,    'LED ', rguMoveL },
    { "MOVEA",  0x30400000, 0xF1C00000, wfCpuAll68K,    'WED ', rguMoveaW },
    { "MOVE",   0x30000000, 0xF0000000, wfCpuAll68K,    'WED ', rguMoveW },

    { "",       0x40000000, 0xF0000000, wfCpuAll68K|ST, &line4, NULL },

    { "",       0x50000000, 0xF0000000, wfCpuAll68K|ST, &line5, NULL },

    { "",       0x60000000, 0xF0000000, wfCpuAll68K|ST, &line6, NULL },

    { "MOVEQ",  0x70000000, 0xF1000000, wfCpuAll68K,    ' qx ', rguMoveQ },

    // arithmetic operatings

    { "",       0x80000000, 0xF0000000, wfCpuAll68K|ST, &line8, NULL },
    { "",       0x90000000, 0xF0000000, wfCpuAll68K|ST, &line9, NULL },

    { "",       0xB0000000, 0xF0000000, wfCpuAll68K|ST, &lineB, NULL },
    { "",       0xC0000000, 0xF0000000, wfCpuAll68K|ST, &lineC, NULL },
    { "",       0xD0000000, 0xF0000000, wfCpuAll68K|ST, &lineD, NULL },
    { "",       0xE0000000, 0xF0000000, wfCpuAll68K|ST, &lineE, NULL },

    // LINE A does nothing special

    { "Line A", 0xA0000000, 0xF0000000, wfCpuAll68K,    '    ', NULL },

    // LINE F is nothing special on 68000 and 68010, FPU on others

    { "",       0xF2000000, 0xFE000000, wfCpu68K32|ST,  &lineF, NULL },

    { "PMOVET0",0xF0000800, 0xFFC0FCFF, wfCpu68K32,     ' n  ', NULL },
    { "PMOVET1",0xF0000C00, 0xFFC0FCFF, wfCpu68K32,     ' n  ', NULL },
    { "PLOADR", 0xF0002000, 0xFFC0FFE0, wfCpu68K32,     ' n  ', NULL },
    { "PLOADW", 0xF0002200, 0xFFC0FFE0, wfCpu68K32,     ' n  ', NULL },

    { "PFLUSHA",0xF0002400, 0xFFC0FF00, wfCpu68K32,     '    ', NULL },
    { "PFLUSH", 0xF0003000, 0xFFC0FF00, wfCpu68K32,     '    ', NULL },
    { "PFLUSH", 0xF0003800, 0xFFC0FF00, wfCpu68K32,     ' n  ', NULL },

    { "PMOVETC",0xF0004000, 0xFFC0FCFF, wfCpu68K32,     ' n  ', NULL },
    {"PMOVESRP",0xF0004800, 0xFFC0FCFF, wfCpu68K32,     ' n  ', NULL },
    {"PMOVECRP",0xF0004C00, 0xFFC0FCFF, wfCpu68K32,     ' n  ', NULL },
    { "PMOVE U",0xF0006000, 0xFFC0FDFF, wfCpu68K32,     ' n  ', NULL },

    { "PTESTR ",0xF0008000, 0xFFC0E000, wfCpu68K32,     ' n  ', NULL },

    { "Line F", 0xF0000000, 0xF0000000, wfCpu68K24,     '    ', NULL },

    // sentinel to mark end of table

    { "",       0,          0,          0,              0,      NULL },
};

