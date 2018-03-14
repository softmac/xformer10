
/***************************************************************************

    XATARI.C

    - Atari 800 hardware emulation

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    12/19/2000  darekm      update for Xformer 2000

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

void Credits(int fDealers)
{
#ifndef HWIN32
#ifdef HDOS16
    __segment videomem = 0xB800;
    BYTE __based(videomem) *qch = 0;
#else
#ifdef HDOS32
    BYTE *qch = 0xB8000;
#endif
#endif
    int cch;

    RestoreVideo();

    if (fDealers)
        return;

#ifdef NDEBUG
    printf("  PC XFORMER CLASSIC 3.81 ATARI 8-BIT EMULATOR FOR MS-DOS\n\n");

    printf("  Written by Darek Mihocka. Copyright (C) 1986-2008. All rights reserved.\n");
    printf("  Atari OS and BASIC used with permission. Copyright (C) 1979-1984 Atari Corp.\n\n");

    printf("  Emulators Inc., 14150 N.E. 20th Street Suite 302, Bellevue WA 98007, U.S.A.\n\n");
    printf("  Visit the Emulators web page at http://www.emulators.com/\n\n");
#endif

    for (cch = 0; cch < (80*1); cch++)
        {
        qch[1] = 0x71;    // blue on white
        qch += 2;
        }

    for (cch = 0; cch < (80*24); cch++)
        {
        qch[1] = 0x17;    // white on blue
        qch += 2;
        }
#endif
}

#ifndef HWIN32

VMINFO *vpvm;

void main(int argc, char *argv[])
{
    extern VMINFO vmi800;

    vpvm = &vmi800;

    FInitVM();
    InitSIOV(argc-1,&argv[1]);  // HACK! HACK!

#ifndef HWIN32
    Credits(0);

    if (!fAutoStart)
        {
        if (fXFCable)
            {
            printf("  Using Xformer 8-bit Peripheral Cable. Calibration value = %d\n\n", uBaudClock);
            }

        printf("  The following special keys can be used while in Atari 8-bit mode:\n\n");
        printf("  F1-F4 - XL/XE function keys. Shift+F5 - exit. F6 - XL HELP key. End - BREAK.\n");
        printf("  F7 - START. F8- SELECT. F9 - OPTION. F10 - RESET. Shift+F10 - Toggle BASIC.\n");
        printf("  F12 - Switch mode. Scrl Lock - turbo/normal. PgUp/PgDown - center screen.\n");
        printf("\n  PRESS ANY KEY TO BEGIN..."); fflush(stdout);

        _bios_keybrd(_NKEYBRD_READ);
        }
#endif

    FColdbootVM();

#ifndef HWIN32
    if (fDebugger)
        mon();
    else
#endif
        FExecVM(FALSE,TRUE);

    FUnInitVM();

    Credits(1);
}

#endif

#endif // XFORMER

