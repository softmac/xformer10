
/****************************************************************************

    BLOCKDEV.H

    Public API for generic block device sector I/O code.
    Supports floppy drives, disk images, and SCSI devices.
    Supports Mac HFS and GEMDOS partitions.

    Copyright (C) 1998-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    01/18/1998  darekm

****************************************************************************/

#pragma once

//
// Types of disks that can be accessed
//

enum disktype
{
    DISK_NONE,
    DISK_FLOPPY,
    DISK_SCSI,
    DISK_IMAGE,
    DISK_WIN32,
};


//
// Types of file systems that are understood
// 

enum fstype
{
    FS_RAW,                     // unknown or just raw sector i/o
    FS_ATARIDOS,                // Atari 8-bit DOS disk
    FS_MYDOS,                   // Atari 8-bit MyDOS disk
    FS_FAT12,                   // 12-bit FAT on floppy
    FS_FAT16,                   // 16-bit FAT on hard disk
    FS_FAT32,                   // 32-bit FAT on hard disk
    FS_FATBGM,                  // Atari ST Big Disk format
    FS_MFS,                     // Macintosh 400K MFS
    FS_HFS,                     // Macintosh 800K+ HFS
    FS_HFSPLUS,                 // Macintosh HFS+
};

#pragma pack(push, 1)

#define CACHESLOTS 16

typedef struct _pdi
{
    // public

    int  sec;                   // current sector #
    int  side;                  // current size (or 0 if n/a)
    int  track;                 // current track (or 0 if n/a)
    int  count;                 // # of sectors to read/write
    BYTE *lpBuf;                // buffer for I/O

    // private

    BOOL fNT;                   // 0 = Windows 9x, 1 = NT
    enum disktype dt;           // disk type
    enum fstype  fst;           // file system type
    BOOL fWP;                   // write protected
    BOOL fEjected;              // media is currently ejected
    int  offsec;                // offset sector (sectors to skip from start)
    int  cbHdr;                 // for image files, the header size to skip over
    int  size;                  // size in K, or 0 if unknown or don't care
    int  cbSector;              // sector size in bytes
    char szImage[MAX_PATH];     // disk image path
    char szVolume[MAX_PATH];    // disk volume name
    int  ctl;                   // SCSI controller
    int  id;                    // SCSI ID or floppy number (0 = A:, 1 = B:)
    HANDLE h;                   // handle to image or device handle
    char szCwd[MAX_PATH];       // current working directory on the disk
    long dirID;                 // directory ID of the current directory
    WIN32_FIND_DATA *pfd;       // array of cached directory entries
    int  cfd;                   // size of the directory entry array

    struct _pdi *pdiStub;       // PDI to a stub file
    int  cSecStub;              // size of stub file

    struct
        {
        char *pb_dc;            // cached sector data
        int  sec_dc;
        } dc[CACHESLOTS+1];     // extra slot for adding new entry
} DISKINFO, *PDI;

#pragma pack(pop)

// Open flags

#define DI_QUERY       1
#define DI_READONLY    2
#define DI_CREATE      4


// The API prototypes

BOOL __stdcall FInitBlockDevLib(void);
DISKINFO * __stdcall PdiOpenDisk(enum disktype dt, long l, long flags);
BOOL __stdcall FRawDiskRWPdi(DISKINFO *pdi, BOOL fWrite);
BOOL __stdcall FlushCachePdi(DISKINFO *pdi);
BOOL __stdcall CloseDiskPdi(DISKINFO *pdi);
int __stdcall CntReadDiskDirectory(PDI pdi, char *szDir, WIN32_FIND_DATA **);
enum fstype __stdcall FstIdentifyFileSystem(DISKINFO *pdi);

void __stdcall CreateHFSBootBlocks(BYTE *pb, ULONG l, char *szVol);

#define LongFromScsi(ctl,trg) (((ctl) << 4) | (trg))

BOOL __stdcall scsiTestUnitReady(int adapter_id, int target_id, char *pbBuf, int cb);
BOOL __stdcall scsiInquiry(int adapter_id, int target_id, char *pbBuf, int cb);
BOOL __stdcall scsiReadSector(int adapter, int target_id, int sec, int count, char *pbBuf, int cbSec);
BOOL __stdcall scsiWriteSector(int adapter, int target_id, int sec, int count, char *pbBuf, int cbSec);
BOOL __stdcall scsiGetDiskInfo(int adapter_id, int target_id, char *pbBuf, int cb);

BOOL __stdcall scsiModeSense(int adapter_id, int target_id, char *pbBuf, int cb, int page);
BOOL __stdcall scsiResetDevice(int adapter_id, int target_id);
BOOL __stdcall scsiReadCapacity(int adapter_id, int target_id, char *pbBuf, int cb);
BOOL __stdcall scsiReadTOC(int adapter_id, int target_id, char *pbBuf, int cb);

BOOL __stdcall CreateHFXBootBlocks(BYTE *pb, ULONG l, char *szVol);

extern WORD NumAdapters;

typedef void *LPSRB;     // also defined in wnaspi32.h
typedef DWORD (__cdecl * WinAspiCommand)(LPSRB);
extern FARPROC pfnGetASPI32SupportInfo;
extern WinAspiCommand pfnSendASPI32Command;

