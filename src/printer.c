
/****************************************************************************

    PRINTER.C

    - Generic printer handling routines

    Copyright (C) 1991-2021 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

    11/30/2008  darekm      Gemulator 9.0 release
    07/08/1998  darekm      last update

****************************************************************************/

#include "gemtypes.h" // main include file 


// private globals
// !!! NOT THREAD SAFE

HANDLE vhPrinter;

#define CCHBUFMAX 1024

int vcchBuf;
BYTE vrgbBuffer[CCHBUFMAX];


BOOL UnInitPrinter()
{
    if (vhPrinter != INVALID_HANDLE_VALUE)
        {
        CloseHandle(vhPrinter);
        }

    vhPrinter = INVALID_HANDLE_VALUE;
    vi.cPrintTimeout = 0;
    return TRUE;
}


BOOL InitPrinter(int iLPT)
{
    char rgch[16];

    UnInitPrinter();

    if (iLPT == 0)
        return TRUE;

    sprintf(rgch, "LPT%d", iLPT);
    vhPrinter = CreateFile(rgch, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL /* | FILE_FLAG_WRITE_THROUGH */, NULL);

    if (vhPrinter == INVALID_HANDLE_VALUE)
        {
        MessageBox (GetFocus(),
#if _ENGLISH
            "Invalid LPT port, or LPT port is already in use.",
#elif _NEDERLANDS
             "Ongeldige LPT-poort of LPT-poort is reeds in gebruik.",
#elif _DEUTSCH
             "Ung�ltiger LPT Port bzw. LPT Port schon in Gebrauch.",
#elif _FRANCAIS
             "Le port LPT est invalide, ou le port LTP est d�j� utilis�.",
#elif
#error
#endif
            rgch, MB_OK|MB_ICONHAND);
        return FALSE;
        }

    vi.cPrintTimeout = 200*30;
    return TRUE;
}


BOOL FPrinterReady(int iVM)
{
    return (rgpvm[iVM]->iLPT != 0);
}


BOOL FlushToPrinter(int iVM)
{
    int cch;

    if (rgpvm[iVM]->iLPT == 0)
        return TRUE;

    if (vhPrinter == INVALID_HANDLE_VALUE)
        InitPrinter(rgpvm[iVM]->iLPT);

    if (vcchBuf && (rgpvm[iVM]->iLPT != 0))
        {
        WriteFile(vhPrinter, vrgbBuffer, vcchBuf, (LPDWORD)&cch, NULL);
        vcchBuf = 0;
        vi.cPrintTimeout = 200*30;
        }
    return TRUE;
}


BOOL ByteToPrinter(int iVM, unsigned char ch)
{
    //DebugStr("FOutputToPrinter:outputting %c\n", ch);

    if (vhPrinter == INVALID_HANDLE_VALUE)
        InitPrinter(rgpvm[iVM]->iLPT);

    if (rgpvm[iVM]->iLPT != 0)
        {
        vrgbBuffer[vcchBuf++] = ch;
        vi.cPrintTimeout = 200*30;

        if (vcchBuf >= CCHBUFMAX)
            FlushToPrinter(iVM);
        }

    return TRUE;
}


BOOL StringToPrinter(int iVM, char *pch)
{
    while (*pch)
        {
        if (!ByteToPrinter(iVM, *pch++))
            return FALSE;
        }

    return TRUE;
}


#if 0
void pmain()
{
    int i;
    int j = GetTickCount();
    char sz[256];

    InitPrinter(1);

    sprintf(sz, "new print job = %08X\r\n", j);
    StringToPrinter(sz);

    for (i = 0; i < 60; i++)
        {
        Sleep(300);
        printf("printer ready = %d\n", FPrinterReady());
        ByteToPrinter(64+i);
        }

    ByteToPrinter(13);
    ByteToPrinter(10);
//    ByteToPrinter(12);

    sprintf(sz, "end print job = %08X\n", j);
    Sleep(1000000);
    UnInitPrinter();
}
#endif
