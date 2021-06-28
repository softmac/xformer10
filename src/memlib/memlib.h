
/***************************************************************************

    MEMLIB.H

    - Memory management layer

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part Xformer project. It is subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

    08/20/2001  darekm      last update

***************************************************************************/

//
// Memory Mapping Descriptor
//
// Describes a particular range of memory in a VM
// Used to map logical address space to physical
// and to map physical address space to RAM and hardware
//

typedef struct MMD
{
    union
        {
        ULONG   lFlags;

        struct
            {
            int     rgfSizes:4;     // allowed size of access B W L
            int     rgfAccess:4;    // allowed type of flags  R W X HW
            int     fRAMA:1;        // maps to first bank of RAM space
            int     fRAMB:1;        // maps to second bank of RAM space
            int     fROM:1;         // maps to ROM space
            int     fVRAM:1;        // maps to VRAM space
            int     fAlias:1;       // alias to another range of descriptors
            int     fRepeat:1;      // repeat previous descriptor cnt times
            int     size:2;         // 0 = bits, 1 = bytes, 2 = pages, 3 = meg

            int     cnt:16;         // block length, or 0 for special/end
            };

        struct
            {
            // access size flags

            int     fIsByte:1;      // allow byte access
            int     fIsWord:1;      // allow word access
            int     fIsLong:1;      // allow long/block/DMA/MOVE16/float access
            int     fIsLine:1;      // allow large memory access (cache, MOVE16, etc)

            // access type flags

            int     fCanRead:1;     // allow reads
            int     fCanWrite:1;    // allow writes
            int     fCanExec:1;     // allow execution of code
            int     fIsHW:1;        // this is hardware, must have handler
            };
        };

    union
        {
        void   *pmmd;               // pointer to aliased range of descriptors
        void   *pfn;                // pointer to hardware handler
        };
} MMD;

enum
{
MEM_SIZ_B = 1,
MEM_SIZ_W = 2,
MEM_SIZ_L = 4,
MEM_SIZ_X = 8,
MEM_SIZ_ALL = 15,
};

enum
{
MEM_TYP_R = 1,
MEM_TYP_W = 2,
MEM_TYP_RW = 3,
MEM_TYP_X = 4,
MEM_TYP_RX = 5,
MEM_TYP_RWX = 7,
MEM_TYP_H = 8,
};

enum
{
MEM_CNT_BIT,
MEM_CNT_BYT,
MEM_CNT_PAG,
MEM_CNT_MEG
};

#define MEM_BYTE    (1)
#define MEM_WORD    (2)
#define MEM_LONG    (4)
#define MEM_DMA     (8)
#define MEM_ALL     (MEM_BYTE | MEM_WORD | MEM_LONG | MEM_DMA)

#define MEM_READ    (16)
#define MEM_WRITE   (32)
#define MEM_READ_WRITE   (48)
#define MEM_EXEC    (64)
#define MEM_NORMAL  (MEM_READ | MEM_WRITE | MEM_EXEC)
#define MEM_HWONLY  (128)

#define MEM_RAMA    (256)
#define MEM_RAMB    (512)
#define MEM_ROM     (1024)
#define MEM_VRAM    (2048)

#define MEM_ALIAS   (4096)

#define MEM_CNT(x)  ((x) << 16)

#define MEM_REP(x)  (8192  | MEM_CNT(x))

#define MEM_BITS(x) (0     | MEM_CNT(x))
#define MEM_BYTES(x) (16384 | MEM_CNT(x))
#define MEM_PAGES(x) (32768 | MEM_CNT(x))
#define MEM_MEGS(x) (49152 | MEM_CNT(x))

#define MEM_END     (0)

// typical usage:
// MMD mmd = { MEM_ALIAS | MEM_NORMAL | MEM_ALL | MEM_BYTES(4096), &mmd1 };


// Allocates the RAM and ROM for a virtual machine.
// Uses the vi.cbRAM and vi.cbROM arrays as input, outputs vi.pbRAM vi.pbROM.

BOOL memAllocAddressSpace(BOOL fBigEnd);

BOOL memFreeAddressSpace();

BOOL memInitAddressSpace();


//
// Guest address space access modes
//
// Flags can be combined
//

#define ACCESS_DEFAULT  (0)     // default mode (user or kernel)
#define ACCESS_USER     (1)
#define ACCESS_SUPER    (2)
#define ACCESS_CODE     (4)     // if not set, implies data access
#define ACCESS_INIT     (8)     // write to ROM to initialize it (acts as read)
#define ACCESS_READ     (16)    // read access
#define ACCESS_WRITE    (32)    // write access
#define ACCESS_RMW      (64)    // read for write access (acts as a write)
#define ACCESS_PHYSICAL (128)
#define ACCESS_BYTESWAP (256)   // reverse the data on the fly

#define ACCESS_INITHW           (ACCESS_PHYSICAL | ACCESS_INIT)
#define ACCESS_CODEREAD         (ACCESS_CODE     | ACCESS_READ)
#define ACCESS_PHYSICAL_READ    (ACCESS_PHYSICAL | ACCESS_READ)
#define ACCESS_PHYSICAL_WRITE   (ACCESS_PHYSICAL | ACCESS_WRITE)

//
// Helpers for manipulating guest space. Uses the above ACCESS_* flags.
// Returns TRUE is data access succeeded. FALSE means bus error / page fault.
//

#define QUAD    __int64

BOOL GuestDataAccessByte(ULONG addr, BYTE *pb, ULONG mode);
BOOL GuestDataAccessWord(ULONG addr, WORD *pw, ULONG mode);
BOOL GuestDataAccessLong(ULONG addr, LONG *pl, ULONG mode);
BOOL GuestDataAccessQuad(ULONG addr, QUAD *pq, ULONG mode);

// General purpose block copy to guest address space

BOOL HostToGuestDataCopy(ULONG addr, BYTE *pb, ULONG mode, ULONG cb);

// General purpose block copy from guest address space

BOOL GuestToHostDataCopy(ULONG addr, BYTE *pb, ULONG mode, ULONG cb);

