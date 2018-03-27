
/***************************************************************************

    XSIO.C

    - Atari serial port emulation

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    09/27/2000  darekm      last update

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

#define DEBUGCOM 0

//
// SIO stuff
//

/* SIO return codes */
#define SIO_OK      0x01
#define SIO_TIMEOUT 0x8A
#define SIO_NAK     0x8B
#define SIO_DEVDONE 0x90

typedef struct
{
    WORD mode;
    WORD  h;
    WORD fWP;
    WORD wSectorMac;
    WORD ofs;
//    char *pbRAMdisk;
    char path[80];
    char name[12];
    ULONG cb;   // size of file when mode == MD_FILE
} DRIVE;

#define MAX_DRIVES 8

DRIVE rgDrives[MAX_VM][MAX_DRIVES];

#define MD_OFF  0
#define MD_SD   1
#define MD_ED   2
#define MD_DD   3
#define MD_QD   4
#define MD_HD   5
#define MD_RD   6
#define MD_35   7
#define MD_EXT  8
#define MD_FILE 9

// scary globals
int wCOM;		// !!! doesn't seem to be used
int fXFCable;	// !!! left unitialized


void DeleteDrive(int iVM, int i)
{
    if ((rgDrives[iVM][i].h > 0) && (rgDrives[iVM][i].h != 65535))
        _close(rgDrives[iVM][i].h);

    rgDrives[iVM][i].mode = MD_OFF;
    rgDrives[iVM][i].h    = -1;
}


void AddDrive(int iVM, int i, BYTE *pchPath)
{
    int h, sc=0;

    if (pchPath[0] == 'd' || pchPath[0] == 'D')
        {
        if (fXFCable && pchPath[2] == ':')
            {
            rgDrives[iVM][i].mode = MD_EXT;
            return;
            }
        }

    if ((rgDrives[iVM][i].h > 0) && (rgDrives[iVM][i].h != 65535))
        _close(rgDrives[iVM][i].h);

    h = _open(pchPath, _O_BINARY | _O_RDWR);

    rgDrives[iVM][i].fWP = 0;   // assume file opened for R/W

    if (h == -1)
        {
        h = _open(pchPath, _O_BINARY | _O_RDONLY);
        if (h == -1)
            goto Lbadfile;

        rgDrives[iVM][i].fWP = 1;  // file is read-only or in use
        }

    rgDrives[iVM][i].h = h;
    rgDrives[iVM][i].wSectorMac = 720;
    rgDrives[iVM][i].mode = MD_SD;
    strcpy(rgDrives[iVM][i].path,pchPath);

    if (_read(h,&sc,2) == 2)
        {
        ULONG l;

        l = _lseek(h,0,SEEK_END);
        rgDrives[iVM][i].cb = l;
        _lseek(h,2,SEEK_SET);

        if (sc == 0x296)
            {
            // it's an SIO2PC created image

            rgDrives[iVM][i].ofs = 16;
            _read(h,&l,2);                     // size lo
            _read(h,&sc,2);                     // bytes/sec
            _read(h,(((char *)&l) + 2),2);     // size hi

            l = l << 4;
            rgDrives[iVM][i].wSectorMac = (WORD)(l / sc);

            if (sc == 256)
                rgDrives[iVM][i].mode = MD_DD;
            }
        else
            {
            // assume it's a Xformer Cable created image
            // so just check for density

            rgDrives[iVM][i].ofs = 0;

            if (l == 368640)
                {
                rgDrives[iVM][i].mode = MD_QD;
                rgDrives[iVM][i].wSectorMac = 1440;
                }
            else if (l == 184320)
                {
                rgDrives[iVM][i].mode = MD_DD;
                rgDrives[iVM][i].wSectorMac = 720;
                }
            else if (l == 133120)
                {
                rgDrives[iVM][i].mode = MD_ED;
                rgDrives[iVM][i].wSectorMac = 1040;
                }
            else if (l == 92160)
                {
                rgDrives[iVM][i].mode = MD_SD;
                rgDrives[iVM][i].wSectorMac = 720;
                }
            else if ((i > 0) && (l <= 88375))
                {
                // it's just a file that we fake up as a virtual disk

                rgDrives[iVM][i].mode = MD_FILE;
                rgDrives[iVM][i].wSectorMac = 720;
                rgDrives[iVM][i].fWP = 1;  // force read-only
                rgDrives[iVM][i].cb = l;

                {
                int j;
                char *pch = pchPath, *pchT;

                memset(rgDrives[iVM][i].name,' ',12);

#if 0
                // REVIEW: why did I do this?

                while ((pchT = strstr(pch,":")) != NULL)
                    pch = pchT+1;

                while ((pchT = strstr(pch,"\\")) != NULL)
                    pch = pchT+1;
#endif

                for (j = 0; j < 12; j++, pch++)
                    {
                    if (*pch == 0)
                        break;
                    if (*pch != '.')
                        {
                        if ((*pch >= 'a') && (*pch <= 'z'))
                            *pch &= ~0x20;
                        rgDrives[iVM][i].name[j] = *pch;
                        }
                    else if (j < 9)
                        j = 7;
                    }
                }
                }
            else
                {
                // invalid disk image
Lbadfile:
                rgDrives[iVM][i].mode = MD_OFF;
                }
            }
        }
    else
        rgDrives[iVM][i].ofs = 0;

//            _close(h);
}

#if 0
void InitSIOV(int iVM, int argc, char **argv)
{
    int i, iArgv = 0;

	vpcandyCur = &vrgcandy[iVM];	// make sure we're looking at the proper instance

#ifdef HDOS16ORDOS32
    _bios_printer(_PRINTER_INIT, 0, 0);
#endif

    wCOM = -1;

    while ((argv[iArgv][0] == '-') || (argv[iArgv][0] == '/'))
        {
        switch (argv[iArgv][1])
            {
        case 'A':
        case 'a':
            fAutoStart = 1;
            break;

        case 'D':
        case 'd':
            fDebugger = 1;
            break;

        case 'N':
        case 'n':
            ramtop = 0xC000;
            break;

        case '8':
            mdXLXE = md800;
            break;

        case 'S':
        case 's':
#ifndef NDEBUG
            printf("sound activated\n");
#endif
            fSoundOn = TRUE;
#ifndef HWIN32
            InitSoundBlaster();
#endif
            break;

        case 'T':
        case 't':
            *pbshift &= ~wScrlLock;
            break;

        case 'C':
        case 'c':
            wCOM = argv[iArgv][2] - '1';
#ifndef NDEBUG
            printf("setting modem to COM%d:\n", wCOM+1);
#endif
            break;

#ifndef HWIN32
        case 'X':
        case 'x':
            fXFCable = 1;
            if (argv[iArgv][2] == ':')
                {
                _SIO_Init();
                sscanf(&argv[iArgv][3], "%d", &uBaudClock);
                }
            else
                {
                _SIO_Calibrate();
                }
            if (uBaudClock == 0)
                {
                fXFCable = 0;
                }
            else
                {
                }
            break;
#endif // !HWIN32

        case 'K':
        case 'k':
            if (argv[iArgv][2] == ':')
                ReadCart(iVM, &argv[iArgv][3]);
            break;

#ifndef HWIN32
        case 'J':
        case 'j':
#ifndef NDEBUG
            printf("joystick activated\n");
#endif
            MyReadJoy();

            if ((wJoy0X || wJoy1X))
                {
                fJoy = TRUE;
    
                wJoy0XCal = wJoy0X;
                wJoy0YCal = wJoy0Y;
                wJoy1XCal = wJoy1X;
                wJoy1YCal = wJoy1Y;
                }
    
#if 0
            for(;;)
                {
                MyReadJoy();
                printf("0X: %04X  0Y: %04X  1X: %04X  1Y: %04X  BUT: %02X\n",
                    wJoy0X, wJoy0Y, wJoy1X, wJoy1Y, bJoyBut);
                }
#endif
            
            break;
#endif // !HWIN32
            };

        iArgv++;
        }

    for (i = 0; i < MAX_DRIVES; i++)
        {
        if (i < argc)
            {
            AddDrive(iVM, i, argv[i+iArgv]);
            }
        else
            rgDrives[iVM][i].mode = MD_OFF;
        }
}
#endif

#define NO_TRANSLATION    32
#define LIGHT_TRANSLATION 0
#define HEAVY_TRANSLATION 16

void BUS1()
{
    WORD wRetStat = 1;
    WORD wStat = 0;
    
	// !!! statics. CIO won't work
	static BYTE oldstat = 0;
    static BYTE mdTranslation = NO_TRANSLATION;
    static BYTE fConcurrent = FALSE;

//    printf("in bus handler, regPC = %04X\n", regPC);

	vpcandyCur = &vrgcandy[v.iVM];	// make sure we're looking at the proper instance

    switch (regPC & 0x00F0)
        {
    default:
        break;

    case 0x00:    // CIO open
        FSetBaudRate(0, 0, 64, 1);
        break;

    case 0x10:  // CIO close
        fConcurrent = FALSE;
        break;

    case 0x20:  // CIO get byte

#if DEBUGCOM
        printf("g%04X", wStat); fflush(stdout);
#endif

        if (0x0100 & wStat)
            {
            wStat = 0;
            CchSerialRead(&wStat, 1);

#if DEBUGCOM
            printf("G%04X", wStat); fflush(stdout);
#endif
            if (wStat & 0xFF00)
                wRetStat = 136;

            regA = (BYTE)wStat;

            if (mdTranslation != NO_TRANSLATION)
                {
                if (regA == 13)
                    regA = 155;
                else
                    regA &= 0x7F;
                }
            }
        else
            wRetStat = 136;
        break;

    case 0x30:  // CIO put byte
        if (mdTranslation != NO_TRANSLATION)
            {
            if (regA == 155)
                regA = 13;
            else
                regA &= 0x7F;
            }

        FWriteSerialPort(regA);
        break;

    case 0x40:  // CIO status
        if (CchSerialPending())
            wStat = 0x4030;
        else
            wStat = 0x4130;

        if (fConcurrent)
            {
            cpuPokeB (746,0);   // error bits
            cpuPokeB (748,0);   // unknown
            cpuPokeB (749,0);   // characters to be sent

            // do character pending

#if DEBUGCOM
            printf("s%04X", wStat); fflush(stdout);
#endif

            if (0x0100 & wStat)
                cpuPokeB (747,1);
            else
                cpuPokeB (747,0);
//    printf("%d",cpuPeekB(747)); fflush(stdout);
            }
        else
            {
            // block mode status

            cpuPokeB (746,0);   // error bits

            // BIOS returns CD in bit 7, DSR in bit 5, CTS in bit 4
            // Atari needs CD in bit 3, DSR in bit 7, CTS in bit 5

            wStat = ((wStat & 0x80)>>4) | ((wStat & 0x20)<<2) | ((wStat & 0x10)<<1);

            wStat |= oldstat;
            oldstat = (BYTE)(wStat >> 1) & 0x54;

            cpuPokeB (747,(BYTE)wStat);   // handshake bits
            }

        break;

    case 0x50:  // CIO special
#if DEBUGCOM
        printf("XIO %d, aux1 = %d, aux2 = %d\n", cpuPeekB(0x22), cpuPeekB(0x2A), cpuPeekB(0x2B));
#endif

        switch (cpuPeekB(0x22))
            {
        default:
            break;

        case 32:     // flush output buffer
            break;

        case 34:     // handshake
            break;

        case 36:     // Baud, stop bits, ready monitoring
            {
            int baud = 300;

            switch (cpuPeekB(0x2A) & 15)
                {
            default:
                break;

            case 2:    //  50 baud
            case 4:    //  75 baud
            case 5:    // 110 baud
                baud = 110;
                break;

            case 6:    // 134 baud
            case 7:    // 150 baud
                baud = 150;
                break;

            case 9:    // 600 baud
                baud = 600;
                break;

            case 10:   // 1200 baud
                baud = 1200;
                break;

            case 11:    // 1800 baud
            case 12:    // 2400 baud
                baud = 2400;
                break;

            case 13:    // 4800 baud
                baud = 4800;
                break;

            case 14:    // 9600 baud
            case 15:    // 19200 baud
                baud = 9600;
                break;
                }

        FSetBaudRate(0, 0, 19200/baud, 1);

#if DEBUGCOM
            printf("setting baud rate to %d\n", baud);
#endif
            }
            break;

        case 38:     // translation and parity

            mdTranslation = cpuPeekB(0x2A) & 48;
#if DEBUGCOM
            printf("mdTranslation set to %d\n", mdTranslation);
#endif
            break;

        case 40:     // set concurrent mode
            fConcurrent = TRUE;
#if DEBUGCOM
            printf("concurrent mode set\n");
#endif
            break;
            }
        break;

    case 0x60:  // init vector
        if ((wCOM != 0) && (wCOM != 1))
            break;

        cpuPokeB (583, cpuPeekB(247) | 1); // tell OS that device 1 is a CIO device

        cpuPokeB (797,'R');
        cpuPokeB (798,0x8F);
        cpuPokeB (799,0xE4);
        break;

    case 0x70:  // SIO vector
        break;

    case 0x80:  // interrupt vector
        break;
        }

    cpuPokeB (0x343,(BYTE)wRetStat);
    regY = (BYTE)wRetStat;
    regP = (regP & ~ZBIT) | ((wRetStat & 0x80) ? 0 : ZBIT);
    regP = (regP & ~NBIT) | ((wRetStat & 0x80) ? NBIT : 0);

    regP |= CBIT; // indicate that command completed successfully

    regPC = cpuPeekW(regSP+1) + 1;        // do an RTS
    regSP = (regSP + 2) & 255 | 256;
}

void SIOV()
{
    WORD wDev, wDrive, wCom, wStat, wBuff, wSector, bAux1, bAux2;
    WORD  wBytes;
    WORD wTimeout;
    WORD fDD;
    WORD wRetStat = SIO_OK;
    WORD md;
    DRIVE *pdrive;
    ULONG lcbSector;
    BYTE rgb[256];
    WORD i;

	vpcandyCur = &vrgcandy[v.iVM];	// make sure we're looking at the proper instance

    if (regPC != 0xE459)
        {
        BUS1();
        return;
        }

#if 0
    printf("Device ID = %2x\n", cpuPeekB(0x300));
    printf("Drive # = %2x\n", cpuPeekB(0x301));
    printf("Command = %2x\n", cpuPeekB(0x302));
    printf("SIO Command = %2x\n", cpuPeekB(0x303));
    printf("Buffer = %2x\n", cpuPeekW(0x304));
    printf("Timeout = %2x\n", cpuPeekW(0x306));
    printf("Byte count = %2x\n", cpuPeekW(0x308));
    printf("Sector = %2x\n", cpuPeekW(0x30A));
    printf("Aux1 = %2x\n", cpuPeekB(0x30A));
    printf("Aux2 = %2x\n", cpuPeekB(0x30B));
#endif

    wDev = cpuPeekB(0x300);
    wDrive = cpuPeekB(0x301)-1;
    wCom = cpuPeekB(0x302);
    wStat = cpuPeekB(0x303);
    wBuff = cpuPeekW(0x304);
    wTimeout = cpuPeekW(0x306);
    wBytes = cpuPeekW(0x308);
    wSector = cpuPeekW(0x30A);
    bAux1 = cpuPeekB(0x30A);
    bAux2 = cpuPeekB(0x30B);

    if (wDev == 0x31)       /* disk drives */
        {
        if ((wDrive < 0) || (wDrive >= MAX_DRIVES))
            goto lCable;

        pdrive = &rgDrives[v.iVM][wDrive];

        md = pdrive->mode;

        if (md == MD_OFF)           /* drive is off */
            {
            wRetStat = 138;
            goto lExit;
            }

        if (md == MD_EXT)
            goto lCable;

        if (md == MD_35)            /* 3.5" 720K disk */
            {
lNAK:
            wRetStat = SIO_NAK;
            goto lExit;
            }

        fDD = (md==MD_QD) || (md==MD_DD) || (md==MD_HD) || (md==MD_RD);

        if (pdrive->h == -1)
            goto lNAK;

        /* the disk # is valid, the sector # is valid, # bytes is valid */

        switch(wCom)
            {
        default:
         /*   printf("SIO command %c\n", wCom); */
            wRetStat = SIO_NAK;
            break;
        
        /* format enhanced density, we don't support that */
        case '"':
            if (md != MD_ED)
                wRetStat = SIO_NAK;
            break;
  
        case '!':
            if (pdrive->fWP)       /* is drive write-protected? */
                {
                wRetStat = SIO_DEVDONE;
                break;
                }

            /* "format" disk, just zero all sectors */

            memset(rgb,0,sizeof(rgb));

            if ((md == MD_SD) || (md == MD_ED))
                lcbSector = 128L;
            else
                lcbSector = 256L;

            _lseek(pdrive->h,(ULONG)pdrive->ofs,SEEK_SET);

            for (i = 0; i < pdrive->wSectorMac; i++)
                {
                if (_write(pdrive->h,(void *)&rgb,(WORD)lcbSector) < (WORD)lcbSector)
                    {
                    wRetStat = SIO_DEVDONE;
                    break;
                    }
                }
            break;

        /* status request */
        case 'S':
          /*  printf("SIO command 'S'\n"); */

            /* b7 = enhanced   b5 = DD/SD  b4 = motor on   b3 = write prot */
            cpuPokeB (wBuff++, ((md == MD_ED) ? 128 : 0) + (fDD ? 32 : 0) + (pdrive->fWP ? 8 : 0));

            cpuPokeB (wBuff++, 0xFF);         /* controller */
            cpuPokeB (wBuff++, 0xE0);         /* format timeout */
            cpuPokeB (wBuff, 0x00);           /* unused */
            break;
            
        /* get configuration */
        case 'N':
          /*  printf("SIO command 'N'\n"); */
            if (md == MD_HD)
                {
                pdrive->wSectorMac--;
                cpuPokeB (wBuff++, 1);   /* tracks */
                cpuPokeB (wBuff++, 0);    /* ?? */
                cpuPokeB (wBuff++, pdrive->wSectorMac & 255);   /* ?? */
                cpuPokeB (wBuff++, pdrive->wSectorMac >> 8);    /* sectors/track */
                cpuPokeB (wBuff++, 0x00);         /* ?? */
                pdrive->wSectorMac++;
                }
            else
                {
                cpuPokeB (wBuff++, 0x28);         /* tracks */
                cpuPokeB (wBuff++, 0x02);         /* ?? */
                cpuPokeB (wBuff++, 0x00);         /* ?? */
                cpuPokeB (wBuff++, (md == MD_ED) ? 0x1A : 0x12); /* secs/track */
                cpuPokeB (wBuff++, 0x00);         /* ?? */
                }

            if (fDD)
                {
                cpuPokeB (wBuff++, 0x04);     /* density: 4 = dbl  0 = sng */
                cpuPokeB (wBuff++, 0x01);     /* bytes/sector hi */
                cpuPokeB (wBuff++, 0x00);     /* bytes/sector lo */
                }
            else
                {
                cpuPokeB (wBuff++, 0x00);     /* density: 4 = dbl  0 = sng */
                cpuPokeB (wBuff++, 0x00);     /* bytes/sector hi */
                cpuPokeB (wBuff++, 0x80);     /* bytes/sector lo */
                }
            cpuPokeB (wBuff++, 0xFF);         /* unused */
            cpuPokeB (wBuff++, 0xFF);         /* unused */
            cpuPokeB (wBuff++, 0xFF);         /* unused */
            cpuPokeB (wBuff, 0xFF);           /* unused */
            break;

        /* set configuration - we don't support it */
        case 'O':
          /*  printf("SIO command 'O'\n"); */
            wRetStat = SIO_OK;
            break;
            goto lNAK;

        case 'P':
        case 'W':
        case 'R':
            {
            int cbSIO2PCFudge = pdrive->ofs;

            if (wSector < 1)            /* invalid sector # */
                goto lNAK;

            if ((wSector < 4) || (md == MD_FILE) || (md == MD_SD) || (md == MD_ED))
                {
                if (wBytes != 128)
                    goto lNAK;
                }
            else if (md == MD_HD)
                wBytes = 256;
            else if (wBytes != 256)
                goto lNAK;

            if (wSector > pdrive->wSectorMac)   /* invalid sector # */
                goto lNAK;

            if ((md == MD_FILE) || (md == MD_SD) || (md == MD_ED))
                lcbSector = 128L;
            else if ((wSector < 4) && pdrive->ofs)  // SIO2PC disk image
                lcbSector = 128L;
            else if (pdrive->ofs)
                {
                lcbSector = 256L;
                if (pdrive->cb != 184720)
                    cbSIO2PCFudge -= 384;
                }
            else
                lcbSector = 256L;

            _lseek(pdrive->h,(ULONG)((wSector-1) * lcbSector) + cbSIO2PCFudge,SEEK_SET);

            if ((wCom == 'R') && (wStat == 0x40))
                {
#if 0
                printf("Read: sector = %d  wBuff = $%4x  wBytes = %d  lcbSector = %ld  md = %d\n",
                    wSector, wBuff, wBytes, lcbSector, md);
#endif

                /* temporary kludge to prevent reading over ROM */
                if (wBuff >= ramtop)
                    wRetStat = SIO_OK;
                else if (md == MD_FILE)
                    {
                    // read a normal file as an 8-bit file in a virtual disk

                    memset(rgb,0,sizeof(rgb));
                    _fmemset(&rgbMem[wBuff],0,128);

                    if (wSector == 360)
                        {
                        // FAT sector

                        rgbMem[wBuff+0] = 0x02;
                        rgbMem[wBuff+1] = 0xC3;
                        rgbMem[wBuff+2] = 0x02;
                        rgbMem[wBuff+3] = 0xC2;
                        rgbMem[wBuff+4] = 0x02;
//                        memset(&rgbMem[wBuff+10],0x00,118);
                        }
                    else if (wSector == 361)
                        {
                        // directory sector

                        rgbMem[wBuff+0] = 0x42;
                        rgbMem[wBuff+1] = (BYTE)((pdrive->cb+124L) / 125L);
                        rgbMem[wBuff+2] = (BYTE)((pdrive->cb+124L) / 32000L);
                        rgbMem[wBuff+3] = 0x04;
                        rgbMem[wBuff+4] = 0x00;
                        memcpy(&rgbMem[wBuff+5],pdrive->name,11);
                        }
                    else if (wSector >= 4)
                        {
                        // data sector

                        if (wSector > 368)   // skip FAT/directory sectors
                            wSector -= 9;

                        _lseek(pdrive->h,(ULONG)((wSector-4) * 125L),SEEK_SET);

                        if ((rgbMem[wBuff+127] =
                                _read(pdrive->h,&rgbMem[wBuff],125)) < 125)
                            {
                            rgbMem[wBuff+125] = 0x00;
                            rgbMem[wBuff+126] = 0x00;
                            }
                        else
                            {
                            wSector++;

                            if (wSector >= 360)
                                wSector += 9;
                            else if (wSector > 720)
                                wSector = 0;

                            rgbMem[wBuff+125] = (BYTE)(wSector >> 8);
                            rgbMem[wBuff+126] = (BYTE)wSector;
                            }
                        }
                    }
                else if (_read(pdrive->h,&rgbMem[wBuff],wBytes) < wBytes)
                    wRetStat = SIO_DEVDONE;
                else
                    wRetStat = SIO_OK;
                }
            else if ((wCom == 'W') || (wCom == 'P'))
                {
                if (wStat != 0x80)
                    goto lNAK;
                if (pdrive->fWP)
                    {
                    wRetStat = SIO_DEVDONE;
                    break;
                    }
                if (_write(pdrive->h,&rgbMem[wBuff],wBytes) < wBytes)
                    wRetStat = SIO_DEVDONE;
                else
                    wRetStat = SIO_OK;
                }
            break;
            }
            }
        }

    else if (!fXFCable && wDev == 0x40)      /* printer */
        {
        int timeout = 32000;

#ifdef xDEBUG
        printf("printer SIO: wDev = %2x  wCom = %2x  wBytes = %2x\n",
            wDev, wCom, wBytes);
        printf("SIO: wBuff = %4x  bAux1 = %2x  bAux2 = %2x\n",
            wBuff, bAux1, bAux2);
#endif

        switch(wCom)
            {
        /* status request */
        case 'S': 
            while (timeout--)
                {
                if (FPrinterReady())
                    {
                    wRetStat = SIO_OK;
                    break;
                    }

                if (timeout == 0)
                    {
                    wRetStat = SIO_TIMEOUT;
                    break;
                    }
                }
            break;
            
        case 'W':
            /* print line */
            {
            BYTE *pchBuf = &rgbMem[wBuff];
            int  cch = wBytes;
            BYTE ch;

            while ((pchBuf[cch-1] == ' ') && (cch>0))
                cch--;

            while ((cch-- > 0) && (wRetStat == SIO_OK))
                {
                ch = *pchBuf++;
#if 0
                printf("printing: %02X\n", ch);
                fflush(stdout);
#endif

                while (timeout--)
                    {
                    if (FPrinterReady())
                        {
                        if (ch != 155)
                            ByteToPrinter(ch);
                        else
                            {
                            ByteToPrinter(13);
                            ByteToPrinter(10);
                            }

                        wRetStat = SIO_OK;
                        break;
                        }

                    if (timeout == 0)
                        {
                        wRetStat = SIO_TIMEOUT;  // else status = timeout error
                        break;
                        }
                    }
                } // while
            }
            } // switch
        }

    else
        {
lCable:
            {
            wRetStat = SIO_TIMEOUT;
            goto lExit;
            }

        ;
        }

#ifdef xDEBUG
    printf("wRetStat = %d\n", wRetStat);
#endif

lExit:
    cpuPokeB (0x303,(BYTE)wRetStat);
    regY = (BYTE)wRetStat;
    regP = (regP & ~ZBIT) | ((wRetStat == 0) ? ZBIT : 0);
    regP = (regP & ~NBIT) | ((wRetStat & 0x80) ? NBIT : 0);

    regPC = cpuPeekW(regSP+1) + 1;        // do an RTS
    regSP = (regSP + 2) & 255 | 256;

#if 0
    printf("SIO: returning to PC = %04X, SP = %03X, stat = %02X\n", regPC, regSP, regY);
#endif
}

#endif // XFORMER

