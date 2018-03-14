
/***************************************************************************

    MAKEDSK.C

    - Make a blank disk image file

    Copyright (C) 1998-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    05/05/2000  darekm

***************************************************************************/

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP

#undef  _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

#define WindowsSDKDesktopARM64Support 1

#include <windows.h>
#include <winnt.h>
#include <excpt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <dos.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>

void __inline PutByte(BYTE *pb, ULONG ofs, BYTE b)
{
    pb[ofs] = b;
}

void __inline PutWord(BYTE *pb, ULONG ofs, WORD w)
{
    PutByte(pb, ofs, (BYTE)(w >> 8));
    PutByte(pb, ofs+1, (BYTE)w);
}

void __inline PutLong(BYTE *pb, ULONG ofs, ULONG l)
{
    PutByte(pb, ofs,   (BYTE)(l >> 24));
    PutByte(pb, ofs+1, (BYTE)(l >> 16));
    PutByte(pb, ofs+2, (BYTE)(l >> 8));
    PutByte(pb, ofs+3, (BYTE)l);
}

void __stdcall CreateHFSBootBlocks(BYTE *pb, ULONG l, char *szVol)
{
    // Format the disk image of size l KB in Macintosh HFS format

    // Total number of sectors on disk in kilobytes * 2
    ULONG cSecsDisk = l * 2;

    // Allocation size is multiple of 512
    ULONG cbAlloc = 512 * (((l - 1) / 32768) + 1);

    // Total allocatable sectors is decreased by 6..21 sectors
    ULONG cSecsVolBmpT =
        1 + ((cSecsDisk - 21) / 4096) / (cbAlloc / 512);

    // Maximum number of allocation blocks
    ULONG cBlkAlloc = (cSecsDisk - 5 - cSecsVolBmpT) / (cbAlloc / 512);

    // Total allocatable sectors is decreased by 6..21 sectors
    ULONG cSecsVolBmp =
        1 + ((cBlkAlloc - 1) / 4096); // 4096 bits per 512-byte sector

    // Size in bytes of the catalog and extent files (each)
    // The size of each must be a multiple of cbAlloc
    // and it is equal to about 1/128th the size of the disk
    ULONG cbSpecial = min(1024*1024,
         ((l * 1024) / 128) - (((l * 1024) / 128) % cbAlloc));

    // Total allocatable sectors is decreased by 6..21 sectors
    // (2 for boot, 1 for MDB, 1..16 for volume bitmap, 2 unused at end)
    ULONG cSecsData = cSecsDisk - 5 - cSecsVolBmp;

    // used in variable size node fields
    ULONG cbVol = strlen(szVol);
    ULONG cbVolE = (cbVol + 0) & ~1;

    // Set the creation time to the current time

    FILETIME ft, ft0;
    SYSTEMTIME st;
    __int64 llTime;

    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ft);

    memset(&st, 0, sizeof(st));
    st.wYear = 1904;
    st.wDay = 1;
    st.wMonth = 1;
    SystemTimeToFileTime(&st, &ft0);

    llTime = *(__int64 *)&ft / 10000000;
    llTime -= *(__int64 *)&ft0 / 10000000;

#if 0

    printf("sectors on disk = %d\n", cSecsDisk);
    printf("allocation size = %d\n", cbAlloc);
    printf("allocation units = %d\n", cBlkAlloc);
    printf("size of vol bmp = %d\n", cSecsVolBmp);
    printf("size of special = %d\n", cbSpecial);
    printf("sectors in data = %d\n", cSecsData);

#endif

    // set Master Dir Block

    PutWord(pb, 1024 + 0,   0x4244);
    PutLong(pb, 1024 + 2,   (ULONG)llTime);
    PutLong(pb, 1024 + 6,   (ULONG)llTime);
    PutWord(pb, 1024 + 10,  0x0100);
    PutWord(pb, 1024 + 14,  0x0003);
    PutWord(pb, 1024 + 18,  cBlkAlloc);
    PutLong(pb, 1024 + 20,  cbAlloc);
    PutLong(pb, 1024 + 24,  cbAlloc * 4);
    PutWord(pb, 1024 + 28,  3 + cSecsVolBmp);
    PutLong(pb, 1024 + 30,  16);
    PutWord(pb, 1024 + 34,  cBlkAlloc - 2*cbSpecial/cbAlloc);
    PutByte(pb, 1024 + 36,  cbVol);
    strcpy(&pb[1024 + 37], szVol);
    PutLong(pb, 1024 + 70,  4);
    PutLong(pb, 1024 + 74,  cbSpecial);
    PutLong(pb, 1024 + 78,  cbSpecial);
    PutLong(pb, 1024 + 130, cbSpecial);
    PutWord(pb, 1024 + 136, cbSpecial/cbAlloc);
    PutLong(pb, 1024 + 146, cbSpecial);
    PutWord(pb, 1024 + 150, cbSpecial/cbAlloc);
    PutWord(pb, 1024 + 152, cbSpecial/cbAlloc);

    // set volume bitmap

    {
    ULONG t = 2*cbSpecial/cbAlloc;

    memset(&pb[1536], 0xFF, t/8);
    if (t & 7)
        PutByte(pb, 1536 + t/8, (0xFF << (8 - (t & 7))));
    }

    // set Catalog File root node

    PutByte(pb, (3 + cSecsVolBmp) * 512 + 8,    1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + 11,   3);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + 32,   2);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + 35,   7);
    PutLong(pb, (3 + cSecsVolBmp) * 512 + 36,   min(2048, (cbSpecial/512)));
    PutLong(pb, (3 + cSecsVolBmp) * 512 + 40,   min(2048, (cbSpecial/512)) - 1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + 248,  0x80);
    PutLong(pb, (3 + cSecsVolBmp) * 512 + 504,  0x01F800F8);
    PutLong(pb, (3 + cSecsVolBmp) * 512 + 508,  0x0078000E);

    // set Extents File root node

    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 8,    1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 11,   3);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 15,   1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 19,   1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 23,   2);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 27,   1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 31,   1);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 32,   2);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 35,   37);
    PutLong(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 36,   min(2048, (cbSpecial/512)));
    PutLong(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 40,   min(2048, (cbSpecial/512)) - 2);
    PutByte(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 248,  0xC0);
    PutLong(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 504,  0x01F800F8);
    PutLong(pb, (3 + cSecsVolBmp) * 512 + cbSpecial + 508,  0x0078000E);

    // set Extents File next node (contains volume name)

    PutLong(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 8,    0xFF010002);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 14,   7 + cbVolE);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 19,   1);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 20,   cbVol);
    strcpy(&pb[(4 + cSecsVolBmp) * 512 + cbSpecial + 21], szVol);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 22 + cbVolE, 1);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 31 + cbVolE, 2);
    PutLong(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 32 + cbVolE, (ULONG)llTime);
    PutLong(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 36 + cbVolE, (ULONG)llTime);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 92 + cbVolE, 7);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 97 + cbVolE, 2);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 100 + cbVolE, 3);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 113 + cbVolE, 1);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 114 + cbVolE, cbVol);
    strcpy(&pb[(4 + cSecsVolBmp) * 512 + cbSpecial + 115 + cbVolE], szVol);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 507,  0x92 + cbVolE);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 509,  0x5C + cbVolE);
    PutByte(pb, (4 + cSecsVolBmp) * 512 + cbSpecial + 511,  0x0E);
}

#if defined(MAKEIMG) || defined(MAKEDSK)

void main(int argc, char **argv)
{
    ULONG l = 0;
    HANDLE h;
    OFSTRUCT ofs;
    BYTE *rgb;
    BYTE sz[256];     // used for filename buffer and pause buffer
    BYTE szNum[256];  // used for file size buffer
    const cb = 1440*1024;

    printf("\n"
#ifdef MAKEIMG
    "MAKEIMG Macintosh .IMG/.HFX Disk Image Creation Utility.\n\n"
    "Creates pre-formatted Macintosh disk images up to 2GB in size.\n"
#else
    "MAKEDSK Unformatted .DSK Disk Image Creation Utility.\n\n"
    "Creates unformatted blank disk images up to 2GB in size.\n"
#endif
    "Programming by Darek Mihocka. Copyright (C) 2000 by Emulators Inc.\n\n");
#ifdef MAKEIMG
    printf("Usage: MAKEIMG <image file name> <disk image size in K>\n"
#else
    printf("Usage: MAKEDSK <image file name> <disk image size in K>\n"
#endif
    "Specify disk image size in kilobytes. e.g. 1.44MB = 1440, 2GB = 2000000.\n");

    if (argc == 1)
        {
        printf("\nEnter the name of the blank disk image to create:\n");
        sz[0] = 0;
        sz[1] = 0;
        sz[2] = 0;
        gets((void *)sz);
        }
    else
        strcpy(sz, argv[1]);

    if (argc < 3)
        {
        printf("\nEnter the disk image size in kilobytes:\n");
        szNum[0] = 0;
        szNum[1] = 0;
        szNum[2] = 0;
        gets((void *)szNum);
        sscanf(&szNum, "%d", &l);
        }
    else
        sscanf(&argv[2][0], "%d", &l);

        {
        if (l < 90)
            {
            printf("Size too small! Disk image must be at least 90K!\n");
            goto Lexit;
            }

        if (l >= 2048 * 1024)
            {
            printf("Size too large! Disk image must be smaller than 2 gig!\n");
            goto Lexit;
            }

        printf("Creating a disk image %dK in size...\n", l);

        // the buffer needs to be the size of a 1.44M floppy disk
        // which is also big enough to handle the boot blocks
        // of a 2 gig partition

        rgb = VirtualAlloc(NULL, cb, MEM_COMMIT, PAGE_READWRITE);

#ifdef MAKEIMG
        CreateHFSBootBlocks(rgb, l, sz);
#endif

        if (((HFILE)h = OpenFile(sz, &ofs, OF_CREATE)) != HFILE_ERROR)
            {
            int cbRead;
            int cbT = min(cb, l*1024);

            // Write the first (possibly non-zero 64K)

            WriteFile(h, rgb, cbT, &cbRead, NULL);

            l -= cbT/1024;

            // Now zero out the buffer for the rest of the image

            memset(rgb, 0, cb);

            while ((l > (cb/1024)) && (l -= (cb/1024)))
                WriteFile(h, rgb, cb, &cbRead, NULL);

            while (l--)
                WriteFile(h, rgb, 1024, &cbRead, NULL);

            CloseHandle(h);
            }

        VirtualFree(rgb, 0, MEM_RELEASE);
        }

Lexit:
    printf("\nPress any key...\n");
    gets((void *)sz);
}

#endif
