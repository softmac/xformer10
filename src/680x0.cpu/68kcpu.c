
/***************************************************************************

    68KCPU.C

    - C based 68000 interpreter module.
    - Includes initalization code, exception hooks, etc.

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/
  
    10/14/2012  darekm      64K allocations instead of large allocation
    11/30/2008  darekm      Gemulator 9.0 release
    04/20/2008  darekm      last update

***************************************************************************/

#include "..\gemtypes.h"
#include "math.h"

#if defined(ATARIST) || defined(SOFTMAC)

#ifndef NDEBUG

#define  TRACETRAP  1
#define  TRACEEXCPT 1
#define  TRACEFPU   1
#define  TRACEMMU   1
#define  TRACE040   1
#define  TRACEPPC   1
#define  TRACEMEM   1

#ifndef NDEBUG
BOOL fLogHWAccess;
#endif

#else  // NDEBUG

#define  TRACETRAP  0
#define  TRACEEXCPT 0
#define  TRACEFPU   0
#define  TRACEMMU   0
#define  TRACEMMULU 0
#define  TRACE040   0
#define  TRACEPPC   0
#define  TRACEMEM   0

#endif // NDEBUG


#if TRACEFPU
ULONG iFPU;
#endif

#if defined(PERFCOUNT)
unsigned cntInstr,  cntCodeMiss;
unsigned cntAccess, cntDataMiss;
#endif

// #pragma optimize("",off)

#if !defined(NDEBUG) || TRACEEXCPT
#pragma optimize("",off)
#endif

int __stdcall DllMain
(
    HINSTANCE h,
    DWORD dw,
    LPVOID lp
)
{
    h; dw; lp;

    return 1;
}


void * __cdecl GetPregs()
{
    return vi.pregs;
}


// return TRUE is number is non-zero and has only a single bit set

BOOL FIsPowerOfTwo(ULONG x)
{
    return x && ((x & (x-1)) == 0);
}

// return TRUE is number is at least 64K and has only a single bit set

BOOL FIsSmallPowerOfTwo(ULONG x)
{
 printf("x is small? %d\n", (x >= (64 * 1024)));
 printf("x is power of two? %d\n", ((x & (x-1)) == 0));
    return (x >= (64 * 1024)) && ((x & (x-1)) == 0);
}

// return TRUE is number is at least 4MB and has only a single bit set

BOOL FIsLargePowerOfTwo(ULONG x)
{
    return (x >= (4 * 1024 * 1024)) && ((x & (x-1)) == 0);
}


BOOL FAllocateGuestCb(ULONG ea, unsigned cb)
{
    ULONG wfAlloc;
    ULONG cbAlloc;
    ULONG index;
    BYTE  *pb = NULL;

    if (cb == 0)
        return TRUE;

#if !defined(NDEBUG)
    printf("allocating: ea=%08X, cb=%08X\n", ea, cb);
#endif

    // make sure ea is multiple of 64K, cb is power-of-2 multiple of 64K
    Assert((ea & 0xFFFF) == 0);
    Assert(FIsSmallPowerOfTwo(cb));

    index = ea >> 16;

    Assert(vi.plAlloc[index] == 0);

#if 0
#if !defined(NDEBUG)
    printf("Large page minimum = %08X\n", GetLargePageMinimum());
#endif
#endif

    // try a large page allocation first

    if (FIsLargePowerOfTwo(cb))
        {
        wfAlloc = MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES;
        pb = (BYTE *)VirtualAlloc(NULL, cb, wfAlloc, PAGE_READWRITE);
        }

     // then try 4K page allocation if still needed

     if (pb == NULL)
        {
        wfAlloc = MEM_RESERVE | MEM_COMMIT;
        pb = (BYTE *)VirtualAlloc(NULL, cb, wfAlloc, PAGE_READWRITE);
        }

     // on success fill in the mapping table in 64K increments

     if (pb)
        {
#if !defined(NDEBUG)
        printf("alloc: host:%08p ea:%08X cb:%08X large=%d\n",
            pb, ea, cb, !!(wfAlloc & MEM_LARGE_PAGES));
#endif

        do
            {
            vi.plAlloc[index++] = (ULONG)(ULONG_PTR)(void *)pb;
            pb += 65536;
            cb -= 65536;
            } while (cb);
        }

    return pb != NULL;
}


//
// memInitAddressSpace allocate the host address map table
//
// This a table of DWORDs covering 4GB guest address range mapping each
// 64K block of guest physical address range to host address range.
//
// Entries are each a 32-bit DWORD, on 32-bit builds giving the true
// 32-bit host address of that block, and on 64-bit builds giving bits
// 47..16 of the host address.  Host blocks are assumed to be allocated
// at the 64K alignment of VirtualAlloc granularity.
//
// An entry of zero (which maps to host address 0) means that 64K block
// is not mapped to either RAM or ROM and is invalid to access on the guest.
//

BOOL memInitAddressSpace()
{
    Assert(vi.plAlloc  == NULL);
    Assert(vi.pbRAM[0] == NULL);
    Assert(vi.pbRAM[1] == NULL);
    Assert(vi.pbRAM[2] == NULL);
    Assert(vi.pbRAM[3] == NULL);
    Assert(vi.pbROM[0] == NULL);
    Assert(vi.pbROM[1] == NULL);
    Assert(vi.pbROM[2] == NULL);
    Assert(vi.pbROM[3] == NULL);

    Assert(vi.cbAlloc  == 0);
    Assert(vi.cbRAM[0] == 0);
    Assert(vi.cbRAM[1] == 0);
    Assert(vi.cbRAM[2] == 0);
    Assert(vi.cbRAM[3] == 0);
    Assert(vi.cbROM[0] == 0);
    Assert(vi.cbROM[1] == 0);
    Assert(vi.cbROM[2] == 0);
    Assert(vi.cbROM[3] == 0);

    // 4GB / 64K = 65536

    vi.plAlloc = (DWORD *) VirtualAlloc(NULL, 65536 * sizeof(DWORD),
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    // all entries are by default unallocated and mapped to 0 (i.e. invalid)

    return (vi.plAlloc != NULL);
}


BOOL memAllocAddressSpace(BOOL fBigEnd)
{
    int i;
    BOOL success = TRUE;

    for (i = 0; i < 4; i++)
        {
        success = success && FAllocateGuestCb(vi.eaRAM[i], vi.cbRAM[i]);
        success = success && FAllocateGuestCb(vi.eaROM[i], vi.cbROM[i]);

        vi.pbRAM[i] = (BYTE *)vi.plAlloc[vi.eaRAM[i] >> 16];
        vi.pbROM[i] = (BYTE *)vi.plAlloc[vi.eaROM[i] >> 16];

        if (fBigEnd)
            {
            vi.pbRAM[i] += vi.cbRAM[i] - 1;
            vi.pbROM[i] += vi.cbROM[i] - 1;
            }
        }

    if (!success)
        {
        goto Lfail;
        }

    // Tell the video thread it's ok

    vi.fVideoEnabled = TRUE;

    return TRUE;

Lfail:
    memFreeAddressSpace();
    return FALSE;
}


BOOL memFreeAddressSpace()
{
    int i;

    if (vpregs == NULL)
        return TRUE;

    // Interlock to make sure video thread is not touching memory

    while (0 != InterlockedCompareExchange(&vi.fVideoEnabled, FALSE, TRUE))
        Sleep(10);

    vi.fVideoEnabled = FALSE;

    if (vi.pbRAM[0])
        MarkAllPagesClean();

    // Free the hell out of everything!

    InvalidatePages(0x00000000, 0xFFFFFFFF);

    if (vi.pProfile)
        {
#if 0
printf("dumping profile\n");
        {
        ULONG i;

        for (i = 0 ; i < 65536; i++)
            {
            if (vi.pProfile[i] > 100000)
                printf("prof: %09X %04X\n",
                    vi.pProfile[i], i);
            }
        }
#endif

        free(vi.pProfile);

        vi.pProfile = NULL;
        }

    if (vi.pHistory)
        {
        free(vi.pHistory);

        vi.pHistory = NULL;
        }

    // use the sledgehammer approach

    for (i = 0; i < 4; i++)
        {
        unsigned index1 = vi.eaRAM[i] >> 16;
        unsigned index2 = vi.eaROM[i] >> 16;

        if (vi.plAlloc[index1])
            VirtualFree((void *)vi.plAlloc[index1], 0, MEM_RELEASE);

        if (vi.plAlloc[index2])
            VirtualFree((void *)vi.plAlloc[index2], 0, MEM_RELEASE);
        }

//  VirtualFree(vi.plAlloc, 0, MEM_RELEASE);

    memset(vi.plAlloc, 0, 65536 * sizeof(DWORD));

    memset(vi.cbRAM, 0, sizeof(vi.cbRAM));
    memset(vi.pbRAM, 0, sizeof(vi.pbRAM));
    memset(vi.cbROM, 0, sizeof(vi.cbROM));
    memset(vi.pbROM, 0, sizeof(vi.pbROM));

    return TRUE;
}

static BOOL __inline FIsHostROM(BYTE *pb)
{
    int i;

    for (i = 0; i < 4; i++)
        {
        if (vi.cbROM[i] == 0)
            continue;

        if ((pb >= (vi.pbROM[i] - vi.cbROM[i] + 1)) && (pb <= vi.pbROM[i]))
            return TRUE;
        }

    return FALSE;
}


//
// call to MapAddressVM will not cause any exceptions
//

static BYTE *memPvGetHostAddress(ULONG addr, ULONG cb)
{
    BYTE *pb0, *pb;

#if 0
    if (vpregs == NULL)
        return NULL;

    if (vi.pbRAM[0] == NULL)
        return NULL;
#endif

    pb0 = MapAddressVM(addr);

    if (pb0 == NULL)
        {
#if !defined(NDEBUG)
        printf("memPv fail, addr = %08X cb = %08X pb0 = %08p\n",
            addr, cb, pb0);
#endif
        return NULL;
        }

    pb  = MapAddressVM(addr + cb - 1);

    if (pb == NULL)
        {
#if !defined(NDEBUG)
        printf("memPv fail, addr = %08X cb = %08X pb1 = %08p\n",
            addr, cb, pb);
#endif
        return NULL;
        }

    // make sure the starting and ending points are contiguous
    // in case we're spanning from RAM to ROM or what have you

    if (pb - pb0 + cb - 1)
        {
#if !defined(NDEBUG)
        printf("memPv fatal error!\n");

        __asm int 3;
#endif
        }

    return (BYTE *)pb0 - (cb - 1);
}

static BOOL FTranslateGuestAddress(ULONG *paddr, BOOL fWrite)
{
    ULONG addr = *paddr;
    unsigned index;

    // 68000 68010 68020 are purely physical addressing

    if (vmCur.bfCPU < cpu68030)
        return TRUE;

    // Check if 68030 translation enabled

    if ((vmCur.bfCPU == cpu68030) && (vpregs2->TCR & 0x80000000))
        {
        ULONG bSRE, bFCL, bPS, bIS, TI[4];
        ULONG DT;       // descriptor type
        ULONG TA;       // table address
        ULONG desc;     // descriptor first 4 bytes
        ULONG descH;    // optional second 4 bytes of descriptor
        ULONG shift;
        ULONG field;
        ULONG cbPage;

#if TRACEMMULU
        printf("***\n");
        printf("Translating 68030 address %08X\n", addr);
#endif

        bSRE = (vpregs2->TCR & 0x02000000) >> 25;
        bFCL = (vpregs2->TCR & 0x01000000) >> 24;
        bPS  = (vpregs2->TCR & 0x00F00000) >> 20;
        bIS  = (vpregs2->TCR & 0x000F0000) >> 16;
        TI[0] = (vpregs2->TCR & 0x0000F000) >> 12;      // TIA
        TI[1] = (vpregs2->TCR & 0x00000F00) >> 8;       // TIB
        TI[2] = (vpregs2->TCR & 0x000000F0) >> 4;       // TIC
        TI[3] = (vpregs2->TCR & 0x0000000F) >> 0;       // TID

        cbPage = 1 << bPS;

#if TRACEMMULU
        printf("TCR = %08X  ", vpregs2->TCR);
        printf("E:%d SRE:%d FCL:%d cbPage:%d IS:%d "
               "TIA:%d TIB:%d TIC:%d TID:%d\n",
                1, bSRE, bFCL, cbPage, bIS, TI[0], TI[1], TI[2], TI[3]);
#endif

        if (vpregs2->TT[0] & 0x00008000)
            {
            if ((addr & ((vpregs2->TT[0] << 8) & 0xFF000000)) ==
                (vpregs2->TT[0] & 0xFF000000))
                {
#if TRACEMMULU
                printf("translate TT0 = %08X\n", vpregs2->TT[0]);
#endif
                goto skip;
                }
            }

        if (vpregs2->TT[1] & 0x00008000)
            {
            if ((addr & ((vpregs2->TT[1] << 8) & 0xFF000000)) ==
                (vpregs2->TT[1] & 0xFF000000))
                {
#if TRACEMMULU
                printf("translate TT1 = %08X\n", vpregs2->TT[1]);
#endif
                goto skip;
                }
            }

        // Check if in supervisor mode and TCR.SRE enabled

        if (vpregs->rgfsr.bitSuper && bSRE)
            {
#if TRACEMMULU
            printf("using SRP: %08X %08X\n",
                vpregs2->SRP30[0], vpregs2->SRP30[1]);
#endif
            DT = vpregs2->SRP30[0] & 3;
            TA = vpregs2->SRP30[1] & 0xFFFFFFF0;
            }
        else
            {
#if TRACEMMULU
            printf("using CRP: %08X %08X\n",
                vpregs2->CRP30[0], vpregs2->CRP30[1]);
#endif
            DT = vpregs2->CRP30[0] & 3;
            TA = vpregs2->CRP30[1] & 0xFFFFFFF0;
            }


#if 0
#if TRACEMMULU
        if ((vpregs2->CRP30[0] & 3) == 2)
            {
            int i, iMax;

            iMax = 1 << ((vpregs2->TCR & 0x0000F000) >> 12);    // TIA

            for (i = 0; i < iMax; i++)
                {
                printf("root desc %3d = %08X\n", i,
                    vmPeekL((vpregs2->CRP30[1] & 0xFFFFFFF0) + i*4));
                }
            }
#endif
#endif

#if TRACEMMULU
        printf("DT = %d   table address = %08X\n", DT, TA);
#endif

        field = 0;          // start with TIA
        shift = bIS;

        if ((addr >= 0x40800000) && (addr < 0x50000000))
            goto skip;

        while (shift < 32)
            {
            desc = descH = 0;

// printf("addr going in is %08X, shift = %d, field = %d\n", addr, shift, field);

            index = (addr << shift);                    // truncate upper bits
            index = index >> (32 - TI[field]);          // extract TIx'th field

#if TRACEMMULU
            printf("table address = %08X, index = %d\n", TA, index);
#endif

            if (DT == 0)
                {
#if TRACEMMULU
                m68k_DumpRegs();
                m68k_DumpHistory(160);

                printf("TYPE 0 - ILLEGAL DESCRIPTOR\n");
#endif

                // return FALSE to indicate address cannot be mapped!

                return FALSE;
                }
            else if (DT == 1)
                {
#if TRACEMMULU
                printf("TYPE 1 - PAGE DESCRIPTOR\n");
#endif
                TA &= 0xFFFFFF00;

                // Add the descriptor's Page Address with lower bits from addr

                addr = addr << shift;
                addr = addr >> shift;   // REVIEW: signed or unsigned?!?!?

// printf("lowest bits %08X being added to TA\n", addr, TA);

                *paddr = TA + addr;
                break;
                }

            else if (DT == 2)
                {
#if TRACEMMULU
                printf("TYPE 2 - VALID 4-BYTE DESCRIPTOR\n");
#endif
                desc = vmPeekL(TA + index*4);

#if TRACEMMULU
                printf("desc = %08X\n", desc);
#endif

                DT = desc & 3;
                TA = desc & 0xFFFFFFF0;
                }

            else // (DT == 3)
                {
#if TRACEMMULU
                printf("TYPE 3 - VALID 8-BYTE DESCRIPTOR\n");
#endif
                desc = vmPeekL(TA + index*8);
                descH = vmPeekL(TA + index*8 + 4);

#if TRACEMMULU
                printf("desc = %08X %08X\n", desc, descH);
#endif
                }

            // Update shift to include all valid upper bits spanned

            shift += TI[field++];
            }
        }

#if 0   // NYI!!!

    // Check if 68040 translation enabled

    else if ((vmCur.bfCPU == cpu68040) && (vpregs2->TCR & 0x00008000))
        {
        ULONG DT;       // descriptor type
        ULONG TA;       // table address
        ULONG desc;     // descriptor 4 bytes
        ULONG index;
        ULONG cbPage;

#if TRACEMMULU
        printf("***\n");
        printf("Translating 68040 address %08X\n", addr);
#endif

        cbPage = ((vpregs2->TCR & 0x4000) ? 8 : 4) << 10;

#if TRACEMMULU
        printf("TCR = %08X  ", vpregs2->TCR);
        printf("E:%d cbPage:%d\n", 1, cbPage); 
#endif

#if 0
        if (vpregs2->DTT[0] & 0x00008000)
            {
            if ((addr & ((vpregs2->TT[0] << 8) & 0xFF000000)) ==
                (vpregs2->TT[0] & 0xFF000000))
                {
#if TRACEMMULU
                printf("translate TT0 = %08X\n", vpregs2->TT[0]);
#endif
                goto skip;
                }
            }

        if (vpregs2->TT[1] & 0x00008000)
            {
            if ((addr & ((vpregs2->TT[1] << 8) & 0xFF000000)) ==
                (vpregs2->TT[1] & 0xFF000000))
                {
#if TRACEMMULU
                printf("translate TT1 = %08X\n", vpregs2->TT[1]);
#endif
                goto skip;
                }
            }
#endif

        // Check if in supervisor mode enabled

        if (vpregs->rgfsr.bitSuper)
            {
#if TRACEMMULU
            printf("using SRP: %08X\n", vpregs2->SRP40);
#endif
            TA = vpregs2->SRP40 & 0xFFFFFE00;
            }
        else
            {
#if TRACEMMULU
            printf("using URP: %08X\n", vpregs2->URP40);
#endif
            TA = vpregs2->URP40 & 0xFFFFFE00;
            }


        // start with TIA

        index = (addr >> 25) & 127;

#if TRACEMMULU
        printf("first level table address = %08X, index = %d\n", TA, index);
#endif

        desc = vmPeekL(TA + index*4);
        DT = desc & 3;

#if TRACEMMULU
        printf("first level type = %d, desc = %08X\n");
#endif
        }
#endif

    else
        {
        goto skip;
        }

    // Address may or may not have been changed, but did not fail translation.

#if TRACEMMULU
    printf("translated guest physical address = %08X\n", *paddr);
#endif
skip:

    return TRUE;
}

//
// Invalidate a range of GUEST PHYSICAL memory.
//

void __fastcall ZapRange(ULONG addr, ULONG cb)
{
    ULONG ispan;
    extern char fetch_empty;

    // If data span is larger than code span, expand to cover whole data span

    if (cbSpan > cbHashSpan)
        {
        cb = ((addr + cb - 1) | (cbSpan - 1)) + 1 - (addr & dwBaseMask);
        addr = addr & dwBaseMask;
        }

    do
        {
        ispan = iHash(addr);

        if (vpregs->mappedRanges[ispan] == (addr & dwHashMask))
            {
            ULONG i;

            for (i = 0; i < cbHashSpan; i++)
                {
                vpregs->duops[(ispan << bitsHash) + i].pfn = &fetch_empty;
                }
            }

        addr += cbHashSpan;

        } while ((cb >= cbHashSpan) && (cb -= cbHashSpan));
}

BYTE * __fastcall pvGetHashedPC(ULONG addr)
{
    ULONG base = addr & dwHashMask;
    ULONG index = addr & 0x0000FFFF;
    ULONG ispan = iHash(addr);

    extern char fetch_empty;

    // Fast path, it's a match

    if (vpregs->mappedRanges[ispan] == base)
        {
        return (vpregs->duops[index].pfn);
        }

    return &fetch_empty;
}

BYTE * __fastcall pvAddToHashedPCs(ULONG addr, void *pfn)
{
    ULONG base = addr & dwHashMask;
    ULONG index = addr & 0x0000FFFF;
    ULONG ispan = iHash(addr);

    extern BOOL fStepMode;
    extern char fetch_empty;

    // Check for mismatched entry

    if (vpregs->mappedRanges[ispan] == base)
        {
        if (vpregs->duops[index].pfn != &fetch_empty)
        if (vpregs->duops[index].pfn != pfn)
            {
#if 0
#if !defined(NDEBUG)
    printf("pfn mismatch, addr = %08X, pfn = %08X\n", addr, pfn);
            __asm int 3;
#endif
#endif
            // Best to flush all caches

            InvalidatePages(0x00000000, 0xFFFFFFFF);
            return (BYTE *)pfn;
            }
        }

#if 0
    // Uncomment for safe return from here, but does not updated code caches
    return (BYTE *)pfn;
#endif

#if 0
    printf("Miss at %08X, pfn = %08X, hash = %08X, index = %04X\n",
         addr, pfn, vpregs->duops[index].pfn, index);
#endif

    {
    ULONG i, lo, hi;
    ULONG ispanlo = iHash(addr - 16);
    ULONG ispanhi = iHash(addr + 16);

#if 0
 printf("ispanlo = %04X ispan = %04X ispanhi = %04X\n",
         ispanlo, ispan, ispanhi);
#endif

    if (vpregs->mappedRanges[ispan] != base)
        {
#if 0
 printf("span base mismatch base = %08X, mapped = %08X\n",
            base, vpregs->mappedRanges[ispan]);
#endif

        vpregs->mappedRanges[ispan]   = base;

        lo = (ispan << bitsHash);
        hi = (ispan << bitsHash) + cbHashSpan;

        for (i = lo; i < hi; i++)
            {
            vpregs->duops[i].pfn = &fetch_empty;
            vpregs->duops[i].guestPC = ~addr;
            }

        ispanlo = iHash(addr - cbHashSpan);
        ispanhi = iHash(addr + cbHashSpan);
        }

    if (ispanlo != ispan)
    if (vpregs->mappedRanges[ispanlo] != (dwHashMask & (base - cbHashSpan)))
        {
#if 0
 printf("spanlo being zapped\n");
#endif

        vpregs->mappedRanges[ispanlo] = (base - cbHashSpan) & dwHashMask;

        lo = (ispanlo << bitsHash);
        hi = (ispanlo << bitsHash) + cbHashSpan;

        for (i = lo; i < hi; i++)
            {
            vpregs->duops[i].pfn = &fetch_empty;
            vpregs->duops[i].guestPC = ~addr;
            }
        }

    if (ispanhi != ispan)
    if (vpregs->mappedRanges[ispanhi] != (dwHashMask & (base + cbHashSpan)))
        {
#if 0
 printf("spanhi being zapped\n");
#endif
        vpregs->mappedRanges[ispanhi] = (base + cbHashSpan) & dwHashMask;

        lo = (ispanhi << bitsHash);
        hi = (ispanhi << bitsHash) + cbHashSpan;

        for (i = lo; i < hi; i++)
            {
            vpregs->duops[i].pfn = &fetch_empty;
            vpregs->duops[i].guestPC = ~addr;
            }
        }

//  if (!fStepMode)
        {
        vpregs->duops[index].pfn = pfn;
        vpregs->duops[index].guestPC = addr;
        }

#if 0
    printf("hash at index %04X set to %08X\n", index, vpregs->duops[index].pfn);
#endif

    return (BYTE *)pfn;
    }
}


//
// ReadGuestLogical* / WriteGuestLogical* are C/C++ wrappers
// for accessing memory at a guest linear address.
//
// These wrappers return HRESULT and must not cause a host exception or fault.
//

static ULONG iSpanCode;     // hint as to the current code block


__forceinline
void * __fastcall pvTranslationLookupForGuestLogicalFetch(
    ULONG addr,
    unsigned cb)
{
    ULONG ispan = iHash(addr);      // index to TLB entry for this span
    ULONG mask  = dwHashMask | (cb > 1);

    void *p;

#if 0
    p = (void *)((addr ^ vpregs->memtlbPC[iSpanCode+iSpanCode+1]) - (cb - 1));

    if (0 == (mask & (addr ^ (vpregs->memtlbPC[iSpanCode+iSpanCode]))))
        {
        __assume(p != NULL);

        return p;
        }
#endif

    p = (void *)((addr ^ vpregs->memtlbPC[ispan+ispan+1]) - (cb - 1));

#if 1
    if (0 == (mask & (addr ^ (vpregs->memtlbPC[ispan+ispan]))))
        {
        __assume(p != NULL);

        return p;
        }
#endif

#if 0
    printf("lookup returning failure, "
           "addr = %08X  cb = %d "
           "index = %03X, addr = %08X, mask = %08X, entry = %08X %08X\n",
            addr, cb, 
            ispan, addr, mask, vpregs->memtlbPC[ispan+ispan],
            vpregs->memtlbRO[ispan+ispan+1]);
#endif

    return NULL;
}

__forceinline
void * __fastcall pvTranslationLookupForGuestLogicalRead(
    ULONG addr,
    unsigned cb)
{
    ULONG ispan = iSpan(addr + ((cb > 2) ? (cb - 2) : 0));
    ULONG mask  = dwBaseMask | (cb > 1);

    void *p = (void *)((addr ^ vpregs->memtlbRO[ispan+ispan+1]) - (cb - 1));

    if (0 == (mask & (addr ^ (vpregs->memtlbRO[ispan+ispan]))))
        {
        __assume(p != NULL);

        return p;
        }

#if 0
    printf("lookup returning failure, "
           "addr = %08X  cb = %d "
           "index = %03X, addr = %08X, mask = %08X, entry = %08X %08X\n",
            addr, cb, 
            ispan, addr, mask, vpregs->memtlbRO[ispan+ispan],
            vpregs->memtlbRO[ispan+ispan+1]);
#endif

    return NULL;
}


__forceinline
void * __fastcall pvTranslationLookupForGuestLogicalWrite(
    ULONG addr,
    unsigned cb)
{
    ULONG ispan = iSpan(addr + ((cb > 2) ? (cb - 2) : 0));
    ULONG mask  = dwBaseMask | (cb > 1);

    void *p = (void *)((addr ^ vpregs->memtlbRW[ispan+ispan+1]) - (cb - 1));

    if (0 == (mask & (addr ^ (vpregs->memtlbRW[ispan+ispan]))))
        {
        __assume(p != NULL);

        return p;
        }

#if 0
    printf("lookup returning failure, "
           "addr = %08X  cb = %d "
           "index = %03X, addr = %08X, mask = %08X, entry = %08X %08X\n",
            addr, cb, 
            ispan, addr, mask, vpregs->memtlbRW[ispan+ispan],
            vpregs->memtlbRW[ispan+ispan+1]);
#endif

    return NULL;
}


void __fastcall SetTranslationForGuestLogicalFetch(
    ULONG addr,
    unsigned flags,
    void * pv,
    unsigned cb)
{
    ULONG ispan = iHash(addr);      // index to TLB entry for this span

    vpregs->memtlbPC[ispan+ispan] = dwHashMask & addr;
    vpregs->memtlbPC[ispan+ispan+1] = (addr ^ ((cb - 1) + (ULONG_PTR)pv));
    iSpanCode = ispan;

#if 0
    printf("setting translation block index %03X, addr = %08X, xor = %08X\n",
        ispan, addr, vpregs->memtlbPC[ispan+ispan+1]);
#endif

#if 0
    if (pvTranslationLookupForGuestLogicalWrite(addr, 1))
        {
        ULONG ispan = iSpan(addr);      // index to TLB entry for this span
        vpregs->memtlbRW[ispan+ispan] = ~addr;
        }
#endif
}

void __fastcall SetTranslationForGuestLogicalRead(
    ULONG addr,
    unsigned flags,
    void * pv,
    unsigned cb)
{
    ULONG ispan = iSpan(addr);      // index to TLB entry for this span

    vpregs->memtlbRO[ispan+ispan] = dwBaseMask & addr;
    vpregs->memtlbRO[ispan+ispan+1] = (addr ^ ((cb - 1) + (ULONG_PTR)pv));

#if 0
    printf("setting translation block index %03X, addr = %08X, xor = %08X\n",
        ispan, addr, vpregs->memtlbRO[ispan+ispan+1]);
#endif
}

void __fastcall SetTranslationForGuestLogicalWrite(
    ULONG addr,
    unsigned flags,
    void * pv,
    unsigned cb)
{
    ULONG ispan = iSpan(addr);      // index to TLB entry for this span

    vpregs->memtlbRW[ispan+ispan] = dwBaseMask & addr;
    vpregs->memtlbRW[ispan+ispan+1] = (addr ^ ((cb - 1) + (ULONG_PTR)pv));

#if 0
    printf("setting translation block index %03X, addr = %08X, xor = %08X\n",
        ispan, addr, vpregs->memtlbRW[ispan+ispan+1]);
#endif
}

//
// Returns a non-NULL pointer to host memory for the given guest linear
// address range. A NULL return value indicates that the address is either
// invalid or I/O space, and needs to be accessed using VM accessors.
//


static __forceinline
void * __fastcall pvHostReadableGuestLinearAddress(ULONG addr, ULONG cb)
{
    BYTE *pb;

    // If 68030/68040 page tables are enabled, map the guest address from
    // logical to physical address. Easiest check is to see if the Paging
    // Enable bit is enabled in the TCR (bit 31 on 68030, bit 15 on 68040).

    if (vpregs2->TCR & 0x80008000)
        {
        if (!FTranslateGuestAddress(&addr, FALSE))
            return NULL;
        }

    pb = MapAddressVM(addr);

    if (pb)
        {
        pb -= (cb - 1);
        }

    return (void *)pb;
}


static __forceinline
void * __fastcall pvHostWritableGuestLinearAddress(ULONG addr, ULONG cb)
{
    BYTE *pb;

    // If 68030/68040 page tables are enabled, map the guest address from
    // logical to physical address. Easiest check is to see if the Paging
    // Enable bit is enabled in the TCR (bit 31 on 68030, bit 15 on 68040).

    if (vpregs2->TCR & 0x80008000)
        {
        if (!FTranslateGuestAddress(&addr, TRUE))
            return NULL;
        }

    pb = MapWritableAddressVM(addr);

    // If overwriting a cached code dispatch range, invalidate it

    if (pb)
        {
        pb -= (cb - 1);

        ZapRange(addr, cb);
        }

    return (void *)pb;
}


#undef  TRACEMEM
#define TRACEMEM    0

extern long lAccessAddr;

HRESULT __fastcall ReadGuestLogicalByte(ULONG addr, BYTE *pbData)
{
    BYTE *pb = pvTranslationLookupForGuestLogicalRead(addr, sizeof(BYTE));
    ADDR phys_addr;

    if (TRACEMEM)
        printf("cRMB: addr=%08X, pb = %08p\n", addr, pb);

    if (pb)
        {
        if (TRACEMEM)
            printf("cRMB: addr=%08X, hit! Returning %02X\n", addr, *pb);

        *pbData = *pb;

        return NO_ERROR;
        }

    // Try to snoop the write TLB before doing slower loopup

    pb = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(BYTE));

    if (NULL == pb)
        {
        pb = (BYTE *)pvHostReadableGuestLinearAddress(addr, sizeof(BYTE));
        }

    if (pb)
        {
        SetTranslationForGuestLogicalRead(addr, 0, pb, sizeof(BYTE));

#if TRACEMEM
        printf("cRMB: addr=%08X, new! Returning %02X\n", addr, *pb);
#endif
        *pbData = *pb;

        return NO_ERROR;
        }

    phys_addr = addr;

    if (FTranslateGuestAddress(&phys_addr, FALSE))
        {
        return ReadPhysicalByte(phys_addr, pbData);
        }

    return ERROR_INVALID_ACCESS;
}

HRESULT __fastcall ReadGuestLogicalWord(ULONG addr, WORD *pwData)
{
    WORD *pw = pvTranslationLookupForGuestLogicalRead(addr, sizeof(WORD));
    ADDR phys_addr;

    if (TRACEMEM)
        printf("cRMW: addr=%08X, pw = %08p\n", addr, pw);

    if (pw)
        {
        if (TRACEMEM)
            printf("cRMW: addr=%08X, hit! Returning %04X\n", addr, *pw);

        *pwData = *pw;

        return NO_ERROR;
        }

    // Try to snoop the write TLB before doing slower loopup

    pw = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(WORD));

    if (NULL == pw)
        {
        pw = (WORD *)pvHostReadableGuestLinearAddress(addr, sizeof(WORD));
        }

    if (pw)
        {
        SetTranslationForGuestLogicalRead(addr, 0, pw, sizeof(WORD));

#if TRACEMEM
        printf("cRMW: addr=%08X, new! Returning %04X\n", addr, *pw);
#endif
        *pwData = *pw;

        return NO_ERROR;
        }

    phys_addr = addr;

    if (FTranslateGuestAddress(&phys_addr, FALSE))
        {
        return ReadPhysicalWord(phys_addr, pwData);
        }

#if TRACEMEM
    printf("cRMW: addr=%08X, failed to map!\n", addr);
#endif

    return ERROR_INVALID_ACCESS;
}

HRESULT __fastcall ReadGuestLogicalLong(ULONG addr, DWORD *plData)
{
    DWORD *pl = pvTranslationLookupForGuestLogicalRead(addr, sizeof(DWORD));
    ADDR phys_addr;
    WORD wHi, wLo;

    if (TRACEMEM)
        printf("cRML: addr=%08X\n", addr);

    if (pl)
        {
        if (TRACEMEM)
            printf("cRML: addr=%08X, hit! Returning %08X\n", addr, *pl);

        *plData = *pl;

        return NO_ERROR;
        }

    // See if this is spanning block boundary

    if ((cbSpan - 2) == (addr & (cbSpan - 1)))
    {
    if (NO_ERROR == ReadGuestLogicalWord(addr + 0, &wHi))
        if (NO_ERROR == ReadGuestLogicalWord(addr + 2, &wLo))
            {
            *plData = ((DWORD)wHi << 16) | wLo;

            return NO_ERROR;
            }
    }

    // Try to snoop the write TLB before doing slower loopup

    pl = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(DWORD));

    if (NULL == pl)
        {
        pl = (DWORD *)pvHostReadableGuestLinearAddress(addr, sizeof(DWORD));
        }

    if (pl)
        {
        SetTranslationForGuestLogicalRead(addr, 0, pl, sizeof(DWORD));

        *plData = *pl;

        return NO_ERROR;
        }

    phys_addr = addr;

    if (FTranslateGuestAddress(&phys_addr, FALSE))
        {
        return ReadPhysicalLong(phys_addr, plData);
        }

#if TRACEMEM
    printf("cRML: addr=%08X, failed to map!\n", addr);
#endif

    return ERROR_INVALID_ACCESS;
}

HRESULT __fastcall WriteGuestLogicalByte(ULONG addr, BYTE *pbData)
{
    BYTE *pb;
    ADDR phys_addr;

    if (TRACEMEM)
        printf("cWMB: addr=%08X %02X\n", addr, *pbData);

    pb = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(BYTE));

    if (pb)
        {
        if (TRACEMEM)
            printf("cWMB: addr=%08X, hit! Returning %08p\n", addr, pb);

        *pb = *pbData;

        return NO_ERROR;
        }

    pb = (BYTE *)pvHostWritableGuestLinearAddress(addr, sizeof(BYTE));

    if (pb)
        {
        SetTranslationForGuestLogicalWrite(addr, 0, pb, sizeof(BYTE));

#if TRACEMEM
        printf("cWMB: addr=%08X, new! Returning %08p\n", addr, pb);
#endif
        *pb = *pbData;

        return NO_ERROR;
        }

    phys_addr = addr;

    if (FTranslateGuestAddress(&phys_addr, TRUE))
        {
        return WritePhysicalByte(phys_addr, pbData);
        }

    return ERROR_INVALID_ACCESS;
}

HRESULT __fastcall WriteGuestLogicalWord(ULONG addr, WORD *pwData)
{
    WORD *pw;
    ADDR phys_addr = addr;

    if (TRACEMEM)
        printf("cWMW: addr=%08X %04X\n", addr, *pwData);

    pw = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(WORD));

    if (pw)
        {
        if (TRACEMEM)
            printf("cWMW: addr=%08X, hit! Returning %08p\n", addr, pw);

        *pw = *pwData;

        return NO_ERROR;
        }

    pw = (WORD *)pvHostWritableGuestLinearAddress(addr, sizeof(WORD));

    if (pw)
        {
        SetTranslationForGuestLogicalWrite(addr, 0, pw, sizeof(WORD));

#if TRACEMEM
        printf("cWMW: addr=%08X, new! Returning %08p\n", addr, pw);
#endif
        *pw = *pwData;

        return NO_ERROR;
        }

    phys_addr = addr;

    if (FTranslateGuestAddress(&phys_addr, TRUE))
        {
        return WritePhysicalWord(phys_addr, pwData);
        }

    return ERROR_INVALID_ACCESS;
}

HRESULT __fastcall WriteGuestLogicalLong(ULONG addr, DWORD *plData)
{
    DWORD *pl;
    ADDR phys_addr = addr;

    if (TRACEMEM)
        printf("cWML: addr=%08X %08X\n", addr, *plData);

    pl = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(DWORD));

    if (pl)
        {
        if (TRACEMEM)
            printf("cWML: addr=%08X, hit! Returning %08p\n", addr, pl);

        *pl = *plData;

        return NO_ERROR;
        }

#if 1
    // See if this is spanning block boundary

    if ((cbSpan - 2) == (addr & (cbSpan - 1)))
    {
    WORD wHi = *plData >> 16;

    if (NO_ERROR == WriteGuestLogicalWord(addr + 0, &wHi))
        {
        WORD wLo = *plData & 0xFFFF;

        if (NO_ERROR == WriteGuestLogicalWord(addr + 2, &wLo))
            {
            return NO_ERROR;
            }
        }
    }
#endif

    pl = (DWORD *)pvHostWritableGuestLinearAddress(addr, sizeof(DWORD));

    if (pl)
        {
        SetTranslationForGuestLogicalWrite(addr, 0, pl, sizeof(DWORD));

#if TRACEMEM
        printf("cWML: addr=%08X, new! Returning %08p\n", addr, pl);
#endif
        *pl = *plData;

        return NO_ERROR;
        }

    phys_addr = addr;

    if (FTranslateGuestAddress(&phys_addr, TRUE))
        {
        return WritePhysicalLong(phys_addr, plData);
        }

    return ERROR_INVALID_ACCESS;
}



 // Helpers for manipulating ROM space (which normally is not writable)

BOOL memReadByteROM(ULONG addr, BYTE *pb)
{
    BYTE *p = (BYTE *)memPvGetHostAddress(addr, sizeof(BYTE));

    if (p && FIsHostROM((BYTE *)p))
        {
        *pb = *p;

        return TRUE;
        }

    return FALSE;
}

BOOL memReadWordROM(ULONG addr, WORD *pw)
{
    WORD *p = (WORD *)memPvGetHostAddress(addr, sizeof(WORD));

    if (p && FIsHostROM((BYTE *)p))
        {
        *pw = *p;

        return TRUE;
        }

    return FALSE;
}

BOOL memReadLongROM(ULONG addr, LONG *pl)
{
    LONG *p = (LONG *)memPvGetHostAddress(addr, sizeof(LONG));

    if (p && FIsHostROM((BYTE *)p))
        {
        *pl = *p;

        return TRUE;
        }

    return FALSE;
}

BOOL memWriteByteROM(ULONG addr, BYTE b)
{
    BYTE *p = memPvGetHostAddress(addr, sizeof(BYTE));

    if (p && FIsHostROM((BYTE *)p))
        {
        *p = b;

        return TRUE;
        }

    return FALSE;
}

BOOL memWriteWordROM(ULONG addr, WORD w)
{
    WORD *p = (WORD *)memPvGetHostAddress(addr, sizeof(WORD));

    if (p && FIsHostROM((BYTE *)p))
        {
        *p = w;

        return TRUE;
        }

    return FALSE;
}


BOOL memWriteLongROM(ULONG addr, LONG l)
{
    LONG *p = (LONG *)memPvGetHostAddress(addr, sizeof(LONG));

    if (p && FIsHostROM((BYTE *)p))
        {
        *p = l;

        return TRUE;
        }

    return FALSE;
}


void __fastcall PrintMiss(ULONG addr)
{
#if !defined(NDEBUG)

    if (addr < vi.cbRAM[0])
        printf("cache miss: %08X\n", addr);

#endif
}

//
// ReadEA/WriteEA
//
// Guest memory read helpers which can be called directly from interpreter ASM
//

__declspec(noreturn) void __cdecl xcptBUSADDR(void);
__declspec(noreturn) void __cdecl xcptBUSREAD(void);
__declspec(noreturn) void __cdecl xcptBUSWRITE(void);

#define FastReadGuestLogicalByte    ReadEA0
#define FastReadGuestLogicalWord    ReadEA1
#define FastReadGuestLogicalLong    ReadEA2
#define FastReadGuestLogicalWordP   ReadEA4
#define FastReadGuestLogicalLongP   ReadEA5

#define FastWriteGuestLogicalByte   WriteEA0
#define FastWriteGuestLogicalWord   WriteEA1
#define FastWriteGuestLogicalLong   WriteEA2
#define FastWriteGuestLogicalWordP  WriteEA4
#define FastWriteGuestLogicalLongP  WriteEA5

#define TRACEMEM    0

//
// Translate the guest PC into a host address
// or return a bus error.
//

void * __fastcall FastTranslatePC(ULONG addr)
{
    WORD *pw = pvTranslationLookupForGuestLogicalFetch(addr, sizeof(WORD));
    void *p;

    if (pw)
        {
        vpregs->PC = addr;

        p = (void *)(&vpregs->duops[addr & 0x0000FFFF]);
        vpregs->regPCLv = p;
        vpregs->regPCSv = p;

//      vpregs->duops[addr & 0x0000FFFF].pfn = pvGetHashedPC(addr);
//      vpregs->duops[addr & 0x0000FFFF].guestPC = addr;
        return p;
        }

    pw = (WORD *)pvHostReadableGuestLinearAddress(addr, sizeof(WORD));

    if (pw)
        {
        SetTranslationForGuestLogicalFetch(addr, 0, pw, sizeof(WORD));

        vpregs->PC = addr;

        p = (void *)(&vpregs->duops[addr & 0x0000FFFF]);
        vpregs->regPCLv = p;
        vpregs->regPCSv = p;

//      vpregs->duops[addr & 0x0000FFFF].pfn = pvGetHashedPC(addr);
//      vpregs->duops[addr & 0x0000FFFF].guestPC = addr;
        return p;
        }

    printf("FastTranslatePC failed on addr=%08X\n", addr);

    lAccessAddr = addr;
    xcptBUSADDR();

    return NULL;
}


BYTE * __fastcall FastTranslateCodeByte(ULONG addr)
{
    BYTE *pb = pvTranslationLookupForGuestLogicalFetch(addr, sizeof(BYTE));

    if (pb)
        {
        return pb;
        }

    pb = (BYTE *)pvHostReadableGuestLinearAddress(addr, sizeof(BYTE));

    if (pb)
        {
        SetTranslationForGuestLogicalFetch(addr, 0, pb, sizeof(BYTE));

        return pb;
        }

    lAccessAddr = addr;
    xcptBUSADDR();

    return NULL;
}


WORD * __fastcall FastTranslateCodeWord(ULONG addr)
{
    WORD *pw = pvTranslationLookupForGuestLogicalFetch(addr, sizeof(WORD));

    if (pw)
        {
        return pw;
        }

    pw = (WORD *)pvHostReadableGuestLinearAddress(addr, sizeof(WORD));

    if (pw)
        {
        SetTranslationForGuestLogicalFetch(addr, 0, pw, sizeof(WORD));

        return pw;
        }

    lAccessAddr = addr;
    xcptBUSADDR();

    return NULL;
}


ULONG * __fastcall FastTranslateCodeLong(ULONG addr)
{
    ULONG *pl = pvTranslationLookupForGuestLogicalFetch(addr, sizeof(ULONG));

    if (pl)
        {
        return pl;
        }

    pl = (ULONG *)pvHostReadableGuestLinearAddress(addr, sizeof(ULONG));

    if (pl)
        {
        SetTranslationForGuestLogicalFetch(addr, 0, pl, sizeof(ULONG));

        return pl;
        }

    lAccessAddr = addr;
    xcptBUSADDR();

    return NULL;
}


ULONG __fastcall FastReadGuestLogicalByte(ULONG addr)
{
    BYTE *pb = pvTranslationLookupForGuestLogicalRead(addr, sizeof(BYTE));

    BYTE b;

#if TRACEMEM
    printf("ReadEA0 addr=%08X\n", addr);
#endif

    if (pb)
        {
#if TRACEMEM
        printf("ReadEA0 addr=%08X, hit! Returning %02X\n", addr, *pb);
#endif
        return *pb;
        }

#if PERFCOUNT
    cntDataMiss++;
#endif

    b = 0;

    if (NO_ERROR != ReadGuestLogicalByte(addr, &b))
        {
        lAccessAddr = addr;
        xcptBUSREAD();
        }

    return b & 255;
}

#undef  TRACEMEM
#define TRACEMEM 0

ULONG __fastcall FastReadGuestLogicalWord(ULONG addr)
{
    USHORT *pw = pvTranslationLookupForGuestLogicalRead(addr, sizeof(WORD));
    USHORT w;

#if TRACEMEM
    printf("ReadEA1 addr=%08X\n", addr);
#endif

    if (pw)
        {
        w = *pw;

#if TRACEMEM
        printf("ReadEA1 addr=%08X, hit! Returning %04X\n", addr, w);
#endif
        return w;
        }

#if PERFCOUNT
    cntDataMiss++;
#endif

    w = 0;

    if (NO_ERROR != ReadGuestLogicalWord(addr, &w))
        {
#if TRACEMEM
        printf("ReadEA1 addr=%08X, FAULTING!\n", addr);
#endif

        lAccessAddr = addr;

        if ((addr & 1) && (vmCur.bfCPU < cpu68020))
            {
            xcptBUSADDR();
            }
        else
            {
            xcptBUSREAD();
            }
        }

    return w & 65535;
}

#undef  TRACEMEM
#define TRACEMEM 0

ULONG __fastcall FastReadGuestLogicalLong(ULONG addr)
{
    ULONG *pl = pvTranslationLookupForGuestLogicalRead(addr, sizeof(ULONG));
    ULONG l;

#if TRACEMEM
    printf("ReadEA2 addr=%08X\n", addr);
#endif

    if (pl)
        {
        l = *pl;

#if TRACEMEM
        printf("ReadEA2 addr=%08X, hit! Returning %08X\n", addr, l);
#endif
        return l;
        }

#if PERFCOUNT
    cntDataMiss++;
#endif

    l = 0;

#if 1
    if (NO_ERROR != ReadGuestLogicalLong(addr, &l))
        {
#if TRACEMEM
        printf("ReadEA2 addr=%08X, FAULTING!\n", addr);
#endif

        lAccessAddr = addr;

        if ((addr & 1) && (vmCur.bfCPU < cpu68020))
            {
            xcptBUSADDR();
            }
        else
            {
            xcptBUSREAD();
            }
        }
#else
    l = (FastReadGuestLogicalWord(addr) << 16) +
         FastReadGuestLogicalWord(addr+2);
#endif

#if TRACEMEM
    printf("ReadEA2 addr=%08X, new! Returning %08X\n", addr, l);
#endif

    return l;
}

ULONG __fastcall FastReadGuestLogicalWordP(ULONG addr)
{
    ULONG w;

#if TRACEMEM
    printf("ReadEA4 addr=%08X\n", addr);
#endif

    w = (FastReadGuestLogicalByte(addr) << 8) +
         FastReadGuestLogicalByte(addr+2);

#if TRACEMEM
    printf("ReadEA4 addr=%08X, new! Returning %04X\n", addr, w);
#endif

    return w;
}

ULONG __fastcall FastReadGuestLogicalLongP(ULONG addr)
{
    ULONG l;

#if TRACEMEM
    printf("ReadEA5 addr=%08X\n", addr);
#endif

    l = (FastReadGuestLogicalByte(addr + 0) << 24) +
        (FastReadGuestLogicalByte(addr + 2) << 16) +
        (FastReadGuestLogicalByte(addr + 4) << 8) +
         FastReadGuestLogicalByte(addr + 6);

#if TRACEMEM
    printf("ReadEA5 addr=%08X, new! Returning %08X\n", addr, l);
#endif

    return l;
}


void __fastcall FastWriteGuestLogicalByte(ULONG addr, BYTE b)
{
    BYTE *pb = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(BYTE));

    if (pb)
        {
        *pb = b;
        return;
        }

#if PERFCOUNT
    cntDataMiss++;
#endif

    {
    BYTE bData = b;

    if (NO_ERROR != WriteGuestLogicalByte(addr, &bData))
        {
        lAccessAddr = addr;
        xcptBUSWRITE();
        }
    }
}


void __fastcall FastWriteGuestLogicalWord(ULONG addr, WORD w)
{
    WORD *pw = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(WORD));

    if (pw)
        {
        *pw = w;
        return;
        }

#if PERFCOUNT
    cntDataMiss++;
#endif

    {
    WORD wData = w;

    if (NO_ERROR != WriteGuestLogicalWord(addr, &wData))
        {
        lAccessAddr = addr;

        if ((addr & 1) && (vmCur.bfCPU < cpu68020))
            {
            xcptBUSADDR();
            }
        else
            {
            xcptBUSWRITE();
            }
        }
    }
}


void __fastcall FastWriteGuestLogicalLong(ULONG addr, ULONG l)
{
    ULONG *pl = pvTranslationLookupForGuestLogicalWrite(addr, sizeof(ULONG));

    if (pl)
        {
        *pl = l;
        return;
        }

#if PERFCOUNT
    cntDataMiss++;
#endif

    {
    ULONG lData = l;

#if 1
    if (NO_ERROR != WriteGuestLogicalLong(addr, &lData))
        {
        lAccessAddr = addr;

        if ((addr & 1) && (vmCur.bfCPU < cpu68020))
            {
            xcptBUSADDR();
            }
        else
            {
            xcptBUSWRITE();
            }
        }
#else
    {
    WORD w;

    w = (WORD)((lData >> 16) & 0xFFFF);

    FastWriteGuestLogicalWord(addr + 0, w);

    w = (WORD)(lData & 0xFFFF);

    FastWriteGuestLogicalWord(addr + 2, w);
    }
#endif
    }
}


void __fastcall FastWriteGuestLogicalWordP(ULONG addr, WORD w)
{
    BYTE b;

    b = (BYTE)(w >> 8);

    FastWriteGuestLogicalByte(addr + 0, b);

    b = (BYTE)(w & 0xFF);

    FastWriteGuestLogicalByte(addr + 2, b);
}


void __fastcall FastWriteGuestLogicalLongP(ULONG addr, LONG l)
{
    BYTE b;

    b = (BYTE)((l >> 24) & 0xFF);

    FastWriteGuestLogicalByte(addr + 0, b);

    b = (BYTE)((l >> 16) & 0xFF);

    FastWriteGuestLogicalByte(addr + 2, b);

    b = (BYTE)((l >> 8) & 0xFF);

    FastWriteGuestLogicalByte(addr + 4, b);

    b = (BYTE)(l & 0xFF);

    FastWriteGuestLogicalByte(addr + 6, b);
}


//
// InvalidatePages
//
// Invalidate the page table entires corresponding to the address range
// specified. Entries are freed in order to force a page fault next time
// they are accessed.
//

void InvalidateDispatch()
{
    if (vpregs)
        {
        int i;

        for (i = 0; i < sizeof(vpregs->duops)/sizeof(vpregs->duops[0]); i++)
            {
            extern char fetch_empty;

            vpregs->duops[i].pfn = &fetch_empty;
            vpregs->duops[i].guestPC = -1;
            }
        }

#if TRACEMMU
    printf("InvalidateDispatch!\n");
#endif
}


void InvalidatePages(ULONG eaMin, ULONG eaLim)
{
    if (vpregs)
        {
        int i;
        ULONG j;

        for (i = 0, j = 0xFFFFFFFF; i < 2048; i++, j -= cbSpan)
            {
            vpregs->memtlbRW[i+i] = j;
            vpregs->memtlbRO[i+i] = j;
            }

        // Ok to blast $FF to all entries
        // Except the last slot, which needs to have values not equal to $FF
        // to avoid a false match on the address lookup!

        memset(&vpregs->memtlbPC, 0xFF, sizeof(vpregs->memtlbPC));
        vpregs->memtlbPC[0*2] = 0x33333333;
        vpregs->memtlbPC[0*2] = 0x33333333;
        vpregs->memtlbPC[1023*2] = 3;
        vpregs->memtlbPC[2047*2] = 3;

        InvalidateDispatch();
        }

#if TRACEMMU
    printf("InvalidatePages!\n");
#endif
}


//
// Blit routines to copy to and from guest logical address space.
// This will apply CPU translation and write protection as requested.
//
// Returns TRUE is full copy succeeds, FALSE if any part fails.
//

BOOL GuestToHostDataCopy(ULONG addr, BYTE *pb, ULONG mode, ULONG cb)
{
    ULONG i;
    BYTE *pbGuestMin, *pbGuestLim;
    int  delta = 1;

#if !defined(NDEBUG)
    if (cb == 0)
        {
        __asm int 3;
        return TRUE;
        }

    if (pb == NULL)
        {
        __asm int 3;
        return FALSE;
        }

    if (0 != (mode & (ACCESS_WRITE | ACCESS_INIT)))
        {
        __asm int 3;
        return FALSE;
        }
#endif

    if (0 == (mode & ACCESS_PHYSICAL))
        {
        if (!FTranslateGuestAddress(&addr, FALSE))
            return FALSE;
        }

    // Fast path when not spanning a memory block

    if (0 == ((addr ^ (addr + cb - 1)) & dwBaseMask))
        {
        if (mode & (ACCESS_RMW | ACCESS_WRITE))
            {
            pbGuestMin = (BYTE *)MapWritableAddressVM(addr);
            }
        else    // READ or INIT
            {
            pbGuestMin = (BYTE *)MapAddressVM(addr);
            }

        if (NULL == pbGuestMin)
            return FALSE;

        if (mode & ACCESS_BYTESWAP)
            {
            pbGuestMin -= cb - 1;

            for (i = 0; i < cb; i++)
                {
                *pb++ = *pbGuestMin++;
                }
            }
        else
            {
            for (i = 0; i < cb; i++)
                {
                *pb++ = *pbGuestMin--;
                }
            }

        return TRUE;
        }

    if (mode & (ACCESS_RMW | ACCESS_WRITE))
        {
        pbGuestMin = (BYTE *)MapWritableAddressVM(addr);
        pbGuestLim = (BYTE *)MapWritableAddressVM(addr+cb-1);
        }
    else    // READ or INIT
        {
        pbGuestMin = (BYTE *)MapAddressVM(addr);
        pbGuestLim = (BYTE *)MapAddressVM(addr+cb-1);
        }

    if (NULL == pbGuestMin)
        return FALSE;

    if (NULL == pbGuestLim)
        return FALSE;

    // Reverse endian copy

    if (pbGuestMin == (pbGuestLim + cb - 1))
        delta = -1;

    // Same endian copy

    else if (pbGuestLim == (pbGuestMin + cb - 1))
        delta = +1;

#if !defined(NDEBUG)
    else
        {
        __asm int 3;
        return FALSE;
        }
#endif

    if (mode & ACCESS_BYTESWAP)
        {
        pbGuestMin = pbGuestLim;
        delta = -delta;
        }

    for (i = 0; i < cb; i++)
        {
        *pb++ = *pbGuestMin;
        pbGuestMin += delta;
        }

    return TRUE;
}

BOOL HostToGuestDataCopy(ULONG addr, BYTE *pb, ULONG mode, ULONG cb)
{
    ULONG i;
    BYTE *pbGuestMin, *pbGuestLim;
    int  delta = 1;

#if !defined(NDEBUG)
    if (cb == 0)
        {
        __asm int 3;
        return TRUE;
        }

    if (pb == NULL)
        {
        __asm int 3;
        return FALSE;
        }

    if (0 == (mode & (ACCESS_WRITE | ACCESS_INIT)))
        {
        __asm int 3;
        return FALSE;
        }
#endif

    if (0 == (mode & ACCESS_PHYSICAL))
        {
        if (!FTranslateGuestAddress(&addr, FALSE))
            return FALSE;
        }

    // Fast path when not spanning a memory block

    if (0 == ((addr ^ (addr + cb - 1)) & dwBaseMask))
        {
        if (mode & (ACCESS_RMW | ACCESS_WRITE))
            {
            pbGuestMin = (BYTE *)MapWritableAddressVM(addr);
            }
        else    // READ or INIT
            {
            pbGuestMin = (BYTE *)MapAddressVM(addr);
            }

        if (NULL == pbGuestMin)
            return FALSE;

        // Invalidate cached dispatch addresses for this block

        ZapRange(addr, cb);

        if (mode & ACCESS_BYTESWAP)
            {
            pbGuestMin -= cb - 1;

            for (i = 0; i < cb; i++)
                {
                *pbGuestMin++ = *pb++;
                }
            }
        else
            {
            for (i = 0; i < cb; i++)
                {
                *pbGuestMin-- = *pb++;
                }
            }

        return TRUE;
        }

    if (mode & (ACCESS_RMW | ACCESS_WRITE))
        {
        pbGuestMin = (BYTE *)MapWritableAddressVM(addr);
        pbGuestLim = (BYTE *)MapWritableAddressVM(addr+cb-1);
        }
    else    // READ or INIT
        {
        pbGuestMin = (BYTE *)MapAddressVM(addr);
        pbGuestLim = (BYTE *)MapAddressVM(addr+cb-1);
        }

    if (NULL == pbGuestMin)
        return FALSE;

    if (NULL == pbGuestLim)
        return FALSE;

    // Reverse endian copy

    if (pbGuestMin == (pbGuestLim + cb - 1))
        delta = -1;

    // Same endian copy

    else if (pbGuestLim == (pbGuestMin + cb - 1))
        delta = +1;

#if !defined(NDEBUG)
    else
        {
        __asm int 3;
        return FALSE;
        }
#endif

    ZapRange(addr, cb);

    if (mode & ACCESS_BYTESWAP)
        {
        pbGuestMin = pbGuestLim;
        delta = -delta;
        }

    for (i = 0; i < cb; i++)
        {
        *pbGuestMin = *pb++;
        pbGuestMin += delta;
        }

    return TRUE;
}



#if 0
void DelayMs(ULONG ms)
{
    ULONG tickStart;

    QueryTickCtr();
    tickStart = Getms();

    do
        {
        QueryTickCtr();
        } while ((Getms() - tickStart) < ms);
}
#endif


#if defined(ATARIST) || defined(SOFTMAC)

//
// F68KFPU and helper routines
//
// Given the current 68K FPIAR and PC, decode the current FPU
// instruction and execute it. If the instruction succeeds,
// update the PC as appropriate and return 0.
// If the instruction fails, set the PC to the start of the
// instruction (FPIAR) and return 1.
//

// Map the size encoding to the size of the data type

unsigned char const mpmdcb[8] =
{
    4,      // 32-bit int
    4,      // single precision real
    12,     // extended precision real
    12,     // BCD real
    2,      // 16-bit int
    8,      // double precision real
    1,      // 8-bit int
    0
};

static void __inline PushFPX(int iFP)
{
    unsigned char *pvSrc = &vpregs2->rgbFPx[iFP * 16];

#if TRACEFPU
    double d;
    BYTE rgch[256];

    __asm mov   eax,pvSrc
    __asm fld   tbyte ptr [eax]
    __asm lea   eax,d
    __asm fstp  qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Loading value %g from FP%d\n", d, iFP);
    printf(rgch);
#endif
#endif

    __asm mov   eax,pvSrc
    __asm fld   tbyte ptr [eax]

#if TRACEFPU
    iFPU++;
#endif
}

static void __inline PushFPX16(int iFP16)
{
    unsigned char *pvSrc = iFP16 + (char *)&vpregs2->rgbFPx[0];

#if TRACEFPU
    double d;
    BYTE rgch[256];

    __asm mov   eax,pvSrc
    __asm fld   tbyte ptr [eax]
    __asm lea   eax,d
    __asm fstp  qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Loading value %g from FP%d\n", d, iFP16);
    printf(rgch);
#endif
#endif

    __asm mov   eax,pvSrc
    __asm fld   tbyte ptr [eax]

#if TRACEFPU
    iFPU++;
#endif
}

static void __inline PushReg(int md, ULONG ea)
{
    ULONG *pvSrc = (ULONG *)&vpregs->D0 + ea;
    ULONG l;

#if TRACEFPU
    printf("Loading mode %d value from register R%d, pv = %08X, %08X\n", md, ea, pvSrc, *pvSrc);
#endif

    switch (md)
        {
    default:
#if TRACEFPU
        printf("UNIMPLEMENTED PUSHREG MODE %d\n", md);
#endif
        __asm fldz
        break;

    case 0:
        __asm mov   eax,pvSrc
        __asm fild  dword ptr [eax]
        break;

    case 1:
        __asm mov   eax,pvSrc
        __asm fld   dword ptr [eax]
        break;

    case 4:
        __asm mov   eax,pvSrc
        __asm fild  word  ptr [eax]
        break;

    case 6:
        l = *(signed char *)pvSrc;
        __asm fild  dword ptr l
        break;
        }

#if TRACEFPU
    iFPU++;
#endif
}


static void __inline Normalize(unsigned char *rgb)
{
    ULONG *rgl = (ULONG *)rgb;

    rgl[2] >>= 16;      // move exponent from 12-byte form to 10-byte form

    while ((rgl[1] & 0x80000000) == 0)
        {
        unsigned long c;

#if TRACEFPU
        printf("DENORMALIZED EXTENDED NUMBER!!!!\n");
#endif

        c = (rgl[0] & 0x80000000);
        rgl[0] += rgl[0];
        rgl[1] += rgl[1] + (c ? 1 : 0);

        if (!(rgl[0] | rgl[1]))
            break;

        rgl[2]--;

#if TRACEFPU
        printf("new exponent = %04X\n", (*(unsigned short *)(&rgl[2])));
#endif
        }
}


static void PushFPMdEa(int md, ULONG ea)
{
    unsigned char rgb[12];

    switch (md)
        {
    default:
        Assert(FALSE);
        break;

    case 7:
#if TRACEFPU
        printf("UNIMPLEMENTED PUSH MODE %d\n", md);
#endif
        __asm fldz
        break;

    case 0:
        ReadGuestLogicalLong(ea, (DWORD *)&rgb[0]);
        __asm fild  dword ptr rgb
        break;

    case 1:
        ReadGuestLogicalLong(ea, (DWORD *)&rgb[0]);
        __asm fld   dword ptr rgb
        break;

    case 2:
        ReadGuestLogicalLong(ea + 0, (DWORD *)&rgb[8]);
        ReadGuestLogicalLong(ea + 4, (DWORD *)&rgb[4]);
        ReadGuestLogicalLong(ea + 8, (DWORD *)&rgb[0]);
        Normalize(rgb);

        __asm fld   tbyte ptr rgb
        break;

    case 3:
        GuestToHostDataCopy(ea, rgb, ACCESS_READ /* | ACCESS_BYTESWAP */, 12);

        {
        double d = 1.0;
        int i, exp;
        BOOL fSM = 0, fSE = 0, fYY = 0;

#if TRACEFPU
        printf("LOADING BCD NUMBER!!!!\n");
#endif

        fSM = (rgb[0] & 0x80) != 0;
        fSE = (rgb[0] & 0x40) != 0;
        fYY = (rgb[0] & 0x30) >> 4;

        exp = ((rgb[0] & 15) * 100) +
              ((rgb[1] >> 4) * 10) +
               (rgb[1] & 15);

#if TRACEFPU
        for (i = 0; i < 12; i++)
            {
            printf("%02X ", rgb[i]);
            }
        printf("\n");

        printf("SM:%d SE:%d YY:%d exp:%03d\n", fSM, fSE, fYY, exp);
#endif

        d = (double)(rgb[3] & 15);

        for (i = 4; i < 12; i++)
            {
            d *= 10.0;
            d += (double)(rgb[i] >> 4);
            d *= 10.0;
            d += (double)(rgb[i] & 15);
            }

        d /= 1E16;

        if (fSM)
            d = -d;

        for (i = 0; i < exp; i++)
            {
            if (fSE)
                d /= 10.0;
            else
                d *= 10.0;
            }

        __asm lea   eax,d
        __asm fld   qword ptr [eax]
        }

        break;

    case 4:
        ReadGuestLogicalWord(ea, (WORD *)&rgb[0]);
        __asm fild  word  ptr rgb
        break;

    case 5:
        ReadGuestLogicalLong(ea + 0, (DWORD *)&rgb[4]);
        ReadGuestLogicalLong(ea + 4, (DWORD *)&rgb[0]);
        __asm fld   qword ptr rgb
        break;

    case 6:
        ReadGuestLogicalByte(ea, (BYTE *)&rgb[0]);
        *(WORD *)&rgb[0] = (signed short)(signed char)rgb[0];
        __asm fild  word ptr rgb
        break;
        }

#if TRACEFPU
    {
    double d;
    BYTE rgch[256];

    __asm lea   eax,d
    __asm fst   qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Loading value %g from %08X, mode %d\n", d, ea, md);
    printf(rgch);
#endif
    }
#endif

#if TRACEFPU
    iFPU++;
#endif
}

static void __inline PopFPX(int iFP)
{
    unsigned char *pvDst = &vpregs2->rgbFPx[iFP * 16];

#if TRACEFPU
    double d;
    BYTE rgch[256];

    __asm lea   eax,d
    __asm fst   qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Storing value %g into FP%d\n", d, iFP);
    printf(rgch);
#endif
#endif

    __asm mov   eax,pvDst
    __asm fstp  tbyte ptr [eax]

#if TRACEFPU
    iFPU--;
#endif
}

static void __inline PopFPX16(int iFP16)
{
    unsigned char *pvDst = iFP16 + (char *)&vpregs2->rgbFPx[0];

#if TRACEFPU
    double d;
    BYTE rgch[256];

    __asm lea   eax,d
    __asm fst   qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Storing value %g into FP%d\n", d, iFP16);
    printf(rgch);
#endif
#endif

    __asm mov   eax,pvDst
    __asm fstp  tbyte ptr [eax]

#if TRACEFPU
    iFPU--;
#endif
}

static void __inline PopReg(int md, ULONG ea)
{
    ULONG *pvDst = (ULONG *)&vpregs->D0 + ea;
    ULONG l;

#if TRACEFPU
    printf("Storing mode %d value into register R%d, pv = %08X\n", md, ea, pvDst);
#endif

    switch (md)
        {
    default:
#if TRACEFPU
        printf("UNIMPLEMENTED POPREG MODE %d\n", md);
#endif
        __asm fstp st(0)
        break;

    case 0:
        __asm mov   eax,pvDst
        __asm fistp dword ptr [eax]
        break;

    case 1:
        __asm mov   eax,pvDst
        __asm fstp  dword ptr [eax]
        break;

    case 4:
        __asm mov   eax,pvDst
        __asm fistp word  ptr [eax]
        break;

    case 6:
        __asm fistp  dword ptr l
        *(char *)pvDst = (char)l;
        break;
        }

#if TRACEFPU
    iFPU--;
#endif
}

static void PopFPMdEa(int md, ULONG ea, int k)
{
    unsigned char rgb[12];
    double d;

#if TRACEFPU
    BYTE rgch[256];

    __asm lea   eax,d
    __asm fst   qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Storing value %g into %08X, mode %d\n", d, ea, md);
    printf(rgch);
#endif
#endif

    switch (md)
        {
    default:
#if TRACEFPU
        printf("UNIMPLEMENTED POP MODE %d\n", md);
#endif
        __asm fstp st(0)

        Assert(FALSE);
        break;

    case 0:
        __asm fistp dword ptr rgb
        WriteGuestLogicalLong(ea, (DWORD *)&rgb[0]);
        break;

    case 1:
        __asm fstp  dword ptr rgb
        WriteGuestLogicalLong(ea, (DWORD *)&rgb[0]);
        break;

    case 2:
        __asm fstp  tbyte ptr rgb
        rgb[10] = rgb[11] = 0;
        WriteGuestLogicalWord(ea+0, (WORD *)&rgb[8]);
        WriteGuestLogicalWord(ea+2, (WORD *)&rgb[10]);
        WriteGuestLogicalLong(ea+4, (DWORD *)&rgb[4]);
        WriteGuestLogicalLong(ea+8, (DWORD *)&rgb[0]);
        break;

    case 4:
        __asm fistp word  ptr rgb
        WriteGuestLogicalWord(ea, (WORD *)&rgb[0]);
        break;

    case 5:
        __asm fstp  qword ptr rgb
        WriteGuestLogicalLong(ea+0, (DWORD *)&rgb[4]);
        WriteGuestLogicalLong(ea+4, (DWORD *)&rgb[0]);
        break;

    case 6:
        __asm fistp word ptr rgb
        WriteGuestLogicalByte(ea+0, (BYTE *)&rgb[0]);
        break;

    case 7:
        k = *((ULONG *)&vpregs->D0 + ((k >> 4) & 7));

        if (k > 17)
            k = 17;
        else if (k < -64)
            k = 17;

        // fall through

    case 3:
        k = ((int)(k << 25)) >> 25;

#if TRACEFPU
        printf("SAVING BCD NUMBER!!!!\n");
#endif

        __asm lea   eax,d
        __asm fstp  qword ptr [eax]

        memset(rgb, 0, 12);

        if (d != 0.0)
            {
            int i, exp = 0;
            BOOL fSM = 0, fSE = 0, fYY = 0;

            if (d < 0.0)
                {
                fSM = fTrue;
                d = -d;
                }

            while (d >= 10.0)
                {
                d /= 10.0;
                exp++;
                }

            while (d < 1.0)
                {
                d *= 10.0;
                exp++;
                fSE = fTrue;
                }

            // d is now normalized to be 1.0 <= d < 10.0
            // fSM, fSE, and exp are set

            rgb[0] |= fSM << 7;
            rgb[0] |= fSE << 6;
            rgb[0] |= fYY << 4;

            rgb[0] |= (exp / 100) % 10;
            rgb[1] |= ((exp / 10) % 10) << 4;
            rgb[1] |= exp % 10;
            rgb[2] |= (exp / 1000) << 4;

            rgb[3] = (BYTE)d;

            for (i = 4; i < 12; i++)
                {
                d -= (double)((BYTE)d);
                d *= 10.0;
                rgb[i] = ((BYTE)d) << 4;
                d -= (double)((BYTE)d);
                d *= 10.0;
                rgb[i] |= (BYTE)d;
                }
#if TRACEFPU
            for (i = 0; i < 12; i++)
                {
                printf("%02X ", rgb[i]);
                }
            printf("\n");

            printf("SM:%d SE:%d YY:%d exp:%03d\n", fSM, fSE, fYY, exp);
#endif
            }

        HostToGuestDataCopy(ea, rgb, ACCESS_WRITE /* | ACCESS_BYTESWAP */, 12);
        }

#if TRACEFPU
    iFPU--;
#endif
}


BOOL __cdecl Fcc(int eamode)
{
    BOOL f = FALSE;

    switch((eamode ^ ((eamode & 8) ? -1 : 0)) & 7)
        {
    default:
        Assert(FALSE);
        break;

    case 0:             // FBF
        f = FALSE;
        break;

    case 1:             // FBEQ
        f = vpregs->rgffpsr.ccZ != 0;
        break;

    case 2:             // FBGT
        f = !(vpregs->rgffpsr.ccZ   ||
              vpregs->rgffpsr.ccN   ||
              vpregs->rgffpsr.ccnan);
        break;

    case 3:             // FBGE
        f = vpregs->rgffpsr.ccZ   ||
           !(vpregs->rgffpsr.ccN  ||
             vpregs->rgffpsr.ccnan);
        break;

    case 4:             // FBLT
        f = vpregs->rgffpsr.ccN   &&
           !(vpregs->rgffpsr.ccZ  ||
             vpregs->rgffpsr.ccnan);
        break;

    case 5:             // FBLE
        f = vpregs->rgffpsr.ccZ   ||
            (vpregs->rgffpsr.ccN  &&
            !vpregs->rgffpsr.ccnan);
        break;

    case 6:             // FBGL
        f = !(vpregs->rgffpsr.ccZ   ||
             vpregs->rgffpsr.ccnan);
        break;

    case 7:             // FBUN
        f = vpregs->rgffpsr.ccnan == 0;
        break;
        }

    if (eamode & 8)
        {
        // This is the opposite condition, so reverse result

        f = !f;
        }

    if (eamode & 16)
        {
        // This is a signaling condition
        // Set BSUN bit if NAN set

        if (vpregs->rgffpsr.ccnan)
            vpregs->rgffpsr.esbsun = 1;
        }

    return f;
}


static void __inline SetFPUPrec()
{
    static ULONG const mpRndMode[4] = { 0 << 10, 3 << 10, 1 << 10, 2 << 10 };
    static ULONG const mpPrcMode[4] = { 3 << 8 , 0 << 8 , 2 << 8 , 1 << 8  };
    ULONG sw, sw0;

    __asm fnstcw sw0

    sw = (sw0 & 0x0000F0FF) |
         mpRndMode[vpregs->rgffpcr.rnd] |
         mpPrcMode[vpregs->rgffpcr.prec];

    if (sw != sw0)
        {
        __asm fldcw sw
        }

    // clear any existing FP exception flags

    __asm fnclex

}


ULONG __fastcall DoOp68KFPU(int mode, int iDst16)
{
    ULONG sw;

    switch (mode)
        {
    case 0x00:                  // FMOVE
    case 0x40:                  // FSMOVE
    case 0x44:                  // FDMOVE
        __asm ftst
        break;

    case 0x01:                  // FINT
        __asm frndint
        break;

    case 0x02:                  // FSINH
        {
        double d;

        __asm fstp  d
        d = sinh(d);            // REVIEW: double precision only
        __asm fld   d
        }
        break;

    case 0x03:                  // FINTRZ
        __asm frndint
        break;

    case 0x04:                  // FSQRT
    case 0x41:                  // FSSUB
    case 0x45:                  // FDSUB
        __asm fsqrt
        break;

    case 0x06:                  // FLOGNP1
        __asm fld1
        __asm fxch
        __asm fyl2xp1
        __asm fldln2
        __asm fmul
        break;

    case 0x08:                  // FETOXM1
        __asm fldl2e            // TOS = log2(e)
        __asm fmul
        __asm f2xm1
        break;

    case 0x09:                  // FTANH
        {
        double d;

        __asm fstp  d
        d = tanh(d);            // REVIEW: double precision only
        __asm fld   d
        }
        break;

    case 0x0A:                  // FATAN
        __asm fld1
        __asm fpatan
        break;

    case 0x0C:                  // FASIN
        // __asm fasin
        __asm fld1
        __asm fadd st,st(1)
        __asm fld1
        __asm fsub st,st(2)
        __asm fmulp st(1),st
        __asm fsqrt
        __asm fpatan
        break;

    case 0x0D:                  // FATANH
        {
        double d;               // REVIEW: double precision only

        __asm fstp  d

        if ((d > -1.0) && (d < 1.0))
            {
            d = (log(1.0 + d) - log(1.0 - d)) / 2.0;
            }

        __asm fld   d
        }
        break;

    case 0x0E:                  // FSIN
        __asm fsin
        break;

    case 0x0F:                  // FTAN
        __asm fptan
        __asm fstp st(0)
        break;

    case 0x10:                  // FE2X
        __asm fldl2e            // TOS = log2(e)
LBaseToX:
        __asm fmul              // TOS = X * log2(e)
        __asm fld1
        __asm fchs              // TOS = -1.0, X * log2(e)
        __asm fld st(1)         // TOS = X * log2(e), -1.0, X * log2(e)
        __asm frndint           // TOS = I, -1.0, X * log2(e)
        __asm fxch st(2)        // X * log2(e), -1.0, I
        __asm fsub st,st(2)
        __asm f2xm1
        __asm fsubr
        __asm fscale
        __asm fstp st(1)
        break;

    case 0x11:                  // F2TOX
        __asm fld1              // TOS = log2(2) = 1.0
        goto  LBaseToX;
        break;

    case 0x12:                  // FTENTOX
        __asm fldl2t            // TOS = log2(10)
        goto  LBaseToX;
        break;

    case 0x14:                  // FLOGN
        __asm fldln2
        __asm fxch
        __asm fyl2x
        break;

    case 0x15:                  // FLOG10
        __asm fld1
        __asm fldl2t
        __asm fdiv
        __asm fxch
        __asm fyl2x
        break;

    case 0x16:                  // FLOG2
        __asm fld1
        __asm fxch
        __asm fyl2x
        break;

    case 0x18:                  // FABS
    case 0x58:                  // FSABS
    case 0x5C:                  // FDABS
        __asm fabs
        break;

    case 0x19:                  // FCOSH
        {
        double d;

        __asm fstp  d
        d = cosh(d);            // REVIEW: double precision only
        __asm fld   d
        }
        break;

    case 0x1A:                  // FNEG
    case 0x5A:                  // FSNEG
    case 0x5E:                  // FDNEG
        __asm fchs
        break;

    case 0x1C:                  // FACOS
        // __asm facos
        __asm fld1
        __asm fadd st,st(1)
        __asm fld1
        __asm fsub st,st(2)
        __asm fmulp st(1),st
        __asm fsqrt
        __asm fxch st(1)
        __asm fpatan
        break;

    case 0x1D:                  // FCOS
        __asm fcos
        break;

    case 0x1E:                  // FGETEXP
        __asm fxtract
        __asm fstp st(0)
        break;

    case 0x1F:                  // FGETMAN
        __asm fxtract
        __asm fstp st(1)
        break;

    case 0x20:                  // FDIV
    case 0x60:                  // FSDIV
    case 0x64:                  // FDDIV
        PushFPX16(iDst16);
        __asm fdivr
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x21:                  // FMOD
        PushFPX16(iDst16);
        __asm fprem
        __asm fstp st(1)
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x22:                  // FADD
    case 0x62:                  // FSADD
    case 0x66:                  // FDADD
        PushFPX16(iDst16);
        __asm fadd
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x23:                  // FMUL
    case 0x63:                  // FSMUL
    case 0x67:                  // FDMUL
        PushFPX16(iDst16);
        __asm fmul
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x25:                  // FREM (IEEE)
        PushFPX16(iDst16);
        __asm fprem1
        __asm fstp st(1)
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x26:                  // FSCALE
        PushFPX16(iDst16);
        __asm fscale
        __asm fstp st(1)
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x28:                  // FSUB
    case 0x68:                  // FSSUB
    case 0x6C:                  // FDSUB
        PushFPX16(iDst16);
        __asm fsubr
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x30:                  // FSINCOS
    case 0x31:                  // FSINCOS
    case 0x32:                  // FSINCOS
    case 0x33:                  // FSINCOS
    case 0x34:                  // FSINCOS
    case 0x35:                  // FSINCOS
    case 0x36:                  // FSINCOS
    case 0x37:                  // FSINCOS
        __asm fsincos
        PopFPX(mode & 7);
        break;

    case 0x38:                  // FCMP
        PushFPX16(iDst16);
#if 0
        __asm fxch
        __asm fcomp
#else
        __asm fsubr
        iDst16 = 128;
#endif
#if TRACEFPU
        iFPU--;
#endif
        break;

    case 0x3A:                  // FTST
        iDst16 = 128;
        __asm ftst
        break;

    default:
#if TRACEFPU
        printf("UNIMPLEMENTED MODE %02X!!!\n", mode);
#endif
        break;
        }

#if TRACEFPU
    __asm fnstsw sw

    if ((sw & 0x3800) != 0x3800)
        {
        fDebug++;
        printf("FPU Stack Overflow %04X: PC = %08X, SP = %08X, mode = %d\n",
            sw, vpregs->PC, vpregs->A7, mode);
        m68k_DumpRegs();
        fDebug--;
        }
#endif

    __asm ftst
    __asm fnstsw sw

    if (iDst16 < 128)
        PopFPX16(iDst16);
    else
        {
        __asm fstp st(0)
#if TRACEFPU
        iFPU--;
#endif
        }

    return sw;
}

void __fastcall DoUpd68KFPU(ULONG sw)
{
#if 0
    // slow code

    vpregs2->rgffpsr.ccI      = 0;
    vpregs2->rgffpsr.ccZ      = (sw & 0x4000) != 0;
    vpregs2->rgffpsr.ccN      = (sw & 0x0100) != 0;
    vpregs2->rgffpsr.ccnan    = (sw & 0x4500) == 0x4500;

    vpregs2->rgffpsr.S        = 0;
    vpregs2->rgffpsr.quotient = 0;

//            __asm fnstsw sw

    vpregs2->rgffpsr.esbsun   = 0;
    vpregs2->rgffpsr.esnan    = 0;
    vpregs2->rgffpsr.esoperr  = 0;
    vpregs2->rgffpsr.esovfl   = (sw & 0x0008) != 0;
    vpregs2->rgffpsr.esunfl   = (sw & 0x0010) != 0;
    vpregs2->rgffpsr.esdz     = (sw & 0x0004) != 0;
    vpregs2->rgffpsr.esinex2  = (sw & 0x0020) != 0;
    vpregs2->rgffpsr.esinex1  = 0;

    vpregs2->rgffpsr.aeiop   |= 0;
    vpregs2->rgffpsr.aeovfl  |= (sw & 0x0008) != 0;
    vpregs2->rgffpsr.aeunfl  |= (sw & 0x0010) != 0;
    vpregs2->rgffpsr.aedz    |= (sw & 0x0004) != 0;
    vpregs2->rgffpsr.aeinex  |= (sw & 0x0028) != 0;
#else
    {
    // faster code

    static ULONG const mpIUOZtoOUIZ[16] =
        {
        (0x0 << 9) | 0x00, (0x2 << 9) | 0x10,
        (0x8 << 9) | 0x40, (0xA << 9) | 0x50,
        (0x4 << 9) | 0x20, (0x6 << 9) | 0x30,
        (0xC << 9) | 0x60, (0xE << 9) | 0x70,
        (0x1 << 9) | 0x80, (0x3 << 9) | 0x18,
        (0x9 << 9) | 0x48, (0xB << 9) | 0x58,
        (0x5 << 9) | 0x28, (0x7 << 9) | 0x38,
        (0xD << 9) | 0x68, (0xF << 9) | 0x78,
        };

#if TRACEFPU
    printf("sw = %08X\n", sw);
#endif

 // vpregs->rgffpsr.ccI      = 0;
 // vpregs->rgffpsr.S        = 0;
 // vpregs->rgffpsr.quotient = 0;
 // vpregs->rgffpsr.esbsun   = 0;
 // vpregs->rgffpsr.esnan    = 0;
 // vpregs->rgffpsr.esoperr  = 0;
 // vpregs->rgffpsr.esinex1  = 0;
 // vpregs->rgffpsr.aeiop   |= 0;

    vpregs->FPSR &= 0x00FF1EF8;

//  vpregs->rgffpsr.ccZ      = (sw & 0x4000) != 0;
//  vpregs->rgffpsr.ccN      = (sw & 0x0100) != 0;
//  vpregs->rgffpsr.ccnan    = (sw & 0x4500) == 0x4500;

    if (sw & 0x4000)
        vpregs->FPSR |= 0x04000000;
    if (sw & 0x0100)
        vpregs->FPSR |= 0x08000000;
    if ((sw & 0x4500) == 0x4500)
        vpregs->FPSR |= 0x01000000;

 // vpregs->rgffpsr.esovfl   = (sw & 0x0008) != 0;
 // vpregs->rgffpsr.esunfl   = (sw & 0x0010) != 0;
 // vpregs->rgffpsr.esdz     = (sw & 0x0004) != 0;
 // vpregs->rgffpsr.esinex2  = (sw & 0x0020) != 0;
 // vpregs->rgffpsr.aeovfl  |= (sw & 0x0008) != 0;
 // vpregs->rgffpsr.aeunfl  |= (sw & 0x0010) != 0;
 // vpregs->rgffpsr.aedz    |= (sw & 0x0004) != 0;
 // vpregs->rgffpsr.aeinex  |= (sw & 0x0028) != 0;

    // sw = IUOZ__
    //  ==> __OUZI

    vpregs->FPSR |= *(ULONG *)(((char *)mpIUOZtoOUIZ) + (sw & 0x3C));
    }
#endif
}


const ULONG rglCR[22*3] =
    {
    0x2168C235, 0xC90FDAA2, 0x4000, // 00: Pi
    0xFBCFF798, 0x9A209A84, 0x3FFD, // 0B: Log10(2)
    0xA2BB4A9A, 0xADF85458, 0x4000, // 0C: e
    0x5C17F0BC, 0xB8AA3B29, 0x3FFF, // 0D: Log2(e)
    0x37287195, 0xDE5BD8A9, 0x3FFD, // 0E: Log10(e)
    0x0,        0x0,        0x0   , // 0F: 0.0
    0xD1CF79AC, 0xB17217F7, 0x3FFE, // 30: ln(2)
    0xAAA8AC17, 0x935D8DDD, 0x4000, // 31: ln(10)
    0x0,        0x80000000, 0x3FFF, // 32: 1.0
    0x0,        0xA0000000, 0x4002, // 33: 10.0
    0x0,        0xC8000000, 0x4005, // 34: 100.0
    0x0,        0x9C400000, 0x400C, // 35: 10^4
    0x0,        0xBEBC2000, 0x4019, // 36: 10^8
    0x04000000, 0x8E1BC9BF, 0x4034, // 37: 10^16
    0x2B70B59E, 0x9DC5ADA8, 0x4069, // 38: 10^32
    0xFFCFA6D5, 0xC2781F49, 0x40D3, // 39: 10^64
    0x80E98CE0, 0x93BA47C9, 0x41A8, // 3A: 10^128
    0x9DF9DE8E, 0xAA7EEBFB, 0x4351, // 3B: 10^256
    0xA60E91C7, 0xE319A0AE, 0x46A3, // 3C: 10^512
    0x81750C17, 0xC9767586, 0x4D48, // 3D: 10^1024
    0xC53D5DE5, 0x9E8B3B5D, 0x5A92, // 3E: 10^2048
    0x8A20979B, 0xC4605202, 0x7525, // 3F: 10^4096
    };


void __fastcall DoMoveCR(ULONG mode)
{
    unsigned char *pvSrc;

#if TRACEFPU
    printf("FMOVECR: constant index = %d\n", mode);
#endif

    if (mode >= 0x40)
        mode = 0x05;        // zero
    else if (mode >= 0x30)
        mode -= (0x30 - 6);
    else if (mode >= 0x10)
        mode = 0x05;        // zero
    else if (mode >= 0x0B)
        mode -= (0x0B - 1);
    else if (mode >= 0x01)
        mode = 0x05;        // zero

    pvSrc = (char *)&rglCR[mode * 3];

#if TRACEFPU
    {
    double d;
    BYTE rgch[256];

    __asm mov   eax,pvSrc
    __asm fld   tbyte ptr [eax]
    __asm lea   eax,d
    __asm fstp  qword ptr [eax]

#if !defined(NDEBUG)
    sprintf(rgch, "Loading value %g\n", d);
    printf(rgch);
#endif
    }
#endif

    __asm mov   eax,pvSrc
    __asm fld   tbyte ptr [eax]

#if TRACEFPU
    iFPU++;
#endif
}


ULONG __fastcall F68KFPU(ULONG instr)
{
    ULONG xword = vi.pregs->xword;
    extern long lAccessAddr;
    ULONG lAddr = lAccessAddr;

    const int eamode = instr & 63;

    const BOOL fMem = (xword & 0x4000) != 0;
    const long iSrc = (xword >> 10) & 7;

    long iDst16 = (xword >> 3) & (7 << 4);
    long mode = xword & 127;

    const BOOL fPreD = (eamode >= 0x20) && (eamode <= 0x27);
    BOOL fReg  = FALSE;

    ULONG sw;

#if TRACEFPU
        {
        ULONG temp = vpregs->PC;
        vpregs->PC = vpregs->FPIAR;
        m68k_DumpHistory(80);
        m68k_DumpRegs();
        vpregs->PC = temp;
        }
    printf("FPU instr at %08X = %04X, %04X\n", vpregs->FPIAR, instr, xword);
    printf("FPU mode = %d\n", ((instr >> 9) & 7));
    printf("fPreD = %d, ea = %08X\n", fPreD, lAddr);
#endif

    // Handle register case

    if (!(instr & 0x0080) && (eamode <= 0x0F))
        {
#if TRACEFPU
        printf("register addressing mode %d at PC = %08X\n", eamode, vpregs->PC);
#endif

        lAddr = eamode;
        fReg = TRUE;

#if TRACEFPU
        printf("new ea = R%d\n", lAddr);
#endif
        }

    // Handle immediate EA case

    if (!(instr & 0x0080) && (eamode == 0x3C))
        {
        lAddr = vpregs->PC;

#if TRACEFPU
        printf("immediate addressing mode at PC = %08X\n", vpregs->PC);
#endif

        // Immediate points at the PC, then increment PC

        if (fMem)
            {
            vpregs->PC += mpmdcb[iSrc] + (iSrc == 6);  // .B mode skips two bytes
            lAddr += (iSrc == 6);
            }

        else if ((xword & 0x8000) && (xword & 0x1C00)) // FMOVE control register
            vpregs->PC += 4;
        }

    switch ((instr >> 6) & 7)
        {
    default:
        goto LnotFPU;

    case 0:
        // FPU TYPE 0, arithmetic operations

        // Weed out bogus memory mode

        if (fMem && (iSrc == 7))
            {
            if (instr == 0xF200)
                {
                // FMOVECR

                DoMoveCR(mode);

                mode = 0;
                goto Larithm;
                }

            if ((xword >> 13) != 3)
                goto LnotFPU;
            }

        // All decodings of the extension word are valid now

#if TRACEFPU
        printf("submode is %d\n", xword >> 13);
#endif

#if TRACEFPU
        printf("EA before adjust is %08X\n", lAddr);
#endif

        // undo any pre- or post-increment that GenEA made
        // NOTE: don't adjust post-increment since lAddr is correct!

        if (fPreD)
            lAddr += 4;

#if TRACEFPU
        printf("EA after adjust is %08X\n", lAddr);
#endif

#if 0
        // clear any existing FP exception flags

        __asm fnclex
#endif

        switch (xword >> 13)
            {
        case 0:             // register-to-register
#if TRACEFPU
            printf("source is register FP%d\n", iSrc);
#endif
            PushFPX(iSrc);

Larithm:

#if TRACEFPU
            printf("arithmetic mode %d\n", mode);
#endif

#if TRACEFPU
            __asm fnstsw sw

            if ((sw & 0x3800) != 0x3800)
                {
                fDebug++;
                printf("FPU Stack Overflow at Larithm %04X: PC = %08X, SP = %08X, mode = %d\n",
                    sw, vpregs->PC, vpregs->A7, mode);
                m68k_DumpRegs();
                fDebug--;
                }
#endif

            sw = DoOp68KFPU(mode, iDst16);
            DoUpd68KFPU(sw);

            break;

        case 1:
            goto LnotFPU;

        case 2:             // FMOVE memory to register
#if TRACEFPU
            printf("source is memory, ea = %08X, %08X %08X\n", lAddr, PeekL(lAddr), PeekL(lAddr+4));
#endif
            if (fPreD)
                lAddr -= mpmdcb[iSrc];
            if (fReg)
                PushReg(iSrc, lAddr);
            else
                PushFPMdEa(iSrc, lAddr);
            if (!fPreD)
                lAddr += mpmdcb[iSrc];

            goto Larithm;

        case 3:             // FMOVE data register to memory
#if TRACEFPU
            printf("FMOVE reg to mem, FP%d, ea = %08X\n", iDst16 >> 4, lAddr);
#endif
            PushFPX16(iDst16);
            if (fPreD)
                lAddr -= mpmdcb[iSrc];
            if (fReg)
                PopReg(iSrc, lAddr);
            else
                PopFPMdEa(iSrc, lAddr, xword & 127);
            if (!fPreD)
                lAddr += mpmdcb[iSrc];
            break;

        case 4:             // FMOVE memory to control register
#if TRACEFPU
            printf("FMOVE(M) mem to control reg, ea = %08X, reg(s) = %d\n", lAddr, iSrc);
#if 0
            m68k_DumpRegs();
            m68k_DumpHistory(160);
#endif
#endif

            if (!fPreD && (iSrc & 4))
                {
                vpregs->FPCR = (WORD)(fReg ? vpregs->rgregDn[lAddr] : PeekL(lAddr)) & 0x0000FFF0;
                SetFPUPrec();
#if TRACEFPU
                printf("FMOVE mem to FPCR, FPCR = %08X, ea = %08X\n", vpregs->FPCR, lAddr);
#endif
                lAddr += 4;
                }
            else if (fPreD && (iSrc & 1))
                {
                lAddr -= 4;
                vpregs->FPIAR = (fReg ? vpregs->rgregDn[lAddr] : PeekL(lAddr));
#if TRACEFPU
                printf("FMOVE mem to FPIAR, FPIAR = %08X, ea = %08X\n", vpregs->FPIAR, lAddr);
#endif
                }

            if (iSrc & 2)
                {
                if (fPreD)
                    lAddr -= 4;
                vpregs->FPSR = (fReg ? vpregs->rgregDn[lAddr] : PeekL(lAddr)) & 0x0FFFFFF8;
#if TRACEFPU
                printf("FMOVE mem to FPSR, FPSR = %08X, ea = %08X\n", vpregs->FPSR, lAddr);
#endif
                if (!fPreD)
                    lAddr += 4;
                }

            if (!fPreD && (iSrc & 1))
                {
                vpregs->FPIAR = (fReg ? vpregs->rgregDn[lAddr] : PeekL(lAddr));
#if TRACEFPU
                printf("FMOVE mem to FPIAR, FPIAR = %08X, ea = %08X\n", vpregs->FPIAR, lAddr);
#endif
                lAddr += 4;
                }
            else if (fPreD && (iSrc & 4))
                {
                lAddr -= 4;
                vpregs->FPCR = (WORD)(fReg ? vpregs->rgregDn[lAddr] : PeekL(lAddr)) & 0x0000FFF0;
                SetFPUPrec();
#if TRACEFPU
                printf("FMOVE mem to FPCR, FPCR = %08X, ea = %08X\n", vpregs->FPCR, lAddr);
#endif
                }

            break;

        case 5:             // FMOVE control register to memory
#if TRACEFPU
            printf("FMOVE control reg to mem, reg = %03X, ea = %08X\n", iSrc, lAddr);
#endif

            if (!fPreD && (iSrc & 4))
                {
                if (fReg)
                    vpregs->rgregDn[lAddr] = vpregs->FPCR;
                else
                    PokeL(lAddr, vpregs->FPCR);
#if TRACEFPU
            printf("FMOVE FPCR to mem, FPCR = %08X, ea = %08X\n", vpregs->FPCR, lAddr);
#endif
                lAddr += 4;
                }
            else if (fPreD && (iSrc & 1))
                {
                lAddr -= 4;
                if (fReg)
                    vpregs->rgregDn[lAddr] = vpregs->FPIAR;
                else
                    PokeL(lAddr, vpregs->FPIAR);
#if TRACEFPU
            printf("FMOVE FPIAR to mem, FPIAR = %08X, ea = %08X\n", vpregs->FPIAR, lAddr);
#endif
                }

            if (iSrc & 2)
                {
                if (fPreD)
                    lAddr -= 4;
                if (fReg)
                    vpregs->rgregDn[lAddr] = vpregs->FPSR;
                else
                    PokeL(lAddr, vpregs->FPSR);
#if TRACEFPU
            printf("FMOVE FPSR to mem, FPSR = %08X, ea = %08X\n", vpregs->FPSR, lAddr);
#endif
                if (!fPreD)
                    lAddr += 4;
                }

            if (!fPreD && (iSrc & 1))
                {
                if (fReg)
                    vpregs->rgregDn[lAddr] = vpregs->FPIAR;
                else
                    PokeL(lAddr, vpregs->FPIAR);
#if TRACEFPU
            printf("FMOVE FPIAR to mem, FPIAR = %08X, ea = %08X\n", vpregs->FPIAR, lAddr);
#endif
                lAddr += 4;
                }
            else if (fPreD && (iSrc & 4))
                {
                lAddr -= 4;
                if (fReg)
                    vpregs->rgregDn[lAddr] = vpregs->FPCR;
                else
                    PokeL(lAddr, vpregs->FPCR);
#if TRACEFPU
            printf("FMOVE FPCR to mem, FPCR = %08X, ea = %08X\n", vpregs->FPCR, lAddr);
#endif
                }

            break;

        case 6:     // FMOVEM data registers
        case 7:
            {
            BOOL fToMem = (xword & 0x2000) != 0;
            BOOL fDynam = (xword & 0x0800) != 0;
            ULONG reglist = xword & 255;
            int  fpreg  = (xword & 0x1000) != 0 ? 0 : 7;

            BOOL fPreD = (xword & 0x1000) == 0;

            if (fDynam)
                reglist = vpregs->rgregDn[(reglist >> 4) & 7] & 255;

#if TRACEFPU
            printf("FMOVEM fToMem = %d, fPreD = %d, fDynam = %d, reglist = %02X\n",
                fToMem, fPreD, fDynam, reglist);
#endif

            while (reglist)
                {
                while ((reglist & 0x80) == 0)
                    {
                    reglist = (reglist << 1) & 0xFF;

                    fpreg += (fPreD ? -1 : 1);

                    if (reglist == 0)
                        break;
                    }

                // pre-decrement mode

                if (fPreD)
                    lAddr -= 12;

                if (fToMem)
                    {
#if TRACEFPU
                    printf("FMOVEM register FP%d to %08X\n", fpreg, lAddr);
#endif
                    PushFPX(fpreg);
                    PopFPMdEa(2, lAddr, 0);
                    }
                else
                    {
#if TRACEFPU
                    printf("FMOVEM %08X to register FP%d\n", lAddr, fpreg);
#endif
                    PushFPMdEa(2, lAddr);
                    PopFPX(fpreg);
                    }

                // all other modes post-increment

                if (!fPreD)
                    lAddr += 12;

                // next register

                reglist = (reglist << 1) & 0xFF;
                fpreg += (fPreD ? -1 : 1);
                }
            }
            break;

            } // switch upper 3 bits of xword

#if TRACEFPU
        m68k_DumpRegs();
#endif

        // Update the address register for pre- post- modes

        if ((eamode >= 0x18) && (eamode <= 0x27))
            {
#if TRACEFPU
            printf("setting final value of A%d = %08X\n", eamode & 7, lAddr);
#endif
            *((ULONG *)&vpregs->A0 + (eamode & 7)) = lAddr;
#if TRACEFPU
            m68k_DumpRegs();
#endif
            }

        return 0;

    case 1:
        // FScc / FDBcc / FTRAPcc

        {
        BOOL f = Fcc(xword & 63);
        ULONG *plreg = &vpregs->D0;

#if TRACEFPU
        printf("in Fcc code!!! eamode = %02X, F = %d\n", xword & 63, f);
#endif

        if ((instr & 38) == 0x08)
            {
            // FDBcc

#if TRACEFPU
            printf("FDBcc!, D%d = %04X\n",
                 instr & 7, *(short *)(&plreg[instr & 7]));
#endif

            if (f)
                vpregs->PC += 2;

            else if (0 == (*(short *)(&plreg[instr & 7]))) // if Dn == 0
                {
                *(short *)(&plreg[instr & 7]) = -1;
                vpregs->PC += 2;
                }

            else
                {
                *(short *)(&plreg[instr & 7]) -= 1;
                vpregs->PC += (int)(short int)PeekW(vpregs->PC) - 2;
                }
            }

        else if ((instr & 38) == 0x38)
            {
            // FTRAPcc

#if TRACEFPU
            printf("FTRAPcc!\n");
#endif

            // REVIEW: need to implement the trap!

            if ((instr & 7) == 2)
                vpregs->PC += 2;
            else if ((instr & 7) == 3)
                vpregs->PC += 4;
            }

        else
            {
            // FScc

#if TRACEFPU
            printf("Fcc! ea = %08X\n", lAddr);
#endif

            if (fReg)
                *(BYTE *)(&plreg[instr & 7]) = (f ? 0xFF : 0);
            else
                PokeB(lAddr, (BYTE)(f ? 0xFF : 0));
            }
        }

        return 0;

    case 2:
    case 3:
        // FBcc

#if TRACEFPU
        printf("FBcc %02X at %08X\n", eamode, vpregs->FPIAR);
#endif
        {
        BOOL f = Fcc(eamode);

#if TRACEFPU
        printf("F = %d, old PC = %08X\n", f, vpregs->PC);
#endif

        if (f && (instr & 0x0040))
            {
            vpregs->PC += (long int)PeekL(vpregs->PC-2) - 2;
            }
        else if (f)
            {
            vpregs->PC += (int)(short int)PeekW(vpregs->PC-2) - 2;
            }
        else if (instr & 0x0040)
            {
            vpregs->PC += 2;
            }

#if TRACEFPU
        printf("F = %d, new PC = %08X\n", f, vpregs->PC);
#endif
        }
        return 0;

    case 4:
        // FSAVE

#if TRACEFPU
        printf("FSAVE\n");
#endif

#ifndef NOFPU
        // undo any pre- or post-increment that GenEA made
        // NOTE: don't adjust post-increment since lAddr is correct!

        if (fPreD)
            {
            lAddr += 4;
            if ((vmCur.bfCPU == cpu68020) && !vi.fFake040)
                lAddr -= 0x1C; // 68881 Idle State Frame size
            else if ((vmCur.bfCPU >= cpu68030) || vi.fFake040)
                lAddr -= 0x3C; // 68882 Idle State Frame size
            else
                lAddr -= 0x04; // 68040 Idle State Frame size
            }

        if ((vmCur.bfCPU == cpu68020) && !vi.fFake040)
            PokeL(lAddr, 0x1F180000);
        else if ((vmCur.bfCPU >= cpu68030) || vi.fFake040)
            PokeL(lAddr, 0x1F380000);
        else
            PokeL(lAddr, 0x1F000000);

        if (0 || vmCur.bfCPU < cpu68040)
            {
            PokeL(lAddr+0x04, 0x00000000);
            PokeL(lAddr+0x08, 0x00000000);
            PokeL(lAddr+0x0C, 0x00000000);
            PokeL(lAddr+0x10, 0x00000000);
            PokeL(lAddr+0x14, 0x00000000);
            PokeL(lAddr+0x18, 0x70000000);
            }

        if (!fPreD)
            {
            if ((vmCur.bfCPU == cpu68020) && !vi.fFake040)
                lAddr += 0x1C;
            else if ((vmCur.bfCPU >= cpu68030) || vi.fFake040)
                lAddr += 0x3C;
            else
                lAddr += 0x04;
            }
#else
        PokeL(lAddr, 0x00180000);

        if (!fPreD)
            lAddr += 0x04;
#endif

        // Update the address register for pre- post- modes

        if ((eamode >= 0x18) && (eamode <= 0x27))
            {
#if TRACEFPU
            printf("setting final value of A%d = %08X\n", eamode & 7, lAddr);
#endif
            *((ULONG *)&vpregs->A0 + (eamode & 7)) = lAddr;
#if TRACEFPU
            m68k_DumpRegs();
#endif
            }

        return 0;

    case 5:
        // FRESTORE

#ifndef NOFPU
        {
        ULONG l = PeekL(lAddr);

        if ((l & 0xFF000000) == 0)
            {
            // Null State Frame

#if TRACEFPU
            printf("restoring Null State Frame\n");
#endif
            lAddr += 0x04;

            vpregs->FPCR = 0;
            vpregs->FPSR = 0;

            memset(vpregs2->rgbFPx, 0xFF, sizeof(vpregs2->rgbFPx));

            // SetFPUPrec(); // will be set below
            }

        else if ((l & 0x00FF0000) == 0x00180000)
            {
            // Idle State Frame - 68881

#if TRACEFPU
            printf("restoring 68881 Idle State Frame\n");
#endif
            lAddr += 0x1C;
            }
        else if ((l & 0x00FF0000) == 0x00380000)
            {
            // Idle State Frame - 68882

#if TRACEFPU
            printf("restoring 68882 Idle State Frame\n");
#endif
            lAddr += 0x3C;
            }
        else
            {
            // Some other state frame, size is 4 bytes more than number

#if TRACEFPU
            printf("restoring Unknown State Frame %08X\n", l);
#endif
            lAddr += 4 + ((l & 0x00FF0000) >> 16);
            }
        }

        // Update the address register for post- modes

        if ((eamode >= 0x18) && (eamode <= 0x1F))
            {
#if TRACEFPU
            printf("setting final value of A%d = %08X\n", eamode & 7, lAddr);
#endif
            *((ULONG *)&vpregs->A0 + (eamode & 7)) = lAddr;
#if TRACEFPU
            m68k_DumpRegs();
#endif
            }
#endif // NOFPU

        // Good time to set rounding and precision

        SetFPUPrec();

        return 0;
        }

LnotFPU:
    vpregs->PC = vpregs->FPIAR;

#if TRACEFPU
    printf("\n\n\nINVALID OR UNIMPLEMENTED FPU INSTRUCTION\n\n\n");
    m68k_DumpRegs();
    m68k_DumpHistory(160);
#endif

    return 1;
}


//
// This is a special version of F68KFPU which is optimized for FPU instructions
// with a $F200 opcodes. This covers all register-to-register operations
// excluding FMOVECR which is encoded as a memory operation.
// Since the instruction is known to be $F200 we don't bother to pass it in.

ULONG __fastcall F68KFPUreg(DWORD xword)
{
    const long iSrc = (xword >> 10) & 7;
    const long iDst16 = ((xword >> 7) & 7) << 4;
    const long op = xword & 127;

    ULONG sw;

#if TRACEFPU
        {
        ULONG temp = vpregs->PC;
        vpregs->PC = vpregs->FPIAR;
        m68k_DumpHistory(80);
        m68k_DumpRegs();
        vpregs->PC = temp;
        }
    printf("FPU reg-to-reg instr at %08X = %04X, %04X\n", vpregs->FPIAR, 0xF200, vpregs->xword);
#endif

#if 0
    // clear any existing FP exception flags

    __asm fnclex
#endif

#if TRACEFPU
    printf("source is register FP%d\n", iSrc);
#endif

    PushFPX(iSrc);

#if TRACEFPU
    printf("arithmetic op %d\n", op);
#endif

#if TRACEFPU
    __asm fnstsw sw

    if ((sw & 0x3800) != 0x3800)
        {
        fDebug++;
        printf("FPU Stack Overflow at Larithm %04X: PC = %08X, SP = %08X, op = %d\n",
            sw, vpregs->PC, vpregs->A7, op);
        m68k_DumpRegs();
        fDebug--;
        }
#endif

    sw = DoOp68KFPU(op, iDst16);
    DoUpd68KFPU(sw);

    return 0;
}


//
// 68030-only MMU instructions
//
// These must use vmPeek/vmPoke as they are accessing untranslated space
// REVIEW: is that correct??
//

ULONG F030MMU()
{
    ULONG instr = vmPeekW(vpregs->FPIAR);
    ULONG xword = vmPeekW(vpregs->FPIAR+2);
    extern long lAccessAddr;

#if !defined(NDEBUG) || TRACEMMU
if (xword != vmPeekW(vpregs->FPIAR+2))
    {
    printf("xword mismatch! PC = %08X FPIAR+2=%04X xword=%04X\n",
       vpregs->PC, PeekW(vpregs->FPIAR+2), xword);

    Assert(0);
    }
#endif

#if TRACEMMU
    m68k_DumpRegs();
    m68k_DumpHistory(160);

    printf("68030 MMU instr = %04X, %04X\n", instr, xword);
    printf("PC = %08X\n", vpregs->PC);
#endif

    if ((instr == 0xF000) && (xword == 0x2400))
        {
        // 68030 PFLUSHA, ignore effective address

#if TRACEMMU
        printf("68030 PFLUSHA\n");
#endif
        InvalidatePages(0x00000000, 0xFFFFFFFF);

        // Because the effective address field is ignored, always 4 code bytes
        vpregs->PC = vpregs->FPIAR + 4;
        return 0;
        }

    // catch bogus 68030 MMU addressing modes which are allowed by
    // the 68851. If the FPIAR == PC, then it isn't valid on 68030

    if (vpregs->FPIAR == vpregs->PC)
        {
#if TRACEMMU
        printf("BOGUS 68030 MMU ADDRESSING MODE\n");
#endif
        return 1;
        }

    Assert (vmCur.bfCPU == cpu68030);

    // Check for PLOAD before PFLUSH

    if ((xword & 0xFDE0) == 0x2000)
        {
#if TRACEMMU
        printf("PLOAD fc,ea\n");
#endif
        return 0;
        }

    if ((xword & 0xF300) == 0x3000)
        {
#if TRACEMMU
        printf("PFLUSH fc,mask,ea\n");
#endif
        return 0;
        }

    if ((xword & 0xE0FF) == 0x4000)
        {
#if TRACEMMU
        printf("PMOVE, preg = %d, fRW = %d, addr = %08X\n",
             xword >> 1, xword & 1, lAccessAddr);
#endif

        switch ((xword >> 9) & 15)
            {
        default:
#if TRACEMMU
            printf("WARNING: unknown MMU instruction!\n");
#endif
            return 1;

        case 0:
            // PMOVE ea, TC

            vpregs2->TCR = vmPeekL(lAccessAddr) & 0x83FFFFFF;

#if TRACEMMU
            {
            ULONG bE, bSRE, bFCL, bPS, bIS, bTIA, bTIB, bTIC, bTID;

            bE   = (vpregs2->TCR & 0x80000000) >> 31;
            bSRE = (vpregs2->TCR & 0x02000000) >> 25;
            bFCL = (vpregs2->TCR & 0x01000000) >> 24;
            bPS  = (vpregs2->TCR & 0x00F00000) >> 20;
            bIS  = (vpregs2->TCR & 0x000F0000) >> 16;
            bTIA = (vpregs2->TCR & 0x0000F000) >> 12;
            bTIB = (vpregs2->TCR & 0x00000F00) >> 8;
            bTIC = (vpregs2->TCR & 0x000000F0) >> 4;
            bTID = (vpregs2->TCR & 0x0000000F) >> 0;

            printf("new TCR = %08X  ", vpregs2->TCR);
            printf("E:%d SRE:%d FCL:%d cbPage:%d IS:%d "
                   "TIA:%d TIB:%d TIC:%d TID:%d\n",
                    bE, bSRE, bFCL, 1 << bPS, bIS, bTIA, bTIB, bTIC, bTID);

            if (bE)
                {
#if 0
                ULONG iA, cb = 1 << bPS;

                for (iA = 0; iA < (1 << (32 - bIS - bPS - bTIA)); iA++)
                    {
                    printf("log addr %08X maps to phys addr %08X\n",
                        0, 0);
                    }
#endif
                }
            }
#endif
            break;

        case 1:
            // PMOVE TC, ea

#if TRACEMMU
            printf("reading TCR = %08X\n", vpregs2->TCR);
#endif
            vmPokeL(lAccessAddr, vpregs2->TCR);
            break;

        case 4:
            // PMOVE ea, SRP

#if TRACEMMU
            printf("PMOVE from %08X to SRP = %08X%08X\n", lAccessAddr,
                     vmPeekL(lAccessAddr), vmPeekL(lAccessAddr+4));
#endif
            vpregs2->SRP30[0] = vmPeekL(lAccessAddr) & 0xFFFF0003;
            vpregs2->SRP30[1] = vmPeekL(lAccessAddr+4) & 0xFFFFFFF3;
            break;

        case 5:
            // PMOVE SRP, ea

#if TRACEMMU
            printf("PMOVE SRP to %08X = %08X%08X\n", lAccessAddr,
                     vpregs2->SRP30[0], vpregs2->SRP30[1]);
#endif
            vmPokeL(lAccessAddr,   vpregs2->SRP30[0]);
            vmPokeL(lAccessAddr+4, vpregs2->SRP30[1]);
            break;

        case 6:
            // PMOVE ea, CRP

#if TRACEMMU
            printf("PMOVE from %08X to CRP = %08X%08X\n", lAccessAddr,
                     vmPeekL(lAccessAddr), vmPeekL(lAccessAddr+4));
#endif
            vpregs2->CRP30[0] = vmPeekL(lAccessAddr) & 0xFFFF0003;
            vpregs2->CRP30[1] = vmPeekL(lAccessAddr+4) & 0xFFFFFFFC;
            break;

        case 7:
            // PMOVE CRP, ea

#if TRACEMMU
            printf("PMOVE CRP to %08X = %08X%08X\n", lAccessAddr,
                     vpregs2->CRP30[0], vpregs2->CRP30[1]);
#endif
            vmPokeL(lAccessAddr,   vpregs2->CRP30[0]);
            vmPokeL(lAccessAddr+4, vpregs2->CRP30[1]);
            break;
            }

        // Check if the FD bit is 0, which means flush TLB

        if (0 == (xword & 0x0100))
            {
#if TRACEMMU
            printf("xword = %04X, FD is 0, clearing TLB\n", xword);
#endif
            InvalidatePages(0x00000000, 0xFFFFFFFF);
            }

        return 0;
        }

    if ((xword & 0xE0FF) == 0x0000)
        {
#if TRACEMMU
        printf("PMOVE, preg = %d, fRW = %d, addr = %08X\n",
             xword >> 1, xword & 1, lAccessAddr);
#endif

        switch ((xword >> 9) & 15)
            {
        default:
#if TRACEMMU
            printf("WARNING: unknown MMU instruction!\n");
#endif
            return 1;

        case 4:
            // PMOVE ea, TT0

            vpregs2->TT[0] = vmPeekL(lAccessAddr) & 0xFFFF8777;
#if TRACEMMU
            printf("new TT0 = %08X\n", vpregs2->TT[0]);
#endif
            break;

        case 5:
            // PMOVE TT0, ea

            vmPokeL(lAccessAddr, vpregs2->TT[0]);
            break;

        case 6:
            // PMOVE ea, TT1

            vpregs2->TT[1] = vmPeekL(lAccessAddr) & 0xFFFF8777;
#if TRACEMMU
            printf("new TT1 = %08X\n", vpregs2->TT[1]);
#endif
            break;

        case 7:
            // PMOVE TT1, ea

            vmPokeL(lAccessAddr, vpregs2->TT[1]);
            break;
            }

        // Check if the FD bit is 0, which means flush TLB

        if (0 == (xword & 0x0100))
            {
#if TRACEMMU
            printf("xword = %04X, FD is 0, clearing TLB\n", xword);
#endif
            InvalidatePages(0x00000000, 0xFFFFFFFF);
            }

        return 0;
        }

    if ((xword & 0xFDFF) == 0x6000)
        {
        if (xword & 0x0200)
            {
#if TRACEMMU
            printf("PMOVE MMUSR,ea = %04X\n", vpregs2->MMUSR);
#endif
            vmPokeW(lAccessAddr, (WORD)vpregs2->MMUSR);
            }
        else
            {
#if TRACEMMU
            printf("PMOVE ea,MMUSR = %04X\n", vmPeekW(lAccessAddr) & 0xEE47);
#endif
            vpregs2->MMUSR = vmPeekW(lAccessAddr) & 0xEE47;
            }

        return 0;
        }

    if ((xword & 0xE000) == 0x8000)
        {
#if TRACEMMU
        printf("PTEST\n");
#endif
        return 0;
        }

#if TRACEMMU
    printf("\n\n\nINVALID OR UNIMPLEMENTED 68030 MMU INSTRUCTION\n\n\n");
    m68k_DumpRegs();
    m68k_DumpHistory(160);
#endif

    vpregs->PC = vpregs->FPIAR;
    return 1;
}


//
// Called by the 68000 emulator when a software exception is about to occur.
// Return 1 to go ahead with the exception, -1 to go ahead and rewind PC,
// or 0 if we handle it or punt on it.
//

static Move16(ULONG addrDst, ULONG addrSrc)
{
    DWORD rgl[4];

    addrSrc &= 0xFFFFFFF0;
    addrDst &= 0xFFFFFFF0;

    ReadGuestLogicalLong(addrSrc +  0, &rgl[0]);
    ReadGuestLogicalLong(addrSrc +  4, &rgl[1]);
    ReadGuestLogicalLong(addrSrc +  8, &rgl[2]);
    ReadGuestLogicalLong(addrSrc + 12, &rgl[3]);

    WriteGuestLogicalLong(addrDst +  0, &rgl[0]);
    WriteGuestLogicalLong(addrDst +  4, &rgl[1]);
    WriteGuestLogicalLong(addrDst +  8, &rgl[2]);
    WriteGuestLogicalLong(addrDst + 12, &rgl[3]);
}

BOOL __cdecl FTryMove16(ULONG instr, long xword)
{
    if ((vmCur.bfCPU == cpu68040) || vi.fFake040)
        {
        if ((instr & 0xFFF8) == 0xF620)
            {
            ULONG *plreg = &vpregs->A0;
            ULONG Ax = instr & 7;
            ULONG Ay = (xword >> 28) & 7;
#if TRACE040
            printf("MOVE16 (A%d)+,(A%d)+\n", Ax, Ay);
#endif

            Move16(plreg[Ay], plreg[Ax]);

            plreg[Ax] += 16;
            plreg[Ay] += 16;

            vpregs->PC += 4;
            return 0;
            }

        if ((instr & 0xFFE0) == 0xF600)
            {
            ULONG *plreg = &vpregs->A0;
            ULONG lAddr = xword;
            BOOL fIncr = !(instr & 0x0010);
            BOOL fAddrFirst = !(instr & 0x0008);
            ULONG Ay = instr & 7;
#if TRACE040
            if (fAddrFirst)
                printf("MOVE16 (A%d)%s,%08X.l\n", Ay, fIncr ? "+" : "", lAddr);
            else
                printf("MOVE16 %08X.l,(A%d)%s\n", lAddr, Ay, fIncr ? "+" : "");
#endif

            if (fAddrFirst)
                {
                Move16(lAddr, plreg[Ay]);
                }
            else
                {
                Move16(plreg[Ay], lAddr);
                }

            if (fIncr)
                plreg[Ay] += 16;

            vpregs->PC += 6;
            return 0;
            }
        }

    return 1;
}

#endif // ATARIST


BOOL fDumpHist = 0;

BOOL fTracing = 0;

BOOL __cdecl TrapHook(int vector)
{
    ULONG SP = vpregs->A7;
    WORD instr, xword;
    extern long lAccessAddr;

#if TRACETRAP
    fDebug++;
    printf("TRAP %d: PC = %08X, SP = %08X\n",
        vector, vpregs->PC, SP);
    m68k_DumpRegs();
    fDebug--;
#endif

    switch (vector)
        {
    default:
#if TRACETRAP
        fDebug++;
        printf("UNKNOWN TRAP: PC = %08X, SP = %08X\n",
            vpregs->PC, SP);
        m68k_DumpRegs();
        fDebug--;
#endif
        break;

    case 33:        // TRAP #1 (GEMDOS)
        if (!FIsAtari68K(vmCur.bfHW))
            return 1;

        return CallGEMDOS(vpregs->PC,
            PeekW(SP),
            PeekW(SP+2),
            PeekW(SP+4),
            PeekW(SP+6),
            PeekW(SP+8),
            PeekW(SP+10),
            PeekW(SP+12),
            PeekW(SP+14)
            );

    case 34:        // TRAP #2
        if (!FIsAtari68K(vmCur.bfHW))
            return 1;

#if TRACETRAP
        if (vpregs->PC < 0x1A6FF)
            {
        fDebug++;
        printf("TRAP #2:   PC = %08X, SP = %08X, ea = %08X\n",
            vpregs->PC, SP, lAccessAddr);
        m68k_DumpRegs();
        m68k_DumpHistory(640);
        fDebug--;

     //     __asm int 3
            }
#endif
        return 1;

    case 2:         // BUS ERROR
        if (fDumpHist)
            {
       //   m68k_DumpHistory(MAX_HIST/16);
            }

#if TRACETRAP
        fDebug++;
        printf("BUS ERROR: PC = %08X, SP = %08X, ea = %08X\n",
            vpregs->PC, SP, lAccessAddr);
        m68k_DumpRegs();
        m68k_DumpHistory(640);
        fDebug--;
#endif

//        fDumpHist = 1;
        break;

    case 3:         // ADDRESS ERROR
        if (fDumpHist)
            {
       //   m68k_DumpHistory(MAX_HIST/16);
            }

#if TRACETRAP
        fDebug++;
        printf("ADDRESS ERROR. TRAP: PC = %08X, SP = %08X\n",
            vpregs->PC, SP);
        m68k_DumpHistory(6400);
        m68k_DumpRegs();
        fDebug--;
#endif
        break;

    case 4:         // ILLEGAL INSTRUCTION
        if (fDumpHist)
            {
       //   m68k_DumpHistory(MAX_HIST/16);
            }

        // Illegal instructions need to push the address of
        // the illegal instructions. The PC has already been
        // rewound by 2 bytes at this point. If we want the
        // illegal instruction to call the regular illegal
        // instruction vector, simply return 1. If we handle
        // it ourselvers, we need to increment the PC by 2
        // and return 0.

        instr = PeekW(vpregs->PC);

#if TRACETRAP && 0
            fDebug++;
            printf("ILLEGAL INSTR. TRAP: PC = %08X, SP = %08X, instr = %04X\n",
                vpregs->PC, SP, instr);
            m68k_DumpRegs();
            m68k_DumpHistory(640);
            fDebug--;
#endif
        switch(instr)
            {
        case 0x0E79:
            if (vmCur.bfCPU < cpu68020)
                return 1;

            if (PeekW(vpregs->PC + 2) == 0x1000)
                {
#if TRACETRAP
                printf("MOVES $xxxxxxxx,D1\n");
#endif

                vpregs->PC += 8;

                // REVIEW: fake a bus error??
                // vpregs->PC = PeekL(vpregs->VBR + 8);

                return 0;
                }
            goto Lunk;

        case 0x0CFC:
        case 0x0EFC:
#ifndef NDEBUG
            if (instr == 0x0EFC)
                printf("CAS2.L\n");
            else
                printf("CAS2.W\n");
#endif

            // CAS2 is illegal on the 68000/68010

            if (vmCur.bfCPU < cpu68020)
                return 1;

            {
            WORD xw1 = PeekW(vpregs->PC+2);
            WORD xw2 = PeekW(vpregs->PC+4);
            BOOL fL  = (instr & 0x0200);
            LONG *plreg = &vpregs->D0;
            ULONG Rn1, Rn2, Du1, Du2, Dc1, Dc2;

            Rn1 = (xw1 >> 12) & 15;
            Rn2 = (xw2 >> 12) & 15;
            Du1 = (xw1 >>  6) &  7;
            Du2 = (xw2 >>  6) &  7;
            Dc1 = (xw1 >>  0) &  7;
            Dc2 = (xw2 >>  0) &  7;

#ifndef NDEBUG
            printf("Rn1 = %c%d ", (Rn1 & 8) ? 'A' : 'D', Rn1 & 7);
            printf("%08X %08X ", plreg[Rn1], PeekL(plreg[Rn1]));
            printf("Du1 = D%d %08X ", Du1, plreg[Du1]);
            printf("Dc1 = D%d %08X\n", Dc1, plreg[Dc1]);
            printf("Rn2 = %c%d ", (Rn2 & 8) ? 'A' : 'D', Rn2 & 7);
            printf("%08X %08X ", plreg[Rn2], PeekL(plreg[Rn2]));
            printf("Du2 = D%d %08X ", Du2, plreg[Du2]);
            printf("Dc2 = D%d %08X\n", Dc2, plreg[Dc2]);
#endif

            vpregs->rgfsr.bitC = 0; //REVIEW: update these for real
            vpregs->rgfsr.bitV = 0;
            vpregs->rgfsr.bitN = 0;

            vpregs->rgfsr.bitZ = 0;

            if (fL)
                {
                if ((PeekL(plreg[Rn1]) == plreg[Dc1]) &&
                    (PeekL(plreg[Rn2]) == plreg[Dc2]))
                    {
                    PokeL(plreg[Rn1], plreg[Du1]);
                    PokeL(plreg[Rn2], plreg[Du2]);
                    vpregs->rgfsr.bitZ = 1;
                    }
                else
                    {
                    plreg[Dc1] = PeekL(plreg[Rn1]);
                    plreg[Dc2] = PeekL(plreg[Rn2]);
                    }
                }

            else // if (!fL)
                {
                if ((PeekW(plreg[Rn1]) == (WORD)plreg[Dc1]) &&
                    (PeekW(plreg[Rn2]) == (WORD)plreg[Dc2]))
                    {
                    PokeW(plreg[Rn1], (WORD)plreg[Du1]);
                    PokeW(plreg[Rn2], (WORD)plreg[Du2]);
                    vpregs->rgfsr.bitZ = 1;
                    }
                  else
                    {
                    plreg[Dc1] &= 0xFFFF0000;
                    plreg[Dc2] &= 0xFFFF0000;
                    plreg[Dc1] |= PeekW(plreg[Rn1]);
                    plreg[Dc2] |= PeekW(plreg[Rn2]);
                    }
                }

#ifndef NDEBUG
            printf("Z = %d\n", vpregs->rgfsr.bitZ);
#endif

            vpregs->PC += 6;
            return 0;
            }

        case 0x4E7A:
        case 0x4E7B:
#ifndef NDEBUG
            if (instr == 0x4E7A)
                printf("MOVEC Rc,Rn\n");
            else
                printf("MOVEC Rn,Rc\n");
#endif

            // MOVEC is illegal on the 68000

            if (vmCur.bfCPU < cpu68010)
                return 1;

            {
            WORD reg = PeekW(vpregs->PC+2);
            BOOL fDr = (instr & 1);
            ULONG *plreg = &vpregs->D0 + (reg >> 12);
            ULONG *plctr = plreg;        // pointer to control register
            ULONG wfMask = 0xFFFFFFFF;   // for registers < 32 bits

#ifndef NDEBUG
            printf("Rn = %c%d\n", (reg & 0x8000) ? 'A' : 'D', (reg >> 12) & 7);
#endif

            // control register greater than $007/$807 is illegal on all

            if ((reg & 0x7FF) > 7)
                return 1;

            // On the Mac, check for 68010 and limit reg to valid values.
            // On the Atari ST, don't do this check, TOS doesn't seem to handle 68010

            if (FIsMac(vmCur.bfHW))
                {
                // control register CAAR illegal on 68040

                if ((vmCur.bfCPU == cpu68040) && ((reg & 0xFFF) == 0x802))
                    return 1;

                // control register greater than $002/$804 illegal on 68020/68030

                if ((vmCur.bfCPU < cpu68040) && !vi.fFake040 &&
                   (((reg & 0x7FF) > 4) || ((reg & 0xFFF) == 3) || ((reg & 0xFFF) == 4)) )
                    return 1;

                // control register greater than $001/$801 is illegal on 68010

                if ((vmCur.bfCPU < cpu68020) && ((reg & 0x7FF) > 1))
                    return 1;

                switch(reg & 0xFFF)
                    {
                default:
                    break;

                // 68010 and up

                case 0x000:     // SFC
                    plctr = (ULONG *)&vpregs->SFC;
                    wfMask = 7;
                    break;

                case 0x001:     // DFC
                    plctr = (ULONG *)&vpregs->DFC;
                    wfMask = 7;
                    break;

                case 0x800:     // USP
                    plctr = &vpregs->USP;
                    break;

                case 0x801:     // VBR
                    plctr = &vpregs->VBR;
                    break;

                // 68020 and up

                case 0x002:     // CACR
                    plctr = &vpregs2->CACR;

                    if (vmCur.bfCPU == cpu68040)
                        vpregs2->CACR &= 0x80008000;

                    else if (vmCur.bfCPU == cpu68030)
                        vpregs2->CACR &= 0x00003313;

                    else // if (vmCur.bfCPU == cpu68020)
                        vpregs2->CACR &= 0x00000003;
                    break;

                case 0x802:     // CAAR
                    Assert (vmCur.bfCPU != cpu68040);

                    plctr = &vpregs2->CAAR;
                    break;

                case 0x803:     // MSP
                    plctr = &vpregs->MSP;
                    break;

                case 0x804:     // ISP
                    plctr = &vpregs->SSP;   // HACK, MSP not used, ISP is always SSP
                    break;

                // 68040 only

                case 0x003:     // TCR
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 TCR!!!\n");
#endif
                    plctr = &vpregs2->TCR;
                    break;

                case 0x004:     // ITT0
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 ITT0!!!\n");
#endif
                    plctr = &vpregs2->ITT[0];
                    break;

                case 0x005:     // ITT1
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 ITT1!!!\n");
#endif
                    plctr = &vpregs2->ITT[1];
                    break;

                case 0x006:     // DTT0
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 DTT0!!!\n");
#endif
                    plctr = &vpregs2->DTT[0];
                    break;

                case 0x007:     // DTT1
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 DTT1!!!\n");
#endif
                    plctr = &vpregs2->DTT[1];
                    break;

                case 0x805:     // MMUSR
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 MMUSR!!!\n");
#endif
                    plctr = &vpregs2->MMUSR;
                    break;

                case 0x806:     // URP
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 URP!!!\n");
#endif
                    plctr = &vpregs2->URP40;
                    break;

                case 0x807:     // SRP
#if TRACE040
                    m68k_DumpRegs();
                    m68k_DumpHistory(160);
                    printf("68040 SRP!!!\n");
#endif
                    plctr = &vpregs2->SRP40;
                    break;
                    }

                DebugStr("Handling MOVEC!!!\n");

                if (instr & 1)
                    *plctr ^= (*plreg ^ *plctr) & wfMask;
                else
                    *plreg = *plctr & wfMask;
                }

            vpregs->PC += 4;
            return 0;
            }

        case 0x7FF2:        // Monster (3.x)
            if (!FIsAtari68K(vmCur.bfHW))
                return 1;

            vsthw.VESAmode = 0x12;
            vpregs->PC += 2;
            EnterMode(1);
            return 0;

        case 0x7FF3:        // Monster (4.x)
            if (!FIsAtari68K(vmCur.bfHW))
                return 1;

            vsthw.VESAmode = vpregs->D4;
            vpregs->PC += 2;
            EnterMode(1);
            return 0;

        default:
Lunk:
#if !defined(NDEBUG)
            printf("UNKNOWN ILLEGAL %04X at %08X!!\n", instr, vpregs->PC);
#endif

#if TRACETRAP
            fDebug++;
            printf("ILLEGAL INSTR. TRAP: PC = %08X, SP = %08X, instr = %04X\n",
                vpregs->PC, SP, instr);
            m68k_DumpRegs();
#if 0
    {
    int i;

    for (i = 0; i <= 0x400; i++)
        {
        if ((i & 15) == 0)
            printf("\n%08X:", i);
        printf(" %02X", PeekB(i));
        }
    }
#endif
            m68k_DumpHistory(4000);
            fDebug--;
#endif
//            __asm { int 3 } ;
            break;
            }

        break;

    case 5:         // Divide by zero

#if TRACETRAP
        fDebug++;
        printf("Peek(PC-2) = %08X\n", PeekL(vpregs->PC-4));
        printf("Peek(D00) = %04X\n", PeekW(0xD00));
        printf("DIV 0: PC = %08X, SP = %08X\n",
            vpregs->PC, SP);
        m68k_DumpHistory(1600);
        m68k_DumpRegs();
        fDebug--;

        fTracing = 1;
#endif

        if (FIsMac(vmCur.bfHW) && (vmCur.bfHW >= vmMacSE))
            {
            // On fast machines the code that calculates the
            // DBRA timing loops will overflow and cause a
            // divide by zero. So this checks for the
            // DIVU D1,D2 SUB.W D2,D3 sequence.

            if (PeekW(0xD00) == 0)
                PokeW(0xD00, 0xFFFE);

#if 0
printf("PC: %08X  @PC-2: %08X\n", vpregs->PC, PeekL(vpregs->PC-2));
            __asm { int 3 } ;
#endif

            if (PeekL(vpregs->PC-2) == 0x84C19642)
                {
                vmachw.fVIA2 = TRUE;
                return 0;
                }

            // This is DIVU D5,D1 MOVE.W D1,D0

            if (PeekL(vpregs->PC-2) == 0x82C53001)
                {
                vmachw.fVIA2 = TRUE;
                return 0;
                }

            // post-IIci 512K and IIsi ROMs always DIV0 on $D00
            // at $40843A90 and are quite happy to just skip it.

            if (vpregs->PC == 0x40843A90)
                {
                vmachw.fVIA2 = TRUE;
                return 0;
                }
            }

        return 1;
        break;

    case 10:        // LINE A / Mac toolbox

        // Same goes for Line A as for illegal instructions.
        // If we want to execute the regular Line A trap,
        // just return 1. If we handle the trap, we need to
        // increment the PC by 2 then return 0.

#if TRACETRAP
        {
        if (0xA9EB == PeekW(vi.pregs->PC))
            {
            extern ULONG lBreakpoint;

         //   fTracing++;
            lBreakpoint = vpregs->PC + 2;
        printf("\n\nSTARTING FPU TRACE!!!\n\n");
            }

#if 0
        if ((0xA9F4 == PeekW(vi.pregs->PC)) ||
            (0xA9C9 == PeekW(vi.pregs->PC)))
            {
            m68k_DumpHistory(40000);
            m68k_DumpRegs();
            exit(0);
            }

        if (0)
#endif
            {
            ULONG instr = PeekW(vi.pregs->PC);

            switch(instr)
                {
            default:
#if 0
            case 0xA058:
            case 0xA059:
            case 0xA05A:
#endif
                printf("A TRAP: PC = %08X, SP = %08X\n", vpregs->PC, SP);
                m68k_DumpHistory(160);
                m68k_DumpRegs();
                break;

#if 0
            default:
            case 0xA02E:
            case 0xAAFE:
#endif
                break;
                }
            }
        }
#endif

#ifdef SOFTMAC
        if (FIsMac(vmCur.bfHW))
            return CallMacLineA();
#endif

        return 1;
        break;

    case 11:        // LINE F / MMU / cp

        // Ditto for Line F as for Line A.
        // In 68000/68010 mode, PC points at start of instruction.
        // Return 1 to execute the normal Line F vector
        // or increment the PC by 2 (or the size of the
        // whole Line F instructions) and return 0.
        //
        // In 68020+ mode, FPIAR has the address of the instruction,
        // Xword has the extension word, and the PC has decoded
        // any effective address. Return 0 if handled, or set the
        // PC to the value in FPIAR and return 1 to execute Line F vector.

#if TRACETRAP // || TRACEMMU  || TRACEFPU
        printf("F TRAP: FPIAR = %08X, PC = %08X, SP = %08X\n",
            vpregs->FPIAR, vpregs->PC, SP);
        {
        ULONG temp = vpregs->PC;
        vpregs->PC = vpregs->FPIAR;
        m68k_DumpHistory(80);
        m68k_DumpRegs();
        vpregs->PC = temp;
        }
#endif

        // Line F are all illegal on the 68000 and 68010

        if (vmCur.bfCPU < cpu68020)
            return 1;

        instr = PeekW(vpregs->FPIAR);
        xword = PeekW(vpregs->FPIAR+2);

#if TRACETRAP
        printf("instr = %04X, %04X\n", instr, xword);
        printf("mode = %d\n", ((instr >> 9) & 7));
#endif

        if (((instr >> 9) & 7) == 1)
            {
            // MC6888x co-processor

            return F68KFPU(instr);
            }

#if TRACETRAP
        printf("NOT AN FPU!!!\n");
#endif

#ifndef NDEBUG
        m68k_DumpHistory(160);
        {
        ULONG temp = vpregs->PC;
        vpregs->PC = vpregs->FPIAR;
        m68k_DumpRegs();
        vpregs->PC = temp;
        }
#endif

        if ((vmCur.bfCPU < cpu68030) && !vi.fFake040)
            return 1;

        if ((vmCur.bfCPU == cpu68030) && ((instr & 0xFFC0) == 0xF000))
            {
            // 68030 MMU

            return F030MMU();
            }

        // 68040 specific instructions

        // Do MOVE16 separately so that we can fake it from 68030 mode

        if (!FTryMove16(instr, PeekL(vpregs->FPIAR+2)))
            return 0;

        if (vmCur.bfCPU == cpu68040)
            {
#if TRACE040
            m68k_DumpRegs();
            m68k_DumpHistory(160);
            fTracing = 1600000;

            printf("68040 ");
#endif

            if ((instr & 0xFF20) == 0xF400)
                {
#if TRACE040
                printf("CINV\n");
                fTracing = 1000;
#endif
                InvalidatePages(0x00000000, 0xFFFFFFFF);

                vpregs->PC += 2;
                return 0;
                }

            if ((instr & 0xFF20) == 0xF420)
                {
#if TRACE040
                printf("CPUSH\n");
                fTracing = 10;
#endif
                InvalidatePages(0x00000000, 0xFFFFFFFF);

                vpregs->PC += 2;
                return 0;
                }

            if ((instr & 0xFFE0) == 0xF500)
                {
#if TRACE040
                printf("PFLUSH\n");
                fTracing = 10;
#endif
                vpregs->PC += 2;
                return 0;
                }

            if ((instr & 0xFFD8) == 0xF548)
                {
                ULONG *plreg = &vpregs->D0 + (instr & 15);
#if TRACE040
                printf("PTEST (A%d)\n", instr & 7);
                fTracing = 10;
#endif
                vpregs->PC += 2;

                vpregs2->MMUSR = *plreg & 0xFFFFF000;
                return 0;
                }

            if ((instr & 0xFFFF) == 0xFE0A)
                {
                // PowerPC ROMs use this
#if TRACE040
                printf("$FE0A\n");
                fTracing = 10;
#endif
                vpregs->PC += 2;

                return 0;
                }
            }

        return 1;
        }

    return 1;
}


__int64 lTicksStep;

int fTraced = 0;

ULONG __cdecl StepHook(ULONG esi)
{
    lTicksStep++;

//    if (vpregs->A7 != 0x40000)
        {
        vi.pHistory[vi.iHistory++] = vpregs->PC +
                 ((vsthw.lAddrMask & 0x80000000) ? 1 : 0);
        vi.pHistory[vi.iHistory++] = vpregs->D0;
        vi.pHistory[vi.iHistory++] = vpregs->D1;
        vi.pHistory[vi.iHistory++] = vpregs->D2;
        vi.pHistory[vi.iHistory++] = vpregs->D3;
        vi.pHistory[vi.iHistory++] = vpregs->D4;
        vi.pHistory[vi.iHistory++] = vpregs->D5;
        vi.pHistory[vi.iHistory++] = vpregs->D6;
        vi.pHistory[vi.iHistory++] = vpregs->D7;
        vi.pHistory[vi.iHistory++] = vpregs->A0;
        vi.pHistory[vi.iHistory++] = vpregs->A1;
        vi.pHistory[vi.iHistory++] = vpregs->A2;
        vi.pHistory[vi.iHistory++] = vpregs->A6;
        vi.pHistory[vi.iHistory++] = (vpregs->A7 & 0x00FFFFFE) |
             !!(vpregs->SR & 0x2000) |
             ((vpregs->SR & 0x010) << 27) |
             ((vpregs->SR & 0x700) << 20) | ((vpregs->SR & 0x00F) << 24);

        if (vpregs->A7 + 8 < vi.cbRAM[0])
            {
            vi.pHistory[vi.iHistory++] = PeekL(vpregs->A7);
            vi.pHistory[vi.iHistory++] = PeekL(vpregs->A7+4);
            }
        else
            {
            vi.pHistory[vi.iHistory++] = 0;
            vi.pHistory[vi.iHistory++] = 0;
            }
        vi.iHistory &= MAX_HIST-1;
        }

    if (vi.pProfile)
        {
// printf("adding %04X to profile\n", PeekW(vpregs->PC));
        vi.pProfile[PeekW(vpregs->PC)]++;
        vi.cFreq++;
        }

//    m68k_DumpRegs();

    return 1;
}


void DumpRTE(ULONG ea, ULONG val)
{
#if TRACETRAP
//	if (!fDebug)
//		return;

#if 0
	if (ea >= 0xE00000)
		return;
#endif

    if ((PeekW(ea-2) & 0xFFF0) != 0x4E40)
        return;
    if ((PeekW(ea-2)) == 0x4E42)
        return;
    if (ea == 0xE21A4A)
        return;
    if (ea == 0xE0FA24)
        return;
    if (ea == 0xE071AE)
        return;

	DebugStr("RTE to location %08X returning %08X, SSP = %08X, USP = %08X, D0 = %08X\n",
		ea, val, vpregs->SSP, vpregs->USP, vpregs->D0);
#endif
}


void m68k_ClearProfile()
{
	ULONG *pl = vi.pProfile;
	int cl = 65536;

	while (cl--)
		*pl++ = 0x00;
}


BOOL __cdecl m68k_DumpRegs(void)
{
	if ((vpregs == 0) || (vi.cbRAM[0] == 0))
		return 0;

#ifdef POWERMAC
    if (vmCur.bfCPU >= cpu601)
        return mppc_DumpRegs();
#endif

	printf("DATA %08X %08X ", vpregs->D0, vpregs->D1);
	printf("%08X %08X ", vpregs->D2, vpregs->D3);
	printf("%08X %08X ", vpregs->D4, vpregs->D5);
	printf("%08X %08X\n", vpregs->D6, vpregs->D7);
	printf("ADDR %08X %08X ",  vpregs->A0, vpregs->A1);
	printf("%08X %08X ",  vpregs->A2, vpregs->A3);
	printf("%08X %08X ",  vpregs->A4, vpregs->A5);
	printf("%08X %08X\n", vpregs->A6, vpregs->A7);

    if (vmCur.bfCPU > cpu68000)
        {
        printf("VBR  %08X ", vpregs->VBR);
        printf("SFC/DFC  %08X %08X ", vpregs->SFC, vpregs->DFC);
        printf("CACR/AAR %08X %08X ", vpregs2->CACR, vpregs2->CAAR);
        printf("%02d-bit addr\n", (vsthw.lAddrMask & 0xFF000000) ? 32 : 24);
        }

    if (vmCur.bfCPU >= cpu68030)
        {
        printf("MMU: %08X ", vpregs2->TCR);
        printf("%08X %08X ", vpregs2->MMUSR, vpregs2->CACR);

        printf("\n");

        if (vmCur.bfCPU >= cpu68040)
            {
            printf("MM40 ");
            printf("%08X %08X ", vpregs2->DTT[0], vpregs2->DTT[1]);
            printf("%08X %08X ", vpregs2->ITT[0], vpregs2->ITT[1]);
            printf("%08X %08X ", vpregs2->SRP40, vpregs2->URP40);
            }
        else
            {
            printf("MM30 ");
            printf("%08X %08X ", vpregs2->TT[0], vpregs2->TT[1]);
            printf("%08X %08X ", vpregs2->SRP30[0], vpregs2->SRP30[1]);
            printf("%08X %08X ", vpregs2->CRP30[0], vpregs2->CRP30[1]);
            }

        printf("\n");
        }

#if TRACEFPU
    if (vmCur.bfCPU >= cpu68020)
        {
        int i;

        printf("FPU  ");

        for (i = 0; i < 8; i++)
            {
            unsigned char *pvSrc = &vpregs2->rgbFPx[i * 16];
            double d;
            BYTE rgch[256];

            __asm mov   eax,pvSrc
            __asm fld   tbyte ptr [eax]
            __asm lea   eax,d
            __asm fstp  qword ptr [eax]

#if !defined(NDEBUG)
            sprintf(rgch, "%g ", d);
            printf(rgch);
#endif
            }
        printf("CR: %04X SR: %08X\n", vpregs->FPCR, vpregs->FPSR);
        }

    if (0)
        {
        // Dump top 64 bytes of Mac RAM (boot globs)

        int i;

        for (i = -64; i < 0; i+= 4)
            printf("%08X ", PeekL(vi.cbRAM[0] + i));

        printf("\n");

#if 0
        printf("VIA vDirB = %02X\n", vmachw.via1.vDirB);
        printf("VIA vBufB = %02X\n", vmachw.via1.vBufB);
#endif
        }
#endif

    if ((vpregs->A7 & 0x80000000) == 0)
    {
    int i;

    // Display the first few longs on the stack, but only if A7 is legal

    printf("stk: ");

    for (i = 0; i < 32; i += 4)
        {
        if (vpregs->A7 + i + 4 < vi.cbRAM[0])
            printf("%08X ",  PeekL(vpregs->A7 + i));
        }
    printf("\n");
    }

	printf("PC   %08X:%04X     ", vpregs->PC, PeekW(vpregs->PC));
	if (vpregs->rgfsr.bitSuper)
		printf("USP %08X      ", vpregs->USP);
	else
		printf("SSP %08X      ", vpregs->SSP);
//	printf("count: %09d  ", vpregs->count);
	printf("Flags: T%d S%d I%d %c %c %c %c %c  ",
	                                  vpregs->rgfsr.bitTrace,
	                                  vpregs->rgfsr.bitSuper,
	                                  vpregs->rgfsr.bitsI,
	                                  vpregs->rgfsr.bitX ? 'X' : '_',
	                                  vpregs->rgfsr.bitN ? 'N' : '_',
	                                  vpregs->rgfsr.bitZ ? 'Z' : '_',
	                                  vpregs->rgfsr.bitV ? 'V' : '_',
	                                  vpregs->rgfsr.bitC ? 'C' : '_');

    printf("\n");

	CDis(vpregs->PC, TRUE);

#if 0
	printf("\n");
	printf("countR: %09d  countW: %09d\n", vi.countR, vi.countW);
#endif
	printf("\n");

    return TRUE;
}


#ifdef POWERMAC

#if TRACEPPC
ULONG cnt;
#endif

BOOL __cdecl mppc_DumpRegs(void)
{
	if ((vpregs == 0) || (vi.cbRAM[0] == 0))
		return 0;

#if TRACEPPC
    printf("cnt = %d\n", cnt);
#endif

	printf("R0  %08X %08X ", vpregs->rggpr[0], vpregs->rggpr[1]);
	printf("%08X %08X ", vpregs->rggpr[2], vpregs->rggpr[3]);
	printf("%08X %08X ", vpregs->rggpr[4], vpregs->rggpr[5]);
	printf("%08X %08X ", vpregs->rggpr[6], vpregs->rggpr[7]);
#if TRACEPPC
#else
    printf("\n");
#endif
	printf("R8  %08X %08X ", vpregs->rggpr[8], vpregs->rggpr[9]);
	printf("%08X %08X ", vpregs->rggpr[10], vpregs->rggpr[11]);
	printf("%08X %08X ", vpregs->rggpr[12], vpregs->rggpr[13]);
	printf("%08X %08X\n", vpregs->rggpr[14], vpregs->rggpr[15]);
	printf("R16 %08X %08X ", vpregs->rggpr[16], vpregs->rggpr[17]);
	printf("%08X %08X ", vpregs->rggpr[18], vpregs->rggpr[19]);
	printf("%08X %08X ", vpregs->rggpr[20], vpregs->rggpr[21]);
	printf("%08X %08X ", vpregs->rggpr[22], vpregs->rggpr[23]);
#if TRACEPPC
#else
    printf("\n");
#endif
	printf("R24 %08X %08X ", vpregs->rggpr[24], vpregs->rggpr[25]);
	printf("%08X %08X ", vpregs->rggpr[26], vpregs->rggpr[27]);
	printf("%08X %08X ", vpregs->rggpr[28], vpregs->rggpr[29]);
	printf("%08X %08X\n", vpregs->rggpr[30], vpregs->rggpr[31]);

	printf("SR0 %08X %08X ", vpregs->rgsgr[0], vpregs->rggpr[1]);
	printf("%08X %08X ", vpregs->rgsgr[2], vpregs->rgsgr[3]);
	printf("%08X %08X ", vpregs->rgsgr[4], vpregs->rgsgr[5]);
	printf("%08X %08X ", vpregs->rgsgr[6], vpregs->rgsgr[7]);
#if TRACEPPC
#else
    printf("\n");
#endif
	printf("SR8 %08X %08X ", vpregs->rgsgr[8], vpregs->rggpr[9]);
	printf("%08X %08X ", vpregs->rgsgr[10], vpregs->rgsgr[11]);
	printf("%08X %08X ", vpregs->rgsgr[12], vpregs->rgsgr[13]);
	printf("%08X %08X\n", vpregs->rgsgr[14], vpregs->rgsgr[15]);

#if TRACEFPU
    if (vmCur.bfCPU >= cpu601)
        {
        int i;

        for (i = 0; i < 32; i++)
            {
            unsigned char *pvSrc = &vpregs->rgfpr[i];
            double d;
            BYTE rgch[256];

            if (!(i & 7))
                printf("FPU%2d ", i);

            __asm mov   eax,pvSrc
            __asm fld   qword ptr [eax]
            __asm lea   eax,d
            __asm fstp  qword ptr [eax]

            sprintf(rgch, "%g ", d);
            printf(rgch);

            if ((i & 7) == 7)
                printf("\n");

            }
        printf("FPCR: %08X\n", vpregs->regFPCR);
        }
#endif

	printf("MQ  %08X ", vpregs->regMQ);
	printf("     DEC %08X ", vpregs->regDEC);
	printf("     XER %08X ", vpregs->regXER);
	printf("     LR  %08X ", vpregs->regLR);
	printf("     CTR %08X ", vpregs->regCTR);

#if TRACEPPC
    printf("");
#else
    printf("\n");
#endif

	printf("CR  %08X ", vpregs->regCR);

	printf(" %c %c %c %c  %c %c %c %c ",
          (vpregs->regCR & 0x80000000) ? 'L' : '_',
          (vpregs->regCR & 0x40000000) ? 'G' : '_',
          (vpregs->regCR & 0x20000000) ? 'Z' : '_',
          (vpregs->regCR & 0x10000000) ? 'O' : '_',
          (vpregs->regCR & 0x08000000) ? 'F' : '_',
          (vpregs->regCR & 0x04000000) ? 'E' : '_',
          (vpregs->regCR & 0x02000000) ? 'V' : '_',
          (vpregs->regCR & 0x01000000) ? 'O' : '_');

	printf(" %c %c %c %c  %c %c %c %c ",
          (vpregs->regCR & 0x00800000) ? 'L' : '_',
          (vpregs->regCR & 0x00400000) ? 'G' : '_',
          (vpregs->regCR & 0x00200000) ? 'Z' : '_',
          (vpregs->regCR & 0x00100000) ? 'O' : '_',
          (vpregs->regCR & 0x00080000) ? 'L' : '_',
          (vpregs->regCR & 0x00040000) ? 'G' : '_',
          (vpregs->regCR & 0x00020000) ? 'Z' : '_',
          (vpregs->regCR & 0x00010000) ? 'O' : '_');

	printf(" %c %c %c %c  %c %c %c %c ",
          (vpregs->regCR & 0x00008000) ? 'L' : '_',
          (vpregs->regCR & 0x00004000) ? 'G' : '_',
          (vpregs->regCR & 0x00002000) ? 'Z' : '_',
          (vpregs->regCR & 0x00001000) ? 'O' : '_',
          (vpregs->regCR & 0x00000800) ? 'L' : '_',
          (vpregs->regCR & 0x00000400) ? 'G' : '_',
          (vpregs->regCR & 0x00000200) ? 'Z' : '_',
          (vpregs->regCR & 0x00000100) ? 'O' : '_');

	printf(" %c %c %c %c  %c %c %c %c\n",
          (vpregs->regCR & 0x00000080) ? 'L' : '_',
          (vpregs->regCR & 0x00000040) ? 'G' : '_',
          (vpregs->regCR & 0x00000020) ? 'Z' : '_',
          (vpregs->regCR & 0x00000010) ? 'O' : '_',
          (vpregs->regCR & 0x00000008) ? 'L' : '_',
          (vpregs->regCR & 0x00000004) ? 'G' : '_',
          (vpregs->regCR & 0x00000002) ? 'Z' : '_',
          (vpregs->regCR & 0x00000001) ? 'O' : '_');

#if 0
	printf("PC %08X:%08X  ", vpregs->PC, PeekL(vpregs->PC));

    {
    int i;

    // Display the first few longs on the stack, but only if A7 is legal

    for (i = 0; i < 44; i += 4)
        {
        if (vpregs->A7 + i + 4 < vi.cbRAM[0])
            printf("%08X ",  PeekL(vpregs->A7 + i));
        }
    printf("\n");
    }
#endif

	CDis(vpregs->PC, TRUE);

	printf("\n");

    return TRUE;
}

ULONG __cdecl mppc_GetCpus(void)
{
    return cpu601;
}

ULONG __cdecl mppc_GetCaps(void)
{
    return 0;
}

ULONG __cdecl mppc_GetVers(void)
{
    return 0;
}

ULONG __cdecl mppc_GetPageBits(void)
{
    return 16;
}

// #pragma optimize("",off)

#include "asmppc.h"

#if 0
#define DecodePpcField(reg, code, start, end, mask) \
    { \
    if (idef.wfDis & (mask)) \
        { \
        reg = (code) >> (31 - (end)); \
        reg &= (1 << (32 - (start))) - 1; \
        idef.wfDis &= ~(mask); \
        } \
    }
#endif


#define PrepareForArithUop()   \
        tmpA = vpregs->EBP;     \
        tmpB = vpregs->EBX;     \
        tmpC = vpregs->regXER;  \
      __asm mov     eax,tmpA    \
      __asm mov     ebx,tmpB ;

#define PostProcrArithUop()    \
      __asm pushfd              \
      __asm pop     eax         \
      __asm mov     tmpB,ebx    \
      __asm mov     tmpC,eax ;  \

#define PostProcrArithUop2()    \
        vpregs->EBX = tmpB;     \
 \
        if (idef.wfCPU & wfSetCA)  \
            {                       \
            if (tmpC & 0x0001)                  \
                vpregs->regXER |=  0x20000000;  \
            else                                \
                vpregs->regXER &= ~0x20000000;  \
            }                                   \
 \
        if ((idef.wfCPU & wfChkOE) && (EBXSav & bitOE)) \
            {                                           \
            if (tmpC & 0x0800)                  \
                vpregs->regXER |=  0xC0000000;  \
            else                                \
                vpregs->regXER &= ~0x40000000;  \
            }                                   \
 \
        if ((idef.wfCPU & wfSetRc) ||           \
           ((idef.wfCPU & wfChkRc) && (EBXSav & bitRc))) \
            {                                   \
            vpregs->regCR &= 0x0FFFFFFF;        \
            if (tmpC & 0x0800)                  \
                vpregs->regCR |= 0x10000000;    \
            if (vpregs->EBX == 0)               \
                vpregs->regCR |= 0x20000000;    \
            else if (vpregs->EBX & 0x80000000)  \
                vpregs->regCR |= 0x80000000;    \
            else                                \
                vpregs->regCR |= 0x40000000;    \
            }


ULONG gen_maskg(ULONG lMBME)
{
    ULONG MB = lMBME >> 5;
    ULONG ME = lMBME & 31;
    ULONG l;

#if TRACEPPC
    printf("maskg: mb = %d, me = %d, ", MB, ME);
#endif

    if (MB == ((ME+1) & 31))
        l = M1;

    else if (MB < (ME+1))
        l = StuffBits(MB,ME,M1);

    // mask wraps around

    else
        l = M1 - StuffBits(ME+1,MB-1,M1);

#if TRACEPPC
    printf("returning %08X\n", l);
#endif

    return l;
}

#ifdef OLDPPC

static rgbTouched[1024*1024];

ULONG __cdecl mppc_StepPPC(int count)
{
    IDEF idef;
    BOOL ok;
    ULONG EBXSav, MBMESav;
    int  i;
    static BOOL fTrace = 0;

    if (PeekL(vi.eaROM[0]) == 0x9A5DC01F)
        {
        // 7100 ROMs

        // HACK! for PowerPC ROMs to skip ROM checksum code

        // skip dec loop on CTR

        if ((vpregs->PC == 0x40B035C0) && (vpregs->regCTR == 0x80000))
            vpregs->PC += 4;

        // skip checksum check

        if (vpregs->PC == 0x40B03030)
            vpregs->PC += 12;

        // some kind of memory test at $51000000 and $52000000

        if (vpregs->PC == 0x40303298)
            {
            printf("CHECKPOINT AT $40303298\n");
            vpregs->PC = 0x40303428;
            }

        if (vpregs->PC == 0x403032B8)
            {
            printf("CHECKPOINT AT $403032B8\n");
            vpregs->PC = 0x40303428;
            }

        // some sort of loop to page in memory?

        if (vpregs->PC == 0x40310654)
            {
            printf("CHECKPOINT AT $40310654\n");
            vpregs->PC += 12;
            }

        if (vpregs->PC == 0x40311D48) // mfsrin
            printf("CHECKPOINT AT $40311D48 (mfsrin)\n");

        if (vpregs->PC == 0x403132DC)
            printf("CHECKPOINT AT $403132DC (lmw)\n");

        if (vpregs->PC == 0x4031330C)
            {
            printf("CHECKPOINT AT $4031330C (mtcrf)\n");
            fTrace++;
            }

        if (vpregs->PC == 0x403105B8)
            {
            printf("CHECKPOINT AT $403105B8 (stfd)\n");
            fTrace++;
            }

        if (vpregs->PC == 0x403032B8)
            fTrace++;

        if (vpregs->PC == 0x40303420)
            fTrace++;

        if ((vpregs->PC >= 0x40303420) && (vpregs->PC < 0x40303428) || fTrace)
     ;//        mppc_DumpRegs();
        }

    else if (PeekL(vi.eaROM[0]) == 0x9FEB69B3)
        {
        // 6100 ROMs

        // HACK! for PowerPC ROMs to skip ROM checksum code

        // skip dec loop on CTR

        if ((vpregs->PC == 0x40B034A4) && (vpregs->regCTR == 0x80000))
            vpregs->PC += 4;

        // skip checksum check

        if (vpregs->PC == 0x40B0300C)
            vpregs->PC += 12;

        }

    if (!rgbTouched[(vpregs->PC & (vi.cbROM[0] - 1)) >> 2])
        {
        rgbTouched[(vpregs->PC & (vi.cbROM[0] - 1)) >> 2] = 1;
        mppc_DumpRegs();
        }

    // We use the full 32-bit instruction word to
    // decode the instruction at jit time
    // and we use EAX and EBX to simulate what is
    // actually happening at runtime

    vpregs->EAX = (WORD)PeekW(vpregs->PC);
    vpregs->EBX = (WORD)PeekW(vpregs->PC+2);
    vpregs->EBP = 0xCCCCCCCC;

    ok = FindInstrDef((vpregs->EAX << 16) | (vpregs->EBX), &idef);

    if (!ok)
        return 1;

#if 0
    if (memcmp((void *)&idef,
        (void *)vpregs->opcodesRW[(vpregs->EAX << 16) | (vpregs->EBX)],
        sizeof(IDEF)))
        {
        printf("ERROR: rgopcodes[opcode] does not match FindInstrDef!!!\n");
        }
#endif

#if TRACEPPC
    cnt++;

    printf("valid PPC instruction %08X\n", (vpregs->EAX << 16) | (vpregs->EBX));
#endif

    vpregs->PC += 4;
    vpregs->regDEC -= 16;   // 16 nano seconds = 1 clock cycle

    // Check the LK flag first

    if ((idef.wfCPU & wfChkLK) && (vpregs->EBX & 1))
        vpregs->regLR = vpregs->PC;

    if (idef.wfCPU & (wfChkOE | wfChkRc))
        EBXSav = vpregs->EBX;
    else
        EBXSav = 0xCCCCCCCC;

    // To save uops, we check the wfDecode flag to
    // see if we can do a quick and dirty decode

//    if (idef.wfCPU & wfDecode)
    if (idef.wfDis)
        {
        // Decode the fields in the instruction as per wfDis
        //
        // Bits 16+ (typically rB or SIMM) end up in EBX
        // Bits 11..15 (typically rA) end up in EBP
        // Bits 6..10 (typically rD) end up in EAX
        //
        // Since the secondary source is always used as is we
        // go ahead and just load it all the way

        if (idef.wfDis & (wf_MB | wf_ME))
            {
            MBMESav = (vpregs->EBX >> 1) & 1023;
            idef.wfDis &= ~(wf_MB | wf_ME);
            }

        if (idef.wfDis & wf_SH)
            {
            vpregs->EBX >>= 11;
            vpregs->EBX &= 31;
            idef.wfDis &= ~wf_SH;
            }

        if (idef.wfDis & wf_dIMM)
            {
            vpregs->EBX = (short)(vpregs->EBX);
            idef.wfDis &= ~wf_dIMM;
            }

        if (idef.wfDis & wf_SIMM)
            {
            vpregs->EBX = (short)(vpregs->EBX);
            idef.wfDis &= ~wf_SIMM;
            }

        if (idef.wfDis & wf_UIMM)
            {
            vpregs->EBX = (unsigned short)(vpregs->EBX);
            idef.wfDis &= ~wf_UIMM;
            }

        if (idef.wfDis & wf_REL26)
            {
            vpregs->EBX = (vpregs->EBX & 65532) | ((vpregs->EAX & 1023) << 16);
            vpregs->EBX = ((((int)vpregs->EBX) << 6) >> 6);

            idef.wfDis &= ~wf_REL26;
            }

        if (idef.wfDis & wf_REL16)
            {
            vpregs->EBX = (signed short)(vpregs->EBX & 65532);

            idef.wfDis &= ~wf_REL16;
            }

        if (idef.wfDis & wf_CRM)
            {
            vpregs->EBP = ((vpregs->EAX & 15) << 4) | (vpregs->EBX >> 12);
printf("decoded CRM as %d\n", vpregs->EBP);
            idef.wfDis &= ~wf_CRM;
            }

        if (idef.wfDis & wf_SGR)
            {
            vpregs->EBP = vpregs->EAX;
            vpregs->EBP = vpregs->EBP & 15;
            idef.wfDis &= ~wf_SGR;
            }

        if (idef.wfDis & wf_SPR)
            {
            vpregs->EBP = (vpregs->EAX & 31) | ((vpregs->EBX >> 11) << 5);
printf("decoded SPR as %d\n", vpregs->EBP);
            idef.wfDis &= ~wf_SPR;
            }

        if (idef.wfDis & wf_BI)
            {
            vpregs->EBP = vpregs->EAX;
            vpregs->EBP = vpregs->EBP & 31;

            idef.wfDis &= ~wf_BI;
            }

        if (idef.wfDis & wf_BO)
            {
            vpregs->EAX = vpregs->EAX >> 5;
            vpregs->EAX = vpregs->EAX & 31;

            idef.wfDis &= ~wf_BO;
            }

        if (idef.wfDis & wf_rB)
            {
            vpregs->EBX = vpregs->EBX >> 11;
            vpregs->EBX = vpregs->EBX & 31;
            vpregs->EBX = vpregs->rggpr[vpregs->EBX];

            idef.wfDis &= ~wf_rB;
            }

        if (idef.wfDis & (wf_rS | wf_fS))
            {
            vpregs->EBP = vpregs->EAX;
            vpregs->EBP = vpregs->EBP >> 5;
            vpregs->EBP = vpregs->EBP & 31;

            // clear bit later since it is referenced below
            }

        if (idef.wfDis & (wf_rA | wf_rA0))
            {
            if (idef.wfDis & (wf_rS | wf_fS))
                {
                vpregs->EAX = vpregs->EAX & 31;
                }
            else
                {
                vpregs->EBP = vpregs->EAX;
                vpregs->EBP = vpregs->EBP & 31;
                }

            idef.wfDis &= ~(wf_rA | wf_rA0);
            }

        if (idef.wfDis & (wf_rS | wf_fS))
            {
            idef.wfDis &= ~(wf_rS | wf_fS);
            }

        if (idef.wfDis & (wf_rD | wf_fD))
            {
            vpregs->EAX = vpregs->EAX >> 5;
            vpregs->EAX = vpregs->EAX & 31;

            idef.wfDis &= ~(wf_rD | wf_fD);
            }

        if (idef.wfDis & wf_crfD)
            {
            vpregs->EAX = vpregs->EAX >> 7;
            vpregs->EAX = vpregs->EAX & 7;

            idef.wfDis &= ~wf_crfD;
            }

        if (idef.wfDis)
            {
#if TRACEPPC
            printf("Flags that were missed: %08X\n", idef.wfDis);
#endif
            return 1;
            }
        }

#if TRACEPPC
    printf("finished field decode: DEST = %08X, SRC1 = %08X, SRC2 = %08X\n",
        vpregs->EAX, vpregs->EBP, vpregs->EBX);
#endif

    // Now execute the micro-ops

    for (i = 0; i < 8; i++)
        {
        BYTE uop = idef.pltUop->rgl[i];

#if TRACEPPC
        printf("executing micro-op %02X\n", uop);
#endif

        switch (uop)
            {
        ULONG tmpA, tmpB, tmpC;

        default:
#if TRACEPPC
            printf("unknown micro-op %02X\n", uop);
#endif
            return 1;

        case uopNop:
            break;

        case uopFetch:
            i = 8;              // force the loop to stop
            break;

        case uopBranch:
            if ((idef.wfCPU & wfChkAA) && !(EBXSav & 2))
                vpregs->EBX += vpregs->PC - 4;

            vpregs->PC = vpregs->EBX;
            break;

        case uopBranchCond:
            // BO is in EAX
            // bit 4 indicates if we bother to check the condition bits
            // bit 3 indicates what the tested bit must equal
            // bit 2 indicates if we bother to decrement the counter
            // bit 1 indicates if we branch on counter hitting 0
            // bit 0 is the branch prediction, we'll ignore it

            // BI is in EBP
            // it represents which bit of the CR to test
            // keep in mind that the PPC bit numbers are backwards!

            {
            if ((vpregs->EAX & 0x10) ||
                (!(vpregs->regCR & (0x80000000 >> vpregs->EBP)) == !(vpregs->EAX & 8)))
                {
                if ((vpregs->EAX & 4) ||
                    (!(--vpregs->regCTR) != !(vpregs->EAX & 2)))
                    {
#if TRACEPPC
                    printf("BRANCH TAKEN!\n");
#endif

                   if ((idef.wfCPU & wfChkAA) && !(EBXSav & 2))
                        vpregs->EBX += vpregs->PC - 4;

                    vpregs->PC = vpregs->EBX;
                    }
                }
            }
            break;

        case uopGenEA_rA0_d:
            if (vpregs->EBP)
                vpregs->EBX += vpregs->rggpr[vpregs->EBP];
            break;

        case uopGenEA_rA_d:
            vpregs->EBX += vpregs->rggpr[vpregs->EBP];
            break;

        case uopGenEA_rA_rB:
            vpregs->EBX += vpregs->rggpr[vpregs->EBP];
            break;

        case uopGenEA_rA:
            vpregs->EBX = vpregs->rggpr[vpregs->EBP];
            break;

        case uopUpdate_rA:
            // The PPC601 does update R0 on all stores

            vpregs->rggpr[vpregs->EBP] = vpregs->EBX;
            break;

        case uopUpdate_rA0_load:
            // The PPC601 does not update R0 on loads when R0 = 0

            if (vpregs->EBP)
                vpregs->rggpr[vpregs->EBP] = vpregs->EBX;
            break;

        case uopConvEA:
            // convert EA in EBX to flat offset in EBP

            vpregs->EBP = MapAddressVM(vpregs->EBX);

            // NOTE: for the purposes of C, we don't really do this

            vpregs->EBP = vpregs->EBX;
            break;

        case uopLoadByteZero:
            vpregs->EBX = (BYTE)PeekB(vpregs->EBP);
            break;

        case uopLoadHalfZero:
            vpregs->EBX = (unsigned short)PeekW(vpregs->EBP);
            break;

        case uopLoadHalfArith:
            vpregs->EBX = (signed short)PeekW(vpregs->EBP);
            break;

        case uopLoadWordZero:
            vpregs->EBX = PeekL(vpregs->EBP);
            break;

        case uopStoreByte:
            PokeB(vpregs->EBP, vpregs->EBX);
            break;

        case uopStoreHalf:
            PokeW(vpregs->EBP, vpregs->EBX);
            break;

        case uopStoreWord:
            PokeL(vpregs->EBP, vpregs->EBX);
            break;

        case uopLoadDouble:
            {
            char rgb[8];

            *(double *)&rgb[0] = vpregs->fp0;

            // REVIEW: Need memory lock!
            // PokeL(vpregs->EBP, vpregs->EBX);
            }
            break;

        case uopStoreDouble:
            {
            char rgb[8];

            *(double *)&rgb[0] = vpregs->fp0;

            // REVIEW: Need memory lock!
            // PokeL(vpregs->EBP, vpregs->EBX);
            }
            break;

        case uopLoad_rA:
            vpregs->EBP = vpregs->rggpr[vpregs->EBP];
            break;

        case uopLoad_rA0:
            if (vpregs->EBP)
                vpregs->EBP = vpregs->rggpr[vpregs->EBP];
            break;

        case uopLoad_rS:
            vpregs->EBP = vpregs->rggpr[vpregs->EBP];
            break;

        case uopLoad_rB:
            vpregs->EBX = vpregs->rggpr[vpregs->EBX];
            break;

        case uopLoad_rD:
            vpregs->EBX = vpregs->rggpr[vpregs->EAX];
            break;

        case uopLoad_frD:
            vpregs->fp0 = vpregs->rgfpr[vpregs->EAX];
            break;

        case uopLoad_LR:
            vpregs->EBX = vpregs->regLR;
            break;

        case uopStore_LR:
            vpregs->regLR = vpregs->EBX;
            break;

        case uopLoad_CTR:
            vpregs->EBX = vpregs->regCTR;
            break;

        case uopStore_CTR:
            vpregs->regCTR = vpregs->EBX;
            break;

        case uopLoad_MSR:
    mppc_DumpRegs();
            printf("READING MSR\n");

            vpregs->EBX = vpregs->MSR;
            break;

        case uopStore_MSR:
    mppc_DumpRegs();
            printf("WRITING MSR with %08X\n", vpregs->EBX);

            vpregs->MSR = vpregs->EBX;
            break;

        case uopLoad_FPSCR:
    mppc_DumpRegs();
            printf("READING FPSCR\n");

            *(long *)&vpregs->fp0 = vpregs->FPSCR;
            break;

        case uopLoad_CR:
    mppc_DumpRegs();
            printf("READING CR\n");

            vpregs->EBX = vpregs->regCR;
            break;

        case uopStore_CR:
    mppc_DumpRegs();
            printf("WRITING CR with %08X, mask %02X\n", vpregs->EBX, vpregs->EBP);

            vpregs->regCR = vpregs->EBX;
            break;

        case uopLoad_SPR:
    mppc_DumpRegs();
            switch (vpregs->EBP)
                {
            default:
                vpregs->EBX = vpregs->rgspr[vpregs->EBP];
                break;

            case 287:           // PVR
                vpregs->EBX = 0x00010002;       // 601 1.2
                break;
                }

            if (8 != vpregs->EBP)
                printf("READING SPR %d as %08X\n", vpregs->EBP, vpregs->EBX);

            break;

        case uopStore_SPR:
    mppc_DumpRegs();
            if (8 != vpregs->EBP)
                printf("WRITING SPR %d with %08X\n", vpregs->EBP, vpregs->EBX);

            switch (vpregs->rgspr[vpregs->EBP])
                {
            default:
                vpregs->rgspr[vpregs->EBP] = vpregs->EBX;
                break;
                }

            break;

        case uopNotSrc:
            vpregs->EBP = ~vpregs->EBP;
            break;

        case uopNotData:
            vpregs->EBX = ~vpregs->EBX;
            break;

        case uopNegData:
            vpregs->EBX = -vpregs->EBX;
            break;

        case uopCntlzw:
            vpregs->EBX = 0;

            while ((vpregs->EBP & 0x80000000) == 0)
                {
                vpregs->EBX++;
                vpregs->EBP <<= 1;
                }

            if ((idef.wfCPU & wfChkRc) && (EBXSav & bitRc))
                {
                vpregs->regCR &= 0x0FFFFFFF;
                if (vpregs->EBX == 0)
                    vpregs->regCR |= 0x20000000;
                else if (vpregs->EBX & 0x80000000)
                    vpregs->regCR |= 0x80000000;
                else
                    vpregs->regCR |= 0x40000000;
                }
            break;

        case uopShl:
            vpregs->EBP &= 63;

            if (vpregs->EBP & 32)
                {
                // shift count >= 32, clear result

                vpregs->EBX = 0;
                }

#ifdef C_ARITH
            vpregs->EBX = vpregs->EBX << vpregs->EBP;
#else
            PrepareForArithUop();
            __asm push    ecx
            __asm mov     ecx,eax
            __asm shl     ebx,cl
            __asm pop     ecx
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopShr:
            vpregs->EBP &= 63;

            if (vpregs->EBP & 32)
                {
                // shift count >= 32, clear result

                vpregs->EBX = 0;
                }

#ifdef C_ARITH
            vpregs->EBX = vpregs->EBX >> vpregs->EBP;
#else
            PrepareForArithUop();
            __asm push    ecx
            __asm mov     ecx,eax
            __asm shr     ebx,cl
            __asm pop     ecx
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopSar:
            vpregs->EBP &= 63;

            if (vpregs->EBP == 0)
                {
                // shift count is zero, clear XER[CA]

                vpregs->regXER &= ~0x40000000;
                vpregs->EBX = vpregs->EBP;
                }

            else if (vpregs->EBP & 32)
                {
                // shift count >= 32, set to sign bit

                if (vpregs->EBP & 0x80000000)
                    {
                    vpregs->regXER |= 0xC0000000;
                    vpregs->EBX = 0xFFFFFFFF;
                    }
                else
                    {
                    vpregs->regXER &= ~0x40000000;
                    vpregs->EBX = 0;
                    }
                }

            else
                {
                // shift count is 1..31

#ifdef C_ARITH
                vpregs->EBX = (int)vpregs->EBX >> vpregs->EBP;
#else
                PrepareForArithUop();
                __asm push    ecx
                __asm mov     ecx,eax
                __asm sar     ebx,cl
                __asm sbb     ecx,ecx
                __asm and     ecx,ebx
                __asm add     ecx,ecx
                __asm pop     ecx
                PostProcrArithUop();
                PostProcrArithUop2();
#endif
                }
            break;

        case uopExtb:
            vpregs->EBX = (int)(signed char)vpregs->EBP;

            if ((idef.wfCPU & wfChkRc) && (EBXSav & bitRc))
                {
                vpregs->regCR &= 0x0FFFFFFF;
                if (vpregs->EBX == 0)
                    vpregs->regCR |= 0x20000000;
                else if (vpregs->EBX & 0x80000000)
                    vpregs->regCR |= 0x80000000;
                else
                    vpregs->regCR |= 0x40000000;
                }
            break;

        case uopExth:
            vpregs->EBX = (int)(signed short)vpregs->EBP;

            if ((idef.wfCPU & wfChkRc) && (EBXSav & bitRc))
                {
                vpregs->regCR &= 0x0FFFFFFF;
                if (vpregs->EBX == 0)
                    vpregs->regCR |= 0x20000000;
                else if (vpregs->EBX & 0x80000000)
                    vpregs->regCR |= 0x80000000;
                else
                    vpregs->regCR |= 0x40000000;
                }
            break;

        case uopAdd:
#ifdef C_ARITH
            vpregs->EBX += vpregs->EBP;
#else
            PrepareForArithUop();
            __asm add     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopAddX:
#ifdef C_ARITH
            // REVIEW: not implemented correctly
            vpregs->EBX += vpregs->EBP;
#else
            PrepareForArithUop();
            __asm bt      ecx,0x1D
            __asm adc     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopAnd:
#ifdef C_ARITH
            vpregs->EBX &= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm and     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopSwap:
            vpregs->EBX ^= vpregs->EBP;
            vpregs->EBP ^= vpregs->EBX;
            vpregs->EBX ^= vpregs->EBP;
            break;

        case uopDivS:
            if (vpregs->EBX == 0)
                {
                if ((idef.wfCPU & wfChkOE) && (EBXSav & bitOE)) \
                    vpregs->regXER |=  0xC0000000;
                }
            else
                {
#ifdef C_ARITH
                vpregs->EBX /= vpregs->EBP;
#else
                PrepareForArithUop();
                __asm xor     edx,edx
                __asm idiv    ebx
                __asm mov     ebx,eax
                PostProcrArithUop();
                PostProcrArithUop2();
#endif
                }
            break;

        case uopDivU:
            if (vpregs->EBX == 0)
                {
                if ((idef.wfCPU & wfChkOE) && (EBXSav & bitOE)) \
                    vpregs->regXER |=  0xC0000000;
                }
            else
                {
#ifdef C_ARITH
                vpregs->EBX /= vpregs->EBP;
#else
                PrepareForArithUop();
                __asm xor     edx,edx
                __asm div     ebx
                __asm mov     ebx,eax
                PostProcrArithUop();
                PostProcrArithUop2();
#endif
                }
            break;

#if 0
        case uopMulS:
#ifdef C_ARITH
            vpregs->EBX *= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm xor     edx,edx
            __asm imul    ebx
            __asm mov     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;
#endif

        case uopMul:
#ifdef C_ARITH
            vpregs->EBX *= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm xor     edx,edx
            __asm mul     ebx
            __asm mov     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopOr:
#ifdef C_ARITH
            vpregs->EBX |= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm or      ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopSub:
#ifdef C_ARITH
            vpregs->EBX -= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm sub     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopSubX:
#ifdef C_ARITH
            // REVIEW: not implemented correctly
            vpregs->EBX -= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm bt      ecx,0x1D
            __asm sbb     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopXor:
#ifdef C_ARITH
            vpregs->EBX ^= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm xor     ebx,eax
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopMaskg:
            vpregs->EBP >>= 11;
            vpregs->EBP &= 31;
            vpregs->EBX = gen_maskg((vpregs->EBP << 5) | (vpregs->EBX));
#ifdef C_ARITH
#else
            PrepareForArithUop();
            __asm test    ebx,ebx
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopRlwnm:
            vpregs->EBP = _rotl(vpregs->EBP, vpregs->EBX);
            EBXSav = gen_maskg((EBXSav >> 1) & 1023);
            vpregs->EBX = vpregs->EBP;
            vpregs->EBX &= EBXSav;
#ifdef C_ARITH
#else
            PrepareForArithUop();
            __asm test    ebx,ebx
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopRlwimi:
            vpregs->EBX = _rotl(vpregs->EBP, vpregs->EBX);
            EBXSav = gen_maskg((EBXSav >> 1) & 1023);

            // use the XOR trick to transfer the necessary bits to dest
            vpregs->EBX ^= vpregs->rggpr[vpregs->EAX];
            vpregs->EBX &= EBXSav;
            vpregs->EBX ^= vpregs->rggpr[vpregs->EAX];
#ifdef C_ARITH
#else
            PrepareForArithUop();
            __asm test    ebx,ebx
            PostProcrArithUop();
            PostProcrArithUop2();
#endif
            break;

        case uopCmpU:
            vpregs->EBX ^= 0x80000000;
            vpregs->EBP ^= 0x80000000;

        case uopCmp:
#ifdef C_ARITH
            vpregs->EBX -= vpregs->EBP;
#else
            PrepareForArithUop();
            __asm xchg    ebx,eax
            __asm sub     ebx,eax
            PostProcrArithUop();

            // have to manually update crfD

            vpregs->EBX = tmpB;

            {
            int cs = (7 - vpregs->EAX) * 4;

            vpregs->regCR &= (0xFFFFFFF0 << cs);
            if (tmpC & 0x0800)
                vpregs->regCR |= (1 << cs);
            if (vpregs->EBX == 0)
                vpregs->regCR |= (2 << cs);
            else if (vpregs->EBX & 0x80000000)
                vpregs->regCR |= (8 << cs);
            else
                vpregs->regCR |= (4 << cs);
            }
#endif
            break;

        case uopStore_rD:
            vpregs->rggpr[vpregs->EAX] = vpregs->EBX;
            break;

        case uopStore_frD:
            vpregs->rgfpr[vpregs->EAX] = vpregs->fp0;
            break;

        case uopRepeatMultWord:
            vpregs->EAX++;      // increment rD
            vpregs->EBP += 4;   // increment EA by 4 (REVIEW: decrement when we really do ConvEA!)

            // keep looping until rD is 32

            if (vpregs->EAX < 32)
                i -= 3;
            break;

        case uopAnd_0x0F:
            vpregs->EBP = (vpregs->EBX >> 28);
            break;

        case uopLoad_SR:
            vpregs->EBX = vpregs->rgsgr[vpregs->EBP];
            break;

        case uopStore_SR:
            vpregs->rgsgr[vpregs->EBP] = vpregs->EBX;
            break;

        case uopShift16:
            vpregs->EBX <<= 16;
            break;

        case uopLdc0:
            vpregs->EBX = 0;
            break;

        case uopLdc1:
            vpregs->EBX = 1;
            break;

        case uopLdcM1:
            vpregs->EBX = M1;
            break;

            }
        }

#if TRACEPPC
    printf("instruction completed\n: DEST = %08X, SRC1 = %08X, DATA = %08X\n",
         vpregs->EAX, vpregs->EBP, vpregs->EBX);

    mppc_DumpRegs();
#endif

    return 0;
}


ULONG __cdecl mppc_GoPPC(int count)
{
    ULONG l;

#if TRACEPPC
    printf("GoPPC: stepping instructions at %08X\n", vpregs->PC);

    mppc_DumpRegs();
#endif

    while (count--)
        {
        extern ULONG lBreakpoint;

        l = mppc_StepPPC(1);

        if (vpregs->PC == lBreakpoint)
            l = 1;

        if (l)
            break;
        }

    return l;
}

#endif // OLDPPC


ULONG __cdecl mppc_ResetPPC()
{
    vpregs->rggpr[1] = 0x4000;

    return TRUE;
}

extern __cdecl Init68000();
extern __cdecl Step68000();
extern __cdecl   Go68000();
extern __cdecl mac_Reset();

ICpuExec cpiPPC =
    {
    NULL,
    mppc_GetCpus,
    mppc_GetCaps,
    mppc_GetVers,
    mppc_GetPageBits,
    Init68000,
    mppc_ResetPPC,
    Go68000,
    Step68000,
    0
    };

#endif // POWERMAC


void m68k_DumpProfile()
{
	ULONG i;
	ULONG cops = 0;
	ULONG iMax = 0;    // offset of most used opcode
	ULONG copsMax = 0; // count of occurances of iMax
	ULONG cexec = 0;
	ULONG rgcnib[16];

	for (i=0; i < 16; i++)
		rgcnib[i] = 0;

    printf("\nDumping profile...\n");

	for (i=0; i < 65536; i++)
		{
		if (vi.pProfile[i] != 0)
			{
			cexec += vi.pProfile[i];
			rgcnib[i>>12] += vi.pProfile[i];

			if (vi.pProfile[i] > copsMax)
				{
				copsMax = vi.pProfile[i];
				iMax = i;
				}

			printf("opcode %04X: %08X\n", i, vi.pProfile[i]);
			cops++;
			}
		}

	printf("\nTotal # of instructions executed = %d\n", cexec);
	printf("Total # of unique opcodes executed = %d\n", cops);
	printf("Most used opcode = $%04X, used %d times\n", iMax, copsMax);
	for (i=0; i < 16; i++)
		printf("Count of all $%X000 opcodes = %d\n", i, rgcnib[i]);
}

void m68k_DumpHistory(int c)
{
	int i = vi.iHistory - c - 1;

    if (vi.pHistory == NULL)
        return;

    if (vi.iHistory == 0)
        return;

#ifdef POWERMAC
    if (vmCur.bfCPU >= cpu601)
        return;
#endif

	while (c-- > 0)
		{
        ULONG lad, ladSav, PC;

        ladSav = vsthw.lAddrMask;

// DebugStr("dump history: c = %d\n", c);
		i = (i+1) & (MAX_HIST-1);
// DebugStr("dump history: i = %d\n", i);
// DebugStr("dump history: pHistory = %08X\n", vi.pHistory);
// DebugStr("dump history: pHistory[i] = %08X\n", vi.pHistory[i]);

        PC = vi.pHistory[i++];
        lad = (PC & 1) ? 0xFFFFFFFF : 0x00FFFFFF;
        PC &= ~1;
		printf("%d%c-", (lad & 0xFF000000) ? 32 : 24,
            (vi.pHistory[i + 12] & 1) ? 'S' : 'U');
		printf("%04X- ", c);
        printf("D0=%08X ",  vi.pHistory[i++]);
        printf("D1=%08X ",  vi.pHistory[i++]);
        printf("D2=%08X ",  vi.pHistory[i++]);
        printf("D3=%08X ",  vi.pHistory[i++]);
        printf("D4=%08X ",  vi.pHistory[i++]);
        printf("D5=%08X ",  vi.pHistory[i++]);
        printf("D6=%08X ",  vi.pHistory[i++]);
        printf("D7=%08X ",  vi.pHistory[i++]);
        printf("A0=%08X ",  vi.pHistory[i++]);
        printf("A1=%08X ",  vi.pHistory[i++]);
        printf("A2=%08X ",  vi.pHistory[i++]);
        printf("A6=%08X ",  vi.pHistory[i++]);
        printf("A7=%08X  ", vi.pHistory[i++] & 0x00FFFFFE);
        printf("%08X ",     vi.pHistory[i++]);
        printf("%08X ",     vi.pHistory[i]);
        printf("%02X ",    (vi.pHistory[i-2] & 0xFF000000) >> 24);
        c -= 15;

        vsthw.lAddrMask = lad;
		CDis(PC, TRUE);
        vsthw.lAddrMask = ladSav;
		}
}


// copy a string from 68000 address space at address addr into buffer pch

BOOL Str68ToStr(ULONG addr, unsigned char *pch)
{
#ifndef NDEBUG
    unsigned cch = 0;
#endif

	do
		{
		*pch++ = (unsigned char)PeekB(addr++);
#ifndef NDEBUG
        cch++;
        Assert(cch < 1024);
#endif
		} while (PeekB(addr));

	*pch = '\0';
	return TRUE;
}


// copy a string from 68000 address space at address addr into buffer pch

BOOL Str68ToStrN(ULONG addr, unsigned char *pch, unsigned cchMax)
{
    unsigned cch = 0;

	do
		{
		*pch++ = (unsigned char)PeekB(addr++);
        cch++;
        if (cch >= cchMax)
            break;

		} while (PeekB(addr));

	*pch = '\0';
	return TRUE;
}


// copy a string into 68000 address space at address addr

BOOL StrToStr68(unsigned char *pch, ULONG addr)
{
#ifndef NDEBUG
    unsigned cch = 0;
#endif

	do
		{
		vmPokeB(addr++, (BYTE)toupper(*pch++));
#ifndef NDEBUG
        cch++;
        Assert(cch < 1024);
#endif
		} while (*pch);

	vmPokeB(addr, 0);
	return TRUE;
}


MemorySnapShot()
{
#if 0
    HANDLE h;
	OFSTRUCT ofs;
    unsigned cb = 262144;
    unsigned buf = 0;
    int i;
	static char sz[] = "GEMUL8R0.DMP";

	return;

    ReverseMemory(buf, cb);
    h = OpenFile(sz, &ofs, OF_CREATE);
    WriteFile(h, memPvGetHostAddress(buf, cb), cb, &i, NULL);
    CloseHandle(h);
    ReverseMemory(buf, cb);
	sz[7]++;
#endif
}

#define TRACEBF 0

#if TRACEBF
#pragma optimize ("",off)
#endif

BOOL __cdecl BitfieldHelper(ULONG_PTR ea, ULONG instr, ULONG xword)
{
    BOOL fDn;
    ULONG reg, width;
    int  offset, doffset;       // bit offset is signed 32-bit number
    ULONG *plDreg = &vpregs->D0;
    ULONG lReg, lMem, lMem0, lMem4;

#if TRACEBF
    static char *mpTypeBitf[8] =
         { "TST ", "EXTU", "CHG ", "EXTS", "CLR ", "FFO ", "SET ", "INS " };
#endif

    fDn = ((ea >= (ULONG_PTR)&vpregs->D0) && (ea <= (ULONG_PTR)&vpregs->D7));

    if (fDn)
        {
        ea -= (ULONG_PTR)&vpregs->D0;
        ea /= sizeof(vpregs->D0);
        }

#if TRACEBF
    instr &= 0x0000FFFF;
    xword &= 0x0000FFFF;
#endif

    reg    = (xword >> 12) & 7;
    offset = (xword >>  6) & 31;

#if TRACEBF
    printf("in bitfield helper for BF%s, PC = %08X, instr = %04X, xword = %04X, ",
        mpTypeBitf[(instr >> 8) & 7], vpregs->PC, instr, xword);

    if (fDn)
        {
        printf("ea = D%x,       ", ea);
        }
    else
        {
        printf("ea = %08X, ", ea);
        }

    printf("reg = D%d, Do = %d, offset = %d, Dw = %d, width = %d\n",
        reg, !!(xword & 2048), offset, !!(xword & 32), width);
#endif

    if (xword & 2048)   // Do
        {
        offset = plDreg[offset & 7];
#if TRACEBF
        printf("Reading signed offset %d from register\n", offset);
#endif
        }

    if (xword & 32)     // Dw
        {
        width  = plDreg[xword & 7];
        }
    else
        width  = xword & 31;

    // Width is modulo 32, with 0 meaning 32

    width = ((width - 1) & 31) + 1;

    if (fDn)
        {
        // REVIEW: what to do when offset >= 32????
        // Assume it is modulo 32

        offset &= 31;
        doffset = 0;
        width = min((int)width, 32-(int)offset);

        lMem0 = plDreg[ea];
        }
    else
        {
        doffset = offset;

        // longword align the effective address and
        // adjust offset accordingly

        ea += (int)(offset >> 3);
        offset &= 7;
        offset += ((ea & 3) << 3);
        ea &= ~3;

        // then set doffset as the adjustment factor
        // so that BFFFO gives the correct answer

        doffset -= offset;

        lMem0 = PeekL(ea);
        }

    lReg = plDreg[reg];

#if TRACEBF
    printf("actual ea = %08X, actual offset = %d, actual width = %d\n", ea, offset, width);
    printf("Reading %08X from register D%d\n", lReg, reg);
#endif

    if ((offset + width) > 32)          // bitfield spans 2 longwords
        {
        lMem4 = PeekL(ea+4);
        lMem = lMem4 >> (64 - (offset + width));
        lMem |= lMem0 << ((offset + width) - 32);
#if TRACEBF
        printf("large span read %08X %08X\n", lMem0, lMem4);
#endif
        }
    else
        {
        lMem = lMem0 >> (32 - (offset + width));
#if TRACEBF
        printf("small span read %08X\n", lMem0);
#endif
        }

    // sign extend the bitfield and set the CCR accordingly

    if (width < 32)
        {
        (int)lMem <<= (32 - width) & 31;
        (int)lMem >>= (32 - width) & 31;
        }

    vpregs->rgfsr.bitN = (lMem & (1 << ((width - 1) & 31))) != 0;
    vpregs->rgfsr.bitZ = (lMem == 0);
    vpregs->rgfsr.bitV = 0;
    vpregs->rgfsr.bitC = 0;

    // Now update Dn (lReg) and/or bitfield (lMem) as appropriate

    switch(instr & 0x0700)
        {
        // 0 = BFTST
        // 1 = BFEXTU
        // 2 = BFCHG
        // 3 = BFEXTS
        // 4 = BFCLR
        // 5 = BFFFO
        // 6 = BFSET
        // 7 = BFINS

    default:
        Assert(FALSE);
        break;

    case 0x000:
        break;

    case 0x100:
        lReg = lMem;

        if (width < 32)
            lReg &= (1 << width) - 1;
        break;

    case 0x200:
        lMem = ~lMem;
        break;

    case 0x300:
        lReg = lMem;

        if (width < 32)
            {
            (int)lReg <<= 32 - width;
            (int)lReg >>= 32 - width;
            }
        break;

    case 0x400:
        lMem = 0;
        break;

    case 0x500:
        // BFFFO scans bitfield for the most significant 1 bit
        // The result is the bit position of that 1 bit.
        // If no 1 bit is found, result is offset+width.
        // NOTE: safe to trash lMem and width since this does
        // not write back to memory.

        lReg = offset + width;

        while (width--)
            {
            if (lMem & 1)
                lReg = offset + width;

            lMem >>= 1;
            }

        lReg += doffset;
        break;

    case 0x600:
        lMem = 0xFFFFFFFF;
        break;

    case 0x700:
        lMem = lReg;
        break;
        }

#if TRACEBF
    printf("Writing %08X to register D%d\n", lReg, reg);
#endif

    plDreg[reg] = lReg;

    switch(instr & 0x700)
        {
        // These instructions have to write data back to memory
        //
        // 2 = BFCHG
        // 4 = BFCLR
        // 6 = BFSET
        // 7 = BFINS

    case 0x200:
    case 0x400:
    case 0x600:
    case 0x700:
        if (width < 32)
            lMem &= (1 << width) - 1;       // zero extend write value

#if TRACEBF
        printf("Writing %08X to memory\n", lMem);
#endif

        if ((offset + width) > 32)          // bitfield spans 2 longwords
            {
            lMem0 &= ~(0xFFFFFFFF >> offset);
            lMem4 &=   0xFFFFFFFF >> (offset + width - 32);

            lMem0 |= (lMem >> ((offset + width) - 32));
            lMem4 |= (lMem << (64 - (offset + width)));

            PokeL(ea,   lMem0);
            PokeL(ea+4, lMem4);
#if TRACEBF
            printf("large span wrote %08X %08X\n", lMem0, lMem4);
#endif
            }
        else
            {
            if (offset + width == 32)
                lMem0 &= ~(0xFFFFFFFF >> offset);
            else
                lMem0 &= (~(0xFFFFFFFF >> offset)) |
                           (0xFFFFFFFF >> (offset + width));

            lMem0 |= (lMem << (32 - (offset + width)));

            if (fDn)
                plDreg[ea] = lMem0;
            else
                PokeL(ea, lMem0);
#if TRACEBF
            printf("small span wrote %08X\n", lMem0);
#endif
            }

        break;
        }

    return TRUE;
}


void __stdcall ProfilePoint(ULONG id)
{
    static int count;
    static ULONG rgPP[1024];

    rgPP[id]++;

    if ((count++ & (1024*1024*128-1)) == 0)
        {
        int i;

        for (i = 0; i < sizeof(rgPP)/sizeof(ULONG); i++)
            {
            printf("PP%03X: %10d ", i, rgPP[i]);
            if ((i & 7) == 7)
                printf("\n");
            }
        }
}

void __cdecl gui_hooks()
{
#if !defined(NDEBUG)
    printf("GUI Hook %04X\n", vpregs->D0 & 65535);
#endif

    switch(vpregs->D0 & 65535)
        {
    default:
        vpregs->D0 = -1;
        break;

        // Hibernate

    case 1:
        // assume hibernate will succeed

        vpregs->D0 = 0;

        SetWindowText(vi.hWnd, "Hibernating...");

        v.fSkipStartup |= 2;

        if (SaveState(TRUE))
            PostQuitMessage(0);

        // if postquit returns, it means hibernate failed, so return error

        v.fSkipStartup &= ~2;
        vpregs->D0 = -1;
        break;

        // Toggle Full Screen State

    case 6:
        v.fFullScreen = !v.fFullScreen;
        CreateNewBitmap();

        // Query Screen State

    case 5:
        vpregs->D0 = !!v.fFullScreen;
        break;
        }

#if !defined(NDEBUG)
    printf("Returning %08X\n", vpregs->D0);
#endif
}

#endif

#if !defined(SOFTMAC)

// Stub for Mac OS file system hooks

void __cdecl mac_EFS()
{
}

#endif

