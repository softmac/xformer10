
/****************************************************************************

    SERIAL.C

    - Serial I/O routines

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    08/25/1995  darekm      last update

****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include "gemtypes.h" // main include file 


HANDLE hComm = INVALID_HANDLE_VALUE;
DCB    dcb;
DWORD  dwEventMask;
int    i;

//
// Write a byte to the serial port
//

BOOL FWriteSerialPort(BYTE b)
{
    int cch;
    COMSTAT ComStat;
    int dummy;
    BOOL f;

    DebugStr("writing %02X to serial port\n", b);

    if (hComm == INVALID_HANDLE_VALUE)
        return FALSE;

    ClearCommError(hComm, &dummy, &ComStat);
    DebugStr("ComStat = %08X, %08X, %d, %d\n", dummy, *(int *)&ComStat, ComStat.cbInQue, ComStat.cbOutQue);

    f = WriteFile(hComm, &b, 1, &cch, NULL);
    if (!f)
        {
        DebugStr("Writefile FAILED, error = %08X\n", GetLastError());
        ClearCommError(hComm, &dummy, &ComStat);
        DebugStr("ComStat = %08X, %08X, %d, %d\n", dummy, *(int *)&ComStat, ComStat.cbInQue, ComStat.cbOutQue);
        }
    return cch;
}


//
// Return number of new bytes in the serial port read buffer
//

int CchSerialPending()
{
    COMSTAT ComStat;
    int dummy;

    if (hComm == INVALID_HANDLE_VALUE)
        {
        vi.cchserial = 0;
        return 0;
        }

    ClearCommError(hComm, &dummy, &ComStat);

    ComStat.cbInQue &= 2047;
//    DebugStr("CchSerialPending returning %d\n", ComStat.cbInQue);
    return (vi.cchserial = ComStat.cbInQue);
}


//
// Read from the serial port and put into buffer
//

BOOL CchSerialRead(char *rgb, int cchRead)
{
    int    cch = 0;
    BOOL f = ReadFile(hComm, rgb, cchRead, &cch, NULL);

    DebugStr("CchSerialRead returning %d %02X\n", cch, rgb[0]);
    return cch;
}


void __inline CheckError(BOOL f, int iCOM, HANDLE hComm, char *pch)
{
    char rgch[256];
    char rgchErr[256];

#if _ENGLISH
    sprintf(rgchErr, "Error on COM%d:", iCOM);
#elif _NEDERLANDS
    sprintf(rgchErr, "Fout in COM%d:", iCOM);
#elif _DEUTSCH
    sprintf(rgchErr, "Fehler an COM%d:", iCOM);
#elif _FRANCAIS
    sprintf(rgchErr, "Erreur sur COM%d :", iCOM);
#elif
#error
#endif
    sprintf(rgch, "%s\nh = %08X\nerror = %08X", pch, hComm, GetLastError());
    if (!f)
        MessageBox(GetFocus(), rgch, rgchErr, MB_OK|MB_ICONHAND);
}

//
// Initialize COM port. Closes any existing COM port in use.
// A value of 0 closes the current port only.
//

BOOL FInitSerialPort(int iCOM)
{
    char rgch[] = "COMx";
    BOOL f;

    if (hComm != INVALID_HANDLE_VALUE)
        {
        CloseHandle(hComm);
        }

    hComm = INVALID_HANDLE_VALUE;
    vi.cchserial = 0;

    if (iCOM == 0)
        return TRUE;

    if ((iCOM < 1) || (iCOM > 4))
        return FALSE;

    rgch[3] = '0' + iCOM;
        
    hComm = CreateFile(rgch, GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hComm == INVALID_HANDLE_VALUE)
        {
        CheckError(0, iCOM, hComm,
           "Invalid COM port, or COM port is already in use.");
        return FALSE;
        }

    f = SetupComm(hComm, 2048, 2048);  // set up 2K buffers
    CheckError(f, iCOM, hComm, "Unable to set up COM port");
    if (!f)
        {
        FInitSerialPort(0);
        return FALSE;
        }

    f = ClearCommError(hComm, &dwEventMask, NULL);
    CheckError(f, iCOM, hComm, "Unable to clear COM port error");

    // clear all timeouts

    {
    COMMTIMEOUTS ct;

    f = GetCommTimeouts(hComm, &ct);
    CheckError(f, iCOM, hComm, "Unable to get COM port timeouts");

    DebugStr("ct = %d %d %d %d %d\n",
        ct.ReadIntervalTimeout,
        ct.ReadTotalTimeoutMultiplier,
        ct.ReadTotalTimeoutConstant,
        ct.WriteTotalTimeoutMultiplier,
        ct.WriteTotalTimeoutConstant);

    memset(&ct, 0, sizeof(ct));
    ct.WriteTotalTimeoutConstant = 100;
    f = SetCommTimeouts(hComm, &ct);
    CheckError(f, iCOM, hComm, "Unable to set COM port timeouts");
    }

    f = GetCommState(hComm, &dcb);
    CheckError(f, iCOM, hComm, "Unable to get COM port state");

#if 1
    DebugStr("baud rate = %d\n", dcb.BaudRate);
    DebugStr("fBinary = %d\n", dcb.fBinary);
    DebugStr("fParity = %d\n", dcb.fParity);
    DebugStr("fOutxCtsFlow = %d\n", dcb.fOutxCtsFlow);
    DebugStr("fOutxDsrFlow = %d\n", dcb.fOutxDsrFlow);
    DebugStr("fDtrControl = %d\n", dcb.fDtrControl);
    DebugStr("fDsrSensitivity = %d\n", dcb.fDsrSensitivity);
    DebugStr("fTXContinueOnXoff = %d\n", dcb.fTXContinueOnXoff);
    DebugStr("fOutX = %d\n", dcb.fOutX);
    DebugStr("fInX = %d\n", dcb.fInX);
    DebugStr("fErrorChar = %d\n", dcb.fErrorChar);
    DebugStr("fNull = %d\n", dcb.fNull);
    DebugStr("fRtsControl = %d\n", dcb.fRtsControl);
    DebugStr("fAbortOnError = %d\n", dcb.fAbortOnError);
    DebugStr("XonLim = %d\n", dcb.XonLim);
    DebugStr("XoffLim = %d\n", dcb.XoffLim);
    DebugStr("ByteSize = %d\n", dcb.ByteSize);
    DebugStr("Parity = %d\n", dcb.Parity);
    DebugStr("StopBits = %d\n", dcb.StopBits);
    DebugStr("XonChar = %d\n", dcb.XonChar);
    DebugStr("XoffChar = %d\n", dcb.XoffChar);
    DebugStr("ErrorChar = %d\n", dcb.ErrorChar);
    DebugStr("EofChar = %d\n", dcb.EofChar);
    DebugStr("EvtChar = %d\n", dcb.EvtChar);
#endif

    // default settings

    dcb.BaudRate = 2400;
    dcb.EofChar = 0;
    dcb.fParity = 0;
    dcb.fBinary = 1;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.ByteSize = 8;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutxCtsFlow = 0;
    dcb.fOutxDsrFlow = 0;
    dcb.XonLim = 2048;
    dcb.XoffLim = 512;
    f =  SetCommState(hComm, &dcb);
    CheckError(f, iCOM, hComm, "Unable to set COM port state");

#if 0
    GetCommModemStatus(hComm, &l);
    DebugStr(" modem status = %08X\n", l, dwEventMask);
#endif

    f = GetCommMask(hComm, &dwEventMask);
    CheckError(f, iCOM, hComm, "Unable to get COM port event mask");
    DebugStr("event mask = %08X, %d\n", dwEventMask, f);

    f = SetCommMask(hComm, EV_RXCHAR);
    CheckError(f, iCOM, hComm, "Unable to set COM port event mask");
    DebugStr("setcommmask = %d\n", f);

    f = GetCommMask(hComm, &dwEventMask);
    DebugStr("event mask = %08X, %d\n", dwEventMask, f);

    SetCommBreak(hComm);
    ClearCommBreak(hComm);

    SetRTS(TRUE);
    SetDTR(TRUE);

    return TRUE;
}


//
// Atari ST specific routine to set baud rate and other serial port parameters
//

BOOL FSetBaudRate(int tsr, int ucr, int tddr, int tcdcr)
{
    BOOL f;

    if (hComm == INVALID_HANDLE_VALUE)
        return 1;

    DebugStr("SetBaudRate: tsr:%02X ucr:%02X tddr:%02X tcdcr:%02X\n",
        tsr, ucr, tddr, tcdcr);

    // break bit

    if (tsr & 3)
        {
        DebugStr("Setting BREAK\n");
        SetCommBreak(hComm);
        }
    else
        {
        DebugStr("Clearing BREAK\n");
        ClearCommBreak(hComm);
        }

    // parity

    switch ((ucr >> 1) & 3)
        {
    default:
        DebugStr("Setting NO parity\n");
        dcb.Parity = NOPARITY;
        break;

    case 2:
        DebugStr("Setting ODD parity\n");
        dcb.Parity = ODDPARITY;
        break;

    case 3:
        DebugStr("Setting EVEN parity\n");
        dcb.Parity = EVENPARITY;
        break;
        }

    // stop bits

    switch ((ucr >> 3) & 3)
        {
    default:
        DebugStr("Setting 1 stop bit\n");
        dcb.StopBits = ONESTOPBIT;
        break;

    case 2:
        DebugStr("Setting 1.5 stop bits\n");
        dcb.StopBits = ONE5STOPBITS;
        break;

    case 3:
        DebugStr("Setting 2 stop bits\n");
        dcb.StopBits = TWOSTOPBITS;
        break;
        }

    // word length

    dcb.ByteSize = 8 - ((ucr >> 5) & 3);
    DebugStr("Setting %d DATA bits\n", dcb.ByteSize);

    // baud rate

    if ((tcdcr & 7) == 1)
        {
        if (tddr == 0)
            dcb.BaudRate = 19200;
        else
            dcb.BaudRate = (19200 + (tddr/2)) / tddr;

#ifdef LATER
        if (vmCur.fQuickBaud) switch (dcb.BaudRate)
            {
        case 110:
            dcb.BaudRate = 115200;
            break;

        case 300:
            dcb.BaudRate = 38400;
            break;

        case 600:
            dcb.BaudRate = 57600;
            break;

        case 1200:
            dcb.BaudRate = 28800;
            break;

        case 4800:
            dcb.BaudRate = 14400;
            break;
             }
#endif
            
        DebugStr("Setting BAUD to %d\n", dcb.BaudRate);
        }

    f = SetCommState(hComm, &dcb);
    DebugStr("SetBaudRate returning %d\n", f);

    SetCommBreak(hComm);
    ClearCommBreak(hComm);

    ClearCommError(hComm, &dwEventMask, NULL);
    SetCommMask(hComm, EV_RXCHAR);
    return f;
}


void SetRTS(BOOL f)
{
    if (hComm == INVALID_HANDLE_VALUE)
        return;

    EscapeCommFunction(hComm, f ? SETRTS : CLRRTS);
}


void SetDTR(BOOL f)
{
    if (hComm == INVALID_HANDLE_VALUE)
        return;

    EscapeCommFunction(hComm, f ? SETDTR : CLRDTR);
}

ULONG GetModemStatus()
{
// Atari ST's port has these bits:
// bit 6, RS-232 ring
// bit 2, RS232 CTS
// bit 1, RS232 carrier detect
//
// We return a long value with the higher 8 bits (AH) being
// set with the above bits, and the lower 8 bits (AL) being
// the bits which should trigger an interrupt

    BYTE new = 0xFF, changed;
    static BYTE old;
    int l;

    if (hComm == INVALID_HANDLE_VALUE)
        {
        }
    else
        {
        GetCommModemStatus(hComm, &l);

        if (l & MS_RING_ON)
            new &= ~0x40;

        if (l & MS_CTS_ON)
            new &= ~0x4;

        if (l & MS_RLSD_ON)
            new &= ~0x2;
        }

    new &= 0x46;

    changed = old ^ new;
    old = new;
    return (new << 8) | changed;
}
