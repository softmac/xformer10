
/***************************************************************************

    VMDOS.H

    - Virtual machine interface definitions.
    - defines memory accessors to guest physical memory and hardware
    - this is a copy of Gemulator's VM.H, used only for DOS builds

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    04/20/2008  darekm      HRESULT based accessors
    07/13/2007  darekm      updated for Gemulator/SoftMac 9.0 engine
    02/08/2001  darekm      updated for Gemulator/SoftMac 8.1 engine

***************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __VM_INCLUDED__
#define __VM_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

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
    BOOL (__cdecl *pfnDumpRegs)();  // Display the VM's CPU registers as ASCII
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

enum
{
    VM_GET_VERSION,         // version of this VM
    VM_GET_STRING,          // default verbose name for this VM
    VM_GET_CAPS,            // return capabilities of this VM

    VM_INSTALL,             // one time initialization of VM
    VM_UNINSTALL,           // shut down this VM

    VM_ACTIVATE,            // activate VM (i.e. give focus to)
    VM_DEACTIVATE,          // lose focus

    VM_COLDBOOT,            // cold boot the virtual machine
    VM_WARMBOOT,            // warm boot
    VM_EXEC,                // step/execute code

    VM_READ_PHYS_U8,        // read physical address byte
    VM_READ_PHYS_U16,       // read physical address word
    VM_READ_PHYS_U32,       // read physical address dword
    VM_READ_PHYS_U64,       // read physical address qword
    VM_READ_PHYS_U128,      // read physical address oword

    VM_WRITE_PHYS_U8,       // write physical address byte
    VM_WRITE_PHYS_U16,      // write physical address word
    VM_WRITE_PHYS_U32,      // write physical address dword
    VM_WRITE_PHYS_U64,      // write physical address qword
    VM_WRITE_PHYS_U128,     // write physical address oword

    VM_MAP_GUEST_RANGE_TO_HOST, // return host pointer to address range in VM
    VM_LOCK_PHYS_RANGE,     // lock physical range and return host pointer
    VM_UNLOCK_PHYS_RANGE,   // unlock physical range in VM
};

//
// Global pointer to the current VM info structure.
// Initialized to 0 by default. Must be set before using any VM.
//
// NOTE: we don't define any of the PMINFO structures exported by any of
// the VMs here. The UI must manualy "extern" them, so that the linker only
// links in the VMs (and thus CPUs) that are actually referenced.
//

extern VMINFO *vpvm;


// a VM address

#ifdef HWIN32
typedef unsigned long int  ADDR;
#else
typedef unsigned short int  ADDR;
#endif


//
// Wrapper functions to hide the VMINFO tables and also perform some
// simple parameter checking. All functions return a BOOL indicating
// TRUE for success and FALSE for failure, except the Peek* functions
// which return a BYTE or WORD.
//

BOOL __inline FInstallVM()
{
    return vpvm->pfnInstall(0);
}

BOOL __inline FInitVM()
{
    if (vpvm == NULL)
        return FALSE;

    if (vpvm->pfnInit == NULL)
        return FALSE;

    return vpvm->pfnInit(0);
}

BOOL __inline FUnInitVM()
{
    if (vpvm == NULL)
        return TRUE;

    if (vpvm->pfnUnInit == NULL)
        return TRUE;

    return vpvm->pfnUnInit(0);
}

BOOL __inline FInitDisksVM()
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnInitDisks(0);
}

BOOL __inline FUnInitDisksVM()
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnUnInitDisks(0);
}

BOOL __inline FColdbootVM()
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnColdboot(0);
}

BOOL __inline FWarmbootVM()
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnWarmboot(0);
}

BOOL __inline FExecVM(int fStep, int fCont)
{
    return vpvm->pfnExec(fStep, fCont);
}

#ifdef HWIN32

BOOL __inline FMountDiskVM(int i)
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnMountDisk(i);
}

BOOL __inline FUnmountDiskVM(int i)
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnUnmountDisk(i);
}

BOOL __inline FWinMsgVM(HWND hWnd, UINT msg, WPARAM uParam, LPARAM lParam)
{
    if (vpvm == NULL)
        return TRUE;

    return vpvm->pfnWinMsg(hWnd, msg, uParam, lParam);
}

BOOL __cdecl m68k_DumpRegs();

BOOL __inline FDumpRegsVM()
{
    return vpvm->pfnDumpRegs();
}

BOOL __inline FDumpHWVM()
{
    return vpvm->pfnDumpHW(0);
}

//
// REVIEW: Deprecate vmPeek*
//

void __fastcall ZapRange(ULONG addr, ULONG cb);

ULONG __inline vmPeekB(ULONG ea)
{
    BYTE b = 0;

    vpvm->pfnReadHWByte(ea, &b);

    return b;
}

ULONG __inline vmPeekW(ULONG ea)
{
    WORD w = 0;

    vpvm->pfnReadHWWord(ea, &w);

    return w;
}

ULONG __inline vmPeekL(ULONG ea)
{
    ULONG l = 0;

    vpvm->pfnReadHWLong(ea, &l);

    return l;
}

HRESULT __inline ReadPhysicalByte(ULONG ea, BYTE *pb)
{
    return vpvm->pfnReadHWByte(ea, pb);
}

HRESULT __inline ReadPhysicalWord(ULONG ea, WORD *pw)
{
    return vpvm->pfnReadHWWord(ea, pw);
}

HRESULT __inline ReadPhysicalLong(ULONG ea, ULONG *pl)
{
    return vpvm->pfnReadHWLong(ea, pl);
}

HRESULT __inline vmPokeB(ULONG ea, BYTE b)
{
    ZapRange(ea, sizeof(BYTE));
    return vpvm->pfnWriteHWByte(ea, &b);
}

HRESULT __inline vmPokeW(ULONG ea, WORD w)
{
    ZapRange(ea, sizeof(WORD));
    return vpvm->pfnWriteHWWord(ea, &w);
}

HRESULT __inline vmPokeL(ULONG ea, ULONG l)
{
    ZapRange(ea, sizeof(LONG));
    return vpvm->pfnWriteHWLong(ea, &l);
}

HRESULT __inline WritePhysicalByte(ULONG ea, BYTE *pb)
{
    ZapRange(ea, sizeof(BYTE));
    return vpvm->pfnWriteHWByte(ea, pb);
}

HRESULT __inline WritePhysicalWord(ULONG ea, WORD *pw)
{
    ZapRange(ea, sizeof(WORD));
    return vpvm->pfnWriteHWWord(ea, pw);
}

HRESULT __inline WritePhysicalLong(ULONG ea, ULONG *pl)
{
    ZapRange(ea, sizeof(LONG));
    return vpvm->pfnWriteHWLong(ea, pl);
}

ULONG __inline LockMemoryVM(ULONG ea, ULONG cb, void **ppv)
{
    if (vpvm == NULL)
        return TRUE;

    if (vpvm->pfnLockBlock == NULL)
        return FALSE;

    return vpvm->pfnLockBlock(ea, cb, ppv);
}

ULONG __inline UnlockMemoryVM(ULONG ea, ULONG cb)
{
    if (vpvm == NULL)
        return TRUE;

    if (vpvm->pfnUnlockBlock == NULL)
        return FALSE;

    return vpvm->pfnUnlockBlock(ea, cb);
}

__inline BYTE * MapAddressVM(ULONG ea)
{
    return vpvm->pfnMapAddress(ea);
}

__inline BYTE * MapWritableAddressVM(ULONG ea)
{
    return vpvm->pfnMapAddressRW(ea);
}

#endif  // HWIN32


#ifdef __cplusplus
}
#endif

#endif __VM_INCLUDED__

