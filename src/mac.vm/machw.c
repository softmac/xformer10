
/***************************************************************************

	MACHW.C

	- Macintosh virtual machine

	08/22/2007  darekm  use vmPeek/vmPoke to avoid MMU translation
	04/20/2008  darekm  implement HRESULT based accessors

***************************************************************************/

#include "gemtypes.h"

#ifdef SOFTMAC

#ifndef NDEBUG
#define  TRACEHW   1
#define  TRACEDISK 1
#define  TRACESCSI 1
#define  TRACEIWM  1
#define  TRACEADB  1
#define  TRACESND  1
#define  TRACESCC  1
#define  TRACEVID  1
#else
#define  TRACEHW   0
#define  TRACEDISK 0
#define  TRACESCSI 0
#define  TRACEIWM  0
#define  TRACEADB  0
#define  TRACESND  0
#define  TRACESCC  0
#define  TRACEVID  0
#endif

#if 0
// Gestalt ID numbers
// Not used, just for reference

#define  idII      6
#define  idIIx     7
#define  idIIcx    8
#define  idIIci    11
#define  idIIsi    18
#define  idLC      19
#define  idQ900    20
#define  idQ700    22
#define  idQ950    26
#define  idC650    30
#define  idQ650    36
#define  idLCII    37
#define  idColClas 49
#define  idC610    52
#define  idQ610    53
#endif

extern int fTracing;

// 0 = Mac II Toby video card, 1 = Mac Hi Res card

#define  HIRES     0

// #pragma optimize("",off)

BOOL __cdecl mac_stub();
BOOL __cdecl mac_Install();
BOOL __cdecl mac_Init();
BOOL __cdecl mac_UnInit();
BOOL __cdecl mac_InitDisks();
BOOL __cdecl mac_MountDisk();
BOOL __cdecl mac_UnInitDisks();
BOOL __cdecl mac_UnmountDisk();
ULONG __cdecl mac_VPollInterrupts();
ULONG __cdecl mac_Reset();
ULONG __cdecl mac_DumpHW();
ULONG __cdecl mac_WinMsg();
ULONG __cdecl mac_Exec();
static HRESULT __cdecl mac_ReadHWByte(ULONG ea);
static HRESULT __cdecl mac_ReadHWWord(ULONG ea);
static HRESULT __cdecl mac_ReadHWLong(ULONG ea);
static HRESULT __cdecl mac_WriteHWByte(ULONG ea, BYTE b);
static HRESULT __cdecl mac_WriteHWWord(ULONG ea, WORD w);
static HRESULT __cdecl mac_WriteHWLong(ULONG ea, ULONG l);
ULONG __cdecl mac_LockBlock(ULONG ea, ULONG cb, void **ppv);
ULONG __cdecl mac_UnlockBlock(ULONG ea, ULONG cb);
ULONG __cdecl mac_MapAddress(ULONG ea);
ULONG __cdecl mac_MapAddressRW(ULONG ea);

#if !defined(DEMO)
#define ColorMacModes (monMacMon  | monMac12C  | monMac13F  | monMac13C  | \
                       monMac14W  | monMac14W2 | monMacVPB  | monMac14F  | \
                       monMac15C  | monMac16C  | monMac16X  | monMac17C  | \
                       monMac18F  | monMac19C  | monMac20C  | monMac21C  | \
                       monMac26C  | monMacMax)
#endif

#define DemoMacModes  (monMacMon  | monMac12C  | monMac13F  | monMac13C  | \
                       monMac14W  | monMac14W2 | monMacVPB)

#define DemoMacMem (ram8M | ram12M | ram14M | ram16M | ram20M | ram24M | ram32M)

#ifdef MAKEDLL
_declspec(dllexport)
#endif
VMINFO const vmiMac =
{
    NULL,
    0,
    "Macintosh Classic",
#if defined(SOFTMAC)
    vmwMac1,
    cpu68000 | cpu68010  | cpu68020 | cpu68030,
    ram512K  | ram1M     | ram2M     | ram2_5M | ram4M,
    osMac_64 | osMac_128 | osMac_256 | osMac_512,
#else
    vmwMac0,
    cpu68000,
    ram512K  | ram1M     | ram2M     | ram2_5M | ram4M,
    osMac_64 | osMac_128,
#endif
    monMacMon,
    mac_Install,
    mac_Init,
    mac_UnInit,
    mac_InitDisks,
    mac_MountDisk,
    mac_UnInitDisks,
    mac_UnmountDisk,
    mac_stub,
    mac_Reset,
    mac_Exec,
    mac_stub,
    mac_WinMsg,
    m68k_DumpRegs,
    mac_DumpHW,
    mac_stub,
    mac_ReadHWByte,
    mac_ReadHWWord,
    mac_ReadHWLong,
    mac_WriteHWByte,
    mac_WriteHWWord,
    mac_WriteHWLong,
    mac_LockBlock,
    mac_UnlockBlock,
    mac_MapAddress,
    mac_MapAddressRW,
};

#if defined(SOFTMAC2)

#ifdef MAKEDLL
_declspec(dllexport)
#endif
VMINFO const vmiMacII =
{
    NULL,
    0,
    "Macintosh II",
    vmwMac2,
    cpu68020 | cpu68030 | cpu68040,
#if !defined(DEMO)
    ~(ram8M-1),                     // select all RAM 8M and higher
    osMac_II | osMac_IIi | osMac_1M,
    ColorMacModes,
#else // DEMO
    DemoMacMem,
    osMac_II | osMac_IIi | osMac_1M,
    DemoMacModes,
#endif
    mac_Install,
    mac_Init,
    mac_UnInit,
    mac_InitDisks,
    mac_MountDisk,
    mac_UnInitDisks,
    mac_UnmountDisk,
    mac_stub,
    mac_Reset,
    mac_Exec,
    mac_stub,
    mac_WinMsg,
    m68k_DumpRegs,
    mac_DumpHW,
    mac_stub,
    mac_ReadHWByte,
    mac_ReadHWWord,
    mac_ReadHWLong,
    mac_WriteHWByte,
    mac_WriteHWWord,
    mac_WriteHWLong,
    mac_LockBlock,
    mac_UnlockBlock,
    mac_MapAddress,
    mac_MapAddressRW,
};

#ifdef MAKEDLL
_declspec(dllexport)
#endif
VMINFO const vmiMacQdra =
{
    NULL,
    0,
    "Macintosh Quadra",
#ifdef POWERMAC
    vmwMacU |
#endif
    vmwMac4 | vmMacQdra,
#ifdef POWERMAC
    cpu601 |
#endif
    cpu68020 | cpu68030 | cpu68040,
#if !defined(DEMO)
    ~(ram8M-1),                   // select all RAM 8M and higher
    osMac_IIi | osMac_1M | osMac_2M | osMac_G1,
    ColorMacModes,
#else // DEMO
    DemoMacMem,
    osMac_IIi | osMac_1M | osMac_2M,
    DemoMacModes,
#endif
    mac_Install,
    mac_Init,
    mac_UnInit,
    mac_InitDisks,
    mac_MountDisk,
    mac_UnInitDisks,
    mac_UnmountDisk,
    mac_stub,
    mac_Reset,
    mac_Exec,
    mac_stub,
    mac_WinMsg,
    m68k_DumpRegs,
    mac_DumpHW,
    mac_stub,
    mac_ReadHWByte,
    mac_ReadHWWord,
    mac_ReadHWLong,
    mac_WriteHWByte,
    mac_WriteHWWord,
    mac_WriteHWLong,
    mac_LockBlock,
    mac_UnlockBlock,
    mac_MapAddress,
    mac_MapAddressRW,
};

#ifdef POWERMAC

#ifdef MAKEDLL
_declspec(dllexport)
#endif
VMINFO const vmiMacPowr =
{
    NULL,
    0,
    "Power Macintosh",
    vmwMacG1,
    cpu601 | cpu603 | cpu604,
#if !defined(DEMO)
    ~(ram8M-1),                   // select all RAM 8M and higher
    osMac_G1,
    ColorMacModes,
#else // DEMO
    DemoMacMem,
    osMac_G1,
    DemoMacModes,
#endif
    mac_Install,
    mac_Init,
    mac_UnInit,
    mac_InitDisks,
    mac_MountDisk,
    mac_UnInitDisks,
    mac_UnmountDisk,
    mac_stub,
    mac_Reset,
    mac_Exec,
    mac_stub,
    mac_WinMsg,
    mppc_DumpRegs,
    mac_DumpHW,
    mac_stub,
    mac_ReadHWByte,
    mac_ReadHWWord,
    mac_ReadHWLong,
    mac_WriteHWByte,
    mac_WriteHWWord,
    mac_WriteHWLong,
    mac_LockBlock,
    mac_UnlockBlock,
    mac_MapAddress,
    mac_MapAddressRW,
};

#ifdef MAKEDLL
_declspec(dllexport)
#endif
VMINFO const vmiMacG3 =
{
    NULL,
    0,
    "Power Macintosh G3",
    vmwMacG3,
    cpuPPCG3 | cpuPPCG4,
#if !defined(DEMO)
    ~(ram8M-1),                   // select all RAM 8M and higher
    osMac_G3,
    ColorMacModes,
#else // DEMO
    DemoMacMem,
    osMac_G3,
    DemoMacModes,
#endif
    mac_Install,
    mac_Init,
    mac_UnInit,
    mac_InitDisks,
    mac_MountDisk,
    mac_UnInitDisks,
    mac_UnmountDisk,
    mac_stub,
    mac_Reset,
    mac_Exec,
    mac_stub,
    mac_WinMsg,
    mppc_DumpRegs,
    mac_DumpHW,
    mac_stub,
    mac_ReadHWByte,
    mac_ReadHWWord,
    mac_ReadHWLong,
    mac_WriteHWByte,
    mac_WriteHWWord,
    mac_WriteHWLong,
    mac_LockBlock,
    mac_UnlockBlock,
    mac_MapAddress,
    mac_MapAddressRW,
};

#endif // POWERMAC

#endif // SOFTMAC2

BOOL __cdecl mac_stub()
{
    return TRUE;
}


//
// Macintosh virtual machine hardware definitions
//
// One physical hardware description for each unique Mac
// We handle 24-bit / 32-bit mapping and virtual memory separately
//
// Define common memory descriptors for common hardware between models.
//

// $5xxxxxxx hardware memory map for Mac II class machines

MMD mm_hwII[] =
 {
 { 0x0000000,   MEM_END },
 };

// memory map for $5xxxxxxx in VIA2 based machines (Mac II, Quadra 900, etc)

MMD mm_hwIIci[] =
 {
 { 0x0000000,   MEM_END },
 };

MMD mm_NuBus[] =
 {
 { 0x0000000,   MEM_END },
 };

//
// main machine memory map definitions
//

MMD mm_II[] =
 {
 { 0x00000000,  MEM_RAMB  | MEM_NORMAL | MEM_ALL | MEM_MEGS(64) },
 { 0x00000000,  MEM_RAMA  | MEM_NORMAL | MEM_ALL | MEM_MEGS(64) },
 { 0x04000000,  MEM_RAMA  | MEM_NORMAL | MEM_ALL | MEM_MEGS(1024-128) },
 { 0x00000000,  MEM_ROM   | MEM_NORMAL | MEM_ALL | MEM_MEGS(256) },
 { &mm_hwII,    MEM_ALIAS | MEM_NORMAL | MEM_ALL | MEM_MEGS(256) },
 { &mm_NuBus,   MEM_ALIAS | MEM_NORMAL | MEM_ALL | MEM_MEGS(256*10) },
 { 0x0000000,   MEM_END },
 };


MMD mm_IIci[] =
 {
 { 0x00000000,  MEM_RAMB  | MEM_NORMAL | MEM_ALL | MEM_MEGS(64) },
 { 0x00000000,  MEM_RAMA  | MEM_NORMAL | MEM_ALL | MEM_MEGS(64) },
 { 0x04000000,  MEM_RAMA  | MEM_NORMAL | MEM_ALL | MEM_MEGS(1024-128) },
 { 0x00000000,  MEM_ROM   | MEM_NORMAL | MEM_ALL | MEM_MEGS(256) },
 { &mm_hwIIci,  MEM_ALIAS | MEM_NORMAL | MEM_ALL | MEM_MEGS(256) },
 { &mm_NuBus,   MEM_ALIAS | MEM_NORMAL | MEM_ALL | MEM_MEGS(256*10) },
 { 0x0000000,   MEM_END },
 };


#ifdef WORMHOLE
extern char *vszDiskImg;
#endif

BOOL __cdecl mac_Install(PVMINFO pvmi, PVM pvm)
{
	pvm->pvmi = pvmi;
	pvm->bfHW = BfFromWfI(pvmi->wfHW, 0);
	pvm->bfCPU = BfFromWfI(pvmi->wfCPU, 0);
	pvm->iOS = -1;
	pvm->bfMon = BfFromWfI(pvmi->wfMon, 0);
	pvm->bfRAM  = BfFromWfI(pvmi->wfRAM, 0);
	pvm->ivdMac = sizeof(v.rgvm[0].rgvd)/sizeof(VD);
	pvm->fUseVHD = fTrue;

    pvm->rgvd[0].dt = DISK_FLOPPY;

#ifdef WORMHOLE
    pvm->rgvd[1].dt = DISK_IMAGE;
    strcpy(pvm->rgvd[1].sz, vszDiskImg);
#endif

    pvm->fValidVM = fTrue;

    return TRUE;
}

void SetMacMemMode(BOOL f32)
{
    // WARNING! Don't do any debug output here that dumps memory
    // since we are in the middle of changing the memory state!!

#ifndef NDEBUG
    fDebug++;
    printf("MODE SWITCH BEFORE, PC = %08X, lAddrMask = %08X\n",
         vpregs->PC, vsthw.lAddrMask);
      //  FDumpRegsVM();
#endif

    if (f32)
        {
        // switching to 32-bit addressing

        vsthw.lAddrMask = 0xFFFFFFFF; // 32-bit addressing

        vmachw.rgvia[1].vBufB |= 0x08;
        }
    else
        {
        // switching to 24-bit addressing

        vsthw.lAddrMask = 0x00FFFFFF; // 24-bit addressing

        vmachw.rgvia[1].vBufB &= ~0x08;
        }


#ifndef NDEBUG
    printf("MODE SWITCH AFTER,  PC = %08X, lAddrMask = %08X\n",
         vpregs->PC, vsthw.lAddrMask);
    fDebug--;
#endif

    // invalidate the page table, minus the first 8 meg of memory
    // which won't change between 24-bit and 32-bit modes

    InvalidatePages(0x00800000, 0xFFFFFFFF);
}

BOOL __cdecl mac_Init()
{
	int i;
	BOOL f;
    int version = osCur.version;
    char szMacPRAM[MAX_PATH];
    ULONG pram;

	SetCurrentDirectory(vi.szDefaultDir);

    // Wipe away any stale 680x0 state

    memset((BYTE *)&vregs,  0, sizeof(REGS));
    memset((BYTE *)&vi.regs2, 0, sizeof(REGS2));

    vi.eaRAM[0] = 0;
    vi.cbRAM[0] = 0;
    vi.eaRAM[1] = 0;
    vi.cbRAM[1] = 0;
    vi.eaROM[0] = 0;
    vi.cbROM[0] = 0;
    vi.eaROM[1] = 0;
    vi.cbROM[1] = 0;

    // HACK: so that video frame address can be set

    vmachw.sVideo = 0x0E;

    // pick a default hardware

	vmCur.bfHW = BfFromWfI(vmCur.pvmi->wfHW, 0);

Lunregistered:

    // all color Macs (i.e. NuBus based) handled here

    if (FIsMacNuBus(vmCur.bfHW))
        {
        // Power Macintosh G3          - 4M ROMs
        // Power Macintosh 601         - 4M ROMs
        // Macintosh Quadra AV         - 2M ROMs
        // Macintosh Quadra Centris    - 1M ROMs
        // Macintosh LC LCII IIci IIsi - 512K ROMs
        // Macintosh II IIx IIcx SE/30 - 256K ROMs

        vi.eaROM[0]     = 0x40800000;
        vi.cbROM[0]     = osCur.cbOS;
        vi.cbRAM[0]     = CbRamFromBf(vmCur.bfRAM);
        vi.cbRAM[1]     = 0x00080000;
#if HIRES
        vi.eaRAM[1]     = 0xF0000000 + 0x01000000 * vmachw.sVideo;
#else
        vi.eaRAM[1]     = 0xF0000000 + 0x01100000 * vmachw.sVideo;
#endif

        if ((version >= 124) && (vmCur.pvmi == &vmiMacQdra))
            {
            if ((vi.cbROM[0] >= 0x200000) || v.fNoQckBoot)
                {
                // 2M and 4M ROMs do not know about the IIci
                // and we can't use Christian Bauer's trick
                // so default to a Quadra model

                if (vi.cbROM[0] < 0x100000)
                    {
                    // All 512K ROMs know about the fake Quadra (really a IIci)

                    vmCur.bfHW = vmMacQdra;
                    }
                else if (vi.cbROM[0] < 0x200000)
                    {
                    // All 1M ROMs know about the ????

                    vmCur.bfHW = vmMacQdra;
                    }
                else
                    {
                    // All 2M ROMs know about the Quadra 700???

                    vmCur.bfHW = vmMacQ650;
                    }
                }
            }

#ifndef DEMO
        if (vmCur.fCPUAuto)
#endif
            {
#ifdef POWERMAC
            if (FIsMacPPC(vmCur.bfHW))
                vmCur.bfCPU = cpu601;
            else
#endif
            if (FIsMac68040(vmCur.bfHW) || (vmCur.bfHW == vmMacQdra))
                vmCur.bfCPU = cpu68040;
            else if (FIsMac68030(vmCur.bfHW))
                vmCur.bfCPU = cpu68030;
            else
                vmCur.bfCPU = cpu68020;
            }

#if HIRES
#else
        if (FIsMac32(vmCur.bfHW))
            {
            // add some extra video RAM to 32-bit clean Macs
            // must match the patch in PatchToby!

            vi.cbRAM[1] += 0x00200000;
            vi.eaRAM[1] -= 0x00200000;
            }
#endif
        }

    // 68000 based Macs

    else if (version >= 118)
        {
        // Macintosh SE - 256K ROMs
        // Macintosh Classic - 512K ROMs

        vmCur.bfHW      = vmMacSE;
        vi.eaROM[0]     = 0x400000;
        vi.cbROM[0]     = osCur.cbOS;  // handles 512K Classic ROM
        vi.cbRAM[0]     = max(CbRamFromBf(vmCur.bfRAM), 1024*1024);
        }
    else if (version >= 117)
        {
        // Macintosh Plus - 128K ROMs

        vmCur.bfHW      = vmMacPlus;
        vi.eaROM[0]     = 0x400000;
        vi.cbROM[0]     = 65536 * 2;
        vi.cbRAM[0]     = max(CbRamFromBf(vmCur.bfRAM), 1024*1024);
        }
    else
        {
        // Macintosh 512 - 64K ROMs

        vmCur.bfHW      = vmMacPlus;
        vi.eaROM[0]     = 0x400000;
        vi.cbROM[0]     = 65536;
        vi.cbRAM[0]     = 512*1024;
        vmCur.bfRAM     = ram512K;
        }

    // Don't allow illegal CPU type

    if ((vmCur.bfCPU & vmCur.pvmi->wfCPU) == 0)
        vmCur.bfCPU = BfFromWfI(vmCur.pvmi->wfCPU, 0);

#ifdef GEMPRO
#ifndef DEMO
#ifndef BETA
    // validate RAM and monitor settings with decrypted key

    vmCur.bfRAM &= ram512K | ram1M | ram2M | ram4M | DemoMacMem |
             ((signed char)(vpes->valid1[0] * vpes->valid2[0]));
#endif // !BETA
#endif // !DEMO

    // if RAM setting is invalid, pick the 3rd smallest RAM setting
    // which at this point is legal in all modes: 2M in Classic mode
    // and 14M in Mac II modes

    if (vmCur.bfRAM == 0)
        {
        vmCur.bfRAM = BfFromWfI(vmCur.pvmi->wfRAM, 2);
        goto Lunregistered;
        }

#ifndef DEMO
#ifndef BETA
    vmCur.bfMon &= DemoMacModes |
                   ((signed char)(vpes->valid1[0] * vpes->valid2[0]));
#endif // !BETA
#endif // !DEMO

    // if monitor setting is invalid, pick the smallest most (512x384)
    // then try if 640x480 mode is also available

    if (vmCur.bfMon == 0)
        {
        vmCur.bfMon = BfFromWfI(vmCur.pvmi->wfMon, 0);

        if (BfFromWfI(vmCur.pvmi->wfMon, 1))
            vmCur.bfMon = BfFromWfI(vmCur.pvmi->wfMon, 1);

        goto Lunregistered;
        }

#ifndef DEMO
#ifndef BETA
    // check some arbitrary digit of the key, if it's blank
    // then disable sound and printing!

    if (!vpes->key[3])
        {
        vmCur.iLPT = 0;
        vmCur.fSound = 0;
        }
#endif // !BETA
#endif // !DEMO
#endif // GEMPRO

    // in 68040 mode, we have to emulate a 68030 and set fake 030 flag

    vi.fFake040 = FALSE;

#if 1
//    if (vi.cbROM[0] < 0x100000)
     if (vmCur.bfCPU == cpu68040)
      if (!v.fNoQckBoot)
        {
        vi.fFake040 = TRUE;
        vmCur.bfCPU = cpu68030;

#ifdef BETA
        printf("faking 68040\n");
#endif
        }
#endif

	// vi.cbRAM[0], vi.cbROM[0], and vi.eaROM[0] must have already been set

    if (FIsMacNuBus(vmCur.bfHW))
        {
        // make sure the CPU isn't somehow set to 68000

        if (vmCur.bfCPU < cpu68020)
            vmCur.bfCPU = cpu68020;
        }

	// Unprotect the RAM and video RAM memory blocks and also
	// unprotect ROMs (at least 256K for Mac Plus detection to work)

    f = memAllocAddressSpace(TRUE);

#if !defined(NDEBUG) || defined(BETA)
    printf("vpregs:%08X, cb:%08X, pbRAM[0]:%08x, pbROM[0]:%08X, pbVID:%08X\n",
             vpregs, k4M, vi.pbRAM[0], vi.pbROM[0], vi.pbRAM[1]);
#endif

    if (!f)
		{
#ifndef WORMHOLE
		MessageBox(NULL, "Unable to allocate Macintosh memory. "
			"Reduce the Memory setting or increase the Windows swap file size.",
			vi.szAppName, MB_OK);
#endif
        return FALSE;
        }

#if !defined(NDEBUG) || defined(BETA)
	vi.pProfile = malloc(65536 * sizeof(DWORD));    // profile array;
	vi.pHistory = malloc(MAX_HIST*4);               // history array;

    if (NULL != vi.pProfile)
        memset(vi.pProfile, 0, 65536 * sizeof(DWORD));

    if (NULL != vi.pHistory)
        memset(vi.pHistory, 0, MAX_HIST*4);
#endif

    // Initialize the CPU

#ifdef POWERMAC
    if ((vmCur.bfCPU & cpu601))
        vpci = &cpiPPC;
    else
#endif
        vpci = &cpi68K;

	vpci->pfnInit(
        vpregs,
        (ULONG)mac_ReadHWByte,
        (ULONG)mac_ReadHWWord,
        (ULONG)mac_WriteHWByte,
        (ULONG)mac_WriteHWWord,
        (ULONG)mac_VPollInterrupts,
        (ULONG)mac_Reset,
        vmCur.bfCPU);

#ifdef POWERMAC
    if (FIsCPUAny68K(vmCur.bfCPU))
#endif
    {
    vpci->rgfExcptHook[0] =  0;
    vpci->rgfExcptHook[0] |= 0x0004; // hook Bus Error vector
    vpci->rgfExcptHook[0] |= 0x0008; // hook Address Error vector
    vpci->rgfExcptHook[0] |= 0x0010; // hook Illegal Instr. vector
    vpci->rgfExcptHook[0] |= 0x0400; // hook Line A vector

    if (FIsMacADB(vmCur.bfHW))
        {
        vpci->rgfExcptHook[0] |= 0x0800; // hook Line F vector
        vpci->rgfExcptHook[0] |= 0x0020; // hook Div by 0 vector
        }

//    vpci->rgfExcptHook[0] |= 0x0FFF; // hook all vectors

    vpci->rgfExcptHook[1] =  0;
    }

    if (FIsMacNuBus(vmCur.bfHW))
        SetMacMemMode(TRUE);
    else
        SetMacMemMode(FALSE);

    // Initialize the ROMs

    if (!FInstallROMs())
		{
#ifndef WORMHOLE
		MessageBox(NULL, "Unable to install ROMs.",
			vi.szAppName, MB_OK);
#endif
		return FALSE;
		}

    // REVIEW: patch ROMs here

    // Start the PC at the start of the ROMs

    vpregs->PC = vmPeekL(vi.eaROM[0] + 4) | vi.eaROM[0];
    vpregs->A7 = 0x40000;
    vpregs->USP = 0x40000;
    vpregs->SSP = 0x40000;

#ifdef POWERMAC
    if ((vmCur.bfCPU & cpu601))
        {
        vpregs->PC = 0x40800000 + (0xFFF00100 & (vi.cbROM[0] - 1));
        }
    else
#endif

    // This is a hack to help fool the Mac II 256K ROM boot code
    // A6 = memtop
    // D7 = CPU, 2  = 68020, 3 = 68030, 4 = 68040
    // Jump to $4080009A on Mac II ROMs to skip hardware check
    // This is necessary for >128M of RAM

    if (FIsMac24(vmCur.bfHW) && (vi.cbROM[0] == 256*1024))
        {
        if (vi.cbRAM[0] > 8*1024*1024)
            {
            vpregs->PC = 0x4080009A;
            vpregs->A6 = vi.cbRAM[0];

            // HACK: Pretend it's a 68030 when >8M

            if (vmCur.fCPUAuto)
                {
                switch (PeekL(vi.eaROM[0]))
                    {
                default:
                    vmCur.bfCPU = cpu68030;
                    vpregs->D7 = 3;
                    break;

                case 0x97851DB6:                // Mac II 1.0
                case 0x9779D2C4:                // Mac II 1.2
                    vmCur.bfCPU = cpu68020;
                    vpregs->D7 = 2;
                    break;
                    }
                }
            }
        }

    else if (FIsMac24(vmCur.bfHW) && (vi.cbROM[0] > 256*1024))
        {
        // Universal ROMs use PMOVE in the boot code when running
        // as a Mac II, so force CPU to be a 68030

        if (vmCur.fCPUAuto)
            vmCur.bfCPU     = cpu68030;

        vmCur.bfHW = vmMacIIcx;
        }

    else if (PeekL(vi.eaROM[0] + 4) == 0x0000002A)
        {
        vpregs->PC = 0x4080002A;

        // HACK: LCII ROMs default to 68020/68040 bacause broken on 68030
        // Same for Mac Classic II ROMs

        if (vi.fFake040 || (vmCur.fCPUAuto))
            {
            switch (PeekL(vi.eaROM[0]))
                {
            default:
Lcheck030:
                if ((vi.cbROM[0] < 0x100000) && (vmCur.bfCPU > cpu68030))
                    vmCur.bfCPU = cpu68030;

                break;

            case 0x35C28F5F:                // LC II
            case 0x3193670E:                // Classic II
                vmCur.bfCPU = cpu68020;
                break;
                }
            }
        }

    else if (FIsMac32(vmCur.bfHW))
        {
#ifndef DEMO
        if (vi.fFake040 || (vmCur.fCPUAuto))
#endif
            goto Lcheck030;
        }

    // Re-initialize the CPU as necessary

    vpci->pfnInit(
        vpregs,
        (ULONG)mac_ReadHWByte,
        (ULONG)mac_ReadHWWord,
        (ULONG)mac_WriteHWByte,
        (ULONG)mac_WriteHWWord,
        (ULONG)mac_VPollInterrupts,
        (ULONG)mac_Reset,
        vmCur.bfCPU);

#ifdef POWERMACx
    InitOpcodesPPC(&vpregs->opcodesRW[0]);
#endif

    if (FIsMacNuBus(vmCur.bfHW))
        SetMacMemMode(TRUE);

    // Initialize the SR lookup table, then initialize the SR.
    // IN THAT ORDER! Since lookup table address stored in upper SR word

    vpregs->SR = 0x2700;

    // Initialize disk image files

    mac_InitDisks();

    // Initialize the VM

    sprintf(szMacPRAM, "%s\\%08X.PRM", vi.szWindowsDir, vmPeekL(vi.eaROM[0]));

	SetCurrentDirectory(vi.szWindowsDir);
	CbReadWholeFile(szMacPRAM, 256, &vmachw.rgbCMOS);
	mac_Reset();

#ifndef NDEBUG 
	ClearProfile();

	DebugStr("pp = %08X, ph = %08X\n", vi.pProfile, vi.pHistory);
	DebugStr("vi.cbROM[0]    = %08X  ", vi.cbROM[0]);
	DebugStr("vi.cbRAM[0]    = %08X  ", vi.cbRAM[0]);
	DebugStr("vi.pregs    = %08X  ", vi.pregs);
	DebugStr("vi.pbRAM[0]    = %08X\n", vi.pbRAM[0]);

	DebugStr("PC = %08X  ", vpregs->PC);
	DebugStr("SR = %08X  ", vpregs->SR);
	DebugStr("A7 = %08X\n", vpregs->A7);

	DebugStr("&D0 = %08X\n", &vpregs->D0);
#endif

    MarkAllPagesDirty();

	return TRUE;
}


BOOL __cdecl mac_UnInit()
{
    mac_UnInitDisks();

    if (vpregs && vi.pbRAM[0] && vi.pbROM[0])
        {
        char szMacPRAM[MAX_PATH];

        if (vi.fFake040 && !vmCur.fCPUAuto)
            vmCur.bfCPU = cpu68040;

        SetCurrentDirectory(vi.szWindowsDir);
		FWriteWholeFile(szMacPRAM, 256, &vmachw.rgbCMOS);

        memFreeAddressSpace();
        }

    vi.fFake040 = FALSE;

    return TRUE;
}


//
// Set up the disk image files. This function is separate from the Init routine so
// that it can be called on the fly as the user changes disk settings on the fly.
//

BOOL __cdecl mac_MountDisk(int i)
{
    PVD pvd = &vmCur.rgvd[i];

	// open VHD files. Try read-write first, then try read-only
	// REVIEW: use registry

    SetCurrentDirectory(vi.szDefaultDir);

    if (vi.rgpdi[i] == NULL)
        {
        // when opening as a floppy test the file system
        // in case of boot partitions that need to be skipped

        if (pvd->dt == DISK_IMAGE)
            {
            vi.rgpdi[i] = PdiOpenDisk(DISK_IMAGE, pvd->sz,
                 ((i >= 2) ? DI_QUERY : 0) | (pvd->mdWP ? DI_READONLY : 0));

            // Check for CD-ROM disk image and set cbSector accordingly

            if (vi.rgpdi[i] && (i >= 2))
                {
                BYTE rgch[512*3];

LcheckCD:
                vi.rgpdi[i]->pdiStub = NULL;
                vi.rgpdi[i]->cSecStub = 0;

                vi.rgpdi[i]->sec = 0;
                vi.rgpdi[i]->count = 3;
                vi.rgpdi[i]->lpBuf = rgch;

                if (pvd->mdWP == 2)
                    {
LsetCD:
                    pvd->mdWP = 2;
                    vi.rgpdi[i]->fWP = fTrue;
                    vi.rgpdi[i]->cbSector = 2048;
                    }

                else if (FRawDiskRWPdi(vi.rgpdi[i], 0))
                    {
                    // check for a bootable Mac OS CD-ROM

                    if (rgch[0] == 0x45 && rgch[1] == 0x52 && rgch[2] == 0x08)
                        goto LsetCD;

                    // check for a true SCSI hard disk (no stub required)

                    else if (rgch[0] == 0x45 && rgch[1] == 0x52 && rgch[2] == 0x02 && rgch[17] == 0x01)
                        ;   // System 7 formatted

                    // check for a true SCSI hard disk (no stub required)

                    else if (rgch[0] == 0x45 && rgch[1] == 0x52 && rgch[2] == 0x02 && rgch[17] == 0x02)
                        ;   // Mac OS 8 formatted

                    // Also check for HFX file

                    else if (rgch[0] == 0x4C && rgch[1] == 0x4B && rgch[2] == 0x60)
                        {
#if !defined(NDEBUG) || defined(BETA)
                        printf("opening bootable HFX stub file!\n");
#endif
                        vi.rgpdi[i]->pdiStub = PdiOpenDisk(DISK_IMAGE,
                             "HFX_STUB.DLL", DI_QUERY | DI_READONLY);

Lcheckstub:
                        if (vi.rgpdi[i]->pdiStub)
                            {
                            vi.rgpdi[i]->cSecStub = 
                                vi.rgpdi[i]->pdiStub->size * 2;

#if !defined(NDEBUG) || defined(BETA)
                            printf("stub file %s opened, size = %d sectors\n",
                                vi.rgpdi[i]->pdiStub->szImage,
                                vi.rgpdi[i]->pdiStub->size * 2);
#endif
                            }
                        }
                    else if (rgch[1024] == 0x42 && rgch[1025] == 0x44)
                        {
#if !defined(NDEBUG) || defined(BETA)
                        printf("opening non-bootable HFX stub file!\n");
#endif
                        vi.rgpdi[i]->pdiStub = PdiOpenDisk(DISK_IMAGE,
                             "HFX_STUB.DLL", DI_QUERY | DI_READONLY);
                        // vi.rgpdi[i]->offsec += 2;

                        goto Lcheckstub;
                        }
                    }
                }
            }
        else if (pvd->dt == DISK_FLOPPY)
            {
            vi.rgpdi[i] = PdiOpenDisk(DISK_FLOPPY, pvd->id,
                    (pvd->mdWP ? DI_READONLY : 0) | DI_QUERY);

            if (vi.rgpdi[i] && (i >= 2))
                goto LcheckCD;
            }
        else if (pvd->dt == DISK_SCSI)
            {
            vi.rgpdi[i] = PdiOpenDisk(DISK_SCSI,
                 LongFromScsi(pvd->ctl, pvd->id),
                    (pvd->mdWP ? DI_READONLY : 0) | (i >= 2) ? DI_QUERY : 0);

            if (vi.rgpdi[i] && (i >= 2))
                goto LcheckCD;
            }
        else if (pvd->dt == DISK_WIN32)
            {
            vi.rgpdi[i] = PdiOpenDisk(DISK_WIN32,
                 LongFromScsi(pvd->ctl, pvd->id),
                 // (pvd->mdWP ? DI_READONLY : 0) | (i >= 2) ? DI_QUERY : 0);
                    DI_READONLY | DI_QUERY);

            if (vi.rgpdi[i] && (i >= 2))
                goto LcheckCD;
            }
        }

    return (vi.rgpdi[i] != NULL);
}

BOOL __cdecl mac_InitDisks()
{
    int i;

    for (i = 0; i < vmCur.ivdMac; i++)
        {
        if (vi.rgpdi[i])
            CloseDiskPdi(vi.rgpdi[i]);
        vi.rgpdi[i] = NULL;
        mac_MountDisk(i);
        }

    return TRUE;
}

//
// Close any open disk image file handles
//

BOOL __cdecl mac_UnmountDisk(int i)
{
    if (vi.rgpdi[i])
        CloseDiskPdi(vi.rgpdi[i]);

    vi.rgpdi[i] = NULL;

    return TRUE;
}

BOOL __cdecl mac_UnInitDisks()
{
    int i;

    for (i = 0; i < vmCur.ivdMac; i++)
        mac_UnmountDisk(i);

    return TRUE;
}


BOOL __cdecl mac_LockBlock(ULONG ea, ULONG cb, void **ppv)
{
    __asm int 3;
    return FALSE;
}

BOOL __cdecl mac_UnlockBlock(ULONG ea, ULONG cb)
{
    __asm int 3;
    return TRUE;
}


BOOL __cdecl mac_DumpHW(void)
{
    int i;
        {
        printf ("\n\nVIA regs: ");

#if LATER
		for (i = 0; i < 16; i++)
            {
            printf("%02X ", vmachw.rgvia[0].rgbVIA[i]);
            }
#endif

        printf("\n");
        printf("VIA BufA: %02X  DirA: %02X  BufB: %02X  DirB: %02X  SR: %02X\n",
            vmachw.rgvia[0].vBufA, vmachw.rgvia[0].vDirA,
            vmachw.rgvia[0].vBufB, vmachw.rgvia[0].vDirB,
            vmachw.rgvia[0].vSR );

        printf("VIA PCR:  %02X  ACR:  %02X  IFR:  %02X  IER:  %02X\n",
            vmachw.rgvia[0].vPCR, vmachw.rgvia[0].vACR, vmachw.rgvia[0].vIFR, vmachw.rgvia[0].vIER );

        printf("VIA data register A:\n");
        printf("  %c %x  SCC Wait/Request\n",
            (vmachw.rgvia[0].vDirA & 0x80) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufA & 0x80)
            );
        printf("  %c %x  0 = alt, 1 = main screen buffer\n",
            (vmachw.rgvia[0].vDirA & 0x40) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufA & 0x40)
            );
        printf("  %c %x  Floppy SEL\n",
            (vmachw.rgvia[0].vDirA & 0x20) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufA & 0x20)
            );
        printf("  %c %x  ROM overlay\n",
            (vmachw.rgvia[0].vDirA & 0x10) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufA & 0x10)
            );
        printf("  %c %x  0 = alt, 1 = main sound buffer\n",
            (vmachw.rgvia[0].vDirA & 0x08) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufA & 0x08)
            );
        printf("  %c %x  sound volume\n",
            (vmachw.rgvia[0].vDirA & 0x04) ? 'O' : 'I',
            (vmachw.rgvia[0].vBufA & 0x07) // 3 bits
            );

        printf("VIA data register B:\n");
        printf("  %c %x  0 = sound enabled\n",
            (vmachw.rgvia[0].vDirB & 0x80) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x80)
            );
        printf("  %c %x  0 = video beam visible\n",
            (vmachw.rgvia[0].vDirB & 0x40) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x40)
            );
        printf("  %c %x  Mouse Y2\n",
            (vmachw.rgvia[0].vDirB & 0x20) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x20)
            );
        printf("  %c %x  Mouse X2\n",
            (vmachw.rgvia[0].vDirB & 0x10) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x10)
            );
        printf("  %c %x  0 = mouse switch down\n",
            (vmachw.rgvia[0].vDirB & 0x08) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x08)
            );
        printf("  %c %x  0 = rtc enabled\n",
            (vmachw.rgvia[0].vDirB & 0x04) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x04)
            );
        printf("  %c %x  rtc clock\n",
            (vmachw.rgvia[0].vDirB & 0x02) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x02)
            );
        printf("  %c %x  rtc data\n",
            (vmachw.rgvia[0].vDirB & 0x01) ? 'O' : 'I',
            !!(vmachw.rgvia[0].vBufB & 0x01)
            );

        printf("VIA Interrupt Flag register:\n");
        printf("  %x  any IRQ\n", !!(vmachw.rgvia[0].vIFR & 0x80));
        printf("  %x  T1\n", !!(vmachw.rgvia[0].vIFR & 0x40));
        printf("  %x  T2\n", !!(vmachw.rgvia[0].vIFR & 0x20));
        printf("  %x  keyboard clock\n", !!(vmachw.rgvia[0].vIFR & 0x10));
        printf("  %x  keyboard data\n", !!(vmachw.rgvia[0].vIFR & 0x08));
        printf("  %x  keyboard data ready\n", !!(vmachw.rgvia[0].vIFR & 0x04));
        printf("  %x  VBI\n", !!(vmachw.rgvia[0].vIFR & 0x02));
        printf("  %x  One second\n", !!(vmachw.rgvia[0].vIFR & 0x01));

        printf("VIA Interrupt Enable register:\n");
        printf("  %x  any IRQ\n", !!(vmachw.rgvia[0].vIER & 0x80));
        printf("  %x  T1\n", !!(vmachw.rgvia[0].vIER & 0x40));
        printf("  %x  T2\n", !!(vmachw.rgvia[0].vIER & 0x20));
        printf("  %x  keyboard clock\n", !!(vmachw.rgvia[0].vIER & 0x10));
        printf("  %x  keyboard data\n", !!(vmachw.rgvia[0].vIER & 0x08));
        printf("  %x  keyboard data ready\n", !!(vmachw.rgvia[0].vIER & 0x04));
        printf("  %x  VBI\n", !!(vmachw.rgvia[0].vIER & 0x02));
        printf("  %x  One second\n", !!(vmachw.rgvia[0].vIER & 0x01));

        printf ("\n\nSCC regs:\n");

        printf("  SCC B (print) RR: ");
		for (i = 0; i < 16; i++)
            {
            printf("%02X ", vmachw.aSCC[0].RR[i]);
            }

        printf("\n  SCC B (print) WR: ");
		for (i = 0; i < 16; i++)
            {
            printf("%02X ", vmachw.aSCC[0].WR[i]);
            }

        printf("\n  SCC A (modem) RR: ");
		for (i = 0; i < 16; i++)
            {
            printf("%02X ", vmachw.aSCC[1].RR[i]);
            }

        printf("\n  SCC A (modem) WR: ");
		for (i = 0; i < 16; i++)
            {
            printf("%02X ", vmachw.aSCC[1].WR[i]);
            }

        printf ("\n\nIWM state: ");

		for (i = 0; i < 8; i++)
            {
            printf("%1X ", vmachw.rgbIWMCtl[i]);
            }

        printf ("\n\nSCSI read state: ");

		for (i = 0; i < 8; i++)
            {
            printf("%1X ", vmachw.rgbSCSIRd[i]);
            }

        printf ("\n\nSCSI write state: ");

		for (i = 0; i < 8; i++)
            {
            printf("%1X ", vmachw.rgbSCSIWr[i]);
            }
        }
	printf("\n\n\n");

    return TRUE;
}


BOOL __cdecl mac_Exec(fStep, fCont)
int fStep;
int fCont;
{
	ULONG tickStart, ticks;  // used to time the exeuction time of this function
	ULONG stat;
    MSG msg;

#if 0
	vi.countR = 0;
	vi.countW = 0;
#endif
	vi.cFreq  = 0;

	/* valid combinations of fStep and fCont
	 *
	 * fStep=0 fCont=0
	 * fStep=0 fCont=1 = execute
	 * fStep=1 fCont=0 = single step
	 * fStep=1 fCont=1 = trace
	 *
	 */

	if (!fStep)
		{
        static int cips = 3000;
        static ULONG lTick;         // current reading of millisecond timer
        static ULONG lTick1000;     // previous reading of millisecond timer

		/* non-tracing execution loop */

//		vrgf.fRefreshScreen = TRUE;

        QueryTickCtr();
		tickStart = Getms();

		if (vi.fMouseMoved && vi.fHaveFocus && (vi.fGEMMouse || !vi.fVMCapture))
			{
			POINT pt;

//					DebugStr("m");
			vi.fMouseMoved = FALSE;
			GetCursorPos(&pt);
			ScreenToClient(vi.hWnd, &pt);
			FMouseMsgST(vi.hWnd, WM_MOUSEMOVE, -1, pt.x, pt.y);
			}

		for(;;)
			{
			// execute some 68000 code

			stat = vpci->pfnGo(cips);

			if (stat != 0)
				{
//				DebugStr("exiting due to exit code %d\n", stat);
				break;
				}

// this is in case we want to start tracing after a fixed # of instructions.
//			if (vpregs->count > 250000)
//				goto totrace;

            lTick1000 = lTick;
            QueryTickCtr();
			lTick = Getms();

            // Try to keep the loops at about 3ms

            if ((lTick - lTick1000) > 2)
                {
                // Slash the cips but prevent truncation to zero

                cips = (cips >> 1) + 1000;
                }
            else
                {
                // Can extend the loop

#if !defined(NDEBUG) || defined(BETA)
                if (cips < 2000)
#else
                if (cips < 20000)
#endif
                    cips = cips + 200;
                }
            
#if 0
            if (vmCur.fSound)
                {
                // check sound

                SoundDoneCallback();
                }
#endif

			// check for 60 Hz VBI

			if ((lTick - vsthw.lTickVBI) >= 17)
				{
// printf("PC = %08X\n", vpregs->PC);

                // HACK! HACK! HACK!
                // Force model ID to make Mac OS 8 boot!

                // mac_Reset set video RAM card base to $FF
                // ok to patch BoxFlag once video card initialized

                if (vi.cbROM[0] >= 0x80000 && \
                    (vmPeekB(vi.eaRAM[1] + vi.cbRAM[1] - 0x80000) != 0xFF))
                    {
                    // Force the Machine ID at BoxFlag ($CB3) to match the machine setting

                    if (vi.fFake040 || (vmCur.bfHW == vmMacQdra))
                        {
                        BYTE b = PeekB(0xCB3);

                        if ((b <= 0x05) || (b == 0x26))
                            PokeB(0xCB3, 14);    // Quadra 900
                        }
                    }

#if 1
                // HACK! HACK! HACK!
                // 1M ROMs get stuck in this loop
                // Figure it out later!

                    {
                    // TST.B   $03A6(A2)                  4A2A 03A6
                    // BNE.S   $0011B208                  66FA

                    if (vi.cbROM[0] >= 0x100000)
                      if (PeekL(vpregs->PC) == 0x4A2A03A6)
                        if (PeekW(vpregs->PC+4) == 0x66FA)
                            vpregs->PC += 6;

                    }
#endif

#if 1
                // HACK! HACK! HACK!
                // Aaron 1.6.1 and Kaleidespoke 2.2 have common code
                // that gets stuck in a loop at boot time
                // Figure it out later!

                    {
                    // MOVE.L  $FFFC(A6),A0               206E FFFC
                    // MOVE.L  $0004(A0),A0               2068 0004
                    // MOVEQ   #$04,D0                    7004
                    // AND.W   $009E(A0),D0               C068 009E
                    // BNE.S   $0007E0DE                  66F0


                    if (PeekL(vpregs->PC) == 0x206EFFFC)
                      if (PeekL(vpregs->PC+4) == 0x20680004)
                        if (PeekL(vpregs->PC+8) == 0x7004C068)
                          if (PeekL(vpregs->PC+12) == 0x009E66F0)
                            vpregs->PC += 0x10;

                    // MOVEQ   #$FF,D0                    70FF
                    // CMP.W    $0020(A3),D0              B06B 0020
                    // BNE.S   $0007DE00                  66F8

                    if (PeekL(vpregs->PC) == 0x70FFB06B)
                      if (PeekL(vpregs->PC+4) == 0x002066F8)
                            vpregs->PC += 8;
                    }

#endif

#if 1
                // HACK! HACK! HACK!
                // This is to get around a BTST loop in the System 6.0.5 boot code
                // Figure it out later!

                    {
                    // 128K ROMs use this one
                    // 41B006: TST.B   $00B0(A2)                  4A2A 00B0
                    // 41B00A: BNE.S   $0041B006                  66FA
                    // 41B00C: TST.B   $00AF(A2)                  4A2A 00AF
                    // 41B010: BEQ.S   $0041AFDC                  66CA

                    if (PeekL(vpregs->PC) == 0x4A2A00B0)
                        if (PeekL(vpregs->PC+4) == 0x66FA4A2A)
                            if (PeekL(vpregs->PC+8) == 0x00AF66CA)
                                vpregs->PC += 12;

                    // 128K ROMs use this one

                    // 41B0C0: TST.B   $00B0(A2)                  4A2A 00B0
                    // 41B0C4: BEQ.S   $0041AFDC                  67FA

                    if (PeekL(vpregs->PC) == 0x4A2A00B0)
                        if (PeekW(vpregs->PC+4) == 0x67FA)
                            vpregs->PC += 6;
                    }
#endif

#if 1
                // HACK! HACK! HACK!
                // This is to get around a TST.B loop in the System 7.x boot code
                // Figure it out later!

                    {
                    // System 7.0 uses this one
                    // 05985A: TST.B   $02C8(A2)                  4A2A 02C8
                    // 05985E: BNE.S   $0005985A                  66FA

                    if (PeekL(vpregs->PC) == 0x4A2A02C8)
                        if (PeekW(vpregs->PC+4) == 0x66FA)
                            vpregs->PC += 6;

                    // System 7.0 uses this one
                    // 059940: TST.B   $02C8(A2)                  4A2A 02C8
                    // 059944: BEQ.S   $00059940                  67FA

                    if (PeekL(vpregs->PC) == 0x4A2A02C8)
                        if (PeekW(vpregs->PC+4) == 0x67FA)
                            vpregs->PC += 6;

                    // System 7.5 uses this one
                    // 08xxxx: TST.B   $063E(A2)                  4A2A 063E
                    // 08xxxx: BEQ.S   $0008xxxx                  66FA

                    if (PeekL(vpregs->PC) == 0x4A2A063E)
                        if (PeekW(vpregs->PC+4) == 0x66FA)
                            vpregs->PC += 6;
                    }
#endif


                if (vmCur.fSound)
					{
                    // check sound

                    SoundDoneCallback();
                    }

				// decrement printer timer

				if (vi.cPrintTimeout && vmCur.fShare)
					{
					vi.cPrintTimeout--;
					if (vi.cPrintTimeout == 0)
						{
						FlushToPrinter();
						UnInitPrinter();
						}
					}

                if ((lTick - vsthw.lTickVBI) < 32)
                    {
    				vsthw.lTickVBI += 17;
                    }
                else
                    {
                    vsthw.lTickVBI = lTick - 15;

                    // Really slash cips

                    cips = 100;
                    }

                vmachw.rgvia[0].vIFR |= 2;

				// check for mouse movement

				if (vi.fMouseMoved && vi.fHaveFocus && (vi.fGEMMouse || !vi.fVMCapture))
					{
					POINT pt;

//					DebugStr("m");
					vi.fMouseMoved = FALSE;
					GetCursorPos(&pt);
					ScreenToClient(vi.hWnd, &pt);
					FMouseMsgST(vi.hWnd, WM_MOUSEMOVE, -1, pt.x, pt.y);
					}

				CchSerialPending();

LsetVBI:
                if (vmachw.V2Base || vmachw.RVBase)
                  {
                  if (!(vmachw.rgvia[1].vIFR & 2))
                    if (vmachw.rgvia[1].vBufA & (1 << (vmachw.sVideo - 9)))
//                    if (vmachw.rgvia[1].vIER & (1 << (vmachw.sVideo - 9)))
                    {
                    // On NuBus machines, video card generates its own VBI

                    vmachw.rgvia[1].vIFR |= 2;

                    // Clear appropriate bit to indicate a slot interrupt

                    vmachw.rgvia[1].vBufA &= ~(1 << (vmachw.sVideo - 9));
                    }
                  }

                if (GetQueueStatus(QS_ALLINPUT))
                    break;

                // HACK! break out once a second to avoid get GetQueueStatus deadlock bug

                if (vmachw.rgvia[0].vIFR & 1)
                    break;
				}

            // about once a second, force full screen refresh and
            // force printer to flush

			if ((lTick - vsthw.lTickSec) >= 1000)
                {
                FlushToPrinter();

#if 0
                printf("R:%9d W:%9d VidWX:%9d VMVidW:%9d CPU:%d\n", vi.countR, vi.countW, vi.countVidW, vi.countVMVidW, PeekB(0x12F));
                printf("cips = %5d,  time = %9d\n", cips, Getms());
#endif

#ifdef BETA
                DisplayStatus();
#endif
                // Mac one second timer

                vmachw.rgvia[0].vIFR |= 1;

                if ((lTick - vsthw.lTickSec) < 2000)
    				vsthw.lTickSec += 1000;
                else
                    vsthw.lTickSec = lTick;

                goto LsetVBI;       // this keeps the VBI more accurate
                }
			}

// Lstop:
        QueryTickCtr();
		ticks = Getms() - tickStart;
		}

	else if (fCont)
		{
		int i = 0;

		/* trace execution */

		do
			{
			stat = vpci->pfnStep(1);
			FDumpRegsVM();
			} while (!stat && (i++ < 20));
		}
	else
		{
		/* step execution */

		stat = vpci->pfnStep(1);
		FDumpRegsVM();
		}

    return stat;
}


/////////////////////////////////////////////////////////////////////////////
//
// MACINTOSH HARDWARE EMULATION
//
// There are 4 routines:
//
//   ReadHWByte(ea)    - read byte at ea
//   ReadHWWord(ea)    - read word at ea
//   WriteHWByte(ea,b) - write byte to ea 
//   WriteHWByte(ea,w) - write word to ea
//
// All routines return 0 or a small positive number on success, or
// $8000XXXX on failure, where XXXX is the trap number that should execute.
//
/////////////////////////////////////////////////////////////////////////////

MACHW vmachw;

BYTE const rgbTobyRO[4096] =
{
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x80, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x9A,
 (BYTE)~0x81, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xC2,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x18,
 (BYTE)~0x02, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x1C,
 (BYTE)~0x20, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x05,
 (BYTE)~0x22, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x2C,
 (BYTE)~0x24, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x4A,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x47, (BYTE)~0x50, (BYTE)~0x72, (BYTE)~0x6F,    // change 'Toby' to 'GPro'
 (BYTE)~0x20, (BYTE)~0x66, (BYTE)~0x72, (BYTE)~0x61,
 (BYTE)~0x6D, (BYTE)~0x65, (BYTE)~0x20, (BYTE)~0x62,
 (BYTE)~0x75, (BYTE)~0x66, (BYTE)~0x66, (BYTE)~0x65,
 (BYTE)~0x72, (BYTE)~0x20, (BYTE)~0x63, (BYTE)~0x61,
 (BYTE)~0x72, (BYTE)~0x64, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x22,
 (BYTE)~0x02, (BYTE)~0x02, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x31, (BYTE)~0x7C, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x26, (BYTE)~0x48,
 (BYTE)~0x70, (BYTE)~0x00, (BYTE)~0x10, (BYTE)~0x10,
 (BYTE)~0xE9, (BYTE)~0x48, (BYTE)~0x80, (BYTE)~0x10,
 (BYTE)~0x00, (BYTE)~0x40, (BYTE)~0x0F, (BYTE)~0x00,
 (BYTE)~0x48, (BYTE)~0x40, (BYTE)~0xE9, (BYTE)~0x88,
 (BYTE)~0x24, (BYTE)~0x40, (BYTE)~0x20, (BYTE)~0x4A,
 (BYTE)~0xD1, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0xFF, (BYTE)~0xFC, (BYTE)~0x43, (BYTE)~0xFA,
 (BYTE)~0x00, (BYTE)~0xD2, (BYTE)~0x70, (BYTE)~0x0F,
 (BYTE)~0x10, (BYTE)~0x99, (BYTE)~0x59, (BYTE)~0x48,
 (BYTE)~0x51, (BYTE)~0xC8, (BYTE)~0xFF, (BYTE)~0xFA,
 (BYTE)~0x20, (BYTE)~0x4A, (BYTE)~0xD1, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x43, (BYTE)~0xFA, (BYTE)~0x00, (BYTE)~0xCC,
 (BYTE)~0x70, (BYTE)~0x0F, (BYTE)~0x12, (BYTE)~0x19,
 (BYTE)~0x46, (BYTE)~0x01, (BYTE)~0x10, (BYTE)~0x81,
 (BYTE)~0x58, (BYTE)~0x48, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xF6, (BYTE)~0x20, (BYTE)~0x4A,
 (BYTE)~0xD1, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x0A,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x42, (BYTE)~0x10,
 (BYTE)~0x20, (BYTE)~0x4A, (BYTE)~0xD1, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x09, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x11, (BYTE)~0x7C, (BYTE)~0x00, (BYTE)~0xFF,
 (BYTE)~0x00, (BYTE)~0x1C, (BYTE)~0xD0, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x18, (BYTE)~0x72, (BYTE)~0xFF,
 (BYTE)~0x30, (BYTE)~0x3C, (BYTE)~0x01, (BYTE)~0x7F,
 (BYTE)~0x10, (BYTE)~0x81, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xFC, (BYTE)~0x72, (BYTE)~0x00,
 (BYTE)~0x30, (BYTE)~0x3C, (BYTE)~0x01, (BYTE)~0x7F,
 (BYTE)~0x10, (BYTE)~0x81, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xFC, (BYTE)~0x9E, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x38, (BYTE)~0x20, (BYTE)~0x4F,
 (BYTE)~0x11, (BYTE)~0x53, (BYTE)~0x00, (BYTE)~0x31,
 (BYTE)~0x42, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x33,
 (BYTE)~0x22, (BYTE)~0x3C, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x23, (BYTE)~0xFC, (BYTE)~0x25, (BYTE)~0xBC,
 (BYTE)~0x4D, (BYTE)~0x41, (BYTE)~0x43, (BYTE)~0x32,
 (BYTE)~0x18, (BYTE)~0x00, (BYTE)~0x2F, (BYTE)~0x3C,
 (BYTE)~0xFF, (BYTE)~0xFF, (BYTE)~0xFF, (BYTE)~0xFF,
 (BYTE)~0x58, (BYTE)~0x4F, (BYTE)~0x20, (BYTE)~0x32,
 (BYTE)~0x18, (BYTE)~0x00, (BYTE)~0x0C, (BYTE)~0x80,
 (BYTE)~0x4D, (BYTE)~0x41, (BYTE)~0x43, (BYTE)~0x32,
 (BYTE)~0x67, (BYTE)~0x08, (BYTE)~0x11, (BYTE)~0x7C,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x00, (BYTE)~0x32,
 (BYTE)~0x60, (BYTE)~0x06, (BYTE)~0x11, (BYTE)~0x7C,
 (BYTE)~0x00, (BYTE)~0x81, (BYTE)~0x00, (BYTE)~0x32,
 (BYTE)~0x70, (BYTE)~0x31, (BYTE)~0xA0, (BYTE)~0x6E,
 (BYTE)~0x66, (BYTE)~0x06, (BYTE)~0x37, (BYTE)~0x7C,
 (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x00, (BYTE)~0x02,
 (BYTE)~0xDE, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x38,
 (BYTE)~0x20, (BYTE)~0x3C, (BYTE)~0xAA, (BYTE)~0xAA,
 (BYTE)~0xAA, (BYTE)~0xAA, (BYTE)~0x22, (BYTE)~0x00,
 (BYTE)~0x46, (BYTE)~0x81, (BYTE)~0x38, (BYTE)~0x3C,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x36, (BYTE)~0x3C,
 (BYTE)~0x07, (BYTE)~0xFF, (BYTE)~0x22, (BYTE)~0x4A,
 (BYTE)~0x20, (BYTE)~0x49, (BYTE)~0x34, (BYTE)~0x3C,
 (BYTE)~0x00, (BYTE)~0x1F, (BYTE)~0x20, (BYTE)~0xC0,
 (BYTE)~0x51, (BYTE)~0xCA, (BYTE)~0xFF, (BYTE)~0xFC,
 (BYTE)~0xC1, (BYTE)~0x41, (BYTE)~0xD2, (BYTE)~0xC4,
 (BYTE)~0x51, (BYTE)~0xCB, (BYTE)~0xFF, (BYTE)~0xEE,
 (BYTE)~0x4E, (BYTE)~0x75, (BYTE)~0xDF, (BYTE)~0xB8,
 (BYTE)~0xFF, (BYTE)~0xFF, (BYTE)~0xE1, (BYTE)~0x1A,
 (BYTE)~0x88, (BYTE)~0xB9, (BYTE)~0xFA, (BYTE)~0xFD,
 (BYTE)~0xFD, (BYTE)~0xFE, (BYTE)~0xF0, (BYTE)~0xBE,
 (BYTE)~0xFA, (BYTE)~0x37, (BYTE)~0x20, (BYTE)~0x47,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x1E, (BYTE)~0xE5,
 (BYTE)~0x77, (BYTE)~0x46, (BYTE)~0x05, (BYTE)~0x02,
 (BYTE)~0x02, (BYTE)~0x01, (BYTE)~0x0F, (BYTE)~0x41,
 (BYTE)~0x05, (BYTE)~0xC8, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x10, (BYTE)~0x03, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x1C, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x24, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x41, (BYTE)~0x70,
 (BYTE)~0x70, (BYTE)~0x6C, (BYTE)~0x65, (BYTE)~0x20,
 (BYTE)~0x43, (BYTE)~0x6F, (BYTE)~0x6D, (BYTE)~0x70,
 (BYTE)~0x75, (BYTE)~0x74, (BYTE)~0x65, (BYTE)~0x72,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x42, (BYTE)~0x65,
 (BYTE)~0x74, (BYTE)~0x61, (BYTE)~0x2D, (BYTE)~0x37,
 (BYTE)~0x2E, (BYTE)~0x30, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x54, (BYTE)~0x46,
 (BYTE)~0x42, (BYTE)~0x2D, (BYTE)~0x31, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x54, (BYTE)~0x02, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x58, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x74, (BYTE)~0x08, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x0A, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x64, (BYTE)~0x0B, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x64, (BYTE)~0x80, (BYTE)~0x00,
 (BYTE)~0x08, (BYTE)~0xAA, (BYTE)~0x81, (BYTE)~0x00,
 (BYTE)~0x08, (BYTE)~0xF4, (BYTE)~0x82, (BYTE)~0x00,
 (BYTE)~0x09, (BYTE)~0x3E, (BYTE)~0x83, (BYTE)~0x00,
 (BYTE)~0x09, (BYTE)~0x78, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x28, (BYTE)~0x02, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x2C, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x48, (BYTE)~0x08, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x0A, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x38, (BYTE)~0x0B, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x38, (BYTE)~0x80, (BYTE)~0x00,
 (BYTE)~0x08, (BYTE)~0x6E, (BYTE)~0x81, (BYTE)~0x00,
 (BYTE)~0x08, (BYTE)~0xB8, (BYTE)~0x82, (BYTE)~0x00,
 (BYTE)~0x09, (BYTE)~0x02, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x03,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x44, (BYTE)~0x69,
 (BYTE)~0x73, (BYTE)~0x70, (BYTE)~0x6C, (BYTE)~0x61,
 (BYTE)~0x79, (BYTE)~0x5F, (BYTE)~0x56, (BYTE)~0x69,
 (BYTE)~0x64, (BYTE)~0x65, (BYTE)~0x6F, (BYTE)~0x5F,
 (BYTE)~0x41, (BYTE)~0x70, (BYTE)~0x70, (BYTE)~0x6C,
 (BYTE)~0x65, (BYTE)~0x5F, (BYTE)~0x54, (BYTE)~0x46,
 (BYTE)~0x42, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x08, (BYTE)~0x2E, (BYTE)~0x4C, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x2E,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x06,
 (BYTE)~0x04, (BYTE)~0xAA, (BYTE)~0x00, (BYTE)~0xD4,
 (BYTE)~0x18, (BYTE)~0x2E, (BYTE)~0x44, (BYTE)~0x69,
 (BYTE)~0x73, (BYTE)~0x70, (BYTE)~0x6C, (BYTE)~0x61,
 (BYTE)~0x79, (BYTE)~0x5F, (BYTE)~0x56, (BYTE)~0x69,
 (BYTE)~0x64, (BYTE)~0x65, (BYTE)~0x6F, (BYTE)~0x5F,
 (BYTE)~0x41, (BYTE)~0x70, (BYTE)~0x70, (BYTE)~0x6C,
 (BYTE)~0x65, (BYTE)~0x5F, (BYTE)~0x54, (BYTE)~0x46,
 (BYTE)~0x42, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x24, (BYTE)~0x48, (BYTE)~0x26, (BYTE)~0x49,
 (BYTE)~0x70, (BYTE)~0x18, (BYTE)~0xA4, (BYTE)~0x40,
 (BYTE)~0x70, (BYTE)~0x18, (BYTE)~0xA7, (BYTE)~0x22,
 (BYTE)~0x66, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x94,
 (BYTE)~0x27, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0xA0, (BYTE)~0x29, (BYTE)~0x49, (BYTE)~0xFA,
 (BYTE)~0x07, (BYTE)~0xC8, (BYTE)~0x70, (BYTE)~0x10,
 (BYTE)~0xA7, (BYTE)~0x1E, (BYTE)~0x66, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x82, (BYTE)~0x31, (BYTE)~0x7C,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x21, (BYTE)~0x4C, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x21, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0x00, (BYTE)~0x0C, (BYTE)~0x70, (BYTE)~0x00,
 (BYTE)~0x10, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x28,
 (BYTE)~0xA0, (BYTE)~0x75, (BYTE)~0x66, (BYTE)~0x66,
 (BYTE)~0x22, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x22, (BYTE)~0x51, (BYTE)~0x23, (BYTE)~0x48,
 (BYTE)~0x00, (BYTE)~0x0C, (BYTE)~0x70, (BYTE)~0x00,
 (BYTE)~0x30, (BYTE)~0x3C, (BYTE)~0x01, (BYTE)~0x0C,
 (BYTE)~0xA7, (BYTE)~0x1E, (BYTE)~0x66, (BYTE)~0x52,
 (BYTE)~0x23, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x42, (BYTE)~0x98, (BYTE)~0x70, (BYTE)~0x01,
 (BYTE)~0x20, (BYTE)~0xC0, (BYTE)~0x20, (BYTE)~0xFC,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x30, (BYTE)~0x3C, (BYTE)~0x00, (BYTE)~0xFF,
 (BYTE)~0x10, (BYTE)~0xC0, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xFC, (BYTE)~0x42, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x20, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x2A, (BYTE)~0x22, (BYTE)~0x3C,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x23, (BYTE)~0xFC,
 (BYTE)~0x21, (BYTE)~0xBC, (BYTE)~0x4D, (BYTE)~0x41,
 (BYTE)~0x43, (BYTE)~0x32, (BYTE)~0x18, (BYTE)~0x00,
 (BYTE)~0x2F, (BYTE)~0x3C, (BYTE)~0xFF, (BYTE)~0xFF,
 (BYTE)~0xFF, (BYTE)~0xFF, (BYTE)~0x42, (BYTE)~0x9F,
 (BYTE)~0x20, (BYTE)~0x30, (BYTE)~0x18, (BYTE)~0x00,
 (BYTE)~0x0C, (BYTE)~0x80, (BYTE)~0x4D, (BYTE)~0x41,
 (BYTE)~0x43, (BYTE)~0x32, (BYTE)~0x56, (BYTE)~0xE9,
 (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0xD1, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x0A, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x42, (BYTE)~0x10, (BYTE)~0x70, (BYTE)~0x00,
 (BYTE)~0x60, (BYTE)~0x02, (BYTE)~0x70, (BYTE)~0xE9,
 (BYTE)~0x4E, (BYTE)~0x75, (BYTE)~0x24, (BYTE)~0x48,
 (BYTE)~0x26, (BYTE)~0x49, (BYTE)~0x28, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x2A, (BYTE)~0xD9, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x0A, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x42, (BYTE)~0x14, (BYTE)~0x20, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x20, (BYTE)~0x50,
 (BYTE)~0x20, (BYTE)~0x68, (BYTE)~0x00, (BYTE)~0x0C,
 (BYTE)~0xA0, (BYTE)~0x76, (BYTE)~0x20, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x20, (BYTE)~0x50,
 (BYTE)~0x20, (BYTE)~0x68, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0xA0, (BYTE)~0x1F, (BYTE)~0x20, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0xA0, (BYTE)~0x23,
 (BYTE)~0x70, (BYTE)~0x00, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0x08, (BYTE)~0x88,
 (BYTE)~0x30, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x1A,
 (BYTE)~0x24, (BYTE)~0x68, (BYTE)~0x00, (BYTE)~0x1C,
 (BYTE)~0x0C, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x62, (BYTE)~0x18, (BYTE)~0xE3, (BYTE)~0x48,
 (BYTE)~0x30, (BYTE)~0x3B, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x4E, (BYTE)~0xFB, (BYTE)~0x00, (BYTE)~0x02,
 (BYTE)~0x00, (BYTE)~0x1C, (BYTE)~0x00, (BYTE)~0x12,
 (BYTE)~0x00, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0xCC,
 (BYTE)~0x02, (BYTE)~0xAE, (BYTE)~0x03, (BYTE)~0x4A,
 (BYTE)~0x03, (BYTE)~0x70, (BYTE)~0x70, (BYTE)~0xEF,
 (BYTE)~0x60, (BYTE)~0x02, (BYTE)~0x70, (BYTE)~0x00,
 (BYTE)~0x4C, (BYTE)~0xDF, (BYTE)~0x11, (BYTE)~0x10,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0xDE,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x05, (BYTE)~0x64,
 (BYTE)~0x34, (BYTE)~0xBC, (BYTE)~0x00, (BYTE)~0x80,
 (BYTE)~0x32, (BYTE)~0x3C, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x70, (BYTE)~0x00, (BYTE)~0x35, (BYTE)~0x40,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x26, (BYTE)~0x53,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x05, (BYTE)~0x7E,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x02,
 (BYTE)~0x25, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x61, (BYTE)~0x00,
 (BYTE)~0x06, (BYTE)~0x3C, (BYTE)~0x60, (BYTE)~0xCA,
 (BYTE)~0x32, (BYTE)~0x12, (BYTE)~0x61, (BYTE)~0x00,
 (BYTE)~0x04, (BYTE)~0xE6, (BYTE)~0x66, (BYTE)~0xBE,
 (BYTE)~0x30, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x05, (BYTE)~0x08,
 (BYTE)~0x66, (BYTE)~0xB4, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x26, (BYTE)~0x53,
 (BYTE)~0x34, (BYTE)~0x12, (BYTE)~0xB4, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x67, (BYTE)~0x0E,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x18,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x05, (BYTE)~0x46,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x05, (BYTE)~0xCA,
 (BYTE)~0x60, (BYTE)~0x04, (BYTE)~0x61, (BYTE)~0x00,
 (BYTE)~0x05, (BYTE)~0xC4, (BYTE)~0x25, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x60, (BYTE)~0x90, (BYTE)~0x48, (BYTE)~0xE7,
 (BYTE)~0xC0, (BYTE)~0xC0, (BYTE)~0x40, (BYTE)~0xE7,
 (BYTE)~0x46, (BYTE)~0xFC, (BYTE)~0x22, (BYTE)~0x00,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x3E,
 (BYTE)~0x20, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x20, (BYTE)~0x50, (BYTE)~0x20, (BYTE)~0x68,
 (BYTE)~0x00, (BYTE)~0x10, (BYTE)~0x30, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x41, (BYTE)~0xE8,
 (BYTE)~0x00, (BYTE)~0x0C, (BYTE)~0xD0, (BYTE)~0xC0,
 (BYTE)~0x32, (BYTE)~0x3C, (BYTE)~0x00, (BYTE)~0x80,
 (BYTE)~0x12, (BYTE)~0x30, (BYTE)~0x10, (BYTE)~0x00,
 (BYTE)~0x20, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0xD1, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x09,
 (BYTE)~0x00, (BYTE)~0x18, (BYTE)~0x30, (BYTE)~0x3C,
 (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x10, (BYTE)~0x81,
 (BYTE)~0x10, (BYTE)~0x81, (BYTE)~0x10, (BYTE)~0x81,
 (BYTE)~0x51, (BYTE)~0xC8, (BYTE)~0xFF, (BYTE)~0xF8,
 (BYTE)~0x46, (BYTE)~0xDF, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x03, (BYTE)~0x03, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0x20, (BYTE)~0x12, (BYTE)~0x67, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x3E, (BYTE)~0x48, (BYTE)~0xE7,
 (BYTE)~0x07, (BYTE)~0x06, (BYTE)~0x20, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x20, (BYTE)~0x50,
 (BYTE)~0x3A, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x20, (BYTE)~0x68, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x30, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x49, (BYTE)~0xE8, (BYTE)~0x00, (BYTE)~0x0C,
 (BYTE)~0xD8, (BYTE)~0xC0, (BYTE)~0x2A, (BYTE)~0x4C,
 (BYTE)~0x2C, (BYTE)~0x4C, (BYTE)~0x0C, (BYTE)~0x68,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x67, (BYTE)~0x14, (BYTE)~0x30, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x32, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x0A, (BYTE)~0x5E, (BYTE)~0x41,
 (BYTE)~0xE6, (BYTE)~0x49, (BYTE)~0xC0, (BYTE)~0xC1,
 (BYTE)~0xDA, (BYTE)~0xC0, (BYTE)~0xDC, (BYTE)~0xC0,
 (BYTE)~0xDC, (BYTE)~0xC0, (BYTE)~0x9E, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x61, (BYTE)~0x00,
 (BYTE)~0x03, (BYTE)~0xF4, (BYTE)~0x20, (BYTE)~0x52,
 (BYTE)~0x32, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x6B, (BYTE)~0x26, (BYTE)~0x0C, (BYTE)~0x46,
 (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x66, (BYTE)~0x0A,
 (BYTE)~0x0C, (BYTE)~0x6A, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x6E, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0xAA, (BYTE)~0x26, (BYTE)~0x4F,
 (BYTE)~0x30, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x36, (BYTE)~0xC1, (BYTE)~0x24, (BYTE)~0x18,
 (BYTE)~0x36, (BYTE)~0xC2, (BYTE)~0x26, (BYTE)~0xD8,
 (BYTE)~0x52, (BYTE)~0x41, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xF4, (BYTE)~0x20, (BYTE)~0x4F,
 (BYTE)~0x40, (BYTE)~0xE7, (BYTE)~0x46, (BYTE)~0xFC,
 (BYTE)~0x22, (BYTE)~0x00, (BYTE)~0x61, (BYTE)~0x00,
 (BYTE)~0x03, (BYTE)~0x80, (BYTE)~0x30, (BYTE)~0x2A,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x2A, (BYTE)~0xD7, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x09, (BYTE)~0x00, (BYTE)~0x18,
 (BYTE)~0x24, (BYTE)~0x18, (BYTE)~0x48, (BYTE)~0x42,
 (BYTE)~0x72, (BYTE)~0xFF, (BYTE)~0x32, (BYTE)~0x02,
 (BYTE)~0xB2, (BYTE)~0x46, (BYTE)~0x62, (BYTE)~0x5C,
 (BYTE)~0xEF, (BYTE)~0xB9, (BYTE)~0x17, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x48, (BYTE)~0x42,
 (BYTE)~0x26, (BYTE)~0x18, (BYTE)~0x28, (BYTE)~0x03,
 (BYTE)~0x48, (BYTE)~0x43, (BYTE)~0x4A, (BYTE)~0x45,
 (BYTE)~0x6A, (BYTE)~0x26, (BYTE)~0xC4, (BYTE)~0xFC,
 (BYTE)~0x4C, (BYTE)~0xCC, (BYTE)~0xC6, (BYTE)~0xFC,
 (BYTE)~0x97, (BYTE)~0x0A, (BYTE)~0xC8, (BYTE)~0xFC,
 (BYTE)~0x1C, (BYTE)~0x28, (BYTE)~0xD4, (BYTE)~0x83,
 (BYTE)~0xD4, (BYTE)~0x84, (BYTE)~0xE1, (BYTE)~0x9A,
 (BYTE)~0x02, (BYTE)~0x42, (BYTE)~0x00, (BYTE)~0xFF,
 (BYTE)~0x14, (BYTE)~0x34, (BYTE)~0x20, (BYTE)~0x00,
 (BYTE)~0x16, (BYTE)~0x82, (BYTE)~0x16, (BYTE)~0x82,
 (BYTE)~0x16, (BYTE)~0x82, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xC0, (BYTE)~0x60, (BYTE)~0x28,
 (BYTE)~0xE0, (BYTE)~0x5A, (BYTE)~0x72, (BYTE)~0x00,
 (BYTE)~0x12, (BYTE)~0x02, (BYTE)~0x12, (BYTE)~0x34,
 (BYTE)~0x10, (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0x81,
 (BYTE)~0xE0, (BYTE)~0x5B, (BYTE)~0x72, (BYTE)~0x00,
 (BYTE)~0x12, (BYTE)~0x03, (BYTE)~0x12, (BYTE)~0x35,
 (BYTE)~0x10, (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0x81,
 (BYTE)~0xE0, (BYTE)~0x5C, (BYTE)~0x72, (BYTE)~0x00,
 (BYTE)~0x12, (BYTE)~0x04, (BYTE)~0x12, (BYTE)~0x36,
 (BYTE)~0x10, (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0x81,
 (BYTE)~0x51, (BYTE)~0xC8, (BYTE)~0xFF, (BYTE)~0x96,
 (BYTE)~0x46, (BYTE)~0xDF, (BYTE)~0xDE, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x60, (BYTE)~0xE0, (BYTE)~0x60, (BYTE)~0x00,
 (BYTE)~0xFE, (BYTE)~0x3E, (BYTE)~0x22, (BYTE)~0x0F,
 (BYTE)~0x20, (BYTE)~0x01, (BYTE)~0x44, (BYTE)~0x40,
 (BYTE)~0x02, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x03,
 (BYTE)~0x67, (BYTE)~0x02, (BYTE)~0x9E, (BYTE)~0xC0,
 (BYTE)~0x2F, (BYTE)~0x01, (BYTE)~0x9E, (BYTE)~0xFC,
 (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x32, (BYTE)~0x2A,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x47, (BYTE)~0xF7,
 (BYTE)~0x14, (BYTE)~0x00, (BYTE)~0x30, (BYTE)~0x2A,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x20, (BYTE)~0x52,
 (BYTE)~0xB2, (BYTE)~0x46, (BYTE)~0x6E, (BYTE)~0x60,
 (BYTE)~0x4A, (BYTE)~0x45, (BYTE)~0x6A, (BYTE)~0x34,
 (BYTE)~0x24, (BYTE)~0x18, (BYTE)~0x36, (BYTE)~0x18,
 (BYTE)~0x38, (BYTE)~0x18, (BYTE)~0xC4, (BYTE)~0xFC,
 (BYTE)~0x4C, (BYTE)~0xCC, (BYTE)~0xC6, (BYTE)~0xFC,
 (BYTE)~0x97, (BYTE)~0x0A, (BYTE)~0xC8, (BYTE)~0xFC,
 (BYTE)~0x1C, (BYTE)~0x28, (BYTE)~0xD4, (BYTE)~0x83,
 (BYTE)~0xD4, (BYTE)~0x84, (BYTE)~0xE1, (BYTE)~0x9A,
 (BYTE)~0x76, (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0x02,
 (BYTE)~0x14, (BYTE)~0x34, (BYTE)~0x30, (BYTE)~0x00,
 (BYTE)~0x16, (BYTE)~0x02, (BYTE)~0xE1, (BYTE)~0x8B,
 (BYTE)~0x16, (BYTE)~0x02, (BYTE)~0xE1, (BYTE)~0x8B,
 (BYTE)~0x16, (BYTE)~0x02, (BYTE)~0x26, (BYTE)~0xC3,
 (BYTE)~0x52, (BYTE)~0x41, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xC8, (BYTE)~0x60, (BYTE)~0x28,
 (BYTE)~0x74, (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x16, (BYTE)~0x34,
 (BYTE)~0x20, (BYTE)~0x00, (BYTE)~0xE1, (BYTE)~0x8B,
 (BYTE)~0x14, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x16, (BYTE)~0x35, (BYTE)~0x20, (BYTE)~0x00,
 (BYTE)~0xE1, (BYTE)~0x8B, (BYTE)~0x14, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x16, (BYTE)~0x36,
 (BYTE)~0x20, (BYTE)~0x00, (BYTE)~0x26, (BYTE)~0xC3,
 (BYTE)~0x52, (BYTE)~0x41, (BYTE)~0x50, (BYTE)~0x48,
 (BYTE)~0x51, (BYTE)~0xC8, (BYTE)~0xFF, (BYTE)~0x9E,
 (BYTE)~0x40, (BYTE)~0xE7, (BYTE)~0x46, (BYTE)~0xFC,
 (BYTE)~0x22, (BYTE)~0x00, (BYTE)~0x20, (BYTE)~0x4B,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x66,
 (BYTE)~0x26, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0xD7, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x09,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x72, (BYTE)~0x00,
 (BYTE)~0x12, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x05,
 (BYTE)~0x46, (BYTE)~0x01, (BYTE)~0x30, (BYTE)~0x2A,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x92, (BYTE)~0x40,
 (BYTE)~0x46, (BYTE)~0x01, (BYTE)~0x17, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x1C, (BYTE)~0xD6, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x18, (BYTE)~0x22, (BYTE)~0x20,
 (BYTE)~0x16, (BYTE)~0x81, (BYTE)~0xE0, (BYTE)~0x89,
 (BYTE)~0x16, (BYTE)~0x81, (BYTE)~0xE0, (BYTE)~0x89,
 (BYTE)~0x16, (BYTE)~0x81, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xF2, (BYTE)~0x46, (BYTE)~0xDF,
 (BYTE)~0xDE, (BYTE)~0xFC, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x2E, (BYTE)~0x5F, (BYTE)~0xDE, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x60, (BYTE)~0xE0, (BYTE)~0x60, (BYTE)~0x00,
 (BYTE)~0xFD, (BYTE)~0x66, (BYTE)~0x2F, (BYTE)~0x09,
 (BYTE)~0x26, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x26, (BYTE)~0x53, (BYTE)~0x20, (BYTE)~0x12,
 (BYTE)~0x67, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x8A,
 (BYTE)~0x22, (BYTE)~0x40, (BYTE)~0x4A, (BYTE)~0x91,
 (BYTE)~0x66, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x82,
 (BYTE)~0x0C, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x00, (BYTE)~0x0A, (BYTE)~0x66, (BYTE)~0x78,
 (BYTE)~0x0C, (BYTE)~0x69, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x66, (BYTE)~0x70,
 (BYTE)~0x20, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x30, (BYTE)~0x29, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0xB0, (BYTE)~0x68, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x66, (BYTE)~0x12, (BYTE)~0x30, (BYTE)~0x29,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0xB0, (BYTE)~0x68,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x67, (BYTE)~0x30,
 (BYTE)~0x6E, (BYTE)~0x06, (BYTE)~0xA0, (BYTE)~0x1F,
 (BYTE)~0x42, (BYTE)~0xAB, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x30, (BYTE)~0x3C, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0xC0, (BYTE)~0xE9, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0xD0, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x06, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x0C,
 (BYTE)~0xA5, (BYTE)~0x1E, (BYTE)~0x66, (BYTE)~0x3C,
 (BYTE)~0x20, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x27, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x4A, (BYTE)~0x80, (BYTE)~0x67, (BYTE)~0x08,
 (BYTE)~0x20, (BYTE)~0x40, (BYTE)~0xA0, (BYTE)~0x1F,
 (BYTE)~0x20, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x30, (BYTE)~0x29, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x32, (BYTE)~0x29, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x20, (BYTE)~0xD9, (BYTE)~0x20, (BYTE)~0xD9,
 (BYTE)~0x20, (BYTE)~0xD9, (BYTE)~0x34, (BYTE)~0x3C,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0xC4, (BYTE)~0xC0,
 (BYTE)~0xD4, (BYTE)~0x41, (BYTE)~0x53, (BYTE)~0x42,
 (BYTE)~0x10, (BYTE)~0x19, (BYTE)~0x46, (BYTE)~0x00,
 (BYTE)~0x10, (BYTE)~0xC0, (BYTE)~0x51, (BYTE)~0xCA,
 (BYTE)~0xFF, (BYTE)~0xF8, (BYTE)~0x22, (BYTE)~0x5F,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0xFC, (BYTE)~0xD0,
 (BYTE)~0x22, (BYTE)~0x5F, (BYTE)~0x60, (BYTE)~0x00,
 (BYTE)~0xFC, (BYTE)~0xC6, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x26, (BYTE)~0x53,
 (BYTE)~0x32, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xDC,
 (BYTE)~0x66, (BYTE)~0x00, (BYTE)~0xFC, (BYTE)~0xB4,
 (BYTE)~0x30, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xFC,
 (BYTE)~0x66, (BYTE)~0x00, (BYTE)~0xFC, (BYTE)~0xA8,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x03, (BYTE)~0x16,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0xFC, (BYTE)~0xA4,
 (BYTE)~0x26, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x26, (BYTE)~0x53, (BYTE)~0x30, (BYTE)~0x2B,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x12, (BYTE)~0x12,
 (BYTE)~0xEF, (BYTE)~0xC0, (BYTE)~0x14, (BYTE)~0x01,
 (BYTE)~0x37, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0xFC, (BYTE)~0x8C,
 (BYTE)~0x30, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x1A,
 (BYTE)~0x24, (BYTE)~0x68, (BYTE)~0x00, (BYTE)~0x1C,
 (BYTE)~0x0C, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x62, (BYTE)~0x18, (BYTE)~0xE3, (BYTE)~0x48,
 (BYTE)~0x30, (BYTE)~0x3B, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x4E, (BYTE)~0xFB, (BYTE)~0x00, (BYTE)~0x02,
 (BYTE)~0x00, (BYTE)~0x0E, (BYTE)~0x00, (BYTE)~0x0E,
 (BYTE)~0x00, (BYTE)~0x1A, (BYTE)~0x00, (BYTE)~0x32,
 (BYTE)~0x00, (BYTE)~0xB0, (BYTE)~0x00, (BYTE)~0xD6,
 (BYTE)~0x01, (BYTE)~0x14, (BYTE)~0x70, (BYTE)~0xEE,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x46,
 (BYTE)~0x70, (BYTE)~0x00, (BYTE)~0x60, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0x40, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x26, (BYTE)~0x53,
 (BYTE)~0x34, (BYTE)~0xAB, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x35, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x25, (BYTE)~0x6B,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x60, (BYTE)~0xE2, (BYTE)~0x20, (BYTE)~0x12,
 (BYTE)~0x67, (BYTE)~0xD8, (BYTE)~0x20, (BYTE)~0x40,
 (BYTE)~0x0C, (BYTE)~0x6A, (BYTE)~0xFF, (BYTE)~0xFF,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x66, (BYTE)~0x14,
 (BYTE)~0x32, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x34, (BYTE)~0x01, (BYTE)~0xD4, (BYTE)~0x6A,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x31, (BYTE)~0x82,
 (BYTE)~0x16, (BYTE)~0x00, (BYTE)~0x53, (BYTE)~0x42,
 (BYTE)~0x51, (BYTE)~0xC9, (BYTE)~0xFF, (BYTE)~0xF8,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0x03, (BYTE)~0x00,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x0E,
 (BYTE)~0x26, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0xD7, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x09,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x32, (BYTE)~0x10,
 (BYTE)~0xB2, (BYTE)~0x46, (BYTE)~0x62, (BYTE)~0x36,
 (BYTE)~0x00, (BYTE)~0x81, (BYTE)~0xFF, (BYTE)~0xFF,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0xEF, (BYTE)~0xB9,
 (BYTE)~0x17, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x1C,
 (BYTE)~0x12, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x28,
 (BYTE)~0x46, (BYTE)~0x41, (BYTE)~0x11, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x11, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x03, (BYTE)~0x12, (BYTE)~0x2B,
 (BYTE)~0x00, (BYTE)~0x28, (BYTE)~0x46, (BYTE)~0x41,
 (BYTE)~0x11, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x11, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x05,
 (BYTE)~0x12, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x28,
 (BYTE)~0x46, (BYTE)~0x41, (BYTE)~0x11, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x11, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x07, (BYTE)~0x50, (BYTE)~0x88,
 (BYTE)~0x51, (BYTE)~0xC8, (BYTE)~0xFF, (BYTE)~0xC0,
 (BYTE)~0x4C, (BYTE)~0xDF, (BYTE)~0x00, (BYTE)~0xC0,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x66,
 (BYTE)~0x32, (BYTE)~0x12, (BYTE)~0x61, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0xDE, (BYTE)~0x66, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x56, (BYTE)~0x20, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x4A, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0x66, (BYTE)~0x04,
 (BYTE)~0x70, (BYTE)~0x04, (BYTE)~0x60, (BYTE)~0x02,
 (BYTE)~0x70, (BYTE)~0x03, (BYTE)~0x80, (BYTE)~0xC1,
 (BYTE)~0x52, (BYTE)~0x40, (BYTE)~0x35, (BYTE)~0x40,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x60, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x40, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x26, (BYTE)~0x53,
 (BYTE)~0x32, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0xB0,
 (BYTE)~0x30, (BYTE)~0x2A, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0xD4,
 (BYTE)~0x66, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x20,
 (BYTE)~0x32, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x04, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x80,
 (BYTE)~0x41, (BYTE)~0xFA, (BYTE)~0x02, (BYTE)~0x32,
 (BYTE)~0xC0, (BYTE)~0xF0, (BYTE)~0x16, (BYTE)~0x04,
 (BYTE)~0xC0, (BYTE)~0xF0, (BYTE)~0x16, (BYTE)~0x06,
 (BYTE)~0x72, (BYTE)~0x04, (BYTE)~0xD0, (BYTE)~0x81,
 (BYTE)~0xD0, (BYTE)~0xA9, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0x25, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x02,
 (BYTE)~0x42, (BYTE)~0x52, (BYTE)~0x26, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x26, (BYTE)~0x53,
 (BYTE)~0x4A, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x6A, (BYTE)~0x00, (BYTE)~0xFE, (BYTE)~0xF2,
 (BYTE)~0x14, (BYTE)~0xBC, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x60, (BYTE)~0x00, (BYTE)~0xFE, (BYTE)~0xEA,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0x80, (BYTE)~0x80,
 (BYTE)~0x40, (BYTE)~0xE7, (BYTE)~0x46, (BYTE)~0xFC,
 (BYTE)~0x22, (BYTE)~0x00, (BYTE)~0x20, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x2A, (BYTE)~0xD1, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x0D, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x20, (BYTE)~0x10, (BYTE)~0x08, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x66, (BYTE)~0xF8,
 (BYTE)~0x20, (BYTE)~0x10, (BYTE)~0x08, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x67, (BYTE)~0xF8,
 (BYTE)~0x46, (BYTE)~0xDF, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x01, (BYTE)~0x01, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0x08, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x09,
 (BYTE)~0x00, (BYTE)~0x06, (BYTE)~0x67, (BYTE)~0x02,
 (BYTE)~0x4E, (BYTE)~0x75, (BYTE)~0x20, (BYTE)~0x78,
 (BYTE)~0x08, (BYTE)~0xFC, (BYTE)~0x4E, (BYTE)~0xD0,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0x88, (BYTE)~0x10,
 (BYTE)~0x26, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x26, (BYTE)~0x53, (BYTE)~0x3E, (BYTE)~0x2B,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x04, (BYTE)~0x47,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x78, (BYTE)~0x01,
 (BYTE)~0xEF, (BYTE)~0x6C, (BYTE)~0x30, (BYTE)~0x04,
 (BYTE)~0x53, (BYTE)~0x40, (BYTE)~0x7C, (BYTE)~0x02,
 (BYTE)~0xE1, (BYTE)~0x6E, (BYTE)~0x53, (BYTE)~0x46,
 (BYTE)~0x7E, (BYTE)~0x08, (BYTE)~0x9E, (BYTE)~0x44,
 (BYTE)~0x4C, (BYTE)~0xDF, (BYTE)~0x08, (BYTE)~0x11,
 (BYTE)~0x4E, (BYTE)~0x75, (BYTE)~0x04, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x6B, (BYTE)~0x24,
 (BYTE)~0x0C, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x03,
 (BYTE)~0x6E, (BYTE)~0x1E, (BYTE)~0x70, (BYTE)~0x01,
 (BYTE)~0xE3, (BYTE)~0x68, (BYTE)~0x32, (BYTE)~0x00,
 (BYTE)~0x0C, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x66, (BYTE)~0x10, (BYTE)~0x2F, (BYTE)~0x08,
 (BYTE)~0x20, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x14,
 (BYTE)~0x20, (BYTE)~0x50, (BYTE)~0x4A, (BYTE)~0x28,
 (BYTE)~0x00, (BYTE)~0x16, (BYTE)~0x20, (BYTE)~0x5F,
 (BYTE)~0x66, (BYTE)~0x02, (BYTE)~0xB2, (BYTE)~0x41,
 (BYTE)~0x4E, (BYTE)~0x75, (BYTE)~0x48, (BYTE)~0xE7,
 (BYTE)~0x20, (BYTE)~0x80, (BYTE)~0x20, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x14, (BYTE)~0x20, (BYTE)~0x50,
 (BYTE)~0x4A, (BYTE)~0x28, (BYTE)~0x00, (BYTE)~0x16,
 (BYTE)~0x66, (BYTE)~0x04, (BYTE)~0x74, (BYTE)~0x04,
 (BYTE)~0x60, (BYTE)~0x02, (BYTE)~0x74, (BYTE)~0x03,
 (BYTE)~0x84, (BYTE)~0xC1, (BYTE)~0xB0, (BYTE)~0x42,
 (BYTE)~0x5E, (BYTE)~0xC2, (BYTE)~0x4A, (BYTE)~0x02,
 (BYTE)~0x4C, (BYTE)~0xDF, (BYTE)~0x01, (BYTE)~0x04,
 (BYTE)~0x4E, (BYTE)~0x75, (BYTE)~0x48, (BYTE)~0xE7,
 (BYTE)~0x80, (BYTE)~0xC0, (BYTE)~0x20, (BYTE)~0x69,
 (BYTE)~0x00, (BYTE)~0x2A, (BYTE)~0xD1, (BYTE)~0xFC,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0xFF, (BYTE)~0xFC,
 (BYTE)~0x43, (BYTE)~0xFA, (BYTE)~0x00, (BYTE)~0x12,
 (BYTE)~0x70, (BYTE)~0x0F, (BYTE)~0x10, (BYTE)~0x99,
 (BYTE)~0x59, (BYTE)~0x48, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xFA, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x03, (BYTE)~0x01, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0xDF, (BYTE)~0xB8, (BYTE)~0xFF, (BYTE)~0xFF,
 (BYTE)~0xE1, (BYTE)~0x1A, (BYTE)~0x88, (BYTE)~0xB9,
 (BYTE)~0xFA, (BYTE)~0xFD, (BYTE)~0xFD, (BYTE)~0xFE,
 (BYTE)~0xF0, (BYTE)~0xBE, (BYTE)~0xFA, (BYTE)~0x37,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0xE0, (BYTE)~0xC0,
 (BYTE)~0x37, (BYTE)~0x52, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x61, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x0E,
 (BYTE)~0x20, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0xD1, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x0C, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x6E, (BYTE)~0x26,
 (BYTE)~0x70, (BYTE)~0x00, (BYTE)~0xE2, (BYTE)~0x49,
 (BYTE)~0x65, (BYTE)~0x04, (BYTE)~0x52, (BYTE)~0x40,
 (BYTE)~0x60, (BYTE)~0xF8, (BYTE)~0x31, (BYTE)~0x7C,
 (BYTE)~0x00, (BYTE)~0xB7, (BYTE)~0x00, (BYTE)~0x3C,
 (BYTE)~0x43, (BYTE)~0xFA, (BYTE)~0x00, (BYTE)~0x1A,
 (BYTE)~0xE9, (BYTE)~0x48, (BYTE)~0xD2, (BYTE)~0xC0,
 (BYTE)~0x70, (BYTE)~0x0F, (BYTE)~0x12, (BYTE)~0x19,
 (BYTE)~0x46, (BYTE)~0x01, (BYTE)~0x10, (BYTE)~0x81,
 (BYTE)~0x58, (BYTE)~0x48, (BYTE)~0x51, (BYTE)~0xC8,
 (BYTE)~0xFF, (BYTE)~0xF6, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x03, (BYTE)~0x07, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0x20, (BYTE)~0x47, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x1E, (BYTE)~0xE5, (BYTE)~0x77, (BYTE)~0x46,
 (BYTE)~0x05, (BYTE)~0x02, (BYTE)~0x02, (BYTE)~0x01,
 (BYTE)~0x0F, (BYTE)~0x41, (BYTE)~0x05, (BYTE)~0xC8,
 (BYTE)~0x40, (BYTE)~0x47, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x3C, (BYTE)~0xE5, (BYTE)~0x77, (BYTE)~0x46,
 (BYTE)~0x05, (BYTE)~0x06, (BYTE)~0x06, (BYTE)~0x04,
 (BYTE)~0x20, (BYTE)~0x04, (BYTE)~0x0B, (BYTE)~0xD8,
 (BYTE)~0x80, (BYTE)~0x47, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x78, (BYTE)~0xE5, (BYTE)~0x77, (BYTE)~0x46,
 (BYTE)~0x05, (BYTE)~0x0E, (BYTE)~0x0E, (BYTE)~0x0A,
 (BYTE)~0x42, (BYTE)~0x8A, (BYTE)~0x16, (BYTE)~0xE8,
 (BYTE)~0x00, (BYTE)~0x47, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0xF0, (BYTE)~0xE5, (BYTE)~0x77, (BYTE)~0x46,
 (BYTE)~0x05, (BYTE)~0x1E, (BYTE)~0x1E, (BYTE)~0x16,
 (BYTE)~0x86, (BYTE)~0x96, (BYTE)~0x2D, (BYTE)~0xF9,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0xC0, (BYTE)~0xC0,
 (BYTE)~0x37, (BYTE)~0x40, (BYTE)~0x00, (BYTE)~0x06,
 (BYTE)~0x32, (BYTE)~0x12, (BYTE)~0x04, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x41, (BYTE)~0xFA,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0xC0, (BYTE)~0xF0,
 (BYTE)~0x16, (BYTE)~0x04, (BYTE)~0xC0, (BYTE)~0xF0,
 (BYTE)~0x16, (BYTE)~0x06, (BYTE)~0x72, (BYTE)~0x20,
 (BYTE)~0xD0, (BYTE)~0x81, (BYTE)~0x22, (BYTE)~0x00,
 (BYTE)~0x20, (BYTE)~0x69, (BYTE)~0x00, (BYTE)~0x2A,
 (BYTE)~0xD0, (BYTE)~0x88, (BYTE)~0x27, (BYTE)~0x40,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0xE4, (BYTE)~0x89,
 (BYTE)~0xD1, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x46, (BYTE)~0x41,
 (BYTE)~0x11, (BYTE)~0x41, (BYTE)~0x00, (BYTE)~0x0C,
 (BYTE)~0xE0, (BYTE)~0x49, (BYTE)~0x11, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x08, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x03, (BYTE)~0x03, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0x48, (BYTE)~0xE7, (BYTE)~0xF8, (BYTE)~0xC0,
 (BYTE)~0x32, (BYTE)~0x12, (BYTE)~0x04, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x0C, (BYTE)~0x41,
 (BYTE)~0x00, (BYTE)~0x03, (BYTE)~0x66, (BYTE)~0x06,
 (BYTE)~0x4A, (BYTE)~0x2B, (BYTE)~0x00, (BYTE)~0x16,
 (BYTE)~0x66, (BYTE)~0x30, (BYTE)~0x43, (BYTE)~0xFA,
 (BYTE)~0x00, (BYTE)~0x34, (BYTE)~0x20, (BYTE)~0x31,
 (BYTE)~0x16, (BYTE)~0x00, (BYTE)~0x38, (BYTE)~0x31,
 (BYTE)~0x16, (BYTE)~0x04, (BYTE)~0x36, (BYTE)~0x31,
 (BYTE)~0x16, (BYTE)~0x06, (BYTE)~0x53, (BYTE)~0x43,
 (BYTE)~0x22, (BYTE)~0x00, (BYTE)~0x46, (BYTE)~0x81,
 (BYTE)~0x22, (BYTE)~0x6B, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x20, (BYTE)~0x49, (BYTE)~0x34, (BYTE)~0x04,
 (BYTE)~0xE4, (BYTE)~0x4A, (BYTE)~0x53, (BYTE)~0x42,
 (BYTE)~0x20, (BYTE)~0xC0, (BYTE)~0x51, (BYTE)~0xCA,
 (BYTE)~0xFF, (BYTE)~0xFC, (BYTE)~0xC1, (BYTE)~0x41,
 (BYTE)~0xD2, (BYTE)~0xC4, (BYTE)~0x51, (BYTE)~0xCB,
 (BYTE)~0xFF, (BYTE)~0xEC, (BYTE)~0x4C, (BYTE)~0xDF,
 (BYTE)~0x03, (BYTE)~0x1F, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0xAA, (BYTE)~0xAA, (BYTE)~0xAA, (BYTE)~0xAA,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x01, (BYTE)~0xE0,
 (BYTE)~0xCC, (BYTE)~0xCC, (BYTE)~0xCC, (BYTE)~0xCC,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xE0,
 (BYTE)~0xF0, (BYTE)~0xF0, (BYTE)~0xF0, (BYTE)~0xF0,
 (BYTE)~0x02, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xE0,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xE0,
 (BYTE)~0x20, (BYTE)~0x49, (BYTE)~0x20, (BYTE)~0x09,
 (BYTE)~0xD1, (BYTE)~0xFC, (BYTE)~0x00, (BYTE)~0x0A,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x42, (BYTE)~0x10,
 (BYTE)~0xE1, (BYTE)~0x98, (BYTE)~0x02, (BYTE)~0x40,
 (BYTE)~0x00, (BYTE)~0x0F, (BYTE)~0x20, (BYTE)~0x78,
 (BYTE)~0x0D, (BYTE)~0x28, (BYTE)~0x4E, (BYTE)~0x90,
 (BYTE)~0x70, (BYTE)~0x01, (BYTE)~0x4E, (BYTE)~0x75,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x20,
 (BYTE)~0x03, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x04,
 (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x03, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x05,
 (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x2E,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x20,
 (BYTE)~0x00, (BYTE)~0x80, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xE0,
 (BYTE)~0x02, (BYTE)~0x80, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x48,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x48,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x20, (BYTE)~0x03, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x02, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x10, (BYTE)~0x03, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x03, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x2E, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x20, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0xE0, (BYTE)~0x02, (BYTE)~0x80,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x02,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x02,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x20,
 (BYTE)~0x03, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x10,
 (BYTE)~0x03, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x02,
 (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0xFF, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x2E,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x20,
 (BYTE)~0x02, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0xE0,
 (BYTE)~0x02, (BYTE)~0x80, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x48,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x48,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x01,
 (BYTE)~0x00, (BYTE)~0x04, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x10, (BYTE)~0x03, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x2E, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x20, (BYTE)~0x04, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x01, (BYTE)~0xE0, (BYTE)~0x02, (BYTE)~0x80,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x48, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x00, (BYTE)~0x01, (BYTE)~0x00, (BYTE)~0x08,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x00,
 (BYTE)~0x00, (BYTE)~0xFF, (BYTE)~0xF0, (BYTE)~0x14,
 (BYTE)~0x00, (BYTE)~0x00, (BYTE)~0x10, (BYTE)~0x00,
 (BYTE)~0x72, (BYTE)~0x97, (BYTE)~0x55, (BYTE)~0xF8,
 (BYTE)~0x01, (BYTE)~0x01, (BYTE)~0x5A, (BYTE)~0x93,
 (BYTE)~0x2B, (BYTE)~0xC7, (BYTE)~0x00, (BYTE)~0xE1,
};

#if HIRES
#include "hires.txt"
#endif

void UpdateT2()
{
    ULONG l = vmachw.lLastEClock2;

#if 0
    // REVIEW: only Mac II (not Mac LC/Quadra) has timer 2?
    // But enabling this code breaks Apple Personal Diagnostics
    // which implies that timer 2 is used in IIci and Quadra!

    if (!FIsMac24(vmCur.bfHW))
        return;
#endif

    if (!vmachw.fTimer2Going || !vmachw.rgvia[0].vT2Cw)
        return;

    QueryTickCtr();
    vmachw.lLastEClock2 = GetEclock();

    if ((vmachw.lLastEClock2 - l) < (ULONG)vmachw.rgvia[0].vT2Cw)
        {
        vmachw.rgvia[0].vT2Cw -= (vmachw.lLastEClock2 - l);
        }
    else
        {
//    printf("T2 interrupt!!\n");
        vmachw.rgvia[0].vT2Cw = 0;
        vmachw.rgvia[0].vIFR |= 0x20;
        vmachw.fTimer2Going = FALSE;
        }
}


//
// Read or write a byte to the SCC.
// fAB selects between SCC A or SCC B half
// fDC selects data or control
// b is the byte (when writing), or returns a byte on read
//

BYTE sccfRWfABfDCb(BOOL fWrite, BOOL fAB, BOOL fDC, BYTE b)
{
    BYTE bRet = 0;
    BOOL fA = !!fAB;
    BOOL fB = !fAB;
    int  iSCC = fA;

#if TRACEHW || TRACESCC
    printf("\nSCC: %08X %c %c %c %02X\n",
         vpregs->PC,
         fWrite ? 'W' : 'R',
         fAB    ? 'A' : 'B',
         fDC    ? 'D' : 'C',
         b);
#endif

    if (fDC)
        {
        // accessing RR8/WR8 data registers directly

        if (fWrite)
            vmachw.aSCC[iSCC].WR[8] = b;
        else
            bRet = vmachw.aSCC[iSCC].RR[8];

#if TRACEHW || TRACESCC
        if (fWrite)
            printf("WRITING DATA %02X\n", b);
        else
            printf("READING DATA %02X\n", bRet);
#endif
        }

    else if (fWrite)
        {
        BYTE bOld = vmachw.aSCC[iSCC].WR[vmachw.iRegSCC];

        // update the write register

        vmachw.aSCC[iSCC].WR[vmachw.iRegSCC] = b;

        // writing to a register determined by register pointer iRegSCC

#if TRACEHW || TRACESCC
    printf("Writing register %d, ", vmachw.iRegSCC);
#endif

    switch (vmachw.iRegSCC)
        {
    default:
        break;

    case 0:
        // WR0
        //
        // D7 D6    = CRC reset codes (we ignore these for now)

        b &= 0x3F;

        // D5 D4 D3 = Command code
        // D2 D1 D0 = low 3 bits of register pointer

        if (b < 16)
            {
            // special case to not reset the register pointer!

            vmachw.iRegSCC = b & 15;
            return 0;
            }

        else if (b == 16)
            {
            vmachw.aSCC[SCCA].RR[0] &= ~8;  // clear interrupt bit
            vmachw.aSCC[SCCB].RR[0] &= ~8;  // clear interrupt bit
            }
        break;

    case 1:
        // Transmit and Receive interrupt enables
        break;

    case 2:
        // Interrupt Vector
        break;

    case 3:
        // Receive paramters and control modes
        break;

    case 4:
        // Transmit and Receive modes and parameters

        vmachw.aSCC[iSCC].RR[vmachw.iRegSCC] = b; // refrect to RR
        break;

    case 5:
        // Transmit parameters and control modes

        vmachw.aSCC[iSCC].RR[vmachw.iRegSCC] = b; // refrect to RR
        break;

    case 9:
        // Master Interrupt Control, global to both A and B

        vmachw.aSCC[SCCA].WR[vmachw.iRegSCC] = b;
        vmachw.aSCC[SCCB].WR[vmachw.iRegSCC] = b;

        vmachw.aSCC[SCCA].RR[vmachw.iRegSCC] = b; // refrect to RR
        vmachw.aSCC[SCCB].RR[vmachw.iRegSCC] = b; // refrect to RR
        break;

    case 12:
        // Baud rate generator time constant, log byte

        vmachw.aSCC[iSCC].RR[vmachw.iRegSCC] = b; // refrect to RR
        break;

    case 13:
        // Baud rate generator time constant, high byte

        vmachw.aSCC[iSCC].RR[vmachw.iRegSCC] = b; // refrect to RR
        break;

    case 15:
        // Value in RR15, with bits 0 and 2 cleared
        vmachw.aSCC[iSCC].RR[vmachw.iRegSCC] = b & 0xFA; // refrect to RR
        break;

        }
        }
    else
        {
        // reading from a register determined by register pointer iRegSCC

#if TRACEHW || TRACESCC
    printf("Reading register %d, ", vmachw.iRegSCC);
#endif

    switch (vmachw.iRegSCC)
        {
    default:
        bRet = vmachw.aSCC[iSCC].RR[vmachw.iRegSCC];
        break;

    case 0:
        bRet = vmachw.aSCC[iSCC].RR[vmachw.iRegSCC];
        break;

    case 1:
    case 5:
        // RR5 returns RR1

        bRet = vmachw.aSCC[iSCC].RR[1];
        break;

    case 2:
    case 6:
        // RR6 returns RR2

        if (iSCC == SCCB)
            {
            vmachw.aSCC[iSCC].RR[2] |= 2;
            }

        bRet = vmachw.aSCC[iSCC].RR[2];
        break;

    case 3:
    case 7:
        // RR7 returns RR3

        bRet = vmachw.aSCC[iSCC].RR[3];
        break;
        }

#if TRACEHW || TRACESCC
        printf("ret %02X\n", bRet);
#endif
        }

    vmachw.iRegSCC = 0;                 // reset register pointer
    vmachw.aSCC[iSCC].RR[0] |= 1;
    vmachw.aSCC[iSCC].RR[1] |= 1;
    return bRet;
}


//
// Read or write a byte to the VIA.
// iVia selects between VIA2 and VIA1
// b is the byte (when writing), or returns a byte on read
//

BYTE BReadWriteVIA(BOOL fWrite, ULONG iVia, ULONG iReg, BYTE b)
{
    if (fWrite == -1)
        {
        // this is a reset of the VIA

        vmachw.rgvia[iVia].vBufA = 0;
        vmachw.rgvia[iVia].vBufB = 0;
        vmachw.rgvia[iVia].vDirA = 0;
        vmachw.rgvia[iVia].vDirB = 0;
        vmachw.rgvia[iVia].vBufAL = 0;
        vmachw.rgvia[iVia].vBufBL = 0;

        vmachw.rgvia[iVia].vACR  = 0;
        vmachw.rgvia[iVia].vPCR  = 0;
        vmachw.rgvia[iVia].vIFR  = 0;
        vmachw.rgvia[iVia].vIER  = 0;

        return 0;
        }

    if (fWrite == 0)
        {
        b = 0;

        // reading the VIA

        switch(iReg)
            {
        default:
#if !defined(NDEBUG) || defined(BETA)
            printf("ILLEGAL iReg value in VIA read\n");
#endif
            break;

        case 0:
            // reading B clears the CB1 and CB2 interrupts

            vmachw.rgvia[iVia].vIFR &= 0xE7;

            b =  vmachw.rgvia[iVia].vBufB  & ~vmachw.rgvia[iVia].vDirB;
            b |= vmachw.rgvia[iVia].vBufBL &  vmachw.rgvia[iVia].vDirB;
            break;

        case 1:
            // reading A' clears the CA1 and CA2 interrupts

            vmachw.rgvia[iVia].vIFR &= 0xFC;

            // fall through

        case 15:
            b =  vmachw.rgvia[iVia].vBufA  & ~vmachw.rgvia[iVia].vDirA;
            b |= vmachw.rgvia[iVia].vBufAL &  vmachw.rgvia[iVia].vDirA;
            break;

        case 2:
            b = vmachw.rgvia[iVia].vDirB;
            break;

        case 3:
            b = vmachw.rgvia[iVia].vDirA;
            break;

        case 4:
            // reading T1L clears T1 interrupt

            vmachw.rgvia[iVia].vIFR &= 0xBF;

            b = vmachw.rgvia[iVia].vT1CL;
            break;

        case 5:
            b = vmachw.rgvia[iVia].vT1CH;
            break;

        case 6:
            b = vmachw.rgvia[iVia].vT1LL;
            break;

        case 7:
            b = vmachw.rgvia[iVia].vT1LH;
            break;

        case 8:
            // reading T2L clears T2 interrupt

            vmachw.rgvia[iVia].vIFR &= 0xDF;

            b = vmachw.rgvia[iVia].vT2CL;
            break;

        case 9:
            b = vmachw.rgvia[iVia].vT2CH;
            break;

        case 10:
            // reading SR clears SR interrupt

            vmachw.rgvia[iVia].vIFR &= 0xF7;

            b = vmachw.rgvia[iVia].vSR;
            break;

        case 11:
            b = vmachw.rgvia[iVia].vACR;
            break;

        case 12:
            b = vmachw.rgvia[iVia].vPCR;
            break;

        case 13:
            b = vmachw.rgvia[iVia].vIFR;

            if (vmachw.rgvia[iVia].vIFR & vmachw.rgvia[iVia].vIER & 0x7F)
                b |= 0x80;
            else
                b &= 0x7F;
            break;

        case 14:
            b = vmachw.rgvia[iVia].vIER | 0x80;
            break;
            }

        return b;
        }

    if (fWrite == 1)
        {
        // writing the VIA

        switch(iReg)
            {
        default:
#if !defined(NDEBUG) || defined(BETA)
            printf("ILLEGAL iReg value in VIA write\n");
#endif
            break;

        case 0:
            // writing B clears the CB1 and CB2 interrupts

            vmachw.rgvia[iVia].vIFR &= 0xE7;

            vmachw.rgvia[iVia].vBufBL = b;
            break;

        case 1:
            // writing A' clears the CA1 and CA2 interrupts

            vmachw.rgvia[iVia].vIFR &= 0xFC;

            // fall through

        case 15:
            vmachw.rgvia[iVia].vBufAL = b;
            break;

        case 2:
            vmachw.rgvia[iVia].vDirB = b;
            break;

        case 3:
            vmachw.rgvia[iVia].vDirA = b;
            break;

        case 4:
        case 6:
            vmachw.rgvia[iVia].vT1LL = b;
            break;

        case 5:
            // writing T1H clears T1 interrupt

            vmachw.rgvia[iVia].vIFR &= 0xBF;

            vmachw.rgvia[iVia].vT1CH = b;
            break;

        case 7:
            // writing T1H clears T1 interrupt

            vmachw.rgvia[iVia].vIFR &= 0xBF;

            vmachw.rgvia[iVia].vT1LH = b;
            break;

        case 8:
            vmachw.rgvia[iVia].vT2LL = b;
            break;

        case 9:
            // writing T2H clears T2 interrupt

            vmachw.rgvia[iVia].vIFR &= 0xDF;

            vmachw.rgvia[iVia].vT2CH = b;
            break;

        case 10:
            // writing SR clears SR interrupt

            vmachw.rgvia[iVia].vIFR &= 0xF7;

            vmachw.rgvia[iVia].vSR = b;
            break;

        case 11:
            vmachw.rgvia[iVia].vACR = b;
            break;

        case 12:
            vmachw.rgvia[iVia].vPCR = b;
            break;

        case 13:
            vmachw.rgvia[iVia].vIFR &= ~b;
            break;

        case 14:
            if (b & 0x80)
                vmachw.rgvia[iVia].vIER |= b;
            else
                vmachw.rgvia[iVia].vIER &= ~b;

            break;
            }

        return 0;
        }

#if !defined(NDEBUG) || defined(BETA)
    printf("ILLEGAL fWrite value in VIA\n");
#endif

    return 0;
}


//
// MtDecodeMacAddress
//
// Common code called by the Read/Write routines to normalize an address.
//
// Given an effective address, convert it to a flat 32-bit address
// and take care of wrapping address spaces (repeating ROM images,
// repeating hardware images, etc). Update *pea as appropriate.
//
// Returns: an enum indicating the type of address
//

enum
{
MT_VOID,    // must not return this value
MT_ZERO,    // unused address space that returns 0
MT_ERROR,   // unused address space that bus errors
MT_UNUSED,  // unused address space, ea set to return value
MT_RAM,     // RAM, ea is adjusted relative to eaRAM
MT_ROM,     // ROM, ea is adjusted relative to eaROM[0]
MT_VIDEO,   // NuBus video card, ea is adjusted relative to video card space
MT_HW,      // Hardware, ea adjusted for wrapping
};

ULONG /* __forceinline */ MtDecodeMacAddress(ULONG *pea)
{
    int mt = MT_RAM;

    // All RAM below 8M will be readable in any mode

    if (*pea < vi.cbRAM[0] && (*pea < 0x00800000))
        goto Ldone;

#if TRACEHW
    printf("MtDecode: ea:%08X ", *pea);
#endif

    mt = MT_VOID;

    if (vmCur.bfCPU < cpu68020)
        *pea &= 0x00FFFFFF;

    else if (FIsMac68000(vmCur.bfHW))
        {
        *pea &= 0x00FFFFFF;
        }

    else if ((vmCur.bfCPU == cpu68020) && !(vsthw.lAddrMask & 0xFF000000))
        {
        // map 24-bit to 32-bit addresses (Mac II MMU)

        const BYTE rgb[32] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x40, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0x50,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x40, 0xF9, 0xFA, 0xFB, 0xFE, 0xFE, 0xFE, 0x50 };

        *pea &= 0x00FFFFFF;

        if (vi.cbRAM[1] > 0x80000)
            *pea |= rgb[(((*pea) >> 20) & 15) + 16] << 24;
        else
            *pea |= rgb[((*pea) >> 20) & 15] << 24;
        }

    if (*pea < vi.cbRAM[0])
        mt = MT_RAM;

    else if ((*pea - vi.eaRAM[1]) < vi.cbRAM[1])
        {
#if TRACEVID
        printf("PC:%08X video address 1 is %08X, ", vpregs->PC, *pea);
#endif

        if (FIsMac32(vmCur.bfHW))
            {
#if HIRES
            *pea &= 0x3FFFFF;
#else
            *pea -= vi.eaRAM[1];
#endif
            }
        else
            *pea &= 0x0FFFFF;

#if TRACEVID
        printf("mapped to %08X\n", *pea);
#endif

        mt = MT_VIDEO;
        }

#ifdef MULT_MON
    else if ((*pea >= 0xF6600000) && (*pea < 0xFFF00000))
        {
#if TRACEVID
        printf("PC:%08X video address 2 is %08X, ", vpregs->PC, *pea);
#endif

        if (FIsMac32(vmCur.bfHW))
            *pea -= vi.eaRAM[1];
        else
            *pea &= 0x0FFFFF;

printf("mapped to %08X\n", *pea);

        mt = MT_VIDEO;
        }
#endif

    // Mac Plus/SE RAM wraps when less than 4M of RAM

    else if ((vi.cbRAM[0] < 0x400000) && (*pea < 0x400000))
        {
        *pea = (*pea & 0x07FFFF) + vi.cbRAM[0] - 0x080000;
        mt = MT_RAM;
        }

    // it's a Mac RAM image at $600000

    else if ((*pea >= 0x600000) && (*pea < 0x680000) && FIsMac68000(vmCur.bfHW))
        {
        *pea -= 0x600000;
        mt = MT_RAM;
        }

    // Quick and dirty check for ROM (handles 24-bit mode and non-wrap)

    else if ((*pea - vi.eaROM[0]) < 0x100000)
        {
        *pea &= (max(vi.cbROM[0], 0x40000)) - 1;
        mt = MT_ROM;
        }

    // At this point we've decoded all RAM and ROM space.
    // On 68000 machines, everything else is hardware.

    else if (FIsMac68000(vmCur.bfHW))
        mt = MT_HW;

    // Unused RAM space below ROM

    else if (*pea < vi.eaROM[0])
        {
        // range $40000000..$40800000 wraps around

        if (*pea >= 0x40000000)
            {
            *pea &= vi.cbROM[0] - 1;
            mt = MT_ROM;
            }

        else if (FIsMac32(vmCur.bfHW))
            {
            if (*pea < 128*1024*1024)
                {
                mt = MT_RAM;
                *pea &= (vi.cbRAM[0] - 1);
                }
            else
                mt = MT_ZERO;   // 32-bit clean machines don't wrap
            }

        else if (vmCur.bfHW == vmMacLC)
            {
            mt = MT_UNUSED;
            *pea = 0x10D010D0;  // LC's missing memory pattern
            }
        else
            {
            // wrap unused RAM in Mac II

            mt = MT_RAM;

            if ((vi.cbRAM[0] > 8*1024*1024) && (vi.cbRAM[0] < 20*1024*1024))
                {
                *pea &= (32*1024*1024 -1);

                if (*pea >= vi.cbRAM[0])
                    {
                    mt = MT_UNUSED;
                    *pea = 0x72417241;
                    }
                else
                    *pea &= vi.cbRAM[0] - 1;
                }

            else if (vi.cbRAM[0] == 4*1024*1024)
                {
                *pea &= (8*1024*1024 -1);

                if (*pea >= vi.cbRAM[0])
                    {
                    mt = MT_UNUSED;
                    *pea = 0x72417241;
                    }
                else
                    *pea &= vi.cbRAM[0] - 1;
                }

            // Other memory sizes seems to just work

            else
                *pea &= vi.cbRAM[0] - 1;
            }
        }

    // Mac LC ROMs wrap from $A00000 to $E00000

    else if ((vmCur.bfHW == vmMacLC) && (*pea < 0xE0000))
        {
        *pea &= vi.cbROM[0]-1;
        mt = MT_ROM;
        }

    // Check for 32-bit access to ROM that wrapped

    else if ((*pea >= 0x40000000) && (*pea < 0x50000000))
        {
        // Mac II wraps entire ROM address space.
        // Mac IIci wraps up to $42000000
        // Mac Quadra and PowerMac don't wrap at all.
        
        if (FIsMac32(vmCur.bfHW) && (*pea > 0x42000000))
            mt = MT_ERROR;
        else
            {
            mt = MT_ROM;
            *pea &= vi.cbROM[0]-1;
            }
        }

    // What remains are NuBus machines. Weed out super slot space.
    // And also the reserved area at $F0000000..$F0FFFFFF

#if 0
    else if ((*pea >= 0x60000000) && (*pea < 0xF0000000))
        mt = MT_ZERO;
#endif

    else if ((*pea >= 0x60000000) && (*pea < 0xF1000000))
        mt = MT_ERROR;

    // Machine ID bytes???

    else if ((*pea >= 0x58000000) && (*pea < 0x58001000))
        {
        mt = MT_ZERO;

#if 0
        if (vmCur.bfHW == vmMacQdra)
            {
            *pea = 0xAAAA5555;  // Quadra 610
            mt = MT_UNUSED;
            }
#endif
        }

    else if (1 && (*pea >= 0x5FF00000) && (*pea < 0x5FF80000))
        {
        // 2M ROMs access $5FFxxxxx
// printf("ACCESSING %08X at PC %08X!!\n", *pea, vpregs->PC);

        *pea = 0xFFFFFFFF;  // Quadra 610
        mt = MT_UNUSED;
        }

    else if ((*pea >= 0x7FF00000) && (*pea < 0x5FFFF000))
        {
        // 4M ROMs access $5FF00000
// printf("ACCESSING %08X at PC %08X!!\n", *pea, vpregs->PC);
        mt = MT_ZERO;

        if ((vmCur.bfHW == vmMacQ610) || (vmCur.bfHW == vmMacQ650))
            {
            *pea = 0xA55A2BAD;  // from Quadra 610/650
            mt = MT_UNUSED;
            }
        }

    else if ((*pea >= 0x5FFFF000) && (*pea < 0x60000000))
        {
        // 1M/2M/4M ROMs access $5FFFFFFC
        // this gives them additional machine ID information
        // as follows:

        /*
                    ID  $5FFFFFFC.w  VIA mask
                    ||        ||||         ||
                    vv        vvvv         vv
        GEST ID:CC003A0F FFFC:3013, CPUID: 00
        GEST ID:CC00450F FFFC:3011, CPUID: 00   6100/60
        GEST ID:CC006A0F FFFC:3012, CPUID: 00   7100/66
        GEST ID:CD004210 FFFC:1808, CPUID: 12   PB5xx
        GEST ID:DC00150B FFFC:0001, CPUID: 00   LC3
        GEST ID:DC00180E FFFC:2BAD, CPUID: 46   C650
        GEST ID:DC001D0E FFFC:2BAD, CPUID: 12   Q800
        GEST ID:DC001E0E FFFC:2BAD, CPUID: 52   Q650
        GEST ID:DC00220B FFFC:0000, CPUID: 00   8100/110
        GEST ID:DC002D0E FFFC:2BAD, CPUID: 56
        GEST ID:DC002E0E FFFC:2BAD, CPUID: 40   C610
        GEST ID:DC002F0E FFFC:2BAD, CPUID: 44   Q610
        GEST ID:DC00340E FFFC:2BAD, CPUID: 42   6400/200
        GEST ID:DC00350E FFFC:2BAD, CPUID: 16
        GEST ID:DC00380B FFFC:0003, CPUID: 00   P460
        GEST ID:DC00390E FFFC:2BAD, CPUID: 50
        GEST ID:DD00170A FFFC:1004, CPUID: 00   PB210
        GEST ID:DD001A0A FFFC:1005, CPUID: 00   PB230
        GEST ID:DD00200A FFFC:1006, CPUID: 00   PB250
        GEST ID:DD00210A FFFC:1003, CPUID: 00

        According to Chapter 1: Machine Identification
        in "Power Macintosh Computers".PDF on Apple's developer CD-ROM,
        the following Gestalt values are used for the first 3 Power Mac
        computers, and the following values are written to the lowest 3
        bits at $5FFFFFFC:
                                          Gestalt  3 bits   CPU      RAM
        Power Mac 6100/60 and 6100/60AV:    $4B     000     601    72M-264M
        Power Mac 7100/60 and 7100/60AV:    $70     010     601    72M-264M
        Power Mac 8100/60 and 8100/60AV:    $41     011     601    72M-264M
        Power Mac 7200/75:                  $6C     ???     601     8M-256M
        Power Mac 7500/100:                 ???     ???     601     8M-1G
        Power Mac 8500/100:                 ???     ???     604    16M-1G
        Power Mac 9500/120:                 ???     ???     604    16M-1.5G
        Power Mac 5200/75 and 6200/75:      ???     ???     603     8M-64M
        Power Mac 5400/120:                 ???     ???     603e    8M-136M
        Power Mac 6400/200:                 ???     ???     603e   16M-136M
        Power Mac 7300/200:                 $6D     ???     604e     ???
        Power Mac 8600/250:                 $65     ???     604e     ???
        Power Mac 9600/300:                 $67     ???     604e     ???
        Power Mac G3/233 and G3/266:       $1FE     ???      G3      0-384M

        */

// printf("ACCESSING %08X at PC %08X!!\n", *pea, vpregs->PC);
        mt = MT_ZERO;

        if ((vmCur.bfHW == vmMacQ610) || (vmCur.bfHW == vmMacQ650))
            {
            *pea = 0xA55A2BAD;  // from Quadra 610/650
            mt = MT_UNUSED;
            }

        if (vmCur.bfHW == vmMacQ605)
            {
            *pea = 0xA55A2221;  // from LC 475 / Quadra 605
            mt = MT_UNUSED;
            }
        }

    // If we've decoded so far, we can exit

    if (mt != MT_VOID)
        goto Ldone;

    // Hardware is at $5xxxxxxx

    if (*pea < 0x52000000)
        {
        Assert (*pea >= 0x50000000);

        mt = MT_HW;

        // weed out bogus hardware addresses

        if (FIsMac24(vmCur.bfHW))
            {
            // Macintosh II / IIx / IIcx
            // Hardware ranges wraps around 8 times 128K in 1 meg

            *pea &= 0x0001FFFF;
            *pea |= 0x50F00000;

            switch ((*pea & 0x0001F000) >> 12)
                {
            default:
                mt = MT_ERROR;
                break;

            case 0x00: // VIA1
            case 0x01:
                break;

            case 0x02: // VIA2
            case 0x03:
                break;

            case 0x04: // SCC
            case 0x05:
                break;

            case 0x10: // SCSI
                break;

            case 0x14: // ASC
            case 0x15:
                break;

            case 0x16: // IWM
            case 0x17:
                break;

            case 0x18: // ???
                // Mac Classic II ROMs insist on writing to $50F18000
                // printf("ACCESSING 50F18000 at PC %08X!!\n", vpregs->PC);

                mt = MT_ZERO;
                break;

            case 0x1A: // ???
                // LC580/LC630 ROMs write to $50F1A101 ?!?!?!?
                // printf("ACCESSING %08X at PC %08X!!\n", *pea, vpregs->PC);

                switch (PeekL(vi.eaROM[0]))
                    {
                case 0x35C28F5F:                // LC II
                case 0x3193670E:                // Classic II
                    mt = MT_ERROR;
                    break;

                default:
                    mt = MT_ZERO;
                    break;
                    }
                break;
                }
            }

        else  if (FIsMac32(vmCur.bfHW))
            {
            // Mac IIci / IIsi / LC / Quadra
            // Hardware ranges wraps around in 256K blocks in $5xxxxxxx

#if 0
            if ((*pea < 0x50000000) || (*pea >= 0x51000000))
                {
                    mt = MT_ZERO;
                    mt = MT_ERROR;
                }

            else
#endif
            {
            *pea &= 0x0003FFFF;
            *pea |= 0x50F00000;

// printf("ea = %08X\n", *pea);

            if ((vmCur.bfHW == vmMacQ650) || (vmCur.bfHW == vmMacQ610))
                {
                // Quadra 610/650 hardware

                switch ((*pea & 0x0003F000) >> 12)
                    {
                default:
                    mt = MT_ERROR;
                    break;
    
                case 0x00: // VIA1
                case 0x01:
                    break;
    
                case 0x02: // VIA2
                case 0x03:
                    if (vmachw.V2Base == 0)
                        mt = MT_ERROR;
                    break;
    
                case 0x0C: // SCC
                case 0x0D:
                    break;
    
                case 0x10: // SCSI
                case 0x11:
                    break;
    
                case 0x14: // ASC
                case 0x15:
                    break;
    
                case 0x1E: // IWM
                case 0x1F:
                    break;
    
                case 0x26: // RBV
                case 0x27:
                    if (vmachw.RVBase == 0)
                        mt = MT_ERROR;
                    break;
    
                case 0x08:
                case 0x09:
                case 0x0A:
                case 0x0B:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                    mt = MT_ZERO;
                    break;

                case 0x0E: // machine ID?
#if 0
printf("ACCESSING %08X at PC %08X!!\n", *pea, vpregs->PC);
#endif
                    *pea = 0xFFFFF320;  // value from Quadra 610
                    mt = MT_UNUSED;
                    }
                }
    
            else if ((vmCur.bfHW == vmMacQ700) || (vmCur.bfHW == vmMacQ900))
                {
                // Quadra 700/900 hardware

                switch ((*pea & 0x0003F000) >> 12)
                    {
                default:
                    mt = MT_ERROR;
                    break;
    
                case 0x00: // VIA1
                case 0x01:
                    break;
    
                case 0x02: // VIA2
                case 0x03:
                    if (vmachw.V2Base == 0)
                        mt = MT_ERROR;
                    break;
    
                case 0x0C: // SCC
                case 0x0D:
                    break;
    
                case 0x0F: // SCSI
                    break;
    
                case 0x14: // ASC
                case 0x15:
                    break;
    
                case 0x1E: // IWM
                case 0x1F:
                    break;
    
                case 0x26: // RBV
                case 0x27:
                    if (vmachw.RVBase == 0)
                        mt = MT_ERROR;
                    break;
    
                case 0x08:
                case 0x09:
                case 0x0A:
                case 0x0B:
                case 0x1A:
                case 0x1B:
                case 0x28:
                    mt = MT_ZERO;
                    break;

                case 0x0E: // machine ID?
                    *pea = 0xFFFFFFFF;  // value from Quadra 700
                    mt = MT_UNUSED;
                    }
                }

            else if (vmCur.bfHW == vmMacQ605)
                {
                // Quadra 605 / LC 475 hardware

                switch ((*pea & 0x0003F000) >> 12)
                    {
                default:
                    mt = MT_ERROR;
                    break;
    
                case 0x00: // VIA1
                case 0x01:
                    break;
    
                case 0x02: // VIA2
                case 0x03:
                    if (vmachw.V2Base == 0)
                        mt = MT_ERROR;
                    break;
    
                case 0x0C: // SCC
                case 0x0D:
                    break;
    
                case 0x0F: // SCSI
                    break;
    
                case 0x14: // ASC
                case 0x15:
                    break;
    
                case 0x1E: // IWM
                case 0x1F:
                    break;
    
                case 0x26: // RBV
                case 0x27:
                    if (vmachw.RVBase == 0)
                        mt = MT_ERROR;
                    break;
    
                case 0x08:
                case 0x09:
                case 0x0A:
                case 0x0B:
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1B:
                case 0x28:
                    mt = MT_ZERO;
                    break;

                case 0x0E: // machine ID?
                    *pea = 0xFFFFFFFF;  // value from Quadra 700
                    mt = MT_UNUSED;
                    }
                }
    
            else
                {
                // Default to a Macintosh IIci otherwise
                // also matches PowerMac 6100 hardware

                switch ((*pea & 0x0003F000) >> 12)
                    {
                default:
                    mt = MT_ERROR;
                    break;
    
                case 0x00: // VIA1
                case 0x01:
                    break;
    
                case 0x02: // VIA2
                case 0x03:
                    if (vmachw.V2Base == 0)
                        mt = MT_ZERO;
                    break;
    
                case 0x04: // SCC
                case 0x05:
                    break;
    
                case 0x0E: // ???
                    // Mac Classic II ROMs insist on writing to $50F0E000
                    // printf("ACCESSING 50F0E000 at PC %08X!!\n", vpregs->PC);

                    mt = MT_ZERO;
                    break;
    
                case 0x10: // SCSI
                case 0x11:
                case 0x12:
                case 0x13:
                    break;
    
                case 0x14: // ASC
                case 0x15:
                    break;
    
                case 0x16: // IWM
                case 0x17:
                    break;
    
                case 0x18: // ???
                    // Mac Classic II ROMs insist on writing to $50F18000
                    // printf("ACCESSING 50F18000 at PC %08X!!\n", vpregs->PC);

                    mt = MT_ZERO;
                    break;

                case 0x1A: // ???
                    // LC580/LC630 ROMs write to $50F1A101 ?!?!?!?
                    // printf("ACCESSING %08X at PC %08X!!\n", *pea, vpregs->PC);
    
                    switch (PeekL(vi.eaROM[0]))
                        {
                    case 0x35C28F5F:                // LC II
                    case 0x3193670E:                // Classic II
                        mt = MT_ERROR;
                        break;
    
                    default:
                        mt = MT_ZERO;
                        break;
                        }
                    break;

                case 0x20: // ???
                case 0x21: // ???
                    *pea = 0x4E004E00;  // IIci's random memory pattern
                    mt = MT_UNUSED;
                    break;

                case 0x24: // ???
                case 0x25: // ???
                    mt = MT_ZERO;
                    break;

                case 0x26: // RBV
                case 0x27:
                    if (vmachw.RVBase == 0)
                        mt = MT_ERROR;
                    break;
                    }
                }
            }
            }
        else
            {
#ifdef BETA
            printf("UNKNOWN HARDWARE!\n");
#endif
            }
        }

    // Unused hardware space

    else if (*pea < 0x60000000)
        {
        mt = MT_ERROR;
        }

    // Mac II video card (Toby card)

    else if ((*pea >= (0xF0000000 + 0x01000000 * vmachw.sVideo)) &&
            (*pea <  (0xF1000000 + 0x01000000 * vmachw.sVideo)))
        {
#if TRACEVID
        printf("PC:%08X video address 3 is %08X, ", vpregs->PC, *pea);
#endif

        if (FIsMac32(vmCur.bfHW))
            {
#if HIRES
            *pea &= 0x3FFFFF;
#else
            *pea -= vi.eaRAM[1];
#endif
            }
        else
            *pea &= 0x0FFFFF;

#if TRACEVID
        printf("mapped to %08X\n", *pea);
#endif

        mt = MT_VIDEO;
        }

    // unused slots between $F1000000 and $FF000000

    else if (*pea < 0xFF000000)
        mt = MT_ZERO; // HACK: MT_ERROR makes Mac OS not boot

    // reserved area at $FFxxxxxx

    else
        mt = MT_ERROR;

Ldone:
#if TRACEHW
    printf("new ea:%08X, mt:%d\n", *pea, mt);
#endif

    return mt;
}

//
// MapAddress(ea)
//
// Returns:
//   Map the virtual machine address to a flat address
//   or $FFFFFFFF if the address requires emulation
//

ULONG __cdecl mac_MapAddress(ULONG ea)
{
    switch(MtDecodeMacAddress(&ea))
        {
    default:
#ifdef BETAx
        printf("MapAddress %08X is bogus\n", ea);
#endif
        break;

    case MT_RAM:
        return (ULONG)vi.pbRAM[0] - ea;

    case MT_ROM:
        return (ULONG)vi.pbROM[0] - ea;

    case MT_VIDEO:
        if (ea < vi.cbRAM[1])
            return (ULONG)vi.pbRAM[1] - ea;
        }

    // bogus addresses return NULL

    return NULL;
}


ULONG __cdecl mac_MapAddressRW(ULONG ea)
{
    switch(MtDecodeMacAddress(&ea))
        {
    default:
#ifdef BETAx
        printf("MapAddress %08X is bogus\n", ea);
#endif
        break;

    case MT_RAM:
        return (ULONG)vi.pbRAM[0] - ea;

    case MT_VIDEO:
        if (ea < vi.cbRAM[1])
            return (ULONG)vi.pbRAM[1] - ea;
        }

    // bogus addresses return NULL

    return NULL;
}


//
// ReadHWByte(ea)
//
// Handler for emulating Macintosh hardware byte reads.
//
// Returns:
//  $000000xx - success, xx is return value
//  $8000xxxx - xxxx is trap value
//

static ULONG __cdecl old_ReadHWByte(ULONG ea, BYTE *pb)
{
    ULONG ea0 = ea;

    switch(MtDecodeMacAddress(&ea))
        {
    default:
#if TRACEHW
    printf("ReadHWByte:  PC:%08X ea:%08X BOGUS DECODE!\n", vpregs->PC, ea);
    m68k_DumpRegs();
    m68k_DumpHistory(1600);
#endif

    case MT_ERROR:
#ifndef NDEBUG
        fDebug++;
        printf("Reading byte at bogus address %08X at %08X\n", ea, vpregs->PC);
        m68k_DumpRegs();
        m68k_DumpHistory(160);
        fDebug--;
#endif
        return 0x80000002;

    case MT_HW:
        break;      // handle it below

    case MT_ZERO:
        return 0;

    case MT_UNUSED:
        return (BYTE)(ea >> (24 - ((ea0 & 3) << 3)));

    case MT_RAM:
        return *(BYTE *)(vi.pbRAM[0] - ea);

    case MT_ROM:
        return *(BYTE *)(vi.pbROM[0] - ea);

    case MT_VIDEO:
        if (ea < vi.cbRAM[1])
            {
            // Reading video card RAM

#if 0
            vi.countVMVidR++;
#endif

#if TRACEVID
            printf("%08X: reading Toby RAM at offset %08X\n", vpregs->PC, ea);
#endif

            return *(BYTE *)(vi.pbRAM[1] - ea);
            }

        // For now, hardware locations return 0

        ea -= vi.cbRAM[1];

#if TRACEHW || TRACEVID
        if (ea < 0x07C000)
            printf("%08X: reading Toby HW at %04X %08X\n",
                vpregs->PC, ea, vsthw.lAddrMask);
#endif

        if (ea < 0x030000)
            return 0;

        if (ea < 0x040000)
            return 0;           // $xB0000 range seems to make the video get ok

        if (ea < 0x050000)    // $xC0000 range seems to make the video go crazy!
            return 0;

        if (ea < 0x060000)
            return (GetTickCount() & 0x0F) ? 0 : 0x01;   // beam in VBI flag?

        if (ea < 0x07C000)
            return 0;

        // Toby has 16K of ROM that repeats 4 times for 64K
        // Hi Res card has 64K of unique ROM

#if HIRES
        ea &= 0xFFFF;
#else
        ea &= 0x3FFF;
#endif

        // Toby uses only byte lane 0
        // HiRes uses only byte lane 3

#if HIRES
        if ((ea & 3) != 3)
            return 0;
#else
        if ((ea & 3) != 0)
            return 0;
#endif

#if HIRES
        return (BYTE)rgbHiRes[ea >> 2];
#else
#if 0
    {
    extern int fTraced;

    printf("instr:%d\n", fTraced);
    printf("returning Toby ROM offset %08x = %02X\n",
        ea, (BYTE)~rgbToby[ea >> 2]);
    }
#endif
        {
        extern int fTracing;

#if 0
        if (ea == 0x293C)
            fTracing++;
#endif
        }

        return (BYTE)~rgbToby[ea >> 2];
#endif
        } // switch

#if TRACEHW
    printf("ReadHWByte:  PC:%08X ea:%08X\n", vpregs->PC, ea);
#endif

    if ((ea >= (vmachw.SCSIRd)) &&
        (ea < ((vmachw.SCSIRd) + 0x4000)))
            {
            char rgch[255];

            // Mac SCSI
    
#if TRACESCSI
            printf("Reading SCSI address %08X at %08X\n", ea, vpregs->PC);
//            FDumpRegsVM();
//            CDis(vpregs->PC, TRUE);
#endif

            if (FIsMacNuBus(vmCur.bfHW))
                {
                }
            else if (ea & 1)
                {
                // Reading a write register! Undefined

                goto LunusedMacRead;
                }

            ea = (ea >> 4) & 7;     // SCSI register 0..7

#if TRACESCSI
            sprintf(rgch, "%08X: Reading %02X from SCSI register %d\n",
                vpregs->PC, vmachw.rgbSCSIRd[ea], ea);
            DbgMessageBox(NULL, rgch, vi.szAppName, MB_OK);
#if 0
            if (ea == 6)
                FDumpRegsVM();
#endif
#endif

            if (ea == 6)
                ea = 0;

#if TRACESCSI
            if ((ea == 0) && vi.cSCSIRead && (vi.iSCSIRead >= vi.cSCSIRead))
                {
                printf("WARNING! iSCSIRead = %04X > cSCSIRead = %04X\n",
                    vi.iSCSIRead, vi.cSCSIRead);
                vi.iSCSIRead++;
                }
#endif

            if ((ea == 0) && vi.cSCSIRead && (vi.iSCSIRead < vi.cSCSIRead))
                {
#if TRACESCSI
                printf("returning SCSI buffer offset $%04X: $%02X\n",
                    vi.iSCSIRead, vi.pvSCSIData[vi.iSCSIRead]);
#endif

#if 0
                if ((vi.iSCSIRead+1) == vi.cSCSIRead)
                    {
                    vmachw.sBSR &= ~0x08; // clear phase match
                    vmachw.sBSR &= ~0x01; // clear ACK
                    vmachw.sCSR &= ~0x20; // clear REQ
                    }
#endif

                return vi.pvSCSIData[vi.iSCSIRead++];
                }
            else if (ea == 0)
                {
#if TRACESCSI
                printf("returning SCSI buffer 0\n");
#endif
                return 0;
                }

            return vmachw.rgbSCSIRd[ea];
            }
    
    if ((ea >= (vmachw.IWMBase))
     && (ea < ((vmachw.IWMBase) + 0x1E01)))
            {
            // IWM chip
    
            ea -= vmachw.IWMBase;

            if ((ea & 0xFF) != 0x00)
                {
                // unused IWM address space

#if TRACEIWM
                printf("reading unused IWM address space at %08X\n", ea);
#endif

                if (ea & 1)
                    {
                    // odd bytes read $FF or $80

                    return (ea & 0x200) ? 0x80 : 0xFF;
                    }

                return 0xE1;
                }

            // ea is now $00DFxxFF where XX is $E1, $E3, ... $FF

            ReadWriteIWM(ea);

            if (ea == 0x1800)
                return 0xF5; // checked by Mac boot code

            if (ea == 0x1A00)
                return 0x1F; // checked by Mac boot code

            if (ea == 0x1C00)
                return 0x1F; // checked by Mac boot code

            return 0xE1;
            }

    if ((ea >= vmachw.V1Base) &&
        (ea < (vmachw.V1Base + 0x1E02)))
            {
            // VIA chip
    
            ea -= vmachw.V1Base;

            if (FIsMacNuBus(vmCur.bfHW))
                {
                // Mac II maps each 512 byte address range to each VIA register

                ea &= ~511;
                }

            if (ea == 0x1400)
                {
#if TRACEADB
    printf("reading %02X from SR, PC = %08X\n", vmachw.rgvia[0].vSR, vpregs->PC);
#endif
           //     vmachw.rgvia[0].vIFR &= ~0x04; // clear SR flag
                return vmachw.rgvia[0].vSR;
                }

            if (ea == 0x1A00)
                return (vmachw.rgvia[0].vIFR & vmachw.rgvia[0].vIER & 0x7F) ?
                     (vmachw.rgvia[0].vIFR | 0x80) : (vmachw.rgvia[0].vIFR & 0x7F);

            if (ea == 0x1C00)
                {
#ifdef BETA
//    printf("reading VIA IER = %02X at %08X\n", vmachw.rgvia[0].vIER | 0x80, vpregs->PC);
#endif
                return vmachw.rgvia[0].vIER | 0x80;
                }

            if (ea == 0x0000)
                {
                vmachw.rgvia[0].vIFR &= ~0x18; // clear CB1 CB2 flags
                return vmachw.rgvia[0].vBufB;
                }

            if ((ea == 0x0200) || (ea == 0x1E00))
                {
                vmachw.rgvia[0].vIFR &= ~0x03; // clear CA1 CA2 flags

                if (FIsMac32(vmCur.bfHW))
                    {
                    // A Quadra sets the 4 CPUID bits depending on the model:
                    //
                    // These are the models where $5FFFFFFC is $A55A2BAD
                    // $00 (0000) - 
                    // $12 (0101) - Quadra 800
                    // $16 (0111) - ?? (Gestalt 59)
                    // $40 (1000) - Centris 610
                    // $42 (1001) - Performa 6400/200
                    // $44 (1010) - Quadra 610
                    // $46 (1011) - Centris 650
                    // $50 (1100) - ?? (Gestalt 63)
                    // $52 (1101) - Quadra 650
                    // $56 (1111) - ?? (Gestalt 59)
                    //
                    // These are the models when $5FFFFFFC is 0
                    // $00 (0000) - 
                    // $02 (0010) - Color Classic
                    // $10 (0100) - Quadra 950
                    // $12 (0101) - PowerBook 170 / 180
                    // $16 (0111) - IIsi
                    // $40 (1000) - Quadra 700
                    // $42 (1001) - 
                    // $44 (1010) - 
                    // $46 (1011) - IIci
                    // $50 (1100) - Quadra 900
                    // $52 (1101) - IIfx
                    // $54 (1110) - LC
                    // $56 (1111) - IIci with cache

                    if (vmCur.bfHW == vmMacQ700)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x40;

                    if (vmCur.bfHW == vmMacQ900)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x50;

                    if (vmCur.bfHW == vmMacQ610)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x44;

                    if (vmCur.bfHW == vmMacQ650)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x52;

#if 0
                    if (vmCur.bfHW == vmMacC650)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x46;
#endif

                    if (vmCur.bfHW == vmMacIIsi)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x16;

                    if (vmCur.bfHW == vmMac6100)
                        return (vmachw.rgvia[0].vBufA & ~0x56) | 0x00;

                    // IIci and Centris 650 use 46

                    return (vmachw.rgvia[0].vBufA & ~0x56) | 0x46;
                    }

                else if (FIsMac24(vmCur.bfHW) && (vi.cbRAM[0] > 0x800000))
                    {
                    // If more than 8M with Mac II ROMs, must be Mac IIx ROMs
                    // so twiddle the bits to indicate a Mac IIcx.
                    // The bits must not be set on plain Mac II ROMs!

                    return vmachw.rgvia[0].vBufA |= 0x44; // REVIEW: or $46 ???
                    }

                return vmachw.rgvia[0].vBufA | 0x80;
                }

            if (ea == 0x0400)
                return vmachw.rgvia[0].vDirB;

            if (ea == 0x0600)
                return vmachw.rgvia[0].vDirA;

            if (ea == 0x0800)
                {
                vmachw.rgvia[0].vIFR &= ~0x40; // clear T1 flag
                return vmachw.rgvia[0].vT1CL;
                }

            if (ea == 0x0A00)
                {
                return vmachw.rgvia[0].vT1CH;
                }

            if (ea == 0x0C00)
                return vmachw.rgvia[0].vT1LL;

            if (ea == 0x0E00)
                {
                return vmachw.rgvia[0].vT1LH;
                }

            if (ea == 0x1000)
                {
                UpdateT2();
                vmachw.rgvia[0].vIFR &= ~0x20; // clear T2 flag
                return vmachw.rgvia[0].vT2CL;
                }

            if (ea == 0x1200)
                {
                UpdateT2();
                return vmachw.rgvia[0].vT2CH;
                }

            if (ea == 0x1600)
                {
//    printf("reading %02X from ACR, PC = %08X\n", vmachw.rgvia[0].vACR, vpregs->PC);
                return vmachw.rgvia[0].vACR;
                }

            if (ea == 0x1800)
                return vmachw.rgvia[0].vPCR;
            }
    
    if ((ea >= vmachw.RVBase) &&
        (ea < (vmachw.RVBase + 0x2000)))
            {
            // RBV chip

            ea -= vmachw.RVBase;

// printf("\n\nRBV READ at %08X, PC = %08X\n", ea, vpregs->PC);

            if (ea == 0x1A00)
                ea = 0x03;
            else if (ea == 0x1C00)
                ea = 0x13;
            else if (ea == 0x1E00)
                ea = 0x02;
            else
                ea &= 0x1F;

            switch (ea)
                {
            default:
                return 0;
                break;

            case 0x00:          // fake VIA2 Register B
                return vmachw.rgvia[1].vBufB;

            case 0x01:          // reserved
                return 0;

            case 0x02:          // slot interrupts
                {
                BYTE b;
                b = vmachw.rgvia[1].vBufA & 0x7F; // bit 7 is reserved
                vmachw.rgvia[1].vBufA = 0x7F;
                return b;
                }

            case 0x03:          // interrupt flags
                return (vmachw.rgvia[1].vIFR & 0x7F) ? (vmachw.rgvia[1].vIFR | 0x80) : 0;

            case 0x10:
                return 0x00;    // monitor flags, $00 = no internal video
                return vmachw.rgvia[1].rMP;

            case 0x11:
                return vmachw.rgvia[1].rCT;

            case 0x12:
                return vmachw.rgvia[1].vIER & 0x7F; // bit 7 reads as 0

            case 0x13:
                return vmachw.rgvia[1].rIFE | 0x80; // bit 7 reads as 1
                }
            }

    if ((ea >= vmachw.V2Base) &&
        (ea < (vmachw.V2Base + 0x2000)))
            {
            // VIA2 chip

            ea -= vmachw.V2Base;

            if (FIsMacNuBus(vmCur.bfHW))
                {
                // Mac II maps each 512 byte address range to each VIA register

                ea &= ~511;
                }

            if (ea == 0x1400)
                {
//    printf("reading %02X from SR, PC = %08X\n", vmachw.rgvia[1].vSR, vpregs->PC);
                return vmachw.rgvia[1].vSR;
                }

            if (ea == 0x1A00)
                {
//    printf("reading %02X from VIA2 IFR, PC = %08X\n", vmachw.rgvia[1].vIFR, vpregs->PC);
                return (vmachw.rgvia[1].vIFR & vmachw.rgvia[1].vIER & 0x7F) ?
                     (vmachw.rgvia[1].vIFR | 0x80) : (vmachw.rgvia[1].vIFR & 0x7F);
                }

            if (ea == 0x1C00)
                {
//    printf("reading %02X from VIA2 IER, PC = %08X\n", vmachw.rgvia[1].vIER, vpregs->PC);
                return vmachw.rgvia[1].vIER | 0x80;
                }

            if (ea == 0x0000)
                return vmachw.rgvia[1].vBufB;

            if ((ea == 0x0200) || (ea == 0x1E00))
                {
//    printf("reading %02X from VIA2 bufA, PC = %08X\n", vmachw.rgvia[1].vBufA, vpregs->PC);
                return vmachw.rgvia[1].vBufA & 0x3F;
                }

            if (ea == 0x0800)
                {
                vmachw.rgvia[1].vIFR &= ~0x40; // clear T1
                return vmachw.rgvia[1].vT1CL;
                }

            if (ea == 0x1000)
                {
                UpdateT2();
                return vmachw.rgvia[1].vT2CL;
                }

            if (ea == 0x1200)
                {
                UpdateT2();
                return vmachw.rgvia[1].vT2CH;
                }

            if (ea == 0x0400)
                return vmachw.rgvia[1].vDirB;

            if (ea == 0x0600)
                return vmachw.rgvia[1].vDirA;

            if (ea == 0x0A00)
                return vmachw.rgvia[1].vT1CH;

            if (ea == 0x0C00)
                return vmachw.rgvia[1].vT1LL;

            if (ea == 0x0E00)
                return vmachw.rgvia[1].vT1LH;

            if (ea == 0x1600)
                return vmachw.rgvia[1].vACR;

            if (ea == 0x1800)
                return vmachw.rgvia[1].vPCR;

            return 0;
            }

    if ((ea >= vmachw.SndBase) &&
        (ea < (vmachw.SndBase + 0x2000)))
            {
            BYTE b = 0;

            // Mac II sound chip

            ea -= vmachw.SndBase;
            ea &= 0xFFF;

            if (ea < 0x800)
                {
                b = vmachw.rgbASC[ea];

#if TRACESND
                printf("%08X: Reading sound DATA offset %04X %02X\n", vpregs->PC, ea, b);
#endif
                }

            else if (ea >= 0xC00)
                b = 0;

            else
                {
                // $800...$FFF range has 2K of registers that repeat every
                // 64 bytes followed by 2K of zero

                ea &= 63;

                if (ea >= 16)
                    {
                    // 32-bit hardware registers, need to byte swap

                    b =  vmachw.rgbASC[0x800 + (ea ^ 3)];
                    }

                else switch (ea)
                    {
                default:
                    // hardware byte registers, no need to byte swap

                    b =  vmachw.rgbASC[0x800 + ea];
                    break;

                case 0:
                    // sound type for Mac II is $F
                    b = vmachw.rgbASC[0x800 + ea] = 0x0F;

                    if (vmCur.bfHW == vmMacQ605)
                        {
                        // sound type for Quadra is $BB
                        b = vmachw.rgbASC[0x800 + ea] = 0xBB;
                        }

                    break;

                case 4:             // bStat
                    b = vmachw.rgbASC[0x800 + ea] =
                        (vi.lms & 64) ? 0x0F : 0x05;
                    break;
                    }

#if TRACESND
                printf("%08X: Reading sound HARDWARE offset %04X %02X\n", vpregs->PC, ea, b);
#endif
                }

            return b;
            }

    if ((ea >= vmachw.SCCRBase) &&
        (ea < (vmachw.SCCRBase + 0x40)))
            {
            // SCC chip

            BYTE b;
    
            ea -= vmachw.SCCRBase;

            ea &= 7;

            if ((ea & 1) && FIsMac68000(vmCur.bfHW))
                {
                DebugStr("Bogus SCC offset\n");
                return 0;
                }

            b = sccfRWfABfDCb(fFalse, ea & 2, ea & 4, 0);

            if (ea == 4)
                b = 0;

            if (ea == 6)
                vmachw.aSCC[0].RR[0] &= ~1;

#if 0
            if (ea == 0)
                b = 0x5C;

            if (ea == 2)
                b = 0x0;

            if (ea == 4)
                b = 0x5C;

            if (ea == 6)
                b = 0x0;
#endif

#if TRACEHW
            printf("%08X: reading SCC offset %d, value %d\n",
                vpregs->PC, ea, b);
#endif
            return b;
            }

LunusedMacRead:

    if (FIsMacNuBus(vmCur.bfHW))
        {
#if !defined(NDEBUG) || defined(BETA)
        fDebug++;
        printf("Reading byte at bogus address %08X at %08X\n", ea, vpregs->PC);
        m68k_DumpRegs();
        m68k_DumpHistory(1600);
        fDebug--;
#endif
        return 0x80000002; // bus error
        }

    // Even bytes return $E1, odd bytes $FE (on a Mac Plus)

    return (ea & 1) ? 0xFE : 0xE1;
}


//
// ReadHWWord(ea)
//
// Handler for emulating Macintosh hardware word reads.
// In most cases will just call ReadHWByte.
//
// Returns:
//  $000000xx - success, xx is return value
//  $8000xxxx - xxxx is trap value
//

static ULONG WordFromOB(ULONG l)
{
    return 0xFF00 | l;
}

static ULONG WordFromEB(ULONG l)
{
    if (l & 0xFFFF0000)
        return l;

    return 0xFF | (l << 8);
}

static ULONG __cdecl old_ReadHWWord(ULONG ea, WORD *pw)
{
    ULONG ea0 = ea;
    BOOL f32BitMode = (vsthw.lAddrMask & 0xFF000000);
    ULONG l, l2;
    BYTE dummy;

    if (ea & 1)
      if (vmCur.bfCPU < cpu68020)
        return 0x80000003; // odd address error

    if (f32BitMode && (ea >= 0xF80000) && (ea < 0xF800FF))
        {
        // Mac II boot code check for magic bytes at $F80000

        return 0;
        }

    switch(MtDecodeMacAddress(&ea))
        {
    default:
#if TRACEHW
    printf("ReadHWWord:  PC:%08X ea:%08X BOGUS DECODE!\n", vpregs->PC, ea);
    m68k_DumpRegs();
    m68k_DumpHistory(1600);
#endif

    case MT_ERROR:
#if !defined(NDEBUG) || defined(BETA)
        fDebug++;
        printf("Reading word at bogus address %08X at %08X\n", ea, vpregs->PC);
        m68k_DumpRegs();
        m68k_DumpHistory(1600);
        fDebug--;
#endif
        return 0x80000002;

    case MT_HW:
        break;      // handle it below

    case MT_ZERO:
        return 0;

    case MT_UNUSED:
        return (WORD)(ea >> (16 - ((ea0 & 2) << 3)));

    case MT_RAM:
        return *(WORD *)(vi.pbRAM[0] - 1 - ea);

    case MT_ROM:
        return *(WORD *)(vi.pbROM[0] - 1 - ea);

    case MT_VIDEO:
        if (ea < vi.cbRAM[1])
            {
            // Reading video card RAM

#if 0
            vi.countVMVidR++;
#endif

#if TRACEVID
            printf("%08X: reading Toby RAM at offset %08X\n", vpregs->PC, ea);
#endif

            return *(WORD *)(vi.pbRAM[1] - 1 - ea);
            }

        break;      // handle video hardware it below
        }

    ea = ea0;

#if TRACEHW
    printf("ReadHWWord:  PC:%08X ea:%08X\n", vpregs->PC, ea);
#endif

    l = old_ReadHWByte(ea, &dummy);

    if (l & 0xFFFFFF00)
        return l;

    l2 = old_ReadHWByte(ea+1, &dummy);

    if (l2 & 0xFFFFFF00)
        return l2;

    return (l << 8) | l2;
}


//
// WriteHWByte(ea,b)
//
// Handler for emulating Macintosh hardware byte writes.
//
// Returns: 0 = success, non-zero = trap number
//

ULONG __cdecl old_WriteHWByte(ULONG ea, BYTE *pb)
{
    BYTE b = *pb;
    ULONG ea0 = ea;

    switch(MtDecodeMacAddress(&ea))
        {
    default:
#if TRACEHW
    printf("WriteHWByte: PC:%08X ea:%08X data:%04X BOGUS DECODE!\n", vpregs->PC, ea, b);
#endif

    case MT_ERROR:
#if !defined(NDEBUG) || defined(BETA)
        fDebug++;
        printf("Writing byte at bogus address %08X at %08X\n", ea, vpregs->PC);
        m68k_DumpRegs();
        m68k_DumpHistory(160);
        fDebug--;
#endif
        return 0x80000002;

    case MT_HW:
        break;      // handle it below

    case MT_ZERO:
        return 0;

    case MT_UNUSED:
        return 0;

    case MT_RAM:
        *(BYTE *)(vi.pbRAM[0] - ea) = b;
        return 0;

    case MT_ROM:
        return 0;

    case MT_VIDEO:
        // Mac II video card (Toby card) and NuBus slot space

        if (ea < vi.cbRAM[1])
            {
            // Writing to video card RAM

#if 0
            vi.countVMVidW++;
#endif

#if TRACEVID
        printf("%08X: writing Toby RAM at offset %08X, value %02X\n", vpregs->PC, ea, b);
#endif

            *(BYTE *)(vi.pbRAM[1] - ea) = b;
            return 0;
            }

#if TRACEHW || TRACEVID
        printf("%08X: writing Toby card at offset %08X, value %02X, %08X\n",
            vpregs->PC, ea, b, vsthw.lAddrMask);
#endif

        // For now, hardware locations return 0

        ea -= vi.cbRAM[1];

        switch (ea & 0xFFFFFFFC)
            {
        case 0x020000:
            // clear VBI flag??

            vmachw.rgvia[1].vIFR &= ~2;
            vmachw.rgvia[1].vBufA |= 1 << (vmachw.sVideo - 9);
            return 0;

        case 0x01001C:
            if (((vsthw.planes == 2) && (b == 0x3F)) ||
                ((vsthw.planes == 4) && (b == 0x0F)) ||
                ((vsthw.planes == 8) && (b == 0xFF)))
                {
                vsthw.iColPal = 0;
                vsthw.fColPal = TRUE;
                }
            break;

        case 0x010018:
// printf("fColPal = %d, iColPal = %d\n", vsthw.fColPal, vsthw.iColPal);

            if (vsthw.fColPal)
                {
                switch (vsthw.iColPal % 3)
                    {
                case 0:
                    vsthw.rgrgb[vsthw.iColPal / 3].rgbRed = ~b;
                    break;

                case 1:
                    vsthw.rgrgb[vsthw.iColPal / 3].rgbGreen = ~b;
                    break;

                case 2:
                    vsthw.rgrgb[vsthw.iColPal / 3].rgbBlue = ~b;
                    break;
                    }

                vsthw.rgrgb[vsthw.iColPal / 3].rgbReserved = 0;

                vsthw.iColPal++;

                if (vsthw.iColPal == (1 << vsthw.planes) * 3)
                    {
                    vsthw.fColPal = FALSE;
#if 1
                    vsthw.iColPal = 0;
#endif

                    SetBitmapColors();
                    FCreateOurPalette();

                    // only need to force screen refresh in windowed mode

                    if (!vi.fInDirectXMode)
                      vi.fRefreshScreen = TRUE;
                    }
                }
            break;

        case 0x00003C:
            switch(b)
                {
            default:
            case 0xB7:      // monochrome 640x480
            case 0x37:      // monochrome 640x480
                vsthw.fMono = TRUE;
                vsthw.planes = 1;
#if 0
                vsthw.xpix = 640;
                vsthw.ypix = 480;
#endif
                break;

            case 0x27:      // 4 color 640x480
                vsthw.fMono = FALSE;
                vsthw.planes = 2;
#if 0
                vsthw.xpix = 640;
                vsthw.ypix = 480;
#endif
                break;

            case 0x17:      // 16 color 640x480
                vsthw.fMono = FALSE;
                vsthw.planes = 4;
#if 0
                vsthw.xpix = 640;
                vsthw.ypix = 480;
#endif
                break;

            case 0x06:      // 256 color 640x480
                vsthw.fMono = FALSE;
                vsthw.planes = 8;
#if 0
                vsthw.xpix = 640;
                vsthw.ypix = 480;
#endif
                break;
                }

            CreateNewBitmap();
            return 0;

            break;
            }

        if (ea < 0x07C000)
            return 0;

        // Toby has 16K of ROM that repeats 4 times

        ea &= 0x3FFF;

        // Toby uses only byte lane 0

        if (ea & 3)
            return 0;

        return 0;
        }

#if TRACEHW
    printf("WriteHWByte: PC:%08X ea:%08X data:%02X\n", vpregs->PC, ea, b);
#endif

    if ((ea >= vmachw.SCSIWr) &&
        (ea < (vmachw.SCSIWr + 0x4000)))
            {
            // Mac SCSI
    
#if TRACESCSI
            printf("Writing SCSI address %08X with %02X at %08X\n", ea, b, vpregs->PC);
            FDumpRegsVM();
            CDis(vpregs->PC, TRUE);
#endif

            if (FIsMacNuBus(vmCur.bfHW))
                {
                }
            else if ((ea & 1) == 0)
                {
                // Writing a read register! Undefined

                return 0;
                }

            ea = (ea >> 4) & 0x27;      // SCSI register 0..7 and $20 bit
            WriteMacSCSI(ea, b);
            return 0;
            }
    
    if ((ea >= 0x00600000) && (ea < 0x00680000))
        {
        // it's a Mac RAM image at $600000
    
        *(BYTE *)(vi.pbRAM[0] - (ea = 0x00600000)) = b;
        return 0;
        }
    
    if ((ea >= vmachw.V1Base) &&
        (ea < (vmachw.V1Base + 0x1E02)))
            {
            // VIA chip
    
            ea -= vmachw.V1Base;

            if (FIsMacNuBus(vmCur.bfHW))
                {
                // Mac II maps each 512 byte address range to each VIA register

                ea &= ~511;
                }

            if (ea == 0x1A00)
                {
//                printf("Writing %02X to VIA IFR\n", b);

                vmachw.rgvia[0].vIFR &= ~(b & 0x7F);
                }

            else if (ea == 0x1C00)
                {
//                printf("Writing %02X to VIA IER\n", b);

                if (b & 0x80)
                    vmachw.rgvia[0].vIER |= b;
                else
                    vmachw.rgvia[0].vIER &= ~b;

#if 0
                // HACK! Don't let interrupts get disabled
                vmachw.rgvia[0].vIER |= 7;
#endif
                }

            else if (ea == 0x1400)
                {
#if TRACEADB
                printf("!! ");
                printf("Writing %02X to VIA SR, PC = %08X\n", b, vpregs->PC);
#endif
                vmachw.rgvia[0].vSR = b;

                if (FIsMacADB(vmCur.bfHW))
                    {
                    // REVIEW: ADB commands not handled yet

                    // writing to shift register causes the data to generate
                    // both the bit shift and data shift interrupts

                    vmachw.rgvia[0].vBufB |= 0x08;
                    return 0;
                    }

                // Mac 512 / Mac Plus keyboard commands

                if (b == 0x00)
                    {
                    // Unknown

                    }
                else if (b == 0x10)
                    {
                    // Inquiry

#if 0
                    if (vvmi.keyhead == vvmi.keytail)
                        AddToPacket(0x7B);
#endif
                    }
                else if (b == 0x14)
                    {
                    // Instant

                    }
                else if (b == 0x16)
                    {
                    // Model Number command, respond with $03

                    AddToPacket(0x03);
                    AddToPacket(0x83);
                    AddToPacket(0x7B);
                    }
                else if (b == 0x36)
                    {
                    // Test, return ACK ($7D)

                    AddToPacket(0x7D);
                    }
                }

            else if ((ea == 0x0200) || (ea == 0x1E00))
                {
                // Data register A
                // Only overwrite output pins

                vmachw.rgvia[0].vIFR &= ~0x03; // clear CA1 CA2 flags

// printf("overwriting BUFA with %02X, old value = %02X, ", b, vmachw.rgvia[0].vBufA);
                vmachw.rgvia[0].vBufA &= ~vmachw.rgvia[0].vDirA;
                vmachw.rgvia[0].vBufA |= b & vmachw.rgvia[0].vDirA;
// printf("DIRA = %02X, new value = %02X\n", vmachw.rgvia[0].vDirA, vmachw.rgvia[0].vBufA);
                }

            else if (ea == 0x0000)
                {
                // Data register B
                // Only overwrite output pins

                BYTE vBufBOld = vmachw.rgvia[0].vBufB;

                vmachw.rgvia[0].vIFR &= ~0x18; // clear CB1 CB2 flags

                vmachw.rgvia[0].vBufB &= ~vmachw.rgvia[0].vDirB;
                vmachw.rgvia[0].vBufB |= b & vmachw.rgvia[0].vDirB;

                if ((vBufBOld ^ vmachw.rgvia[0].vBufB) & 0x30)
                  if (FIsMacADB(vmCur.bfHW))
                    {
                    int state;

                    // ADB lines changed

#if TRACEADB
                printf("PC = %08X  ", vpregs->PC);
                printf("ADB changed! old = %02X, new = %02X  ", vBufBOld, vmachw.rgvia[0].vBufB);
                printf("New state = %d\n", (vmachw.rgvia[0].vBufB >> 4) & 3);
#endif

                    state = (vmachw.rgvia[0].vBufB >> 4) & 3;

                    if (state == 0)
                        {
                        // ADB start new command

                        vmachw.bADBCmd = vmachw.rgvia[0].vSR;
                        vmachw.cbADBCmd = 0;
                        vmachw.ibADBCmd = 0;
                        vmachw.fADBR = vmachw.fADBW = 0;
                        memset(&vmachw.rgbADBCmd[0], 0x80, sizeof(vmachw.rgbADBCmd));

                        switch (vmachw.bADBCmd)
                            {
                        default:
#if TRACEADB
                            printf("************ UNKNOWN talk reg\n");
#endif
                            vmachw.rgbADBCmd[0] = 0xFF;
                            vmachw.rgbADBCmd[1] = 0xFF;
                            vmachw.cbADBCmd = 2;
                            vmachw.fADBR = TRUE;
                            break;

                        case 0x2F:
                            // Keyboard Talk register 3

#if TRACEADB
                        printf("************ Keyboard talk reg 3\n");
#endif

                            vmachw.rgbADBCmd[0] = 0x02;    // ADB address 2
                            vmachw.rgbADBCmd[1] = 0x02;    // Device handler ID 2
                            vmachw.cbADBCmd = 2;
                            vmachw.fADBR = TRUE;
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;

                        case 0x2E:
                            // Keyboard Talk register 2

#if TRACEADB
                        printf("************ Keyboard talk reg 2\n");
#endif

                            vmachw.rgbADBCmd[0] = 0xFF;
                            vmachw.rgbADBCmd[1] = 0xFF;
                            vmachw.cbADBCmd = 2;
                            vmachw.fADBR = TRUE;
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;

                        case 0x2C:
                            // Keyboard Talk register 0

#if TRACEADB
                        printf("************ Keyboard talk reg 0\n");
#endif

                            vmachw.rgbADBCmd[0] = (BYTE)vmachw.ADBKey.reg0;
                            vmachw.rgbADBCmd[1] = 0xFF;
                            vmachw.cbADBCmd = 2;
                            vmachw.fADBR = TRUE;
                            vmachw.ADBKey.reg0 = 0xFF;
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;

                        case 0x21:
                            // Keyboard Flush

#if TRACEADB
                        printf("************ Keyboard flush\n");
#endif
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;

                        case 0x3F:
                            // Mouse Talk register 3

#if TRACEADB
                        printf("************ Mouse talk reg 3\n");
#endif

                            vmachw.rgbADBCmd[0] = 0x03;    // ADB address 3
                            vmachw.rgbADBCmd[1] = 0x00;    // Device handler ID 0
                            vmachw.cbADBCmd = 2;
                            vmachw.fADBR = TRUE;
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;

                        case 0x3C:
                            // Mouse Talk register 0

#if TRACEADB
                        printf("************ Mouse talk reg 0\n");
#endif

                            vmachw.rgbADBCmd[0] = (vmachw.ADBMouse.fUp ? 0x80 : 0x00) | 0x00;
                            vmachw.rgbADBCmd[1] = 0x80 | 0x00;
                            vmachw.cbADBCmd = 2;
                            vmachw.fADBR = TRUE;
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;

                        case 0x31:
                            // Mouse Flush

#if TRACEADB
                        printf("************ Mouse flush\n");
#endif
                            vmachw.rgvia[0].vBufB |= 0x08;
                            break;
                            }

                        if (vmachw.rgvia[0].vBufB & 0x08)
                        vmachw.rgvia[0].vIFR |= 0x04;

#if 0
#if TRACEADB
printf("setting bit 3\n");
#endif
                        vmachw.rgvia[0].vBufB |= 0x08;
#endif

#if 0
                        if (vmachw.cbADBCmd)
                            vmachw.rgvia[0].vSR = vmachw.rgbADBCmd[vmachw.ibADBCmd++];
#endif
                        }

                    else if ((state == 1) || (state == 2))
                        {
                        if (vmachw.fADBW)
                            vmachw.rgbADBCmd[vmachw.cbADBCmd++] = vmachw.rgvia[0].vSR;
                        else if (vmachw.fADBR)
                            {
                            if (1 || vmachw.ibADBCmd < vmachw.cbADBCmd)
                                {
                                vmachw.rgvia[0].vSR = vmachw.rgbADBCmd[vmachw.ibADBCmd++];
#if TRACEADB
printf("STUFFING DATA INTO ADB: %02X\n", vmachw.rgvia[0].vSR);
#endif

                                vmachw.rgvia[0].vIFR &= ~0x04;
                                vmachw.rgvia[0].vIFR |= 0x04;
                                }
                            else
                                {
#if 0
                                vmachw.rgvia[0].vBufB &= ~0x08;
#endif
                                vmachw.rgvia[0].vIFR |= 0x04;
                                }
                            }
                        else
                            {
#if 0
                            vmachw.rgvia[0].vBufB &= ~0x08;
#endif
                            vmachw.rgvia[0].vIFR |= 0x04;
                            }
                        }

                    else // (state == 3)
                        {
                        vmachw.rgvia[0].vIFR &= ~0x04;
                        vmachw.rgvia[0].vSR = vmachw.rgbADBCmd[0];
                        }

                //    vmachw.rgvia[0].vSR = 0xFF;
                    }

                if ((vBufBOld ^ vmachw.rgvia[0].vBufB) & 0x06)
                    {
                    // rtcEnb and/or rtcCLK changed, handle it

                    HandleMacRTC(vBufBOld & 7, vmachw.rgvia[0].vBufB & 7);
                    }
                }

            else if (ea == 0x0800)
                {
//    printf("setting T1CL = %02X\n", b);
                vmachw.rgvia[0].vT1CL = b;
                vmachw.fTimer1Going = FALSE;
                }

            else if (ea == 0x0A00)
                {
//    printf("setting T1CH = %02X\n", b);
                vmachw.rgvia[0].vT1CH = b;
                vmachw.rgvia[0].vIFR &= ~0x40;
                vmachw.fTimer1Going = TRUE;
                QueryTickCtr();
                vmachw.lLastEClock1 = GetEclock();
                }

            else if (ea == 0x0C00)
                vmachw.rgvia[0].vT1LL = b;

            else if (ea == 0x0E00)
                {
                vmachw.rgvia[0].vIFR &= ~0x40; // clear T1 flag
                vmachw.rgvia[0].vT1LH = b;
                }

            else if (ea == 0x1000)
                {
//    printf("setting T2CL = %02X\n", b);
                vmachw.rgvia[0].vT2CL = b;
                vmachw.fTimer2Going = FALSE;
                }

            else if (ea == 0x1200)
                {
//    printf("setting T2CH = %02X\n", b);
                vmachw.rgvia[0].vT2CH = b;
                vmachw.rgvia[0].vIFR &= ~0x20; // clear T2 flag
                vmachw.fTimer2Going = TRUE;
                QueryTickCtr();
                vmachw.lLastEClock2 = GetEclock();
                }

            else if (ea == 0x1600)
                {
//    printf("setting ACR = %02X\n", b);
                vmachw.rgvia[0].vACR = b;
                }

            else if (ea == 0x0400)
                vmachw.rgvia[0].vDirB = b;

            else if (ea == 0x0600)
                vmachw.rgvia[0].vDirA = b;

            else if (ea == 0x1800)
                vmachw.rgvia[0].vPCR = b;

            return 0;
            }
    
    if ((ea >= vmachw.RVBase) &&
        (ea < (vmachw.RVBase + 0x2000)))
            {
            // RBV chip

            ea -= vmachw.RVBase;

// printf("\n\nRBV WRITE at %08X, PC = %08X, b = %02X\n", ea, vpregs->PC, b);

            if (ea == 0x1A00)
                ea = 0x03;
            else if (ea == 0x1C00)
                ea = 0x13;
            else if (ea == 0x1E00)
                ea = 0x02;
            else
                ea &= 0x1F;

            switch (ea)
                {
            default:
                break;

            case 0x00:          // fake VIA2 Register B

                if ((vmachw.rgvia[1].vBufB ^ b) & 0x04)
                    {
                    // Soft Power Off bit

                    if ((b & 0x04) == 0)
                        {
#if !defined(NDEBUG) || defined(BETA)
                        printf("RBV power off\n");
#endif
                        ShutdownDetected();
                        }
                    }

                vmachw.rgvia[1].vBufB = b;
                break;

            case 0x01:          // reserved
                break;

            case 0x02:          // slot interrupts
                vmachw.rgvia[1].vBufA = b;
                break;

            case 0x03:          // interrupt flags
                vmachw.rgvia[1].vIFR &= ~(b & 0x7F);
                break;

            case 0x10:
                vmachw.rgvia[1].rMP = b;
                break;

            case 0x11:
                vmachw.rgvia[1].rCT = b;
                break;

            case 0x12:
                if (b & 0x80)
                    vmachw.rgvia[1].vIER |= b;
                else
                    vmachw.rgvia[1].vIER &= ~b;
                break;

            case 0x13:
                if (b & 0x80)
                    vmachw.rgvia[1].rIFE |= b;
                else
                    vmachw.rgvia[1].rIFE &= ~b;
                break;
                }

            return 0;
            }

    if ((ea >= vmachw.V2Base) &&
        (ea < (vmachw.V2Base + 0x2000)))
            {
            // VIA2 chip

            ea -= vmachw.V2Base;

            if (FIsMacNuBus(vmCur.bfHW))
                {
                // Mac II maps each 512 byte address range to each VIA register

                ea &= ~511;
                }

            if (ea == 0x1A00)
                {
//                printf("Writing %02X to VIA IFR\n", b);

                vmachw.rgvia[1].vIFR &= ~(b & 0x7F);
                }

            else if (ea == 0x1C00)
                {
//                printf("Writing %02X to VIA2 IER\n", b);

                if (b & 0x80)
                    vmachw.rgvia[1].vIER |= b;
                else
                    vmachw.rgvia[1].vIER &= ~b;

#if 0
                // HACK! Don't let interrupts get disabled
                vmachw.rgvia[1].vIER |= 7;
#endif
                }

            else if (ea == 0x1400)
                {
//                printf("Writing %02X to VIA2 SR, PC = %08X\n", b, vpregs->PC);
                vmachw.rgvia[1].vSR = b;
                }

            else if ((ea == 0x0200) || (ea == 0x1E00))
                {
                // Data register A
                // Only overwrite output pins

// printf("overwriting BUFA with %02X, old value = %02X, ", b, vmachw.rgvia[1].vBufA);
                vmachw.rgvia[1].vBufA &= ~vmachw.rgvia[1].vDirA;
                vmachw.rgvia[1].vBufA |= b & vmachw.rgvia[1].vDirA;
// printf("DIRA = %02X, new value = %02X\n", vmachw.rgvia[1].vDirA, vmachw.rgvia[1].vBufA);
                }

            else if (ea == 0x0000)
                {
                // Data register B
                // Only overwrite output pins

                BYTE vBufBOld = vmachw.rgvia[1].vBufB;

                vmachw.rgvia[1].vBufB &= ~vmachw.rgvia[1].vDirB;
                vmachw.rgvia[1].vBufB |= b & vmachw.rgvia[1].vDirB;

                if ((vBufBOld ^ vmachw.rgvia[1].vBufB) & 0x08)
                    {
                    // MMU control bit

// printf("MMU control bit = %d\n", !!(b & 8));

                    if (vi.cbROM[0] <= 0x080000)
                      if (vmCur.bfCPU == cpu68020)
                        SetMacMemMode(b & 0x08);
                    }

                if ((vBufBOld ^ vmachw.rgvia[1].vBufB) & 0x04)
                    {
                    // Soft Power Off bit

                    if ((vmachw.rgvia[1].vBufB & 0x04) == 0)
                        {
#if !defined(NDEBUG) || defined(BETA)
                        printf("VIA2 power off\n");
#endif
                        ShutdownDetected();
                        }
                    }
                }

            else if (ea == 0x0800)
                {
//    printf("setting T1CL = %02X\n", b);
                vmachw.rgvia[1].vT1CL = b;
#if 0
                vmachw.fTimer1Going = FALSE;
#endif
                }

            else if (ea == 0x0A00)
                {
//    printf("setting T1CH = %02X\n", b);
                vmachw.rgvia[1].vT1CH = b;
                vmachw.rgvia[1].vIFR &= ~0x40;
#if 0
                vmachw.fTimer1Going = TRUE;
                QueryTickCtr();
                vmachw.lLastEClock1 = GetEclock();
#endif
                }

            else if (ea == 0x0C00)
                vmachw.rgvia[1].vT1LL = b;

            else if (ea == 0x0E00)
                vmachw.rgvia[1].vT1LH = b;

            else if (ea == 0x1000)
                {
//    printf("setting T2CL = %02X\n", b);
                vmachw.rgvia[1].vT2CL = b;
#if 0
                vmachw.fTimer2Going = FALSE;
#endif
                }

            else if (ea == 0x1200)
                {
//    printf("setting T2CH = %02X\n", b);
                vmachw.rgvia[1].vT2CH = b;
                vmachw.rgvia[1].vIFR &= ~0x20;
#if 0
                vmachw.fTimer2Going = TRUE;
                QueryTickCtr();
                vmachw.lLastEClock2 = GetEclock();
#endif
                }

            else if (ea == 0x1600)
                vmachw.rgvia[1].vACR = b;

            else if (ea == 0x0400)
                vmachw.rgvia[1].vDirB = b;

            else if (ea == 0x0600)
                vmachw.rgvia[1].vDirA = b;

            else if (ea == 0x1800)
                vmachw.rgvia[1].vPCR = b;
            return 0;
            }

    if ((ea >= vmachw.SCCWBase) &&
        (ea < (vmachw.SCCWBase + 0x40)))
            {
            // SCC chip
    
            ea -= vmachw.SCCWBase;

#if TRACEHW
            printf("%08X: writing SCC offset %d, value %d\n",
                vpregs->PC, ea, b);
#endif
            ea &= 7;

            if ((ea & 1) && FIsMac68000(vmCur.bfHW))
                return 0;

            sccfRWfABfDCb(fTrue, ea & 2, ea & 4, b);

            if (ea == 6)
                vmachw.aSCC[0].RR[1] |= 1;

            return 0;
            }
    
    if ((ea >= vmachw.IWMBase)
     && (ea < (vmachw.IWMBase + 0x1E01)))
            {
            // IWM chip
    
            ea -= vmachw.IWMBase;

            if ((ea & 0xFF) != 0x00)
                {
#if TRACEIWM
                printf("writing unused IWM address space at %08X\n", ea);
#endif
                return 0;
                }

            // ea is now $00DFxxFF where XX is $E1, $E3, ... $FF

            ReadWriteIWM(ea);
            return 0;
            }


    if ((ea >= vmachw.SndBase) &&
        (ea < (vmachw.SndBase + 0x2000)))
            {
            // Mac II sound chip

            ea -= vmachw.SndBase;
            ea &= 0xFFF;

            if (ea < 0x800)
                {
                static LONG eaTL, eaTR;

#if 1
                vmachw.rgbASC[ea] = b;
#else
                if (ea & 0x400)
                    {
                    eaTR = (eaTR + 1) & 0x3FF;
                    vmachw.rgbASC[eaTR + 0x400] = b;
                    }
                else
                    {
                    eaTL = (eaTL + 1) & 0x3FF;
                    vmachw.rgbASC[eaTL] = b;
                    }
#endif

#if TRACESND
                printf("%08X: Writing sound DATA offset %04X %02X\n", vpregs->PC, ea, b);
#endif

                }

            else if (ea < 0xC00)
                {
                ea &= 63;

#if TRACESND
                printf("%08X: Writing sound HARDWARE offset %04X %02X\n", vpregs->PC, ea, b);
#endif

                if (ea >= 16)
                    {
                    // 32-bit hardware registers, need to byte swap

                    vmachw.rgbASC[0x800 + (ea ^ 3)] = b;
                    }

                else
                    {
                    // hardware byte registers, no need to byte swap

                    vmachw.rgbASC[0x800 + ea] = b;
                    }
                }

            return 0;
            }

    if (FIsMacNuBus(vmCur.bfHW))
        {
#if !defined(NDEBUG) || defined(BETA)
        fDebug++;
        printf("Writing byte at bogus address %08X at %08X\n", ea, vpregs->PC);
        m68k_DumpRegs();
        m68k_DumpHistory(160);
        fDebug--;
#endif
        return 0x80000002; // bus error
        }

    return 0;
}

ULONG __cdecl old_WriteHWWord(ULONG ea, unsigned short *pw)
{
    ULONG w = *pw;
    ULONG ea0 = ea;
    ULONG l;
    BYTE  b;

    if (ea & 1)
      if (vmCur.bfCPU < cpu68020)
        return 0x80000003; // odd address error

    switch(MtDecodeMacAddress(&ea))
        {
    default:
#if TRACEHW
    printf("WriteHWWord: PC:%08X ea:%08X data:%04X BOGUS DECODE!\n", vpregs->PC, ea, w);
        m68k_DumpRegs();
        m68k_DumpHistory(1600);
#endif

    case MT_ERROR:
#if !defined(NDEBUG) || defined(BETA)
        fDebug++;
        printf("Writing word at bogus address %08X at %08X\n", ea, vpregs->PC);
        m68k_DumpRegs();
        m68k_DumpHistory(1600);
        fDebug--;
#endif
        return 0x80000002;

    case MT_HW:
        break;      // handle it below

    case MT_ZERO:
        return 0;

    case MT_UNUSED:
        return 0;

    case MT_RAM:
        *(WORD *)(vi.pbRAM[0] - 1 - ea) = w;
        return 0;

    case MT_ROM:
        return 0;

    case MT_VIDEO:
        if (ea < vi.cbRAM[1])
            {
            // Writing to video card RAM

#if 0
            vi.countVMVidW++;
#endif

//        printf("%08X: writing Toby RAM at offset %08X, value %04X\n", vpregs->PC, ea, w);

            *(WORD *)(vi.pbRAM[1] - 1 - ea) = w;
            return 0;
            }

        break;      // handle it below
        }

    ea = ea0;

#if TRACEHW
    printf("WriteHWWord: PC:%08X ea:%08X data:%04X\n", vpregs->PC, ea, w);
#endif

    b = (BYTE)(w >> 8);
    l = old_WriteHWByte(ea, &b);

    if (l & 0xFFFFFF00)
        return l;

    b = (BYTE)(w & 255);
    return old_WriteHWByte(ea+1, &b);
}


static HRESULT __cdecl mac_ReadHWByte(ULONG ea, BYTE *pb)
{
    ULONG b = old_ReadHWByte(ea, pb);

    if (b & 0x80000000)
        {
        *pb = 0;
        return ERROR_INVALID_ACCESS;
        }

    *pb = (BYTE)(b & 255);
    return NO_ERROR;
}


static HRESULT __cdecl mac_ReadHWWord(ULONG ea, WORD *pw)
{
    ULONG w = old_ReadHWWord(ea, pw);

    if (w & 0x80000000)
        {
        *pw = 0;
        return ERROR_INVALID_ACCESS;
        }

    *pw = (WORD)(w & 0xFFFF);
    return NO_ERROR;
}

static HRESULT __cdecl mac_ReadHWLong(ULONG ea, ULONG *pl)
{
    WORD  w = 0;
    ULONG l = 0;

    if (NO_ERROR == mac_ReadHWWord(ea, &w))
        {
        l = w;

        if (NO_ERROR == mac_ReadHWWord(ea + 2, &w))
            {
            *pl = (l << 16) | (w & 0xFFFF);

            return NO_ERROR;
            }
        }

    *pl = 0;

    return ERROR_INVALID_ACCESS;
}


static HRESULT __cdecl mac_WriteHWByte(ULONG ea, BYTE *pb)
{
    ULONG l = old_WriteHWByte(ea, pb);

    if (l & 0x80000000)
        {
        return ERROR_INVALID_ACCESS;
        }

    return NO_ERROR;
}


static HRESULT __cdecl mac_WriteHWWord(ULONG ea, WORD *pw)
{
    ULONG l = old_WriteHWWord(ea, pw);

    if (l & 0x80000000)
        {
        return ERROR_INVALID_ACCESS;
        }

    return NO_ERROR;
}

static HRESULT __cdecl mac_WriteHWLong(ULONG ea, ULONG *pl)
{
    WORD w = (WORD)(*pl >> 16);

    if (NO_ERROR != mac_WriteHWWord(ea, &w))
        {
        return ERROR_INVALID_ACCESS;
        }

    w = (WORD)(*pl);

    if (NO_ERROR != mac_WriteHWWord(ea + 2, &w))
        {
        return ERROR_INVALID_ACCESS;
        }

    return NO_ERROR;
}


//
// Patch the Mac II video card ROM to set the default screen size
//

void PatchToby()
{
    int i, total = 0;

#if HIRES
    vsthw.xpix = 640;
    vsthw.ypix = 480;
    return;
#endif

    memcpy(rgbToby, rgbTobyRO, sizeof(rgbToby));

    switch(vmCur.bfMon)
        {
    default:
    case monMono:
    case monMacMon:
        vsthw.xpix = 512;
        vsthw.ypix = 342;
        break;

    case monMac12C:
        vsthw.xpix = 640;
        vsthw.ypix = 480;
        break;

    case monMac13F:
        vsthw.xpix = 800;
        vsthw.ypix = 512;
        break;

    case monMac13C:
        vsthw.xpix = 800;
        vsthw.ypix = 600;
        break;

    case monMac14W:
        vsthw.xpix = 864;
        vsthw.ypix = 600;
        break;

    case monMac14W2:
        vsthw.xpix = 832;
        vsthw.ypix = 624;
        break;

    case monMacVPB:
        vsthw.xpix = 1024;
        vsthw.ypix = 480;
        break;

    case monMac14F:
        vsthw.xpix = 1024;
        vsthw.ypix = 600;
        break;

    case monMac15C:
        vsthw.xpix = 1024;
        vsthw.ypix = 768;
        break;

    case monMac16C:
        vsthw.xpix = 1152;
        vsthw.ypix = 864;
        break;

    case monMac16X:
        vsthw.xpix = 1280;
        vsthw.ypix = 960;
        break;

    case monMac17C:
        vsthw.xpix = 1280;
        vsthw.ypix = 1024;
        break;

    case monMac18F:
        vsthw.xpix = 1600;
        vsthw.ypix = 1024;
        break;

    case monMac19C:
        vsthw.xpix = 1600;
        vsthw.ypix = 1200;
        break;

    case monMac20C:
        vsthw.xpix = 1600;
        vsthw.ypix = 1280;
        break;

    case monMac21C:
        vsthw.xpix = 1920;
        vsthw.ypix = 1080;
        break;

    case monMac26C:
        vsthw.xpix = 1920;
        vsthw.ypix = 1200;
        break;

    case monMac28C:
        vsthw.xpix = 1920;
        vsthw.ypix = 1440;
        break;

    case monMacMax:
        vsthw.xpix = min(1920, GetSystemMetrics(SM_CXSCREEN));
        vsthw.ypix = min(1200, GetSystemMetrics(SM_CYSCREEN));
        break;
        }

    // monochrome mode

    rgbToby[0xA74] = (BYTE)~0x00;
    rgbToby[0xA75] = (BYTE)~0x00;
    rgbToby[0xA76] = (BYTE)~0x00;
    rgbToby[0xA77] = (BYTE)~0x20;
    rgbToby[0xA78] = (BYTE)~(vsthw.xpix >> 11);
    rgbToby[0xA79] = (BYTE)~(vsthw.xpix >> 3);
    rgbToby[0xA7E] = (BYTE)~(vsthw.ypix >> 8);
    rgbToby[0xA7F] = (BYTE)~(vsthw.ypix);
    rgbToby[0xA80] = (BYTE)~(vsthw.xpix >> 8);
    rgbToby[0xA81] = (BYTE)~(vsthw.xpix);

#if 1
    // 4 color mode

    rgbToby[0xAC6] = (BYTE)~(vsthw.xpix >> 10);
    rgbToby[0xAC7] = (BYTE)~(vsthw.xpix >> 2);
    rgbToby[0xACC] = (BYTE)~(vsthw.ypix >> 8);
    rgbToby[0xACD] = (BYTE)~(vsthw.ypix);
    rgbToby[0xACE] = (BYTE)~(vsthw.xpix >> 8);
    rgbToby[0xACF] = (BYTE)~(vsthw.xpix);
#else
    // 65536 color mode

    rgbToby[0xAC6] = (BYTE)~(vsthw.xpix >> 7);
    rgbToby[0xAC7] = (BYTE)~(vsthw.xpix << 1);
    rgbToby[0xACC] = (BYTE)~(vsthw.ypix >> 8);
    rgbToby[0xACD] = (BYTE)~(vsthw.ypix);
    rgbToby[0xACE] = (BYTE)~(vsthw.xpix >> 8);
    rgbToby[0xACF] = (BYTE)~(vsthw.xpix);
    rgbToby[0xAE3] = (BYTE)~0x10;
    rgbToby[0xAE7] = (BYTE)~0x10;
#endif

    // 16 color mode

    rgbToby[0xB14] = (BYTE)~(vsthw.xpix >> 9);
    rgbToby[0xB15] = (BYTE)~(vsthw.xpix >> 1);
    rgbToby[0xB1A] = (BYTE)~(vsthw.ypix >> 8);
    rgbToby[0xB1B] = (BYTE)~(vsthw.ypix);
    rgbToby[0xB1C] = (BYTE)~(vsthw.xpix >> 8);
    rgbToby[0xB1D] = (BYTE)~(vsthw.xpix);

    // 256 color mode

    rgbToby[0xB52] = (BYTE)~(vsthw.xpix >> 8);
    rgbToby[0xB53] = (BYTE)~(vsthw.xpix >> 0);
    rgbToby[0xB58] = (BYTE)~(vsthw.ypix >> 8);
    rgbToby[0xB59] = (BYTE)~(vsthw.ypix);
    rgbToby[0xB5A] = (BYTE)~(vsthw.xpix >> 8);
    rgbToby[0xB5B] = (BYTE)~(vsthw.xpix);

    if (FIsMac32(vmCur.bfHW))
        {
        // relocate the video base address to allow for larger video RAM

        rgbToby[0xA74] = (BYTE)~0xFF; // $000020 => $FFE00020
        rgbToby[0xA75] = (BYTE)~0xE0; // $000020 => $FFE00020
        rgbToby[0xA76] = (BYTE)~0x00; // $000020 => $FFE00020
        rgbToby[0xA77] = (BYTE)~0x20; // $000020 => $FFE00020

        rgbToby[0xAC2] = (BYTE)~0xFF; // $000020 => $FFE00020
        rgbToby[0xAC3] = (BYTE)~0xE0; // $000020 => $FFE00020
        rgbToby[0xAC4] = (BYTE)~0x00; // $000020 => $FFE00020
        rgbToby[0xAC5] = (BYTE)~0x20; // $000020 => $FFE00020

        rgbToby[0xB10] = (BYTE)~0xFF; // $000020 => $FFE00020
        rgbToby[0xB11] = (BYTE)~0xE0; // $000020 => $FFE00020
        rgbToby[0xB12] = (BYTE)~0x00; // $000020 => $FFE00020
        rgbToby[0xB13] = (BYTE)~0x20; // $000020 => $FFE00020

        rgbToby[0xB4E] = (BYTE)~0xFF; // $000020 => $FFE00020
        rgbToby[0xB4F] = (BYTE)~0xE0; // $000020 => $FFE00020
        rgbToby[0xB50] = (BYTE)~0x00; // $000020 => $FFE00020
        rgbToby[0xB51] = (BYTE)~0x20; // $000020 => $FFE00020
        }

    // update the CRC

    for (i = 0; i < sizeof(rgbToby); i++)
        {
        total = (total + total) + (total < 0);

        if ((i < (sizeof(rgbToby)-12)) || (i >= (sizeof(rgbToby)-8)))
            total += (BYTE)~rgbToby[i];
//        printf("i = %04X, b = %02X, total = %08X\n", i, rgbToby[i], total);
        }

//    printf("total = %08X\n", total);

    rgbToby[sizeof(rgbToby)- 9] = (BYTE)~total;
    total >>= 8;
    rgbToby[sizeof(rgbToby)-10] = (BYTE)~total;
    total >>= 8;
    rgbToby[sizeof(rgbToby)-11] = (BYTE)~total;
    total >>= 8;
    rgbToby[sizeof(rgbToby)-12] = (BYTE)~total;
}


//
// Initialize Macintosh hardware registers. Most will be cleared.
//

BOOL __cdecl mac_Reset()
{
    int version = osCur.version;
    char szMacPRAM[MAX_PATH];
    void *pv;

    sprintf(szMacPRAM, "%s\\%08X.PRM", vi.szWindowsDir, PeekL(vi.eaROM[0]));

	SetCurrentDirectory(vi.szWindowsDir);
	FWriteWholeFile(szMacPRAM, 256, &vmachw.rgbCMOS);

    memset(&vsthw, 0, sizeof(vsthw));
    memset(&vmachw, 0, sizeof(vmachw));

    // Reset MMU state

    memset((BYTE *)&vi.regs2, 0, sizeof(REGS2));

    if (FIsMacNuBus(vmCur.bfHW))
        SetMacMemMode(TRUE);
    else
        SetMacMemMode(FALSE);

	CbReadWholeFile(szMacPRAM, 256, &vmachw.rgbCMOS);

    if (vmCur.iBootDisk)
        {
        *(ULONG *)&vmachw.rgbCMOS[0x78] = 0xFFFFFFFF;
        vmachw.rgbCMOS[0x7B] = 0xE0 - vmCur.iBootDisk;
        vmachw.rgbCMOS[0x77] = 1;   // seems to need to be 1
        }

//    printf("PRAM is now set to %08X\n", (*(ULONG *)&vmachw.rgbCMOS[0x78]));

    if (FIsMac32(vmCur.bfHW))
        {
        // Macintosh Quadra - 1M ROMs
        // Macintosh IIci - 512K ROMs
        // Macintosh IIsi - 512K ROMs
        // Macintosh LC   - 512K ROMs

        vmachw.V1Base   = 0x50F00000;
        vmachw.SndBase  = 0x50F14000;

        if (FIsMac68030(vmCur.bfHW) || (vmCur.bfHW == vmMac6100))
            {
            // Mac IIci and fake Quadra hardware
            // and looks like PowerMac 6100 as well

            vmachw.V2Base   = 0;
            vmachw.RVBase   = 0x50F26000;

            vmachw.SCCRBase = 0x50F04000;
            vmachw.SCCWBase = 0x50F04000;

            vmachw.IWMBase  = 0x50F16000;
            }
        else
            {
            // real Quadra hardware

            vmachw.V2Base   = 0x50F02000;
            vmachw.RVBase   = 0;

            vmachw.SCCRBase = 0x50F0C000;
            vmachw.SCCWBase = 0x50F0C000;

            vmachw.IWMBase  = 0x50F1E000;
            }

        if ((vmCur.bfHW == vmMacQ700) || (vmCur.bfHW == vmMacQ900))
            {
            vmachw.SCSIRd   = 0x50F0F000;
            vmachw.SCSIWr   = 0x50F0F000;
            }
        else
            {
            vmachw.SCSIRd   = 0x50F10000;
            vmachw.SCSIWr   = 0x50F10000;
            }

        vmachw.sVideo   = 0x0E;
        PatchToby();

        vsthw.fMono = TRUE;
        vsthw.planes = 1;
#if 0
        vsthw.xpix = 640;
        vsthw.ypix = 480;
#endif
        vsthw.dbase     = vi.eaRAM[1] + 32;
        }

    else if (FIsMac24(vmCur.bfHW))
        {
        // Macintosh II IIx IIcx SE/30 - 256K ROMs

        vmachw.V1Base   = 0x50F00000;
        vmachw.V2Base   = 0x50F02000;
        vmachw.RVBase   = 0;
        vmachw.SCCRBase = 0x50F04000;
        vmachw.SCCWBase = 0x50F04000;
        vmachw.SndBase  = 0x50F14000;
        vmachw.IWMBase  = 0x50F16000;
        vmachw.SCSIRd   = 0x50F10000;
        vmachw.SCSIWr   = 0x50F10000;

        vmachw.sVideo   = 0x0E;
        PatchToby();

        vsthw.fMono = TRUE;
        vsthw.planes = 1;
#if 0
        vsthw.xpix = 640;
        vsthw.ypix = 480;
#endif
        vsthw.dbase     = vi.eaRAM[1] + 32;
        }

    else if (FIsMacADB(vmCur.bfHW))
        {
        // Mac Classic  - 512K ROMs
        // Macintosh SE - 256K ROMs

        vmachw.V1Base   = 0x00EFE1FE;
        vmachw.V2Base   = 0;
        vmachw.RVBase   = 0;
        vmachw.SCCRBase = 0x009FFFF8;
        vmachw.SCCWBase = 0x00BFFFF9;
        vmachw.SndBase  = 0;
        vmachw.IWMBase  = 0x00DFE1FF;
        vmachw.SCSIRd   = 0x005FF000;
        vmachw.SCSIWr   = 0x005FF001;

        vsthw.fMono = TRUE;
        vsthw.planes = 1;
        vsthw.xpix = 512;
        vsthw.ypix = 342;
        vsthw.dbase     = vi.eaRAM[1];
        }

    else
        {
        // Macintosh Plus - 128K ROMs
        // Macintosh 512 - 64K ROMs

        vmachw.V1Base   = 0x00EFE1FE;
        vmachw.V2Base   = 0;
        vmachw.RVBase   = 0;
        vmachw.SCCRBase = 0x009FFFF8;
        vmachw.SCCWBase = 0x00BFFFF9;
        vmachw.SndBase  = 0;
        vmachw.IWMBase  = 0x00DFE1FF;
        vmachw.SCSIRd   = 0x00580000;
        vmachw.SCSIWr   = 0x00580001;

        vsthw.fMono = TRUE;
        vsthw.planes = 1;
        vsthw.xpix = 512;
        vsthw.ypix = 342;
        vsthw.dbase     = vi.cbRAM[0] - 0x5900;
        }

#ifdef BETA
    printf("bfHW: %08X\n", vmCur.bfHW);
    printf("VIA1: %08X\n", vmachw.V1Base);
    printf("VIA2: %08X\n", vmachw.V2Base);
    printf("RBV:  %08X\n", vmachw.RVBase);
    printf("SCCR: %08X\n", vmachw.SCCRBase);
    printf("SCCW: %08X\n", vmachw.SCCWBase);
    printf("IWM:  %08X\n", vmachw.IWMBase);
    printf("SCSI: %08X\n", vmachw.SCSIRd);
    printf("ASC:  %08X\n", vmachw.SndBase);

    if (vi.cbROM[0] >= 0x80000)
        {
        // dump out the universal ROM info

        ULONG ofs;
        ULONG i, q;
        ULONG lScan = 0xDC000505;

Lrescan:
        for (i=0, ofs=0; i<0x7FFFC; i+=2, ofs+=2)
            {
            if (PeekL(vi.eaROM[0] + ofs) == lScan)
                {
                ofs -= 16;

                for (q=ofs; q && (PeekL(vi.eaROM[0] + q) != (ofs - q)); )
                    q -= 2;

                if (q > 0)
                    {
                    printf("Offset   ID  HW Addrs "
                           "???????? ???????? ???????? HWCfg    ROM85FPU "
                           "???????? ???????? VIA mask VIA ID                 "
                           "           ROMbase           VIA1     SCCRbase "
                           "SCCWbase IWMbase      SCSIRd   SCSIDMA  SCSIHsk  "
                           "VIA2     SndBase  RVBase   VDACAddr BIOSAddr "
                           "\n");

                    while (ofs = PeekL(vi.eaROM[0] + q))
                        {
                        ULONG base = vi.eaROM[0] + ofs + q;
                        ULONG base2;
                        BYTE id = PeekB(base + 18);
                        WORD hwcfg = PeekW(base + 16);
                        WORD rom85 = PeekW(base + 20);

                        printf("%08x %02x  "
                            "%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X "
                       //     "%08X %08X %08X %08X"
                            " ",
                            q + ofs, id,
                            PeekL(base) + base,
                            PeekL(base + 4) + base,
                            PeekL(base + 8) + base,
                            PeekL(base + 12) + base,
                            PeekL(base + 16),
                            PeekL(base + 20),
                            PeekL(base + 24),
                            PeekL(base + 28),
                            PeekL(base + 32),
                            PeekL(base + 36),
                            PeekL(base + 40),
                            PeekL(base + 48),
                            PeekL(base + 52),
                            0);

                        base2 = PeekL(base) + base;

                        printf("HW: "
                            "%08X %08X %X "
                            "%08X %08X %08X %08X %08X %08X %X %X %08X %08X "
                            "%08X %08X %08X %08X %08X %08X"
                            "\n",
                            PeekL(base2-12),
                            PeekL(base2-8) + base,
                            PeekB(base2-4),
                            PeekL(base2),
                            PeekL(base2 + 4),
                            PeekL(base2 + 8),
                            PeekL(base2 + 12),
                            PeekL(base2 + 16),
                            PeekL(base2 + 20),
                            PeekL(base2 + 24),
                            PeekL(base2 + 28),
                            PeekL(base2 + 32),
                            PeekL(base2 + 36),
                            PeekL(base2 + 40),
                            PeekL(base2 + 44),
                            PeekL(base2 + 48),
                            PeekL(base2 + 52),
                            PeekL(base2 + 56),
                            PeekL(base2 + 60),
                            0);

                        q += 4;
                        }

                    lScan = 0;
                    break;
                    }
                }
            }

        if (lScan == 0xDC000505)
            {
            lScan = 0xDC001008;
            goto Lrescan;
            }

        printf("\n");
        }
#endif

    // Free up Mac RAM from the swap file (all except first 256K)

    if ((vi.cbRAM[1] > 0x40000) && vi.pbRAM[0])
        {
        memset(vi.pbRAM[0] - vi.cbRAM[0] + 1, 0, vi.cbRAM[0] - 0x40000);
        }

    // clear Mac video memory if it is separate from RAM

    if (vi.eaRAM[1] && vi.cbRAM[1] && vi.pbRAM[1])
        {
        memset(vi.pbRAM[1] - vi.cbRAM[1] + 1, 0, vi.cbRAM[1]);
        }

    // this is part of the Mac OS 8 hack

    if (vi.cbROM[0] >= 0x80000)
        *(BYTE *)(vi.pbRAM[1] - (vi.cbRAM[1] - 0x80000)) = 0xFF;

    CreateNewBitmap();
    DisplayStatus();

    BReadWriteVIA(-1, 0, 0, 0);

    // set the sound volume to 7 and on

    vmachw.rgvia[0].vBufA = 7;

    // initialize SCSI state

    vmachw.sCDR  = 0xFF;
    vmachw.sCSR  = 0x5B;
    vmachw.sBSR  = 0x1B;

    vmachw.MacSCSIstate = BUSFREE;

    // initialize VIA2 or RBV

    BReadWriteVIA(-1, 1, 0, 0);

    // Set NuBus interrupt flags off

    vmachw.rgvia[1].vBufA = 0x7F;

    // initialize SCC registers

    vmachw.aSCC[0].RR[0] = 0x3C | 0x40;
    vmachw.aSCC[0].RR[1] = 0x07;
    vmachw.aSCC[1].RR[0] = 0x3C | 0x40;
    vmachw.aSCC[1].RR[1] = 0x07;

    if (FIsMacNuBus(vmCur.bfHW))
        {
        vmachw.aSCC[0].RR[0] = 0x04;
        vmachw.aSCC[0].RR[1] = 0x00;
        vmachw.aSCC[1].RR[0] = 0x04;
        vmachw.aSCC[1].RR[1] = 0x00;
        }

    if (!FInitSerialPort(vmCur.iCOM))
        vmCur.iCOM = 0;
    if (!InitPrinter(vmCur.iLPT))
        vmCur.iLPT = 0;

    if (FIsMacNuBus(vmCur.bfHW))
        InitSound(512);
    else
        InitSound(370);
    InitMIDI();
	FResetIKBD();

    // find the sampled startup sound (1M ROMs and higher)

    if (vi.cbROM[0] >= 0x100000)
        {
        ULONG i;

        for (i = 0xC0000; i < (vi.cbROM[0] - 100); i++)
            {
// printf("checking offset %08X\n", i);
            if (PeekL(vi.eaROM[0] + i) == 0x00010001)
                if (PeekL(vi.eaROM[0] + i + 4) == 0x00050000)
                    if (PeekL(vi.eaROM[0] + i + 10) == 0x00018051)
                        if (PeekL(vi.eaROM[0] + i + 16) == 0x00000014)
                          if (PeekL(vi.eaROM[0] + i + 24) >= 0x3000)
                        {
                        vmachw.iSndStart = i;
                        vmachw.cSndStart = PeekL(vi.eaROM[0] + i + 24);
// printf("found sound at %08X, size %08X\n", i, vmachw.cSndStart);
                    //    break;
                        }
            }
        }
    else
        {
        vmachw.iSndStart = 0;
        vmachw.cSndStart = 0;
        }

    switch (PeekL(vi.eaROM[0]))
        {
    default:
        break;

    case 0x35C28F5F:                // LC II
    case 0x3193670E:                // Classic II
        vmachw.rgbCMOS[0x8A] = 0x05; // need to run in 32-bit mode
        }

    // If we are running on Universal ROMs and the user has not disabled
    // the Quick Boot option, skip the hardware detection.

    // Requires at least Mac II hardware

    if (FIsMac68000(vmCur.bfHW))
        return TRUE;

    // Universal ROMs started with 512K ROMs

    if (vi.cbROM[0] < 0x80000)
        return TRUE;

    // Boot code changed in 2M and larger ROMs
    // so can't use Christian's trick

    if (vi.cbROM[0] > 0x100000)
        return TRUE;

    // Weed out the initial VM reset when the PC is at the start of ROM

    if (vpregs->PC == 0x0000002A)
        return TRUE;

    if (vpregs->PC == (vi.eaROM[0] + 0x2A))
        return TRUE;

    if (vpregs->PC == (vi.eaROM[0] + 0x9A))
        return TRUE;

    // And finally check if the user has disabled this option

    if (v.fNoQckBoot)
        return TRUE;

    // set up UniversalInfoPtr as per Christian Bauer's directions
    //
    // The ROM info structure has the following format:
    //
    // offset $00:
    // offset $04:
    // offset $08:
    // offset $0C: nuBusInfoPtr
    // offset $10: $DD or $DC byte, a $00 byte, the Gestalt ID - 6, unknown byte
    // offset $14: always the bytes $3F FF
    // offset $16: a byte indicating the types of FPU: 4 = optional
    // offset $17: $00 on 512K/1M ROMs, $01 on 2M/4M ROMs
    // offset $18: appears to be a long of hardware info bits

    for (vpregs->A1 = vi.eaROM[0];
         vpregs->A1 < (vi.eaROM[0] + vi.cbROM[0] - 32); vpregs->A1 += 4)
        {
        ULONG l = PeekL(vpregs->A1 + 16);

        // Look for Mac II/IIx/IIcx headers

        if (PeekL(vpregs->A1 + 20) == 0x3FFF0100)
            {
            if (FIsMac24(vmCur.bfHW) &&
                 (l == 0xDC000104)) // Mac IIx
                {
              //  printf("FOUND MAC IIx UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }

            if (FIsMac24(vmCur.bfHW) &&
                 (l == 0xDC000304)) // Mac IIcx
                {
              //  printf("FOUND MAC IIcx UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }

            if (FIsMac24(vmCur.bfHW) &&
                 (l == 0xDC000004)) // Mac II
                {
              //  printf("FOUND MAC II UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }

            if (FIsMac68030(vmCur.bfHW) &&
                 (l == 0xDC000505)) // Mac IIci
                {
              //  printf("FOUND MAC IIci UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }
            }

        // Look for IIsi/LC headers

        if ((PeekL(vpregs->A1 + 20) == 0x3FFF0400) && FIsMac32(vmCur.bfHW))
            {
            if (l == 0xDC000C05) // Mac IIsi
                {
              //  printf("FOUND IIsi UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }

            if (l == 0xDC000D07) // Mac LC
                {
              //  printf("FOUND LC UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }
            }

        // Look for Quadra headers

        if ((PeekL(vpregs->A1 + 20) == 0x3FFF0200) && FIsMac32(vmCur.bfHW))
            {
            if (l == 0xDC000E08) // Mac Quadra 900
                {
              //  printf("FOUND QUADRA 900 UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }

            if (l == 0xDC001408) // Mac Quadra 950
                {
              //  printf("FOUND QUADRA 950 UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }

            if (l == 0xDC001008) // Mac Quadra 700
                {
              //  printf("FOUND QUADRA 700 UNIVERSALINFO at %08X\n", vpregs->A1);
                goto Lunivers;
                }
            }
        }

// printf("DID NOT FIND UNIVERSAL!!\n");

    return TRUE;

Lunivers:
  // printf("FOUND UNIVERSALINFO at %08X\n", vpregs->A1);

    vpregs->A0 = vpregs->A1 + PeekL(vpregs->A1);
    vpregs->A6 = vi.cbRAM[0] - 0x1C;
    vpregs->A7 = 0x10000;
    vpregs->D0 = PeekL(vpregs->A1 + 0x18);
    vpregs->D1 = PeekL(vpregs->A1 + 0x1C);
    vpregs->D2 = PeekL(vpregs->A1 + 0x10) | 0x20000000; // we have CPU
    vpregs->PC = vi.eaROM[0] + 0xBA;

    {
    int i;

    for (i = 0; i < 4096; i+= 4)
        PokeL(vi.cbRAM[0] - 4096 + i, 0);
    }

    PokeL(vpregs->A6       , 0);
    PokeL(vpregs->A6 + 0x04, vi.cbRAM[0]);
    PokeL(vpregs->A6 + 0x08, -1);
    PokeL(vpregs->A6 + 0x0C, 0);
    PokeL(vpregs->A6 + 0x10, 0);

#if 0
    vpregs->A4 = vpregs->A6;
    PokeL(vpregs->A6 - 74, 0x70000000);
    PokeL(vpregs->A6 - 70, 0);
    PokeL(vpregs->A6 - 26, 0);
    PokeL(vpregs->A6 - 25, 5);
    PokeL(vpregs->A6 - 24, 0);
    PokeL(vpregs->A6 - 20, vi.cbRAM[0]);
    PokeL(vpregs->A6 - 12, -92); // negative size of bootglobs
    PokeL(vpregs->A6 - 8,  20);  // positive size of bootglobs
    PokeL(vpregs->A6 - 4,  'WLSC');
    vpregs->A6 = vi.cbRAM[0];
    vpregs->PC = 0x4080010E;
#endif

    return TRUE;
}



//
// VPollInterrupts
//
// Called by asm based PollInterrupts. Checks for any MFP interrupts that
// should go off, and sets the appropriate bits in vrgf.
// Returns the vector number.
//
// REVIEW: needs to be optimized to handle multiple interrupts
//

static ULONG BitFromW(unsigned int i)
{
    ULONG bit = 0;
    ULONG lRet;

    Assert(i);

    for (bit = 0; i; bit++, i>>=1)
        {
        if (i & 1)
            lRet = bit;
        }

    return lRet;
}

ULONG __cdecl mac_VPollInterrupts()
{
        static int sfKeyTimeOut;

//        QueryTickCtr();

        if (!vi.fExecuting)
            return 0;

#if 0
        if (((vpregs->SR & 0x0700) < 0x200) && (vmachw.aSCC[SCCA].RR[0] & 8))
            {
            // SCC interrupt for mouse X movement

printf("sending X interrupt\n");
#if 0
            vmachw.aSCC[SCCA].RR[0] &= ~8;  // clear interrupt bit
            vmachw.aSCC[SCCB].RR[0] |= 8;  // clear interrupt bit
#endif
            return (2 << 24) | 0x1A;       // VBI vector, IPL 2
            }

        if (((vpregs->SR & 0x0700) < 0x200) && (vmachw.aSCC[SCCB].RR[0] & 8))
            {
            // SCC interrupt for mouse Y movement

printf("sending Y interrupt\n");
#if 0
            vmachw.aSCC[SCCB].RR[0] &= ~8;  // clear interrupt bit
            vmachw.aSCC[SCCB].RR[0] |= 8;  // clear interrupt bit
#endif
            return (2 << 24) | 0x1A;       // VBI vector, IPL 2
            }
#endif

        // VIA2 or RBV interrupts

        if (vmachw.V2Base || vmachw.RVBase)
            {
            if (vmachw.rgvia[1].vIFR & vmachw.rgvia[1].vIER & 0x7F)
                {
                vmachw.rgvia[1].vIFR |= 0x80;

                if ((vpregs->SR & 0x0700) < 0x200)
                  if (vmachw.fVIA2)
                    {
#if 0
            printf("generating VIA2 interrupt, vIFR = %02X\n", vmachw.rgvia[1].vIFR);
#endif
#if 0
            printf("2"); fflush(stdout);
#endif
                    return (2 << 24) | 0x1A;    // VIA2 vector, IPL 2
                    }
                }
            else
                vmachw.rgvia[1].vIFR &= 0x7F;
            }

        // T1 interrupts

        if (vmachw.fTimer1Going && vmachw.rgvia[0].vT1Cw)
            {
            ULONG l = vmachw.lLastEClock1;

            QueryTickCtr();
            vmachw.lLastEClock1 = GetEclock();

            if ((vmachw.lLastEClock1 - l) < (ULONG)vmachw.rgvia[0].vT1Cw)
                {
                vmachw.rgvia[0].vT1Cw -= (vmachw.lLastEClock1 - l);
                }
            else
                {
//    printf("T1 interrupt!!\n");
                vmachw.rgvia[0].vIFR |= 0x40;

                if (vmachw.rgvia[0].vACR & 0x40)
                    {
                    vmachw.rgvia[0].vT1CL = vmachw.rgvia[0].vT1LL;
                    vmachw.rgvia[0].vT1CH = vmachw.rgvia[0].vT1LH;
                    }
                else
                    {
                    vmachw.rgvia[0].vT1Cw = 0;
                    vmachw.fTimer1Going = FALSE;
                    }
                }
            }

        // T2 interrupts

        UpdateT2();

        // check for keyboard/mouse packet

        if (vvmi.keyhead != vvmi.keytail)
            {
            sfKeyTimeOut = 1000;

            if ((vpregs->SR & 0x0700) < 0x100)
            if ((vmachw.rgvia[0].vIFR & 0x04) == 0)
            if (vmachw.rgvia[0].vIER & 0x04)  // make sure keyboard interrupt enabled
                {
#if 1
                if (FIsMacADB(vmCur.bfHW))
                    {
                    vmachw.rgvia[0].vIFR |= 4;

                    vmachw.ADBKey.reg0 =
                    vmachw.rgvia[0].vSR = vvmi.rgbKeybuf[vvmi.keytail++];
                    vvmi.keytail &= 1023;

#if TRACEADB
     printf("Sending KEY %02X to Mac, head = %d, tail = %d\n", vmachw.rgvia[0].vSR, vvmi.keyhead, vvmi.keytail);
#endif

                    vmachw.cbADBCmd = 2;
                    vmachw.fADBR = TRUE;

                    if (((vmachw.rgvia[0].vSR == 0xFF) && ((vmachw.bADBCmd & 0xFF) != 0x3C)) ||
                        ((vmachw.rgvia[0].vSR != 0xFF) && ((vmachw.bADBCmd & 0xFF) != 0x2C)))
                        {
                        // if the Mac is querying the wrong ADB device, the data will be eaten
                        // so we'll keep sending the same data byte over and over again until
                        // it is sent to the right device!

                        if (vmachw.fVIA2)
                            vvmi.keytail--;

                        vmachw.rgvia[0].vBufB &= ~0x08;
#if TRACEADB
                    printf("clearing bit 3\n");
#endif
                        }

                    else if (vmachw.rgvia[0].vSR == 0xFF)
                        {
                        // Mouse movement

                        vmachw.rgvia[0].vSR = 
                        vmachw.rgbADBCmd[0] = (vmachw.ADBMouse.fUp ? 0x80 : 0x00) | 0x00;
                        vmachw.rgbADBCmd[1] = 0x80 | min(63, max(-63, (int)(vmachw.ADBMouseX - PeekW(0x82A))));
                        }
                    else
                        {
                        // Keystroke

                        vmachw.rgbADBCmd[0] = vmachw.rgvia[0].vSR;
                        vmachw.rgbADBCmd[1] = 0xFF;
                        }
#if 0
                    // if ADB state is idle and ADB interrupt isn't already signaled
                    // go ahead and pull the pin low

                    if ((((vmachw.rgvia[0].vBufB >> 4) & 3) == 3)
                             && (vmachw.rgvia[0].vBufB & 8))
                        {
                        vmachw.rgvia[0].vBufB &= ~0x08;
#if TRACEADB
                    printf("clearing bit 3\n");
#endif
    //                    return 0;
                        }
#endif
                    }
                else
#endif
                    {
                    vmachw.rgvia[0].vSR = vvmi.rgbKeybuf[vvmi.keytail++];
                    vvmi.keytail &= 1023;

#if TRACEADB
     printf("Sending KEY %02X to Mac, head = %d, tail = %d\n", vmachw.rgvia[0].vSR, vvmi.keyhead, vvmi.keytail);
#endif

#if 0
                    // HACK: F12 is the NMI key

                    if (vmachw.rgvia[0].vSR == 0x65)
                        return (4 << 24) | 0x1C;    // NMI vector, IPL 4
#endif
                    vmachw.rgvia[0].vIFR |= 4;
                    }
                }
            }
        else if (1)
            {
            if (sfKeyTimeOut)
                sfKeyTimeOut--;
            else if (!FIsMacADB(vmCur.bfHW))
                AddToPacket(0x7B);
            else
#if 0
                if ((vmachw.rgvia[0].vIFR & 4) == 0)
#endif
                {
                // ADB Macs - check if scanning for ADB device

                if ((((vmachw.rgvia[0].vBufB >> 4) & 3) == 3)
                         && (vmachw.rgvia[0].vBufB & 8)
                         && ((vmachw.bADBCmd >= 0x40) || (vmachw.bADBCmd < 0x20)))
                    {
#if TRACEADB
                    printf("SCANNING!\n");
#endif

                    vmachw.rgvia[0].vBufB &= ~0x08;
                    vmachw.cbADBCmd = 2;
                    vmachw.fADBR = TRUE;
                    vmachw.rgbADBCmd[0] = 0xFF;
                    vmachw.rgbADBCmd[1] = 0xFF;
                    vmachw.rgvia[0].vIFR |= 4;
                    }
                }
            }
    
        if (vmachw.rgvia[0].vIFR & vmachw.rgvia[0].vIER & 0x7F)
            {
            vmachw.rgvia[0].vIFR |= 0x80;

            if ((vpregs->SR & 0x0700) < 0x100)
                {
#if TRACEADB
        if (vmachw.rgvia[0].vIFR & 0x1C)
            printf("generating VIA interrupt, vIFR = %02X\n", vmachw.rgvia[0].vIFR);
#endif
                return (1 << 24) | 0x19;    // VBI vector, IPL 1
                }
            }
        else
            vmachw.rgvia[0].vIFR &= 0x7F;

        return 0;
}

ULONG __cdecl mac_WinMsg(HWND hWnd, UINT msg, WPARAM uParam, LPARAM lParam)
{
    return FWinMsgST(hWnd, msg, uParam, lParam);
}

#endif // SOFTMAC


