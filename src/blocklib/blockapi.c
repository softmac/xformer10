
/****************************************************************************

    BLOCKAPI.C

    High level API for block device routines.

    Copyright (C) 1998-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    10/10/2001  darekm

****************************************************************************/

#include "precomp.h"


enum fstype __stdcall FstIdentifyFileSystem(DISKINFO *pdi)
{
    char unsigned rgch[2048*10];

    if (pdi->dt == DISK_NONE)
        return FS_RAW;

    // Keep looping through partition sectors 
    // until we find a valid boot sector

    for (;;)
        {
        pdi->sec = 0;
        pdi->count = 4; // 2048/pdi->cbSector;
        pdi->lpBuf = rgch;

        if (!FRawDiskRWPdi(pdi, 0))
            break;

        if (rgch[0] == 0x45 && rgch[1] == 0x52)
            {
            // 'ER' signature, skip to next sector

            pdi->offsec++;
            continue;
            }

        if (rgch[0] == 0x50 && rgch[1] == 0x4D)
            {
            // 'PM' signature, Mac partition map

            if (strncmp(&rgch[48], "Apple_HFS", 10))
                {
                pdi->offsec++;
                continue;
                }

            pdi->offsec = SwapL(*(int *)&rgch[8]);
            continue;
            }

        if (rgch[1024] == 0x42 && rgch[1025] == 0x44)
            {
#if TRACEDISK
            printf("Mac HFS file system\n");
#endif
            return FS_HFS;
            }

        if (rgch[1024] == 0xD2 && rgch[1025] == 0xD7)
            {
#if TRACEDISK
            printf("Mac MFS file system\n");
#endif
            // either 400K or 800K disk image / Spectre Disk. Get the actual
            // disk size from the boot sector

            if (!pdi->fNT && pdi->dt == DISK_FLOPPY)
                {
                pdi->size = SwapW(*(WORD *)&rgch[1024+18]) *
                     SwapL(*(ULONG *)&rgch[1024+20]) / 1024;

                if (pdi->size <= 400)
                    pdi->size = 400;
                else
                    pdi->size = 800;
                }

            return FS_MFS;
            }

        if (rgch[0] == 0x00 && rgch[1] == 0x03)
            {
#if TRACEDISK
            printf("Atari DOS file system\n");
#endif
            return FS_ATARIDOS;
            }

        if (rgch[0] == 0x4D && rgch[1] == 0x03)
            {
#if TRACEDISK
            printf("Atari MyDOS file system\n");
#endif
            return FS_MYDOS;
            }

#if 0
        if (rgch[0] == 0x60 && rgch[1] == 0x1C && rgch[21] == 0xF8)
          if (rgch[11] == 0 && rgch[12] == 2 && rgch[16] == 2)
            {
#if TRACEDISK
            printf("Atari ST boot sector\n");
#endif
            if ((pdi->size = (*(WORD *)&rgch[19])) == 0)
                pdi->size = (*(ULONG *)&rgch[32]);

            pdi->size /= 2;     // convert # of sectors to K
            return FS_FAT16;
            }

        if (rgch[510] == 0x55 && rgch[511] == 0xAA && rgch[21] == 0xF8)
          if (rgch[11] == 0 && rgch[12] == 2 && rgch[16] == 2)
            {
#if TRACEDISK
            printf("Atari ST boot sector\n");
#endif
            if ((pdi->size = (*(WORD *)&rgch[19])) == 0)
                pdi->size = (*(ULONG *)&rgch[32]);

            pdi->size /= 2;     // convert # of sectors to K
            return FS_FAT16;
            }
#endif

        if (rgch[21] == 0xF8 && rgch[11] == 0x00 && rgch[16] == 2)
            {
#if TRACEDISK
            printf("Very minimal MS-DOS boot sector (possibly Atari ST hard disk)\n");
#endif
            if ((pdi->size = (*(WORD *)&rgch[19])) == 0)
                pdi->size = (*(ULONG *)&rgch[32]);

            pdi->size /= 2;     // convert # of sectors to K

            // Check for incorrectly formatted Atari ST floppy disk
            // which has a bogus $F8 media descriptor

            if (pdi->size <= 720)
                return FS_FAT12;

            return FS_FAT16;
            }

        if (rgch[21+12] == 0xF9 && rgch[11+12] == 0x00 && rgch[12+12] == 0x02 && rgch[16+12] == 2)
            {
#if TRACEDISK
            printf("Atari ST .MSA boot sector\n");
#endif
            pdi->cbHdr = 12;
            pdi->size = 40 * rgch[24+12] * rgch[26+12];
            return FS_FAT12;
            }

        // MS-DOS boot sectors vary a great deal, so we check for it last

        if (rgch[0] == 0xE9 || rgch[0] == 0xEB || !rgch[0] || rgch[21] == 0xF9)
          if (rgch[11] == 0 && rgch[12] == 2 && rgch[16] == 2)
            {
#if TRACEDISK
            printf("MS-DOS boot sector\n");
#endif
            if (!pdi->fNT && pdi->dt == DISK_FLOPPY)
                {
                // size = 40K * sec/track * sides

                pdi->size = 40 * rgch[24] * rgch[26];

#if TRACEDISK
                printf("size = %dK\n", pdi->size);
#endif
                }
            return FS_FAT12;
            }

        // keep going (it'll run out eventually)

        pdi->offsec++;

        if (pdi->offsec >= 100)
            {
            // probably won't find anything

            pdi->offsec = 0;
            break;
            }
        }

    return FS_RAW;
}


//
// Main Disk I/O API
//

BOOL __stdcall FInitBlockDevLib(void)
{
    HANDLE hLib;
    DWORD  ASPIStatus;

    hLib = LoadLibrary ("WNASPI32.DLL");
    pfnGetASPI32SupportInfo = GetProcAddress (hLib, "GetASPI32SupportInfo");
    pfnSendASPI32Command = (WinAspiCommand)GetProcAddress (hLib, "SendASPI32Command");

    if (hLib == NULL)
        goto Lfail;

    if (pfnGetASPI32SupportInfo == NULL)
        goto Lfail;

    if (pfnSendASPI32Command == NULL)
        goto Lfail;

    ASPIStatus = pfnGetASPI32SupportInfo();

    switch (HIBYTE(LOWORD(ASPIStatus)))
        {
    case SS_COMP:      // ASPI for Windows is resident.
        NumAdapters = (LOWORD(LOBYTE(ASPIStatus)));
#if TRACEDISK
        printf("number of adapters = %d\n", NumAdapters);
#endif
        break; 

    default:
Lfail:
#if TRACEDISK
        printf("ASPI for Windows not available!\n");
#endif
        FreeLibrary (hLib);
        return FALSE;
        }

    return TRUE;
}


DISKINFO * __stdcall PdiOpenDisk(enum disktype dt, long l, long flags)
{
    DISKINFO *pdi;
    OSVERSIONINFO oi;
    ULONG lSize;
    
#if USEHEAP
    pdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DISKINFO));
#else
    pdi = malloc(sizeof(DISKINFO));
#endif

    if (pdi == NULL)
        return pdi;

    pdi->dt = dt;
    pdi->fst = FS_RAW;
    pdi->fWP = FALSE;
    pdi->size = 0;
    pdi->cbSector = 512;
    memset(pdi->szImage, 0, MAX_PATH);
    memset(pdi->szVolume, 0, MAX_PATH);
    pdi->ctl = 0;
    pdi->id = 0;
    pdi->h = INVALID_HANDLE_VALUE;
    memset(pdi->szCwd, 0, MAX_PATH);
    pdi->dirID = 0;
    pdi->sec = 0;
    pdi->side = 0;
    pdi->track = 0;
    pdi->count = 0;
    pdi->lpBuf = NULL;
    pdi->offsec = 0;
    pdi->cfd = 0;
	pdi->pfd = NULL;

#if USEHEAP
    HeapCompact(GetProcessHeap(), 0);
    pdi->pfd = HeapAlloc(GetProcessHeap(), 0, sizeof(WIN32_FIND_DATA));
#else
    pdi->pfd = malloc(sizeof(WIN32_FIND_DATA));
#endif

    if (pdi->pfd == NULL)
        goto Lerror;

    oi.dwOSVersionInfoSize = sizeof(oi);
    GetVersionEx(&oi);  // REVIEW: requires Win32s 1.2+

    switch(oi.dwPlatformId)
        {
    default:
    case VER_PLATFORM_WIN32s:
//    case VER_PLATFORM_WIN32_WINDOWS:
        pdi->fNT = 0;

#if 0
        // Windows 95 build 950 sucks at SCSI
        if ((oi.dwBuildNumber & 0xFFFF) == 950)
            if (dt == DISK_SCSI)
                goto Lerror;
#endif
        break;

    case VER_PLATFORM_WIN32_NT:
        pdi->fNT = 1;
           break;
        }

    switch (dt)
        {
    default:
Lerror:
#if TRACEDISK
    printf("error opening disk\n");
#endif

#if USEHEAP
        if (pdi->pfd != NULL)
            HeapFree(GetProcessHeap(), 0, pdi->pfd);
        HeapFree(GetProcessHeap(), 0, pdi);
#else
        if (pdi->pfd != NULL)
            free(pdi->pfd);
        free(pdi);
#endif
        pdi = NULL;
        return pdi;
        break;

    case DISK_FLOPPY:
        pdi->size = 1440; // won't know real size till later
        pdi->id = l & 1;
        break;

    case DISK_WIN32:
        pdi->ctl = -1;
        pdi->id = l & 31;

        strcpy(pdi->szImage, "\\\\.\\.:\\");

        pdi->szImage[4] = 'A' + (char)pdi->id;

        switch(GetDriveType(&pdi->szImage[4]))
            {
        default:
#if TRACEDISK
            printf("unknown drive type\n");
#endif
            goto Lerror;

        case DRIVE_CDROM:
#if TRACEDISK
            printf("drive is a CD-ROM\n");
#endif
            pdi->cbSector = 2048;

        case DRIVE_REMOVABLE:
#if TRACEDISK
            printf("drive is removable\n");
#endif

        case DRIVE_FIXED:
#if TRACEDISK
            printf("drive is valid\n");
#endif
            break;
            }

        pdi->szImage[6] = 0;

        goto Limage;
        break;

#if defined(ATARIST) || defined(SOFTMAC) || defined(SOFTMAC2) || defined(POWERMAC)

    case DISK_SCSI:
        pdi->ctl = (l>>4) & 7;
        pdi->id = l & 15;

        if (NumAdapters == 0)
            return FALSE;

Lblock:
        {
        unsigned char bufID[256];

        if (!scsiInquiry(pdi->ctl, pdi->id, bufID, 32))
            {
            if (pdi->dt == DISK_WIN32)
                goto Limage;

            goto Lerror;
            }

        bufID[32] = 0;
        strcpy(pdi->szVolume, &bufID[8]);

        // device type 5, media removable likely means CD-ROM

        if ((bufID[0] == 0x05) && (bufID[0x01] & 0x80))
            pdi->cbSector = 2048;

        // if the device has media present, let it tell us

        if (scsiReadCapacity(pdi->ctl, pdi->id, bufID, 8))
            {
            unsigned int cSecMax, cbSec;

            cSecMax = SwapL(*(int *)bufID) + 1;
            pdi->cbSector = SwapL(*(int *)&bufID[4]);

            // to avoid overflow of cSecMax*cbSector, pre-divide cbSector
            pdi->size = cSecMax * (pdi->cbSector / 512) / 2;
            }
        }
        break;
#endif

    case DISK_IMAGE:
        strcpy(pdi->szImage, (char *)l);

Limage:
        if (flags & DI_CREATE)
            {
            pdi->h = CreateFile(pdi->szImage,
                     GENERIC_READ | GENERIC_WRITE,
                     0 /* FILE_SHARE_READ */,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                     NULL);

            if (pdi->h == (HANDLE)INVALID_HANDLE_VALUE)
                goto Lerror;
            }
        else if ((flags & DI_READONLY) ||
            (pdi->h = CreateFile(pdi->szImage,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                     NULL)) == INVALID_HANDLE_VALUE)
            {
            pdi->h = CreateFile(pdi->szImage,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE, // needs the write permission in case file already open
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                     NULL);

            pdi->fWP = TRUE;

            if (pdi->h == (HANDLE)INVALID_HANDLE_VALUE)
                goto Lerror;
            }

#if TRACEDISK
        printf("opened drive handle %08X\n", pdi->h);
#endif

        lSize = GetFileSize(pdi->h, NULL);

        pdi->size = lSize / 1024;

        if ((lSize & 255) == 84)
            {
            // Probably a Disk Copy .image file

            pdi->cbHdr = 84;
            }
        else if ((lSize & 255) == 128)
            {
            // Probably a Disk Copy .image file

            pdi->cbHdr = 212;
            }
        else if ((lSize & 255) == 16)
            {
            // Probably an SIO2PC .ATR file

            pdi->cbHdr = 16;
            }

        if (pdi->fNT)
            {
            DISK_GEOMETRY Geometry;

            if (GetDiskGeometry(pdi->h, &Geometry))
                pdi->cbSector = Geometry.BytesPerSector;
            }

        if (pdi->dt == DISK_WIN32)
            {
            }

        break;
        }

    if (flags & DI_READONLY)
        pdi->fWP = TRUE;

    if (!(flags & DI_QUERY))
        pdi->fst = FstIdentifyFileSystem(pdi);

    return pdi;
}


ULONG CSectorsInCache(PDI pdi, BYTE *pb, ULONG sec, ULONG count)
{
    ULONG cCached = 0;
    ULONG isec;

    for (isec = 0; isec < count; isec++)
        {
        // look for cached sector in the given pdi

        int i;

        for (i = 0; i < CACHESLOTS; i++)
            {
            if (pdi->dc[i].pb_dc && (pdi->dc[i].sec_dc == sec + isec))
                {
                // sector is cached, one less sector to read

                memcpy(pb, pdi->dc[i].pb_dc, pdi->cbSector);
                pb += pdi->cbSector;
                cCached++;
                break;
                }
            }

        // sector wasn't found

        if (i == CACHESLOTS)
            break;
        }

    return cCached;
}


BOOL FAddSectorsToCache(PDI pdi, BYTE *pb, ULONG sec, ULONG count)
{
    ULONG isec;

    // update the cache

    for (isec = 0; isec < count; isec++)
        {
        int i;

        for (i = CACHESLOTS-1; i > 0; i--)
            {
            if (pdi->dc[i].pb_dc && (pdi->dc[i].sec_dc == sec + isec))
                {
                // sector was already cached, update cached data
                // should be the same!!!

                Assert(!memcmp(pdi->dc[i].pb_dc, pb + isec * pdi->cbSector,
                     pdi->cbSector));
                break;
                }
            }

        // Add sector to ith cache entry

        if (pdi->dc[i].pb_dc == NULL)
#if USEHEAP
            pdi->dc[i].pb_dc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pdi->cbSector);
#else
            pdi->dc[i].pb_dc = malloc(pdi->cbSector);
#endif

        if (pdi->dc[i].pb_dc)
            {
            memcpy(pdi->dc[i].pb_dc, pb + isec * pdi->cbSector, pdi->cbSector);
            pdi->dc[i].sec_dc = sec + isec;
            }

        // now move the ith cache entry to the end, and slide things down

        pdi->dc[CACHESLOTS] = pdi->dc[i];
        memcpy(&pdi->dc[i], &pdi->dc[i+1], sizeof(pdi->dc[i]) * (CACHESLOTS - i));
        }

    return TRUE;
}


BOOL __stdcall FlushCachePdi(PDI pdi)
{
    int i;

    // flush any cached sectors

    for (i = CACHESLOTS-1; i > 0; i--)
        {
        if (pdi->dc[i].pb_dc)
            {
#if USEHEAP
            HeapFree(GetProcessHeap(), 0, pdi->dc[i].pb_dc);
#else
            free(pdi->dc[i].pb_dc);
#endif

            pdi->dc[i].pb_dc = NULL;
            }
        }

    return TRUE;
}


BOOL __stdcall FRawDiskRWPdi(DISKINFO *pdi, BOOL fWrite)
{
    BOOL f = FALSE;

    if (pdi->fWP && fWrite)
        return FALSE;

    if (pdi->track || pdi->side)
      if (pdi->fNT || pdi->dt != DISK_FLOPPY)
        {
        // need to convert to logical sector #

        if (pdi->size == 360)
            {
            pdi->sec += pdi->track * 9;
            }
        else if (pdi->size == 400)
            {
            pdi->sec += pdi->track * 10;
            }
        else if (pdi->size == 720)
            {
            pdi->sec += pdi->track * 18 + pdi->side * 9;
            }
        else if ((pdi->size == 800) || (pdi->size == 820))
            {
            pdi->sec += pdi->track * 20 + pdi->side * 10;
            }
        else if (pdi->size == 1440)
            {
            pdi->sec += pdi->track * 36 + pdi->side * 18;
            }

        pdi->track = 0;
        pdi->side = 0;
        }

    switch (pdi->dt)
        {
    default:
        return FALSE;

    case DISK_FLOPPY:
        if (pdi->fNT)
            {
            ULONG ccached = 0, sec = pdi->sec, count = pdi->count;
            BYTE *lpBuf = pdi->lpBuf;

            if (!fWrite && 0)
                ccached = CSectorsInCache(pdi, lpBuf, sec, count); 

            if (count == ccached)
                f = TRUE;
            else
                f = FReadWriteSecNT(pdi, fWrite);

            if (f && 0)
                FAddSectorsToCache(pdi, lpBuf, sec, count); 
            }
        else
            {
            // on Win 95, break it up into per track I/O requests
            // if size is 0, assume that track/side/sec are already set
            // otherwise convert sec into track/side/sec based on size

            int sector = pdi->sec;
            int count = pdi->count;

            while (count > 0)
                {
                int countT = count;

                if (pdi->size == 360)
                    {
                    if ((sector % 9) + countT > 9)
                        countT = 9 - (sector % 9);

                    pdi->track = sector / 9;
                    pdi->side  = 0;
                    pdi->sec   = (sector % 9);
                    }
                else if (pdi->size == 400)
                    {
                    if ((sector % 10) + countT > 10)
                        countT = 10 - (sector % 10);

                    pdi->track = sector / 10;
                    pdi->side  = 0;
                    pdi->sec   = (sector % 10);
                    }
                else if (pdi->size == 720)
                    {
                    if ((sector % 9) + countT > 9)
                        countT = 9 - (sector % 9);

                    pdi->track = sector / 18;
                    pdi->side  = (sector % 18) / 9;
                    pdi->sec   = (sector % 9);
                    }
                else if ((pdi->size == 800) || (pdi->size == 820))
                    {
                    if ((sector % 10) + countT > 10)
                        countT = 10 - (sector % 10);

                    pdi->track = sector / 20;
                    pdi->side  = (sector % 20) / 10;
                    pdi->sec   = (sector % 10);
                    }
                else if (pdi->size == 1440)
                    {
                    if ((sector % 18) + countT > 18)
                        countT = 18 - (sector % 18);

                    pdi->track = sector / 36;
                    pdi->side  = (sector % 36) / 18;
                    pdi->sec   = sector % 18;
                    }

                pdi->count = countT;

                f = FReadWriteSec9x(pdi, fWrite);

                if (!f)
                    return FALSE;

                if ((pdi->size == 1440) && (sector == 2))
                    {
                    if ((pdi->lpBuf[0] == 0xD2) && (pdi->lpBuf[1] == 0xD7))
                        {
                        // either 400K or 800K disk image / Spectre Disk.
                        // Get the disk size from the boot sector
            
                        pdi->size = SwapW(*(WORD *)&pdi->lpBuf[18]) *
                                 SwapL(*(ULONG *)&pdi->lpBuf[20]) / 1024;

                        if (pdi->size <= 400)
                            pdi->size = 400;
                        else
                            pdi->size = 800;
                        }
                    }
            
                count  -= countT;
                sector += countT;
                pdi->lpBuf += countT << 9;
                }
            }
        break;

#if defined(ATARIST) || defined(SOFTMAC) || defined(SOFTMAC2) || defined(POWERMAC)

    case DISK_SCSI:
        if (NumAdapters == 0)
            break;

        {
        ULONG realsec = pdi->sec + pdi->offsec, realcount = pdi->count;
        ULONG ccached = 0;
        BYTE *IoBuffer;
        DWORD VirtBufSize = 0;

        if (pdi->cbSector > 512)
            {
            realcount = ((realcount + realsec) * 512) / pdi->cbSector;
            realsec = (realsec * 512) / pdi->cbSector;
            realcount -= realsec - 1;

            // Don't write to CD-ROM, Windows 95 can't seem to handle that

            if (fWrite)
                return FALSE;

            VirtBufSize = realcount * pdi->cbSector;
            IoBuffer = VirtualAlloc(NULL,VirtBufSize,MEM_COMMIT,PAGE_READWRITE);

            if (IoBuffer == NULL)
                return FALSE;
            }
        else
            IoBuffer = pdi->lpBuf;

        if (!fWrite)
            ccached = CSectorsInCache(pdi, IoBuffer, realsec, realcount); 

        if (realcount == ccached)
            f = TRUE;
        else if (fWrite)
            {
            ULONG blockcount = realcount;

            Assert(pdi->cbSector == 512);

            while (blockcount != 0)
                {
                f = scsiWriteSector(pdi->ctl, pdi->id,
                     realsec + realcount - blockcount,
                     min(32, blockcount),
                     IoBuffer + ((realcount - blockcount) * pdi->cbSector),
                     pdi->cbSector);

                blockcount -= min(32, blockcount);
                }
            }
        else
            {
            // Break up reads into blocks of at most 32 sectors (64K on CD)

            ULONG blockcount = realcount - ccached;

            while (blockcount != 0)
                {
                f = scsiReadSector(pdi->ctl, pdi->id,
                     realsec + realcount - blockcount,
                     min(32, blockcount),
                     IoBuffer + ((realcount - blockcount) * pdi->cbSector),
                     pdi->cbSector);

                blockcount -= min(32, blockcount);
                }
            }

        if (pdi->cbSector > 512)
            {
            if (f)
                {
                if (!fWrite)
                    {
                    memcpy(pdi->lpBuf, (BYTE *)IoBuffer + ((pdi->sec + pdi->offsec) * 512 - (realsec * pdi->cbSector)), pdi->count * 512);
                    }

                FAddSectorsToCache(pdi, IoBuffer, realsec, realcount);
                }

            VirtualFree(IoBuffer, VirtBufSize, MEM_DECOMMIT);
            VirtualFree(IoBuffer, 0, MEM_RELEASE);
            }
        }
        break;
#endif

    case DISK_WIN32:
    case DISK_IMAGE:
        f = FReadWriteImage(pdi, fWrite);
        break;
        }

    return f;
}


BOOL __stdcall CloseDiskPdi(DISKINFO *pdi)
{
    switch (pdi->dt)
        {
    default:
        return FALSE;

    case DISK_FLOPPY:
    case DISK_IMAGE:
        if (pdi->h != INVALID_HANDLE_VALUE)
            CloseHandle(pdi->h);

        // fall through

    case DISK_WIN32:
    case DISK_SCSI:
        if (pdi->pdiStub)
            CloseDiskPdi(pdi->pdiStub);

        FlushCachePdi(pdi);


#if USEHEAP
        if (pdi->pfd != NULL)
            HeapFree(GetProcessHeap(), 0, pdi->pfd);
        HeapFree(GetProcessHeap(), 0, pdi);
#else
        if (pdi->pfd != NULL)
            free(pdi->pfd);
        free(pdi);
#endif
        break;
        }

    return TRUE;
}

