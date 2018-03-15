
/***************************************************************************

	MACDISK.C

	- low level NCR 5380 SCSI interface emulation for Mac
	- low level IWM floppy disk interface emulation for Mac

	04/16/2002

***************************************************************************/


#include "gemtypes.h"

#ifdef SOFTMAC

#ifndef NDEBUG

#define  TRACEHW   0
#define  TRACEDISK 0
#define  TRACESCSI 0
#define  TRACEIWM  0

#else  // NDEBUG

#define  TRACEHW   0
#define  TRACEDISK 0
#define  TRACESCSI 0
#define  TRACEIWM  0

#endif // NDEBUG

// #pragma optimize("",off)

#if TRACESCSI
int fTracing;
#endif

//
// LMacTimeFromFt
//
// Convert the Windows FILETIME to a Macintosh 32-bit count of seconds
// since 1/1/1904
//
// Used by Macintosh file timestamps and by the RTC
//

ULONG LMacTimeFromFt(FILETIME ft)
{
    FILETIME ft0, ft1;
    SYSTEMTIME st;
    __int64 llTime;

    FileTimeToLocalFileTime(&ft, &ft1);

    memset(&st, 0, sizeof(st));
    st.wYear = 1904;
    st.wDay = 1;
    st.wMonth = 1;
    SystemTimeToFileTime(&st, &ft0);

#ifdef GEMPRO
#ifndef DEMO
#ifndef BETA

    // starting with the Jan 27 2000 build, we check the first two
    // characters of the "registered to" string for an expiration
    // date. The first decoded character is a lowercase letter
    // which is the expiration month. a = Jan, b = Feb, c = Mar.
    // The second character is an expiration year, 0 = 2000, 1 = 2001,
    // etc. If the current date is past the expiration date, we
    // cripple the current Mac VM's memory, screen, and sound settings
    // just as we do in mac_Init. The next time the user reboots the
    // VM, they'll be bumped down to the crappy settings.

    vpes->valid1[0] *= ((vpes->field1[0] >= (st.wMonth - 1 + 'a'))) ||
                       ((vpes->field1[1] >  (st.wYear - 2000 + '0')));
    vpes->valid2[0] *= ((vpes->field1[1] >= (st.wYear - 2000 + '0')));

#endif // !BETA
#endif // !DEMO
#endif // GEMPRO

    llTime = *(__int64 *)&ft1 / 10000000;
    llTime -= *(__int64 *)&ft0 / 10000000;
    return (ULONG)llTime;
}

ULONG LGetMacTime()
{
    FILETIME ft;
    SYSTEMTIME st;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    return LMacTimeFromFt(ft);
}

//
// Macintosh IWM chip handler.
//
// The IWM responds to odd byte accesses to addresses in the
// $00DFxxFF range, where xx is $E1, $E3, .. $FF
//
// Either a read or write to the location sets an IWM state line,
// this this code is called from both byte read and byte write handlers.
//

void ReadWriteIWM(ULONG ea)
{
    // ea is now $0000 to $1E00 in $0200 increments.
    // Bit 9 determines if a line is being set or cleared
    // The line offset is bits 12..10.

    Assert((ea >> 10) < 8);
    Assert((ea & 0xFF) == 0x00);

    vmachw.rgbIWMCtl[ea >> 10] = (BYTE)((ea >> 9) & 1);

#if TRACEIWM
    m68k_DumpRegs();

    switch (ea)
        {
    default:
        Assert(fFalse);
        break;

    case 0x0000:         // ph0L
        printf("IWM: ph0L       - CA0 = 0\n");
        break;

    case 0x0200:         // ph0H
        printf("IWM: ph0H       - CA0 = 1\n");
        break;

    case 0x0400:         // ph1L
        printf("IWM: ph1L       - CA1 = 0\n");
        break;

    case 0x0600:         // ph1H
        printf("IWM: ph1H       - CA1 = 1\n");
        break;

    case 0x0800:         // ph2L
        printf("IWM: ph2L       - CA2 = 0\n");
        break;

    case 0x0A00:         // ph2H
        printf("IWM: ph2H       - CA2 = 1\n");
        break;

    case 0x0C00:         // ph3L
        printf("IWM: ph3L       - LSTRB = 0\n");
        break;

    case 0x0E00:         // ph3H
        printf("IWM: ph3H       - LSTRB = 1\n");
        break;

    case 0x1000:         // mtrOff
        printf("IWM: mtrOff     - ENABLE = 0\n");
        break;

    case 0x1200:         // mtrOn
        printf("IWM: mtrOn      - ENABLE = 1\n");
        break;

    case 0x1400:         // intDrive
        printf("IWM: intDrive   - SELECT = 0\n");
        break;

    case 0x1600:         // extDrive
        printf("IWM: extDrive   - SELECT = 1\n");
        break;

    case 0x1800:         // q6L
        printf("IWM: q6L        - Q6 = 0\n");
        break;

    case 0x1A00:         // q6H
        printf("IWM: q6H        - Q6 = 1\n");
        break;

    case 0x1C00:         // q7L
        printf("IWM: q7L        - Q7 = 0\n");
        break;

    case 0x1E00:         // q7H
        printf("IWM: q7H        - Q7 = 1\n");
        break;
        }

    printf("CA2 CA1 CA0 SEL = %d %d %d %d\n",
        vmachw.CA2,
        vmachw.CA1,
        vmachw.CA0,
        (vmachw.rgvia[0].vBufA & 0x20) ? 1 : 0);
#endif

    return;
}


void HandleSCSICmd(int id)
{
    // Check that the SCSI device is mapped to something

    PDI pdi = vi.rgpdi[2 + id];

    vi.cSCSIRead = 0;
    vi.iSCSIRead = 0;

    if (pdi == NULL)
        goto badHD;

#if TRACESCSI
    printf("SCSI command on id %d = %02X %02X %02X %02X %02X %02X\n",
        id,
        vsthw.byte0,
        vsthw.byte1,
        vsthw.byte2,
        vsthw.byte3,
        vsthw.byte4,
        vsthw.byte5);
#endif

    switch (vsthw.byte0)
        {
    default:
        // unknown command, pretend it's good

#if TRACESCSI
        printf("Unknown SCSI command %02X\n", vsthw.byte0);
#endif

        goto goodHD;
        goto badHD;

    case 0x00:
        // test unit ready

#if TRACESCSI
        printf("SCSI command $00 TEST UNIT READY\n");
#endif

goodHD:
        vsthw.cmdreg = 0;
        vsthw.idiskreg = 0;
        vsthw.dmastat = 0; // DMA OK
        break;

badHD:
        vsthw.cmdreg = 0xFF;
        vsthw.idiskreg = 0;
        vsthw.dmastat = 1;

        vi.cSCSIRead = 0;
        vi.iSCSIRead = 0;

        break;

    case 0x01:
        // restore

        goto goodHD;

    case 0x03:
        // request sense - return 4 zeroes in buffer

        vi.pvSCSIData[0] = 0;
        vi.pvSCSIData[1] = 0;
        vi.pvSCSIData[2] = 0x06;
        vi.pvSCSIData[3] = 0;

        vi.cSCSIRead = 4;
        vi.iSCSIRead = 0;

#if TRACESCSI
        fTracing = 0;

        printf("SCSI command $03 REQUEST SENSE\n");
#endif

        goto goodHD;

#if 0
    case 0x05:
        // read block limits - return some valid data

        vi.pvSCSIData[0] = 0;
        vi.pvSCSIData[1] = 0;
        vi.pvSCSIData[2] = 0xFF;
        vi.pvSCSIData[3] = 0xFF;
        vi.pvSCSIData[4] = 0;
        vi.pvSCSIData[5] = 1;

        vi.cSCSIRead = 6;
        vi.iSCSIRead = 0;

        goto goodHD;
#endif

    case 0x08:
    case 0x28:
    case 0x0A:
    case 0x2A:
        // read sector
        // $08 is the 6 byte version
        // $28 is the 10 byte version
        // write sector
        // $0A is the 6 byte version
        // $2A is the 10 byte version

        {
        // SCSI read sector

        BOOL fWrite = (vsthw.byte0 & 2);
        ULONG sector = ((vsthw.byte1 & 0x1F) << 16) |
            ((vsthw.byte2 & 0xFF) << 8) | (vsthw.byte3 & 0xFF);

        ULONG count = vsthw.byte4;

        if (pdi->fEjected)
            goto Lbadread;

        if (vsthw.byte0 & 0x20)
            {
            sector = ((vsthw.byte2 & 0xFF) << 24) |
            ((vsthw.byte3 & 0xFF) << 16) |
            ((vsthw.byte4 & 0xFF) << 8) | (vsthw.byte5 & 0xFF);

            count = ((vsthw.byte7 & 0xFF) << 8) | (vsthw.byte8 & 0xFF);
            }

#if TRACESCSI
        printf("%s sector %d, count %d\n", fWrite ? "writing" : "reading", sector, count);
        printf("cbSector = %d\n", pdi->cbSector);
#endif

        vi.cSCSIRead = 0;
        vi.iSCSIRead = 0;

        if (pdi->cbSector > 512)
            {
            // Adjust the large sectors to small sectors as the disk layer
            // will adjust them right back, for example, CD-ROM

            count = count * (pdi->cbSector / 512);
            sector = sector * (pdi->cbSector / 512);
#if TRACESCSI
            printf("adjusted sector %d, count %d\n", sector, count);
#endif
            }

        pdi->lpBuf = (BYTE *)vi.pvSCSIData;
        pdi->track = 0;
        pdi->side  = 0;
        pdi->sec   = sector;
        pdi->count = count;

        // clear out read buffer

#ifdef DO_WE_REALLY_NEED_TO_CLEAR_THE_BUFFER
        if (!fWrite)
            memset(vi.pvSCSIData, 0, min(vi.cbSCSIAlloc, (count << 9)));
#endif

        if (vi.cbSCSIAlloc < (count << 9))
            {
            if (!VirtualAlloc(vi.pvSCSIData, vi.cbSCSIAlloc = (count << 9), MEM_COMMIT, PAGE_READWRITE))
                {
#if !defined(NDEBUG) || defined(BETA) || TRACESCSI
                printf("SCSI alloc size %08X failed!\n", vi.cbSCSIAlloc);
#endif
                vi.cbSCSIAlloc = 0;
                break;
                }
            }

        // If this is an HFX file or CD-ROM image, adjust the sector offset
        // accordingly and read from the stub file if necessary

        if (pdi->cSecStub && (pdi->sec < pdi->cSecStub))
            {
            if (fWrite)
                goto Lbadread;

            while (pdi->sec < pdi->cSecStub)
                {
                if (pdi->count == 0)
                    goto Lgoodread;

                pdi->pdiStub->lpBuf = pdi->lpBuf;
                pdi->pdiStub->track = 0;
                pdi->pdiStub->side  = 0;
                pdi->pdiStub->sec   = pdi->sec;
                pdi->pdiStub->count = 1;

                if (!FRawDiskRWPdi(pdi->pdiStub, fFalse))
                    goto Lbadread;

                pdi->lpBuf += 512;
                pdi->sec++;
                pdi->count--;
                }

            if (pdi->count == 0)
                goto Lgoodread;
            }

        if (pdi->cSecStub)
            pdi->sec -= pdi->cSecStub;

        if (FRawDiskRWPdi(pdi, fWrite))
            {
Lgoodread:
            vi.cSCSIRead = count << 9;
            vi.iSCSIRead = 0;
            vmachw.sBSR |= 0x08;    // set phase match
#if TRACESCSI
            printf("Successfully %s %d bytes\n", fWrite ? "wrote" : "read", vi.cSCSIRead);
#endif
            goto goodHD;
            }
        else
            {
Lbadread:
            vi.cSCSIRead = 0;
            vi.iSCSIRead = 0;
            vmachw.sBSR &= ~0x08; // clear phase match
            vmachw.sBSR &= ~0x01; // clear ACK
            vmachw.sCSR &= ~0x20; // clear REQ
#if TRACESCSI
            printf("%s failed\n", fWrite ? "Write" : "Read");
#endif
            goto badHD;
            }
        }

    case 0x0B:
        // seek

        goto goodHD;

    case 0x12:
        // inquiry
        //
        // sends 5 byte header, plus 31 or 49 bytes of data
        // first 4 bytes are 0, 5th byte is $1F (31 bytes of stuff to follow)
        // first 3 bytes are 0, 8 bytes of ID, 16 bytes of product ID

#if TRACESCSI
        printf("INQUIRY id:%d cbSec:%d cb:%d\n",
                id, pdi->cbSector, vsthw.byte4);
#endif

    if ((pdi->cbSector == 512) && (pdi->dt == DISK_SCSI))
        if (scsiInquiry(pdi->ctl, pdi->id, vi.pvSCSIData, vsthw.byte4))
            {
            vi.cSCSIRead = vsthw.byte4;
            vi.iSCSIRead = 0;

#if TRACESCSI
            printf("SCSI command $12 INQUIRY succeeded\n");

            {
            int i;

            for (i = 0; i < vsthw.byte4; i++)
                printf("%02X ", vi.pvSCSIData[i]);

            printf("\n");
            }
#endif

            vpregs->D0 = vsthw.byte4;
            goto goodHD;
            }
        else
            {
#if TRACESCSI
            printf("SCSI command $12 INQUIRY failed\n");
#endif
            goto badHD;
            }

        {
        char *pch = "QUANTUM LP80S  9808094943.3         ";
        int i;

        if (pdi->cbSector > 512)
            {
            // CD-ROM

            vi.pvSCSIData[0] = 0x05;
            vi.pvSCSIData[1] = 0x80;

            pch = "MATSHITACD-ROM CR-8004              ";
            }
        else
            {
            // hard disk

            vi.pvSCSIData[0] = 0;
            vi.pvSCSIData[1] = 0;
            }

        if (vsthw.byte4 < 36)
            vsthw.byte4 = 36;

        vi.pvSCSIData[2] = 2; // 1 = SCSI 1, 2 = SCSI 2 compatible
        vi.pvSCSIData[3] = 2;
        vi.pvSCSIData[4] = vsthw.byte4 - 5; // indicates bytes of data to come
        vi.pvSCSIData[5] = 0;
        vi.pvSCSIData[6] = 0;
        vi.pvSCSIData[7] = 0;

        i = 8;

#if 0
        // Don't display the real device name because the Mac CD driver won't
        // support newer CD and DVD drives!

        if (pdi->szVolume[0])
            pch = &pdi->szVolume[0];
#endif

#if TRACESCSI
        printf("SCSI command $12 INQUIRY %d: returning %s\n", vsthw.byte4, pch);
#endif

        while (*pch)
            vi.pvSCSIData[i++] = *pch++;

        while (i < vsthw.byte4)
            vi.pvSCSIData[i++] = 0;

        vi.pvSCSIData[32] = '1';
        vi.pvSCSIData[33] = '.';
        vi.pvSCSIData[34] = '0';
        vi.pvSCSIData[35] = 'p';

        vi.cSCSIRead = i;
        vi.iSCSIRead = 0;

#if TRACESCSI
#if 0
        if (pdi->cbSector > 512)
            fTracing = 400;
#endif
#endif
        }
    
        goto goodHD;

    case 0x55:
#if TRACESCSI
        printf("MODE SELECT 10 - NOT IMPLEMENTED!!\n");
#endif
        break;

    case 0x5A:
#if TRACESCSI
        printf("MODE SENSE 10 - NOT IMPLEMENTED!!\n");
#endif
        break;

    case 0x1A:
        // mode sense
        //
        // See the description in the Atari ST SCSI code

#if TRACESCSI
        fTracing = 000;

        printf("MODE SENSE id:%d cbSec:%d cb:%d page:%d\n",
                id, pdi->cbSector, vsthw.byte4, vsthw.byte2);
#endif

    if ((pdi->cbSector == 512) && (pdi->dt == DISK_SCSI))
        if (scsiModeSense(pdi->ctl, pdi->id, vi.pvSCSIData,
                 vsthw.byte4, vsthw.byte2))
            {
            vi.cSCSIRead = vsthw.byte4;
            vi.iSCSIRead = 4;    // start on 4th byte
            vi.iSCSIRead = 0;

#if TRACESCSI
            printf("SCSI command $1A MODE SENSE succeeded\n");
#endif

            vpregs->D0 = vsthw.byte4;
            goto goodHD;
            }
        else
            {
#if TRACESCSI
            printf("SCSI command $1A MODE SENSE failed\n");
#endif
            goto badHD;
            }

        {
        ULONG i = 0;
        unsigned int cSecMax = 0x00040000, cbSec = 512;
        char *pch = "APPLE COMPUTER, INC     ";

        if (pdi->cbSector)
            cbSec = pdi->cbSector;

        if (pdi->size && pdi->size >= 20000)
            cSecMax = (pdi->size * 2) / (cbSec / 512);

        vi.pvSCSIData[i++] = vsthw.byte4 - 4;

        if (cbSec > 512)
            vi.pvSCSIData[i++] = pdi->fEjected ? 0x71 : 0x01;
        else
            vi.pvSCSIData[i++] = 0;

        vi.pvSCSIData[i++] = 0;
        vi.pvSCSIData[i++] = 8;

        if (cbSec > 512)
            {
            // for CD-ROM, DCode = 1, Blocks = 0
            vi.pvSCSIData[i++] = 1;
            vi.pvSCSIData[i++] = 0;
            vi.pvSCSIData[i++] = 0;
            vi.pvSCSIData[i++] = 0;
            }
        else
            {
            // for hard disk, DCode = 0, Blocks = size
            vi.pvSCSIData[i++] = 0;
            vi.pvSCSIData[i++] = cSecMax >> 16;
            vi.pvSCSIData[i++] = cSecMax >> 8;
            vi.pvSCSIData[i++] = cSecMax;
            }

        // Reserv = 0, Blklen = 2048 or 512
        vi.pvSCSIData[i++] = 0;
        vi.pvSCSIData[i++] = 0;
        vi.pvSCSIData[i++] = cbSec >> 8;
        vi.pvSCSIData[i++] = cbSec;

        // Vendor data goes here, different for each mode page
        vi.pvSCSIData[i++] = 0;
        vi.pvSCSIData[i++] = 0;

#if TRACESCSI
        printf("SCSI command $1A MODE SENSE ID:%d page:%d cb:%d: returning %s\n", id, vsthw.byte2, vsthw.byte4, pch);
#endif

        while (*pch)
            vi.pvSCSIData[i++] = *pch++;

        while (i < vsthw.byte4)
            vi.pvSCSIData[i++] = 0;

        vi.pvSCSIData[vsthw.byte4] = 0;

        vi.cSCSIRead = i;
        vi.iSCSIRead = 0;
        }

        goto goodHD;

    case 0x15:
        // mode select, used by Mac OS disk format

#if TRACESCSI
        printf("MODE SELECT 6 - NOT IMPLEMENTED!!\n");
#endif
        goto goodHD;

    case 0x1B:
        // park hard disk
        // START STOP UNIT (CD-ROM)

#if TRACESCSI
        printf("START STOP\n");
#endif

        if ((vsthw.byte4 & 0x03) == 0x02)
            {
            // eject the CD-ROM

            FlushCachePdi(pdi);
            pdi->fEjected = vmachw.drive;

#if TRACESCSI
            printf("EJECTED drive %d\n", pdi->fEjected);
#endif
            }

        else if ((vsthw.byte4 & 0x01) == 0x01)
            {
            // load the CD-ROM

            pdi->fEjected = fFalse;

#if TRACESCSI
            printf("LOAD\n");
#endif
            }
        else
            {
#if TRACESCSI
            printf("UNKNOWN\n");
#endif
            }

        goto goodHD;

    case 0xCE:
    case 0xC1:
    case 0x1E:
        // prevent media eject (CD-ROM)

#if TRACESCSI
        fTracing = 000;
#endif

        goto goodHD;

    case 0x43:
        // read TOC

    if (0)
        if (pdi->dt == DISK_SCSI)
            if (scsiReadTOC(pdi->ctl, pdi->id, vi.pvSCSIData, 2048))
                {
                vi.cSCSIRead = 2048;
                vi.iSCSIRead = 0;

#if TRACESCSI
                printf("SCSI command $43 READ TOC returning %08X %08X\n", 
                     SwapL(*(unsigned *)&vi.pvSCSIData[0]),
                     SwapL(*(unsigned *)&vi.pvSCSIData[4]));
#endif
                goto goodHD;
                }

        pdi->lpBuf = (BYTE *)vi.pvSCSIData;
        pdi->track = 0;
        pdi->side  = 0;
        pdi->sec   = 0;
        pdi->count = 1;

        {
        ULONG i = 0;

        if (!pdi->fEjected &&
            FRawDiskRWPdi(pdi, 0))
            {
            vi.pvSCSIData[0] = 0;
            vi.pvSCSIData[1] = 0x12;
            vi.pvSCSIData[2] = 1;
            vi.pvSCSIData[3] = 1;
            }
        else
            {
            vi.pvSCSIData[0] = 0;
            vi.pvSCSIData[1] = 0;
            vi.pvSCSIData[2] = 0;
            vi.pvSCSIData[3] = 0;
            }

        for (i = 4; i < 2048; i++)
            vi.pvSCSIData[i] = 0;

        vi.cSCSIRead = i;
        vi.iSCSIRead = 0;
        }

        goto goodHD;

    case 0x25:
        // Read Capacity
        //
        // returns a 4 byte count of blocks and a 4 byte block size

        vi.cSCSIRead = 8;
        vi.iSCSIRead = 0;

        if (pdi->dt == DISK_SCSI)
            if (scsiReadCapacity(pdi->ctl, pdi->id, vi.pvSCSIData, 8))
                {
#if TRACESCSI
                printf("SCSI command $25 READ CAPACITY returning %08X %08X\n", 
                     SwapL(*(unsigned *)&vi.pvSCSIData[0]),
                     SwapL(*(unsigned *)&vi.pvSCSIData[4]));
#endif
                goto goodHD;
                }
            else
                {
#if TRACESCSI
                printf("SCSI command $25 READ CAPACITY failed!\n");
#endif

                // return something so as not to return garbage

                *(unsigned *)&vi.pvSCSIData[0] = SwapL(1);
                *(unsigned *)&vi.pvSCSIData[4] = SwapL(pdi->cbSector);
                goto badHD;
                }

        *(unsigned *)&vi.pvSCSIData[0] = SwapL(((pdi->size * 2) / (pdi->cbSector / 512)) - 1);
        *(unsigned *)&vi.pvSCSIData[4] = SwapL(pdi->cbSector);

#if TRACESCSI
        printf("SCSI command $25 READ CAPACITY : returning %08X %08X\n", 
             (pdi->size * 2) / (pdi->cbSector / 512), pdi->cbSector);
#endif
    
        goto goodHD;
        }

}

void WriteMacSCSI(ULONG ea, BYTE b)
{
#if TRACESCSI
    char rgch[255];
#endif
    static int id;

    // Mac SCSI

#if TRACESCSI
    printf("Writing SCSI address %08X with %02X\n", ea, b);
#if 0
    m68k_DumpRegs();
    CDis(vpregs->PC, TRUE);
#endif
#endif

    vmachw.rgbSCSIWr[ea] = b;

#if TRACESCSI
    sprintf(rgch, "%08X: Writing %02X to SCSI register %d\n",
        vpregs->PC, b, ea);
    DbgMessageBox(NULL, rgch, vi.szAppName, MB_OK);
    printf("state = %d\n", vmachw.MacSCSIstate);
#endif

    switch(ea)
        {
    default:
        DbgMessageBox(NULL, "Unknown SCSI register!", vi.szAppName, MB_OK);
        break;

    case 0:
    case 0x21:
        if (vmachw.MacSCSIstate == BUSFREE)
            {
            DbgMessageBox(NULL, "Putting SCSI ID on Bus", vi.szAppName, MB_OK);
            vmachw.rgbSCSIRd[0] = b;
            }
        break;

    case 1:
        if (b & 0x80)
            {
#if TRACESCSI
            DbgMessageBox(NULL, "SCSI Reset", vi.szAppName, MB_OK);
#endif
            vsthw.fHDC = 1;         // reset index counter
            vmachw.MacSCSIstate = BUSFREE;
            }

        if (b & 0x10)
            {
#if TRACESCSI
            DbgMessageBox(NULL, "sending ACK", vi.szAppName, MB_OK);
#endif
            vmachw.rgbSCSIRd[4] &= ~0x20;    // clear REQ
            }

        if (b == 0x01)
            {
#if TRACESCSI
            DbgMessageBox(NULL, "Sending data", vi.szAppName, MB_OK);
#endif

            // stuff in next byte

            if (vsthw.fHDC >= 11)
                break;

            vsthw.rgbHDC[vsthw.fHDC++] = vmachw.rgbSCSIWr[0];

            break;
            }

        if (b == 0x00)
            {
            if (vmachw.MacSCSIstate != BUSFREE)
                {
#if TRACESCSI
                DbgMessageBox(NULL, "setting REQ", vi.szAppName, MB_OK);
#endif
                vmachw.rgbSCSIRd[4] |= 0x20;    // set REQ
                }
            }

        if (b & 0x04)     // setting SEL
            {
#if TRACESCSI
            DbgMessageBox(NULL, "asserting SEL", vi.szAppName, MB_OK);
#endif
            vmachw.MacSCSIstate = SELECTION;

            if (b & 1)
                {
                unsigned char bf;

                vmachw.rgbSCSIRd[4] &= ~0x40;    // clear BSY
                vsthw.fHDC = 1;         // reset index counter

                // walk through bits 0..7 representing SCSI LUN 0 to 7
                // Bit 7 must also be set

                for (id = 0, bf = 1; bf < 0x80; id++, bf <<= 1)
                    {
                    if (vmachw.rgbSCSIWr[0] == (0x80 | bf))
                        {
#if TRACESCSI
                        DbgMessageBox(NULL, "responding to LUN", vi.szAppName, MB_OK);
                        printf("ID bits = %02X\n", bf);
#endif
                        vmachw.rgbSCSIRd[0] |= bf;      // set bit on data bus
                        vmachw.rgbSCSIRd[4] |= 0x40;    // set BSY
                        break;
                        }
                    }

                }
            }

        vmachw.rgbSCSIRd[1] &= 0x60;
        vmachw.rgbSCSIRd[1] |= (~0x60 & b);
        break;

    case 2:
        if (b == 0x00)
            {
            if (vmachw.MacSCSIstate == BUSFREE)
                {
#if TRACESCSI
                DbgMessageBox(NULL, "Preparing to arbitrate", vi.szAppName, MB_OK);
#endif
                }

            else if (vmachw.MacSCSIstate == SELECTION)
                {
#if TRACESCSI
                DbgMessageBox(NULL, "Preparing to enter RESELECTION", vi.szAppName, MB_OK);
#endif
                vmachw.MacSCSIstate = RESELECTION;
                }

            vmachw.rgbSCSIRd[1] &= ~0x60; // clear arbitration bits
            }

        else if (b == 0x01)     // setting arbitration bit
            {
            DbgMessageBox(NULL, "ARBITRATE mode", vi.szAppName, MB_OK);
            vmachw.MacSCSIstate = ARBITRATION;
            vmachw.rgbSCSIRd[1] |= 0x40; // set arbitration in progress bit
            vmachw.rgbSCSIRd[1] &= ~0x20; // clear lost arbitration bit

            if (vmachw.rgbSCSIWr[0] == 0x80)        // SCSI LUN 7 (the Mac itself)
                {
#if TRACESCSI
                DbgMessageBox(NULL, "no one at LUN 7!", vi.szAppName, MB_OK);
#endif
                vmachw.rgbSCSIRd[0] = 0x80; // no one else contended
                }
            else
                {
#if TRACESCSI
                DbgMessageBox(NULL, "Unknown arbitration state!", vi.szAppName, MB_OK);
#endif
                vmachw.rgbSCSIRd[4] &= ~0x40;    // clear BSY
                }
            }

        else if (b == 0x02)     // setting DMA mode
            {
#if TRACESCSI
            DbgMessageBox(NULL, "setting DMA mode", vi.szAppName, MB_OK);
#endif
            vmachw.MacSCSIstate = (vmachw.rgbSCSIRd[3] & 1) ? DATAIN : DATAOUT;
            vmachw.rgbSCSIRd[1] &= ~0x60; // clear arbitration bits
            }

        else if ((b & 1) == 0)
            vmachw.rgbSCSIRd[1] &= ~0x60; // clear arbitration bits

        vmachw.rgbSCSIRd[2] = b;
        break;

    case 3:
        if (b == 0x00)
            {
#if TRACESCSI
            if (vmachw.MacSCSIstate == SELECTION)
                DbgMessageBox(NULL, "pulling I/O low during SELECTION", vi.szAppName, MB_OK);
#endif
            }

#if TRACESCSI
        switch(b & 7)
            {
        default:
            printf("UNSPECIFIED PHASE %d\n", b & 7);
            break;

        case 0:
            printf("DATA OUT PHASE\n");
            break;

        case 2:
            printf("COMMAND PHASE\n");
            break;

        case 3:
            printf("MESSAGE OUT PHASE\n");
            break;

        case 4:
            printf("DATA IN PHASE\n");
            break;

        case 6:
            printf("STATUS PHASE\n");
            break;

        case 7:
            printf("MESSAGE IN PHASE\n");
            break;
            }
#endif

        vmachw.rgbSCSIRd[3] = b;
        break;

    case 4:
        break;

    case 5:
        break;

    case 6:
        break;

    case 7:
        // start DMA receive (read operation)
        // All 7 bytes arrived, start hard disk operation

        Assert(vsthw.fHDC <= 11);

        vmachw.sBSR |= 0x40;    // set DMA Req
        vmachw.sBSR |= 0x08;    // set phase match
        vmachw.rgbSCSIRd[0] = 0x00;     // set data bus to 0 for now

#if TRACESCSI
        {
        int i;

        printf("\nSCSI cmd to ID %d = $%02X !!\n", id, vsthw.byte0);
        printf("Raw command =");

        if (id == 0)
            fTracing = 0;
        else if (fTracing)
            {
            fTracing = 0;
            // exit(0);
            }

        for (i = 1; i < vsthw.fHDC; i++)
            printf(" %02X", vsthw.rgbHDC[i]);

        printf("\n");
        }
#endif

        // perform op on controller 0 drive 0

#ifndef WORMHOLE
        HandleSCSICmd(id);
#endif

        vsthw.fHDC = 1;         // reset index counter
        break;
        }

}


void __inline StuffStringP(ULONG Data, BYTE *pch)
{
#if 0
    printf("StuffString, stuffing '%s' (length %d) to address %08X\n",
        pch, strlen(pch), Data);
#endif

    vmPokeB(Data++, strlen(pch));

    while (*pch)
        vmPokeB(Data++, *pch++);
}

void __inline GetStringP(ULONG Data, BYTE *pch)
{
    ULONG cb = PeekB(Data++);

// printf("GetStringP: cb = %d, Data = %08X, pch = %08X\n", cb, Data, pch);

    while (cb-- && PeekB(Data))
        {
        *pch++ = PeekB(Data++);
        }

    *pch++ = 0;
}


void __cdecl mac_EFS()
{
    ULONG lID, Result, Code, Len, FilePos, Flags;
    ULONG Name, Data;
    BYTE rgb[MAX_PATH], rgb2[MAX_PATH];
    HANDLE hFind, hFile;
    WIN32_FIND_DATA fd;
    DWORD emOld;

    // Make sure Windows File System option is set

    if (vmCur.fUseVHD)
        return;

    emOld = SetErrorMode(SEM_FAILCRITICALERRORS);

    lID = PeekL(vpregs->A0);

    if (lID == 'ExFS')
        {
        Result = 0xFFFFFFFF; // assume the worst

        Code = PeekL(vpregs->A0 + 8);
        Name = PeekL(vpregs->A0 + 12);
        Len  = PeekL(vpregs->A0 + 16);
        Data = PeekL(vpregs->A0 + 20);
        FilePos = PeekL(vpregs->A0 + 24);
        Flags = PeekL(vpregs->A0 + 28);

#ifdef BETA
   //     m68k_DumpRegs();
   //     m68k_DumpHistory(1600);

        GetStringP(Name, rgb);

        printf("\nEFS:");
        printf("  Code = %d", Code);
        printf("  Len  = %d", Len);
        printf("  Data = %08x", Data);
        printf("  FPos = %08x", FilePos);
        printf("  Flgs = %08x\n", Flags);
        printf("  Name = %08X (%s)\n", Name, rgb);
#endif

        switch(Code)
            {
        default:
#ifdef BETA
            printf("Unknown code!\n");
#endif
            break;

        case 1:
#ifdef BETA
            printf("Get Prefix Separator\n");
            printf("Incoming length = %d\n", Len);
#endif

            StuffStringP(Name, "C:\\");

            Result = 0;
            break;

        case 42:
#ifdef BETA
            printf("Get Shared Directory Path\n");
#endif

            strcpy(rgb, (char *)&v.rgchGlobalPath);
            strcat(rgb, "\\");

            StuffStringP(Name, rgb);

            Result = 0;
            break;

        case 30:
            GetStringP(Name, rgb);

#ifdef BETA
            printf("Get Count of Files in Dir\n");
            printf("Directory = %s\n", rgb);
#endif

            strcat(rgb, "*.*");

            if ((hFind = FindFirstFile(rgb, &fd)) != INVALID_HANDLE_VALUE)
                {
                do
                    {
                    // filter out all file starting with a .
                    // not including . and ..

                    if ((fd.cFileName[0] == '.') && (fd.cFileName[1]))
                        if ((fd.cFileName[1] != '.') || (fd.cFileName[2]))
                            continue;

                    if (!strcmp("RESOURCE.FRK", fd.cFileName))
                        continue;

#ifdef BETA
                    printf("found file %s\n", fd.cFileName);
#endif

                    Len++;
                    } while (FindNextFile(hFind, &fd));

                FindClose(hFind);

                Result = 0;
                }
            else
                Result = -1;

#ifdef BETA
printf("returning count = %d\n", Len);
#endif

            break;

        case 31:
            GetStringP(Data, rgb);
            GetStringP(Data, rgb2);

            if (rgb[strlen(rgb)-1] == '@')
                rgb[strlen(rgb)-1] = 0;

#ifdef BETA
            printf("Get ith File in directory = %s, index = %d\n", rgb, Len);
#endif

            Data = 0;
            Result = -1;

            if (Len != 0)
                {
                strcat(rgb, "*.*");

                if ((hFind = FindFirstFile(rgb, &fd)) != INVALID_HANDLE_VALUE)
                    {
                    do
                        {
                        // filter out all file starting with a .
                        // including . and ..

                        if ((fd.cFileName[0] == '.') && (fd.cFileName[1]))
                            if ((fd.cFileName[1] != '.') || (fd.cFileName[2]))
                            continue;

                        if (!strcmp("RESOURCE.FRK", fd.cFileName))
                            continue;

                        Len--;
                        } while (Len && FindNextFile(hFind, &fd));

Lfileinfo:

#ifdef BETA
                    printf("found file %s\n", fd.cFileName);
#endif

                    StuffStringP(Name, fd.cFileName);
                    PokeL(vpregs->A0 + 24, fd.dwFileAttributes);
                    PokeL(vpregs->A0 + 28, fd.nFileSizeLow);

                    if (fd.cAlternateFileName[0])
                        StuffStringP(vpregs->A0 + 32, fd.cAlternateFileName);
                    else
                        StuffStringP(vpregs->A0 + 32, fd.cFileName);

                    PokeL(vpregs->A0 + 60,
                      LMacTimeFromFt(fd.ftCreationTime));

                    PokeL(vpregs->A0 + 64,
                      LMacTimeFromFt(fd.ftLastWriteTime));

                    FindClose(hFind);

                    // get resource fork info if it exists

                    strcpy(rgb, rgb2);
                    if (Data == 0)
                        strcat(rgb, fd.cFileName);

                    strcat(rgb,":AFP_Resource");

#ifdef BETAx
                    printf("Trying to open resource fork %s\n", rgb);
#endif

                    if ((hFile = CreateFile((LPCTSTR)rgb,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE)NULL)) != (HANDLE)-1)
                        {
#ifdef BETA
                        printf("Resource fork exists! size = %d\n",
                            GetFileSize(hFile, NULL));
#endif

                        PokeL(vpregs->A0 + 56, GetFileSize(hFile, NULL));

                        CloseHandle(hFile);
                        }
                    else
                        PokeL(vpregs->A0 + 56, 0);      // resource fork size

                    // get information fork info if it exists

                    strcpy(rgb, rgb2);
                    if (Data == 0)
                        strcat(rgb, fd.cFileName);
                    strcat(rgb,":AFP_AFPInfo");

                    PokeL(vpregs->A0 + 48, 0x3F3F3F3F);     // '????'
                    PokeL(vpregs->A0 + 52, 0x3F3F3F3F);     // '????'

                    if (Data != 0)
                        {
                        vmPokeL(Data     , 0x3F3F3F3F);     // '????'
                        vmPokeL(Data + 4 , 0x3F3F3F3F);     // '????'
                        vmPokeW(Data + 8 , 0x0100);
                        vmPokeL(Data + 10, 0xFFFFFFFF);
                        vmPokeW(Data + 14, 0);
                        }

#ifdef BETAx
                    printf("Trying to open info fork %s\n", rgb);
#endif

                    if ((hFile = CreateFile((LPCTSTR)rgb,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE)NULL)) != (HANDLE)-1)
                        {
                        BYTE rgInfo[60];
                        ULONG cbRead = 0;

#ifdef BETA
                        printf("Info fork exists! size = %d\n",
                            GetFileSize(hFile, NULL));
#endif
                        ReadFile(hFile, rgInfo, sizeof(rgInfo), &cbRead, NULL);
                        CloseHandle(hFile);

                        if (cbRead == sizeof(rgInfo))
                            {
                            int i;

                            for (i = 0; i < 4; i++)
                                {
                                vmPokeB(vpregs->A0 + 48 + i, rgInfo[20+i]);
                                vmPokeB(vpregs->A0 + 52 + i, rgInfo[16+i]);
                                }

                            if (Data != 0)
                            for (i = 0; i < 16; i++)
                                {
#ifdef BETA
                                if (i == 0)
                                    printf("setting info = ");
                                printf("%02X ", rgInfo[16+i]);
                                if (i == 15)
                                    printf("\n");
#endif
                                vmPokeB(Data + i, rgInfo[16+i]);
                                }
                            }
                        }
                    }

                // Len will have decremented to 0 as we counted the files
                // Is Len is not 0, it means the file index was too large

                if (Len == 0)
                    {
                    Len = fd.nFileSizeLow;
                    Result = 0;
                    }
                }
            break;

        case 32:
            GetStringP(Name, rgb);
#ifdef BETA
            printf("ReadFileBlock, ");
            printf("File = %s, %s\n", rgb, (Flags & 1) ? "Resource" : "Data");
            printf("Read  File Offset = %d, size = %d\n", FilePos, Len);
#endif

            // on NTFS, the resource fork is stored in the :AFP_Resource fork
            // on NTFS, the fileinfo fork is stored in the :AFP_AFPInfo  fork

            Result = -1;

            if (Flags & 1)
                strcat(rgb,":AFP_Resource");

            if (Len != 0)
                {
                HANDLE h;
                ULONG cbRead = 0;
                void *pv = NULL;

#if 0
                OFSTRUCT ofstruct;

                h = (HANDLE)OpenFile((const char *)&rgb, &ofstruct, OF_READ);

                if (h != INVALID_HANDLE_VALUE)
#else
                if ((h /* File */ = CreateFile((LPCTSTR)rgb,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    (HANDLE)NULL)) != (HANDLE)-1)
#endif
                    {
                    SetFilePointer(h, FilePos, NULL, FILE_BEGIN);

                    LockMemoryVM(Data, Len, &pv);
                    if (pv)
                        ReadFile(h, pv, Len, &cbRead, NULL);
                    UnlockMemoryVM(Data, Len);

                    Result = 0;
                    Len = cbRead;

#ifdef BETA
                    printf("ReadFileBlock read %d bytes!\n", Len);
                    printf("File Size = %d bytes\n", GetFileSize(h, NULL));
#endif
                    CloseHandle(h);
                    }
                else
                    {
                    Result = GetLastError();
                    Len = 0;
#ifdef BETA
                    printf("file open failed! Error = %d\n", Result);
#endif
                    }
                }
            break;

        case 33:
            GetStringP(Name, rgb);
#ifdef BETA
            printf("WriteFileBlock, ");
            printf("File = %s, %s\n", rgb, (Flags & 1) ? "Resource" : "Data");
            printf("Write File Offset = %d, size = %d\n", FilePos, Len);
#endif

            // on NTFS, the resource fork is stored in the :AFP_Resource fork
            // on NTFS, the fileinfo fork is stored in the :AFP_AFPInfo  fork

            Result = -1;

            if (Flags & 1)
                strcat(rgb,":AFP_Resource");

            if (Len != 0)
                {
                HANDLE h;
                ULONG cbRead = 0;
                void *pv;
                UINT uStyle = OF_WRITE | (FilePos ? 0 : OF_CREATE);

#if 0
                OFSTRUCT ofstruct;

                h = (HANDLE)OpenFile((const char *)&rgb, &ofstruct, uStyle);

                if (h != INVALID_HANDLE_VALUE)
#else
                if ((h /* File */ = CreateFile((LPCTSTR)rgb,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    (HANDLE)NULL)) != (HANDLE)-1)
#endif
                    {
                    BYTE b;
                    ULONG count = Len;

                    SetFilePointer(h, FilePos, NULL, FILE_BEGIN);

                    // write it a byte at a time in order to support ROM

// printf("writing ");
                    while (count > 1024)
                        {
                        BYTE rgb[1024];
                        int i;

                        for (i = 0; i < 1024; i++)
                            rgb[i] = (BYTE)vmPeekB(Data++);

                        WriteFile(h, &rgb, 1024, &cbRead, NULL);

                        count -= 1024;
                        }

                    while (count > 32)
                        {
                        BYTE rgb[32];
                        int i;

                        for (i = 0; i < 32; i++)
                            rgb[i] = (BYTE)vmPeekB(Data++);

                        WriteFile(h, &rgb, 32, &cbRead, NULL);

                        count -= 32;
                        }

                    while (count--)
                        {
                        b = (BYTE)vmPeekB(Data++);
// printf("%02X ", b);
                        WriteFile(h, &b, 1, &cbRead, NULL);
                        }
// printf("\n");

                    Result = 0;
                    Len = cbRead;

#ifdef BETA
                    printf("WriteFileBlock wrote %d bytes!\n", Len);
                    printf("File Size = %d bytes\n", GetFileSize(h, NULL));
#endif
                    CloseHandle(h);
                    }
                else
                    {
                    Result = GetLastError();
                    Len = 0;
#ifdef BETA
                    printf("file open failed! Error = %d\n", Result);
#endif
                    }
                }
            break;

        case 34:        // GetFileInfo, Name, Data points to 16 byte Finder info
            GetStringP(Name, rgb);
#ifdef BETA
            printf("GetFileInfo, file = %s, pInfo = %08X\n", rgb, Data);
#endif

            if ((hFind = FindFirstFile(rgb, &fd)) != INVALID_HANDLE_VALUE)
                {
                // set it up as if we just did an indexed lookup

                Len = 0;
                Result = -1;
                strcpy(rgb2,rgb);

                goto Lfileinfo;
                }

#ifdef BETA
            printf("FindFirstFile failed with error %d\n", GetLastError());
#endif

            Result = GetLastError();
            break;

        case 35:        // SetFileInfo, Name, Data points to 16 byte Finder info
            GetStringP(Name, rgb);
#ifdef BETA
            printf("SetFileInfo\n");
            printf("File = %s, pInfo = %08X\n", rgb, Data);
#endif

            strcat(rgb,":AFP_AFPInfo");

            if ((hFile = CreateFile((LPCTSTR)rgb,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                (HANDLE)NULL)) != (HANDLE)-1)
                {
                BYTE rgInfo[60];
                ULONG cbRead = 0;

#ifdef BETA
                printf("Info fork exists! size = %d\n",
                    GetFileSize(hFile, NULL));
#endif
                memset(rgInfo, 0, sizeof(rgInfo));

                rgInfo[0] = 'A';
                rgInfo[1] = 'F';
                rgInfo[2] = 'P';
                rgInfo[6] = 1;

                ReadFile(hFile, rgInfo, sizeof(rgInfo), &cbRead, NULL);

                {
                int i;

                for (i = 0; i < 16; i++)
                    {
                    rgInfo[16+i] = (BYTE)vmPeekB(Data + i);
                    }
                }

                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                WriteFile(hFile, rgInfo, sizeof(rgInfo), &cbRead, NULL);
                CloseHandle(hFile);
                }
            Result = 0;
            break;

        case 36:        // CreateDir, Name
            GetStringP(Name, rgb);
#ifdef BETA
            printf("CreateDir %s\n", rgb);
#endif
            {
            char *pch = rgb;

            if (pch = strstr(pch, "\\"))
                {
                strcpy(rgb2, rgb);  // save original string away

                pch = rgb;

                while (pch = strstr(pch+1, "\\"))
                    {
                    *pch = 0;
                    CreateDirectory(rgb,NULL);
                    strcpy(rgb, rgb2);
                    }
                }
            }

            if (CreateDirectory(rgb,NULL))
                Result = 0;
            else
                Result = GetLastError();
            break;

        case 37:        // DeleteDir, Name
            GetStringP(Name, rgb);
#ifdef BETA
            printf("DeleteDir %s\n", rgb);
#endif
            if (RemoveDirectory(rgb))
                Result = 0;
            else
                Result = GetLastError();
            break;
            break;

        case 38:        // CreateFile, Name, Type, Creator
            GetStringP(Name, rgb);
            *(ULONG *)rgb2 = PeekL(vpregs->A0 + 48);
            rgb2[4] = 0;
            *(ULONG *)(rgb2+8) = PeekL(vpregs->A0 + 52);
            rgb2[12] = 0;

#ifdef BETA
            printf("CreateFile %s\n", rgb);
            printf("Type = %s, Creator = %s\n", rgb2, rgb2+8);
#endif

            if ((hFile = CreateFile((LPCTSTR)rgb,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                (HANDLE)NULL)) != (HANDLE)-1)
                {
                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                SetEndOfFile(hFile);
                CloseHandle(hFile);

#ifdef BETA
                printf("File created!\n");
#endif

                // now create a resource fork, not so worried if that fails

                strcat(rgb,":AFP_Resource");

                if ((hFile = CreateFile((LPCTSTR)rgb,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    (HANDLE)NULL)) != (HANDLE)-1)
                    {
#ifdef BETA
                    printf("Resource fork created!\n");
#endif

                    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                    SetEndOfFile(hFile);
                    CloseHandle(hFile);
                    }
#ifdef BETA
                printf("CreateFile succeeded!\n");
#endif
                Result = 0;

                }
            else
                {
#ifdef BETA
                printf("CreateFile failed, error = %d!\n", GetLastError());
#endif
                Result = GetLastError();
                }
            break;


        case 39:        // DeleteFile, Name
            GetStringP(Name, rgb);
#ifdef BETA
            printf("DeleteFile %s\n", rgb);
#endif
            if (DeleteFile(rgb))
                {
#ifdef BETA
                printf("DeleteFile succeeded!\n");
#endif
                Result = 0;
                }
            else
                {
#ifdef BETA
                printf("DeleteFile failed, error = %d\n", GetLastError());
#endif
                Result = GetLastError();
                }

            break;

        case 40:        // RenameFile, Name, Data
            GetStringP(Name, rgb);
            GetStringP(Data, rgb2);
#ifdef BETA
            printf("RenameFile from %s to %s\n", rgb, rgb2);
#endif
            if (MoveFile(rgb, rgb2))
                {
#ifdef BETA
                printf("MoveFile succeeded!\n");
#endif
                Result = 0;
                }
            else
                {
#ifdef BETA
                printf("MoveFile failed, error = %d\n", GetLastError());
#endif
                Result = GetLastError();
                }

            break;

        case 41:        // DebugStr
#ifdef BETA
            GetStringP(Name, rgb);
            printf("DebugStr %s\n", rgb);
#endif
            Result = 0;
            break;

        case 43:
#ifdef BETA
            printf("Init File IDs\n");
#endif

            {
            int i;

            for (i = MAXINTATOM; i <= 0xFFFF; i++)
                DeleteAtom((ATOM)i);
            }

            Result = 0;
            break;

        case 44:
            GetStringP(Name, rgb);
#ifdef BETA
            printf("File ID From Path %s\n", rgb);
#endif

            if (strlen(rgb) < 4)
                Len = 2;

            else
                Len = FindAtom(rgb);

            if (Len == 0)
                Len = AddAtom(rgb);

            Result = (Len == 0) ? GetLastError() : 0;

#ifdef BETA
            printf("Returning ID %08X\n", Len);
#endif
            break;

        case 45:
#ifdef BETA
            printf("Path From File ID %08X\n", Len);
#endif

            Result = 0;

            if (Len == 2)
                {
                strcpy(rgb, "C:\\");
                StuffStringP(Name, rgb);
                }

            else if (GetAtomName((ATOM)Len, rgb, sizeof(rgb)))
                StuffStringP(Name, rgb);

            else
                Result = GetLastError();

#ifdef BETA
            printf("Returning path %s\n", rgb);
#endif
            break;

        case 46:
            GetStringP(Name, rgb);
#ifdef BETA
            printf("Setting End Of File on file %s %s fork to %08X\n",
                rgb, (Flags & 1) ? "Resource" : "Data", Len);
#endif
            if (Flags & 1)
                strcat(rgb,":AFP_Resource");

            Result = -1;

            if ((hFile = CreateFile((LPCTSTR)rgb,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                (HANDLE)NULL)) != (HANDLE)-1)
                {
                SetFilePointer(hFile, Len, NULL, FILE_BEGIN);
                SetEndOfFile(hFile);
                CloseHandle(hFile);

#ifdef BETA
                printf("End of file set!\n");
#endif
                Result = 0;
                }

            break;

        case 47:        // RenameDir, Name, Data
            GetStringP(Name, rgb);
            GetStringP(Data, rgb2);
#ifdef BETA
            printf("Move Directory from %s to %s\n", rgb, rgb2);
#endif
            if (MoveFile(rgb, rgb2))
                {
#ifdef BETA
                printf("Move Directory succeeded!\n");
#endif
                Result = 0;
                }
            else
                {
#ifdef BETA
                printf("Move Directory failed, error = %d\n", GetLastError());
#endif
                Result = GetLastError();
                }

            break;

        case 48:
            GetStringP(Name, rgb);
#ifdef BETA
            printf("Returning free disk space on %s\n", rgb);
#endif

            {
            UINT (__stdcall *pfnGF)();
            HMODULE h;
            __int64 llFree, llTotal;


            llFree = 0;     // assume free disk space is 0

            if ((h = LoadLibrary("kernel32.dll")) &&
               (pfnGF = (void *)GetProcAddress(h, "GetDiskFreeSpaceExA")))
                { 
                if ((*pfnGF)(rgb, &llFree, &llTotal, NULL))
                    ;
#ifdef BETA
                printf("extended free disk space = %dK\n", llFree / 1000L);
#endif
                }
            else
                {
                // Stupid Windows 95

                ULONG lClusterSize, lSectorSize, lFreeClusters, lTotalClusters;

                if (GetDiskFreeSpace(rgb, &lClusterSize, &lSectorSize,
                                       	&lFreeClusters, &lTotalClusters))
                    {
                    llFree = (__int64)lClusterSize * (__int64)lFreeClusters
                           * (__int64)lSectorSize;
                    }
#ifdef BETA
                printf("stupid free disk space = %dK\n", llFree / 1000L);
#endif
                }

            // overflow

            if (llFree != (long)llFree)
                {
#ifdef BETA
                printf("More than 2G free! %08X%08X\n", llFree);
#endif
                Len = 0x7FFFF000;
                }
            else
                Len = (long)llFree;

#ifdef BETA
            printf("Free disk space being returned = %d bytes\n", Len);
#endif
            }

            Result = 0;
            break;

        // hidden files should not be ignored

        // hidden bit and read-only bit need to be returns in Set/GetFileInfo

        // system and archive bit can be ignored

            }

        PokeL(vpregs->A0 + 4, Result);
        PokeL(vpregs->A0 + 16, Len);
        }

#ifdef BETA
    printf("returning Result %d\n", Result);
#endif

    SetErrorMode(emOld);
    return;
}

#endif // SOFTMAC

