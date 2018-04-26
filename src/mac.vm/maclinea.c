
//
// MACLINEA.C
//
// Apple Macintosh LINEA hooks
//
// 08/27/2007 last update
//

#include "..\gemtypes.h"

#ifdef SOFTMAC

BOOL __cdecl m68k_DumpRegs(void);

#ifndef NDEBUG
#define  TRACEHW   1
#define  TRACEDISK 1
#define  TRACESCSI 1
#define  TRACEIWM  1
#else
#define  TRACEHW   0
#define  TRACEDISK 0
#define  TRACESCSI 0
#define  TRACEIWM  0
#endif

// #pragma optimize("",off)

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

int CallMacLineA()
{
    WORD instr;
    static BOOL fSelected;  // 0 = no device selected, >0 = SCSI ID - 1
    static BOOL fTraceThis;
    extern ULONG lBreakpoint;

#ifndef NDEBUG
    char rgch[1024], rgch2[1024];
#endif

    // If we want to execute the regular Line A trap,
    // just return 1. If we handle the trap, we need to
    // increment the PC by 2 then return 0.

    instr = (WORD)PeekW(vpregs->PC);

#if 0
    printf("$376 = %04X, IPL = %d, ", PeekW(0x376), (vpregs->SR >> 8) & 7);
    printf("%08X: LINEA $%04X, SP = %08X, ticks=%08X\n", vpregs->PC, instr, vpregs->A7, PeekL(0x16A));
#endif

#if 0
        printf("TAG BUFFER = %08X %08X %08X\n",
            PeekL(0x2FC), PeekL(0x300), PeekL(0x304));
#endif

    if (instr & 0x0800)
        {
        // TOOLBOX TRAPS
        //
        // Of the form 1010-1-AP-10 bit trap number ($A800 - $AFFF)
        //
        // The AP bit means the trap was called from glue code and the
        // return address should be popped from the stack.

        ULONG cbAutoPop = ((instr & 0x0400) == 0x0400) ? 4 : 0;
        ULONG eaAutoPop = cbAutoPop ? PeekL(vpregs->A7) : vpregs->PC + 2;

#ifndef NDEBUG
        switch (instr & 0xF3FF)
            {
        default:
            printf("MAC TOOLBOX UNKNOWN TRAP %04X\n", instr);
            break;

        case 0xA015:
            printf("_SCSIDispatch command %d\n", PeekW(vpregs->A7 + cbAutoPop));

            switch(PeekW(vpregs->A7 + cbAutoPop))
                {
            default:
                printf("_SCSI UNKNOWN %02X!!!\n", PeekW(vpregs->A7 + cbAutoPop));
                break;

            case 0:
                printf("_SCSI Reset\n");
                break;

            case 1:
                printf("_SCSI Get\n");
                break;

            case 2:
                printf("_SCSI Select device %d\n", PeekW(vpregs->A7 + cbAutoPop + 2));
                break;

            case 3:
                {
                ULONG buf;

                printf("_SCSI Cmd, buffer = $%08X, count = %d\n",
                    buf = PeekL(vpregs->A7 + cbAutoPop + 4), PeekW(vpregs->A7 + cbAutoPop + 2));
                printf("_SCSI Cmd buffer = %08X %08X %08X\n",
                    PeekL(buf), PeekL(buf + 4), PeekL(buf + 8));
                }
                break;

            case 4:
                printf("_SCSI Complete, pstat = $%08X, pmsg = $%08X, wait = %d\n",
                    PeekL(vpregs->A7 + cbAutoPop + 10), PeekL(vpregs->A7 + cbAutoPop + 6), PeekL(vpregs->A7 + cbAutoPop + 2));
                break;

            case 5:     // SCSIRead
            case 8:     // SCSIReadBlind
            case 6:     // SCSIWrite
            case 9:     // SCSIWriteBlind
                {
                ULONG tibPtr = PeekL(vpregs->A7 + cbAutoPop + 2);
                BOOL fRead  = (PeekW(vpregs->A7 + cbAutoPop) % 3) != 0;

                printf("_SCSI %s, tibPtr = $%08X\n", fRead ? "Read" : "Write", tibPtr);
                printf("_SCSI TIB buffer = %08X %08X %08X\n",
                    PeekW(tibPtr),    PeekL(tibPtr + 2),  PeekL(tibPtr + 6),
                    PeekW(tibPtr+10), PeekL(tibPtr + 12), PeekL(tibPtr + 16));

                for (;;)
                    {
                    ULONG tibCmd = PeekW(tibPtr);
                    ULONG tibL1  = PeekL(tibPtr + 2);
                    ULONG tibL2  = PeekL(tibPtr + 6);

                    printf("TIB = %04X %08X %08X\n", tibCmd, tibL1, tibL2);

                    tibPtr += 10;

                    if (tibCmd == 7)
                        break;
                    }
                }
                break;

            case 10:
                printf("_SCSI Stat\n");
                break;

            case 14:
                printf("_SCSI SCSIMgrBusy\n");
                break;
                }
            break;

        case 0xA051:
            printf("_SetCursor\n");
            break;

        case 0xA052:
            printf("_HideCursor\n");
            break;

        case 0xA053:
            printf("_ShowCursor\n");
            break;

        case 0xA06E:
            printf("_InitGraf\n");
            break;

        case 0xA06F:
            printf("_OpenPort\n");
            break;

        case 0xA0A2:
            printf("_PaintRect\n");
            break;

        case 0xA8B4:
            printf("_FillRoundRect\n");
            break;
            }
#endif // NDEBUG

        switch (instr & 0xF3FF)
            {
        default:
            break;

        case 0xA015:
#if 0
            m68k_DumpRegs();
            printf("_SCSIDispatch command %d cbAutoPop = %d\n", PeekW(vpregs->A7 + cbAutoPop), cbAutoPop);

            lBreakpoint = eaAutoPop;
#endif

            // return 1;

            switch(PeekW(vpregs->A7 + cbAutoPop))
                {
            default:
                return 1;
#if TRACESCSI
                printf("_SCSI UNKNOWN!!!\n");
#endif
                DbgMessageBox(NULL, "SCSI unknown!", vi.szAppName, MB_OK);
                break;

            case 0:
#if TRACESCSI
                printf("_SCSI Reset\n");
#endif

                vpregs->D0 = 0;     // noErr
                vpregs->A7 += 2;    // HACK!!

                fSelected = fFalse;
                goto LhandledSCSI0_2;

LhandledSCSI0:
                vpregs->A7 += 2;    // pop selector
                PokeW(vpregs->A7 + cbAutoPop, vpregs->D0);  // put return value on stack

LhandledSCSI0_2:
                vpregs->PC = eaAutoPop;
                vpregs->A7 += cbAutoPop;

                vpregs->rgfsr.bitZ = (vpregs->D0 == 0);

                vpregs->A0 = vpregs->PC;    // SCSI manager returns through A0

#ifndef NDEBUG
                m68k_DumpRegs();
#endif
                return 0;

            case 1:
#if TRACESCSI
                printf("_SCSI Get\n");
#endif

                vpregs->D0 = 0;     // noErr
                goto LhandledSCSI0;

            case 2:
            case 11:
#if TRACESCSI
                printf("_SCSI Select device %d\n", PeekW(vpregs->A7 + cbAutoPop + 2));
#endif

                fSelected = 1 + PeekW(vpregs->A7 + cbAutoPop + 2);

                if (vi.rgpdi[2 + fSelected - 1] == NULL)
                    fSelected = 0;

                if (fSelected >= 1 && fSelected <= 7)
                    vpregs->D0 = 0;     // noErr
                else
                    {
                    vpregs->D0 = 2;     // scCommErr
                    fSelected = 0;
                    }

                vpregs->A7 += 2;        // pop targetID
                goto LhandledSCSI0;

            case 3:
                {
                ULONG buf = PeekL(vpregs->A7 + cbAutoPop + 4);
                ULONG count = PeekW(vpregs->A7 + cbAutoPop + 2);

                vpregs->A7 += 6;    // pop buffer, count

#if TRACESCSI
     //       if ((PeekB(buf) != 0x08) && (PeekB(buf) != 0x28))
                {
                printf("_SCSI ID = %d\n", fSelected - 1);
                printf("_SCSI Cmd, buffer = $%08X, count = %d\n", buf, count);
                printf("_SCSI Cmd buffer = %08X %08X %08X\n",
                    PeekL(buf), PeekL(buf + 4), PeekL(buf + 8));
                fTraceThis = 1;
                }
#endif

                if (!fSelected || ((count != 6) && (count != 10)))
                    {
                    vpregs->D0 = 2;     // scCommErr
                    goto LhandledSCSI0;
                    }

                vsthw.fHDC = 1;
                vi.iSCSIRead = 0;

                while (count--)
                    vsthw.rgbHDC[vsthw.fHDC++] = (BYTE)PeekB(buf++);

                vpregs->D0 = 0;     // noErr
                goto LhandledSCSI0;
                }

            case 4:
#if TRACESCSI
            if (fTraceThis)
                {
                printf("_SCSI Complete, pstat = $%08X, pmsg = $%08X, wait = %d\n",
                    PeekL(vpregs->A7 + cbAutoPop + 10), PeekL(vpregs->A7 + cbAutoPop + 6), PeekL(vpregs->A7 + cbAutoPop + 2));
                printf("Contents of stat = %04X, msg = %04X\n",
                    PeekW(PeekL(vpregs->A7 + cbAutoPop + 10)),
                    PeekW(PeekL(vpregs->A7 + cbAutoPop + 6)));
                fTraceThis = 0;
                }
#endif

                vsthw.fHDC = 1;

                if (!fSelected)
                    {
                    vpregs->D0 = 2;     // scCommErr
                    goto LhandledSCSI0;
                    }

                PokeW(PeekL(vpregs->A7 + cbAutoPop + 10), 0),
                PokeW(PeekL(vpregs->A7 + cbAutoPop + 6), 0);

                vpregs->A7 += 12;      // pop pstat, pmessage, wait

                vpregs->D0 = 0;     // noErr
                goto LhandledSCSI0;

            case 5:     // SCSIRead
            case 8:     // SCSIReadBlind
            case 6:     // SCSIWrite
            case 9:     // SCSIWriteBlind
                {
                ULONG tibPtr = PeekL(vpregs->A7 + cbAutoPop + 2);
                BOOL fRead  = (PeekW(vpregs->A7 + cbAutoPop) % 3) != 0;

                vpregs->A7 += 4;    // pop tibPtr

#if TRACESCSI
                printf("_SCSI %s, tibPtr = $%08X\n", fRead ? "Read" : "Write", tibPtr);
                printf("_SCSI TIB buffer = %08X %08X %08X\n",
                    PeekW(tibPtr),    PeekL(tibPtr + 2),  PeekL(tibPtr + 6),
                    PeekW(tibPtr+10), PeekL(tibPtr + 12), PeekL(tibPtr + 16));
#endif

                if (!fSelected /* || !fRead */)
                    {
                    vpregs->D0 = 2;     // scCommErr
                    goto LhandledSCSI0;
                    }

                if (fRead)
                    HandleSCSICmd(fSelected - 1);

                for (;;)
                    {
                    ULONG tibCmd = PeekW(tibPtr);
                    ULONG tibL1  = PeekL(tibPtr + 2);
                    ULONG tibL2  = PeekL(tibPtr + 6);

#if TRACESCSI
                    printf("TIB at %08X = %04X %08X %08X\n", tibPtr, tibCmd, tibL1, tibL2);
#endif

                    tibPtr += 10;

                    switch(tibCmd)
                        {
                    default:
#if TRACESCSI
                        printf("unknown TIB CMD %d!\n", tibCmd);
#endif
                        vpregs->D0 = 2;     // scCommErr
                        goto LhandledSCSI0;

                    case 1:         // scInc
                        PokeL(tibPtr-8, tibL1 + tibL2);

                    case 2:         // scNoInc
                        {
                        ULONG i;

#if TRACESCSI
              //          if (tibL2 < 256)
                            printf("copying %d bytes %s location $%08X\n", tibL2,
                                 fRead ? "to" : "from", tibL1);
#endif

                        if (vi.cbSCSIAlloc < (vi.iSCSIRead + tibL2))
                            {
                            if (!VirtualAlloc(vi.pvSCSIData, vi.cbSCSIAlloc = (vi.iSCSIRead + tibL2), MEM_COMMIT, PAGE_READWRITE))
                                {
#if !defined(NDEBUG) || defined(BETA) || TRACESCSI
                                printf("SCSI alloc size %08X failed!\n",
                                    vi.cbSCSIAlloc);
#endif
                                vi.cbSCSIAlloc = 0;
                                break;
                                }
                            }

#if !defined(DEMO)
                        if (fRead)
                            {
                            if (HostToGuestDataCopy(tibL1,
                                &vi.pvSCSIData[vi.iSCSIRead],
                                ACCESS_PHYSICAL_WRITE, tibL2))
                                {
                                vi.iSCSIRead += tibL2;
                                break;
                                }
                            }
                        else
                            {
                            if (GuestToHostDataCopy(tibL1,
                                &vi.pvSCSIData[vi.iSCSIRead],
                                ACCESS_PHYSICAL_READ, tibL2))
                                {
                                vi.iSCSIRead += tibL2;
                                break;
                                }
                            }
#endif

                        for (i = 0; i < tibL2; i++)
                            {
                            if (fRead)
                                {
#if TRACESCSI
                                if (tibL2 < 256 || (vi.iSCSIRead < 16))
                                    printf("%08X %02X\n", vi.iSCSIRead, vi.pvSCSIData[vi.iSCSIRead]);
#endif

                                PokeB(tibL1+i, vi.pvSCSIData[vi.iSCSIRead++]);
                                }
                            else
                                {
                                vi.pvSCSIData[vi.iSCSIRead++] = (BYTE)PeekB(tibL1+i);

#if TRACESCSI
                                if (0 || tibL2 < 256 || (vi.iSCSIRead < 16))
                                    printf("%08X %02X\n", vi.iSCSIRead-1, vi.pvSCSIData[vi.iSCSIRead-1]);
#endif
                                }
                            }
                        }

                        break;

                    case 3:         // scAdd
                        PokeL(tibL1, PeekL(tibL1) + tibL2);
                        break;

                    case 4:         // scMove
                        PokeL(tibL2, PeekL(tibL1));
                        break;

                    case 5:         // scLoop
                        PokeL(tibPtr - 4, tibL2 - 1);

                        if (tibL2 != 1)
                            tibPtr += tibL1 - 10;

                        break;

                    case 6:         // scNop
                        break;

                    case 7:         // scStop
                        vpregs->D0 = 0;     // noErr

                        if (!fRead)
                            HandleSCSICmd(fSelected - 1);

                        goto LhandledSCSI0;
                        }
                    }

                // should not reach here, but just in case

                vpregs->D0 = 2;     // scCommErr
                goto LhandledSCSI0;
                }

            case 10:
#if TRACESCSI
                printf("_SCSI Stat\n");
#endif

                if (vsthw.byte0 == 0x04)    // last command was format
                    vpregs->D0 = 0x037F;
                else
                    vpregs->D0 = 0;

                goto LhandledSCSI0;

            case 12:
#if TRACESCSI
                printf("_SCSI MsgIn\n");
#endif

                vpregs->A7 += 4;    // pop pmessage
                vpregs->D0 = 0;     // noErr
                goto LhandledSCSI0;

            case 13:
#if TRACESCSI
                printf("_SCSI MsgOut\n");
#endif

                vpregs->A7 += 2;    // pop message
                vpregs->D0 = 0;     // noErr
                goto LhandledSCSI0;

            case 14:
#if TRACESCSI
                printf("_SCSI MgrBusy\n");
#endif
                vpregs->D0 = 0;     // noErr
                goto LhandledSCSI0;
                }
            break;

        case 0xA1C9:
           // printf("_Syserror %08X %08X\n", vpregs->PC, vpregs->D0);

            // Mac OS 8 HACK!!!

            if (vi.cbROM[0] == 0x80000)
            if (vmCur.bfCPU >= cpu68020)
            if (vpregs->PC == (PeekL(0x570) + 2))
              if (vpregs->D0 == 12)
                {
                // MemoryDispatch(-3)

                vpregs->D0 = 4096;

                // RTS

                vpregs->PC = PeekL(vpregs->A7);
                vpregs->A7 += 4;

                return 0;
                }

#if 0
            printf("_Syserror\n");
            m68k_DumpRegs();
            m68k_DumpHistory(1000);
            break;
#endif
            }
        }
    else
        {
        // OS TRAPS
        //
        // Of the form 1010-0-fSys-fImmed-8 bit trap number ($A000 - $A7FF)

        ULONG fSys  = ((instr & 0x0400) == 0x0400);
        ULONG fImm  = ((instr & 0x0200) == 0x0200);
        ULONG fInA0 = ((instr & 0x0100) == 0x0100);

#ifndef NDEBUG
        switch (instr & 0xF0FF)
            {
        default:
            break;

        case 0xA000:
        case 0xA001:
        case 0xA002:
        case 0xA003:
        case 0xA004:
        case 0xA005:
        case 0xA017:
            printf("ioCompletion: %08X\n", PeekL(vpregs->A0 + 12));
            printf("ioVRefNum   : %04X\n", PeekW(vpregs->A0 + 22));
            printf("ioRefNum    : %04X\n", PeekW(vpregs->A0 + 24));
            printf("iocsCode    : %04X\n", PeekW(vpregs->A0 + 26));
            printf("iocsParam   : %04X\n", PeekW(vpregs->A0 + 28));
            printf("ioBuffer    : %08X\n", PeekL(vpregs->A0 + 32));
            printf("ioReqCount  : %08X\n", PeekL(vpregs->A0 + 36));
            printf("ioActCount  : %08X\n", PeekL(vpregs->A0 + 40));
            printf("ioPosMode   : %04X\n", PeekW(vpregs->A0 + 44));
            printf("ioPosOffset : %08X\n", PeekL(vpregs->A0 + 46));
            }

        switch (instr & 0xF0FF)
            {
        default:
            printf("MAC OS UNKNOWN TRAP %04X\n", instr);
            break;

        case 0xA000:
            printf("_PBOpen\n");
            break;

        case 0xA001:
            printf("_PBClose\n");
            break;

        case 0xA002:
            printf("_PBRead\n");
            break;

        case 0xA003:
            printf("_PBWrite\n");
            break;

        case 0xA004:
            printf("_PBControl\n");
            break;

        case 0xA005:
            printf("_PBStatus\n");
            break;

        case 0xA006:
            printf("_PBKillIO\n");
            break;

        case 0xA007:
            printf("_PBGetVInfo\n");
            break;

        case 0xA008:
            printf("_PBCreate\n");
            break;

        case 0xA009:
            printf("_PBDelete\n");
            break;

        case 0xA00A:
            printf("_PBOpenRF\n");
            break;

        case 0xA00B:
            printf("_PBRename\n");
            break;

        case 0xA00C:
            printf("_PBGetInfo\n");
            break;

        case 0xA00D:
            printf("_PBSetFInfo\n");
            break;

        case 0xA00E:
            printf("_PBUnmountVol\n");
            break;

        case 0xA00F:
            printf("_PBMountVol\n");
            break;

        case 0xA010:
            printf("_PBAllocate\n");
            break;

        case 0xA011:
            printf("_PBGetEOF\n");
            break;

        case 0xA012:
            printf("_PBSetEOF\n");
            break;

        case 0xA013:
            printf("_PBFlushVol\n");
            break;

        case 0xA014:
            printf("_PBGetVol\n");
            break;

        case 0xA015:
            printf("_PBSetVol\n");
            break;

        case 0xA017:
            printf("_PBEject\n");
            break;

        case 0xA018:
            printf("_PBGetFPos\n");
            break;

        case 0xA019:
            printf("_InitZone\n");
            break;

        case 0xA02E:
            printf("_BlockMove %d bytes from %08X to %08X\n",
                vpregs->D0, vpregs->A0, vpregs->A1);
            break;

        case 0xA031:
            printf("_GetOSEvent\n");
            break;

        case 0xA03C:
#ifndef NDEBUG
            printf("_CmpString: %08X %08X %08X\n",
                vpregs->A0, vpregs->A1, vpregs->D0);
            Str68ToStrN(vpregs->A0, rgch, 256);
            Str68ToStrN(vpregs->A1, rgch2, 256);
            rgch[vpregs->D0 >> 16] = 0;
            rgch2[vpregs->D0 & 255] = 0;
            printf("'%s' vs. '%s'\n", rgch, rgch2);
#endif
            break;

        case 0xA03D:
            printf("_DrvrInstall\n");
            break;

        case 0xA04C:
            printf("_CompactMem\n");
            break;

        case 0xA051:
            printf("_ReadXPRAM, offset = %02X, length = %02X\n",
                vpregs->D0 & 65535, vpregs->D0 >> 16);
            break;

        case 0xA052:
            printf("_WriteXPRAM, offset = %02X, length = %02X\n",
                vpregs->D0 & 65535, vpregs->D0 >> 16);
            break;

        case 0xA05D:
            printf("_SwapMMUMode %d\n", vpregs->D0);
            break;

        case 0xA093:
            printf("_Microseconds %d\n", vpregs->D0);
            break;

        case 0xA022:
            printf("_NewHandle\n");
            break;

        case 0xA01E:
            printf("_NewPtr\n");
            break;

        case 0xA060:
            printf("_HFSDispatch sel %08X\n", vpregs->D0);

            switch(vpregs->D0)
                {
            case 9:
                printf("CatInfo ");
                {
                char rgb[256];

                GetStringP(PeekL(vpregs->A0 + 18), rgb);
                printf("'%s'\n", rgb);
                }
                break;

            case 27:
                printf("MakeFSSpec ");
                {
                char rgb[256];

                GetStringP(PeekL(vpregs->A0 + 18), rgb);
                printf("'%s'\n", rgb);
                }
                break;
                }
            break;
            }
#endif // NDEBUG

        switch(instr & 0xF0FF)
            {
        default:
            break;

        case 0xA002:
        case 0xA003:
            {
            static fFirst = TRUE;
            int fWrite = ((instr & 0xF3FF) == 0xA003);

#if TRACEDISK
            printf("ioCompletion: %08X\n", PeekL(vpregs->A0 + 12));
            printf("ioVRefNum   : %04X\n", PeekW(vpregs->A0 + 22));
            printf("ioRefNum    : %04X\n", PeekW(vpregs->A0 + 24));
            printf("iocsCode    : %04X\n", PeekW(vpregs->A0 + 26));
            printf("iocsParam   : %04X\n", PeekW(vpregs->A0 + 28));
            printf("ioBuffer    : %08X\n", PeekL(vpregs->A0 + 32));
            printf("ioReqCount  : %08X\n", PeekL(vpregs->A0 + 36));
            printf("ioActCount  : %08X\n", PeekL(vpregs->A0 + 40));
            printf("ioPosMode   : %04X\n", PeekW(vpregs->A0 + 44));
            printf("ioPosOffset : %08X\n", PeekL(vpregs->A0 + 46));
#endif

#if 0
            if (PeekW(vpregs->A0 + 24) == 0xFFF7)    // .BOut (printer port) driver
                {
                if (fWrite)
                    {
                    printf("WRITING TO PRINTER PORT\n");
                    printf("ioBuffer    : %08X\n", PeekL(vpregs->A0 + 32));
                    printf("ioReqCount  : %08X\n", PeekL(vpregs->A0 + 36));

                    PokeW(vpregs->A0 + 16, 0);  // noErr
                    vpregs->D0 = 0; // noErr
                    vpregs->rgfsr.bitZ = 1;
                    vpregs->rgfsr.bitN = 0;

                    fSys = 0;
                    goto Lreadcomplete;
                    }
                }
            else
#endif

            if (PeekW(vpregs->A0 + 24) == 0xFFFB)    // disk driver
                {
                PDI pdi;
                int sector;
                int count;

//                printf("_PB%s FAKE DISK OP!\n", fWrite ? "Write" : "Read");

                vmachw.drive =     PeekW(vpregs->A0 + 22);
                vmachw.posmode =   PeekW(vpregs->A0 + 44);
                vmachw.posoff =    PeekL(vpregs->A0 + 46);
                vmachw.pbIO =      PeekL(vpregs->A0 + 32);
                vmachw.cbIO =      PeekL(vpregs->A0 + 36);

#if !defined(NDEBUG) || TRACEIWM
                printf("\n\n%08X: ", vpregs->PC);
                printf("_PB%s drive %X, sector $%08X\n",
                        fWrite ? "Write" : "Read",
                        vmachw.drive, vmachw.posoff >> 9);
#endif

#if !defined(NDEBUG) || TRACEIWM
            printf("ioCompletion: %08X\n", PeekL(vpregs->A0 + 12));
            printf("ioVRefNum   : %04X\n", PeekW(vpregs->A0 + 22));
            printf("ioRefNum    : %04X\n", PeekW(vpregs->A0 + 24));
            printf("iocsCode    : %04X\n", PeekW(vpregs->A0 + 26));
            printf("iocsParam   : %04X\n", PeekW(vpregs->A0 + 28));
            printf("ioBuffer    : %08X\n", PeekL(vpregs->A0 + 32));
            printf("ioReqCount  : %08X\n", PeekL(vpregs->A0 + 36));
            printf("ioActCount  : %08X\n", PeekL(vpregs->A0 + 40));
            printf("ioPosMode   : %04X\n", PeekW(vpregs->A0 + 44));
            printf("ioPosOffset : %08X\n", PeekL(vpregs->A0 + 46));

            m68k_DumpRegs();
            m68k_DumpHistory(800);
#endif

#if 0
                DisplayStatus();
#endif

                DebugStr("vmachw.pbIO = %08X\n", vmachw.pbIO);

                if (1 && (vmachw.drive != 1) && (vmachw.drive != 2))
                    {
                    // fail every drive other than drive 1 and 2

                    PokeW(vpregs->A0 + 16, -56);  // nsDrvErr
                    vpregs->D0 = -56; // nsDrvErr
                    vpregs->rgfsr.bitZ = 0;
                    vpregs->rgfsr.bitN = 1;

                    goto Lreadcomplete;
                    }

                // drive is a virtual disk or physical drive

                sector = vmachw.posoff >> 9;
                count  = vmachw.cbIO >> 9;

#if !defined(NDEBUG) || TRACEIWM
                printf("\nMac floppy I/O: drive = %d, sector = %5d, count = %3d\n",
                    vmachw.drive, sector, count);
#endif

                // if the DISKINFO structure doesn't exist, forget it

                pdi = vi.rgpdi[vmachw.drive-1];

                if (pdi == NULL)
                    {
#if !defined(NDEBUG) || defined(BETA) || TRACEIWM
                    printf("No device mapped for floppy drive %d, failing disk I/O!\n", vmachw.drive);
#endif
                    goto Lreadfail;
                    }

                while (count > 0)
                    {
                    int countT = max(count,1);

                    // limit to 18 sectors so as not to overflow rgbDiskBuffer

                    if ((sector % 18) + countT > 18)
                        countT = 18 - (sector % 18);

#if !defined(NDEBUG) || TRACEIWM
                    printf("    reading sector = %5d, count = %3d, countT = %3d, ",
                        sector, count, countT);
#endif

                    vsthw.DMAbasis = vmachw.pbIO;

                    pdi->sec = sector;
                    pdi->count = countT;
                    pdi->lpBuf = (BYTE *)&vsthw.rgbDiskBuffer;

                    if (fWrite)
                        {
                        GuestToHostDataCopy(vmachw.pbIO,
                            vsthw.rgbDiskBuffer,
                            ACCESS_PHYSICAL_READ, countT * 512);
                        }

                    if (FRawDiskRWPdi(pdi, fWrite))
                        {
                        if (!fWrite)
                            {
                            HostToGuestDataCopy(vmachw.pbIO,
                                vsthw.rgbDiskBuffer,
                                ACCESS_PHYSICAL_WRITE, countT * 512);
                            }

                        PokeW(vpregs->A0 + 16, 0);  // noErr
                        vpregs->D0 = 0; // noErr
                        vpregs->rgfsr.bitZ = 1;
                        vpregs->rgfsr.bitN = 0;

#if !defined(NDEBUG) || TRACEIWM
                        printf("I/O SUCCEEDED!\n");
#endif
                        }
                    else
                        {
Lreadfail:
                        PokeW(vpregs->A0 + 16, -65);  // noLinErr
                        vpregs->D0 = -65; // noLinErr
                        vpregs->rgfsr.bitZ = 0;
                        vpregs->rgfsr.bitN = 1;
#if !defined(NDEBUG) || defined(BETA) || TRACEIWM
                        printf("I/O FAILED!!!\n");
#endif

Sleep(200);
                        break;
                        }

#if !defined(NDEBUG) || TRACEIWM
                    printf("    data = %02X %02X %02X %02X\n",
                        vsthw.rgbDiskBuffer[0],
                        vsthw.rgbDiskBuffer[1],
                        vsthw.rgbDiskBuffer[2],
                        vsthw.rgbDiskBuffer[3]);
#endif

                    count -= countT;
                    sector += countT;
                    vmachw.pbIO += (countT << 9);
                    }

Lreadcomplete:
                vpregs->PC += 2;

                if (fSys)
                  {
                  if (PeekL(vpregs->A0 + 12) != 0xFFFFFFFF)
                    if (PeekL(vpregs->A0 + 12) != 0x00000000)
                    {
                        // I/O completion routine, yikes!

#if TRACEDISK
                        printf("calling I/O completion routine at %08X\n", PeekL(vpregs->A0 + 12));
#endif

                        PokeL(vpregs->A7 - 4, vpregs->PC);
                        vpregs->A7 -= 4;
                        vpregs->PC = PeekL(vpregs->A0 + 12);
                        PokeB(0x376, PeekB(0x376) & 0x7F);
                    }
                  }

                else
                    {
                    // synchronous calls clear the completion routine address

                    PokeL(vpregs->A0 + 12, 0);
                    }

                return 0;
                }
#if TRACEDISK
            else
                {
                vmachw.drive =     PeekW(vpregs->A0 + 22);
                vmachw.posmode =   PeekW(vpregs->A0 + 44);
                vmachw.posoff =    PeekL(vpregs->A0 + 46);
                vmachw.pbIO =      PeekL(vpregs->A0 + 32);
                vmachw.cbIO =      PeekL(vpregs->A0 + 36);

                printf("%08X: ", vpregs->PC);
                printf("_PBRead/write OTHER drive %X, sec $%08X, dev $%02X\n",
                        vmachw.drive, PeekW(vpregs->A0 + 46),
                        PeekW(vpregs->A0 + 24));
                }
#endif

            // printf("_PB%s\n", fWrite ? "Write" : "Read");
            }
            return 1;

        case 0xA004:
            if (1 && PeekW(vpregs->A0 + 24) == 0xFFFB)    // disk driver
                {
                vmachw.drive =     PeekW(vpregs->A0 + 22);
                vmachw.posmode =   PeekW(vpregs->A0 + 44);
                vmachw.posoff =    PeekL(vpregs->A0 + 46);
                vmachw.pbIO =      PeekL(vpregs->A0 + 32);
                vmachw.cbIO =      PeekL(vpregs->A0 + 36);

#if TRACEDISK
                printf("%08X: ", vpregs->PC);
                printf("_PBControl drive %X, cmd $%02X\n",
                        vmachw.drive, PeekW(vpregs->A0 + 26));
#endif

                if ((vmachw.drive == 1) || (vmachw.drive == 2)) // drive 1 or 2
                    {
                    if (PeekW(vpregs->A0 + 26) == 6)    // Format
                        {
                        PokeW(vpregs->A0 + 16, 0);  // noErr
                        vpregs->D0 = 0; // noErr
                        vpregs->rgfsr.bitZ = 1;
                        vpregs->rgfsr.bitN = 0;

                        goto Lreadcomplete;
                        }
                    else if (PeekW(vpregs->A0 + 26) == 7)    // DiskEject
                        {
                        if (vi.rgpdi[vmachw.drive-1])
                            {
                            // printf("FLUSHING FLOPPY %d\n", vmachw.drive);

                            FlushCachePdi(vi.rgpdi[vmachw.drive-1]);
                            vi.rgpdi[vmachw.drive-1]->fEjected = vmachw.drive;
                            }
L_eject:
#if 0
                        if (vi.rgpdi[vmachw.drive-1])
                            CloseDiskPdi(vi.rgpdi[vmachw.drive-1]);
                        vi.rgpdi[vmachw.drive-1] = NULL;
                        // HandleDiskEject(vmachw.drive-1);
#endif

                        PokeW(vpregs->A0 + 16, 0);  // noErr
                        vpregs->D0 = 0; // noErr
                        vpregs->rgfsr.bitZ = 1;
                        vpregs->rgfsr.bitN = 0;

                        goto Lreadcomplete;
                        }
                    else if (PeekW(vpregs->A0 + 26) == 8) // DriveStatus
                        {
             //   MessageBox(NULL, "DriveStatus!", vi.szAppName, MB_OK);
                        }

                    else if (PeekW(vpregs->A0 + 26) == 23) // DriveInfo
                        {
                        // return the drive type at csParam+28

                        PDI pdi = vi.rgpdi[vmachw.drive-1];

                        if (pdi && (pdi->cbSector > 512))
                            PokeL(vpregs->A0 + 28, 0x0B01); // SCSI CD-ROM
                        else if (pdi && (pdi->size > 2880))
                            PokeL(vpregs->A0 + 28, 0x0601); // SCSI removable
                        else if (vmachw.drive == 2)
                            PokeL(vpregs->A0 + 28, 0x0104); // FDHD (external)
                        else
                            PokeL(vpregs->A0 + 28, 0x0004); // FDHD

                        PokeW(vpregs->A0 + 16, 0);  // noErr
                        vpregs->D0 = 0; // noErr
                        vpregs->rgfsr.bitZ = 1;
                        vpregs->rgfsr.bitN = 0;

                        goto Lreadcomplete;
                        }

#if 0
                    PokeW(vpregs->A0 + 16, 0);  // noErr
                    vpregs->D0 = 0; // noErr
                    vpregs->rgfsr.bitZ = 1;
                    vpregs->rgfsr.bitN = 0;
#if 0
                    printf("_PBControl FAKE DISK OP $%04X!\n",
                        PeekW(vpregs->A0 + 26));
#endif

                    vpregs->PC += 2;
                    return 0;
                    }
                else
                    {
                    PokeW(vpregs->A0 + 16, -56);  // nsDrvErr
                    vpregs->D0 = -56; // nsDrvErr
                    vpregs->rgfsr.bitZ = 0;
                    vpregs->rgfsr.bitN = 1;
#if 0
                    printf("_PBControl FAILED FAKE DISK OP $%04X!\n",
                        PeekW(vpregs->A0 + 26));
#endif
//                    goto Lreadcomplete;

                    vpregs->PC += 2;
                    return 0;
#endif // 0
                    }
                }
            else
                {
                vmachw.drive =     PeekW(vpregs->A0 + 22);
                vmachw.posmode =   PeekW(vpregs->A0 + 44);
                vmachw.posoff =    PeekL(vpregs->A0 + 46);
                vmachw.pbIO =      PeekL(vpregs->A0 + 32);
                vmachw.cbIO =      PeekL(vpregs->A0 + 36);

#if TRACEDISK
                printf("%08X: ", vpregs->PC);
                printf("_PBControl OTHER drive %X, cmd $%02X, dev $%02X\n",
                        vmachw.drive, PeekW(vpregs->A0 + 26),
                        PeekW(vpregs->A0 + 24));
#endif

                // HACK! some sort of command issued during Shutdown
                // If we don't return a noErr it may cause the Mac OS
                // to hang on a Restart or Shutdown

                if ((PeekW(vpregs->A0 + 26) == 0xFFFF) && (PeekW(vpregs->A0 + 24) == 0xFFF6))
                    {
                    vpregs->D0 = 0; // noErr
                    vpregs->rgfsr.bitZ = 1;
                    vpregs->rgfsr.bitN = 0;
                    vpregs->PC += 2;
                    return 0;
                    }

                if ((PeekW(vpregs->A0 + 26) == 0x00FC) && (PeekW(vpregs->A0 + 24) == 0xFFF6))
                    {
                    vpregs->D0 = 0; // noErr
                    vpregs->rgfsr.bitZ = 1;
                    vpregs->rgfsr.bitN = 0;
                    vpregs->PC += 2;
                    return 0;
                    }
#if 0
                if ((vmachw.drive > 2) && (vmachw.drive < 9) &&
                     (PeekW(vpregs->A0 + 26) == 7))   // DiskEject
                    {
                    int i, newdrive = 1;

                    for (i = 2; i < 9; i++)
                        {
                        PVD pvd = &vmCur.rgvd[i];

                        if (pvd->dt != DISK_NONE)
                            {
                            newdrive++;

                            if (newdrive == vmachw.drive)
                                {
printf("EJECTING pdi %d, Mac drive %d\n", i, vmachw.drive);
                                vmachw.drive = i+1;
                                // goto L_eject;
                                }
                            }
                        }
                    }
#endif
                }

            // printf("_PBControl\n");
            return 1;

        case 0xA00F:
#if TRACEDISK
            printf("_PBMountVol drive %X\n", PeekW(vpregs->A0 + 22));
#endif

            return 1;
            if (1)
                {
                if (PeekW(vpregs->A0 + 22) == 1)    // drive 1
                    {
                    vpregs->PC += 4;
                    PokeW(vpregs->A0 + 16, 0);  // noErr
                    vpregs->D0 = 0; // noErr
                    vpregs->rgfsr.bitZ = 1;
                    vpregs->rgfsr.bitN = 0;
#ifndef NDEBUG
                    printf("_PBMountVol FAKE!\n");
#endif
                    return 0;
                    }
                else
                    {
                    }
                }
            return 1;

        case 0xA031:
            // printf("_GetOSEvent\n");
// printf("_GetOSEvent 1\n");

            if (1)
                {
// printf("_GetOSEvent 2\n");
                if (vpregs->D0 & 0x0080) // disk inserted mask
                    {
                    int i;

Ltrydrive:
// printf("_GetOSEvent 3\n");
                    if (vmachw.fDisk1Dirty)
                        {
// printf("_GetOSEvent 4\n");
                        i = 1;
                        vmachw.fDisk1Dirty = fFalse;
                        if (vi.rgpdi[0] && vi.rgpdi[0]->fEjected)
                            vi.rgpdi[0]->fEjected = fFalse;
                        }
                    else if (vmachw.fDisk2Dirty)
                        {
// printf("_GetOSEvent 5\n");
                        i = 2;
                        vmachw.fDisk2Dirty = fFalse;
                        if (vi.rgpdi[1] && vi.rgpdi[1]->fEjected)
                            vi.rgpdi[1]->fEjected = fFalse;
                        }
                    else
                        {
// printf("_GetOSEvent 6\n");
                        for (i = 2; i < vmCur.ivdMac; i++)
                            {
                            if (vi.rgpdi[i] && vi.rgpdi[i]->fEjected)
                                break;
                            }

                        if (i == vmCur.ivdMac)
                            return 1;

                        // map the pdi to the Mac OS drive number

                        i = vi.rgpdi[i]->fEjected;
                        vi.rgpdi[i]->fEjected = fFalse;
                        }

// printf("_GetOSEvent 7\n");
#if 0
                    if (vi.rgpdi[i-1] == NULL)
                        {
                        if (!MountMacDriveI(i-1))
                            goto Ltrydrive;
                        }
#endif

                    PokeW(vpregs->A0 + 0, 7);  // event = disk inserted
                    PokeL(vpregs->A0 + 2, i);  // disk number
                    PokeW(vpregs->A0 + 14, 0); // event flags
                    vpregs->D0 = 0; // noErr
                    vpregs->rgfsr.bitZ = 1;
                    vpregs->rgfsr.bitN = 0;
#if 0
                    printf("_GetOSEvent!\n");
#endif
// printf("_GetOSEvent 8\n");
                    vpregs->PC += 2;
                    return 0;
                    }
                }
            return 1;

#if 0
        // HACK! HACK! HACK! to make PRAM work

        case 0xA051:
            DebugStr("_ReadXPRAM, offset = %02X, length = %02X\n",
                vpregs->D0 & 65535, vpregs->D0 >> 16);

            {
            int i;
            int from = vpregs->D0 & 65535, len = vpregs->D0 >> 16;

            if ((from <= 0x8A) && (from + len > 0x8A))
            for (i = 0; i < (vpregs->D0 >> 16); i++)
                PokeB(vpregs->A0 + i, vmachw.rgbCMOS[(vpregs->D0 & 65535) + i]);

            if (from == 12 && len == 4)
                PokeL(0x1E4, 0x4E754D63);
            }

            // m68k_DumpRegs();
            DebugStr("Current MODE32 setting is: %02X\n", vmachw.rgbCMOS[0x8A]);
            vpregs->D0 = 0;
            vpregs->PC += 2;
            return 0;
            break;

        case 0xA052:
            DebugStr("_WriteXPRAM, offset = %02X, length = %02X\n",
                vpregs->D0 & 65535, vpregs->D0 >> 16);

            {
            int i;
            int from = vpregs->D0 & 65535, len = vpregs->D0 >> 16;

            if ((from <= 0x8A) && (from + len > 0x8A))
            for (i = 0; i < (vpregs->D0 >> 16); i++)
                vmachw.rgbCMOS[(vpregs->D0 & 65535) + i] = PeekB(vpregs->A0 + i);
            }

            // m68k_DumpRegs();
            DebugStr("Current MODE32 setting is: %02X\n", vmachw.rgbCMOS[0x8A]);
            vpregs->D0 = 0;
            vpregs->PC += 2;
            return 0;
            break;
#endif

#if 1
        case 0xA0AD:
#if 0
            printf("_Gestalt %08X %c%c%c%c  ", vpregs->D0, vpregs->D0 >> 24,
                (vpregs->D0 >> 16) & 255, (vpregs->D0 >> 8) & 255, vpregs->D0 & 255);
#endif

            // right now we only need to hack Gestalt when faking a 68040

            if (!vi.fFake040)
                break;

         //   lBreakpoint = vpregs->PC+2;
         //   m68k_DumpRegs();
// printf("setting breakpoint at %08X\n", lBreakpoint);

            switch(vpregs->D0)
                {
            case 'proc':
                // HACK!! HACK!! HACK!! return 68040 Gestalt value

//                if ((vmCur.bfCPU == cpu68040) || vi.fFake040)
                    {
                    vpregs->A0 = 5;
                    vpregs->D0 = 0;
                    vpregs->PC += 2;
                    return 0;
                    }
                break;

            case 'cput':
                // HACK!! HACK!! HACK!! return 68040 Gestalt value

//                if ((vmCur.bfCPU == cpu68040) || vi.fFake040)
                    {
                    vpregs->A0 = 4;
                    vpregs->D0 = 0;
                    vpregs->PC += 2;
                    return 0;
                    }
                break;

#if 0
            case 'mach':
                vpregs->A0 = 11;
                vpregs->D0 = 0;
                vpregs->PC += 2;
                return 0;
                break;
#endif

#if 0
            case 'hdwr':
                vpregs->A0 = 0x481D;
                vpregs->D0 = 0;
                vpregs->PC += 2;
                return 0;
                break;
#endif

#if 0
            case 'ram ':        // physical RAM size
                vpregs->A0 = vi.cbRAM[0];
                vpregs->D0 = 0;
                vpregs->PC += 2;
                return 0;
                break;
#endif

#if 0
            case 'fpu ':
                vpregs->A0 = 0;
                vpregs->D0 = 0;
                vpregs->PC += 2;
                return 0;
                break;
#endif

#if 0
            case 'romv':
                vpregs->A0 = 0x077C;
                vpregs->D0 = 0;
                vpregs->PC += 2;
                return 0;
                break;
#endif
                }

            break;
#endif
            }
        }

    return 1;
}

void __cdecl FastLineA()
{
    // Try to optimize the Line A handling by emulating the function of the
    // Line A trap handler

    {
    ULONG l = PeekL(0x28) + vpregs->VBR;
    int i = 20;

#if 0
    FDumpRegsVM();

    printf("call site...\n");
    CDis(vpregs->PC, TRUE);

    printf("Line A handler...\n");

    while (i--)
        l += 2*CDis(l, TRUE);
#endif

    if (l == 0x408099B0)
        {
        }
    }
}

#endif // SOFTMAC

