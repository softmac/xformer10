
/****************************************************************************

    TESTMAC.C

    Tests the blockdev library by doing a disk directory of a Mac disk

    Usage: TESTMAC [device [W]]

    If device is blank, use the floppy disk by default. Otherwise device
    specifies a disk image or SCSI device in the form SCSIhi

    W specifies to do a 256 sector write test (dangerous!!)

    Copyright (C) 1998-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    10/10/01    darekm

****************************************************************************/

#include "precomp.h"

void main(int argc, char *argv[])
{
    DISKINFO *pdi;
    BOOL fWrite;
    ULONG rgf;

    fWrite = (argc > 2 && argv[2][0] == 'W');
    rgf    = fWrite ? DI_QUERY : DI_READONLY;

//  __asm int 3;

    FInitBlockDevLib();

    if (argc > 1)
        {
        if (!strnicmp("scsi", argv[1], 4))
            {
            pdi = PdiOpenDisk(DISK_SCSI,
                 LongFromScsi(argv[1][4]-'0',argv[1][5]-'0'),
                 rgf);
            }
        else if (!strnicmp("disk", argv[1], 4))
            {
            pdi = PdiOpenDisk(DISK_WIN32, (argv[1][4]-'a') & 31, rgf);
            }
        else
            pdi = PdiOpenDisk(DISK_IMAGE, argv[1], rgf);
        }
    else
        pdi = PdiOpenDisk(DISK_FLOPPY, 0, rgf);

    if (pdi == NULL)
        {
        printf("error opening file or device\n");
        return;
        }

    if (fWrite)
        {
        int  sec;
        unsigned char rgch[4096];
        BOOL f;

        printf("writing test pattern to the disk...\n");

        for (sec = 0; sec < 256; sec++)
            {
            printf("writing %02X to sector %d... ", sec, sec);
            memset (rgch, (char)sec, sizeof(rgch));

            pdi->sec = sec;
            pdi->count = 1;
            pdi->lpBuf = rgch;

            f = FRawDiskRWPdi(pdi, 1);

            printf("result = %d\n", f);
            }
        }
    else
        {
        int  sec;
        unsigned char rgch[4096];
        BOOL f;

        printf("reading disk...\n");

        for (sec = 0; sec < 16; sec++)
            {
            printf("reading sector %d...\n", sec);
            memset (rgch, (char)sec, sizeof(rgch));

            pdi->sec = sec;
            pdi->count = 1;
            pdi->lpBuf = rgch;

            f = FRawDiskRWPdi(pdi, 0);

            printf("result = %d, data = %02X %02X %02X %02X\n", f,
                rgch[0], rgch[1], rgch[2], rgch[3]);
            }

        CntReadDiskDirectory(pdi, "", NULL);
        }
}

