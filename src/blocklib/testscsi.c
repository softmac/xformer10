
/****************************************************************************

    TESTSCSI.C

    Tests the SCSI support in the blockdev library

    Copyright (C) 1998-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    01/18/1998  darekm

****************************************************************************/

#include "precomp.h"

int adapter_id = 0, target_id = 0, sector = 0, count = 1;

int main(int argc, char **argv)
{
    BOOL   fWriteImg = FALSE;
    HANDLE h;
    OFSTRUCT ofs;
    int cbRead;

    if (argc >= 4)
        {
Lbozo:
        printf("usage: READSCSI [imagefile] [SCSIbu]\n");
        return -1;
        }

    if ((argc == 3) && !strnicmp("scsi", argv[2], 4))
        {
        adapter_id = argv[2][4]-'0';
        target_id =  argv[2][5]-'0';
        }

    if (argc >= 2)
        {
        h = OpenFile(argv[1], &ofs, OF_CREATE);
        fWriteImg = TRUE;
        }

    if (!FInitBlockDevLib())
        return -1;

 do
  {
    unsigned char buf[8192];
    unsigned char bufID[256];
    unsigned int cSecMax, cbSec;
  //  int cb;
    BOOL f;

    printf("scanning adapter %d target %d: ", adapter_id, target_id);

    f = scsiResetDevice(adapter_id, target_id);

    Sleep(300);

    if (scsiInquiry(adapter_id, target_id, bufID, 32) || f)
        {
        bufID[40] = 0;
        printf("%-32s\n", &bufID[8]);

        printf("Device type = %02X\n", bufID[0]);

        if (scsiTestUnitReady(adapter_id, target_id, buf, 255))
            printf("Test Unit Read PASSED\n");
        else
            printf("Test Unit Read FAILED\n");

        scsiGetDiskInfo(adapter_id, target_id, buf, 255);

        printf("Got Disk Info\n");

        if (1 && scsiReadCapacity(adapter_id, target_id, buf, 8))
            {
            cSecMax = SwapL(*(int *)buf) + 1;
            cbSec = SwapL(*(int *)&buf[4]);

            printf("CAPACITY = %d blocks\n", cSecMax);
            printf("BLOCK SIZE = %d\n", cbSec);
            }
        else
            {
            cSecMax = 1;
            cbSec = 512;

            printf("CAPACITY FAILED (disc ejected?)\n");
            }

        printf("reading TOC\n");

        if (1 && scsiReadTOC(adapter_id, target_id, buf, 2048))
            {
            int i;

            printf("TOC READ!\n");

            for (i = 0; i < 44; i++)
                printf("%02X\n", buf[i]);
            }

        printf("getting mode sense\n");

        for (count = 0; count < 256; count++)
            {
            if (1 && scsiModeSense(adapter_id, target_id, buf, 255, count))
                {
                int i;

                printf("MODE SENSED PAGE %d!\n", count);

                for (i = 0; i < 255; i++)
                    printf("%02X\n", buf[i]);
                }
            }

        printf("starting block read)\n");

        sector = 0;
        count = 1;

        while (1 && scsiReadSector(adapter_id, target_id, sector, count, buf, cbSec))
            {
            int ib;

            if (fWriteImg)
                {
                WriteFile(h, buf, cbSec*count, &cbRead, NULL);
                sector += count;
                continue;
                }

          if ((sector % 1   ) == 0)
            printf("%s sector %d read successfully!!!\n",
                &bufID[8], sector);

            for (ib = 0; ib < 1*16/*cbSec*/; ib++)
                {
                if ((ib & 15) == 0)
                    printf("%08X: ", sector * cbSec + ib);

                printf("%02X ", buf[ib]);

                if ((ib & 15) == 15)
                    printf("\n");
                }

            sector+=1000;

            if (sector > 1 )
                break;
            }

        printf("%s contains %d sectors!\n",
            &bufID[8], sector);

        if (fWriteImg)
            break;
        }
    else
        printf("not present\n");

Lnext:
    target_id = (target_id + 1) & 15; // Move to next SCSI ID#

    if (target_id == 0)
        {
        adapter_id++;
        if (adapter_id >= (BYTE)NumAdapters)
            {
            adapter_id = 0; 
            sector++;
            }
        }

  } while (argc < 2);

  CloseHandle(h);
}


