
//
// PPC.C
//
// C based PowerPC interpreter module.
//
// 10/07/99 last update
//

#include "..\gemtypes.h"

#ifndef XFORMER

#ifndef NDEBUG
#define  TRACETRAP  1
#define  TRACEEXCPT 1
#define  TRACEFPU   1
#define  TRACEMMU   1
#define  TRACE040   1
#else
#define  TRACETRAP  0
#define  TRACEEXCPT 0
#define  TRACEFPU   0
#define  TRACEMMU   0
#define  TRACE040   0
#endif

extern BOOL fLogHWAccess;
int fTracing;

#if TRACEFPU || defined(BETA)
ULONG iFPU;
#endif

BOOL __cdecl mppc_DumpRegs(void);
void mppc_DumpHistory(int c);
void mppc_RAMToBuf(ULONG csecs);
void mppc_BufToRAM(ULONG csecs);

// #pragma optimize("",off)

void mppc_ReverseMemory(lAddr, cb)
ULONG lAddr;
ULONG cb;
{
	BYTE t;
	BYTE *pbSrc = vpbRAM - lAddr - cb + 2;
	BYTE *pbDst = vpbRAM - lAddr + 1;

	cb >>= 1;

	while (cb--)
		{
		t        = *pbSrc;
		*pbSrc++ = *pbDst;
		*pbDst-- = t;
		}
	}


BOOL __cdecl mppc_DumpRegs(void)
{
#if defined(BETA) || (!defined(DEMO) && !defined(SOFTMAC))
	if ((vpregs == 0) || (vi.cbRAM == 0))
		return 0;

	printf("DATA %08X %08X ", vpregs->D0, vpregs->D1); 
	printf("%08X %08X ", vpregs->D2, vpregs->D3); 
	printf("%08X %08X ", vpregs->D4, vpregs->D5); 
	printf("%08X %08X\n", vpregs->D6, vpregs->D7); 
	printf("ADDR %08X %08X ",  vpregs->A0, vpregs->A1); 
	printf("%08X %08X ",  vpregs->A2, vpregs->A3); 
	printf("%08X %08X ",  vpregs->A4, vpregs->A5); 
	printf("%08X %08X\n", vpregs->A6, vpregs->A7); 

#ifdef GEMPRO
    if (vmCur.bfCPU > cpu68000)
        {
        printf("VBR  %08X ", vpregs->VBR);
        printf("SFC/DFC  %08X %08X ", vpregs->SFC, vpregs->DFC);
        printf("CACR/AAR %08X %08X ", vpregs->CACR, vpregs->CAAR);
        printf("%02d-bit addressing ", (vpregs->lAddrMask & 0xFF000000) ? 32 : 24);
        printf("\n");
        }

#if TRACEFPU
    if (vmCur.bfCPU >= cpu68020)
        {
        int i;

        printf("FPU  ");

        for (i = 0; i < 8; i++)
            {
            unsigned char *pvSrc = &vi.pregs2->rgbFPx[i * 10];
            double d;
            BYTE rgch[256];

            __asm mov   eax,pvSrc
            __asm fld   tbyte ptr [eax]
            __asm lea   eax,d
            __asm fstp  qword ptr [eax]

#if !defined(NDEBUG) || defined(BETA)
            sprintf(rgch, "%g ", d);
            printf(rgch);
#endif
            }
        printf("CR: %04X SR: %08X\n", vi.pregs2->FPCR, vi.pregs2->FPSR);
        }
#endif

    if (1)
        {
        // Dump top 64 bytes of Mac RAM (boot globs)

        int i;

        for (i = -64; i < 0; i+= 4)
            printf("%08X ", PeekL(vi.cbRAM + i));

        printf("\n");

#if 0
        printf("VIA vDirB = %02X\n", vmachw.via1.vDirB);
        printf("VIA vBufB = %02X\n", vmachw.via1.vBufB);
#endif
        }
#endif

	printf("PC %08X:%04X  ", vpregs->PC, PeekW(vpregs->PC));
	if (vpregs->rgfsr.bitSuper)
		printf("USP %08X  ", vpregs->USP); 
	else
		printf("SSP %08X  ", vpregs->SSP); 
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

    {
    int i;

    // Display the first few longs on the stack, but only if A7 is legal

    for (i = 0; i < 44; i += 4)
        {
        if (vpregs->A7 + i + 4 < vi.cbRAM)
            printf("%08X ",  PeekL(vpregs->A7 + i));
        }
    printf("\n");
    }

#ifndef DEMO
	CDis(vpregs->PC, TRUE);
#endif

#if 0
	printf("\n");
	printf("countR: %09d  countW: %09d\n", vi.countR, vi.countW);
#endif
	printf("\n");
#endif

    return TRUE;
}


void mppc_DumpProfile()
{
#ifndef NDEBUG
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
#endif
}

void mppc_DumpHistory(int c)
{
	int i = vi.iHistory - c - 1;
    ULONG lAddrMaskSav = vpregs->lAddrMask;

#if defined(BETA) || (!defined(DEMO) && !defined(SOFTMAC))
    if (vi.pHistory == NULL)
        return;

    // Force 24-bit mode so that disassembly of both modes works

    vpregs->lAddrMask = 0x00FFFFFF;

	while (c-- > 0)
		{
        ULONG lad, ladSav, PC;

        ladSav = vpregs->lAddrMask;

// DebugStr("dump history: c = %d\n", c);
		i = (i+1) & (MAX_HIST-1);
// DebugStr("dump history: i = %d\n", i);
// DebugStr("dump history: pHistory = %08X\n", vi.pHistory);
// DebugStr("dump history: pHistory[i] = %08X\n", vi.pHistory[i]);

        PC = vi.pHistory[i++];
        lad = (PC & 1) ? 0xFFFFFFFF : 0x00FFFFFF;
        PC &= ~1;
		printf("%d-bit - ", (lad & 0xFF000000) ? 32 : 24);
		printf("%04X - ", c);
        printf("D0=%08X ", vi.pHistory[i++]);
        printf("D1=%08X ", vi.pHistory[i++]);
        printf("D2=%08X ", vi.pHistory[i++]);
        printf("D6=%08X ", vi.pHistory[i++]);
        printf("D7=%08X ", vi.pHistory[i++]);
        printf("A0=%08X ", vi.pHistory[i++]);
        printf("A1=%08X ", vi.pHistory[i++]);
        printf("A2=%08X ", vi.pHistory[i++]);
        printf("A3=%08X ", vi.pHistory[i++]);
        printf("A4=%08X ", vi.pHistory[i++]);
        printf("A5=%08X ", vi.pHistory[i++]);
        printf("A6=%08X ", vi.pHistory[i++]);
        printf("A7=%08X  ", vi.pHistory[i++]);
        printf("%08X ", vi.pHistory[i++]);
        printf("%08X ", vi.pHistory[i]);
        c -= 15;

        vpregs->lAddrMask = lad;
		CDis(PC, TRUE);
        vpregs->lAddrMask = ladSav;
		}

    vpregs->lAddrMask = lAddrMaskSav;
#endif // BETA || (!DEMO && !SOFTMAC)
}


//
// Blit from 68000 address space to vsthw.rgbDiskBuffer
// csecs = # of 512 byte sectors
//

void mppc_RAMToBuf(ULONG csecs)
{
    BYTE *pbBuf = (BYTE *)&vsthw.rgbDiskBuffer;
    ULONG ea = vsthw.DMAbasis;
    ULONG cb = csecs*512;

    if (ea + cb < vi.cbRAM)
        {
        while (cb--)
            *pbBuf++ = RAMB(vpbRAM, ea++);
        }
}


//
// Blit from disk buffer to 68000 address space
// csecs = # of 512 byte sectors
//

void mppc_BufToRAM(ULONG csecs)
{
    BYTE *pbBuf = (BYTE *)&vsthw.rgbDiskBuffer;
    ULONG ea = vsthw.DMAbasis;
    ULONG cb = csecs*512;

    if (ea + cb < vi.cbRAM)
        {
        while (cb--)
            RAMB(vpbRAM, ea++) = *pbBuf++;
        }
}

#else  // XFORMER

BOOL __cdecl TrapHook(int vector)
{
    return TRUE;
}

BOOL __cdecl BitfieldHelper(ULONG ea, LONG instr, LONG xword)
{
    return TRUE;
}

#endif // XFORMER
