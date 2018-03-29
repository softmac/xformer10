
/****************************************************************************

    ROMCARD.C

    - Reads and displays contents of Gemulator card(s)

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    03/03/2008  darekm      update for EmuTOS
    
****************************************************************************/

#if defined(ATARIST) || defined(SOFTMAC)

#define _CRT_SECURE_NO_WARNINGS

#include "gemtypes.h"

// 17 countries supported in ROMs

#define MAXCOUNTRY 17

char const * const rgszCountry[MAXCOUNTRY] =
        {
        "U.S.A.",
        "Germany",
        "France",
        "United Kingdom",
        "Spain",
        "Italy",
        "Sweden",
        "Swiss (French)",
        "Swiss (German)",
        "Turkey",
        "Finland",
        "Norway",
        "Denmark",
        "Saudi Arabia",
        "Holland",
        "Czechoslovakia",
        "Hungary"
        };

const char szMagicRAM[] = ".\\MAGIC.RAM";
const char szMagicPC[] =  ".\\MAGIC_PC.OS";
const char szTOS_RAM[] =  ".\\TOS.IMG";
const char szEMU_TOS[] =  ".\\ETOS*.IMG";

const char szMAC_RAM[] =  ".\\*.ROM";


// from devioctl.h

// Device type           -- in the "User Defined" range."
#define GPD_TYPE 40000

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )

#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#define METHOD_BUFFERED                 0

static unsigned int cbCache;
static unsigned char rgbCache[512];

static unsigned int cbSocket = 128*1024;
static unsigned int cSockets = 8;

#define cScktMax 64
#define wScktMsk (cScktMax-1)


int InPort()
{
    if (cbCache)
        {
        cbCache--;
        return rgbCache[sizeof(rgbCache)-1-cbCache];
        }

    if (vi.fWinNT)
        {
        // The following is returned by IOCTL.  It is true if the read succeeds.
        BOOL    IoctlResult;

        // The following parameters are used in the IOCTL call
        ULONG   PortNumber = vi.ioPort & ~3; // Buffer sent to the driver (Port #).
        LONG     IoctlCode = (LONG)IOCTL_GPD_READ_PORT_ULONG;
        DWORD   ReturnedLength;     // Number of bytes returned

        if (vi.hROMCard == INVALID_HANDLE_VALUE)
            memset(&rgbCache[0], 0xFF, sizeof(rgbCache));
        else
            IoctlResult = DeviceIoControl(
                vi.hROMCard,        // Handle to device
                IoctlCode,          // IO Control code for Read
                &PortNumber,        // Buffer to driver.
                sizeof(PortNumber), // Length of buffer in bytes.
                &rgbCache[0],       // Buffer from driver.
                512,                // Length of buffer in bytes.
                &ReturnedLength,    // Bytes placed in DataBuffer.
                NULL                // NULL means wait till op. completes.
                );

        cbCache = 511;
        }
#if !defined(_M_AMD64) && !defined(_M_ARM) && !defined(_M_ARM64)
    else
        {
        *(int *)&rgbCache[sizeof(rgbCache)-4] = _inpd((unsigned short)vi.ioPort);
        cbCache = 3;
        }
#endif

    return rgbCache[sizeof(rgbCache)-1-cbCache];
}


void OutPort(BYTE b)
{
    cbCache = 0;

    if (vi.fWinNT)
        {
        // The following is returned by IOCTL.  It is true if the read succeeds.
        BOOL    IoctlResult;

        // The following parameters are used in the IOCTL call
        LONG     IoctlCode = (LONG)IOCTL_GPD_WRITE_PORT_UCHAR;
        DWORD   ReturnedLength;     // Number of bytes returned
        GENPORT_WRITE_INPUT InputBuffer;

        if (vi.hROMCard == INVALID_HANDLE_VALUE)
            return;

        InputBuffer.PortNumber = vi.ioPort & ~3; // Buffer sent to the driver (Port #).
        InputBuffer.CharData = b;

        IoctlResult = DeviceIoControl(
                vi.hROMCard,        // Handle to device
                IoctlCode,          // IO Control code for Read
                &InputBuffer,       // Buffer to driver.
                5,                  // Length of buffer in bytes.
                NULL,               // Buffer from driver.
                0,                  // Length of buffer in bytes.
                &ReturnedLength,    // Bytes placed in DataBuffer.
                NULL                // NULL means wait till op. completes.
                );

        GetLastError();
        return;
        }

#if !defined(_M_AMD64) && !defined(_M_ARM) && !defined(_M_ARM64)
    _outp((unsigned short)vi.ioPort, b);
#endif
}


void SkipROMs(cb)
int cb;
{
    while (cb--)
        InPort();
}


//
// Given the current port number is vi.ioPort, read in the ROM card
// contents and determine if it is an 8 socket 1M (128K per socket) card,
// or an 11 socket 5.5M (512K per socket) card.
//
// The way to do this, it assume it is an 8M card with 64 128K sockets
// and then do a sanity check. If the every group of 8 sockets matches
// every other one, it's an 8 socket 1M card.

void GetCardSize(/* unsigned char rgrgb[cScktMax][256] */)
{
    cbSocket = 128*1024;
    cSockets = 8;

#if 1
    // On versions of Gemulator that don't support the
    // 11 socket card, just return the standard size.

    return;
#else
    {
    int i, j;

    OutPort(0);

    for (i = 0; i < cScktMax; i++)
        {
        for (j = 0; j < 256; j++)
            {
            rgrgb[i][j] = InPort();
            }

        SkipROMs(cbSocket-256);
        }

    for (i = 1; i < 8; i++)
        {
        for (j = 0; j < 8; j++)
            {
            if (memcmp(rgrgb[0], rgrgb[i*8], 256))
                {
                cbSocket = 512*1024;
                cSockets = 11;
        MessageBox(NULL, "port is 512K 11 sockets!", vi.szAppName, MB_OK);
                return;
                }
            }
        }

    MessageBox(NULL, "port is 128K 8 sockets!", vi.szAppName, MB_OK);
    }
#endif
}

void ListROMs(HWND hDlg, ULONG wf)
{
    // Stuff description of all detected OSes to given dialog box
    // Only display ROMs that match wf if wf!=0

    unsigned i;
    char rgch[256], rgch2[80];

    SendDlgItemMessage(hDlg, IDC_COMBOTOSLIST, CB_RESETCONTENT, 0, 0);

    for (i = 0; i < v.cOS; i++)
        {
        if (wf && !(v.rgosinfo[i].osType & wf))
            continue;

        if ((v.rgosinfo[i].szImg[0]) && (v.rgosinfo[i].szImg[1] > 3))
            sprintf(rgch2, "%s", v.rgosinfo[i].szImg);
        else
            sprintf(rgch2,
                 "port %03X/%d", v.rgosinfo[i].ioPort,
                  v.rgosinfo[i].iChip & wScktMsk);

        switch (v.rgosinfo[i].osType)
            {
        default:
            sprintf(rgch, " Unknown");
            break;

        case osAtari48:
        case osAtariXL:
        case osAtariXE:
            sprintf(rgch, " %s", PchFromBfRgSz(v.rgosinfo[i].osType, rgszROM));
            sprintf(rgch2, "built-in");
            break;

        case osDiskImg:
            sprintf(rgch, " Tutor ROM image");
            break;

        case osMac_64:
        case osMac_128:
        case osMac_256:
        case osMac_512:
        case osMac_II:
        case osMac_IIi:
        case osMac_1M:
        case osMac_2M:
        case osMac_G1:
        case osMac_G2:
        case osMac_G3:
        case osMac_G4:
            sprintf(rgch, " Macintosh %d%c ver: %d.%02d,",
                  (v.rgosinfo[i].cbOS >= 1024*1024) ?
                  (v.rgosinfo[i].cbOS+512)/1024/1024 :
                  (v.rgosinfo[i].cbOS+512)/1024,
                  (v.rgosinfo[i].cbOS >= 1024*1024) ? 'M' : 'K',
                  v.rgosinfo[i].version/100, v.rgosinfo[i].version % 100);
            break;

        case osMagic_2:
        case osMagic_4:
        case osMagicPC:
            sprintf(rgch, " Atari ST  MagiC, %dK", (v.rgosinfo[i].cbOS+512)/1024);
            break;

        case osTOS1X_D:
        case osTOS1X_2:
        case osTOS1X_6:
            sprintf(rgch, " Atari ST  TOS %1X.%02X, %02X/%02X/%04X, %s   ",
                v.rgosinfo[i].version>>8,
                v.rgosinfo[i].version&15,
                v.rgosinfo[i].date>>24,
                (v.rgosinfo[i].date>>16)&255,
                v.rgosinfo[i].date&0xFFFF,
                (v.rgosinfo[i].country >= MAXCOUNTRY) ? " " : rgszCountry[v.rgosinfo[i].country]);
            break;

        case osTOS2X_2:
        case osTOS2X_D:
            sprintf(rgch, " Atari STE TOS %1X.%02X, %02X/%02X/%04X, %s   ",
                v.rgosinfo[i].version>>8,
                v.rgosinfo[i].version&15,
                v.rgosinfo[i].date>>24,
                (v.rgosinfo[i].date>>16)&255,
                v.rgosinfo[i].date&0xFFFF,
                (v.rgosinfo[i].country >= MAXCOUNTRY) ? " " : rgszCountry[v.rgosinfo[i].country]);
            break;

        case osTOS3X_4:
        case osTOS3X_D:
            sprintf(rgch, " Atari TT  TOS %1X.%02X, %02X/%02X/%04X, %s   ",
                v.rgosinfo[i].version>>8,
                v.rgosinfo[i].version&15,
                v.rgosinfo[i].date>>24,
                (v.rgosinfo[i].date>>16)&255,
                v.rgosinfo[i].date&0xFFFF,
                (v.rgosinfo[i].country >= MAXCOUNTRY) ? " " : rgszCountry[v.rgosinfo[i].country]);
            break;
            }

        strcat(rgch, " (");
        strcat(rgch, rgch2);
        strcat(rgch, ")");

#if !defined(NDEBUG)
        printf("Adding: %s\n", rgch);
#endif
        SendDlgItemMessage(hDlg, IDC_COMBOTOSLIST, CB_ADDSTRING, 0, (LPARAM)rgch);
        }

    SendDlgItemMessage(hDlg, IDC_COMBOTOSLIST, CB_SETCURSEL, 0, 0);
}


void UpdateDIPs(HWND hDlg, int w)
{
    CheckRadioButton(hDlg, IDC_DIP1UP, IDC_DIP1DN,
        IDC_DIP1UP + ((w & 0x004) != 0));
    CheckRadioButton(hDlg, IDC_DIP2UP, IDC_DIP2DN,
        IDC_DIP2UP + ((w & 0x008) != 0));
    CheckRadioButton(hDlg, IDC_DIP3UP, IDC_DIP3DN,
        IDC_DIP3UP + ((w & 0x010) != 0));
    CheckRadioButton(hDlg, IDC_DIP4UP, IDC_DIP4DN,
        IDC_DIP4UP + ((w & 0x020) != 0));
    CheckRadioButton(hDlg, IDC_DIP5UP, IDC_DIP5DN,
        IDC_DIP5UP + ((w & 0x040) != 0));
    CheckRadioButton(hDlg, IDC_DIP6UP, IDC_DIP6DN,
        IDC_DIP6UP + ((w & 0x080) != 0));
    CheckRadioButton(hDlg, IDC_DIP7UP, IDC_DIP7DN,
        IDC_DIP7UP + ((w & 0x100) != 0));
    CheckRadioButton(hDlg, IDC_DIP8UP, IDC_DIP8DN,
        IDC_DIP8UP + ((w & 0x200) != 0));
}


void CalcCheckSum(HWND hDlg)
{
    unsigned iChip = 0;  /* current starting socket # (0..7) */
    unsigned i;
    ULONG chksum;
    BYTE rgch[256], *pch;
    HCURSOR h = SetCursor(LoadCursor(NULL, IDC_WAIT));
    BYTE rgb[16];
//    unsigned char rgrgb[cScktMax][256];

    GetLastError();

    SendDlgItemMessage(hDlg, IDC_CHECKSUMLIST, LB_RESETCONTENT, 0, 0);

    for (iChip = 0; iChip < (v.cCards * cScktMax); iChip++)
        {
        if ((iChip & wScktMsk) == 0)
            {
            vi.ioPort = v.ioBaseAddr + (iChip / cScktMax)*4;
            GetCardSize(/* rgrgb */);
            OutPort(0);
            sprintf(rgch, "Port $%03X:", vi.ioPort);
            SendDlgItemMessage(hDlg, IDC_CHECKSUMLIST, LB_ADDSTRING, 0, (LPARAM)rgch);
            }
        else if ((iChip & wScktMsk) >= cSockets)
            {
            iChip += (wScktMsk - (iChip & wScktMsk));
            continue;
            }

#if 0
printf("CalcCheckSum: iChip = %d\n", iChip);
printf("CalcCheckSum: ioPort = %d\n", vi.ioPort);
printf("CalcCheckSum: cbSocket = %d\n", cbSocket);
printf("CalcCheckSum: cSockets = %d\n", cSockets);
#endif

        sprintf(rgch, "    U%d: chksum = ", (iChip&wScktMsk)+1);

        for (chksum = 0, i = 0; i < cbSocket; i++)
            {
            BYTE b = ~InPort();

            if (i < 16)
                rgb[i] = ~b;

            if (i < (128*1024))
                chksum += (b << ((i & (128*1024 - 1)) % 10));
            }

        // with 512K sockets, the checksum on older ROMs will be
        // 4 times the value. Rotate right by 2 bits so that
        // the old ROMs give the correct value for 128K sockets,
        // while larger ROMs will give a new unique value.

        if (cbSocket == (512*1024))
            {
            chksum = (chksum >> 2);
            }

        pch = rgch + strlen(rgch);
        sprintf(pch, "%08X, ", chksum);
        pch = rgch + strlen(rgch);

        switch (chksum)
            {
        default:
            break;

        case 0x00000000:
            sprintf(pch, "Empty socket");
            break;

        case 0x92218B4C:
            sprintf(pch, "U.S.A. TOS 1.00 H0");
            break;

        case 0x7ECF6AC2:
            sprintf(pch, "U.S.A. TOS 1.00 L0");
            break;

        case 0x91BBE82D:
            sprintf(pch, "U.S.A. TOS 1.00 H1");
            break;

        case 0x87FEA6E2:
            sprintf(pch, "U.S.A. TOS 1.00 L1");
            break;

        case 0x86588706:
            sprintf(pch, "U.S.A. TOS 1.00 H2");
            break;

        case 0x7EAD8C48:
            sprintf(pch, "U.S.A. TOS 1.00 L2");
            break;

        case 0x90B51FCB:
            sprintf(pch, "U.S.A. TOS 1.02 H0");
            break;

        case 0x7D2E9E31:
            sprintf(pch, "U.S.A. TOS 1.02 L0");
            break;

        case 0x8E360AD0:
            sprintf(pch, "U.S.A. TOS 1.02 H1");
            break;

        case 0x85C5A7A7:
            sprintf(pch, "U.S.A. TOS 1.02 L1");
            break;

        case 0x85E68623:
            sprintf(pch, "U.S.A. TOS 1.02 H2");
            break;

        case 0x7F549B93:
            sprintf(pch, "U.S.A. TOS 1.02 L2");
            break;

        case 0x9240DCAB:
            sprintf(pch, "U.S.A. TOS 1.04 H0");
            break;

        case 0x7CBB9A6E:
            sprintf(pch, "U.S.A. TOS 1.04 L0");
            break;

        case 0x853179FE:
            sprintf(pch, "U.S.A. TOS 1.04 H1");
            break;

        case 0x806A55D4:
            sprintf(pch, "U.S.A. TOS 1.04 L1");
            break;

        case 0x8FC799E0:
            sprintf(pch, "U.S.A. TOS 1.04 H2");
            break;

        case 0x8147A9A7:
            sprintf(pch, "U.S.A. TOS 1.04 L2");
            break;

        case 0x8C2C02BD:
            sprintf(pch, "U.S.A. TOS 2.06 EVEN");
            break;

        case 0x79CAD0A6:
            sprintf(pch, "U.S.A. TOS 2.06 ODD");
            break;

        case 0x8C299942:
            sprintf(pch, "U.K.   TOS 2.06 EVEN");
            break;

        case 0x79CB2E1F:
            sprintf(pch, "U.K.   TOS 2.06 ODD");
            break;

        case 0x8CE93420:
            sprintf(pch, "French TOS 2.06 EVEN");
            break;

        case 0x7A6D4002:
            sprintf(pch, "French TOS 2.06 ODD");
            break;

        case 0x8CB070DF:
            sprintf(pch, "German TOS 2.06 EVEN");
            break;

        case 0x79E750E5:
            sprintf(pch, "German TOS 2.06 ODD");
            break;

        case 0x4A20204A:
            sprintf(pch, "U.S.A. TOS 3.05 EE");
            break;

        case 0x3FE8FB8D:
            sprintf(pch, "U.S.A. TOS 3.05 OE");
            break;

        case 0x49BFC307:
            sprintf(pch, "U.S.A. TOS 3.05 EO");
            break;

        case 0x4057B7C8:
            sprintf(pch, "U.S.A. TOS 3.05 OO");
            break;

        case 0x4F038778:
            sprintf(pch, "U.S.A. TOS 3.06 EE");
            break;

        case 0x455CAE4D:
            sprintf(pch, "U.S.A. TOS 3.06 OE");
            break;

        case 0x501F1614:
            sprintf(pch, "U.S.A. TOS 3.06 EO");
            break;

        case 0x45577695:
            sprintf(pch, "U.S.A. TOS 3.06 OO");
            break;

        case 0x8885F1C9:
            sprintf(pch, "U.S.A. TOS 2.05 EE");
            break;

        case 0x75C1470C:
            sprintf(pch, "U.S.A. TOS 2.05 EO");
            break;

        case 0x8B1301F8:
            sprintf(pch, "Mac 512 64K HI");
            break;

        case 0x81A38EE7:
            sprintf(pch, "Mac 512 64K LO");
            break;

        case 0x8B13D0C1:
            sprintf(pch, "Mac 512 64K HI");
            break;

        case 0x81A2A0F1:
            sprintf(pch, "Mac 512 64K LO");
            break;

        case 0x8E91A10D:
            sprintf(pch, "Mac Plus 128K HI-C");
            break;

        case 0x852C8B50:
            sprintf(pch, "Mac Plus 128K LO-C");
            break;

        case 0x8E918658:
            sprintf(pch, "Mac Plus 128K HI-B");
            break;

        case 0x852D38D4:
            sprintf(pch, "Mac Plus 128K LO-B");
            break;

        case 0x8C28C517:
            sprintf(pch, "Mac SE HI (800K-A)");
            break;

        case 0x83E78152:
            sprintf(pch, "Mac SE LO (800K-A)");
            break;

        case 0x8454D95B:
            sprintf(pch, "Mac SE HI (800K-B)");
            break;

        case 0x7F2964F8:
            sprintf(pch, "Mac SE LO (800K-B)");
            break;

        case 0x8457B695:
            sprintf(pch, "Mac SE HI (HDFD)");
            break;

        case 0x7FC8C963:
            sprintf(pch, "Mac SE LO (HDFD)");
            break;

        case 0x875A3B20:
            sprintf(pch, "Mac Classic");
            break;

        // Mac II revision A
        // part numbers 105 106 107 108

        case 0x8F861BD9:
            sprintf(pch, "Mac II 256K HH-A");
            break;

        case 0x8882C543:
            sprintf(pch, "Mac II 256K MH-A");
            break;

        case 0x8F4C4E3C:
            sprintf(pch, "Mac II 256K ML-A");
            break;

        case 0x88250EC3:
            sprintf(pch, "Mac II 256K LL-A");
            break;

        // Mac II revision B
        // part numbers 105 106 107 108

        case 0x8F838A3E:
            sprintf(pch, "Mac II 256K HH-B");
            break;

        case 0x88866B47:
            sprintf(pch, "Mac II 256K MH-B");
            break;

        case 0x8F53A8EF: // ????

        case 0x8F53A83F:
            sprintf(pch, "Mac II 256K ML-B");
            break;

        case 0x8826D0E7:
            sprintf(pch, "Mac II 256K LL-B");
            break;

        // Mac IIcx /SE/30 revision C
        // part numbers 639 640 641 642

        case 0x8FA8CDF9:
            sprintf(pch, "Mac IIcx 256K HH");
            break;

        case 0x888D2254:
            sprintf(pch, "Mac IIcx 256K MH");
            break;

        case 0x8F718890:
            sprintf(pch, "Mac IIcx 256K ML");
            break;

        case 0x881AAD9F:
            sprintf(pch, "Mac IIcx 256K LL");
            break;

        // Mac IIci
        // part numbers 733 734 735 736

        case 0x8DFDD10C:
            sprintf(pch, "Mac IIci 512K HH");
            break;

        case 0x86B32B88:
            sprintf(pch, "Mac IIci 512K MH");
            break;

        case 0x8DAC9715:
            sprintf(pch, "Mac IIci 512K ML");
            break;

        case 0x86DADA8C:
            sprintf(pch, "Mac IIci 512K LL");
            break;

        // Mac LC
        // part numbers 397 396 395 394

        case 0x8E6CAA3F:
            sprintf(pch, "Mac LC 512K HH");
            break;

        case 0x8668595A:
            sprintf(pch, "Mac LC 512K MH");
            break;

        case 0x8DC1CF53:
            sprintf(pch, "Mac LC 512K ML");
            break;

        case 0x8700E553:
            sprintf(pch, "Mac LC 512K LL");
            break;

        // Mac LC II
        // part numbers 476 475 474 473

        case 0x8E48AAA1:
            sprintf(pch, "Mac LC II 512K HH");
            break;

        case 0x864A21C2:
            sprintf(pch, "Mac LC II 512K MH");
            break;

        case 0x8DA80FB1:
            sprintf(pch, "Mac LC II 512K ML");
            break;

        case 0x86E47249:
            sprintf(pch, "Mac LC II 512K LL");
            break;
            }

        pch = rgch + strlen(rgch);
        sprintf(pch, ":");

        for (i = 0; i < sizeof(rgb); i++)
            {
            pch = rgch + strlen(rgch);
            sprintf(pch, " %02X", rgb[i]);
            }

        SendDlgItemMessage(hDlg, IDC_CHECKSUMLIST, LB_ADDSTRING, 0, (LPARAM)rgch);
        }

    SetCursor(h);
}


//
// Poor man's sscanf for hex numbers
//

BOOL FParseHexNum(BYTE const *pch, int *pi)
{
    *pi = 0;

    while (*pch)
        {
        *pi <<= 4;

        if ((*pch >= '0') && (*pch <= '9'))
            *pi += (*pch - '0');
        else if ((*pch >= 'A') && (*pch <= 'F'))
            *pi += (*pch - 'A') + 10;
        else if ((*pch >= 'a') && (*pch <= 'f'))
            *pi += (*pch - 'f') + 10;
        else
            return FALSE;

        pch++;
        }

    return TRUE;
}


LRESULT CALLBACK CheckSumProc(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    char rgch[256];
    int  i;

// printf("CheckSumProc: %08X %08X %08X %08X\n", hDlg, message, uParam, lParam);

    switch (message)
        {
        case WM_INITDIALOG:
            // initialize dialog box

            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

            sprintf(rgch, "%3X", v.ioBaseAddr);
            SetDlgItemText(hDlg, IDC_IOPORT, rgch);
            sprintf(rgch, "%1X", v.cCards);
            SetDlgItemText(hDlg, IDC_NUMBERCARDS, rgch);

            UpdateDIPs(hDlg, v.ioBaseAddr);

            return (TRUE);

        case WM_COMMAND:
            // item notifications

            switch (LOWORD(uParam))     // switch on the dialog item
                {
            default:
                break;

            case IDC_IOPORT:
            case IDC_NUMBERCARDS:
                if (HIWORD(uParam) == EN_SETFOCUS)
                    {
                    // Hilight text in edit fields when they get focus

#if 0
                    // This doesn't work, see:
                    // http://support.microsoft.com/support/kb/articles/q96/6/74.asp

                    SendDlgItemMessage(hDlg, LOWORD(uParam), EM_SETSEL, 0, -1);
#else
                    // This works

                    PostMessage(GetDlgItem(hDlg, LOWORD(uParam)), EM_SETSEL, 0, -1);
#endif
                    }
                break;

            case IDCANCEL:
            case IDOK:
                   EndDialog(hDlg, TRUE);
                return (TRUE);
                break;

            case IDSCAN:

                // verify the i/o port base address

                GetDlgItemText(hDlg, IDC_IOPORT, rgch, sizeof(rgch));
                DebugStr("%s\n", rgch);

                if ((strlen(rgch) < 1) || (strlen(rgch) > 3) ||
                      !FParseHexNum(rgch, &i) ||
                      (i <= 0) || (i > 0x3FF))
                    {
                    HANDLE h;

                    // invalid string, restore old one
                    sprintf(rgch, "%3X", v.ioBaseAddr);
                    SetDlgItemText(hDlg, IDC_IOPORT, rgch);

                    MessageBox(h = GetFocus(),
#if _ENGLISH
                        "Base Address must be in the range 0 to 3FC (hex)",
#elif _NEDERLANDS
                        "Het start-adres moet zich tussen 0 to 3FC (hex) bevinden",
#elif _DEUTSCH
                        "Base Adresse muP zwischen 0 und 3FC (hex) liegen",
#elif _FRANCAIS
                        "L'adresse de base doit être comprise entre 0 et 3FC (hex)",
#elif
#error
#endif
                        vi.szAppName, MB_OK|MB_ICONHAND);
                    SetFocus(h);
                    return FALSE;
                    }

                // verify the count of cards

                GetDlgItemText(hDlg, IDC_NUMBERCARDS, rgch, sizeof(rgch));
                DebugStr("%s\n", rgch);

                if ((strlen(rgch) != 1) ||
                      (rgch[0] < '0') || (rgch[0] > '9'))
                    {
                    HANDLE h;

                    // invalid string, restore old one
                    sprintf(rgch, "%d", v.cCards);
                    SetDlgItemText(hDlg, IDC_NUMBERCARDS, rgch);

                    (h = GetFocus(),
#if _ENGLISH
                        "Number of ROM cards must be in the range 0 to 9",
#elif _NEDERLANDS
                        "Het aantal Gemulator-kaarten is minimaal 0 en maximaal 9",
#elif _DEUTSCH
                        "Die Anzahl der ROM Karten muP zwischen 0 und 9 liegen",
#elif _FRANCAIS
                        "Le nombre de cartes ROM doit être compris entre 0 et 9",
#elif
#error
#endif
                        vi.szAppName, MB_OK|MB_ICONHAND);
                    SetFocus(h);
                    return FALSE;
                    }

                UpdateDIPs(hDlg, i);

                v.ioBaseAddr = i;
                v.cCards = rgch[0] - '0';

                CalcCheckSum(hDlg);
//                return (TRUE);
                break;
                }
        }

    return (FALSE); // Didn't process the message

    lParam; // This will prevent 'unused formal parameter' warnings
}


void CheckSum(HWND hDlg)
{
    // disable the caller

    EnableWindow(hDlg,FALSE);

    DialogBox(vi.hInst,
        MAKEINTRESOURCE(IDD_CHECKSUM),
        hDlg,
        CheckSumProc);

    // re-enable the caller

    EnableWindow(hDlg,TRUE);
}



//
// Given the current v.ioBaseAddr and v.cCards, scan for ROMs on all
// Gemulator cards and disk based OSes and fill in the rgosinfo structure.
//
// Returns number of valid OSes detected.
//

void AddROMsToVM()
{
    int i, j;

    for (i = 0; i < MAX_VM; i++)
        {
        for (j = 0; j < MAXOSUsable; j++)
            {
            if (v.rgvm[i].fValidVM &&
                (v.rgvm[i].pvmi->wfROM & v.rgosinfo[j].osType))
                if (v.rgvm[i].iOS == -1)
                    v.rgvm[i].iOS = j;
            }
        }
}

int WScanROMs(BOOL fScanROMs, BOOL fScanMagiC, BOOL fScanFloppy, BOOL fScanNet)
{
    unsigned iChip;      /* current starting socket # (0..7) */
    HCURSOR h = SetCursor(LoadCursor(NULL,IDC_WAIT));
//    unsigned char rgrgb[cScktMax][256];
    char rgch[MAX_PATH];

    v.cOS = 0;

    {
    int i;

    for (i = 0; i < MAX_VM; i++)
        v.rgvm[i].iOS = -1;
    }

    // clear ROM info, but don't clear global path area

    memset(&v.rgosinfo, 0, sizeof(OSINFO) * (MAXOSUsable));

    // fake in the Atari 8-bit entries

    v.rgosinfo[v.cOS++].osType  = osAtari48;
    v.rgosinfo[v.cOS++].osType  = osAtariXL;
    v.rgosinfo[v.cOS++].osType  = osAtariXE;

    if (!fScanROMs)
        goto LscanMagic;

    iChip = 0;

    for (iChip = 0; iChip < v.cCards*cScktMax; )
        {
#if 0
printf("wScanROMs: iChip = %d\n", iChip);
#endif

        if ((iChip & wScktMsk) == 0)
            {
            vi.ioPort = v.ioBaseAddr + (iChip / cScktMax)*4;
            GetCardSize(/* rgrgb */);
            OutPort(0);
            }
        else if ((iChip & wScktMsk) >= cSockets)
            {
            iChip += (cScktMax - (iChip & wScktMsk));
            continue;
            }
#if 0
printf("wScanROMs: ioPort = %d\n", vi.ioPort);
printf("wScanROMs: cbSocket = %d\n", cbSocket);
printf("wScanROMs: cSockets = %d\n", cSockets);
#endif

#if _ENGLISH
        sprintf(rgch, "Scanning Gemulator card on port $%03X, socket U%d...", vi.ioPort, (iChip & wScktMsk)+1);
#elif _NEDERLANDS
        sprintf(rgch, "Zoek naar TOS ROMs op Gemulator kaart(en) port $%03X slot U%d...",vi.ioPort,(iChip&wScktMsk)+1);
#elif _DEUTSCH
        sprintf(rgch, "Suche nach TOS ROMs auf Gemulator 95 Karte...", vi.ioPort, (iChip & wScktMsk)+1);
#elif _FRANCAIS
        sprintf(rgch, "Cherche le ROMs sur carte Gemulator...", vi.ioPort, (iChip & wScktMsk)+1);
#elif
#error
#endif
        SetWindowText(vi.hWnd, rgch);

        v.rgosinfo[v.cOS].cbSocket = cbSocket;
        v.rgosinfo[v.cOS].cSockets = cSockets;

        switch(InPort())
            {
        default:
            // Check to see if Mac ROMs

            if ((InPort() & ~0x40) == 0x00) // 0x00 or 0x40
                {
                switch (InPort())
                    {
                case 0x01:
                    // possible 4-chip 256K Mac II ROM set

                    SkipROMs(cbSocket - 1);

                    v.rgosinfo[v.cOS].ioPort  = vi.ioPort;
                    v.rgosinfo[v.cOS].version = InPort();
                    v.rgosinfo[v.cOS].date    = 0;
                    v.rgosinfo[v.cOS].iChip   = iChip;
                    v.rgosinfo[v.cOS].cChip   = 4;
                    v.rgosinfo[v.cOS].country = 0;
                    v.rgosinfo[v.cOS].eaOS    = 0x40800000;
                    v.rgosinfo[v.cOS].osType  = osMac_II;
                    v.rgosinfo[v.cOS].cbOS    = 0x040000;

                    if (v.rgosinfo[v.cOS].version >= 124)
                        {
                        SkipROMs(cbSocket*3 - 3);
                        iChip += 4;
                        v.cOS++;
                        goto Lfound;
                        }

                    else if (v.rgosinfo[v.cOS].version >= 120)
                        {
                        SkipROMs(cbSocket*3 - 3);
                        iChip += 4;
                        v.cOS++;
                        goto Lfound;
                        }
                    else
                        {
                        SkipROMs(cbSocket - 3);
                        goto Lcart;
                        }

                case 0x06:
                    // possible 4-chip 512K Mac II ROM set (e.g. Mac IIci)

                    SkipROMs(cbSocket - 1);

                    v.rgosinfo[v.cOS].ioPort  = vi.ioPort;
                    v.rgosinfo[v.cOS].version = InPort();
                    v.rgosinfo[v.cOS].date    = 0;
                    v.rgosinfo[v.cOS].iChip   = iChip;
                    v.rgosinfo[v.cOS].cChip   = 4;
                    v.rgosinfo[v.cOS].country = 0;
                    v.rgosinfo[v.cOS].eaOS    = 0x40800000;
                    v.rgosinfo[v.cOS].osType  = osMac_IIi;
                    v.rgosinfo[v.cOS].cbOS    = 0x080000;

                    if (v.rgosinfo[v.cOS].version >= 124)
                        {
                        SkipROMs(cbSocket*3 - 3);
                        iChip += 4;
                        v.cOS++;
                        goto Lfound;
                        }

                    else if (v.rgosinfo[v.cOS].version >= 120)
                        {
                        SkipROMs(cbSocket*3 - 3);
                        iChip += 4;
                        v.cOS++;
                        goto Lfound;
                        }
                    else
                        {
                        SkipROMs(cbSocket - 3);
                        goto Lcart;
                        }

                default:
                    SkipROMs(cbSocket*2 - 2);
                    goto Lcart;
                    }
                }

            InPort();
            if (InPort() != 0x00)
                SkipROMs(cbSocket*2 - 4);
            else
                {
                SkipROMs(cbSocket - 1);

                if (InPort() == 0x2A)
                    {
                    // possible Mac ROMs

                    v.rgosinfo[v.cOS].ioPort  = vi.ioPort;
                    v.rgosinfo[v.cOS].version = InPort();
                    v.rgosinfo[v.cOS].date    = 0;
                    v.rgosinfo[v.cOS].iChip   = iChip;
                    v.rgosinfo[v.cOS].cChip   = 2;
                    v.rgosinfo[v.cOS].country = 0;
                    v.rgosinfo[v.cOS].eaOS    = 0x400000;

                    if (v.rgosinfo[v.cOS].version >= 118)
                        {
                        v.rgosinfo[v.cOS].osType  = osMac_256;
                        v.rgosinfo[v.cOS].cbOS    = 0x040000;
                        }

                    else if (v.rgosinfo[v.cOS].version >= 110)
                        {
                        v.rgosinfo[v.cOS].osType  = osMac_128;
                        v.rgosinfo[v.cOS].cbOS    = 0x020000;
                        }

                    else
                        {
                        v.rgosinfo[v.cOS].osType  = osMac_64;
                        v.rgosinfo[v.cOS].cbOS    = 0x010000;
                        }

                    v.cOS++;
                    }
                else
                    InPort();

                SkipROMs(cbSocket-5);
                }
Lcart:
            iChip += 2;
Lfound:
            break;

        case 0xFF:
            // emtry socket, advance two sockets.
            SkipROMs(cbSocket*2-1);
            iChip += 2;
            break;

        case 0x60:
            {
            // Assume start of Atari TOS ROMs

            int version = 0;
            int date    = 0;
            int cChip   = 0;
            int country = 0;

            version = InPort() << 8;

            if (version == 0x00)
                {
                // possible 4-chip TT ROMs

                SkipROMs(4);

                date |= InPort() << 24;
                SkipROMs(cbSocket-7);

                if (InPort() != 0x2E)
                    {
                    SkipROMs(cbSocket-1);
                    goto Lcart;
                    }

                SkipROMs(5);
                date |= InPort() << 16;
                SkipROMs(cbSocket-7);

                version = InPort() << 8;
                SkipROMs(5);
                date |= InPort() << 8;
                SkipROMs(cbSocket-7);

                version |= InPort();
                SkipROMs(5);
                date |= InPort();
                SkipROMs(cbSocket-7);

                v.rgosinfo[v.cOS].ioPort  = vi.ioPort;
                v.rgosinfo[v.cOS].version = version;
                v.rgosinfo[v.cOS].date    = date;
                v.rgosinfo[v.cOS].iChip   = iChip;
                v.rgosinfo[v.cOS].cChip   = 4;
                v.rgosinfo[v.cOS].country = country;
                v.rgosinfo[v.cOS].osType  = osTOS3X_4;
                v.rgosinfo[v.cOS].eaOS = 0xE00000;
                v.rgosinfo[v.cOS].cbOS = 0x080000;

                iChip += 4;
                v.cOS++;
                break;
                }

            SkipROMs(10);
            date |= InPort() << 24;
            date |= InPort() << 8;
            SkipROMs(32768-14);

            // check if 32769th byte is a duplicate of first byte
            cChip  = (InPort() == 0x60) ? 6 : ((version >= 0x300) ? 4 : 2);
            SkipROMs(cbSocket-32768);
            version |= InPort();
            SkipROMs(10);
            date |= InPort() << 16;
            date |= InPort();
            country = ((InPort() >> 1) & 31);

            // Assume they're valid, so fill in while ioPort is valid
            v.rgosinfo[v.cOS].ioPort  = vi.ioPort;
            v.rgosinfo[v.cOS].version = version;
            v.rgosinfo[v.cOS].date    = date;
            v.rgosinfo[v.cOS].iChip   = iChip;
            v.rgosinfo[v.cOS].cChip   = cChip;
            v.rgosinfo[v.cOS].country = country;

            if (version >= 0x300 && cChip == 4)
                {
                v.rgosinfo[v.cOS].osType  = osTOS3X_4;
                v.rgosinfo[v.cOS].eaOS = 0xE00000;
                v.rgosinfo[v.cOS].cbOS = 0x080000;
                }
            else if (version >= 0x200 && cChip == 2)
                {
                v.rgosinfo[v.cOS].osType  = osTOS2X_2;
                v.rgosinfo[v.cOS].eaOS = 0xE00000;
                v.rgosinfo[v.cOS].cbOS = 0x040000;
                }
            else if (version >= 0x106 && cChip == 2)
                {
                v.rgosinfo[v.cOS].osType  = osTOS1X_2;
                v.rgosinfo[v.cOS].eaOS = 0xFC0000;
                v.rgosinfo[v.cOS].cbOS = 0x030000;
                }
            else if (version >= 0x100 && cChip == 6)
                {
                v.rgosinfo[v.cOS].osType  = osTOS1X_6;
                v.rgosinfo[v.cOS].eaOS = 0xFC0000;
                v.rgosinfo[v.cOS].cbOS = 0x030000;
                }

            // skip to the end of 2nd chip

            SkipROMs(cbSocket - 15);

            // If bogus, treat as a cartridge

            if ((date & 0x0000FF00) != 0x00001900)
                goto Lcart;

            // If not bogus, skip to end of TOS ROMs

            SkipROMs(((cChip-2) * cbSocket));
            iChip += cChip;

            v.cOS++;
            break;
            }
            }
//        break;
        }

LscanMagic:

    if (fScanMagiC)
        {
        int i;
        HANDLE hFind;
        WIN32_FIND_DATA fd;
        
        for (i = 0; i < 28; i++)
            {
            OFSTRUCT ofs;
            char sz[MAX_PATH] = "A:\\";
            int type;

            if (v.cOS >= (MAXOSUsable))
                break;

            sz[0] = 'A' + i;

            if (i == 26)
                {
                // use the saved path

                strcpy(sz, (char *)&v.rgchGlobalPath);

                // don't allow blank or root level path

                if (strlen(sz) < 4)
                    continue;

                // if global directory is simply the program directory

                if (!strcmp(sz, vi.szDefaultDir))
                    continue;

                strcat(sz, "\\");
                }
            else if (i == 27)
                {
                // use the current default

                strcpy(sz, vi.szDefaultDir);

                // don't allow blank or root level path

                if (strlen(sz) < 4)
                    continue;

                strcat(sz, "\\");
                }
            else
                {
                type = GetDriveType(sz);

                if (type == DRIVE_NO_ROOT_DIR)
                    continue;
    
                if (type == DRIVE_CDROM)
                    continue;
    
                if (type == DRIVE_REMOTE && !fScanNet)
                        continue;
    
                if (type == DRIVE_REMOVABLE && !fScanFloppy)
                    continue;

                }

            DebugStr("i = %d, cOS = %d, sz = %s\n", i, v.cOS, sz);

            if (!SetCurrentDirectory(sz))
                continue;

            if (i == 26)
                sz[0] = 0;              // global directory is encoded funny
            else if (i == 27)
                strcpy(sz, ".\\");      // current directory

            DebugStr("i = %d, cOS = %d, sz = %s\n", i, v.cOS, sz);

            // search for loadable MagiC 2.x or 4.x image

            if (((HFILE)h = _lopen(szMagicRAM, OF_READ)) != HFILE_ERROR)
                {
                int cbRead;
                BYTE rgb[256];

                ReadFile(h, rgb, 256, &cbRead, NULL);

                strcpy(v.rgosinfo[v.cOS].szImg, sz);
                strcat(v.rgosinfo[v.cOS].szImg, &szMagicRAM[2]);
                v.rgosinfo[v.cOS].version = 0x0200;
                v.rgosinfo[v.cOS].date    = 0x04271996;
                v.rgosinfo[v.cOS].eaOS = 0xFC0000; // hack in what we want
                v.rgosinfo[v.cOS].cbOS = GetFileSize(h, NULL);
                v.rgosinfo[v.cOS].country = 0;
                v.rgosinfo[v.cOS].osType  = osMagic_2;

                v.cOS++;
                CloseHandle(h);
                }

            // search for Magic PC image

            if (((HFILE)h = OpenFile(szMagicPC, &ofs, OF_READ)) != HFILE_ERROR)
                {
                int cbRead;
                BYTE rgb[256];

                ReadFile(h, rgb, 256, &cbRead, NULL);

                strcpy(v.rgosinfo[v.cOS].szImg, sz);
                strcat(v.rgosinfo[v.cOS].szImg, &szMagicPC[2]);
                v.rgosinfo[v.cOS].version = (rgb[2] << 8) | rgb[3];
                v.rgosinfo[v.cOS].date    = (rgb[24] << 24) | (rgb[25] << 16) |
                                            (rgb[26] << 8) | rgb[27];
                v.rgosinfo[v.cOS].eaOS = 0xE00000;
                v.rgosinfo[v.cOS].cbOS = 0x040000;
                v.rgosinfo[v.cOS].country = 0;
                v.rgosinfo[v.cOS].osType  = osMagicPC;

                v.cOS++;
                CloseHandle(h);
                }

            // search for TOS 1.0/1.4 boot disk image

            if (((HFILE)h = OpenFile(szTOS_RAM, &ofs, OF_READ)) != HFILE_ERROR)
                {
                int cbRead;
                BYTE rgb[256];

                ReadFile(h, rgb, 256, &cbRead, NULL);

                if (rgb[0] == 0x46 && rgb[1] == 0xFC)
                    ReadFile(h, rgb, 256, &cbRead, NULL);  // skip header

                strcpy(v.rgosinfo[v.cOS].szImg, sz);
                strcat(v.rgosinfo[v.cOS].szImg, &szTOS_RAM[2]);
                v.rgosinfo[v.cOS].version = (rgb[2] << 8) | rgb[3];
                v.rgosinfo[v.cOS].date    = (rgb[24] << 24) | (rgb[25] << 16) |
                                            (rgb[26] << 8) | rgb[27];
                v.rgosinfo[v.cOS].eaOS = (rgb[8] << 24) | (rgb[9] << 16) |
                                            (rgb[10] << 8) | rgb[11];
                v.rgosinfo[v.cOS].eaOS = 0xFC0000; // hack in what we want
                v.rgosinfo[v.cOS].cbOS = 0x030000;
                v.rgosinfo[v.cOS].country = 0;
                v.rgosinfo[v.cOS].osType  = osTOS1X_D;

                if (GetFileSize(h, NULL) == 0x040000)
                    {
                    v.rgosinfo[v.cOS].eaOS = 0xE00000;
                    v.rgosinfo[v.cOS].cbOS = 0x040000;
                    v.rgosinfo[v.cOS].osType  = osTOS2X_D;
                    }

                else if (GetFileSize(h, NULL) == 0x080000)
                    {
                    v.rgosinfo[v.cOS].eaOS = 0xE00000;
                    v.rgosinfo[v.cOS].cbOS = 0x080000;
                    v.rgosinfo[v.cOS].osType  = osTOS3X_D;
                    }

                v.cOS++;
                CloseHandle(h);
                }

            // search for EmuTOS ROM image

            hFind = FindFirstFile(szEMU_TOS, &fd);

            while ((hFind != INVALID_HANDLE_VALUE) &&
              strcpy(rgch, sz) &&
              strcat(rgch, fd.cFileName) &&
              (((HFILE)h = OpenFile(rgch, &ofs, OF_READ))
               != HFILE_ERROR))
                {
                int cbRead;
                BYTE rgb[256];

                ReadFile(h, rgb, 256, &cbRead, NULL);

                strcpy(v.rgosinfo[v.cOS].szImg, rgch);

                v.rgosinfo[v.cOS].version = rgb[9];
                v.rgosinfo[v.cOS].date    = 0;
                v.rgosinfo[v.cOS].country = 0;
                v.rgosinfo[v.cOS].eaOS    = 0x400000;
                v.rgosinfo[v.cOS].cbOS = GetFileSize(h, NULL);

                // search for Atari TOS ROMs first

                if ((rgb[0] == 0x60) && (rgb[4] == 0x00) &&
                    (rgb[6] == 0x00) && (rgb[8] == 0x00) &&
                   ((rgb[1] == 0x1E) || (rgb[1] == 0x2E)))
                    {
                    v.rgosinfo[v.cOS].version = (rgb[2] << 8) | rgb[3];

                    // EmuTOS ROM is backward from normal TOS

                    v.rgosinfo[v.cOS].date    = (rgb[26] << 24) | (rgb[27] << 16) |
                                                (rgb[24] << 8) | rgb[25];
                    v.rgosinfo[v.cOS].eaOS = (rgb[8] << 24) | (rgb[9] << 16) |
                                                (rgb[10] << 8) | rgb[11];
                    v.rgosinfo[v.cOS].eaOS = 0xFC0000; // hack in what we want
                    v.rgosinfo[v.cOS].cbOS = 0x030000;
                    v.rgosinfo[v.cOS].country = 0;
                    v.rgosinfo[v.cOS].osType  = osTOS1X_D;

                    if (GetFileSize(h, NULL) == 0x040000)
                        {
                        v.rgosinfo[v.cOS].eaOS = 0xE00000;
                        v.rgosinfo[v.cOS].cbOS = 0x040000;
                        v.rgosinfo[v.cOS].osType  = osTOS2X_D;
                        }

                    else if (GetFileSize(h, NULL) == 0x080000)
                        {
                        v.rgosinfo[v.cOS].eaOS = 0xE00000;
                        v.rgosinfo[v.cOS].cbOS = 0x080000;
                        v.rgosinfo[v.cOS].osType  = osTOS3X_D;
                        }

                    v.cOS++;
                    }

                CloseHandle(h);

                if (v.cOS >= (MAXOSUsable))
                    break;

                if (!FindNextFile(hFind, &fd))
                    break;
                }

            FindClose(hFind);

            // search for Mac 512/Plus/SE ROM image

            hFind = FindFirstFile(szMAC_RAM, &fd);

            while ((hFind != INVALID_HANDLE_VALUE) &&
              strcpy(rgch, sz) &&
              strcat(rgch, fd.cFileName) &&
              (((HFILE)h = OpenFile(rgch, &ofs, OF_READ))
               != HFILE_ERROR))
                {
                int cbRead;
                BYTE rgb[256];

                ReadFile(h, rgb, 256, &cbRead, NULL);

                strcpy(v.rgosinfo[v.cOS].szImg, rgch);

                v.rgosinfo[v.cOS].version = rgb[9];
                v.rgosinfo[v.cOS].date    = 0;
                v.rgosinfo[v.cOS].country = 0;
                v.rgosinfo[v.cOS].eaOS    = 0x400000;
                v.rgosinfo[v.cOS].cbOS = GetFileSize(h, NULL);

                // search for Atari TOS ROMs first

                if ((rgb[0] == 0x60) && (rgb[4] == 0x00) &&
                    (rgb[6] == 0x00) && (rgb[8] == 0x00) &&
                   ((rgb[1] == 0x1E) || (rgb[1] == 0x2E)))
                    {
                    v.rgosinfo[v.cOS].version = (rgb[2] << 8) | rgb[3];
                    v.rgosinfo[v.cOS].date    = (rgb[24] << 24) | (rgb[25] << 16) |
                                                (rgb[26] << 8) | rgb[27];
                    v.rgosinfo[v.cOS].eaOS = (rgb[8] << 24) | (rgb[9] << 16) |
                                                (rgb[10] << 8) | rgb[11];
                    v.rgosinfo[v.cOS].eaOS = 0xFC0000; // hack in what we want
                    v.rgosinfo[v.cOS].cbOS = 0x030000;
                    v.rgosinfo[v.cOS].country = 0;
                    v.rgosinfo[v.cOS].osType  = osTOS1X_D;

                    if (GetFileSize(h, NULL) == 0x040000)
                        {
                        v.rgosinfo[v.cOS].eaOS = 0xE00000;
                        v.rgosinfo[v.cOS].cbOS = 0x040000;
                        v.rgosinfo[v.cOS].osType  = osTOS2X_D;
                        }

                    else if (GetFileSize(h, NULL) == 0x080000)
                        {
                        v.rgosinfo[v.cOS].eaOS = 0xE00000;
                        v.rgosinfo[v.cOS].cbOS = 0x080000;
                        v.rgosinfo[v.cOS].osType  = osTOS3X_D;
                        }

                    v.cOS++;
                    goto LnotMac;
                    }

                // everything else has to be Mac

                if ((rgb[6] != 0x00) || (rgb[7] != 0x2A))
                    {
                    goto LnotMac;
                    }

                if (v.rgosinfo[v.cOS].version >= 125)
                    {
                    if (GetFileSize(h, NULL) == 0x200000)
                        goto lQdra;

                    v.rgosinfo[v.cOS].osType  = osMac_G1;
                    }

                else if (v.rgosinfo[v.cOS].version >= 124)
                    {
                    v.rgosinfo[v.cOS].osType  = osMac_IIi;
                    v.rgosinfo[v.cOS].eaOS    = 0x40800000;
                    v.rgosinfo[v.cOS].cbOS    = 0x080000;

                    if (GetFileSize(h, NULL) == 0x100000)
                        {
                        v.rgosinfo[v.cOS].cbOS    = 0x100000;
                        v.rgosinfo[v.cOS].osType  = osMac_1M;
                        }

                    // Mac IIvi ROMs don't work in Mac II mode

                    if ((rgb[0x12] == 0x20) && (rgb[0x13] == 0xF2))
                        goto lQdra;

                    if (GetFileSize(h, NULL) == 0x200000)
                        {
lQdra:
                        v.rgosinfo[v.cOS].cbOS    = GetFileSize(h, NULL);
                        v.rgosinfo[v.cOS].osType  = osMac_2M;
                        }
                    }

                else if (v.rgosinfo[v.cOS].version >= 120)
                    {
                    v.rgosinfo[v.cOS].osType  = osMac_II;
                    v.rgosinfo[v.cOS].eaOS    = 0x40800000;
                    v.rgosinfo[v.cOS].cbOS    = 0x040000;
                    }

                else if (v.rgosinfo[v.cOS].version >= 118)
                    {
                    v.rgosinfo[v.cOS].osType  = osMac_256;
                    v.rgosinfo[v.cOS].cbOS    = 0x040000;

                    // Check for Mac Classic ROM image, which is the regular
                    // 256K of Mac SE ROM followed by 256K of boot disk

                    if (GetFileSize(h, NULL) == 0x080000)
                        v.rgosinfo[v.cOS].cbOS    = 0x080000;
                    }

                else if (v.rgosinfo[v.cOS].version >= 110)
                    {
                    v.rgosinfo[v.cOS].osType  = osMac_128;
                    v.rgosinfo[v.cOS].cbOS    = 0x020000;
                    }

                else
                    {
                    v.rgosinfo[v.cOS].osType  = osMac_64;
                    v.rgosinfo[v.cOS].cbOS    = 0x010000;
                    }

                v.cOS++;

LnotMac:
                CloseHandle(h);

                if (v.cOS >= (MAXOSUsable))
                    break;

                if (!FindNextFile(hFind, &fd))
                    break;
                }

            FindClose(hFind);
            }

        SetCurrentDirectory(vi.szDefaultDir);
        }

    AddROMsToVM();

    SetCursor(h);
    
    return v.cOS;
}


int CbReadROMImage(char *szImg, ULONG cb, ULONG addr)
{
    char sz[262];

    if (szImg[0] == '.')
        strcpy(sz, szImg);

    else if (szImg[0] == '\\')
        strcpy(sz, szImg);

    else if (v.rgosinfo[vmCur.iOS].szImg[2] != '\\')
        {
        strcpy(sz, (char *)&v.rgchGlobalPath);
        strcat(sz, "\\");
        strcat(sz, szImg);
        }

    else
        strcpy(sz, szImg);

    return CbReadWholeFileToGuest(sz, cb, addr, ACCESS_INITHW);
}


//
// load MAGIC 2.0 from disk
//

void ReadMagiC(char *sz)
{
    unsigned cb = 262144;    // MAGIC.RAM won't be bigger than 256K
    unsigned buf = 0x040000; // arbitrary load address
    unsigned addrMagic;
    unsigned lLoadAddr;      // actual start of MagiC in memory

    if (!CbReadROMImage(sz, cb, buf))
        return;

#ifdef LATER
    addrMagic = vmPeekL(buf+0x30) + buf + 28;
    vmPeekL(addrMagic);        // magic number $87654321
    lLoadAddr = vmPeekL(addrMagic+4);        // load address $7C2C
    addrMagic+=4;
    *(DWORD *)memPvGetFlatAddress(addrMagic, sizeof(DWORD)) =
        lLoadAddr + vmPeekL(buf+2) + vmPeekL(buf+6);

    // override the serial number
    *(BYTE *)MapAddressVM(addrMagic-144) = 0x12;
    *(BYTE *)MapAddressVM(addrMagic-143) = 0x34;
    *(BYTE *)MapAddressVM(addrMagic-142) = 0x56;
    *(BYTE *)MapAddressVM(addrMagic-141) = 0x78;

    addrMagic = buf + 28 + vmPeekL(buf+2) + vmPeekL(buf+6);
        {
        unsigned b;
        unsigned papply;
        unsigned i;

        b = vmPeekL(addrMagic);
        papply = buf + 28 + b;

        addrMagic+=4;
        while(b)
            {
            if (b != 1)
                *(DWORD *)memPvGetFlatAddress(papply, sizeof(DWORD)) = vmPeekL(papply) + lLoadAddr;
            
            b = vmPeekB(addrMagic++);
            papply += ((b == 1) ? 254 : b);
            }
        }

    vpregs->PC = lLoadAddr;

    for (i = 0; i < v.rgosinfo[vmCur.iOS].cbOS-28; i+=4)
        *(DWORD *)memPvGetFlatAddress(v.rgosinfo[vmCur.iOS].eaOS + i, sizeof(DWORD)) =
             vmPeekL(buf + 28 + i);

    for (i = 0; i < 262144-28; i+=4)
        *(DWORD *)memPvGetFlatAddress(lLoadAddr + i, sizeof(DWORD)) = vmPeekL(buf + 28 + i);
#endif
}

//
// load MAGIC PC from disk
//

void ReadMagiC_PC(char *sz)
{
    unsigned cb = 262144;
    unsigned buf = 0xE00000; // arbitrary load address
    unsigned lLoadAddr;      // actual start of MagiC in memory

    if (!CbReadROMImage(sz, cb, buf))
        return;

#ifdef LATER
    // MAGIC_PC.OS is normally encryped, so first check if this version
    // isn't, and if it is, decrypt it using the only way we know

    {
    unsigned char *pch, ch;

    pch = memPvGetFlatAddress(buf, cb);

    if (NULL == pch)
        return;

    if (*pch != 0x60)
        {
        int i = 0;

        // Sure looks encrypted!

        if (pch[1] == 0x2E)
            {
            // complicated method used by retail MagiC PC

            int cb = 0x34A5C;
            unsigned char magic = 0x1C;
            unsigned char magic2 = 0;
        
            pch[i++] = 0x60;
            i++;
        
            while (cb > 255)
                {
                static unsigned char rgbAdd[8] =
                     { 0x53, 0x6D, 0x6B, 0xDF, 0x4F, 0x67, 0x47, 0xC7 };
        
                int j;
                unsigned char rgb[256];
                unsigned char chXOR = magic;
                unsigned char chADD = rgbAdd[magic2 & 7];
        
                magic2++;
                magic += magic2;
        
                memcpy(rgb, &pch[i], 256);
        
                for (j = 0; j < 256; j++)
                    {
                    pch[i] = rgb[chXOR] ^ chXOR;
                    chXOR += chADD;
                    i++;
                    }
        
                magic += 0x11;
                cb -= 256;
                }
            }

        else
            {
            // lame method used by MagiC PC demo

            ch = 0x1D;

            for (i = 0; i < 262144; i++)
                {
                *pch = (*pch ^ ch);
                ch += 0x53;
                pch++;
                }
            }
        }
    }
#endif

    vpregs->PC = lLoadAddr = 0xE00000;
}

//
// load TOS 1.0 / 1.4 boot disk image from disk
//

void ReadTOS_RAM(char *sz)
{
    unsigned cb = 204*1024;  // TOS.IMG won't be bigger than 204K
    unsigned buf = 0x040000; // arbitrary load address
    unsigned i;

    cb = CbReadROMImage(sz, cb, buf);

    if (cb < (128*1024))
        return;

    if (vmPeekW(buf) == 0x46FC)
        {
        // TOS.IMG disk based TOS
        // Jump into the 6th byte of the header

        vpregs->PC = buf + 6;

        for (i = 0; i < cb; i += sizeof(LONG))
            memWriteLongROM(vmPeekL(buf + 0x108) + i, vmPeekL(buf + 0x100 + i));

        for (i = 0; i < 192*1024; i += sizeof(LONG))
            memWriteLongROM(0xFC0000 + i, vmPeekL(buf + 0x100 + i));
        }
    else
        {
        // assume it's a 192K TOS ROM image

        vpregs->PC = 0xFC0000;

        for (i = 0; i < 192*1024; i += sizeof(LONG))
            memWriteLongROM(vpregs->PC + i, vmPeekL(buf + i));
        }
}

//
// load TOS 1.6 / 2.06 disk image from disk
//

void ReadTOS2_RAM(char *sz)
{
    int      cb = 256*1024;
    unsigned buf = 0x040000; // arbitrary load address
    unsigned i;

    if (cb != CbReadROMImage(sz, cb, buf))
        return;

    // assume it's a 256K TOS ROM image

    vpregs->PC = 0xE00000;

    for (i = 0; i < 256*1024; i += sizeof(LONG))
        memWriteLongROM(vpregs->PC + i, vmPeekL(buf + i));
}

//
// load TOS 3.06 disk image from disk
//

void ReadTOS3_RAM(char *sz)
{
    int      cb = 512*1024;
    unsigned buf = 0x040000; // arbitrary load address
    unsigned i;

    if (cb != CbReadROMImage(sz, cb, buf))
        return;

    // assume it's a 512K TOS ROM image

    vpregs->PC = 0xE00000;

    for (i = 0; i < 512*1024; i+=4)
        memWriteLongROM(vpregs->PC + i, vmPeekL(buf + i));
}

//
// load MAC 64K ROMs (or 128K ROMs from disk)
//

BOOL FReadMac64K()
{
    unsigned cb = 65536 * 2;
    unsigned buf = 0xFA0000; // arbitrary load address
    unsigned i;

    for (i = 0xFA0000; i < 0xFC0000; i++)
        {
        memWriteByteROM(i, 0xFF);
        }

    for (i = 0; i < v.cOS; i++)
        {
        if (v.rgosinfo[i].osType == osMac_64)
            {
            int ichip = v.rgosinfo[i].iChip;

            if ((v.rgosinfo[i].szImg[0]) && (v.rgosinfo[i].szImg[1] > 3))
                {
                return CbReadROMImage(v.rgosinfo[i].szImg, 0x10000, 0xFA0000);
                }

            cbSocket = v.rgosinfo[i].cbSocket;

            vi.ioPort = v.ioBaseAddr + (ichip / cScktMax)*4;
            OutPort(0);
            SkipROMs(cbSocket * (ichip & wScktMsk));

            // Read the low ROM

            for (i=0; i<32768; i++)
                memWriteByteROM(0x00FA0000 + i + i, InPort());
            SkipROMs(cbSocket - 32768);

            // Read the high ROM

            for (i=0; i<32768; i++)
                memWriteByteROM(0x00FA0001 + i + i, InPort());

            return fTrue;
            }
        }

    return fFalse;
}


//
// Read the contents of a 256 kilobit 27256 ROM (6 chip type)
//

void ReadROM256(fReset, lAddr)
BOOL fReset;
ULONG lAddr;
{
    unsigned i;

    if (fReset)
        OutPort(0);

    for (i=0; i<cbSocket; i++)
        {
        if (i < 32768)
            *(BYTE *)MapAddressVM(lAddr+i+i) = InPort();
        else
            InPort();
        }
}

//
// Read the contents of a 512 kilobit 27512 ROM (4 chip type)
//

void ReadROM512_4(fReset, lAddr)
BOOL fReset;
ULONG lAddr;
{
    unsigned i;

    if (fReset)
        OutPort(0);

    for (i=0; i<cbSocket; i++)
        {
        if (i < 65536)
            *(BYTE *)MapAddressVM(lAddr+i+i+i+i) = InPort();
        else
            InPort();
        }
}

//
// Read the contents of a 512 kilobit 27512 ROM (2 chip type)
//

void ReadROM512(fReset, lAddr)
BOOL fReset;
ULONG lAddr;
{
    unsigned i;

    if (fReset)
        OutPort(0);

    for (i=0; i<cbSocket; i++)
        {
        if (i < 65536)
            *(BYTE *)MapAddressVM(lAddr+i+i) = InPort();
        else
            InPort();
        }
}

//
// Read the contents of a 1 megabit 27010 ROM (2 chip type)
//

void ReadROM010(fReset, lAddr)
BOOL fReset;
ULONG lAddr;
{
    unsigned i, iMac;

    if (fReset)
        OutPort(0);

    iMac = (vi.cbROM[0]/2);

    for (i=0; i < iMac; i++)
        *(BYTE *)MapAddressVM(lAddr+i+i) = InPort();

    SkipROMs(cbSocket - i);
}


//
// Read the contents of a 1 megabit 27010 ROM (4 chip type)
//

void ReadROM010_4(fReset, lAddr)
BOOL fReset;
ULONG lAddr;
{
    int i, iMac;

    if (fReset)
        OutPort(0);

    iMac = (vi.cbROM[0]/4);

    for (i=0; i < iMac; i++)
        *(BYTE *)MapAddressVM(lAddr+i+i+i+i) = InPort();

    SkipROMs(cbSocket - i);
}


//
// Read the i'th operating system in rgosinfo.
// Returns TRUE if valid OS installed.
//

BOOL FInstallROMs()
{
    const ULONG lLoadAddr = vi.eaROM[0];
    const ULONG cbROM = v.rgosinfo[vmCur.iOS].cbOS;
    int ichip;
    HCURSOR h = SetCursor(LoadCursor(NULL, IDC_WAIT));

    Assert(v.cOS);

    if (v.cOS == 0)
        goto Lfail;

    if (vmCur.iOS == -1)
        goto Lfail;

    ichip = v.rgosinfo[vmCur.iOS].iChip;
    cbSocket = v.rgosinfo[vmCur.iOS].cbSocket;
    cSockets = v.rgosinfo[vmCur.iOS].cSockets;

    switch(v.rgosinfo[vmCur.iOS].osType)
        {
    default:
            goto Lfail;

    case osMac_64:
    case osMac_128:
    case osMac_256:
    case osMac_512:
    case osMac_II:
    case osMac_IIi:
    case osMac_1M:
    case osMac_2M:
    case osMac_G1:
    case osMac_G2:
    case osMac_G3:
    case osMac_G4:
        if (!FIsMac(vmCur.bfHW))
            goto Lfail;

    case osDiskImg:
        Assert (v.rgosinfo[vmCur.iOS].version != 0);

        if ((v.rgosinfo[vmCur.iOS].szImg[0]) && (v.rgosinfo[vmCur.iOS].szImg[1] > 3))
            {
            if (!CbReadROMImage(v.rgosinfo[vmCur.iOS].szImg, cbROM, lLoadAddr))
                {
                goto Lfail;
                }
            goto Lexit;
            }

        // reset ROM board

        vi.ioPort = v.ioBaseAddr + (ichip / cScktMax)*4;
        OutPort(0);
        SkipROMs(cbSocket * (ichip & wScktMsk));

        if (v.rgosinfo[vmCur.iOS].cbOS > 0x40000)
            {
            // 512K Mac ROM (4 x 128K)
            ReadROM010_4(FALSE, lLoadAddr);
            ReadROM010_4(FALSE, lLoadAddr+1);
            ReadROM010_4(FALSE, lLoadAddr+2);
            ReadROM010_4(FALSE, lLoadAddr+3);
            }
        else if (v.rgosinfo[vmCur.iOS].cChip == 4)
            {
            // 256K Mac ROM (4 x 64K)
            ReadROM512_4(FALSE, lLoadAddr);
            ReadROM512_4(FALSE, lLoadAddr+1);
            ReadROM512_4(FALSE, lLoadAddr+2);
            ReadROM512_4(FALSE, lLoadAddr+3);
            }
        else if (v.rgosinfo[vmCur.iOS].cbOS > 0x20000)
            {
            // 256K Mac ROM (2 x 128K)
            ReadROM010(FALSE, lLoadAddr);
            ReadROM010(FALSE, lLoadAddr+1);
            }
        else if (v.rgosinfo[vmCur.iOS].cbOS > 0x10000)
            {
            // 128K Mac ROM (2 x 64K)
            ReadROM512(FALSE, lLoadAddr);
            ReadROM512(FALSE, lLoadAddr+1);
            }
        else
            {
            // 64K Mac ROM (2 x 32K)
            ReadROM256(FALSE, lLoadAddr);
            ReadROM256(FALSE, lLoadAddr+1);
            }
        goto Lexit;

    case osMagic_2:
    case osMagic_4:
        if (!FIsAtari68K(vmCur.bfHW))
            goto Lfail;
        ReadMagiC(v.rgosinfo[vmCur.iOS].szImg);
        goto Lexit;

    case osMagicPC:
        if (!FIsAtari68K(vmCur.bfHW))
            goto Lfail;
        ReadMagiC_PC(v.rgosinfo[vmCur.iOS].szImg);
        goto Lexit;

    case osTOS1X_D:
        if (!FIsAtari68K(vmCur.bfHW))
            goto Lfail;
        ReadTOS_RAM(v.rgosinfo[vmCur.iOS].szImg);
        goto Lexit;

    case osTOS2X_D:
        if (!FIsAtari68K(vmCur.bfHW))
            goto Lfail;
        ReadTOS2_RAM(v.rgosinfo[vmCur.iOS].szImg);
        goto Lexit;

    case osTOS3X_D:
        if (!FIsAtari68K(vmCur.bfHW))
            goto Lfail;
        ReadTOS3_RAM(v.rgosinfo[vmCur.iOS].szImg);
        goto Lexit;
        }

    if (!FIsAtari68K(vmCur.bfHW))
        goto Lfail;

    // reset ROM board

    vi.ioPort = v.ioBaseAddr + (ichip / cScktMax)*4;
    OutPort(0);
    SkipROMs(cbSocket * (ichip & wScktMsk));

    Assert (v.rgosinfo[vmCur.iOS].version != 0);

    if (v.rgosinfo[vmCur.iOS].cChip == 2)
        {
        ReadROM010(FALSE, lLoadAddr);

        if (vmPeekB(lLoadAddr) != 0x60)
            goto Lfail;

        ReadROM010(FALSE, lLoadAddr+1);
        }
    else if (v.rgosinfo[vmCur.iOS].cChip == 4)
        {
        ReadROM010_4(FALSE, lLoadAddr);

        if (vmPeekB(lLoadAddr) != 0x60)
            goto Lfail;

        ReadROM010_4(FALSE, lLoadAddr+1);
        ReadROM010_4(FALSE, lLoadAddr+2);
        ReadROM010_4(FALSE, lLoadAddr+3);
        }
    else
        {
        ReadROM256(FALSE, lLoadAddr);

        if (vmPeekB(lLoadAddr) != 0x60)
            goto Lfail;

        ReadROM256(FALSE, lLoadAddr + 0x00001);
        ReadROM256(FALSE, lLoadAddr + 0x10000);
        ReadROM256(FALSE, lLoadAddr + 0x10001);
        ReadROM256(FALSE, lLoadAddr + 0x20000);
        ReadROM256(FALSE, lLoadAddr + 0x20001);
        }

Lexit:
    if (FIsAtari68K(vmCur.bfHW))
        FReadMac64K();
    {
    // Try to load a patch file based on the ROM signature

    char sz[MAX_PATH];

    sprintf(sz, "%s\\%08X", (char *)&v.rgchGlobalPath, vmPeekL(lLoadAddr));

    CbReadWholeFileToGuest(sz, cbROM, lLoadAddr, ACCESS_INITHW);
    }

    SetCursor(h);
    return TRUE;

Lfail:
    SetCursor(h);
    return FALSE;
}

LRESULT CALLBACK FirstTimeProc(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    switch (message)
        {
        case WM_INITDIALOG:
            // initialize dialog box

            CheckDlgButton(hDlg, IDC_CHECKROM,
                 vi.fWinNT && (vi.hROMCard != INVALID_HANDLE_VALUE));
            CheckDlgButton(hDlg, IDC_CHECKMAGIC, TRUE);
            SetDlgItemText(hDlg, IDC_EDITPATH, (char *)&v.rgchGlobalPath);
            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
            return (TRUE);

        case WM_COMMAND:
            // item notifications

            switch (LOWORD(uParam))
                {
            default:
                break;

            case IDROMSET:
                CheckSum(hDlg);
                break;

            case IDCANCEL:
            case IDOK:
                   EndDialog(hDlg, TRUE);
                return (TRUE);
                break;

            case IDPATH:
                // disable the caller

                EnableWindow(hDlg,FALSE);

                OpenThePath(hDlg, (char *)&v.rgchGlobalPath);

                DebugStr("global path set to *%s*\n", (char *)&v.rgchGlobalPath);

                // re-enable the caller

                EnableWindow(hDlg,TRUE);

                SetDlgItemText(hDlg, IDC_EDITPATH, (char *)&v.rgchGlobalPath);
                return FALSE;

            case IDSCAN:
                WScanROMs(
                    IsDlgButtonChecked(hDlg, IDC_CHECKROM),
                    IsDlgButtonChecked(hDlg, IDC_CHECKMAGIC),
                    IsDlgButtonChecked(hDlg, IDC_CHECKMAGFLOP),
                    IsDlgButtonChecked(hDlg, IDC_CHECKMAGNET)
                    );
                ListROMs(hDlg, ~(osAtari48 | osAtariXL | osAtariXE));
                return FALSE;
                }
        }

    return (FALSE); // Didn't process the message

    lParam; // This will prevent 'unused formal parameter' warnings
}

#endif // ATARIST || SOFTMAC

