
/***************************************************************************

    GEMTYPES.H

    - basic type definitions for Gemulator
    - basic type definitions and function prototypes for Gemulator
    - hardware state definitions

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008 darekm       Gemulator 9.0 release
    07/13/2007 darekm       updated for Gemulator/SoftMac 9.0
    02/28/2001 darekm       updated for Gemulator/SoftMac 8.1

***************************************************************************/

#pragma once

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP

#undef  _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

#define WindowsSDKDesktopARM64Support 1

#define STRICT
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOICONS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NODRAWTEXT
#define NONLS
#define NOMETAFILE
#define NOMINMAX
#define NOSCROLL
#define NOSERVICE
#define NOTEXTMETRIC
#define NOWH
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <mmsystem.h>
#include <winnt.h>
#include <winioctl.h>
#include <winuser.h>
#include <excpt.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <conio.h>
#include <io.h>
#include <dos.h>
#include <fcntl.h>
#include <time.h>
#include <commdlg.h>    // includes common dialog functionality
#include <dlgs.h>       // includes common dialog template defines
#include <cderr.h>      // includes the common dialog error codes
#include <ddraw.h>
#include "shlobj.h"     // from the Platform SDK, defines for SHFOLDER.DLL

#pragma warning (disable:4214) // nonstandard extension: bitfield types

#include "gemul8r.h"    // build flags

// only one of these should be set at any one time
#define _ENGLISH    1
#define _DEUTSCH    0
#define _FRANCAIS   0
#define _NEDERLANDS 0

//
// Basic Types
//

#if 0
#ifndef VOID        // copied from winnt.h
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif

#ifndef BASETYPES   // copied from windef.h
#define BASETYPES
typedef unsigned char      BYTE;
typedef unsigned char      UCHAR;
typedef unsigned short int WORD;
typedef unsigned short int USHORT;
typedef unsigned long int  ULONG;
typedef int                BOOL;
#endif
#endif

typedef void           (__cdecl *PFN) (int x, ...);
typedef ULONG          (__cdecl *PFNL)(int x, ...);
typedef BYTE *         (__cdecl *PFNP)(int x, ...);

typedef void *         (__fastcall *PHNDLR)(void *, long);

#define fTrue 1
#define fFalse 0

#define k4M     (0x00400000)
#define k8M     (0x00800000)
#define k16M    (0x01000000)
#define k32M    (0x02000000)
#define k64M    (0x04000000)
#define k1G     (0x40000000)
#define k2G     (0x80000000)


#ifdef XFORMER
// the size of an 8 bit screen
#define X8 352
#define Y8 240
#endif

//
// Byte swapping macros
//

#define swab(x) ((WORD)((((WORD)(x & 0xFF00)) >> 8) | (((WORD)(x & 0x00FF)) << 8)))
        
unsigned long __inline SwapW(unsigned long w)
{
    return (((w) & 0xFF00) >> 8) | (((w) & 0x00FF) << 8);
}

unsigned long __inline SwapL(unsigned long l)
{
    return SwapW(((l) & 0xFFFF0000) >> 16) | (SwapW((l) & 0x0000FFFF) << 16);
}

// set higher for smaller dead zone, no lower than 3
#define wJoySens  3

//
// Debug functions
//

#ifndef NDEBUG
#define Assert(f) _assert((f), __FILE__, __LINE__)

__inline void _assert(int f, char *file, int line)
{
    char sz[99];

    if (!f)
        {
        int ret;

        wsprintf(sz, "%s: line #%d", file, line);
        ret = MessageBox(GetFocus(), sz, "Assert failed", MB_APPLMODAL | MB_ABORTRETRYIGNORE);

        if (ret == 3)
            {
            exit(0);
            }
        else if (ret == 4)
            {
            __debugbreak();
            }
        }
}

#define DebugStr printf

#else  // NDEBUG

#define DebugStr __noop
#define Assert(f) (__assume(f))

#endif // NDEBUG

// a handy debug counter which outputs every millionth time through a line

#if !defined(NDEBUG)
#define COUNTER(text) { \
    static unsigned count; \
    if (0 == ((++count) & 1048575)) { \
        printf("%8d: %8dM - %s\n", GetTickCount(), count/1048576, text); } }
#else
#define COUNTER(text) {}
#endif


#define HERTZ CLOCKS_PER_SEC

// !!! these need to be way higher now

// maximum 9 cards with 7 sets of ROMs each

#define MAXOS 63

// maximum number of virtual machines

#define MAXOSUsable  52

// maximum number of virtual machines

#define MAX_VM 108
                

//
// Library prototypes
//

#include "blocklib\blockdev.h"
#include "memlib\memlib.h"
#include "res\resource.h"
#include "cpu.h"

#if defined(ATARIST) || defined(SOFTMAC)
#include "680x0.cpu\68k.h"
#endif


//
// Monitor types
//

enum
    {
    monNone   = 0x00000000,
    monMono   = 0x00000001,         // generic monochrome monitor
    monColor  = 0x00000002,         // generic color monitor
    monGreyTV = 0x00000004,         // B&W TV (grey scales in color)
    monColrTV = 0x00000008,         // Color TV (256 GTIA colors)
    monSTMono = 0x00000010,         // Atari ST monochrome monitor
    monSTColor = 0x00000020,        // Atari ST color monitor

    monMacMon = 0x00000080,         // Macintosh 9" (512x342)
    monMac12C = 0x00000100,         // Macintosh 12" (640x480)
    monMac13F = 0x00000200,         // Flat Panel (800x512)
    monMac13C = 0x00000400,         // Macintosh 13" (800x600)
    monMac14W2 = 0x00000800,         // Macintosh 14" (832x624)
    monMac14W = 0x00001000,         // Macintosh 14" (864x600)
    monMacVPB = 0x00002000,         // VAIO Picturebook (1024x480)
    monMac14F = 0x00004000,         // Flat Panel 14" (1024x600)
    monMac15C = 0x00008000,         // Macintosh 15" (1024x768)
    monMac16C = 0x00010000,         // Macintosh 16" (1152x864)
    monMac16X = 0x00020000,         // Macintosh 16" (1280x960)
    monMac17C = 0x00040000,         // Macintosh 17" (1280x1024)
    monMac18F = 0x00080000,         // Flat Panel 18" (1600x1024)
    monMac19C = 0x00100000,         // Macintosh 19" (1600x1200)
    monMac20C = 0x00200000,         // Macintosh 20" (1600x1280)
    monMac21C = 0x00400000,         // Macintosh 21" (1900x1080)
    monMac26C = 0x00800000,         // Macintosh 26" (1900x1200)
    monMac28C = 0x01000000,         // Macintosh 28" (1900x1440)
    monMacMax = 0x02000000,         // Full Windows Desktop Resolution
    };

extern char const * const rgszMon[];

//
// RAM sizes
//

enum
    {
    ramNone   = 0x00000000,
    ram8K     = 0x00000001,
    ram16K    = 0x00000002,
    ram24K    = 0x00000004,
    ram32K    = 0x00000008,
    ram40K    = 0x00000010,
    ram48K    = 0x00000020,
    ram64K    = 0x00000040,
    ram128K   = 0x00000080,
    ram256K   = 0x00000100,
    ram320K   = 0x00000200,
    ram512K   = 0x00000400,
    ram1M     = 0x00000800,
    ram2M     = 0x00001000,
    ram2_5M   = 0x00002000,
    ram4M     = 0x00004000,
    ram8M     = 0x00008000,
    ram10M    = 0x00010000,
    ram12M    = 0x00020000,
    ram14M    = 0x00040000,
    ram16M    = 0x00080000,
    ram20M    = 0x00100000,
    ram24M    = 0x00200000,
    ram32M    = 0x00400000,
    ram48M    = 0x00800000,
    ram64M    = 0x01000000,
    ram128M   = 0x02000000,
    ram192M   = 0x04000000,
    ram256M   = 0x08000000,
    ram384M   = 0x10000000,
    ram512M   = 0x20000000,
    ram768M   = 0x40000000,
    ram1G     = 0x80000000,
    };

extern char const * const rgszRAM[];

//
// Operating system ROM types
//

enum
    {
    osNone    = 0x00000000,
    osDiskImg = 0x80000000, // VM will load disk image

    // Atari 8-bit ROMs

    osAtari48 = 0x00000001, // Atari 400/800
    osAtariXL = 0x00000002, // Atari 800XL
    osAtariXE = 0x00000004, // Atari 130XE

    // Atari ST/STE/TT ROMs

    osTOS1X_2 = 0x00000010, // 2-chip TOS 1.x ($FC0000)
    osTOS1X_6 = 0x00000020, // 6-chip TOS 1.x ($FC0000)
    osTOS2X_2 = 0x00000040, // 2-chip TOS 2.x ($E00000)
    osTOS3X_4 = 0x00000080, // 4-chip TOS 3.x ($E00000)
    osTOS4X_1 = 0x00000100, // 1-chip TOS 4.x ($E00000)
    osTOS1X_D = 0x00000200, // TOS 1.x boot disk (in RAM)
    osMagic_2 = 0x00000400, // Magic 2.x (in RAM)
    osMagic_4 = 0x00000800, // Magic 4.x
    osMagicPC = 0x00001000, // MagicPC (must decrypt)
    osTOS2X_D = 0x00002000, // TOS 2.x ROM image file
    osTOS3X_D = 0x00004000, // TOS 3.x ROM image file
    osTOS4X_D = 0x00008000, // TOS 4.x ROM image file

    // Macintosh ROMs

    osMac_64  = 0x00010000, // 64K  68000 Mac 128
    osMac_128 = 0x00020000, // 128K 68000 Mac Plus
    osMac_256 = 0x00040000, // 256K 68000 Mac SE
    osMac_512 = 0x00080000, // 512K 68000 Mac Classic
    osMac_II  = 0x00100000, // 256K 68020 Mac II 32-bit dirty (Mac II/IIx/IIcx)
    osMac_IIi = 0x00200000, // 512K 68030 Mac II 32-bit clean (IIci/IIsi/LC)
    osMac_1M  = 0x00400000, // 1M   68040 Mac Quadra 32-bit clean
    osMac_2M  = 0x00800000, // 2M   68040 Mac Quadra 32-bit clean
    osMac_G1  = 0x01000000, // 4M   NuBus PowerMac (6100/7100/8100)
    osMac_G2  = 0x02000000, // 4M   PCI   PowerMac (6400)
    osMac_G3  = 0x04000000, // 4M   USB   PowerMac G3 / iMac
    osMac_G4  = 0x08000000, // 4M   USB   PowerMac G4

    };

extern char const * const rgszROM[];

#define FIsROMAnyAtari68K(os) !!((os) & 0x00001FF0)

#define FIsROMAnyMac68K(os)   !!((os) & 0x007F0000)

enum
    {
    // Atari 8-bit machines without cartridge

    vmAtari48  = 0x00000001,
    vmAtariXL  = 0x00000002,
    vmAtariXE  = 0x00000004,

    // Atari 8-bit machines with cartridge - no longer used, but we need to read in old PROPERTIES that use them

    vmAtari48C = 0x00000080,
    vmAtariXLC = 0x00000100,
    vmAtariXEC = 0x00000800,

    // Tutor 68000 simulators

    vmTutor    = 0x00000008,

    // Atari ST/STE/TT 680x0 machines

    vmAtariST  = 0x00000010,
    vmAtariSTE = 0x00000020,
    vmAtariTT  = 0x00000040,

    // 68000 based Macs (64K 128K 256K 512K ROMs)

    vmMacPlus  = 0x00000200,
    vmMacSE    = 0x00000400,

    // 68020/68030 based 32-bit dirty Macs (256K ROMs)

    vmMacII    = 0x00001000, // needs to match old value
    vmMacIIcx  = 0x00004000, // needs to match old value

    // 68020/68030 based 32-bit clean Macs (512K ROMs)

    vmMacIIci  = 0x00010000, // needs to match old value
    vmMacIIsi  = 0x00020000,
    vmMacLC    = 0x00040000,

    // 68030/68040 based 32-bit clean Macs (1M ROMs)

    vmMacQdra  = 0x00080000, // fake Quadra, really a IIci with a 68040
    vmMacQ605  = 0x00100000, // a.k.a. LC 475
    vmMacQ700  = 0x00200000,
    vmMacQ610  = 0x00400000,
    vmMacQ650  = 0x00800000,
    vmMacQ900  = 0x01000000,

    // PowerPC 601 upgraded Macs (4M ROMs)

    vmMacP900  = 0x02000000,
    vmMacP700  = 0x04000000,
    vmMacP610  = 0x08000000,
    vmMacP650  = 0x10000000,

    // PowerPC 601/603/604 based Power Macs (4M ROMs)

    vmMac6100  = 0x00002000,
    vmMac7100  = 0x00008000,
    vmMac6400  = 0x20000000,

    // PowerPC G3/G4 based New World ROM Macs (4M ROMs)

    vmMac_iMac = 0x40000000,
    vmMac_G4   = 0x80000000,
    };

extern char const * const rgszVM[];

// vmwMask is set to the vm bits that are supported by a given build

#define vmw8bit  (vmAtari48 | vmAtariXL | vmAtariXE /*| vmAtari48C | vmAtariXLC | vmAtariXEC*/)
#define vmwST    (vmAtariST | vmAtariSTE)
#define vmwSTTT  (vmwST     | vmAtariTT)
#define vmwSTET  (vmAtariSTE | vmAtariTT)
#define vmwMac0  (vmMacPlus)
#define vmwMac1  (vmwMac0   | vmMacSE)
#define vmwMac2  (vmMacII   | vmMacIIcx)
#define vmwMac3  (vmMacIIci | vmMacIIsi | vmMacLC   | vmMacQdra)
#define vmwMac4  (vmMacQ900 | vmMacQ700 | vmMacQ610 | vmMacQ650   | vmMacQ605)
#define vmwMac68 (vmwMac0   | vmwMac1   | vmwMac2   | vmwMac3     | vmwMac4)
#define vmwMacU  (vmMacP900 | vmMacP700 | vmMacP610 | vmMacP650)
#define vmwMacG1 (vmMac6100 | vmMac7100 | vmMac6400)
#define vmwMacG3 (vmMac_iMac)
#define vmwMacG4 (vmMac_G4)
#define vmwMacPPC (vmwMacU  | vmwMacG1  | vmwMacG3  | vmwMacG4)
#define vmwMacAll (vmwMac68 | vmwMacPPC)

#ifdef POWERMAC
    #define vmMaskPPC (vmwMacPPC)
#else
    #define vmMaskPPC (0)
#endif

#ifdef SOFTMAC2
    #define vmMaskMII (vmwMac68)
#else
    #define vmMaskMII (0)
#endif

#ifdef SOFTMAC
    #define vmMaskMac (vmMacPlus)
#else
    #define vmMaskMac (0)
#endif

#ifdef ATARIST
    #define vmMaskA16 (vmwSTTT)
#else
    #define vmMaskA16 (0)
#endif

#ifdef XFORMER
    #define vmMaskA8  (vmw8bit)
#else
    #define vmMaskA8  (0)
#endif

// Combine all the masks of the enabled machines

#define vmMask (vmMaskA8 | vmMaskA16 | vmMaskMac | vmMaskMII | vmMaskPPC)

// all Atari 8-bit models
#define FIsAtari8bit(vm)    !!((vm) & vmMask & vmw8bit)

// Atari ST/STE/TT models
#define FIsAtari68000(vm)   !!((vm) & vmMask & vmwST)
#define FIsAtari68K(vm)     !!((vm) & vmMask & vmwSTTT)
#define FIsAtariEhn(vm)     !!((vm) & vmMask & vmwSTET)

// all Macintosh models
#define FIsMac(vm)          !!((vm) & vmMask & vmwMacAll)

// all 680x0 models

#define FIsMac68K(vm)       !!((vm) & vmMask & vmwMac68)

// specific 680x0 models based on processor
#define FIsMac68000(vm)     !!((vm) & vmMask & vmwMac1)
#define FIsMac68020(vm)     !!((vm) & vmMask & vmwMac2)
#define FIsMac68030(vm)     !!((vm) & vmMask & vmwMac3)
#define FIsMac68040(vm)     !!((vm) & vmMask & vmwMac4)

// all PowerPC models

#define FIsMacPPC(vm)       !!((vm) & vmMask & vmwMacPPC)

// all Mac SE and up
#define FIsMacADB(vm)       !!((vm) & vmMask & (vmwMacAll - vmwMac0))

// all 68000 Macs (128 512 Plus SE Classic)
#define FIsMac16(vm)        !!((vm) & vmMask & vmwMac1)

// all Mac II (32-bit dirty, Mac II IIx IIcx SE/30)
#define FIsMac24(vm)        !!((vm) & vmMask & vmwMac2)

// all 32-bit clean Macs (IIci LC Quadra PowerPC)
#define FIsMac32(vm)        !!((vm) & vmMask & (vmwMac3 | vmwMac4 | vmwMacPPC))

// all color Macs (Mac II and up)
#define FIsMacColor(vm)     !!((vm) & vmMask & (vmwMacAll - vmwMac1))

// all NuBus Macs (which currently is treated the same as color Macs, won't be true for PCI!)
#define FIsMacNuBus(vm)     !!((vm) & vmMask & (vmwMacAll - vmwMac1))

#define FIsTutor(vm)        !!((vm) & vmMask & vmTutor)


//
// Helper routines for manipulating bit vectors
//

// Return the string corresponding to the bit in bfth

char *PchFromBfRgSz(ULONG bf, char const * const *rgsz);
BOOL FMonoFromBf(ULONG bf);
ULONG CbRamFromBf(ULONG bf);
ULONG BfFromWfI(ULONG wf, int i);

extern char const * const rgszCPU[];


//
// Operating system description
//

typedef struct _osinfo
    {
    ULONG    osType;        // one of the above
    ULONG    eaOS;          // load address of the OS
    ULONG    cbOS;          // size of the OS
    ULONG    version;       // OS specific version
    ULONG    date;          // OS specific date
    ULONG    country;       // OS specific country info

    union
        {
        struct
            {
            // Disk image settings when fDiskImage

            char        szImg[24];  // disk image file name (X:\8\8.3)
            };
        struct
            {
            // ROM settings when !fDiskImage
            short   ioPort;         // i/o port
            int     iChip;          // starting socket number
            int     cChip;          // number of sockets used
            int     cbSocket;       // size of each socket
            int     cSockets;       // number of sockets on the card
            };
        };

    } OSINFO;


//
// Persistent properties saved to INI file
//

typedef struct
{
    // virtual disk descriptor

    BYTE     dt;           // disk type (from blockdev.h)
    BYTE     ctl;          // SCSI controller
    BYTE     id;           // SCSI device id
    BYTE     xfloppy:1;    // 0 = A:, 1 = B:
    BYTE     res5:5;
    BYTE     mdWP:2;       // 0 = read/write, 1 = read only, 2 = CD-ROM
    char     sz[MAX_PATH]; // path to disk image
} VD, *PVD;

//
// VMINFO structure exported by each virtual machine module
//

typedef struct _vminfo
{
	PFNL pfnVm;             // function to handle VM operations
	ULONG ver;              // version
	BYTE *pchModel;         // default verbose name for this VM
	ULONG wfHW;             // bit vector of supported hardware models
	ULONG wfCPU;            // bit vector of supported CPUs
	ULONG wfRAM;            // bit vector of supported RAM sizes
	ULONG wfROM;            // bit vector of supported ROM chips
	ULONG wfMon;            // bit vector of supported monitors
	PFNL pfnInstall;        // VM installation (one time init)
	PFNL pfnInit;           // VM initialization (switch to this VM)
	PFNL pfnUnInit;         // VM uninit (switch away from VM)
	PFNL pfnInitDisks;      // VM disk initialization
#ifdef HWIN32
	PFNL pfnMountDisk;      // VM disk initialization
#endif
	PFNL pfnUnInitDisks;    // VM disk uninitialization
#ifdef HWIN32
	PFNL pfnUnmountDisk;    // VM disk uninitialization
#endif
	PFNL pfnColdboot;       // VM resets hardware (coldboot)
	PFNL pfnWarmboot;       // VM resets hardware (warmboot)
	PFNL pfnExec;           // VM execute code
	PFNL pfnTrace;          // Execute one single instruction in the VM
	PFNL pfnWinMsg;         // handles Windows messages
	BOOL(__cdecl *pfnDumpRegs)();  // Display the VM's CPU registers as ASCII
	PFNL pfnDumpHW;         // dumps hardware state
	PFNL pfnDisasm;         // Disassemble code in VM as ASCII
	PFNL pfnReadHWByte;     // reads a byte from the VM
	PFNL pfnReadHWWord;     // reads a word from the VM
	PFNL pfnReadHWLong;     // reads a long from the VM
	PFNL pfnWriteHWByte;    // writes a byte to the VM
	PFNL pfnWriteHWWord;    // writes a word to the VM
	PFNL pfnWriteHWLong;    // writes a long to the VM
#ifdef HWIN32
	PFNL pfnLockBlock;      // lock and returns pointer to memory block in VM
	PFNL pfnUnlockBlock;    // release memory block in VM
	PFNP pfnMapAddress;     // convert virtual machine address to flat address
	PFNP pfnMapAddressRW;   // convert virtual machine address to flat address
	PFNL pfnSaveState;      // save snapshot to disk
	PFNL pfnLoadState;      // load snapshot from disk and resume
#endif
} VMINFO, *PVMINFO;

typedef struct _cart
{
	char szName[MAX_PATH];
	int  cbData;        // actual amount of data on the cart
	BYTE fCartIn;		// there is a cartridge plugged in
	char temp;			// make it the same size as a VD structure
} CART, *PCART;

typedef struct
{
    // virtual machine descriptor

    char     szModel[28];  // verbose name of current VM model

    // Many global settings from Gemulator 98/99/2000 are now
    // VM specific so that things like boot disk don't conflict.
    // For backward compatibility with Gemulator 98/99/2000
    // the total size of szModel and flags must be 32 bytes

    // 32 bits of unsigned values and flags

    int      fSound:1;      // enable sound output
    int      fJoystick:1;   // enable joystick support
    int      iCOM:3;        // 0 = no COM port, 1 = COM1:, ... 4 = COM4:
    int      iLPT:2;        // 0 = no LPT port, 1 = LPT1:, ... 3 = LPT3:
    int      fShare:1;      // 0 = hog the printer port, 1 = share it

    int      xCart:1;       // install cartridge ROM if present
    int      xPatchROM:1;   // patch an older ROM to a newer version
    int      iBootDisk:5;    // 0 = floppy, 2 = C:, 3 = D:, etc.
    int      fCPUAuto:1;    // TRUE means VM determines the CPU

    int      fBlitter:1;    // enable blitter hardware in Atari ST mode
    int      fMIDI:1;       // enable MIDI hardware emulation
    int      fUseVHD:1;     // TRUE means to use virtual disks
    int      fSwapKeys:1;   // use alternate keyboard layout
    int      xQuickBaud:1;  // Acceleration: activate fast baud rates
    int      fWarmReset:1;  // if set, this VM needs a warm boot
    int      fColdReset:1;  // if set, this VM needs a cold boot
    int      fValidVM:1;    // if set, this VM is initialized

    int      res83:8;

    PVMINFO  pvmi;          // pointer to this VM's VMINFO data
    ULONG    bfHW;          // current hardware (bit index into rgfHW)
    ULONG    bfCPU;         // current hardware (bit index into rgfCPU)
    ULONG    bfMon;         // current monitor (bit index into rgfMon)
    ULONG    bfRAM;         // current RAM size (bit index into rgfRAM)
    int      iOS;           // current OS index (varies by VM)
    int      ivdMac;        // maximum index into rgvd

#ifdef XFORMER
	VD		 rgvd[8];		// disk descriptors
	CART	 rgcart;		// 8 disks and a cartridge for 8 bit
#else
	VD		 rgvd[9];		// disk descriptors
#endif
	} VM, *PVM;

//
// Cartridge stuff
//

#define CART_8K     0
#define CART_16K    1
#define CART_OSS    2   // Mac65 etc.

#define MAX_CART_SIZE 16384
extern char FAR rgbSwapCart[MAX_VM][MAX_CART_SIZE];

void ReadCart();
void InitCart(int iVM);
void BankCart(int iVM, int i);

BOOL FAddVM(PVMINFO pvmi, int *pi);

#define vmCur (*vi.pvmCur)
#define osCur v.rgosinfo[vmCur.iOS]

//
// Per-VM instance data (is not saved between sessions)
//

typedef struct VMINST
{
    DWORD  iVM;             // index into v.rgvm

    void *pvBits;           // pointer to current bitmap
    HBITMAP hbm;            // handle of bitmap
    HBITMAP hbmOld;         // handle of previous bitmap
    HDC  hdcMem;            // hdc of memory context

    BOOL fInited;
    int  keyhead;           // keyboard buffer head
    int  keytail;           // keyboard buffer tail
    BYTE rgbKeybuf[1024];   // circular keyboard buffer
} VMINST, *PVMINST;

extern VMINST vrgvmi[MAX_VM];

#define vpvmi ((VMINST *)&vrgvmi[v.iVM])
#define vvmi  (*(VMINST *)&vrgvmi[v.iVM])
// #define vvmi  (*(VMINST *)vi.pvmiCur)

// __declspec(thread) VMINST vmi;


typedef struct
{
    // This is to identify this particular version of the structure

    unsigned cb;            // size of this structure
    unsigned wMagic;        // magic word (for versioning)

    // ROM card and OS settings

    unsigned ioBaseAddr;    // base i/o port address of ROM card(s)
    unsigned cCards;        // number of ROM cards

    OSINFO   rgosinfo[MAXOS];
    unsigned cOS;           // count of valid OSes, or 0

    unsigned iVM;           // our current instance
    unsigned cVM;           // total number of valid VM entries
    VM       rgvm[MAX_VM];  // VM descriptors (indexed by iVM)

    // to speed up boot speed

    ULONG    cbTryAlloc;    // previous large block size that worked
    void    *pvTryAlloc;    // previous large block address that worked

    // obsolete Atari ST/STE specific settings used as placeholders now

    BOOL    _fSTESound;
    BOOL    _fUseVHD;       // Hard disk: use virtual disks
    unsigned _BootDisk;     // 0 = floppy, 2 = C:, 3 = D:, etc.
    BOOL    _fMacROMs;      // install Mac ROMs from card if present
    BOOL    _fSwapUKUS;     // patch and swap UK/US keyboard tables

    // Common OS settings

    BOOL    _xQuickBaud;    // Acceleration: activate fast baud rates
    BOOL    _xQuickPrint;   // Acceleration: hook Bconout(printer)
    BOOL    _xQuickST;      // Acceleration: hook Bconout and VDI
    BOOL    _xPatchROM;     // patch an older ROM to a newer version

    // Common hardware settings

    BOOL    _fJoystick;
    BOOL    _fSTSound;
    unsigned _COM;          // 0 = no COM port, 1 = COM1:, etc.
    unsigned _LPT;          // 0 = no LPT port, 1 = LPT1:, etc.
    BOOL    _fShare;        // 0 = hog the printer port, 1 = share it

    // Display settings and Misc. options

    ULONG    lCPUID1;       // 32-bit CPU identification word 1
    ULONG    lCPUID2;       // 32-bit CPU identification word 2
    ULONG    lCPUID3;       // 32-bit CPU identification word 3

    BOOL     fSkipStartup:8;// bit 0 = skip splash, bit 1 = one time unhybernate
    BOOL     fNoQckBoot:8;  // 0 = don't skip hardware check in BIOS
    BOOL     fNoMono:1;     // 0 = use mono bitmaps, 1 = use 256 color for mono
    BOOL             :1;    // reserved (deprecated)
    BOOL     fSaveOnExit:1; // 0 = normal, 1 = save INI file on exit
    BOOL     fHibrOnExit:1; // 0 = normal, 1 = hibernate on exit
	BOOL     fTiling : 1;     // 0 = normal, 1 - tile the display (was RESERVED)
    BOOL     fZoomColor:1;  // 0 = normal, 1 = zoom low rez and medium rez
    BOOL     fFullScreen:1; // 0 = normal, 1 = display in full screen mode
	BOOL     fCPUID:1;      // 0 = old,    1 = CPUID values are valid
    BOOL     fPrivate:8;    // set to indicate we're using private VM settings


    BOOL     fDebugMode:8;  // 1 = put CPU into debug mode
    BOOL     fNoSCSI:8;     // 1 = disables SCSI support
    BOOL               :8;  // reserved (deprecated)
    BOOL     fNoTwoBut:8;   // 1 = disables use of two mouse buttons (force F11)

    BOOL    _mdCPU:8;       // -1 = Auto, 0 = 68000, 1 = 68010, 2 = 68020, etc.
    BOOL     fNoDDraw:8;    // 1 = no preload of DirectX, 2 = disable DirectX
    BOOL     fNoWW:8;       // 1 = disables Windows 98 write watch
    BOOL     fNoJit:8;      // 1 = disable jitter

    RECT     rectWinPos;    // saved window pos (want top left corner only)

} PROPS;

extern PROPS v;

#include "vm.h"	// after definition of PROPS

// In SoftMac 2000 we add the global path setting for ROM file images.
// This is aliased off the end of rgosinfo for backward compatibility

#define rgchGlobalPath rgosinfo[58]

// In SoftMac 2000 we add the registration key and user name protection.
// This is aliased off the end of rgosinfo for backward compatibility

#define rgchEncrypt    rgosinfo[MAXOSUsable]
#define vpes           ((_STS_ENCRYPT_STRUCT *)&v.rgchEncrypt)

//
// Global instance data (is not saved between sessions)
//

#if defined(ATARIST) || defined(SOFTMAC)
REGS vregs;             // 680x0 commonly used registers and TLB structures
#endif

#define SAMPLE_RATE 48000
#define SNDBUFS     6
#define SAMPLES_PER_VOICE SAMPLE_RATE / 60	// 1/60th of a second, one buffer per VBI

typedef struct
{
    BOOL fWin32s;       // true if on Win32s, otherwise Win32

    BYTE rgbShiftMs[6]; // shift coefficients to convert tick count to ms

    int  fRefreshScreen:1; // true if the window needs redrawing
    int  fHaveFocus:1;  // true if Gemulator window is on top
    int  fWinNT:1;      // true if on NT, otherwise Win95 or Win 3.1
    int  fGEMMouse:1;   // true if mouse is currently captured
    int  fMouseMoved:1;
    int  fVMCapture:1;  // true if the VM needs to capture the mouse
    int  fFake040:1;    // TRUE when we're 68030 faking 68040 (enabled MOVE16)
    int  fQuitting:1;   // TRUE when all threads should quit!

    int  fExecuting:1;  // TRUE if emulator is executing
    int  fInDebugger:1; // TRUE if debugger console is active
    int  fDebugBreak:1; // TRUE to signal a break into debugger
    int  fParentCon:1;  // TRUE if parent console present (i.e. command prompt)
    int  fRes4:4;

    BYTE rgbShiftE[6];  // shift coefficients to convert tick count to E clk

    char fXscale;       // non-zero scaling of bitmap horizontally
    char fYscale;       // non-zero scaling of bitmap vertically

    union
        {
        __int64 llTicks;// QueryPerformanceCounter() result
        ULONG rglTicks[2];
        };

	ULONGLONG qpcCold;
	ULONGLONG qpfCold;

    ULONG lms;          // 32-bit millisecond counter
    ULONG lEclk;        // 32-bit E clock counter

    // All virtual machine state memory comes from one large allocation

    ULONG cbAlloc;      // size of the large allocation
    ULONG *plAlloc;     // guest-to-host address map table allocation

    ULONG eaROM[4];     // guest effective address of up to 4 banks of ROM
    ULONG cbROM[4];     // sizes of up to 4 banks of ROM
    ULONG eaRAM[4];     // guest effective address of up to 4 banks of RAM
    ULONG cbRAM[4];     // size of RAM (starts at eaRAM)
    BYTE *pbROM[4];     // host pointer to first guest byte of RAM block
    BYTE *pbRAM[4];     // host pointer to first guest byte of RAM block

    PVM  pvmCur;        // pointer to v.rgvm[v.iVM]
    PVMINST pvmiCur;    // pointer to vrgvmi[v.iVM]

#if defined(ATARIST) || defined(SOFTMAC)
    REGS *pregs;        // pointer to registers and large memory block
    REGS2 regs2;        // 68030/68040 rarely used registers
#endif

    HANDLE hFrameBuf;   // frame buffer in shared memory
    BYTE *pbFrameBuf;   // frame buffer in shared memory
    BYTE *pbAudioBuf;   // audio buffer in shared memory

    HANDLE hAccelTable; // accelerator table
    HWND hWnd;          // main window handle
    HMENU hMenu;        // pop-up menu handle
    HDC  hdc;           // hdc of screen
    ULONG cbScan;       // bytes per scan in DIB section
    HPALETTE hpal;      // palette for color modes - obsolete
    LONG fVideoEnabled; // set to allow video refresh or bitmap change, zero means busy
    BOOL fVideoWrite;   // set by exception handler, cleared by video refresh
    HANDLE hVideoThread; // handle of video thread
    HANDLE hIdleThread; // handle of idle timer thread
    BOOL fInDirectXMode; // true if really in full screen DirectX mode
    int  sx, sy;        // screen X and Y size in Direct X mode
    int  dx, dy;        // screen offset to StretchBlt when in DirectX mode

    int  cPrintTimeout; // counts down to 0, when 0, closes LPT port

	WAVEOUTCAPS woc;    // wave output capabilities structure
	int  fWaveOutput;   // true if suitable wave output device found
	int  iWaveOutput;   // if fWaveOutput, the identifier of the device
	WAVEHDR rgwhdr[SNDBUFS]; // sound buffers
	char rgbSndBuf[SNDBUFS][SAMPLES_PER_VOICE * 2 * 2];// sound buffer data

    HANDLE hROMCard;    // on NT, the handle to the Gemulator device
    unsigned ioPort;    // current port being scanned

    char *szAppName;    // message box title
    char *szTitle;      // title bar text

    int  cchserial;     // how many bytes in serial read buffer

    char *pszOutput;    // printf capture buffer

    BYTE *pvSCSIData;   // huge SCSI transfer buffer
    ULONG cSCSIRead;    // total size of data in pvSCSIData
    ULONG iSCSIRead;    // current index into pvSCSIData
    ULONG cbSCSIAlloc;  // current committed size of pvSCSIData

    JOYINFOEX rgji[4];  // joystick state
    JOYCAPS   rgjc[4];  // joystick info

    PDI  rgpdi[10];     // pointers to DISKINFO structures for disks

    char szDefaultDir[262]; // default directory
    char szWindowsDir[262]; // Windows system directory

    char rgbwrite[1024/8]; // dirty flags for each page of VRAM

    SYSTEM_INFO si;     // CPU/system info

    // debug stuff

    ULONG *pHistory;    // pointer to history buffer or NULL
    ULONG iHistory;     // current index into history buffer
    ULONG *pProfile;    // pointer to profile array or NULL
    ULONG cFreq;        // count of frequent instructions

    ULONG countR;       // count of read exceptions
    ULONG countW;       // count of write exceptions
    ULONG countVidR;    // count of video read exceptions
    ULONG countVidW;    // count of video write exceptions
    ULONG countVMRAMR;  // count of VM reads from RAM
    ULONG countVMRAMW;  // count of VM writes to RAM
    ULONG countVMROMR;  // count of VM reads from ROM
    ULONG countVMROMW;  // count of VM writes to ROM
    ULONG countVMVidR;  // count of VM reads from video
    ULONG countVMVidW;  // count of VM writes to video
    ULONG countVMHWR;   // count of VM reads from hardware
    ULONG countVMHWW;   // count of VM writes to hardware

    HINSTANCE hInst;    // current instance
} INST;

extern INST vi;


//
// NT support
//

typedef struct  _GENPORT_WRITE_INPUT {
    ULONG   PortNumber;     // Port # to write to
    union   {               // Data to be output to port
        ULONG   LongData;
        USHORT  ShortData;
        UCHAR   CharData;
    };
}   GENPORT_WRITE_INPUT;


#define IOCTL_GPD_READ_PORT_UCHAR \
    CTL_CODE( GPD_TYPE, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_READ_PORT_ULONG \
    CTL_CODE( GPD_TYPE, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS )

#define IOCTL_GPD_WRITE_PORT_UCHAR \
    CTL_CODE(GPD_TYPE,  0x910, METHOD_BUFFERED, FILE_WRITE_ACCESS)


// enables DEBUG output
extern BOOL fDebug;

// #define vpregs vi.pregs
#define vpregs   ((REGS *)(&vregs))
#define vpregs2 ((REGS2 *)(&vi.regs2))

// sthw.asm
BOOL PrinterStatus(void);
ULONG KeyFlags(void);
BOOL FKeyIn(void);
ULONG KeyIn(void);
BOOL __cdecl F486(void);
void ScreenRefresh(void);
void __cdecl InitSTHW(void);
void AddToPacket(ULONG);
BOOL FReadSector(void *, ULONG, ULONG, ULONG);
BOOL FWriteSector(void *, ULONG, ULONG, ULONG);
void EnterMode(BOOL);
void SetPalReg(ULONG,ULONG);
BOOL FPollMouse(void);
BOOL FPollMouseRel(void);
void InitSound();
void UpdateSound();
void UnInitSound();


// 68kcpu.c

BOOL __cdecl m68k_DumpRegs(void);
#ifdef POWERMAC
BOOL __cdecl mppc_DumpRegs(void);
#endif
void m68k_DumpHistory(int c);
void InvalidatePages(ULONG eaMin, ULONG eaLim);


// 68k.c

BOOL FInit68K(void);
BOOL FReset68K(void);
BOOL FExec68K(void);
BOOL FDraw68K(void);
BOOL FTimer68K(void);
BOOL FTerm68K(void);
void SaveSettings();

void DumpAssert(int);
void FirstTime(void);
BOOL FResizeRAM(void);
void AutoExec(void);
void Mon(void);
void ScanROMs(void);
void PatchTOS(int, ULONG);
void DumpRegs(void);
char *PchSkipSpace(char *);
char *PchGetWord(char *, ULONG *);
void ClearMemory(void);
void ClearProfile(void);
void DumpProfile(void);
BOOL FFastReadTrackSideSec(BOOL, BYTE *, int, int, int, int, int);
BOOL FRWVirtualDisk(BOOL, ULONG, ULONG, ULONG);
void ReadBootSect(BYTE *, ULONG);
void WriteBootSect(BYTE *, ULONG, ULONG);

// fastvdi.c

BOOL FastRectfl(ULONG, ULONG, ULONG);
BOOL FastPline(ULONG, ULONG, ULONG);
BOOL FastGtext(ULONG, ULONG, ULONG);
void FastBconout(void);
void FastVDI(void);


// dis.c
ULONG CDis(ULONG, BOOL);


// ikbd.c

BOOL FResetIKBD(void);
BOOL FKey68K(BOOL, DWORD);
BOOL FMouse68K(HWND, int, int, int, int);
BOOL FWriteIKBD(int);
BOOL FReadIKBD(void);
BOOL FWinMsgST(HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam);
BOOL FMouseMsgST(HWND hWnd, int msg, int flags, int x, int y);


// machw.c


// maclinea.c

int CallMacLineA(void);


// gemdos.c

BOOL FInitGemDosHooks();
BOOL FResetGemDosHooks();
ULONG CallGEMDOS();


// gemul8r.c

//void UpdateMenuCheckmarks();
//BOOL FVerifyMenuOption();
BOOL CreateNewBitmap(void);

BOOL OpenThePath(HWND hWnd, char *psz);
BOOL OpenTheFile(HWND hWnd, char *psz, BOOL fCreate);


// printer.c

BOOL InitPrinter(int);
BOOL UnInitPrinter(void);
BOOL ByteToPrinter(unsigned char);
BOOL FlushToPrinter(void);
BOOL FPrinterReady();


// props.c

BOOL InitProperties(void);
BOOL EditProperties(void);
BOOL LoadProperties(HWND hOwner);
BOOL SaveProperties(HWND hOwner);
BOOL SaveState(BOOL fSave);


// romcard.c

void ListROMs(HWND, ULONG wf);
BOOL FInstallROMs(void);
void CheckSum(HWND);
int  CbReadWholeFile(char *sz, int cb, void *buf);
int  CbReadWholeFileToGuest(char *sz, int cb, ULONG addr, ULONG access_mode);
BOOL FWriteWholeFile(char *sz, int cb, void *buf);
BOOL FWriteWholeFileFromGuest(char *sz, int cb, ULONG addr, ULONG access_mode);

// serial.c

BOOL FInitSerialPort(int iCOM);
int  CchSerialPending(void);
BOOL CchSerialRead(char *rgb, int cchRead);
BOOL FSetBaudRate(int, int, int, int);
void SetRTS(int);
void SetDTR(int);
ULONG GetModemStatus();
BOOL FWriteSerialPort(BYTE b);

// sound.c

//void TestSound(void);
void SoundDoneCallback(LPWAVEHDR, int);
//void UpdateVoice(int iVoice, ULONG new_frequency, BOOL new_distortion, ULONG new_volume);
void InitJoysticks();
void CaptureJoysticks();
void ReleaseJoysticks();
void InitMIDI();
void InitSound();
void UninitSound();

// blitter.c

void DoBlitter(struct _blitregs *);

// yamaha.c

BOOL FWriteYamaha(int, int);
BOOL FReadYamaha(void);

// ddlib.c
BOOL FInitDirectDraw();
BOOL InitDrawing(int *pdx,int *pdy, int *pbpp, HANDLE,BOOL);
void UninitDrawing(BOOL fFinal);
BYTE *LockSurface(int *);
void UnlockSurface();
void ClearSurface();
//BOOL FChangePaletteEntries(BYTE iPalette, int count, RGBQUAD *ppq);
//BOOL FCyclePalette(BOOL fForward);

// xatari.c
void ForceRedraw();

// xkey.c
void ControlKeyUp8();

// xmon.c
void mon();

// misc

BOOL CenterWindow (HWND, HWND);
LRESULT CALLBACK Properties(HWND, UINT, WPARAM, LPARAM);
//LRESULT CALLBACK About  (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Info   (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DisksDlg(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK FirstTimeProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChooseProc(HWND, UINT, WPARAM, LPARAM);
void DisplayStatus();
//BOOL FCreateOurPalette();
BOOL SetBitmapColors();

// counter stuff

ULONG QueryTickCtr();        // updates vi.llTicks
ULONG Getms();               // updates vi.lms
ULONG GetEclock();           // updates vi.lEclk

#if 0

typedef struct _ep
{
        EXCEPTION_RECORD *per;
        CONTEXT *pctx;
} EP, *PEP;

int OurXcptFilter(DWORD code, PEP pep);

#endif

void MarkAllPagesDirty(void);
void MarkAllPagesClean(void);


//
// Our custom printf which maps to the Windows equivalent
//
//int __cdecl printf(const char *format, ...);

#ifndef NDEBUG
#define DbgMessageBox(a,b,c,d) (fDebug ? MessageBox(a,b,c,d) : printf("%s\n", (b)))
#else
#define DbgMessageBox(a,b,c,d) (0)
#endif

#if defined(NDEBUG)
#define sprintf wsprintf
#endif

#if defined(PERFCOUNT)
extern unsigned int cntInstr,  cntCodeMiss;
extern unsigned int cntAccess, cntDataMiss;
#endif


//
// Structure that maintains the Atari ST hardware state
//

typedef struct _sthw
{
    ULONG lAddrMask;    // current address bus mask for 24- or 32-bit addressing

// the BIOS video mode for 16 color modes ($0D = 320x200, $12 = 640x480)

    int     VESAmode;

// current graphics mode settings (this is not in vi. since it must be restorable)

    int   xpix;         // horizontal resolution (ST pixels)
    int   ypix;         // vertical resolution (ST pixels)
    int   planes;       // number of bit planes (ST mode)
    BOOL fMono;         // true if emulating mono (using the mono bitmap)

//    BOOL fColPal;       // color palette is changing
//    ULONG iColPal;      // color palette index (0..767)

// VIDEO chip

    // write only

    union
        {
        ULONG    dbase; // start address of video memory

        struct
            {
            BYTE dbase0; // dbase0 is STE specific
            BYTE dbasel;
            BYTE dbaseh;
            };
        };

    BYTE    scanskip;   // STE specific
    BYTE    horscroll;  // STE specific
    BYTE    pad3;
    BYTE    pad4;

    // read only

    union
        {
        ULONG    vcount; // current address being read

        struct
            {
            BYTE vcountlo;
            BYTE vcountmid;
            BYTE vcounthi;
            };
        };

    // read/write

    int     shiftmd;

    union
        {
        WORD    wcolor[16];  // ST color registers
        BYTE    bColor[32];
        };


// FDC registers
    
    union
        {
        WORD    rgwFDC[5];

        struct
            {
            WORD cmdreg;   // R/W command/status register
            WORD trackreg; // R/W track register
            WORD secreg;   // R/W sector register
            WORD datareg;  // R/W data register
            WORD countreg; // R/W count of sectors register
            };
        };

    int  idiskreg; // index into FDC registers (0..4)
    WORD sidereg;  // R/W side selection (not an FDC register!)
    WORD disksel;  // drive select, 0 = A:, 1 = B:

    union
        {
        BYTE    rgbHDC[12];

        struct
            {
            BYTE fHDC;     // non-zero when in hard disk operation
            BYTE byte0;
            BYTE byte1;
            BYTE byte2;
            BYTE byte3;
            BYTE byte4;
            BYTE byte5;
            BYTE byte6;
            BYTE byte7;
            BYTE byte8;
            BYTE byte9;
            BYTE diskctrl;  // data written to FDC
            };
        };

// _DMAms    LABEL DWORD

    WORD    dmacmd;  // DMA command (write)
    WORD    dmastat; // DMA status  (read)

    union
        {
        ULONG    DMAbasis;

        struct
            {
            BYTE dmalo;
            BYTE dmamid;
            BYTE dmahi;
            };
        };

    BOOL    fMediaChange;


// Yamaha sound chip

    union
        {
        BYTE    rgbYamaha[16];

        struct
            {
            WORD    periodA;
            WORD    periodB;
            WORD    periodC;
            BYTE    pernoise;
            BYTE    rgfSound;
            BYTE    volA;
            BYTE    volB;
            BYTE    volC;
            BYTE    sustainL;
            BYTE    sustainH;
            BYTE    envelope;
            BYTE    portA;
            BYTE    portB;
            };
        };

    // we create these 3 decay registers that go from 15 to 0

    BYTE rgbDecay[3];
    BYTE iRegSelect;     // 0..15 - Yamaha register select


// BLiTter

    union
        {
        BYTE    rgbBlitter[64];
        WORD    rgwBlitter[32];

        struct
            {
            WORD    rgwFill[16];
            WORD    sxinc;
            WORD    syinc;
            ULONG   saddr;
            WORD    endm1;
            WORD    endm2;
            WORD    endm3;
            WORD    dxinc;
            WORD    dyinc;
            ULONG   daddr;
            WORD    xcount;
            WORD    ycount;
            BYTE    hop;
            BYTE    op;
            BYTE    reg1;
            BYTE    skew;
            };
        } blitter;


// STEreo sound chip

    union
        {
        BYTE    rgbSTESound[64];
        WORD    rgwSTESound[32];
        };


// MFP

    union
        {
        BYTE    rgbMFP[32];

        struct
            {
            BYTE gpip;  // 8-bit I/O port
            BYTE aer;   // active edge register
            BYTE ddr;   // data direction register
            BYTE iera;  // interrupt enable A
            BYTE ierb;  // interrupt enable B
            BYTE ipra;  // interrupt pending A
            BYTE iprb;  // interrupt pending B
            BYTE isra;  // in-service bit A
            BYTE isrb;  // in-service bit B
            BYTE imra;  // interrupt mask register A
            BYTE imrb;  // interrupt mask register B
            BYTE vr;    // interrupt vector register
            BYTE tacr;  // timer A control register
            BYTE tbcr;  // timer B control register
            BYTE tcdcr; // timer C/D control register
            BYTE tadr;  // timer A data register
            BYTE tbdr;  // timer B data register
            BYTE tcdr;  // timer C data register
            BYTE tddr;  // timer D data register
            BYTE scr;
            BYTE ucr;
            BYTE rsr;
            BYTE tsr;
            BYTE udr;
            };
        };

// tbcnt is used as a counter to determine the value of _tbdr. In order
// to detect a vertical blank period, TOS looks at timer B. It will first
// decrement in value from about 255 to 1, and then stay at 0 for at least
// 616 reads. So we count back from 1024 and subract 620. If the result is
// negative, we'll read a zero, othersize we divide by 2 and that's read.

    int     tbcnt;

// timer C needs to count down 1 every 2 reads

    int     tccnt;


// ACIA

    BYTE    KeyCtrl;
    BYTE    KeyData;
    BYTE    MIDICtrl;
    BYTE    MIDIData;


// IKBD

    int     mdMouse;        // mouse reading state - off, absolute, relative
    int     mdJoys;         // joystick reading state - 0=manual 1=automatic

    int     xMouse, yMouse; // mouse position when in ABS mode
    int     fLeft, fRight;  // mouse button positions in ABS mode
    int     wJoy1, wJoy2;

    BYTE    mdIKBD;         // IKBD state
    BYTE    cbToIKBD;       // total # bytes being received in command
    BYTE    cbInIKBD;       // total # byets received so far
    BYTE    res3;

    ULONG   lTickSec;       // tick value of last 1 second interrupt
    ULONG   lTickVBI;       // tick value of last VBI
    ULONG   lTick200;       // tick value of last 200 Hz interrupt

    BYTE    rgbIKBD[256];   // IKBD command buffer

// large buffers

    BITMAPINFOHEADER bmiHeader;
    union
        {
        RGBQUAD rgrgb[256];
        //PALETTEENTRY rgpe[256];
        ULONG    lrgb[256];
        };

    BYTE    rgbDiskBuffer[16384];

} STHW;

STHW vsthw;


//
// Structure that maintains the Macintosh hardware state
//

#define SCCA 1
#define SCCB 0

typedef struct _machw
{
    int     drive;          // 1 = internal, 2 = external
    int     posmode;        // positioning mode
    int     posoff;         // positioning offset (multiple of 512)
    int     pbIO;           // pointer to buffer
    int     cbIO;           // number of bytes (multiple of 512)

    ULONG    V1Base;        // base address of VIA1
    ULONG    V2Base;        // base address of VIA2
    ULONG    RVBase;        // base address of RBV
    ULONG    SndBase;       // base address of Mac II sound chip
    ULONG    SCCRBase;      // base address of SCC reads
    ULONG    SCCWBase;      // base address of SCC writes
    ULONG    IWMBase;       // base address of IWM
    ULONG    SCSIRd;        // base address of SCSI reads
    ULONG    SCSIWr;        // base address of SCSI writes

    struct
        {
        BYTE    vBufB;      // VIA data register B
                            // $50F2600 RBV data register B

        BYTE    vBufA;      // VIA data register A
                            // $50F2602 RBV slot interrupts (SI)

        BYTE    vDirB;      // VIA data direction register B
        BYTE    vDirA;      // VIA data direction register A

        union
            {
            struct
                {
                BYTE    vT1CL;
                BYTE    vT1CH;
                };
            WORD vT1Cw;
            };

        union
            {
            struct
                {
                BYTE    vT2CL;
                BYTE    vT2CH;
                };
            WORD vT2Cw;
            };

        BYTE    vSR;        // VIA shift register
        BYTE    vACR;       // VIA auxilary control register
        BYTE    vPCR;       // VIA peripheral register

        BYTE    vIFR;       // VIA interrupt flags
                            // $50F2603 RBV interrupt flags

        BYTE    vIER;       // VIA interrupt enable
                            // $50F2612 RBV interrupt enable

        // write latches for ports and timers

        BYTE    vBufBL;
        BYTE    vBufAL;

        BYTE    vT1LL;
        BYTE    vT1LH;

        BYTE    vT2LL;
        BYTE    vT2LH;

        // RBV specific registers

        BYTE    rMP;        // $50F2610 RBV monitor parameters
        BYTE    rCT;        // $50F2611 RBV chip test
        BYTE    rIFE;       // $50F2613 RBV interrupt flags enable
        } rgvia[2];         // VIA1 is on all Macs, VIA2 on Mac II

    ULONG lLastEClock1;     // timestamp of last Timer 1 set/decrement
    ULONG lLastEClock2;     // timestamp of last Timer 2 set/decrement

    union
        {
        ULONG lRTC;
        struct
            {
            BYTE rgbRTC[4];             // 32-bit real time clock
            BYTE rgbCMOS[256 + 4];      // 256 bytes of parameter RAM, WP, Test
            };
        };

    struct
        {
        BYTE    RR[16];
        BYTE    WR[16];
        } aSCC[2];

    int iRegSCC;            // register pointer is global for A/B

    enum
        {
        BUSFREE,
        ARBITRATION,
        SELECTION,
        RESELECTION,
        COMMAND,
        DATAIN,
        DATAOUT,
        STATUS,
        MESSAGEIN,
        MESSAGEOUT
        } MacSCSIstate;

    BYTE    rgbSCSIWr[8];

    union
        {
        BYTE    rgbSCSIRd[8];

        struct
            {
            BYTE    sCDR;   // SCSI current data (read)
                            // SCSI output data (write)
            BYTE    sICR;   // SCSI initiator command register
            BYTE    sMR;    // SCSI mode register
            BYTE    sTCR;   // SCSI target command register
            BYTE    sCSR;   // SCSI bus status (read)
                            // SCSI select enable register (write)
            BYTE    sBSR;   // SCSI bus and status register (read)
                            // SCSI start DMA send (write)
            BYTE    sIDR;   // SCSI input data register (read)
                            // SCSI start DMA target receive (write)
            BYTE    sRESET; // SCSI reset parity/interrupt
                            // SCSI start DMA initator receive(write)
            };
        };

    union
        {
        BYTE    rgbIWMCtl[8];

        struct
            {
            BYTE    CA0;
            BYTE    CA1;
            BYTE    CA2;
            BYTE    LSTRB;
            BYTE    ENABLE;
            BYTE    SELECT;
            BYTE    Q6;
            BYTE    Q7;
            };
        };

    union
        {
        BYTE    rgbIWMReg[32];

        struct
            {
            BYTE    DIRTN;  // head step direction (read/write)
            BYTE    CSTIN;  // floppy disk in place (read)
            BYTE    STEP;   // step head one track (read/write)
            BYTE    WRTPRT; // write protected (read)
            BYTE    MOTORON;// motor is running (read/write)
            BYTE    TKO;    // head is at track 0 (read)
            BYTE    EJECT;  // (write)
            BYTE    TACH;   // tachometer (read)
            BYTE    RDDATA0;// read data, lower head (read)
            BYTE    RDDATA1;// read data, upper head (read)
            BYTE    foo10;
            BYTE    foo11;
            BYTE    SIDES;  // single or double sided (read)
            BYTE    foo13;
            BYTE    foo14;
            BYTE    DRVIN;  // drive attached (read)
            } macfloppy[2];
        };

    BOOL fDisk1Dirty;
    BOOL fDisk2Dirty;
    BOOL fTimer1Going;
    BOOL fTimer2Going;

    union
        {
        int    rgreg[4];    // 4 32-bit ADB device registers

        struct
            {
            int     reg0;
            int     reg1;
            int     reg2;
            int     reg3;
            };

        // ADB mouse registers

        struct
            {
            int     dx:7;   // mouse X delta
            int     :1;
            int     dy:7;   // mouse Y delta
            int     fUp:1;
            };

        // ADB keyboard registers

        struct
            {
            int     scan2:7;
            int     fUp2:1;
            int     scan1:7;
            int     fUp1:1;
            int     :16;

            int     :32;    // register 1 unused

            int     fLEDNum:1;
            int     fLEDCaps:1;
            int     fLEDScrl:1;
            int     :3;
            int     ScrlLock:1;
            int     NulLock:1;
            int     fCommand:1;
            int     fOption:1;
            int     fShift:1;
            int     fControl:1;
            int     fReset:1;
            int     fCapsLock:1;
            int     fDelete:1;
            int     :17;

            int     :32;    // register 3 unused
            };
        } ADBMouse, ADBKey;

    long ADBMouseX, ADBMouseY;
    BYTE rgbADBCmd[15];
    BYTE bADBCmd;
    long cbADBCmd;
    long ibADBCmd;

    BYTE fADBR;             // device returning data
    BYTE fADBW;             // writing data to device
    BYTE fVIA2;             // TRUE = ok to emulate VIA2 interrupts
    BYTE sVideo;            // NuBus slot that video is in ($9 to $E)

    BYTE mpAddrMt[4096];    // maps each 1M of address space to memory type

    ULONG iSndStart;        // ROM offset to current startup sample
    ULONG cSndStart;        // samples remaining

    BYTE rgbTobyRW[4096];   // patchable Mac II video card ROM

    union
        {
        BYTE rgbASC[4096];  // Mac II sound chip buffer

        struct
            {
            BYTE rgb0[512]; // 512-byte channel 0 buffer
            BYTE rgb1[512]; // 512-byte channel 1 buffer
            BYTE rgb2[512]; // 512-byte channel 2 buffer
            BYTE rgb3[512]; // 512-byte channel 3 buffer
            BYTE bType;     // hardcoded to read $F for ASC
            BYTE bMode;     // 0 = off, 1 = samples, 2 = synth
            BYTE bChan;     // # of channels?
            BYTE bRst;      // ???
            BYTE bStat;     // ???
            BYTE bUnk5;     // ???
            BYTE bVol;      // volume in bits 7..5, lower 5 bits unused?
            BYTE bUnk7;     // ???
            BYTE bUnk8;     // ???
            BYTE bUnk9;     // ???
            BYTE bDir;      // ???
            BYTE bUnkB;     // ???
            BYTE bUnkC;     // ???
            BYTE bUnkD;     // ???
            BYTE bUnkE;     // ???
            BYTE bUnkF;     // ???
            ULONG lDisp0;   // buffer 0 displacement
            ULONG lFreq0;   // buffer 0 frequency
            ULONG lDisp1;   // buffer 1 displacement
            ULONG lFreq1;   // buffer 1 frequency
            ULONG lDisp2;   // buffer 2 displacement
            ULONG lFreq2;   // buffer 2 frequency
            ULONG lDisp3;   // buffer 3 displacement
            ULONG lFreq3;   // buffer 3 frequency
            ULONG lRMask;   // right channel mask
            ULONG lLMask;   // left channel mask
            ULONG lEnd;     // unused after this
            } asc;
        };
} MACHW;

MACHW vmachw;

#define rgbToby vmachw.rgbTobyRW

#ifndef MAKEDLL
extern VMINFO const vmi800;
extern ICpuExec cpi6502;
#else
extern _declspec(dllimport) VMINFO vmi800;
extern _declspec(dllimport) ICpuExec cpi6502;
#endif

#if defined(ATARIST) || defined(SOFTMAC)
#ifndef MAKEDLL
extern VMINFO const vmiST;
extern VMINFO const vmiMac;
extern VMINFO const vmiMacII;
extern VMINFO const vmiMacQdra;
extern VMINFO const vmiMacPowr;
extern VMINFO const vmiMacG3;
extern ICpuExec cpi68K;
#ifdef POWERMAC
extern ICpuExec cpiPPC;
#endif // POWERMAC
#else
extern _declspec(dllimport) VMINFO vmiST;
extern _declspec(dllimport) VMINFO vmiMac;
extern _declspec(dllimport) VMINFO vmiMacII;
extern _declspec(dllimport) VMINFO vmiMacQdra;
extern _declspec(dllimport) VMINFO vmiMacPowr;
extern _declspec(dllimport) VMINFO vmiMacG3;
extern _declspec(dllimport) VMINFO vmiTutor;
extern _declspec(dllimport) ICpuExec cpi68K;
#endif // MAKEDLL
#endif // ATARIST

//
// External thread control messages
//

#define TM_CONTROL             (WM_APP+1)
#define TM_KEYDOWN             (WM_APP+2)
#define TM_KEYUP               (WM_APP+3)
#define TM_JOY0FIRE            (WM_APP+4)
#define TM_JOY0MOVE            (WM_APP+5)
#define TM_TOGGLEROM           (WM_APP+6)
#define TM_TOGGLECOLOR         (WM_APP+7)

ULONGLONG GetCycles();
//ULONGLONG GetJiffies();

#pragma hdrstop

