
////////////////////////////////////////////////////////////////////////
//
// GIECORE.C
//
// - back end API for the Gemulator Information Exchange
//
// 10/10/98 DarekM created
//
////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <windows.h>
#include <winnt.h>
#include <excpt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <dos.h>
#include <direct.h>
#include <fcntl.h>
#include <time.h>
#include <commdlg.h>    // includes common dialog functionality
#include <dlgs.h>       // includes common dialog template defines
#include <cderr.h>      // includes the common dialog error codes

#define  USEHEAP 1

// If STUB is define, fake all the drive stuff using the PC's drives

#define STUB

#pragma pack(1)

#define Assert(f) 0

////////////////////////////////////////////////////////////////////////

char **vrgszDrives;
int  vcDrives;
int  iDriveSCSI, iDriveFloppy, iDriveImages;
int  vid;
PDI  vpdi;

int  EnumeratePhysicalDrives()
{
    char rgch[256];

    // first free all the existing strings

    while (vcDrives)
        {
#if USEHEAP
        HeapFree(GetProcessHeap(), 0, vrgszDrives[--vcDrives]);
#else
        free(vrgszDrives[--vcDrives]);
#endif
        }

	if (vrgszDrives)
        {
#if USEHEAP
		HeapFree(GetProcessHeap(), 0, vrgszDrives);
#else
		free(vrgszDrives);
#endif
        }

#if USEHEAP
    HeapCompact(GetProcessHeap(), 0);
    vrgszDrives = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(char *));
#else
    vrgszDrives = malloc(sizeof(char *));
#endif

    // make sure SCSI is initialized

    FInitBlockDevLib();

#ifdef STUB
    {
    int rgfDrives = GetLogicalDrives();
    int cch, ich = 0;
#if USEHEAP
    char *szDrives = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
         cch = GetLogicalDriveStrings(0, NULL));
#else
    char *szDrives = malloc(cch = GetLogicalDriveStrings(0, NULL));
#endif

    GetLogicalDriveStrings(cch, szDrives);

//    printf("GetLogicalDrives() returned: $%08X\n", rgfDrives);
//    printf("string buffer size returned: %d\n", cch);

    while (ich < cch && szDrives[ich])
        {
//		printf("%dth drive name = %s\n", vcDrives, &szDrives[ich]);

#if USEHEAP
        vrgszDrives = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            vrgszDrives, sizeof(char *) * (vcDrives+1));
#else
        vrgszDrives = realloc(vrgszDrives, sizeof(char *) * (vcDrives+1));
#endif
        vrgszDrives[vcDrives++] = strdup(&szDrives[ich]);
        ich += strlen(&szDrives[ich]) + 1;
        }
    }
#endif

    // Add SCSI devices

    iDriveSCSI = vcDrives;

    {
    int ctl_id, trg_id;

    for (ctl_id = 0; ctl_id < NumAdapters; ctl_id++)
        {
        for (trg_id = 0; trg_id < 16; trg_id++)
            {
            PDI pdi;

            if (pdi = PdiOpenDisk(DISK_SCSI,
                   LongFromScsi(ctl_id, trg_id), DI_QUERY))
                {
#if USEHEAP
                vrgszDrives = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    vrgszDrives, sizeof(char *) * (vcDrives+1));
#else
                vrgszDrives = realloc(vrgszDrives, sizeof(char *) * (vcDrives+1));
#endif
                wsprintf(rgch, "ASPI ctl:%d dev:%d", ctl_id, trg_id);

                if (pdi->szVolume[0])
                    {
                    strcat(rgch, " (");
                    strcat(rgch, pdi->szVolume);
                    strcat(rgch, ")");
                    }

//                printf("%dth drive name = %s\n", vcDrives, rgch);

                vrgszDrives[vcDrives++] = strdup(rgch);

                CloseDiskPdi(pdi);
                }
            }
        }
    }
        
    // Add floppy disks 

    iDriveFloppy = vcDrives;

    if (GetLogicalDrives() & 1 && (GetDriveType("A:\\") == DRIVE_REMOVABLE))
        {
//        printf("%dth drive name = Non-DOS floppy on A:\n", vcDrives);

#if USEHEAP
        vrgszDrives = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            vrgszDrives, sizeof(char *) * (vcDrives+1));
#else
        vrgszDrives = realloc(vrgszDrives, sizeof(char *) * (vcDrives+1));
#endif
        vrgszDrives[vcDrives++] = strdup("Non-DOS floppy on A:");
        }

    if (GetLogicalDrives() & 2 && (GetDriveType("B:\\") == DRIVE_REMOVABLE))
        {
//        printf("%dth drive name = Non-DOS floppy on B:\n", vcDrives);

#if USEHEAP
        vrgszDrives = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            vrgszDrives, sizeof(char *) * (vcDrives+1));
#else
        vrgszDrives = realloc(vrgszDrives, sizeof(char *) * (vcDrives+1));
#endif
        vrgszDrives[vcDrives++] = strdup("Non-DOS floppy on B:");
        }

    // No disk images yet

    iDriveImages = vcDrives;

    return vcDrives;
}

BOOL GetIthPhysicalDrive(int id, char *pch, int cch)
{
    if (id >= vcDrives || id < 0)
        return FALSE;

    if ((int)strlen(vrgszDrives[id]) > cch)
        return FALSE;

    if (id < iDriveSCSI)
        {
        wsprintf(pch, "Disk image files on %s", vrgszDrives[id]);
        }
    else if (id >= iDriveImages)
        {
        wsprintf(pch, "Disk image %s", vrgszDrives[id]);
        }
    else
        strcpy(pch, vrgszDrives[id]);
    return TRUE;
}


////////////////////////////////////////////////////////////////////////

HANDLE vhFind; // handle for FindNextFile
WIN32_FIND_DATA *vrgpfd;
LONG vparentID;
int vcFind;

int  EnumerateDirectory(int id, const char *szPath)
{
    char rgch[MAX_PATH];
    int emOld;

    if (id >= vcDrives || id < 0)
        return 0;

    strcpy(rgch, vrgszDrives[id]);
    strcat(rgch, szPath);
//    strcat(rgch, "*.*");

    if (szPath[0] == 0)
        {
        // getting a root directory, possibly a new disk

        if (vpdi)
            CloseDiskPdi(vpdi);
        vpdi = NULL;
        }
//    else if (vpdi == NULL)
//       return 0;

    if (vrgpfd)
        {
#if USEHEAP
        HeapFree(GetProcessHeap(), 0, vrgpfd);
#else
        free(vrgpfd);
#endif
        }

#if USEHEAP
    HeapCompact(GetProcessHeap(), 0);
    vrgpfd = HeapAlloc(GetProcessHeap(), 0, sizeof(WIN32_FIND_DATA));
#else
    vrgpfd = malloc(sizeof(WIN32_FIND_DATA));
#endif

    vcFind = 0;
    vid = id;

    if (id >= iDriveImages)
        {
        if (vpdi == NULL)
            vpdi = PdiOpenDisk(DISK_IMAGE, (long)vrgszDrives[id], DI_READONLY);

        if (vpdi == NULL)
            return 0;

        vcFind = CntReadDiskDirectory(vpdi, szPath, &vrgpfd);
        return vcFind;
        }

    if (id >= iDriveFloppy)
        {
        if (vpdi == NULL)
            vpdi = PdiOpenDisk(DISK_FLOPPY, id - iDriveFloppy, DI_READONLY);

        if (vpdi == NULL)
            return 0;

        vcFind = CntReadDiskDirectory(vpdi, szPath, &vrgpfd);
        return vcFind;
        }

    if (id >= iDriveSCSI)
        {
        long l = id - iDriveSCSI;

        if (vpdi == NULL)
            {
            l = LongFromScsi(vrgszDrives[id][9]-'0',vrgszDrives[id][15]-'0');
            vpdi = PdiOpenDisk(DISK_SCSI, l, DI_READONLY);
            }

        if (vpdi == NULL)
            return 0;

        vcFind = CntReadDiskDirectory(vpdi, szPath, &vrgpfd);
        return vcFind;
        }

    strcpy(rgch, vrgszDrives[id]);
    strcat(rgch, szPath);

	if (rgch[strlen (rgch) - 1] != '\\')
			strcat (rgch, "\\");
    strcat(rgch, "*.*");

    emOld = SetErrorMode(SEM_FAILCRITICALERRORS);
    if ((vhFind = FindFirstFile(rgch, &vrgpfd[0])) == INVALID_HANDLE_VALUE)
        {
        SetErrorMode(emOld);
        return 0;
        }

    SetErrorMode(emOld);

    do
        {
        if(strcmp(vrgpfd[vcFind].cFileName, ".") == 0)
			continue;
		
//		printf("found file: %s\n", vrgpfd[vcFind].cFileName);

		{
		FILETIME ft = vrgpfd[vcFind].ftLastWriteTime;

		FileTimeToLocalFileTime(&ft, &vrgpfd[vcFind].ftLastWriteTime);
		}

        vcFind++;
#if USEHEAP
        vrgpfd = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            vrgpfd, sizeof(WIN32_FIND_DATA) * (vcFind+1));
#else
        vrgpfd = realloc(vrgpfd, sizeof(WIN32_FIND_DATA) * (vcFind+1));
#endif
        } while (vhFind && FindNextFile(vhFind, &vrgpfd[vcFind]));

    FindClose(vhFind);

    return vcFind;
}

BOOL GetIthDirectory(int iFile, char *pch, int cch, BOOL *pfDir)
{
    if (iFile >= vcFind || iFile < 0)
        return FALSE;

    if ((int)strlen(vrgpfd[iFile].cFileName) > cch)
        return FALSE;

    *pfDir = (vrgpfd[iFile].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    strcpy(pch, vrgpfd[iFile].cFileName);

	if (*pfDir)
        {
        if (vpdi && vpdi->fst == FS_HFS)
            strcat(pch, ":");
        else
            strcat(pch, "\\");
        }
    
    return TRUE;
}

BOOL GetIthDirectoryInfo(int iFile, long *plSize, FILETIME *pft,
 char *szType, char *szCreator)
{
    if (iFile >= vcFind || iFile < 0)
        return FALSE;

    *plSize = vrgpfd[iFile].nFileSizeLow;
    *pft = vrgpfd[iFile].ftLastWriteTime;

    szType[0] = 0;
    szCreator[0] = 0;

    if (vrgpfd[iFile].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		*plSize = -1;

    if (vpdi->fst == FS_HFS)
        {
        szType[4] = 0;
        szCreator[4] = 0;
        }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////

//
// Add the disk image to the path and return the new count of drives.
// If the operation fails, simply return the unchanged count of drives.
//

int AddDiskImage(const char *szPathName)
{
#if USEHEAP
    vrgszDrives = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        vrgszDrives, sizeof(char *) * (vcDrives+1));
    vrgszDrives[vcDrives] = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
#else
    vrgszDrives = realloc(vrgszDrives, sizeof(char *) * (vcDrives+1));
    vrgszDrives[vcDrives] = malloc(MAX_PATH);
#endif

	if (((LONG)szPathName) & 0xFFFF0000)
        {
        strcpy(vrgszDrives[vcDrives], szPathName);
        }
    else
        {
        int iFile = (int)szPathName;

        if (iFile >= vcFind || iFile < 0)
            return vcDrives;

		int pch;
		int iMac = CbReadFileContents(vpdi, &pch, &vrgpfd[iFile]);

        strcpy(vrgszDrives[vcDrives], vpdi->szCwd);
        strcat(vrgszDrives[vcDrives], vrgpfd[iFile].cFileName);
        }

    // test the disk image for validity here

    if (vpdi->dt == DISK_WIN32)
        {
        // then increment vcDrives if it's good

        PDI pdiT = PdiOpenDisk(DISK_IMAGE, (long)vrgszDrives[vcDrives], DI_READONLY);

        if (pdiT)
            {
            CloseDiskPdi(pdiT);

            if (pdiT->fst != FS_RAW)
            vcDrives++;
            }
        }

    return vcDrives;
}

BOOL ExtractFile(const char *szFile, const char *szDestFile)
{
    return TRUE;
}

////////////////////////////////////////////////////////////////////////

BOOL ShowFileProperties(const char *szFile)
{
    char sz[512], szT[256];
	char szDate[256], szTime[256];
	SYSTEMTIME st;
    int iFile;

    // Files are sorted in the display windows, so we need to do a name match

    for (iFile = 0; iFile < vcFind; iFile++)
        {
        if (!strcmp(szFile, vrgpfd[iFile].cFileName))
            break;
        }

    wsprintf(sz, "File: %s\n\n", szFile);

    wsprintf(szT, "Size: %d bytes\n\n", vrgpfd[iFile].nFileSizeLow);
    strcat(sz, szT);

    FileTimeToSystemTime(&vrgpfd[iFile].ftLastWriteTime, &st);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szDate, sizeof(szDate));
    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, szTime, sizeof(szTime));
    strcat(szDate, " ");
    strcat(szDate, szTime);
    wsprintf(szT, "Modified: %s\n\n", szDate);
    strcat(sz, szT);

#if 0
	if (vpdi->fst == FS_HFS)
        {
        wsprintf(szT, "Type: %4s\n", &vrgpfd[iFile].ftCreationTime);
        szT[10] = '\n';
        szT[11] = '\0';
        strcat(sz, szT);
#endif

        wsprintf(szT, "Creator: %4s\n", (BYTE *)(&vrgpfd[iFile].ftCreationTime) + 4);
        szT[13] = '\n';
        szT[14] = '\0';
        strcat(sz, szT);
#if 0
	}
#endif

    MessageBox(NULL, sz, "File Properties", MB_OK);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////

static int mpgdi[100000];

BOOL AddDragDescriptor(int iFile, HGLOBAL *phFileGroupDescriptor)
{
    HGLOBAL hgNew;
	FILEGROUPDESCRIPTOR *pfgd;
	int i;
    int cItems;

	pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(*phFileGroupDescriptor);
    cItems = pfgd->cItems; 
	GlobalUnlock(*phFileGroupDescriptor);

	hgNew = GlobalReAlloc(*phFileGroupDescriptor, sizeof(FILEGROUPDESCRIPTOR) + cItems * sizeof(FILEDESCRIPTOR), GMEM_MOVEABLE | GMEM_ZEROINIT);
	if (hgNew == NULL)
		return FALSE;

	*phFileGroupDescriptor = hgNew;
	pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(*phFileGroupDescriptor);
	
	i = pfgd->cItems;
    pfgd->fgd[i].dwFlags = FD_ATTRIBUTES|FD_WRITESTIME|FD_FILESIZE;
    pfgd->fgd[i].dwFileAttributes = vrgpfd[iFile].dwFileAttributes;
	pfgd->fgd[i].ftLastWriteTime  = vrgpfd[iFile].ftLastWriteTime;
    pfgd->fgd[i].nFileSizeHigh    = 0;
    pfgd->fgd[i].nFileSizeLow     = vrgpfd[iFile].nFileSizeHigh ?
                                    vrgpfd[iFile].nFileSizeHigh :
                                    vrgpfd[iFile].nFileSizeLow;
    lstrcpy(pfgd->fgd[i].cFileName, vrgpfd[iFile].cFileName);
	pfgd->cItems++;

    mpgdi[i] = iFile;

	GlobalUnlock(*phFileGroupDescriptor);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////

HGLOBAL DropFileContents(int iFileGroupDescriptor)
{
	HGLOBAL hg;
    char *pch;
    int iFile = mpgdi[iFileGroupDescriptor];

	hg = GlobalAlloc(GMEM_MOVEABLE, vrgpfd[iFile].nFileSizeLow + 65536);
	if (hg == NULL)
		return NULL;

	pch = (char*)GlobalLock(hg);
	memset(pch, '-', vrgpfd[iFile].nFileSizeLow);
    CbReadFileContents(vpdi, pch, &vrgpfd[iFile]);
    GlobalUnlock(hg);
	hg = GlobalReAlloc(hg, vrgpfd[iFile].nFileSizeLow, GMEM_MOVEABLE);
	return hg;
}


//
// Copy the data stream to the clipboard
//
// Size of data stream is stored in nFileSizeHigh
//

HGLOBAL GetIthFileText(int iFile)
{
	HGLOBAL hg;
    char *pch;
    int i, iMac;

	hg = GlobalAlloc(GMEM_MOVEABLE, vrgpfd[iFile].nFileSizeHigh + 65536);
	if (hg == NULL)
		return NULL;

	pch = (char*)GlobalLock(hg);
	memset(pch, '+', vrgpfd[iFile].nFileSizeHigh);
    iMac = CbReadFileContents(vpdi, pch, &vrgpfd[iFile]);

    iMac = min(iMac, (int)vrgpfd[iFile].nFileSizeHigh);

    // convert CR into CR/LF, growing buffer as you go

    for (i = 0; i < iMac; i++)
        {
        if ((vpdi->fst == FS_HFS) && (pch[i] == 0x0D))
            {
            memmove(pch+i+2, pch+i+1, iMac-i-1);
            pch[++i] = 0x0A;
            iMac++;
            }

        else if (((vpdi->fst == FS_MYDOS) || (vpdi->fst == FS_ATARIDOS)) && (pch[i] == 155))
            {
            pch[i++] = 0x0D;
            pch[i] = 0x0A;
            iMac++;
            }
        }

    pch[iMac++] = 0;    // seems to need a trailing 0

    GlobalUnlock(hg);

	hg = GlobalReAlloc(hg, iMac, GMEM_MOVEABLE);
	return hg;
}

////////////////////////////////////////////////////////////////////////

BOOL IsBlockDevice(int id)
{
#if 0
    if (id >= vcDrives || id < 0)
        return FALSE;
#endif

    if (vpdi == NULL)
        return FALSE;

    if (vpdi->dt == DISK_FLOPPY)
        return TRUE;

    if (vpdi->dt == DISK_SCSI)
        return TRUE;

    return FALSE;
}


BOOL CopyDiskImage(HWND hwnd, BOOL fWrite, int id, PDI pdiFile, BOOL fWholeDisk)
{
    PDI pdi;
    int sec, secMac;
    BYTE rgb[32768];
    BYTE rgch0[256], rgch[256];
    int dsec = sizeof(rgb)/512;

    // HACK: assume id is the current drive

    id = vid;

    if (!IsBlockDevice(id))
        return FALSE;

    GetWindowText(hwnd, rgch0, sizeof(rgch0));

    if (fWrite)
        {
        // Writing a disk image.
#if 0
        //  - Need to close current PDI
        //  - Re-open the disk for writing
        //  - Write the image to the disk
        //  - Close the PDI
        //  - Re-open the disk read-only and re-read root directory
        //    since the contents of the disk have changed!

        if (vpdi)
            CloseDiskPdi(vpdi);

        vpdi = NULL;
#endif

        if (id >= iDriveFloppy)
            {
            pdi = PdiOpenDisk(DISK_FLOPPY, id - iDriveFloppy, DI_QUERY);
            }
        else if (id >= iDriveSCSI)
            {
            long l = id - iDriveSCSI;

            l = LongFromScsi(vrgszDrives[id][9]-'0',vrgszDrives[id][15]-'0');
            pdi = PdiOpenDisk(DISK_SCSI, l, DI_QUERY);
            }

        if (pdi == NULL)
            return FALSE;
        }
    else if (fWholeDisk)
        {
        if (id >= iDriveFloppy)
            {
            pdi = PdiOpenDisk(DISK_FLOPPY, id - iDriveFloppy, DI_READONLY | DI_QUERY);
            }
        else if (id >= iDriveSCSI)
            {
            long l = id - iDriveSCSI;

            l = LongFromScsi(vrgszDrives[id][9]-'0',vrgszDrives[id][15]-'0');
            pdi = PdiOpenDisk(DISK_SCSI, l, DI_READONLY | DI_QUERY);
            }

        if (pdi == NULL)
            return FALSE;
        }
    else
        {
        // reading disk image, use the current PDI for now

        pdi = vpdi;
        }

    // convert disk size in K to # of 512-byte sectors

    secMac = pdi->size * 2;

    if (secMac == 0)
        {
        if (fWrite)
            secMac = GetFileSize(pdiFile->h, NULL) / 512;
        else if (pdi->dt == DISK_FLOPPY)
            secMac = 2880;
        else
            secMac = 0x7FFFFFFF;
        }

    if (pdi->dt == DISK_FLOPPY)
        dsec = 9;

    StartProgress(fWrite ? "Writing disk image to physical disk" : \
                 (fWholeDisk ? "Reading physical disk to disk image" : \
                                "Reading logical disk to disk image"));

    for (sec = 0; sec < secMac; sec+=dsec)
        {
#if 0
        // non-progress bar method

        if (fWrite)
            wsprintf(rgch, "Writing disk image to physical disk sector %d (%02d%%)", sec, (100 * sec) / secMac);
        else
            wsprintf(rgch, "Reading physical disk sector %d to disk image", sec);

        if (hwnd)
            SetWindowText(hwnd, rgch);

        Sleep(1);
#else
        // progress bar method

        wsprintf(rgch, "Disk sector %d of %d\n%dK copied (%d%%)", sec, secMac,
            sec/2, (100 * sec) / secMac);

        if (!SetProgress((100 * sec) / secMac, rgch))
            break;
#endif

        dsec = min(dsec, secMac - sec);

Lretry:
        if (dsec < 1)
            break;

        pdi->count = dsec;
        pdi->sec   = sec;
        pdi->lpBuf = rgb;

        pdiFile->count = dsec;
        pdiFile->sec   = sec;
        pdiFile->lpBuf = rgb;

        if (fWrite && !FRawDiskRWPdi(pdiFile, !fWrite))
            {
Lerror:
            dsec--;
            goto Lretry;
            }

        if (!FRawDiskRWPdi(pdi, fWrite))
            goto Lerror;

        if (!fWrite && !FRawDiskRWPdi(pdiFile, !fWrite))
            goto Lerror;
        }

    if (fWrite)
        CloseDiskPdi(pdi);

    EndProgress();

    SetWindowText(hwnd, rgch0);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS

void main()
{
    int c, i;

    printf("count of drives = %d\n", c = EnumeratePhysicalDrives());

    for (i = 0; i < c; i++)
        {
        char rgch[MAX_PATH];
        int cFiles, iFile;

        GetIthPhysicalDrive(i, rgch, sizeof(rgch));
        printf("%dth drive name = %s\n", i, rgch);

        printf("count of files in root = %d\n",
            cFiles = EnumerateDirectory(i, ""));

        for (iFile = 0; iFile < cFiles; iFile++)
            {
            char rgch2[MAX_PATH];
            BOOL fDir;

            GetIthDirectory(iFile, rgch2, sizeof(rgch2), &fDir);
            printf("%dth file name = %s %s\n", iFile, rgch2, fDir ? "<DIR>" : "");
            }
        }
}

#endif

