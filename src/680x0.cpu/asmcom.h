
/***************************************************************************

    ASMCOM.H

    - Common include file for all CPU disassemblers

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release

***************************************************************************/

#ifndef ASMCOM_H
#define ASMCOM_H

#pragma once

//
// Instruction Definition
//
// Defines a PowerPC machine language instruction and its format
//

typedef struct
{
    ULONG   rgl[];
} LIST, *PLIST;

typedef struct
{
    BYTE    szMne[8];   // mnemonic name of the instruction
    ULONG   lCode;      // bits that specify the instruction
    ULONG   lMask;      // bit mask used to test for lCode
    ULONG   wfCPU;      // bit mask containing CPU related flags
    ULONG   wfDis;      // bit mask with disassembly related flags
    PLIST   pltUop;     // microcode list
    PLIST   pltRes;     // reserved
    PLIST   pltRes2;    // reserved
    PLIST   pltRes3;    // reserved
    PLIST   pltRes4;    // reserved
} IDEF;

ULONG const vcNumInstrs;

//
// CPU flags
//
// Used by the interpreter and jitter to determine whether an instruction
// is valid in the current CPU emulation mode.
//
// May optionally be used by the disassembler.
//

enum
{
    sfCpu68000 = 0,     // 68K processors
    sfCpu68010,
    sfCpu68020,
    sfCpu68030,
    sfCpu68040,

    sfCpuPOWER,         // PPC processors
    sfCpu601,
    sfCpu603,
    sfCpu604,
    sfCpu750,
    sfCpuG4,

    sfFpu68881,         // 68K FPUs
    sfFpu68882,
    sfFpu68040,

    sfFpu601,           // PPC FPUs

    sf64,               // machine modes that support the instruction
    sfSupervisor,
    sfPseudoOp,

    sfExt19,            // PPC extended instructions
    sfExt31,
    sfExt59,
    sfExt63,

    // these fields could be disassembly fields, but technically they
    // are used to decode different instructions such as 'ori' and 'ori.'
    // so they are here in the CPU flags.
    // The disassembler checks these to add the proper suffix to the
    // instruction name (such as l a o or '.')
    // The execution engine checks these at runtime.
    // We do this instead of having unique entries in the IDEF table
    // since some instructions (such as ori and ori.) have idential
    // bits in the upper 16-bits and therefore a runtime check is required
    // to distinguish between the two instructions. The jitter can of course
    // optimize the checks away if the bits in the instruction are not set.

    sfChkAA,            // AA bit, bit 30
    sfChkLK,            // LK bit, bit 31
    sfChkOE,            // OE bit, bit 21
    sfChkRc,            // Rc bit, bit 31

    // this flag isn't needed during disassembly or instruction decode
    // but is helpful at runtime to tell the execution engine that
    // CR0 needs to be updated. This is consistent with the flags above

    sfSetRc,            // Rc update is implied (e.g. andi.)

    // this flag also isn't needed during disassembly or instruction decode
    // but is helpful at runtime to tell the execution engine that
    // XER[CA] needs to be updated. This is consistent with the flags above

    sfSetCA,            // CA update is implied (e.g. addic)

    // this flag tells the execution engine to NOT decode the various fields
    // using the disassembly flags. Otherwise they are automatically decoded

    sfNoDecode,

    sfCpuLast
};

#if (sfCpuLast > 32)
#error
#endif

#define wfCpu68000      (1 << sfCpu68000)
#define wfCpu68010      (1 << sfCpu68010)
#define wfCpu68020      (1 << sfCpu68020)
#define wfCpu68030      (1 << sfCpu68030)
#define wfCpu68040      (1 << sfCpu68040)

#define wfCpuPOWER      (1 << sfCpuPOWER)
#define wfCpu601        (1 << sfCpu601)
#define wfCpu603        (1 << sfCpu603)
#define wfCpu604        (1 << sfCpu604)
#define wfCpu750        (1 << sfCpu750)
#define wfCpuG4         (1 << sfCpuG4)

#define wfFpu68881      (1 << sfFpu68881)
#define wfFpu68882      (1 << sfFpu68882)
#define wfFpu68040      (1 << sfFpu68040)
#define wfFpu601        (1 << sfFpu601)

#define wf32BitOnly     (1 << sf32BitOnly)
#define wf64BitOnly     (1 << sf64BitOnly)
#define wfUserOnly      (1 << sfUserOnly)
#define wfSuperOnly     (1 << sfSuperOnly)
#define wfPseudoOp      (1 << sfPseudoOp)

#define wfExt19         (1 << sfExt19)
#define wfExt31         (1 << sfExt31)
#define wfExt59         (1 << sfExt59)
#define wfExt63         (1 << sfExt63)

#define wfChkAA         (1 << sfChkAA)
#define wfChkLK         (1 << sfChkLK)
#define wfChkOE         (1 << sfChkOE)
#define wfChkRc         (1 << sfChkRc)
#define wfSetRc         (1 << sfSetRc)
#define wfSetCA         (1 << sfSetCA)
#define wfNoDecode      (1 << sfNoDecode)


#define wfCpuAll68K \
    (wfCpu68000 | wfCpu68010 | wfCpu68020 | wfCpu68030 | wfCpu68040)

#define wfCpu68K24 \
    (wfCpu68000 | wfCpu68010)

#define wfCpu68K32 \
    (wfCpu68020 | wfCpu68030 | wfCpu68040)

#define wfCpu68KVM \
    (wfCpu68010 | wfCpu68020 | wfCpu68030 | wfCpu68040)

#define wfFpuAll68K \
    (wfFpu68881 | wfFpu68882 | wfFpu68040)

#define wfCpuAllPPC \
    (wfCpuPOWER | wfCpu601 | wfCpu603 | wfCpu604 | wfCpu750 | wfCpuG4)

#define wfFpuAllPPC \
    (wfFpu601)

#endif

