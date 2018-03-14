
/***************************************************************************

    68KDIS.C

    - 68000 disassembler module.

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    06/20/2001  darekm      last update
  
***************************************************************************/

#ifdef TEST
#include <windows.h>
#include <stdio.h>
#else
#include "gemtypes.h"
#endif

#if defined(ATARIST) || defined(SOFTMAC)

// #pragma optimize("",off)

#ifdef POWERMAC
#include "ppcdis.c"
#endif

#include "asm68k.h"
#include "68kops.c"

//
// Maps the 3-bit size field in FPU instructions to an appropriate SIZE_*
//

const BYTE mpFPUSize[8] =
{
    SIZE_L, SIZE_S, SIZE_E, SIZE_P, SIZE_W, SIZE_D, SIZE_B, SIZE_X
};


//
// These helper fuctions extract a value from the instruction stream
//

ULONG __inline LFromRgbI(unsigned const char *pb, int i)
{
    return (pb[i] << 24) | (pb[i+1] << 16) | (pb[i+2] << 8) | pb[i+3];
}

WORD  __inline WFromRgbI(unsigned const char *pb, int i)
{
    return (pb[i] << 8) | pb[i+1];
}

int __inline D8FromInstr(ULONG instr)
{
    return (int)(short int)instr;
}

int __inline Imm3FromInstr(ULONG instr)
{
    return (ULONG)((((instr >> 9) - 1) & 7) + 1);
}

int __inline Imm8FromInstr(ULONG instr)
{
    return (ULONG)(unsigned short int)instr;
}


//
// Size68K(pidef, pbCode)
//
// Return true the operand size of the instruction. This handles cases where
// the operand size is encoded in the instruction and where it is implicit.
//

ESIZE68K Size68K(IDEF const * const pidef, unsigned const char * pbCode)
{
    const ULONG instr = WFromRgbI(pbCode, 0);
    const ULONG xword = WFromRgbI(pbCode, 2);
    ULONG size;

    switch(_68kSiz(pidef->wfDis))
        {
    default:
        goto Lerror;

    case eNoSize:
        size = SIZE_X; // no size
        break;

    case eSizeB:
    case eSizeBI:
        size = SIZE_B;
        break;

    case eSizeW:
    case eSizeWI:
        size = SIZE_W;
        break;

    case eSizeL:
    case eSizeLI:
        size = SIZE_L;
        break;

    case eSize76:
        size = SIZE_B + ((instr >> 6) & 3);

        if (size == SIZE_X)
            goto Lerror;
        break;

    case eSize90:
        size = SIZE_B + ((instr >> 9) & 3);

        if (size == SIZE_X)
            goto Lerror;
        break;

    case eSize6:
        size = SIZE_W + ((instr >> 6) & 1);
        break;

    case eSize7:
        size = SIZE_L - ((instr >> 7) & 1); // bit is reversed!
        break;

    case eSize8:
        size = SIZE_W + ((instr >> 8) & 1);
        break;

    case eSizeFP:
        if (xword & 0x4000)     // check R/M bit
            size = SIZE_E;
        else
            {
            size = mpFPUSize[(xword >> 10) & 7];

            if (size == SIZE_X)
                goto Lerror;
            }
        break;
        }

    return size;

Lerror:
    return SIZE_BAD;
}


//
// FValid68K(pidef, pbCode)
//
// Return true if the instruction appears to be a valid match for the
// template at pidef. Data operands like addressing extension words are
// not checked here since they do not affect the decoding on the instruction.
// The instruction can still turn out to be valid as a result, but what this
// will guarantee is that no other instruction can match.
//

BOOL FValid68K(IDEF const * const pidef, unsigned const char * pbCode)
{
    ULONG j;
    ULONG instr = WFromRgbI(pbCode, 0);
    ULONG mode  = (instr >> 3) & 7;
    ULONG reg   = (instr >> 0) & 7;
    ULONG size  = Size68K(pidef, pbCode);

    // verify the operands to weed out bogus addressing modes
    // Note: we don't verify data operands, so immediates and addressing
    // extensions words are not checked! Those produce runtime exceptions
    // if they are invalid

    if (size == SIZE_BAD)
        return FALSE;

    for (j = 0; j < MAX_OPN68; j++)
        {
        BYTE b = (BYTE)_68kOpN(pidef->wfDis, j);

// printf("checking parameter %d, type = %02X '%c'\n", j, b, b);

        switch(b)
            {
        default:
            printf("ERROR: unknown operand descriptor %d\n", b);
            return FALSE;

        // these guys don't need checking

        case eNone:
        case eCCR:
        case eSR:
        case eUSP:
        case eSP:
        case eImm3:
        case eVec3:
        case eVec4:
        case eDy:
        case eDx:
        case eAxi:
        case eAyi:
        case edAx:
        case edAy:
        case eCC:
        case eImm8:
        case eImm16:
        case eImm32:
        case eImm:
        case eQ8:
        case eD8:
        case eD16:
        case eD32:
        case eS16:
        case eRegsS:
        case eRegsD:
        case eDn:
        case eRn:
        case eRn2:
        case eDc:
        case eDu:
        case eDhDl:
        case eDrDq:
        case eDc1Dc2:
        case eDu1Du2:
        case eRn1Rn2:
        case eSPR68:
        case eBF:
        case eSH:
        case eSHM:
            break;

        case eAx:
        case eAy:
            if (size == SIZE_B)
                return FALSE;

            break;

        case eEaDest:
            mode = (instr >> 6) & 7;
            reg =  (instr >> 9) & 7;

            b = eEaAlt;

            // fall through

        case eEaAll:
        case eEaAllFP:
        case eEaData:
        case eEaCntl:
        case eEaBitf:
        case eEaBitfA:
        case eEaPred:
        case eEaPost:
        case eEaAlt:
        case eEaDAlt:
        case eEaMAlt:
            if (!FAny(mode, reg, size))
                return FALSE;

            // weed out modes that don't support An

            if (mode == 1)
                {
                if (size == SIZE_B)
                    return FALSE;

                if ((b != eEaAll) && (b != eEaAlt))
                    return FALSE;
                }

            // weed out modes that don't support immediates

            if ((mode == 7) && (reg == 4))
                {
                if ((b != eEaAll) && (b != eEaAllFP) && (b != eEaData))
                    return FALSE;
                }

            // weed out modes that don't support PC relative addressing

            if ((mode == 7) && ((reg == 2) || (reg == 3)))
                {
                if ((b != eEaAll) && (b != eEaAllFP) && (b != eEaData) && (b != eEaCntl) && (b != eEaBitf) && (b != eEaPost))
                    return FALSE;
                }

            // weed out modes that don't support (An)+ or -(An)

            if (mode == 3)
                {
                if ((b == eEaPred) || (b == eEaCntl) || (b == eEaBitf))
                    return FALSE;
                }

            if (mode == 4)
                {
                if ((b == eEaPost) || (b == eEaCntl) || (b == eEaBitf))
                    return FALSE;
                }

            // weed out modes that don't support Dn

            if ((b==eEaCntl) || (b==eEaMAlt) || (b==eEaPred) || (b==eEaPost))
                {
                if (mode == 0)
                    return FALSE;
                }


            break;
            }

// printf("PASSED!\n");
        }

    return TRUE;
}

//
// Decode the operands of the instruction and return the instruction size.
//
// Input:
//      pidef   - pointer to instruction definition
//      pbCode  - pointer to instruction code stream
//      popn    - pointer to OPN structure array to fill in for this operand
//
// Returns:
//      how many bytes of code stream were decoded, or 0 on an error
//

ULONG CbDecode68K(IDEF const * const pidef, unsigned const char * pbCode,
                  OPN *popn)
{
    WORD  instr = WFromRgbI(pbCode, 0);
    WORD  xword = WFromRgbI(pbCode, 2);
    ULONG lword = LFromRgbI(pbCode, 2);
    ULONG size  = Size68K(pidef, pbCode);
    int   j;

    // Figure out the starting instruction size based on width of mask.
    // This is fudged as each operand is processed

    ULONG cb = (pidef->lMask & 0x0000FFFF) ? 4 : 2;

    for (j = 0; j < MAX_OPN68; j++, popn++)
        {
        BYTE mode = (instr >> 3) & 7;
        BYTE reg  = (instr >> 0) & 7;
        BYTE b    = (BYTE)_68kOpN(pidef->wfDis, j);

        memset(popn, 0, sizeof(OPN));
        popn->instr = instr;    // instruction word the same for all operands
        popn->type  = b;        // the type of operand

        switch(b)
            {
        default:
            printf("ERROR: unknown operand descriptor %d\n", b);
            return 0;

        // these guys contain no data

        case eNone:
        case eCCR:
        case eSR:
        case eUSP:
        case eSP:
            break;

        case eImm3:
            popn->l = Imm3FromInstr(instr);
            break;

        case eVec3:
            popn->l = instr & 7;
            break;

        case eVec4:
            popn->l = instr & 15;
            break;

        case eDy:
            popn->reg = (BYTE)(instr & 7);
            break;

        case eDx:
            popn->reg = (BYTE)((instr >> 9) & 7);
            break;

        case eAx:
        case edAx:
        case eAxi:
            popn->reg = (BYTE)(8 + (instr >> 9) & 7);
            break;

        case eAy:
        case edAy:
        case eAyi:
            popn->reg = (BYTE)(8 + instr & 7);
            break;

        case eImm8:
imm8:
            popn->l = xword & 255;
            cb = 4;
            break;

        case eImm16:
imm16:
            popn->l = xword;
            cb = 4;
            break;

        case eImm32:
imm32:
            popn->l = lword;
            cb = 6;
            break;

        case eImm:
            if (size == SIZE_B)
                goto imm8;
            if (size == SIZE_W)
                goto imm16;
            if (size == SIZE_L)
                goto imm32;
            break;

        case eQ8:
            popn->l = (unsigned char)instr;
            cb = 2;
            break;

        case eD8:
            popn->disp = (long)(signed char)instr;
            cb = 2;
            break;

        case eD16:
            popn->disp = (long)(short)xword;
            cb = 4;
            break;

        case eD32:
            popn->disp = lword;
            cb = 6;
            break;

        case eS16:
            popn->disp = xword;
            popn->reg  = (BYTE)(instr & 7);
            cb = 4;
            break;

        case eRegsS:
        case eRegsD:
            popn->l = xword;
            break;

        case eDn:
            popn->reg = (xword >> 12) & 7;
            break;

        case eRn:
            popn->reg = (xword >> 12) & 15;
            break;

        case eRn2:
            popn->reg = (BYTE)((lword >> 12) & 15);
            break;

        case eDc:
            popn->reg = xword & 7;
            break;

        case eDu:
            popn->reg = (xword >> 6) & 7;
            break;

        case eDhDl:
        case eDrDq:
            popn->reg  = (BYTE)(xword & 7);
            popn->reg2 = (BYTE)((xword >> 12) & 7);
            break;

        case eDc1Dc2:
            popn->reg  = (BYTE)(xword & 7);
            popn->reg2 = (BYTE)(lword & 7);
            break;

        case eDu1Du2:
            popn->reg  = (BYTE)((xword >> 6) & 7);
            popn->reg2 = (BYTE)((lword >> 6) & 7);
            break;

        case eRn1Rn2:
            popn->reg  = (BYTE)((xword >> 12) & 15);
            popn->reg2 = (BYTE)((lword >> 12) & 15);
            break;

        case eSPR68:
            if (xword & 0x800)
                popn->l = (xword & 7) + 8;
            else
                popn->l = (xword & 7);
            cb = 4;
            break;

        case eCC:
        case eSHM:
            popn->l = (instr >> 8) & 15;
            break;

        case eBF:
            break;

        case eSH:
            popn->l = ((instr >> 2) & 6) + ((instr >> 8) & 1);
            break;

        case eEaDest:
            mode = (BYTE)((instr >> 6) & 7);
            reg =  (BYTE)((instr >> 9) & 7);

            // fall through

        case eEaAll:
        case eEaAllFP:
        case eEaData:
        case eEaCntl:
        case eEaBitf:
        case eEaBitfA:
        case eEaPred:
        case eEaPost:
        case eEaAlt:
        case eEaDAlt:
        case eEaMAlt:
            if (mode == 0)
                popn->reg = reg;

            else if (mode == 1)
                popn->reg = 8 + reg;

            else if (mode == 2)
                popn->reg = 8 + reg;

            else if (mode == 3)
                popn->reg = 8 + reg;

            else if (mode == 4)
                popn->reg = 8 + reg;

            else if (mode == 5)
                {
                popn->disp = (short) WFromRgbI(pbCode, cb);
                cb += 2;
                popn->reg = 8 + reg;
                }
            else if (mode == 6)
                {
                FFEW ffew;

indexed:
                ffew.w = (WORD)WFromRgbI(pbCode, cb);
                popn->ew = ffew;
                popn->reg = reg;
                popn->reg2 = (BYTE)ffew.ireg;
                cb += 2;

                if (ffew.fFull)
                    {
                    switch(ffew.BDSIZ)
                        {
                    default:
                        return 0;

                    case 1:
                        break;

                    case 2:
                        popn->bd = (long)(short)WFromRgbI(pbCode, cb);
                        cb += 2;
                        break;

                    case 3:
                        popn->bd = LFromRgbI(pbCode, cb);
                        cb += 4;
                        break;
                        }

                    switch(ffew.IIS)
                        {
                    default:
                        return 0;

                    case 0:
                    case 8:
                        break;

                    case 1:
                    case 5:
                    case 9:
                        break;

                    case 2:
                    case 6:
                    case 10:
                        popn->od = (long)(short)WFromRgbI(pbCode, cb);
                        cb += 2;
                        break;

                    case 3:
                    case 7:
                    case 11:
                        popn->od = LFromRgbI(pbCode, cb);
                        cb += 4;
                        break;
                        }
                    }
                else if (mode == 6)
                    popn->disp = (long)(signed char)ffew.d8;

                else
                    popn->disp = (long)(signed char)ffew.d8 + cb - 2;

                }
            else if (reg == 0)
                {
                popn->addr = (long)(short) WFromRgbI(pbCode, cb);
                cb += 2;
                }
            else if (reg == 1)
                {
                popn->addr = LFromRgbI(pbCode, cb);
                cb += 4;
                }
            else if (reg == 2)
                {
                popn->disp = cb + (long)(short) WFromRgbI(pbCode, cb);
                cb += 2;
                }
            else if (reg == 4)
                {
                if (size == SIZE_B)
                    {
                    popn->l = WFromRgbI(pbCode, cb) & 255;
                    cb += 2;
                    }
                else if (size == SIZE_W)
                    {
                    popn->l = WFromRgbI(pbCode, cb);
                    cb += 2;
                    }
                else if (size == SIZE_L)
                    {
                    popn->l = LFromRgbI(pbCode, cb);
                    cb += 4;
                    }
                else
                    return 0;
                }
            else
                goto indexed;
            break;
            }
        }

    return cb;
}


//
// PIdefLookup68K(pidef, pbCode)
//
// Return true if a recursive lookup on the instruction table brings up a match
//

IDEF *PIdefLookup68K(IDEF *pidef, unsigned const char * pbCode)
{
    ULONG lCode = LFromRgbI(pbCode, 0);

    for (;; pidef++)
        {
        ULONG cch = strlen(pidef->szMne);

        if (pidef->lMask == 0)
            break;

#if 0
printf("checking %08X %08X %08X %s\n",
     pidef->lMask, lCode, pidef->lCode, pidef->szMne);
#endif

        if ((pidef->lMask & lCode) != pidef->lCode)
            continue;

        // check for recursion

        if (pidef->wfCPU & ST)
            return PIdefLookup68K((IDEF *)pidef->wfDis, pbCode);

        // make sure the instruction breaks down legally

//printf("checking validity of %s\n", pidef->szMne);

        if (!FValid68K(pidef, pbCode))
            continue;

        // we've got a match

// printf("looks good!\n");
        return pidef;
        }

    return NULL;    // nothing found
}

#if defined(NEWJIT)

int PreJit68K
(
    OPN                 * popn[MAX_OPN68], // decode operands
    unsigned const char * pbCode,   // byte stream to disassemble
    IDEF                **ppidef,   // idef pointer that gets set
    ULONG               * psize     // instruction data size
)
{
    ULONG cb;                       // total size of instruction, or 0 if bogus

    *ppidef = PIdefLookup68K((IDEF *)&rgidef68, pbCode);

    if (*ppidef == NULL)
        return 0;

    // determine the data size of the instruction, if there is any

    *psize = Size68K(*ppidef, pbCode);

    if (*psize == SIZE_BAD)
        return 0;

    // decode the operands and calculate instruction size

    cb = CbDecode68K(*ppidef, pbCode, popn);

    return cb;
}

#endif // NEWJIT

int Disasm68K
(
    unsigned const int    lPC,      // Program Counter
    unsigned const char * pbCode,   // byte stream to disassemble
    char                * buffer,   // buffer in which to disassemble
    int                   bufLen,   // Length of buffer.        
    char                * symName   // symbol name to use in disassembly.
)
{
    IDEF *pidef = PIdefLookup68K((IDEF *)&rgidef68, pbCode);
    int  cch;                       // length of output string
    ULONG cb;                       // total size of instruction, or 0 if bogus
    ULONG j, k;
    ULONG mode, reg, size;
    ULONG esize;
    OPN   rgopn[MAX_OPN68];         // decode operands

    buffer[0] = 0;

    if (pidef == NULL)
        return 0;

    // determine the data size of the instruction, if there is any

    esize = _68kSiz(pidef->wfDis);
    size = Size68K(pidef, pbCode);

    if (size == SIZE_BAD)
        goto Lerror;

    // decode the operands and calculate instruction size

    cb = CbDecode68K(pidef, pbCode, rgopn);

    if (cb == 0)
        return 0;

    cch = sprintf(buffer, "%s", pidef->szMne);

    for (j = 0; j < MAX_OPN68; j++)
        {
        BYTE b     = rgopn[j].type;

        char const * const mpCcSz[16] =
            {
            "T",  "F",  "HI", "LS", "CC", "CS", "NE", "EQ",
            "VC", "VS", "PL", "MI", "GE", "LT", "GT", "LE"
            };

        char const * const mpShSz[8] =
            {
            "ASR", "ASL", "LSR", "LSL", "ROXR", "ROXL", "ROR", "ROL"
            };

        if (b == eCC)
            {
            cch += sprintf(buffer + cch, "%s", mpCcSz[rgopn[j].l]);
            }

        else if (b == eFCC)
            {
            }

        else if (b == eSH)
            {
            cch += sprintf(buffer + cch, "%s", mpShSz[rgopn[j].l]);
            }

        else if (b == eSHM)
            {
            cch += sprintf(buffer + cch, "%s", mpShSz[rgopn[j].l]);
            }
        }

    // Don't display a size if it is implicit or there is none

    if ((size != SIZE_X) &&
        (esize != eSizeBI) && (esize != eSizeWI) && (esize != eSizeLI))
        {
        static const BYTE rgbSizes[] = { 'B','W','L','?','S','D','X','P'};

        WORD instr = rgopn[0].instr;

        if (((instr >> 12) == 6) && (size == SIZE_B))
            cch += sprintf(buffer + cch, ".S");
        else if (((instr >> 12) == 6) && (size == SIZE_W))
            ; // cch += sprintf(buffer + cch, ".W");
        else
            cch += sprintf(buffer + cch, ".%c", rgbSizes[size]);
        }

    // Pad out the mnemonic

    while (cch < 8)
        {
        buffer[cch++] = ' ';
        }

    // Now display each operand

    k = 0;          // count of operands

    for (j = 0; j < MAX_OPN68; j++)
        {
        BYTE b = (BYTE)_68kOpN(pidef->wfDis, j);

        if (b == eNone)
            break;

        k++;

        if (k > 1)
            {
            buffer[cch++] = ',';
            }

        switch(b)
            {
        default:
            printf("ERROR: unknown operand descriptor %d\n", b);
            break;

        case eCCR:
            cch += sprintf(buffer+cch, "CCR");
            break;

        case eSR:
            cch += sprintf(buffer+cch, "SR");
            break;

        case eUSP:
            cch += sprintf(buffer+cch, "USP");
            break;

        case eSP:
            cch--;  // this is an invisible parameter
            break;

        case eImm3:
            cch += sprintf(buffer+cch, "#%d", rgopn[j].l);
            break;

        case eImm:
            if (size == SIZE_B)
                goto imm8;
            if (size == SIZE_W)
                goto imm16;
            if (size == SIZE_L)
                goto imm32;
            break;

        case eImm8:
imm8:
            cch += sprintf(buffer+cch, "#$%02X", rgopn[j].l);
            break;

        case eImm16:
imm16:
            cch += sprintf(buffer+cch, "#$%04X", rgopn[j].l);
            break;

        case eImm32:
imm32:
            cch += sprintf(buffer+cch, "#$%08X", rgopn[j].l);
            break;

        case eQ8:
            cch += sprintf(buffer+cch, "#$%02X", rgopn[j].l);
            break;

        case eD8:
        case eD16:
        case eD32:
            cch += sprintf(buffer+cch, "$%08X", lPC + 2 + rgopn[j].disp);
            break;

        case eS16:
            cch += sprintf(buffer+cch, "%04X(A%d)", rgopn[j].disp & 65535, rgopn[j].reg);
            break;

        case eVec3:
            cch += sprintf(buffer+cch, "%d", rgopn[j].l);
            break;

        case eVec4:
            cch += sprintf(buffer+cch, "#%d", rgopn[j].l);
            break;

        case eCC:
        case eBF:
        case eSH:
        case eSHM:
            k--;        // field was part of mnemonic
            break;

        case eDy:
        case eDx:
        case eDn:
        case eDc:
        case eDu:
dn:
            cch += sprintf(buffer+cch, "D%d", rgopn[j].reg & 7);
            break;

        case eAx:
        case eAy:
an:
            cch += sprintf(buffer+cch, "A%d", rgopn[j].reg & 7);
            break;

        case eAxi:
        case eAyi:
            cch += sprintf(buffer+cch, "(A%d)+", rgopn[j].reg & 7);
            break;

        case edAx:
        case edAy:
            cch += sprintf(buffer+cch, "-(A%d)", rgopn[j].reg & 7);
            break;

        case eRn:
        case eRn2:
            if (rgopn[j].reg & 8)
                goto an;
            else
                goto dn;
            break;

        case eRn1Rn2:
            if (rgopn[j].reg & 8)
                cch += sprintf(buffer+cch, "A%d", rgopn[j].reg & 7);
            else
                cch += sprintf(buffer+cch, "D%d", rgopn[j].reg & 7);

            if (rgopn[j].reg2 & 8)
                cch += sprintf(buffer+cch, "A%d", rgopn[j].reg2 & 7);
            else
                cch += sprintf(buffer+cch, "D%d", rgopn[j].reg2 & 7);

            break;

        case eDc1Dc2:
        case eDu1Du2:
        case eDhDl:
        case eDrDq:
            cch += sprintf(buffer+cch, "D%d:D%d", rgopn[j].reg, rgopn[j].reg2);
            break;

        case eRegsS:
        case eRegsD:
            cch += sprintf(buffer+cch, "%04X", rgopn[j].l);
            break;

        case eSPR68:
            {
            char const * const mpszSPR[16] =
                {
                "SFC",  "DFC",  "CACR", "TCR", 
                "ITT0", "ITT1", "DTT0", "DTT1",
                "USP",  "VBR",  "CAAR", "MSP",
                "ISP",  "MMUSR", "URP", "SRP",
                };

            cch += sprintf(buffer+cch, "%s", mpszSPR[rgopn[j].l]);
            }
            break;

        case eEaDest:
            mode = (rgopn[j].instr >> 6) & 7;
            reg =  (rgopn[j].instr >> 9) & 7;
            goto ea;

        case eEaAll:
        case eEaAllFP:
        case eEaData:
        case eEaCntl:
        case eEaBitf:
        case eEaBitfA:
        case eEaPred:
        case eEaPost:
        case eEaAlt:
        case eEaDAlt:
        case eEaMAlt:
            mode = (rgopn[j].instr >> 3) & 7;
            reg  = (rgopn[j].instr >> 0) & 7;

ea:
            if (mode == 0)
                goto dn;

            else if (mode == 1)
                goto an;

            else if (mode == 2)
                cch += sprintf(buffer+cch, "(A%d)", rgopn[j].reg & 7);

            else if (mode == 3)
                cch += sprintf(buffer+cch, "(A%d)+", rgopn[j].reg & 7);

            else if (mode == 4)
                cch += sprintf(buffer+cch, "-(A%d)", rgopn[j].reg & 7);

            else if (mode == 5)
                {
                cch += sprintf(buffer+cch, "$%04X", rgopn[j].disp & 65535);
                cch += sprintf(buffer+cch, "(A%d)", rgopn[j].reg & 7);
                }
            else if (mode == 6)
                {
                FFEW ffew;

indexed:
                ffew = rgopn[j].ew;

                if (ffew.fFull)
                    {
                    if (mode == 6)
                        {
                        cch += sprintf(buffer+cch, "$%02X(A%c,%c%c.%c)", 
                                  (unsigned char)ffew.d8, 
                                  (reg & 0x07) + '0',
                            (ffew.ireg & 8) ? 'A' : 'D',
                            (ffew.ireg & 0x07) + '0',
                            ffew.WL ? 'l' : 'w');
                        }
                    else
                        {
                        cch += sprintf(buffer+cch, "$%08X(PC,%c%c.%c)", 
                            lPC + rgopn[j].disp,
                            (ffew.ireg & 8) ? 'A' : 'D',
                            (ffew.ireg & 0x07) + '0',
                            ffew.WL ? 'l' : 'w');
                        }
                    }
                else
                    {
                    if (mode == 6)
                        {
                        cch += sprintf(buffer+cch, "$%02X(A%c,%c%c.%c)", 
                                  (unsigned char)ffew.d8, 
                                  (reg & 0x07) + '0',
                            (ffew.ireg & 8) ? 'A' : 'D',
                            (ffew.ireg & 0x07) + '0',
                            ffew.WL ? 'l' : 'w');
                        }
                    else
                        {
                        cch += sprintf(buffer+cch, "$%08X(PC,%c%c.%c)", 
                            lPC + rgopn[j].disp,
                            (ffew.ireg & 8) ? 'A' : 'D',
                            (ffew.ireg & 0x07) + '0',
                            ffew.WL ? 'l' : 'w');
                        }
                    }
                }
            else if (reg == 0)
                {
                cch += sprintf(buffer+cch, "$%04X.w", (WORD)rgopn[j].addr);
                }
            else if (reg == 1)
                {
                cch += sprintf(buffer+cch, "$%08X", rgopn[j].addr);
                }
            else if (reg == 2)
                {
                cch += sprintf(buffer+cch, "$%06X(PC)",
                     lPC + rgopn[j].disp);
                }
            else if (reg == 4)
                {
                if (size == SIZE_B)
                    {
                    cch += sprintf(buffer+cch, "#$%02X", rgopn[j].l);
                    }
                else if (size == SIZE_W)
                    {
                    cch += sprintf(buffer+cch, "#$%04X", rgopn[j].l);
                    }
                else if (size == SIZE_L)
                    {
                    cch += sprintf(buffer+cch, "#$%08X", rgopn[j].l);
                    }
                else
                    goto Lerror;
                }
            else
                goto indexed;
            break;
            }
        }

    buffer[cch] = 0;

    return cb;

Lerror:

    return 0;
}

#endif // ATARIST

