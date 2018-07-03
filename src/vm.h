
/***************************************************************************

    VM.H

    - Virtual machine interface definitions.
    - defines memory accessors to guest physical memory and hardware

    Copyright (C) 1991-2018 by Darek Mihocka. All Rights Reserved.
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

// !!! unused?
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
//
//extern VMINFO *vpvm;

// a VM address

typedef unsigned long int  ADDR;

//
// Wrapper functions to hide the VMINFO tables and also perform some
// simple parameter checking. All functions return a BOOL indicating
// TRUE for success and FALSE for failure, except the Peek* functions
// which return a BYTE or WORD.
//

// some functions can operate on any instance, and they take a parameter (which instance)
// and need to look at the state of that instance

BOOL __inline FInstallVM(void **pPrivateX, int *iPrivateSizeX, int iVM, PVMINFO pvmi, int type)
{
    return rgvm[iVM].pvmi->pfnInstall(pPrivateX, iPrivateSizeX, iVM, pvmi, type);
}

BOOL __inline FUnInstallVM(int iVM)
{
	return rgvm[iVM].pvmi->pfnUnInstall(vrgvmi[iVM].pPrivate);
}

BOOL __inline FInitVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnInit(vrgvmi[iVM].pPrivate);
}

BOOL __inline FUnInitVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnUnInit(vrgvmi[iVM].pPrivate);
}

BOOL __inline FInitDisksVM(int iVM)
{
	return rgvm[iVM].pvmi->pfnInitDisks(vrgvmi[iVM].pPrivate);
}

BOOL __inline FUnInitDisksVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnUnInitDisks(vrgvmi[iVM].pPrivate);
}

BOOL __inline FColdbootVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnColdboot(vrgvmi[iVM].pPrivate);
}

BOOL __inline FWarmbootVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnWarmboot(vrgvmi[iVM].pPrivate);
}

BOOL __inline FExecVM(int iVM, BOOL fStep, BOOL fCont)
{
	return rgvm[iVM].pvmi->pfnExec(vrgvmi[iVM].pPrivate, fStep, fCont);
}

BOOL __inline FSaveStateVM(int iVM)
{
	return rgvm[iVM].pvmi->pfnSaveState(vrgvmi[iVM].pPrivate);
}

BOOL __inline FLoadStateVM(int iVM, void *pNew)
{
	return rgvm[iVM].pvmi->pfnLoadState(pNew, vrgvmi[iVM].pPrivate, vrgvmi[iVM].iPrivateSize);
}

BOOL __inline FWriteProtectDiskVM(int iVM, int i, BOOL fSet, BOOL fWP)
{
    return rgvm[iVM].pvmi->pfnWriteProtectDisk(vrgvmi[iVM].pPrivate, i, fSet, fWP);
}

BOOL __inline FMountDiskVM(int iVM, int i)
{
    return rgvm[iVM].pvmi->pfnMountDisk(vrgvmi[iVM].pPrivate, i);
}

BOOL __inline FUnmountDiskVM(int iVM, int i)
{
    return rgvm[iVM].pvmi->pfnUnmountDisk(vrgvmi[iVM].pPrivate, i);
}

BOOL __inline FWinMsgVM(int iVM, HWND hWnd, UINT msg, WPARAM uParam, LPARAM lParam)
{
    return rgvm[iVM].pvmi->pfnWinMsg(vrgvmi[iVM].pPrivate, hWnd, msg, uParam, lParam);
}

BOOL __cdecl m68k_DumpRegs(int iVM);

BOOL __inline FDumpRegsVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnDumpRegs(vrgvmi[iVM].pPrivate);
}

BOOL __inline FDumpHWVM(int iVM)
{
    return rgvm[iVM].pvmi->pfnDumpHW(vrgvmi[iVM].pPrivate);
}

BOOL __inline FMonVM(int iVM)
{
	return rgvm[iVM].pvmi->pfnMon(vrgvmi[iVM].pPrivate);
}

//
// These do not apply to ATARI 800
//

void __fastcall ZapRange(ULONG addr, ULONG cb);

BYTE __inline vmPeekB(int iVM, ULONG ea)
{
    BYTE b = 0;

	rgvm[iVM].pvmi->pfnReadHWByte(vrgvmi[iVM].pPrivate, ea, &b);

    return b;
}

WORD __inline vmPeekW(int iVM, ULONG ea)
{
    WORD w = 0;

    rgvm[iVM].pvmi->pfnReadHWWord(vrgvmi[iVM].pPrivate, ea, &w);

    return w;
}

ULONG __inline vmPeekL(int iVM, ULONG ea)
{
    ULONG l = 0;

    rgvm[iVM].pvmi->pfnReadHWLong(vrgvmi[iVM].pPrivate, ea, &l);

    return l;
}

BOOL __inline vmPokeB(int iVM, ULONG ea, BYTE b)
{
	ZapRange(ea, sizeof(BYTE));
	return rgvm[iVM].pvmi->pfnWriteHWByte(vrgvmi[iVM].pPrivate, ea, &b);
}

BOOL __inline vmPokeW(int iVM, ULONG ea, WORD w)
{
	ZapRange(ea, sizeof(WORD));
	return rgvm[iVM].pvmi->pfnWriteHWWord(vrgvmi[iVM].pPrivate, ea, &w);
}

BOOL __inline vmPokeL(int iVM, ULONG ea, ULONG l)
{
	ZapRange(ea, sizeof(LONG));
	return rgvm[iVM].pvmi->pfnWriteHWLong(vrgvmi[iVM].pPrivate, ea, &l);
}

// !!! So what does pfnReadHW* actually return?

HRESULT __inline ReadPhysicalByte(int iVM, ULONG ea, BYTE *pb)
{
    return rgvm[iVM].pvmi->pfnReadHWByte(vrgvmi[iVM].pPrivate, ea, pb);
}

HRESULT __inline ReadPhysicalWord(int iVM, ULONG ea, WORD *pw)
{
    return rgvm[iVM].pvmi->pfnReadHWWord(vrgvmi[iVM].pPrivate, ea, pw);
}

HRESULT __inline ReadPhysicalLong(int iVM, ULONG ea, ULONG *pl)
{
    return rgvm[iVM].pvmi->pfnReadHWLong(vrgvmi[iVM].pPrivate, ea, pl);
}

// !!! Huh? A minute ago pfnWriteHW* returned a BOOL

HRESULT __inline WritePhysicalByte(int iVM, ULONG ea, BYTE *pb)
{
    ZapRange(ea, sizeof(BYTE));
    return rgvm[iVM].pvmi->pfnWriteHWByte(vrgvmi[iVM].pPrivate, ea, pb);
}

HRESULT __inline WritePhysicalWord(int iVM, ULONG ea, WORD *pw)
{
    ZapRange(ea, sizeof(WORD));
    return rgvm[iVM].pvmi->pfnWriteHWWord(vrgvmi[iVM].pPrivate, ea, pw);
}

HRESULT __inline WritePhysicalLong(int iVM, ULONG ea, ULONG *pl)
{
    ZapRange(ea, sizeof(LONG));
    return rgvm[iVM].pvmi->pfnWriteHWLong(vrgvmi[iVM].pPrivate, ea, pl);
}

ULONG __inline LockMemoryVM(int iVM, ULONG ea, ULONG cb, void **ppv)
{
    if (rgvm[iVM].pvmi == NULL)
        return TRUE;

    if (rgvm[iVM].pvmi->pfnLockBlock == NULL)
        return FALSE;

    return rgvm[iVM].pvmi->pfnLockBlock(vrgvmi[iVM].pPrivate, ea, cb, ppv);
}

ULONG __inline UnlockMemoryVM(int iVM, ULONG ea, ULONG cb)
{
    if (rgvm[iVM].pvmi == NULL)
        return TRUE;

    if (rgvm[iVM].pvmi->pfnUnlockBlock == NULL)
        return FALSE;

    return rgvm[iVM].pvmi->pfnUnlockBlock(vrgvmi[iVM].pPrivate, ea, cb);
}

__inline BYTE * MapAddressVM(int iVM, ULONG ea)
{
    return rgvm[iVM].pvmi->pfnMapAddress(vrgvmi[iVM].pPrivate, ea);
}

__inline BYTE * MapWritableAddressVM(int iVM, ULONG ea)
{
    return rgvm[iVM].pvmi->pfnMapAddressRW(vrgvmi[iVM].pPrivate, ea);
}

#ifdef __cplusplus
}
#endif

#endif // __VM_INCLUDED__