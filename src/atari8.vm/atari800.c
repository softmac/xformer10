
/***************************************************************************

    ATARI800.C

    - Implements the public API for the Atari 800 VM.

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

#include "atari800.h"

//
// Our VM's VMINFO structure
//

#ifdef XFORMER

VMINFO const vmi800 =
    {
    NULL,
    0,
    "Atari 800/800XL/130XE",
#ifdef HWIN32
    vmAtari48C | vmAtariXLC | vmAtariXEC | vmAtari48 | vmAtariXL | vmAtariXE,
    cpu6502,
    ram48K | ram64K | ram128K,
    osAtari48 | osAtariXL | osAtariXE,
    monGreyTV | monColrTV,
    InstallAtari,
#else
    0,
    0,
    0,
    0,
    0,
    0,
#endif
    InitAtari,
    UninitAtari,
    InitAtariDisks,
#ifdef HWIN32
    MountAtariDisk,
#endif
    UninitAtariDisks,
#ifdef HWIN32
    UnmountAtariDisk,
#endif
    ColdbootAtari,
    WarmbootAtari,
    ExecuteAtari,
    TraceAtari,
    KeyAtari,
    DumpRegsAtari,
    DumpHWAtari,
    DisasmAtari,
    PeekBAtari,
    PeekWAtari,
    PeekLAtari,
    PokeBAtari,
    PokeWAtari,
    PokeLAtari,
    };


//
// Globals
//

CANDYHW vrgcandy[MAXOSUsable];

CANDYHW *vpcandyCur = &vrgcandy[0];

WORD fBrakes;        // 0 = run as fast as possible, 1 = slow down
BYTE bshiftSav;

#ifdef HWIN32

void DumpROM(char *filename, char *label, char *rgb, int start, int len)
{
    FILE *fp;
    int i, j;

    fp = fopen(filename, "wt");

    fprintf(fp, "unsigned char %s[%d] =\n{\n", label, len);

    for (i = 0; i < len; i += 8)
      {
      fprintf(fp, "    ");

      for (j = 0; j < 8; j++)
        {
        fprintf(fp, "0x%02X", (unsigned char)rgb[i + j]);

        if (j < 7)
          fprintf(fp, ", ");
        else
          fprintf(fp, ", // $%04X\n", start + i);
        }
      }

    fprintf(fp, "};\n\n");
    fclose(fp);
}

void DumpROMS()
{
    FILE *fp;
    int i, j;

    DumpROM("atariosb.c", "rgbAtariOSB", rgbAtariOSB, 0xC800, 0x2800); // 10K
    DumpROM("atarixl5.c", "rgbXLXE5000", rgbXLXE5000, 0x5000, 0x1000); //  4K
    DumpROM("atarixlb.c", "rgbXLXEBAS",  rgbXLXEBAS,  0xA000, 0x2000); //  8K
    DumpROM("atarixlc.c", "rgbXLXEC000", rgbXLXEC000, 0xC000, 0x1000); //  4K
    DumpROM("atarixlo.c", "rgbXLXED800", rgbXLXED800, 0xD800, 0x2800); // 10K
}

BOOL __cdecl InstallAtari(PVMINFO pvmi, PVM pvm)
{
    // initialize the default Atari 8-bit VM

    pvm->pvmi = pvmi;
    pvm->bfHW = vmAtari48;
    pvm->bfCPU = cpu6502;
    pvm->iOS = 0;
    pvm->bfMon = monColrTV;
    pvm->bfRAM  = ram48K;
    pvm->ivdMac = sizeof(v.rgvm[0].rgvd)/sizeof(VD);

    pvm->rgvd[0].dt = DISK_IMAGE;

    strcpy(pvm->rgvd[0].sz, "DOS25.XFD");

    // fake in the Atari 8-bit OS entires

    if (v.cOS < 3)
        {
        v.rgosinfo[v.cOS++].osType  = osAtari48;
        v.rgosinfo[v.cOS++].osType  = osAtariXL;
        v.rgosinfo[v.cOS++].osType  = osAtariXE;
        }

    pvm->fValidVM = fTrue;

 // DumpROMS();

    return TRUE;
}

#endif // HWIN32

BOOL __cdecl InitAtari()
{
    static BOOL fInited;
    BYTE bshiftSav;

    // save shift key status
    bshiftSav = *pbshift & (wNumLock | wCapsLock | wScrlLock);
    *pbshift &= ~(wNumLock | wCapsLock | wScrlLock);

    *pbshift |= wScrlLock;
    fBrakes = 1;
    clockMult = 1;
    countInstr = 1;

    if (!fInited)
        {
#ifdef H32BIT
        extern void * jump_tab[512];
        extern void * jump_tab_RO[256];
        unsigned i;

        for (i = 0 ; i < 256; i++)
            {
            jump_tab[i] = jump_tab_RO[i];
            }
#endif

#ifdef HWIN32
        wStartScan = 8;
#else
        wStartScan = 27;
#endif

        ramtop = 0xA000;
#if XE
        mdXLXE = mdXE;
#else
        mdXLXE = mdXL;
#endif
        fInited = TRUE;
        }

    InitAtariDisks();

    ReadROMs();

#ifdef HWIN32
    if (!FInitSerialPort(vmCur.iCOM))
        vmCur.iCOM = 0;
    if (!InitPrinter(vmCur.iLPT))
        vmCur.iLPT = 0;

    fSound = TRUE;

    // sound buffer large enough to handle at least 10ms @ 48000 Hz

    InitSound(480);

    {
    BOOL fCart = (vmCur.bfHW > vmAtariXE);

    switch(vmCur.bfHW)
        {
    default:
        mdXLXE = md800;
        break;

    case vmAtariXL:
    case vmAtariXLC:
        mdXLXE = mdXL;
        break;

    case vmAtariXE:
    case vmAtariXEC:
        mdXLXE = mdXE;
        break;
        }

    ramtop = fCart ? 0xA000 : 0xC000;
    }

    vmCur.bfRAM = BfFromWfI(vmCur.pvmi->wfRAM, mdXLXE);

//  vi.pbRAM[0] = &rgbMem[0xC000-1];
//  vi.eaRAM[0] = 0;
//  vi.cbRAM[0] = 0xC000;
//  vi.eaRAM[1] = 0;
//  vi.cbRAM[1] = 0xC000;
//  vi.pregs = &rgbMem[0];
#endif

    return TRUE;
}

BOOL __cdecl UninitAtari()
{
#ifndef HWIN32
    if (fSound)
        {
        InitSoundBlaster();
        }
#else
//  vi.pregs = NULL;
#endif
    *pbshift = bshiftSav;

    UninitAtariDisks();
    return TRUE;
}


BOOL __cdecl MountAtariDisk(int i)
{
#ifdef HWIN32
    PVD pvd = &vmCur.rgvd[i];

    if (pvd->dt == DISK_IMAGE)
        AddDrive(i, pvd->sz);
#endif

    return TRUE;
}

BOOL __cdecl InitAtariDisks()
{
#ifdef HWIN32
    int i;

    for (i = 0; i < vmCur.ivdMac; i++)
        MountAtariDisk(i);
#endif

    return TRUE;
}

BOOL __cdecl UnmountAtariDisk(int i)
{
#ifdef HWIN32
    PVD pvd = &vmCur.rgvd[i];

    if (pvd->dt == DISK_IMAGE)
        DeleteDrive(i);
#endif

    return TRUE;
}


BOOL __cdecl UninitAtariDisks()
{
#ifdef HWIN32
    int i;

    for (i = 0; i < vmCur.ivdMac; i++)
        UnmountAtariDisk(i);
#endif

    return TRUE;
}

BOOL __cdecl WarmbootAtari()
{
    NMIST = 0x20 | 0x1F;
    regPC = cpuPeekW((mdXLXE != md800) ? 0xFFFC : 0xFFFA);
    cntTick = 50*4;
    QueryTickCtr();
    countJiffies = 0;

    return TRUE;
}

BOOL __cdecl ColdbootAtari()
{
    unsigned addr;

    // Initialize mode display counter

    cntTick = 50*4;
    QueryTickCtr();
    countJiffies = 0;

#if 0
    printf("ColdStart: mdXLXE = %d, ramtop = %04X\n", mdXLXE, ramtop);
#endif

#ifdef HWIN32
    if (mdXLXE == md800)
        vmCur.bfHW = (ramtop == 0xA000) ? vmAtari48C : vmAtari48;
    else if (mdXLXE == mdXL)
        vmCur.bfHW = (ramtop == 0xA000) ? vmAtariXLC : vmAtariXL;
    else
        vmCur.bfHW = (ramtop == 0xA000) ? vmAtariXEC : vmAtariXE;
    DisplayStatus();
#endif

    // Swap in BASIC and OS ROMs.

    InitBanks();
//     ReadCart();

    // initialize memory up to ramtop

    for (addr = 0; addr < ramtop; addr++)
        {
        cpuPokeB(addr,0xAA);
        }

    // initialize the 6502

    cpuInit(PokeBAtari);

    // initialize hardware
    // NOTE: at $D5FE is ramtop and then the jump table

    for (addr = 0xD000; addr < 0xD5FE; addr++)
        {
        cpuPokeB(addr,0xFF);
        }

    // CTIA/GTIA

    CONSOL = ((mdXLXE != md800) && (ramtop == 0xC000)) ? 3 : 7;
    PAL = 14;
    TRIG0 = 1;
    TRIG1 = 1;
    TRIG2 = 1;
    TRIG3 = 1;
    *(ULONG *)MXPF  = 0;
    *(ULONG *)MXPL  = 0;
    *(ULONG *)PXPF  = 0;
    *(ULONG *)PXPL  = 0;

    // POKEY

    POT0 = 228;
    POT1 = 228;
    POT2 = 228;
    POT3 = 228;
    POT4 = 228;
    POT5 = 228;
    POT6 = 228;
    POT7 = 228;
    SKSTAT = 0xFF;
    IRQST = 0xFF;
    AUDC1 = 0x00;
    AUDC2 = 0x00;
    AUDC3 = 0x00;
    AUDC4 = 0x00;

    SKCTLS = 0;
    IRQEN = 0;
    SEROUT = 0;

    // ANTIC

    VCOUNT = 0;
    NMIST = 0x1F;

    NMIEN = 0;
    CHBASE = 0;
    PMBASE = 0;
    VSCROL = 0;
    HSCROL = 0;
    DLISTH = 0;
    DLISTL = 0;
    CHACTL = 0;
    DMACTL = 0;

    // PIA

    rPADATA = 255;
    wPBDATA = (ramtop == 0xC000) ? 255: 253;
    PACTL  = 60;
    PBCTL  = 60;

    // misc.

//    wStartScan = 28;

    return TRUE;
}


BOOL __cdecl DumpHWAtari(char *pch)
{
#ifndef NDEBUG
//    RestoreVideo();

    printf("NMIEN  %02X IRQEN  %02X SKCTLS %02X SEROUT %02X\n",
        NMIEN, IRQEN, SKCTLS, SEROUT);
    printf("DLISTH %02X DLISTL %02X VSCROL %02X HSCROL %02X\n",
        DLISTH, DLISTL, VSCROL, HSCROL);
    printf("PMBASE %02X DMACTL %02X CHBASE %02X CHACTL %02X\n",
        PMBASE, DMACTL, CHBASE, CHACTL);
    printf("HPOSP0 %02X HPOSP1 %02X HPOSP2 %02X HPOSP3 %02X\n",
        HPOSP0, HPOSP1, HPOSP2, HPOSP3);
    printf("HPOSM0 %02X HPOSM1 %02X HPOSM2 %02X HPOSM3 %02X\n",
        HPOSM0, HPOSM1, HPOSM2, HPOSM3);
    printf("SIZEP0 %02X SIZEP1 %02X SIZEP2 %02X SIZEP3 %02X\n",
        SIZEP0, SIZEP1, SIZEP2, SIZEP3);
    printf("COLPM0 %02X COLPM1 %02X COLPM2 %02X COLPM3 %02X\n",
        COLPM0, COLPM1, COLPM2, COLPM3);
    printf("COLPF0 %02X COLPF1 %02X COLPF2 %02X COLPF3 %02X\n",
        COLPF0, COLPF1, COLPF2, COLPF3);
    printf("GRAFP0 %02X GRAFP1 %02X GRAFP2 %02X GRAFP3 %02X\n",
        GRAFP0, GRAFP1, GRAFP2, GRAFP3);
    printf("PRIOR  %02X COLBK  %02X GRAFM  %02X SIZEM  %02X\n",
        PRIOR, COLBK, GRAFM, SIZEM);
    printf("AUDF1  %02X AUDC1  %02X AUDF2  %02X AUDC2  %02X\n",
        AUDF1, AUDC1, AUDF2, AUDC2);
    printf("AUDF3  %02X AUDC3  %02X AUDF4  %02X AUDC4  %02X\n",
        AUDF3, AUDC3, AUDF4, AUDC4);
    printf("AUDCTL %02X  $87   %02X   $88  %02X  $A2  %02X\n",
        AUDCTL, cpuPeekB(0x87), cpuPeekB(0x88), cpuPeekB(0xA2));

    return TRUE;

#ifdef HDOS16ORDOS32
    _bios_keybrd(_NKEYBRD_READ);
    SaveVideo();
#endif
    ForceRedraw();
#endif // DEBUG
    return TRUE;
}


void Interrupt()
{
    cpuPokeB(regSP, regPC >> 8);  regSP = (regSP-1) & 255 | 256;
    cpuPokeB(regSP, regPC & 255); regSP = (regSP-1) & 255 | 256;
    cpuPokeB(regSP, regP);          regSP = (regSP-1) & 255 | 256;

    regP |= IBIT;
}

BOOL __cdecl TraceAtari()
{
    fTrace = TRUE;
    ExecuteAtari();
    fTrace = FALSE;

    return TRUE;
}


BOOL fDumpHW;

BOOL __cdecl ExecuteAtari()
{
    vpcandyCur = &vrgcandy[v.iVM];

#ifndef HWIN32
    if (!fTrace)
        {
        SaveVideo();
        ForceRedraw();
        }
#endif

    fStop = 0;

    do
        {
        RANDOM = (RANDOM << 1) ^ 7 + (BYTE)wScan ^ regA - btick;

        if (wLeft == 0)
            {
            // Display scan line here

//            ProcessScanLine(0);
            ProcessScanLine(1);

#ifdef HWIN32
            // This helps with choppy sound as it allows the next
            // sound to start before we go render the scan line

            SoundDoneCallback();
#endif

#ifndef NDEBUG
            if (fDumpHW)
                {
                WORD PCt = cpuPeekW(0x200);
                extern void CchDisAsm(WORD *);
                int i;

                printf("\n\nscan = %d, DLPC = %04X, %02X\n", wScan, DLPC,
                    cpuPeekB(DLPC));
                for (i = 0; i < 60; i++)
                    {
                    CchDisAsm(&PCt); printf("\n");
                    }
                PCt = regPC;
                for (i = 0; i < 60; i++)
                    {
                    CchDisAsm(&PCt); printf("\n");
                    }
                FDumpHWVM();
                }
#endif // DEBUG

            // next scan line

            wScan = wScan+1;
            if (wScan >= 262)
                wScan = 0;

            // hack for programs that check $D41B instead of $D40B
            rgbMem[0xD41B] =

            VCOUNT = (BYTE)(wScan >> 1);

            ProcessScanLine(0);

            // Reinitialize clock cycle counter

            wLeft = (fBrakes && (DMACTL & 3)) ? 20 : 30;
            wLeft *= clockMult;

            if (wScan < 250)
                {
                }

            else if (wScan == 250)
                {
                // start the NMI status early so that programs
                // that care can see it

                NMIST = 0x40 | 0x1F;
                }
            else if (wScan == 251)
                {
#ifndef NDEBUG
                fDumpHW = 0;
#endif

                // Process VBI here

                countJiffies++;

                if ((countJiffies % 180) == 0)
                    {
                    ULONG lastMs = Getms(), currMs, delta;
                    static ULONG lastInstr;
                    ULONG kHz;

                    QueryTickCtr();
                    currMs = Getms();

                    delta = currMs - lastMs;

#if !defined(NDEBUG)
                    printf("Getms = %d, delta = %d\n", Getms(), delta);
#endif

                    printf("guest time: %3dh %02dm %02ds    ",
                        countJiffies / (3600 * 60),
                        (countJiffies / 3600) % 60,
                        (countJiffies / 60) % 60);

                    printf("Minstrs: %6d    ", countInstr/1000000);

                    if (delta)
                        {
                        kHz = clockMult * 1790 * 3000 / delta;

                        printf("%4u kips    ", (countInstr - lastInstr) / delta);
                        printf("effective 6502 clock speed = %4d.%02d MHz\n", kHz / 1000, (kHz % 1000 / 10));

                        lastInstr = countInstr;
                        }

                    else
                        printf("\n");
                    }

                if (NMIEN & 0x40)
                    {
                    // VBI enabled, generate VBI

                    Interrupt();
                    NMIST = 0x40 | 0x1F;
                    regPC = cpuPeekW(0xFFFA);
                    }

                // check joysticks before keyboard in case user is pressing
                // a keyboard cursor key

#ifndef HWIN32
                if (!(wFrame & 3) && fJoy)
                    {
                    MyReadJoy();

                    rPADATA = 255;

                    if (wJoy0XCal)
                        {
                        // if joystick 0 present

                        if ((wJoy0XCal-50) > wJoy0X)
                            rPADATA &= ~4;              // left

                        else if ((wJoy0XCal+50) < wJoy0X)
                            rPADATA &= ~8;              // right

                        if ((wJoy0YCal-50) > wJoy0Y)
                            rPADATA &= ~1;              // up

                        else if ((wJoy0YCal+50) < wJoy0Y)
                            rPADATA &= ~2;              // down
                        }

                    if (wJoy1XCal)
                        {
                        // if joystick 1 present

                        if ((wJoy1XCal-10) > wJoy1X)
                            rPADATA &= ~0x40;              // left

                        else if ((wJoy1XCal+10) < wJoy1X)
                            rPADATA &= ~0x80;              // right

                        if ((wJoy1YCal-10) > wJoy1Y)
                            rPADATA &= ~0x10;              // up

                        else if ((wJoy1YCal+10) < wJoy1Y)
                            rPADATA &= ~0x20;              // down
                        }

                    TRIG0 = ((bJoyBut & 0x30) != 0x30) ? 0 : 1;
                    TRIG1 = ((bJoyBut & 0xC0) != 0xC0) ? 0 : 1;

                    UpdatePorts();
                    }
#endif // !HWIN32

                CheckKey();

                cVBI++;
                wFrame++;

                if (/* fBrakes && */ (CHBASE != 224) && !(wFrame%3))
                    {
                    // if using redefined characters then force a full
                    // screen redraw once in a while in case the
                    // character set is being changed on the fly
                    // as in Boulder Dash.

                    static long chksum;
                    long newchk = 0;
                    int i;

                    for (i = 0; i < 256; i++)
                        newchk += (long)cpuPeekB((CHBASE << 8) + i + i);

                    if (newchk != chksum)
                        {
                        chksum = newchk;
                        ForceRedraw();
                        }
                    }
                else if (fTrace)
                    {
                    ForceRedraw();
                    }

                if (fSound)
                    UpdatePokey();

                // every VBI, shadow the hardware registers
                // to their higher locations

                memcpy(&rgbMem[0xD410], &rgbMem[0xD400], 16);
                memcpy(&rgbMem[0xD420], &rgbMem[0xD400], 32);
                memcpy(&rgbMem[0xD440], &rgbMem[0xD400], 64);
                memcpy(&rgbMem[0xD480], &rgbMem[0xD400], 128);

                memcpy(&rgbMem[0xD210], &rgbMem[0xD200], 16);
                memcpy(&rgbMem[0xD220], &rgbMem[0xD200], 32);
                memcpy(&rgbMem[0xD240], &rgbMem[0xD200], 64);
                memcpy(&rgbMem[0xD280], &rgbMem[0xD200], 128);

                memcpy(&rgbMem[0xD020], &rgbMem[0xD000], 32);
                memcpy(&rgbMem[0xD040], &rgbMem[0xD000], 64);
                memcpy(&rgbMem[0xD080], &rgbMem[0xD000], 128);

#ifdef HWIN32
                {
                MSG msg;

                // make sure sound has the first crack

                SoundDoneCallback();

#ifdef NDEBUG
                if ((wFrame % 3) == 0)
#endif
                    {
                    if (wScanMin > wScanMac)
                        {
                        // screen is not dirty
                        }
                    else
                        {
                        extern void RenderBitmap();
                        RenderBitmap();
                        }

                    UpdateOverlay();

                    wScanMin = 9999;
                    wScanMac = 0;
                    }

                if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                    fStop = fTrue;

                // decrement printer timer

                if (vi.cPrintTimeout && vmCur.fShare)
                    {
                    vi.cPrintTimeout--;
                    if (vi.cPrintTimeout == 0)
                        {
                        FlushToPrinter();
                        UnInitPrinter();
                        }
                    }
                else
                    FlushToPrinter();
                }
#endif
                if ((fBrakes) && (cVBI >= 3))
                    {
                    while (btick == *pbtick)
                        {
#ifdef HWIN32
                        MSG msg;

                        SoundDoneCallback();

                        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                            fStop = fTrue;

                        Sleep(10);
#endif
                        }

                    btick = *pbtick;
                    cVBI = 0;
//                    WaitForVRetrace();
                    }

                // exit to message loop always, REVIEW: any effect???

                fStop = fTrue;
                }
            else if (wScan > 251)
                {
                wLeft = wLeft | (wLeft >> 2);
                }

            }

        if (!(regP & IBIT))
            {
            if (IRQST != 255)
                {
//        printf("IRQ interrupt\n");
                Interrupt();
                regPC = cpuPeekW(0xFFFE);
                }
            }

        Go6502();

        if (fSIO)
            {
            // REVIEW: need to check if PC == $E459 and if so make sure
            // XL/XE ROMs are swapped in, otherwise ignore

            fSIO = 0;

            SIOV();
            }

        } while (!fTrace && !fStop);

#ifndef HWIN32
    if (!fTrace)
        RestoreVideo();
#endif

    return FALSE;
}


//
// Stubs
//

#ifndef HWIN32

BOOL __cdecl KeyAtari(ULONG l)
{
    // do nothing, keyboard is polled

    return TRUE;
}

#endif

BOOL __cdecl DumpRegsAtari()
{
    // later

    return TRUE;
}


BOOL __cdecl DisasmAtari(char *pch, ADDR *pPC)
{
    // later

    return TRUE;
}


BYTE __cdecl PeekBAtari(ADDR addr)
{
    // reads always read directly

    return cpuPeekB(addr);
}


WORD __cdecl PeekWAtari(ADDR addr)
{
    // reads always read directly

    return cpuPeekW(addr);
}


ULONG __cdecl PeekLAtari(ADDR addr)
{
    // reads always read directly

    return cpuPeekW(addr);
}


BOOL __cdecl PokeWAtari(ADDR addr, WORD w)
{
    PokeBAtari(addr, w & 255);
    PokeBAtari(addr+1, w >> 8);
    return TRUE;
}


BOOL __cdecl PokeLAtari(ADDR addr, ULONG w)
{
    return TRUE;
}


BOOL __cdecl PokeBAtari(ADDR addr, BYTE b)
{
    BYTE bOld;
    addr &= 65535;
    b &= 255; // just to be sure

    // this is an ugly hack to keep wLeft from getting stuck

    if (wLeft > 1)
        wLeft--;

#if 0
    printf("write: addr:%04X, b:%02X, PC:%04X\n", addr, b & 255, regPC);
#endif

    if (addr < ramtop)
        {
#if 0
        printf("poking into RAM, addr = %04X, ramtop = %04X\n", addr, ramtop);
        flushall();
#endif

        cpuPokeB(addr, b);
        return TRUE;
        }

    switch((addr >> 8) & 255)
        {
    default:
        if (mdXLXE == md800)
            break;

        // writing to XL/XE memory

        if (addr == 0xD1FF)
            {
            static const unsigned char rgbSIOR[] =
                {
                0x00, 0x00, 0x04, 0x80, 0x00, 0x4C, 0x70, 0xD7,
                0x4C, 0x80, 0xD7, 0x91, 0x00, 0x00, 0xD7, 0x10,
                0xD7, 0x20, 0xD7, 0x30, 0xD7, 0x40, 0xD7, 0x50,
                0xD7, 0x4C, 0x60, 0xD7
                };

            if (b & 1)
                {
                // swap in XE BUS device 1 (R: handler)

                memcpy(&rgbMem[0xD800], rgbSIOR, sizeof(rgbSIOR));
                }
            else
                {
                // swap out XE BUS device 1 (R: handler)

                memcpy(&rgbMem[0xD800], rgbXLXED800, sizeof(rgbSIOR));
                }
            }

        if (!(PORTB & 1) && (addr >= 0xC000))
            {
            // write to XL/XE RAM under ROM

            if ((addr < 0xD000) || (addr >= 0xD800))
                cpuPokeB(addr, b);

            break;
            }
        break;

    case 0xD0:      // GTIA
        addr &= 31;
        bOld = rgbMem[writeGTIA+addr];
        rgbMem[writeGTIA+addr] = b;

        if (addr == 30)
            {
            // HITCLR

            *(ULONG *)MXPF = 0;
            *(ULONG *)MXPL = 0;
            *(ULONG *)PXPF = 0;
            *(ULONG *)PXPL = 0;

            pmg.fHitclr = 1;

            DebugStr("HITCLR!!!\n");
            }
        else if (addr == 31)
            {
            // CONSOL

            // REVIEW: click the speaker

//            printf("CONSOL %02X %02X\n", bOld, b);

#ifdef HWIN32
            if (!vi.fWinNT)
#endif
            if ((bOld ^ b) & 8)
                {
                // toggle speaker state (mask out lowest bit?)

#if !defined(_M_ARM) && !defined(_M_ARM64)
                outp(0x61, (inp(0x61) & 0xFE) ^ 2);
#endif
                }
            }
        break;

    case 0xD2:      // POKEY
        addr &= 15;
        rgbMem[writePOKEY+addr] = b;

        if (addr == 10)
            {
            // SKRES

            SKSTAT |=0xE0;
            }
        else if (addr == 13)
            {
            // SEROUT

            IRQST &= ~(IRQEN & 0x18);
            }
        else if (addr == 14)
            {
            // IRQEN

            IRQST |= ~b;
            }
        break;

    case 0xD3:      // PIA
        addr &= 3;
        rgbMem[writePIA+addr] = b;

        // The PIA is funky in that it has 8 registers in a window of 4:
        //
        // D300 - PORTA data register (read OR write) OR data direction register
        //      - the data register reads the first two joysticks
        //      - the direction register just behaves as RAM
        //        for each of the 8 bits: 1 = write, 0 = read
        //      - when reading PORTA, write bits behave as RAM
        //
        // D301 - PACTL, controls PORTA
        //      - bits 7 and 6 are ignored, always read as 0
        //      - bits 5 4 3 1 and 0 are ignored and just behave as RAM
        //      - bit 2 selects: 1 = data register, 0 = direction register
        //
        // D302 - PORTB, similar to PORTB, on the XL/XE it controls memory banks
        //
        // D303 - PBCTL, controls PORTB
        //
        // When we get a write to PACTL or PBCTL, strip off the upper two bits and
        // stuff the result into the appropriate read location. The only bit we care
        // about is bit 2 (data/direction select). When this bit toggles we need to
        // update the PORTA or PORTB read location appropriately.
        //
        // When we get a write to PORTA or PORTB, if the appropriate select bit
        // selects the data direction register, write to wPADDIR or wPBDDIR.
        // Otherwise, write to wPADATA or wPBDATA using ddir as a mask.
        //
        // When reading PORTA or PORTB, return (writereg ^ ddir) | (readreg ^ ~ddir)
        //

        if (addr & 2)
            {
            // it is a control register

            cpuPokeB(PACTLea + (addr & 1), b & 0x3C);
            }
        else
            {
            // it is either a data or ddir register. Check the control bit

            if (cpuPeekB(PACTLea + addr) & 4)
                {
                // it is a data register. Update only bits that are marked as write.

                BYTE bMask = cpuPeekB(wPADDIRea + addr);
                BYTE bOld  = cpuPeekB(wPADATAea + addr);
                BYTE bNew  = (b & bMask) | (bOld & ~bMask);

                if (bOld != bNew)
                    {
                    cpuPokeB(wPADATAea + addr, bNew);

                    if ((addr == 1) && (mdXLXE != md800))
                        {
                        // Writing a new value to PORT B on the XL/XE.

                        if (mdXLXE != mdXE)
                            {
                            // in 800XL mode, mask bank select bits
                            bNew |= 0x7C;
                            wPBDATA |= 0x7C;
                            }

                        SwapMem(bOld ^ bNew, bNew, 0);
                        }
                    }
                }
            else
                {
                // it is a data direction register.

                cpuPokeB(wPADDIRea + addr, b);
                }
            }

        // update PORTA and PORTB read registers

        UpdatePorts();
        break;

    case 0xD4:      // ANTIC
        addr &= 15;
        rgbMem[writeANTIC+addr] = b;

        if (addr == 10)
            {
            // WSYNC

            wLeft = 1;
            }
        else if (addr == 15)
            {
            // NMIRES - reset NMI status

            NMIST = 0x1F;
            }
        break;

    case 0xD5:      // Cartridge bank select
//        printf("addr = %04X, b = %02X\n", addr, b);
        BankCart(addr & 255);
        break;
        }

    return TRUE;
}

void UpdatePorts()
{
    if (PACTL & 4)
        {
        BYTE b = wPADATA ^ rPADATA;

        PORTA = (b & wPADDIR) ^ rPADATA;
        }
    else
        PORTA = wPADDIR;

    if (PBCTL & 4)
        {
        BYTE b = wPBDATA ^ rPBDATA;

        PORTB = (b & wPBDDIR) ^ rPBDATA;
        }
    else
        PORTB = wPBDDIR;
}

#endif // XFORMER

