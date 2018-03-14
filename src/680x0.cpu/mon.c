
/***************************************************************************

    MON.C

    - 68000 debugger/monitor (based on original Xformer monitor)

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      last update

***************************************************************************/


#include "gemtypes.h"

#if defined(ATARIST) || defined(SOFTMAC)

ULONG lBreakpoint;

BOOL fStepMode;

ULONG lTimeStamp;


// Set the opcodes array to either point to our hook code or to the normal value
// based on the setting of the v.fDebugMode and fStepMode flags

void SetDebugMode(BOOL fStep)
{
    fStepMode = v.fDebugMode || fStep || vi.fInDebugger;
}

ULONG CDis(ULONG PC, BOOL fPrint)
{
    int  cb;
    BYTE rgb[32];
    BYTE sz[80];
    int  t;

    for (t = 0; t < sizeof(rgb); t++)
        rgb[t] = (BYTE)vmPeekB(PC + t);

    sz[0] = 0;

#ifdef POWERMAC
    if (FIsCPUAnyPPC(vmCur.bfCPU))
        {
        char rgch[80];

        DisasmPPC(PC, vmPeekL(PC), rgch, sizeof(rgch), NULL);
        printf("%08X: ", PC);
        printf("  %-40s | %08X\n", rgch, vmPeekL(PC));

        return sizeof(ULONG)/sizeof(WORD);
        }
#endif

    Assert (FIsCPUAny68K(vmCur.bfCPU));

    cb = Disasm68K(PC, rgb, sz, sizeof(sz), NULL);

    if (fPrint)
        {
        printf("%08X: ", PC);

        printf("%-35s", (cb == 0) ? "???" : sz);

        for (t = 0; t < ((cb == 0) ? 2 : cb); t+=2)
            printf("%02X%02X ", rgb[t], rgb[t+1]);
        printf("\n");
        }

    if (cb == 0)
        cb += 2;

    return cb;
}

enum
{
DBG_DISASM,
DBG_READMEMORY,
DBG_WRITEMEMORY,
DBG_DUMPREGS,
DBG_READREG,
DBG_WRITEREG,
DBG_STEP,
DBG_GETBREAK,
DBG_SETBREAK,
};


// long (*pfnCmdProc)(int cmd, long lParam1, long lParam2, char *pch, int cch);

long CmdProc(int cmd, long lParam1, long lParam2, char *pch, int cch)
{
    long l1 = lParam1;
    long l2 = lParam2;

    switch(cmd)
        {
    case DBG_DISASM:
        vi.pszOutput = pch;

        while (lParam2--)
            {
            lParam1 += CDis(lParam1, TRUE);
            }

        vi.pszOutput = NULL;
        return lParam1;

    case 100:
    case DBG_READMEMORY:
        memset(pch, 0, lParam2);

        while (l2--)
            {
            unsigned int t = vmPeekB(l1);

            *pch = (char)t;

            if (t & 0x80000000)
                lParam2 = 0;

            l1++;
            pch++;
            }
        return lParam2;

    case DBG_WRITEMEMORY:
        while (l2--)
            {
            if ((l1 >= vi.eaROM[0]) && (l1 < (vi.eaROM[0] + vi.cbROM[0])))
                {
                *(BYTE *)MapAddressVM(l1) = *pch;
                }
            else
                vmPokeB(l1, *pch);

            l1++;
            pch++;
            }
        return lParam2;

    case DBG_READREG:
        if (lParam1 >= 19)
            {
            return vpregs->SR;
            }

        return *(((ULONG *)(&vpregs->D0)) + lParam1);

    case DBG_WRITEREG:
        if (lParam1 >= 19)
            {
            vpregs->SR = (WORD) lParam2;
            }

        *(((ULONG *)(&vpregs->D0)) + lParam1) = lParam2;

        return lParam2;

    case DBG_STEP:
        vpregs->PC = lParam1;

        if (pch)
            {
            vi.pszOutput = VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
            CDis(vpregs->PC, TRUE);
            strcpy(pch + 20*sizeof(ULONG), vi.pszOutput);
            }

        while (lParam2--)
            {
            FExecVM(TRUE, FALSE);
            }

        if (pch)
            {
            memcpy(pch, &vpregs->D0, 19*sizeof(ULONG));
      //    memcpy(pch + 19*sizeof(ULONG), &vpregs->foo, sizeof(ULONG));

            VirtualFree(vi.pszOutput, 4096, MEM_DECOMMIT);
            VirtualFree(vi.pszOutput, 0, MEM_RELEASE);
            vi.pszOutput = NULL;
            }

        return vpregs->PC;

    case DBG_GETBREAK:
        return lBreakpoint;

    case DBG_SETBREAK:
        lBreakpoint = lParam1;
        return lBreakpoint;
        }

    return 0;
}

typedef ULONG (__stdcall *PFNSL)(void *, void *);

PFNSL PfnGuiDebugger()
{
    HANDLE hLib = LoadLibrary ("68000UI.DLL");
    PFNSL pfn = (long *)GetProcAddress (hLib, "GuiDebugger");

    return pfn;
}


BOOL FGuiDebugger(PFNSL pfn)
{
    if (pfn)
        {
        (*pfn)(&CmdProc, vi.hWnd);    // basically make sure the DLL loads and is working

#if 0
        while ((*pfn)(&CmdProc, 1))
            ;
#endif

        return TRUE;
        }

    return FALSE;
}


void Mon()
{
    char sz[250];
    char *pch;
    ULONG lPCCur;
    PFNSL pfn = PfnGuiDebugger();

    // make sure a VM has initialized

    if (vpregs == NULL)
        {
        return;
        }

    lPCCur = vpregs->PC;

    if (pfn == NULL)
        {
        CreateDebuggerWindow();
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        }

    // disassemble at the current PC and display registers

    {
    int i = 10;
    ULONG lPC = lPCCur;

    while (i--)
        lPC += CDis(lPC, TRUE);

    FDumpRegsVM();
    }

    SetDebugMode(1);

    if (FGuiDebugger(pfn))
        {
        SetDebugMode(0);
        return;
        }

    vi.fInDebugger = TRUE;
    vi.fDebugBreak = FALSE;
    vi.fExecuting = FALSE;

    printf("\nGemulator Debugger. Type '?' for help.\n\n");

    if (lBreakpoint != 0)
        {
        if (lBreakpoint == vpregs->PC)
            {
            printf("At breakpoint at %08X\n", vpregs->PC);
            }
        }

    while (1)
        {
        int cch;
        int i;
        MSG msg;

        while (PeekMessage(&msg, // message structure
           NULL,   // handle of window receiving the message
           0,      // lowest message to examine
           0,      // highest message to examine
           PM_REMOVE)) // remove the message, as if a GetMessage
            {
            if (msg.message == WM_PAINT)
                DispatchMessage(&msg); // Dispatches message to window
            }

        if (lTimeStamp != 0)
            {
            printf("exeution time: %d ms\n", GetTickCount() - lTimeStamp);
            lTimeStamp = 0;
            }

        printf("GEMULATOR: enter command: ");

        // zero out sz[]
        memset(sz, 0, sizeof(sz));

        ReadConsole(GetStdHandle(STD_INPUT_HANDLE), sz, sizeof(sz), &cch, NULL);
        printf("\n");

        for (i = 0; sz[i]; i++)
            {
            char ch = sz[i];

            if ((ch >= 'a') && (ch <= 'z'))
                sz[i] = ch - 32;
            else if (ch == 13)
                sz[i] = 0;
            else if (ch == 10)
                sz[i] = 0;
            else if (ch == 8)
                {
                if (i == 0)
                    strcpy(&sz[i], &sz[i+1]);
                else
                    strcpy(&sz[i-1], &sz[i+1]);
                i-=2;
                }
            }

        pch = PchSkipSpace(sz);

        // check for blank line

        if (pch == NULL)
            continue;

        // check for verbose command names

        if (0 == _strnicmp("disasm", pch, 6))
            {
            pch += 6;
            goto label_disasm;
            }

        if (0 == _strnicmp("hw", pch, 6))
            {
            pch += 2;
            goto label_dump_hw;
            }

        if (0 == _strnicmp("memdump", pch, 7))
            {
            pch += 7;
            goto label_mem_dump;
            }

        if (0 == _strnicmp("regdump", pch, 7))
            {
            pch += 7;
            goto label_reg_dump;
            }

        if (0 == _strnicmp("mm.b", pch, 4))
            {
            pch += 4;
            goto label_mem_edit_byte;
            }

        if (0 == _strnicmp("mm.w", pch, 4))
            {
            pch += 4;
            goto label_mem_edit_word;
            }

        if (0 == _strnicmp("mm.l", pch, 4))
            {
            pch += 4;
            goto label_mem_edit_long;
            }

        if (0 == _strnicmp("mm", pch, 2))
            {
            pch += 2;
            goto label_mem_edit_byte;
            }

        if (0 == _strnicmp("step", pch, 4))
            {
            pch += 4;
            goto label_step;
            }

        if (0 == _strnicmp("trace", pch, 5))
            {
            pch += 5;
            goto label_trace;
            }

        if (0 == _strnicmp("exit", pch, 4))
            {
            pch += 4;
            goto label_quit;
            }

        if (0 == _strnicmp("quit", pch, 4))
            {
            pch += 4;
            goto label_quit;
            }

        if (0 == _strnicmp("reboot", pch, 6))
            {
            pch += 6;
            goto label_coldboot;
            }

        if (0 == _strnicmp("forcequit", pch, 9))
            {
            exit(0);
            }

        // check for abbreviations

        if (!pch)
            continue;

        switch(*pch++)
            {
        default:
Lerror:
            break;

        case '?':
            {
            printf("\nGemulator debugger commands:\n\n");
            printf("  . (or R)           - dump registers\n");
            printf("  , (or HW)          - dump hardware state\n");
            printf("  M addr [addr-end]  - dump memory from addr to addr-end\n");
            printf("  U addr [addr-end]  - unasassemble from addr to addr-end\n");
            printf("\n");
            printf("  L addr size file   - load file to address in memory\n");
            printf("  : addr b0 [b1 ...] - set bytes at address\n");
            printf("  RDn=val            - set data register Dn to value\n");
            printf("  RAn=val            - set address register An to value\n");
            printf("  Rf=val             - set condition flag bit to value\n");

            printf("\n");
            printf("  B addr             - set breakpoint at addr\n");
            printf("  BD                 - delete breakpoint\n");
            printf("  BL                 - list breakpoint\n");
            printf("\n");
            printf("  G [addr]           - go at current PC or addr\n");
            printf("  S [addr]           - single step at current PC or addr\n");
            printf("  T [addr]           - trace at current PC or addr\n");
            printf("  Q or QUIT          - quit\n");
            printf("  @                  - reboot virtual machine\n");
            printf("\n");
            printf("  D or DISASM        - alias for U (unassemble)\n");
            printf("  E[.B/.W/.L]        - alias for : (set bytes at address)\n");
            printf("  MEMDUMP            - alias for M (dump memory)\n");
            printf("  MM[.B/.W/.L]       - alias for : (set bytes at address)\n");
            printf("  R                  - alias for . (dump register)\n");
            printf("  REBOOT             - alias for @ (reboot)\n");
            printf("  STEP               - alias for S (step)\n");
            printf("  TRACE              - alias for T (trace)\n");
            printf("  X or EXIT          - alias for Q (quit)\n");
            printf("\n");

            }
            break;

        case '@':
label_coldboot:
            FUnInitVM();
            FInitVM();
            FDumpRegsVM();

            // read the rebooted PC in case followed by disassemble command

            lPCCur = vpregs->PC;
            break;

        case ':':
        case 'E':
            if (0 == _strnicmp(".w", pch, 2))
                {
                pch += 2;
                goto label_mem_edit_word;
                }

            if (0 == _strnicmp(".l", pch, 2))
                {
                pch += 2;
                goto label_mem_edit_long;
                }

label_mem_edit_byte:
            {
            ULONG PC, w;

            pch = PchGetWord(pch, &PC);

            if (pch == 0)
                break;

            if (vi.cbRAM[0] == 0)
                break;

            while ((pch = PchGetWord(pch, &w)) != 0)
                {
                vmPeekB(PC);
                vmPokeB(PC++, w);
                }
            }
            break;

label_mem_edit_word:
            {
            ULONG PC, w;

            pch = PchGetWord(pch, &PC);

            if (pch == 0)
                break;

            if (vi.cbRAM[0] == 0)
                break;

            while ((pch = PchGetWord(pch, &w)) != 0)
                {
                vmPeekW(PC);
                vmPokeW(PC, w);
                PC += 2;
                }
            }
            break;

label_mem_edit_long:
            {
            ULONG PC, w;

            pch = PchGetWord(pch, &PC);

            if (pch == 0)
                break;

            if (vi.cbRAM[0] == 0)
                break;

            while ((pch = PchGetWord(pch, &w)) != 0)
                {
                vmPeekL(PC);
                vmPokeL(PC, w);
                PC += 4;
                }
            }
            break;

        case 'M':
label_mem_dump:
            {
            ULONG mem, memMin, memMac = 0;
            static ULONG newmem;
            BOOL fFirst = TRUE;

            if (vi.cbRAM[0] == 0)
                break;

            memMin = newmem;

            if ((pch = PchGetWord(pch, &memMin)) != 0)
                PchGetWord(pch, &memMac);

            if (memMac <= memMin)
                memMac = (memMin + 256) & 0xFFFFFFF0;

            newmem = memMac;

            for (mem = (memMin & 0xFFFFFFF0);
                 mem <= ((memMac+15) & 0xFFFFFFF0);
                  mem++)
                {
                static char rgbASCII[17];

                if ((mem & 15) == 0)
                    {
                    if (fFirst)
                        fFirst = FALSE;
                    else
                        printf("  %s\n", rgbASCII);

                    if (mem >= memMac)
                        break;

                    printf("%08X:  ", mem);

                    memset(rgbASCII, ' ', sizeof(rgbASCII));
                    rgbASCII[sizeof(rgbASCII) - 1] = 0;
                    }
                else if ((mem & 15) == 8)
                    printf(" ");

                if ((mem < memMin) || (mem >= memMac))
                    {
                    printf("   ");
                    }
                else
                    {
                    unsigned int t = vmPeekB(mem);

                    if (t & 0xFFFFFF00)
                        printf("-- ", t);
                    else
                        printf("%02X ", t);

                    if ((t >= 32) && (t < 127))
                        rgbASCII[mem & 15] = t;
                    else
                        rgbASCII[mem & 15] = '.';
                    }
                }
            printf("\n");

            mem = newmem;
            }
            break;

        case '.':
label_reg_dump:
            FDumpRegsVM();
            break;

        case ',':
label_dump_hw:
            FDumpHWVM();
            break;

        case 'D':
        case 'U':
label_disasm:
            {
            int cLines = 10;
            ULONG lPCMac;

            if (vi.cbRAM[0] == 0)
                break;

            pch = PchGetWord(pch, &lPCCur);

            lPCMac = 0;
            pch = PchGetWord(pch, &lPCMac);

            // If user specified an end address, set the
            // count of lines to be huge

            if (lPCMac > lPCCur)
                cLines = lPCMac - lPCCur;
            else
                lPCMac = 0xFFFFFFFF;

            while ((cLines--) && (lPCCur <= lPCMac))
                lPCCur += CDis(lPCCur, TRUE);
            }
            break;

        case 'R':
            {
            ULONG *preg = &vpregs->D0;
            ULONG l=0;

            switch(*pch++)
                {
            default:
                goto label_reg_dump;

            case 'S':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitSuper = l;
                break;

            case 'T':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitTrace = l;
                break;

            case 'I':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitsI = l;
                break;

            case 'X':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitX = l;
                break;

            case 'N':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitN = l;
                break;

            case 'Z':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitZ = l;
                break;

            case 'V':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitV = l;
                break;

            case 'C':
                PchGetWord(++pch, &l); /* skip '=' or space */
                vpregs->rgfsr.bitC = l;
                break;

            case 'A':
                preg = &vpregs->A0;

                // fall through

            case 'D':
                if ((*pch >= '0') && (*pch <= '7'))
                    preg += (*pch++ - '0');
                else
                    goto Lerror;

                PchGetWord(++pch, preg); /* skip '=' or space */
                break;
                }
            }
            break;

        case 'S':
label_step:
            if (vi.cbRAM[0] == 0)
                break;

            PchGetWord(pch, &vpregs->PC);
            FExecVM(TRUE, FALSE);
            lPCCur = vpregs->PC;
            break;

        case 'T':
label_trace:
            if (vi.cbRAM[0] == 0)
                break;

            PchGetWord(pch, &vpregs->PC);
            FExecVM(TRUE, TRUE);
            lPCCur = vpregs->PC;
            break;

        case 'B':
            pch = PchSkipSpace(pch);

            if (!pch)
                break;

            switch(*pch)
                {
            default:
                PchGetWord(pch, &lBreakpoint);
                break;

            case 'L':
                break;

            case 'D':
                lBreakpoint = 0;
                break;
                }

            if (lBreakpoint == 0)
                printf("No breakpoints.\n");
            else
                printf("Breakpoint at %08X.\n", lBreakpoint);

            break;

        case 'H':
            {
            ULONG c = 0;

            PchGetWord(pch, &c);
            m68k_DumpHistory(c ? c : 160);
            }
            break;

        case 'P':
            pch = PchSkipSpace(pch);
    
            switch(*pch)
                {
            default:
                m68k_DumpProfile();
                break;

            case 'C':
                ClearProfile();
                printf("Profile log cleared\n");
                break;
                }

            break;

        case 'L':
            if (vi.cbRAM[0] == 0)
                break;

            {
            ULONG mem = 0, cb = 0;

            if ((pch = PchGetWord(pch, &mem)) == NULL)
                SaveState(FALSE);
            else
                {
                void *pv;

                pch = PchGetWord(pch, &cb);

                if (cb != 0)
                    {
                    cb = CbReadWholeFileToGuest(pch, cb, mem, ACCESS_INITHW);
                    }

                printf("Loaded %08X bytes at %08X\n", cb, mem);
                }
            }
            break;

        case 'W':
            if (vi.cbRAM[0] == 0)
                break;

            SaveState(TRUE);
            break;

        case 'G':
            if (vi.cbRAM[0] == 0)
                break;

            PchGetWord(pch, &vpregs->PC);

            lTimeStamp = GetTickCount();    // remember current time

            vi.fExecuting = TRUE;
            return;

        case 'Q':
        case 'X':
label_quit:
            SetDebugMode(0);
            vi.fInDebugger = FALSE;
            vi.fExecuting = TRUE;
            return;
            }
        }
}

#endif  // ATARIST
