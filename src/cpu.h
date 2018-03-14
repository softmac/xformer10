
/***************************************************************************

    CPU.H

    - CPU interface definitions.
    - defines memory accessors to guest logical memory

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    04/20/2008  darekm      HRESULT based accessors
    07/13/2007  darekm      updated for Gemulator/SoftMac 9.0 engine
    04/23/2001  darekm      updated for Gemulator/SoftMac 8.1 engine

***************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CPU_INCLUDED__
#define __CPU_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//
// CPU main processor types
//

enum
{
cpuNone    = 0,

cpu6502    = 0x00000001,

cpu68000   = 0x00000002,
cpu68010   = 0x00000004,
cpu68020   = 0x00000008,
cpu68030   = 0x00000010,
cpu68040   = 0x00000020,

cpu601     = 0x00000040,
cpu603     = 0x00000080,
cpu604     = 0x00000100,
cpuPPCG3   = 0x00000200,
cpuPPCG4   = 0x00000400,

cpu386     = 0x00001000,
cpu486     = 0x00002000,
cpu586     = 0x00004000,
cpu686     = 0x00008000,
cpuP4      = 0x00010000,
cpuK7      = 0x00020000,
cpuK8      = 0x00040000,
};

#define cpu68K  (cpu68000 | cpu68010 | cpu68020 | cpu68030 | cpu68040)
#define cpuPPC  (cpu601   | cpu603   | cpu604   | cpuPPCG3 | cpuPPCG4)
#define cpuIA32 (cpu386   | cpu486   | cpu586   | cpu686   | cpuP4   )
#define cpuAMD  (cpuK7    | cpuK8)
#define cpuX86  (cpuIA32  | cpuAMD)

#define FIsCPUAny68K(cpu)   ((cpu) & cpu68K)
#define FIsCPUAnyPPC(cpu)   ((cpu) & cpuPPC)
#define FIsCPUAnyIA32(cpu)  ((cpu) & cpuIA32)
#define FIsCPUAnyAMD(cpu)   ((cpu) & cpuAMD)
#define FIsCPUAnyX86(cpu)   ((cpu) & cpuX86)


//
// CPU capabilities
//

enum
{
CPUCAPS_CAN_DEBUG   = 0x00000001,       // is debugger capable
CPUCAPS_CAN_STEP    = 0x00000002,       // can single step
CPUCAPS_CAN_HOOK    = 0x00000004,       // can hook exceptions
CPUCAPS_CAN_SWAP    = 0x00000008,       // can swap endianness
};


//
// Interface for the CPU execution module
//

typedef struct ICpuExec
{
    PFNL pfnCpu;            // function to handle CPU operations
    PFNL pfnGetCpus;        // returns the bit vector of CPUs supported
    PFNL pfnGetCaps;        // returns capabilities of this CPU module
    PFNL pfnGetVers;        // returns version number of this CPU module
    PFNL pfnGetPageBits;    // returns granularity of page size
    PFNL pfnInit;           // initialization routine
    PFNL pfnReset;          // reset routine
    PFNL pfnGo;             // interpreter entry point for go
    PFNL pfnStep;           // interpreter entry point for step
    ULONG *rgfExcptHook;    // pointer to bit vector of exceptions to hook
    PFNL pfnPeekB;          // read byte
    PFNL pfnPeekW;          // read word
    PFNL pfnPeekL;          // read long
    PFNL pfnPokeB;          // write byte
    PFNL pfnPokeW;          // write word
    PFNL pfnPokeL;          // write long
} ICpuExec, *LPCPUEXEC;

extern ICpuExec *vpci;

enum
{
    CPU_GET_VERSION,        // version of this CPU
    CPU_GET_STRING,         // default verbose name for this CPU
    CPU_GET_CAPS,           // return capabilities of this CPU

    CPU_INSTALL,            // one time initialization of VM
    CPU_UNINSTALL,          // shut down this VM

    CPU_RESET,              // reset CPU state
    CPU_EXEC,               // step/execute code

    CPU_READ_VIRT_U8,       // read logical address byte
    CPU_READ_VIRT_U16,      // read logical address word
    CPU_READ_VIRT_U32,      // read logical address dword
    CPU_READ_VIRT_U64,      // read logical address qword
    CPU_READ_VIRT_U128,     // read logical address oword
    CPU_READ_VIRT_U256,     // read logical address vword
    CPU_READ_VIRT_U512,     // read logical address cache line

    CPU_WRITE_VIRT_U8,      // write logical address byte
    CPU_WRITE_VIRT_U16,     // write logical address word
    CPU_WRITE_VIRT_U32,     // write logical address dword
    CPU_WRITE_VIRT_U64,     // write logical address qword
    CPU_WRITE_VIRT_U128,    // write logical address oword
    CPU_WRITE_VIRT_U256,    // write logical address vword
    CPU_WRITE_VIRT_U512,    // write logical address cache line
};


// These macros allow direct access to guest CPU linear virtual memory space
// at the current CPU mode's priviledge level.
// The return value is the zero-extended read data (for a read),
// and the high bit is set to indicate that the guest read/write failed.

HRESULT __fastcall ReadGuestLogicalByte(ULONG ea, BYTE *pb);
HRESULT __fastcall ReadGuestLogicalWord(ULONG ea, WORD *pw);
HRESULT __fastcall ReadGuestLogicalLong(ULONG ea, DWORD *pl);

HRESULT __fastcall WriteGuestLogicalByte(ULONG ea, BYTE *pb);
HRESULT __fastcall WriteGuestLogicalWord(ULONG ea, WORD *pw);
HRESULT __fastcall WriteGuestLogicalLong(ULONG ea, DWORD *pl);

// Peek variants ignore error return values and simply treat the return value
// as a value read.

__forceinline
ULONG PeekB(ULONG addr)
{
    BYTE b = 0;

    ReadGuestLogicalByte(addr, &b);
    return b;
}

__forceinline
ULONG PeekW(ULONG addr)
{
    WORD w = 0;

    ReadGuestLogicalWord(addr, &w);
    return w;
}

__forceinline
ULONG PeekL(ULONG addr)
{
    DWORD l = 0;

    ReadGuestLogicalLong(addr, &l);
    return l;
}

// Poke variants return a non-zero flag to indicate failure

__forceinline
HRESULT PokeB(ULONG addr, BYTE b)
{
    return WriteGuestLogicalByte(addr, &b);
}

__forceinline
HRESULT PokeW(ULONG addr, WORD w)
{
    return WriteGuestLogicalWord(addr, &w);
}

__forceinline
HRESULT PokeL(ULONG addr, ULONG l)
{
    return WriteGuestLogicalLong(addr, &l);
}


#ifdef __cplusplus
}
#endif

#endif __CPU_INCLUDED__

