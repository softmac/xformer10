
/****************************************************************************

    SECTORIO.C

    Generic block device sector I/O code.
    Supports floppy drives, disk images, and SCSI devices.

    Copyright (C) 1998-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    10/10/2001  darekm

****************************************************************************/

#include "precomp.h"
#include "blockdev.h"

#define VOLATILE volatile


WORD    NumAdapters;
FARPROC pfnGetASPI32SupportInfo;
WinAspiCommand pfnSendASPI32Command;


//
// NT disk sector I/O code
//

#include <winioctl.h>


BOOL GetDiskGeometry(HANDLE hDisk, PDISK_GEOMETRY lpGeometry)
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                lpGeometry,
                sizeof(*lpGeometry),
                &ReturnedByteCount,
                NULL
                );
}

BOOL LockVolume(HANDLE hDisk)
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                FSCTL_LOCK_VOLUME,
                NULL,
                0,
                NULL,
                0,
                &ReturnedByteCount,
                NULL
                );
}

BOOL UnlockVolume(HANDLE hDisk)
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                FSCTL_UNLOCK_VOLUME,
                NULL,
                0,
                NULL,
                0,
                &ReturnedByteCount,
                NULL
                );
}

BOOL DismountVolume(HANDLE hDisk)
{
    DWORD ReturnedByteCount;

    return DeviceIoControl(
                hDisk,
                FSCTL_DISMOUNT_VOLUME,
                NULL,
                0,
                NULL,
                0,
                &ReturnedByteCount,
                NULL
                );
}

BOOL FReadWriteSecNT(DISKINFO *pdi, BOOL fWrite)
{
    int retry = 3;
    //int newcount = pdi->count;

    Assert(pdi->count >= 1);

    if (pdi->count == 0)
        return FALSE;

    while (retry--)
        {
        static DISK_GEOMETRY Geometry;

        //BOOL f;

        LPVOID IoBuffer;
        BOOL b;
		DWORD BytesRead;
		//DWORD BytesWritten;
        //DWORD FileSize;
        DWORD VirtBufSize;
        //DWORD NumBufs;
        DWORD emOld;

#if TRACEDISK
        printf("Trying to %s %d sectors\n", fWrite ? "write":"read", newcount);
#endif

        if (pdi->h == INVALID_HANDLE_VALUE)
            {
            char sz[8];

            strcpy(sz, "\\\\.\\A:");
            sz[4] = 'A' + (char)pdi->id;

            // Open and Lock the drive

            emOld = SetErrorMode(SEM_FAILCRITICALERRORS);

            pdi->h = CreateFile(sz,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

            SetErrorMode(emOld);
            }

        if (LockVolume(pdi->h) == FALSE)
            goto Lfail;

        if (!GetDiskGeometry(pdi->h, &Geometry))
            goto Lfail;

        VirtBufSize = pdi->count << 9;
        IoBuffer = VirtualAlloc(NULL,VirtBufSize,MEM_COMMIT,PAGE_READWRITE);

        if (Geometry.MediaType == F3_720_512)
            {
            pdi->size = 720;
            SetFilePointer(pdi->h, (pdi->track * 18 + pdi->side * 9  + pdi->sec + pdi->offsec) << 9, NULL, 0);
            }
        else // if (Geometry.MediaType == F3_1Pt44_512)
            {
            pdi->size = 1440;
            SetFilePointer(pdi->h, (pdi->track * 36 + pdi->side * 18 + pdi->sec + pdi->offsec) << 9, NULL, 0);
            }

        emOld = SetErrorMode(SEM_FAILCRITICALERRORS);

        if (fWrite)
            {
            memcpy(IoBuffer, pdi->lpBuf, VirtBufSize);
            b = WriteFile(pdi->h, IoBuffer, VirtBufSize, &BytesRead, NULL);
            }
        else
            {
            b = ReadFile(pdi->h, IoBuffer, VirtBufSize, &BytesRead, NULL);
            memcpy(pdi->lpBuf, IoBuffer, VirtBufSize);
            }

        SetErrorMode(emOld);

        VirtualFree(IoBuffer, VirtBufSize, MEM_DECOMMIT);
        VirtualFree(IoBuffer, 0, MEM_RELEASE);

//      DismountVolume(pdi->h);
        UnlockVolume(pdi->h);

        if (b && BytesRead)
            {
#if TRACEDISK
            printf("%s succeeded, ByteCount = %d\n",
                fWrite ? "Write" : "Read", BytesRead);
#endif

            pdi->count = 0;

            if (fWrite)
                return 1;

#if TRACEDISK
        {
        BYTE *buf = pdi->lpBuf;

         printf("buf[0..7] = %02X %02X %02X %02X %02X %02X %02X %02X\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]); 
        }
#endif

            return 1;
            }
        }

Lfail:
#if TRACEDISK
    printf("FNTFastReadTrack failed\n");
#endif

    CloseHandle(pdi->h);
    pdi->h = INVALID_HANDLE_VALUE;

    pdi->count = 0;
    return 0;
}


//
// WIN95 disk sector I/O code
//

typedef struct DIOCRegs
    {
    DWORD   reg_EBX;
    DWORD   reg_EDX;
    DWORD   reg_ECX;
    DWORD   reg_EAX;
    DWORD   reg_EDI;
    DWORD   reg_ESI;
    DWORD   reg_Flags;
    } DIOC_REGISTERS;


BOOL FReadWriteSec9x(DISKINFO *pdi, BOOL fWrite)
{
    int retry = 3;
    int newcount = pdi->count;

    if (pdi->fNT)
        return FReadWriteSecNT(pdi, fWrite);

    Assert(pdi->count >= 1);

    if (pdi->count == 0)
        return FALSE;

    while (retry--)
        {
        HANDLE h;
        DIOC_REGISTERS regs;
        int ReturnedByteCount;
        BOOL f;

#if TRACEDISK
        printf("Trying to %s %d sectors\n", fWrite ? "write":"read", newcount);
    printf("fWrite = %d, drive = %d, buf = %08X, track = %d, side = %d, sec = %d, count = %d\n",
      fWrite, pdi->id, pdi->lpBuf, pdi->track, pdi->side, pdi->offsec+pdi->sec+1, newcount);
#endif
        // Open and Lock the drive

        h = CreateFile("\\\\.\\VWIN32",
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

        regs.reg_EBX = (long)pdi->lpBuf;    // buffer
        regs.reg_EAX = (fWrite ? 0x0300 : 0x0200) | newcount;   /* read, count */
        regs.reg_ECX = (pdi->track<<8)|((pdi->track>>2)&192)|((pdi->sec+pdi->offsec+1)&63);
        regs.reg_EDX = (pdi->side << 8) | pdi->id;                     /* side, drive */

        f = DeviceIoControl(
                    h,
                    4, // VWIN32_DIOC_DOS_INT13
                    &regs,
                    sizeof(regs),
                    &regs,
                    sizeof(regs),
                    (LPDWORD)&ReturnedByteCount,
                    NULL
                    );

        CloseHandle(h);

        if (f && (regs.reg_EAX & 255))
            {
#if TRACEDISK
            printf("%s succeeded\n", fWrite ? "Write" : "Read");
#endif
            pdi->count = 0;

            if (fWrite)
                return 1;

#if TRACEDISK
        {
        BYTE *buf = pdi->lpBuf;
         printf("buf[0..7] = %02X %02X %02X %02X %02X %02X %02X %02X\n",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]); 
        }
#endif
            return 1;
            }
        }

#if TRACEDISK
    printf("FFastReadTrack failed\n");
#endif
    pdi->count = 0;
    return 0;
}


//
// Disk image sector I/O code
//

BOOL FReadWriteImage(DISKINFO *pdi, BOOL fWrite)
{
    // Reading or writing a sector from virtual disk.
    // Sector numbers are logical 512-byte sectors.

    BOOL f;
    ULONG cb, cb2 = 0;
    ULONG i;
    ULONG ofsFile;
    unsigned char *pb;
    unsigned char rgb[2048];    // size of a CD-ROM sector

    Assert(pdi->count >= 1);

#if TRACEDISK
    printf("ErrRWImage: iDrive = %d, fWrite = %d, buf = %08X, sec = %08X, count = %d\n",
          pdi->id, fWrite, pdi->lpBuf, pdi->sec, pdi->count);
#endif

    if (pdi->count == 0)
        return FALSE;

    cb = pdi->count << 9;

    if (pdi->h == (HANDLE)INVALID_HANDLE_VALUE)    // no virtual file
        {
        return FALSE;
        }

    if (pdi->lpBuf == NULL)
        return FALSE;

    pb = pdi->lpBuf;

    // Calculate seek offset into image file

    ofsFile = ((pdi->sec + pdi->offsec) << 9) + pdi->cbHdr;

    // Grow the image file as necessary, since ReadFile/WriteFile fails
    // if it goes past the EOF.

    if ((ofsFile+cb) > GetFileSize(pdi->h, NULL))
        {
        SetFilePointer(pdi->h, ofsFile+cb, NULL, FILE_BEGIN);
        SetEndOfFile(pdi->h);
        }

    if (SetFilePointer(pdi->h, ofsFile, NULL, FILE_BEGIN) == 0xFFFFFFFF)
        {
#if TRACEDISK
        printf("ErrRWImage: seek to offset %08X failed!\n", ofsFile);
#endif
        return FALSE;     // seek failed
        }

    pdi->count = 0;

#if TRACEDISK
    printf("ErrRWImage: cbSector = %d, ofs = %08X, cb = %08X\n",
        pdi->cbSector, ofsFile, cb);
#endif

    if (pdi->cbSector > 512)
        {
        const ULONG cbSec = pdi->cbSector;
        const ULONG wMask = pdi->cbSector - 1;

        // assume it's a CD-ROM, thus read-only

        if (fWrite)
            return FALSE;

        // we need to do the i/o in aligned 2048 byte blocks, so round down
        // the offset to a multiple of 2048 and break up the read into 3
        // reads of 2048, multiple of 2048, and 2048

        if ((cb2 = ofsFile & wMask) != 0)
            {
#if TRACEDISK
            printf("ErrRWImage: seeking to offset %08X\n", ofsFile & ~wMask);
            printf("ErrRWImage: reading %d byte leading block to align\n", cb2);
#endif

            SetFilePointer(pdi->h, ofsFile & ~wMask, NULL, FILE_BEGIN);
            ReadFile(pdi->h, &rgb, sizeof(rgb), &i, NULL);

            f = (i == sizeof(rgb));

            if (!f)
                return f;

            if (cb <= cb2)
                {
                memcpy(pb, rgb + cb2, cb);
                return f;
                }

            memcpy(pb, rgb + cb2, cbSec - cb2);
            pb += cb2;

            cb -= (cbSec - cb2);
            }

        cb2 = cb & wMask;
        cb = cb & ~wMask;
        }

    if (cb != 0)
    {
    if (fWrite)
        {
        WriteFile(pdi->h, pb, cb, &i, NULL);
        Assert((i == 0) || (i == cb));

//    printf("errno = %d, i = %d\n", errno, i);

        f = (i == cb);
        }
    else
        {
        ReadFile(pdi->h, pb, cb, &i, NULL);

        // partial reads are treated as error
        f = (i == cb);
        }
    }
    else
        f = TRUE;

    if (cb2 != 0)
        {
#if TRACEDISK
        printf("ErrRWImage: reading %d byte trailing block\n", cb2);
#endif

        ReadFile(pdi->h, &rgb, sizeof(rgb), &i, NULL);
        memcpy(pb + cb, rgb, cb2);
        }

#if TRACEDISK
     if (!fWrite)
        {
        BYTE *buf = pdi->lpBuf;
        printf("ErrRWImage: returning data: %02X %02X %02X %02X\n",
            buf[0], buf[1], buf[2], buf[3]);
        }
#endif

    return f;
}

#if defined(ATARIST) || defined(SOFTMAC) || defined(SOFTMAC2) || defined(POWERMAC)

//
// SCSI I/O routines
//


// **************************************************************************
//
// Function:  scsiTestUnitReady
//
// Description: Tests a given device and returns string
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiTestUnitReady(int adapter_id, int target_id, char *pbBuf, int cb)
{          
  VOLATILE SRB_ExecSCSICmd ExecSRB;

// Now we construct the SCSI Test Unit Ready SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));
  memset(pbBuf, 0, cb);

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_IN;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cb;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = 6;
  ExecSRB.CDBByte[0]    = SCSI_TST_U_RDY;
  ExecSRB.CDBByte[1]    = 0;
  ExecSRB.CDBByte[2]    = 0;
  ExecSRB.CDBByte[3]    = 0;
  ExecSRB.CDBByte[4]    = cb;
  ExecSRB.CDBByte[5]    = 0;

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    Sleep(10);

  return (ExecSRB.SRB_Status==SS_COMP);
}

// **************************************************************************
//
// Function:  scsiInquiry
//
// Description: Tests a given device and returns string
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiInquiry(int adapter_id, int target_id, char *pbBuf, int cb)
{          
  VOLATILE SRB_ExecSCSICmd ExecSRB;

// Now we construct the SCSI Inquiry SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));
  memset(pbBuf, 0, cb);

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_IN;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cb;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = 6;
  ExecSRB.CDBByte[0]    = SCSI_INQUIRY;
  ExecSRB.CDBByte[1]    = 0;
  ExecSRB.CDBByte[2]    = 0;
  ExecSRB.CDBByte[3]    = 0;
  ExecSRB.CDBByte[4]    = cb;
  ExecSRB.CDBByte[5]    = 0;

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    Sleep(10);

  return (ExecSRB.SRB_Status==SS_COMP);
}

// **************************************************************************
//
// Function:  scsiReadSector
//
// Description: Read a sector from a SCSI device (512 byte or 2K sectors)
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiReadSector(int adapter_id, int target_id, int sec, int count, char *pbBuf, int cbSec)
{
  VOLATILE SRB_ExecSCSICmd ExecSRB;
  BOOL  fLarge = (cbSec > 512) || (sec >= 0x200000) || (count > 255);

Ltryagain:

// Now we construct the SCSI Read SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));
  memset(pbBuf, 0, cbSec*count);

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_IN;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cbSec*count;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = fLarge ? 10 : 6;
  ExecSRB.CDBByte[0]    = fLarge ? SCSI_READ10: SCSI_READ6;

  if (fLarge)
    {
    ExecSRB.CDBByte[1]    = 0;
    ExecSRB.CDBByte[2]    = sec >> 24;
    ExecSRB.CDBByte[3]    = sec >> 16;
    ExecSRB.CDBByte[4]    = sec >> 8;
    ExecSRB.CDBByte[5]    = sec;
    ExecSRB.CDBByte[6]    = 0;
    ExecSRB.CDBByte[7]    = count >> 8;
    ExecSRB.CDBByte[8]    = count;
    ExecSRB.CDBByte[9]    = 0;
    }
  else
    {
    ExecSRB.CDBByte[1]    = sec >> 16;
    ExecSRB.CDBByte[2]    = sec >> 8;
    ExecSRB.CDBByte[3]    = sec;
    ExecSRB.CDBByte[4]    = count;
    ExecSRB.CDBByte[5]    = 0;
    }

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    ;

  // read may have failed because we used a 6 byte SRB on a SCSI 2 device

  if (!fLarge && (ExecSRB.SRB_Status != SS_COMP))
    {
    fLarge++;
    goto Ltryagain;
    }

  return (ExecSRB.SRB_Status==SS_COMP);
}

// **************************************************************************
//
// Function:  scsiWriteSector
//
// Description: Write a sector to a SCSI device (512 byte sectors only!)
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiWriteSector(int adapter_id, int target_id, int sec, int count, char *pbBuf, int cbSec)
{
  VOLATILE SRB_ExecSCSICmd ExecSRB;
  BOOL  fLarge = (cbSec > 512) || (sec >= 0x200000) || (count > 255);

Ltryagain:

// Now we construct the SCSI Write6 SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_OUT;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cbSec*count;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = fLarge ? 10 : 6;
  ExecSRB.CDBByte[0]    = fLarge ? SCSI_WRITE10: SCSI_WRITE6;

  if (fLarge)
    {
    ExecSRB.CDBByte[1]    = 0;
    ExecSRB.CDBByte[2]    = (BYTE)(sec >> 24);
    ExecSRB.CDBByte[3]    = (BYTE)(sec >> 16);
    ExecSRB.CDBByte[4]    = (BYTE)(sec >> 8);
    ExecSRB.CDBByte[5]    = (BYTE)(sec & 255);
    ExecSRB.CDBByte[6]    = 0;
    ExecSRB.CDBByte[7]    = count >> 8;
    ExecSRB.CDBByte[8]    = count;
    ExecSRB.CDBByte[9]    = 0;
    }
  else
    {
    ExecSRB.CDBByte[1]    = (BYTE)(sec >> 16);
    ExecSRB.CDBByte[2]    = (BYTE)(sec >> 8);
    ExecSRB.CDBByte[3]    = (BYTE)(sec & 255);
    ExecSRB.CDBByte[4]    = count;
    ExecSRB.CDBByte[5]    = 0;
    }

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    ;

  // write may have failed because we used a 6 byte SRB on a SCSI 2 device

  if (!fLarge && (ExecSRB.SRB_Status != SS_COMP))
    {
    fLarge++;
    goto Ltryagain;
    }

  return (ExecSRB.SRB_Status==SS_COMP);
}

// **************************************************************************
//
// Function:  scsiGetDiskInfo
//
// Description: Returns info about the device
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiGetDiskInfo(int adapter_id, int target_id, char *pbBuf, int cb)
{          
  VOLATILE SRB_GetDiskInfo InfoSRB;

// Now we construct the SCSI Get Info SRB and send it to ASPI!
 
  memset(&InfoSRB, 0, sizeof(InfoSRB));
  memset(pbBuf, 0, cb);

  InfoSRB.SRB_Cmd     = SC_GET_DISK_INFO;
  InfoSRB.SRB_HaId     = adapter_id;
  InfoSRB.SRB_Flags    = 0;
  InfoSRB.SRB_Target    = target_id;

  pfnSendASPI32Command(&InfoSRB);

  while ((volatile BYTE)InfoSRB.SRB_Status == SS_PENDING)
    ;

#if TRACEDISK
  printf("GetDiskInfo: status = %d\n", InfoSRB.SRB_Status);
  printf("GetDiskInfo: flags  = %02X\n", InfoSRB.SRB_DriveFlags);
  printf("GetDiskInfo: info   = %d\n", InfoSRB.SRB_Int13HDriveInfo);
  printf("GetDiskInfo: heads  = %d\n", InfoSRB.SRB_Heads);
  printf("GetDiskInfo: sectors= %d\n", InfoSRB.SRB_Sectors);
#endif

  return (InfoSRB.SRB_Status==SS_COMP);
}

// **************************************************************************
//
// Function:  scsiResetDevice
//
// Description: Resets a given device
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiResetDevice(int adapter_id, int target_id)
{          
  VOLATILE SRB_BusDeviceReset ResetSRB;

// Now we construct the SCSI Get Info SRB and send it to ASPI!
 
  memset(&ResetSRB, 0, sizeof(ResetSRB));

  ResetSRB.SRB_Cmd     = SC_GET_DISK_INFO;
  ResetSRB.SRB_HaId     = (BYTE)adapter_id;
  ResetSRB.SRB_Flags    = 0;
  ResetSRB.SRB_Target    = (BYTE)target_id;

  pfnSendASPI32Command(&ResetSRB);

  while ((volatile BYTE)ResetSRB.SRB_Status == SS_PENDING)
    ;

#if TRACEDISK
  printf("ResetDevice: status         = %02X\n", ResetSRB.SRB_Status);
  printf("ResetDevice: adapter status = %02X\n", ResetSRB.SRB_HaStat);
  printf("ResetDevice: target status  = %02X\n", ResetSRB.SRB_TargStat);
#endif

  return (ResetSRB.SRB_Status==SS_COMP);
}

// **************************************************************************
//
// Function:  scsiReadTOC
//
// Description: Returns the TOC of the CD-ROM
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiReadTOC(int adapter_id, int target_id, char *pbBuf, int cb)
{          
  VOLATILE SRB_ExecSCSICmd ExecSRB;

// Now we construct the SCSI ReadTOC SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));
  memset(pbBuf, 0, cb);

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_IN;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cb;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = 10;
  ExecSRB.CDBByte[0]    = SCSI_READ_TOC;
  ExecSRB.CDBByte[1]    = 2;
  ExecSRB.CDBByte[2]    = 0;
  ExecSRB.CDBByte[3]    = 0;
  ExecSRB.CDBByte[4]    = 0;
  ExecSRB.CDBByte[5]    = 0;
  ExecSRB.CDBByte[6]    = 0;
  ExecSRB.CDBByte[7]    = 0;
  ExecSRB.CDBByte[8]    = 4;
  ExecSRB.CDBByte[9]    = 0;

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    ;

  return (ExecSRB.SRB_Status==SS_COMP);
}


// **************************************************************************
//
// Function:  scsiReadCapacity
//
// Description: Returns the storage capacity of this unit
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiReadCapacity(int adapter_id, int target_id, char *pbBuf, int cb)
{          
  VOLATILE SRB_ExecSCSICmd ExecSRB;
  //long cnt = 1000000;

// Now we construct the SCSI ReadCapacity SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));
  memset(pbBuf, 0, cb);

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_IN;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cb;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = 10;
  ExecSRB.CDBByte[0]    = SCSI_RD_CAPAC;

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    ;

  return (ExecSRB.SRB_Status==SS_COMP);
}



// **************************************************************************
//
// Function:  scsiModeSense
//
// Description: Returns the mode sense page of the device
//
// Returns -   TRUE is successful
//
// **************************************************************************
BOOL __stdcall scsiModeSense(int adapter_id, int target_id, char *pbBuf, int cb, int page)
{          
  VOLATILE SRB_ExecSCSICmd ExecSRB;
  //long cnt = 1000000;

// Now we construct the SCSI ReadCapacity SRB and send it to ASPI!
 
  memset(&ExecSRB, 0, sizeof(ExecSRB));
  memset(pbBuf, 0, cb);

  ExecSRB.SRB_Cmd     = SC_EXEC_SCSI_CMD;
  ExecSRB.SRB_HaId     = adapter_id;
  ExecSRB.SRB_Flags    = SRB_DIR_IN;
  ExecSRB.SRB_Target    = target_id;
  ExecSRB.SRB_BufPointer   = pbBuf;
  ExecSRB.SRB_BufLen    = cb;
  ExecSRB.SRB_SenseLen   = SENSE_LEN;
  ExecSRB.SRB_CDBLen    = 6;
  ExecSRB.CDBByte[0]    = SCSI_MODE_SEN6;
  ExecSRB.CDBByte[2]    = page;
  ExecSRB.CDBByte[4]    = cb;

  pfnSendASPI32Command(&ExecSRB);

  while ((volatile BYTE)ExecSRB.SRB_Status == SS_PENDING)
    ;

  return (ExecSRB.SRB_Status==SS_COMP);
}

#endif


