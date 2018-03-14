
/***************************************************************************

    GEMDOS.C

    - Atari ST GEMDOS hooks
    
    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    09/09/2008  darekm      last update

***************************************************************************/

#include "gemtypes.h"

#ifdef ATARIST

#define TRACEGEMDOS 0

#define WW2L(w1,w2) (((w1)<<16) + (w2))

// these locals are the state of our GEMDOS hooks

HANDLE hFind; // handle for FindNextFile
BYTE   bFind; // attribute being searched for
int CurDrive;

typedef struct _hm
{
    ULONG lBP;      // process basepage
    WORD hST;       // Atari ST file handle
    WORD hDOS;      // MS-DOS file handle
}HM;

#define MAX_HM 512

HM rghm[MAX_HM];

typedef struct _mint_dir
{
    unsigned long lMagic;   // 0x12345678
    unsigned long iCur;     // current index into rgfd
    unsigned long iMac;     // count of valid rgfd
    unsigned long fCompat;  // compatibility mode flag
    WIN32_FIND_DATA rgfd[1000];
} MINT_DIR;


// given an MS-DOS file handle and a basepage,
// create a virtual GEMDOS handle

WORD HSTFromHDOS(WORD hDOS, ULONG lBP)
{
    WORD hST = 6;
    int i;

    DebugStr("HSTFromHDOS: hDOS = %04X, lBP = %08X\n", hDOS, lBP);

    // first find the first unused handle >= 6
Lnext:
    for (i = 0; i < MAX_HM; i++)
        {
        if ((rghm[i].lBP == lBP) && (rghm[i].hST == hST))
            {
            hST++;
            goto Lnext;
            }
        }

    DebugStr("HSTFromHDOS: hST = %04X, ", hST);

    // now add it to the table in first available spot

    for (i = 0; i < MAX_HM; i++)
        {
        if (rghm[i].hST == 0)
            {
            Assert(rghm[i].hDOS == 0);
            Assert(rghm[i].lBP == 0);

            rghm[i].hST = hST;
            rghm[i].hDOS = hDOS;
            rghm[i].lBP = lBP;
            break;
            }
        }

    Assert(i != MAX_HM);

    DebugStr("i = %d\n", i);

    return hST;
}


// given a virtual GEMDOS file handle and a basepage,
// return the actual MS-DOS file handle

WORD HDOSFromHST(WORD hST, ULONG lBP)
{
    int i;

    DebugStr("HDOSFromHST: hST = %04X, lBP = %08X\n", hST, lBP);

    for (i = 0; i < MAX_HM; i++)
        {
        if ((rghm[i].hST == hST) && (rghm[i].lBP == lBP))
            break;
        }

    DebugStr("i = %d, hDOS = %04X\n", i, rghm[i].hDOS);

    if (i == MAX_HM)
        return -100;

    return rghm[i].hDOS;
}


// given a virtual GEMDOS file handle and a basepage,
// close the actual MS-DOS file handle

int CloseHST(WORD hST, ULONG lBP)
{
    WORD hDOS;
    int i;

    DebugStr("CloseHST: hST = %04X, lBP = %08X\n", hST, lBP);

    for (i = 0; i < MAX_HM; i++)
        {
        if ((rghm[i].hST == hST) && (rghm[i].lBP == lBP))
            break;
        }

    DebugStr("i = %d, hDOS = %04X\n", i, rghm[i].hDOS);

    if (i == MAX_HM)
        return 0;

    hDOS = rghm[i].hDOS;
    memset(&rghm[i], 0, sizeof(HM));

    return CloseHandle((HANDLE)hDOS);
}


// given a basepage, close all associated files

void CloseFilesProcess(ULONG lBP)
{
    int i;

    DebugStr("CloseFilesProcess: lBP = %08X\n", lBP);

    for (i = 0; i < MAX_HM; i++)
        {
        if ((lBP == 0xFFFFFFFF) || (rghm[i].lBP == lBP))
            {
            if (rghm[i].hDOS == 0)
                continue;

            DebugStr("i = %d, hDOS = %04X\n", i, rghm[i].hDOS);
            CloseHandle((HANDLE)rghm[i].hDOS);

            memset(&rghm[i], 0, sizeof(HM));
            }
        }
}



// initialize for a coldboot

BOOL FResetGemDosHooks()
{
    char szReset[] = "A:\\";
    DWORD emOld;

    CloseFilesProcess(0xFFFFFFFF);

    if (vmCur.fUseVHD)
        return TRUE;

    hFind = NULL;

    szReset[0] = 'A' + vmCur.iBootDisk;
    emOld = SetErrorMode(SEM_FAILCRITICALERRORS);
    SetCurrentDirectory(szReset);
    SetErrorMode(emOld);
    CurDrive = vmCur.iBootDisk;
//    PokeL(0x4C2, GetLogicalDrives()); // _drvbits

    return TRUE;
}


// extract handle from DTA

HANDLE HFromDTA(unsigned long pDTA)
{

//    return hFind;

//    if (PeekL(pDTA+10) == 0x12345678)
//        return (HANDLE)PeekL(pDTA+14);

    return (HANDLE)PeekL(pDTA+4);

    return NULL;
}


// convert find_data structure to DTA

BOOL FDToDTA(WIN32_FIND_DATA *pFD, unsigned long pDTA, HANDLE h)
{
    unsigned char *pch;
    int i;
    WORD DOSdate, DOStime;
    FILETIME ft;

    // reserved

#if TRACEGEMDOS
    DebugStr("\npDTA = %08X\n", pDTA);
#endif

#if 1
    for (i = 0; i < 21; i++)
        vmPokeB(pDTA+i, 0);
#endif

#if 1
    vmPokeB(pDTA+0,  0x2A);
    vmPokeB(pDTA+1,  0x2E);
    vmPokeB(pDTA+2,  0x2A);
#endif
#if 0
    vmPokeB(pDTA+6,  0xFF);
    vmPokeB(pDTA+7,  0xFF);
    vmPokeB(pDTA+9,  0x04);
    vmPokeB(pDTA+11, 0x01);
    vmPokeB(pDTA+15,  0x05);
    vmPokeB(pDTA+17,  0xA0);
    vmPokeB(pDTA+18,  0x13);
    vmPokeB(pDTA+19,  0x44);
#endif
#if 1
    vmPokeB(pDTA+20,  0x31);
#endif

    // write out our magic cookie with the search handle

//    vmPokeL(pDTA+10, 0x12345678);
    vmPokeL(pDTA+4, (int)h);
// hFind = h;

    if (h == NULL)
        {
        return TRUE;
        }

    FileTimeToLocalFileTime(&pFD->ftLastWriteTime, &ft);
    FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);

    // attributes (DOS maps to GEMDOS exactly)

#if TRACEGEMDOS
    DebugStr("attributes: %02X, ", pFD->dwFileAttributes);
#endif
    vmPokeB(pDTA+21, pFD->dwFileAttributes);

    // time stamp

    vmPokeW(pDTA+22, DOStime);

    // date stamp

    vmPokeW(pDTA+24, DOSdate);
    
    // file size

#if TRACEGEMDOS
    DebugStr("size: %d, ", pFD->nFileSizeLow);
#endif
    vmPokeL(pDTA+26, pFD->nFileSizeLow);

    // filename

    if (pFD->cAlternateFileName[0])
        pch = (unsigned char *)&pFD->cAlternateFileName;
    else
        pch = (unsigned char *)&pFD->cFileName;

    DebugStr("filename: %s\n", pch);

    StrToStr68(pch, pDTA+30);

    return TRUE;
}


void DumpDTA(unsigned pDTA)
{
    int i;

    DebugStr("\nDTA = ");
    for (i = 0; i < 44; i++)
        {
        DebugStr("%02X ", PeekB(pDTA+i));
        }
    DebugStr("\n");
}

ULONG BPCur()
{
    ULONG osHDR = PeekL(0x04F2);

    // check for TOS 1.00

    if (0x0102 > PeekW(osHDR+2))
        {
        if ((PeekW(osHDR+28) >> 1) == 4) // spanish TOS 1.0
            return PeekL(0x873C);
        return PeekL(0x602C);
        }

    return PeekL(PeekL(osHDR+0x28));
}

int GetCurrentGEMDOSDirectory(int cchMac, char *pch)
{
    char szDrv[4];
    char rgch3[MAX_PATH];
    int  cch;

    szDrv[0] = 'A' + CurDrive;
    szDrv[1] = ':';
    szDrv[2] = '.';
    szDrv[3] = 0;

    cch = GetFullPathName(szDrv, cchMac, pch, rgch3);
    return cch;
}

BOOL CallGEMDOS(ULONG ea, ULONG w1, ULONG w2, ULONG w3, ULONG w4, ULONG w5, ULONG w6, ULONG w7, ULONG w8)
{
    char rgch[1024], rgch2[1024];
    BOOL fHandled = FALSE;
    WIN32_FIND_DATA finddata;
    HFILE h;
    OFSTRUCT ofstruct;
    BOOL fDontPrintCRLF = 0;
    unsigned long pDTA;
    int fInFsfirst = 0;
    DWORD emOld;

    static int savedSP = 0;
    static int mdPexec = 0;
    static int savedMode;
    static int savedFile;
    static HANDLE hPexec;

#if 0
    sprintf(rgch, "GEMDOS (%04X, %04X, %04X, %04X, %04X) PC:%08X SSP:%08X USP:%08X, ",
          w1, w2, w3, w4, w5, ea, vpregs->SSP, vpregs->USP);
    SetWindowText(vi.hWnd, rgch);
#endif

    // get the DTA from the current base page

    pDTA = PeekL(BPCur()+32);

#ifndef NDEBUG
#if TRACEGEMDOS
    {
    int i;
    ULONG PC = ea-2;
    extern ULONG CDis(ULONG, BOOL);

    for (i = 0; i < 10; i++)
        {
        if (PeekW(PC) == 0x2F39)
            PC = PeekL(PeekL(PC+2));
        PC += 2*CDis(PC, TRUE);
        }
    }

    GetCurrentDirectory(sizeof(rgch), rgch);
    DebugStr("current MS-DOS directory = %s\n", rgch);

    DebugStr("BP=%08X, GEMDOS (%04X, %04X, %04X, %04X, %04X) PC:%08X SSP:%08X USP:%08X, ",
          BPCur(), w1, w2, w3, w4, w5, ea, vpregs->SSP, vpregs->USP);


    // first just print out the GEMDOS function and parameters
        
    switch(w1)
        {
    default:
        fDontPrintCRLF = TRUE;
        break;

    case 0:
        DebugStr("Pterm0()");
        break;

    case 1:        
        DebugStr("Cconin()");
        break;

    case 2:        
        DebugStr("Cconout(%d)", w2);
        break;

    case 3:        
        DebugStr("Cauxin()");
        break;

    case 4:        
        DebugStr("Cauxout(%d)", w2);
        break;

    case 5:
        DebugStr("Cprnout(%d)", w2);
        break;

    case 6:
        DebugStr("Crawio(%d)", w2);
        break;

    /// ...

    case 9:        
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Cconws(\"%s\")", rgch);
        break;

    /// ...

    case 14:
        DebugStr("Dsetdrv(%d)", w2);
        break;

    /// ...

    case 25:
        DebugStr("Dgetdrv()");
        break;

    case 26:
        DebugStr("Fsetdta($%08X)", WW2L(w2,w3));
        pDTA = WW2L(w2,w3);
        DumpDTA(pDTA);
        break;

    case 32:
        DebugStr("Super($%08X)", WW2L(w2,w3));
        break;

    /// ...

    case 47:
        DebugStr("Fgetdta()");
        break;

    case 48:
        DebugStr("Sversion()");
        break;

    case 49:
        DebugStr("Ptermres(%d, %d)", WW2L(w2,w3), w4);
        break;

    case 51: // Magic Sconfig
        DebugStr("MAGIC Sconfig ($%08X, %08X)", WW2L(w2,w3), WW2L(w4,w5));
        break;

    case 54:
        DebugStr("Dfree($%08X, %d)", WW2L(w2,w3), w4);
        break;

    case 57:        
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Dcreate(\"%s\")", rgch);
        break;

    case 58:        
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Ddelete(\"%s\")", rgch);
        break;

    case 59:        
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Dsetpath(\"%s\")", rgch);
        break;

    case 60:
    case 61:
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("%s(\"%s\", %d)", (w1 == 60) ? "Fcreate" : "Fopen", rgch, w4);
        break;

    case 62:        
        DebugStr("Fclose(%04X)", w2);
        break;

      case 63:
        DebugStr("Fread($%04X, %d, %08X)", w2, WW2L(w3,w4), WW2L(w5,w6));
        break;

      case 64:
        DebugStr("Fwrite($%04X, %d, %08X)", w2, WW2L(w3,w4), WW2L(w5,w6));
        break;

    case 65:        
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Fdelete(\"%s\")", rgch);
        break;

      case 66:
        DebugStr("Fseek(%d, %04X, %d)", WW2L(w2,w3), w4, w5);
        break;

    case 67:        
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Fattrib(\"%s\", %d, %d)", rgch, w4, w5);
        break;

    case 69:        
        DebugStr("Fdup($%04X)", w2);
        break;

    case 70:        
        DebugStr("Fforce($%04X, %04X)", w2, w3);
        break;

    case 71:        
        DebugStr("Dgetpath($%08X, %d)", WW2L(w2,w3), w4);
        break;

    case 72:        
        DebugStr("Malloc(%d)", WW2L(w2,w3));
        break;

    case 73:        
        DebugStr("Mfree($%08X)", WW2L(w2,w3));
        break;

    case 74:        
        DebugStr("Mshrink($%08X, %d)", WW2L(w3,w4), WW2L(w5,w6));
        break;

    case 75:        
        Str68ToStr(WW2L(w3,w4), rgch);
        DebugStr("Pexec(%d, \"%s\", %08X)", w2, rgch, WW2L(w5,w6));
        DebugStr(" SP = %08X, USP = %08X\n", vpregs->A7, vpregs->USP);
        break;

    case 76:        
        DebugStr("Pterm(%d)", w2);
        break;

    case 78:
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Fsfirst(\"%s\", %d)", rgch, w4);
        DumpDTA(pDTA);
        break;

    case 79:
        DebugStr("Fsnext()");
        DumpDTA(pDTA);
        break;

     case 86:        
        Str68ToStr(WW2L(w3,w4), rgch);
        Str68ToStr(WW2L(w5,w6), rgch2);
        DebugStr("Frename(%d, \"%s\",\"%s\")", w2, rgch, rgch2);
        break;

    case 87:
        DebugStr("Fdatime($%08X, %04X, %d)", WW2L(w2,w3), w4, w5);
        break;

    case 292: // DPathConf
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Dpathconf(\"%s\", %d)", rgch, w4);
        break;

    case 296: // Dopendir
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Dopendir(\"%s\", %d)", rgch, w4);
        break;

    case 297: // Dreaddir
        DebugStr("Dreaddir(%d, $%08X)", w2, WW2L(w3, w4));
        break;

    case 298: // Drewinddir
        DebugStr("Drewinddir($%08X)", WW2L(w2, w3));
        break;

    case 299: // Closedir
        DebugStr("Dclosedir($%08X)", WW2L(w2, w3));
        break;

    case 300: // Fxattr
        Str68ToStr(WW2L(w3,w4), rgch);
        DebugStr("Fxattr ($%04X, %s, $%08X)", w2, rgch, WW2L(w5, w6));
        break;

    case 322: // Magic 4 function $142 - directory read
        DebugStr("MAGIC 4 $142 ($%04X, $%08X, $%08X, $%08X)", w2, WW2L(w3,w4), WW2L(w5,w6), WW2L(w7,w8));
        break;

    case 338: // Magic 4 DreadLabel
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("DreadLabel (%s, $%08X, $%04X)", rgch, WW2L(w4,w5), w6);
        break;

        }

    if (!fDontPrintCRLF)
        {
        DebugStr("\n");
        }

#endif // TRACEGEMDOS
#endif // NDEBUG

    if (vmCur.fUseVHD)
        {
        DebugStr("\n");
        return TRUE;
        }

    // now process the GEMDOS commands we are interested in

//    if (w1 < 12)
        PokeL(0x4C2, GetLogicalDrives()); // _drvbits

    emOld = SetErrorMode(SEM_FAILCRITICALERRORS);

    switch(w1)
        {
    default:
        break;

    case 0:                    // Pterm0
        CloseFilesProcess(BPCur());
        break;

    case 14:                // Dsetdrv
        fHandled = TRUE;
        CurDrive = w2;
        vpregs->D0 = PeekL(0x4C2);

        // set the global MS-DOS dir to the dir on the current drive

        GetCurrentGEMDOSDirectory(sizeof(rgch), rgch);
        SetCurrentDirectory(rgch);
        break;

    case 25:                // Dgetdrv
        fHandled = TRUE;
        vpregs->D0 = CurDrive;
        break;

    case 26:                // Fsetdta
        fHandled = TRUE;
        vmPokeL(BPCur()+32, WW2L(w2,w3));
        break;

    case 47:                // Fgetdta
        fHandled = TRUE;
        vpregs->D0 = PeekL(BPCur()+32);
        break;

    case 49:                // Ptermres
        CloseFilesProcess(BPCur());
        break;

    case 54:                // Dfree
        fHandled = TRUE;

        {
        DWORD lFreeClusters, lTotalClusters, lSectorSize, lClusterSize;

        if (w4 == 0)
            {
            GetCurrentDirectory(sizeof(rgch), rgch);
            }
        else
            rgch[0] = 'A'+ (BYTE)w4 - 1;
        rgch[1] = ':';
        rgch[2] = '\\';
        rgch[3] = 0;

        if (GetDiskFreeSpace(rgch, &lClusterSize, &lSectorSize,    &lFreeClusters, &lTotalClusters))
            {
            vmPokeL(WW2L(w2,w3)+0,  lFreeClusters);
            vmPokeL(WW2L(w2,w3)+4,  lTotalClusters);
            vmPokeL(WW2L(w2,w3)+8,  lSectorSize);
            vmPokeL(WW2L(w2,w3)+12, lClusterSize);
            vpregs->D0 = 0;
            }
        else
            vpregs->D0 = -1;
        }
        break;

    case 57:                // Dcreate
        fHandled = TRUE;
        Str68ToStr(WW2L(w2,w3), rgch);
        vpregs->D0 = CreateDirectory(rgch,NULL) ? 0 : -36;
        break;

    case 58:                // Ddelete
        GetCurrentDirectory(sizeof(rgch2), rgch2);
        DebugStr("current directory before Ddelete = %s\n", rgch2);

        fHandled = TRUE;
        Str68ToStr(WW2L(w2,w3), rgch);
        if (!strcmp(rgch, rgch2))
            {
            // can't delete current diretory! move up
            SetCurrentDirectory("..");
            }
        vpregs->D0 = RemoveDirectory(rgch) ? 0 : -36;
        break;

    case 59:                // Dsetpath        
        fHandled = TRUE;

        GetCurrentDirectory(sizeof(rgch2), rgch2);
        DebugStr("current directory before = '%s'\n", rgch2);

        Str68ToStr(WW2L(w2,w3), rgch);
        if (rgch[0] != '.')
            {
            rgch[0] = CurDrive + 'A';
            rgch[1] = ':';
            Str68ToStr(WW2L(w2,w3), rgch+2);
            if (rgch[3] == ':')
                Str68ToStr(WW2L(w2,w3), rgch);
            }
        else
            {
            DebugStr("");
            }

        DebugStr("Trying to set directory to '%s'\n", rgch);

        if (!strcmp(rgch, rgch2))
            vpregs->D0 = 0;            // already in the directory
        else
            vpregs->D0 = SetCurrentDirectory(rgch) ? 0 : -34;
        DebugStr("Dsetpath returned %d\n", vpregs->D0);
        GetCurrentDirectory(sizeof(rgch2), rgch2);
        DebugStr("current directory after = '%s'\n", rgch2);
        break;

    case 60:                // Fcreate
        Str68ToStr(WW2L(w2,w3), rgch);

        // Special device names should be handled by GEMDOS

        if (stricmp(rgch, "CON:") == 0)
            break;

        if (stricmp(rgch, "PRN:") == 0)
            break;

        if (stricmp(rgch, "AUX:") == 0)
            break;

        fHandled = TRUE;

        // Windows doesn't create the directory if one doesn't exist
        // whereas GEMDOS does. So we need to fake this behaviour.

        {
        char *pch = rgch;

        if (pch = strstr(pch, "\\"))
            {
            strcpy(rgch2, rgch);  // save original string away

            pch = rgch;

            while (pch = strstr(pch+1, "\\"))
                {
                *pch = 0;
                CreateDirectory(rgch,NULL);
                strcpy(rgch, rgch2);
                }
            }
        }

        h = CreateFile(rgch, GENERIC_READ | GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, w4 & 7, (HANDLE)NULL);
        DebugStr("CreateFile handle = %d\n", h);

        if (h == HFILE_ERROR)
            {
            DebugStr("GEMDOS: CreateFile failed: %s\n", rgch);
            vpregs->D0 = -36;
            }
        else
            {
            vpregs->D0 = HSTFromHDOS(h, BPCur());
            }
        break;

    case 61:                // Fopen
        fHandled = TRUE;

        mdPexec = 0;    // makes Timeworks Publisher work!

        Str68ToStr(WW2L(w2,w3), rgch);
        {
        int mode;

        w4 &= 3; // mask higher bits used by later TOSes
                
        if (w4 == 0)
            mode = OF_READ;
        else if (w4 == 1)
            mode = OF_WRITE;
        else if (w4 == 2)
            mode = OF_READWRITE;
        else
            {
Lopenerror:
            DebugStr("GEMDOS: OpenFile failed: %s\n", rgch);
            vpregs->D0 = -33; // file not found
            break;
            }

        Str68ToStr(WW2L(w2,w3), rgch);

        if (strstr(rgch, "\?") || strstr(rgch, "*"))
            {
            // do wildcard expansion on filename

            HANDLE hFind;
            WIN32_FIND_DATA fd;

            if ((hFind = FindFirstFile(rgch, &fd)) != INVALID_HANDLE_VALUE)
                {
                do
                    {
                    if ((fd.dwFileAttributes & 0x1E) == 0)
                        break;

                    } while (FindNextFile(hFind, &fd));

                FindClose(hFind);

                strcpy(rgch, fd.cFileName);
                }
            }

        h = OpenFile(rgch, &ofstruct, mode);
        DebugStr("OpenFile handle = %d\n", h);

        if (h == HFILE_ERROR)
            goto Lopenerror;
        
        vpregs->D0 = HSTFromHDOS(h, BPCur());
        }
        break;

    case 62:                // Fclose
        if ((short)w2 <= 5)
            break;

        fHandled = TRUE;
        vpregs->D0 = (CloseHST((HANDLE)w2, BPCur()) ? 0 : -37);
        break;

      case 63:                // Fread
        // check for one of the default file handles

        if ((short)w2 <= 5)
            break;

        fHandled = TRUE;
        {
        unsigned long cb  = WW2L(w3,w4);
        unsigned long buf = WW2L(w5,w6);
        unsigned long cbRead = 0;
        void *pv = NULL;

        if (buf + cb >= vi.cbRAM[0])
            {
            // trying to read way too much, so figure out how long the
            // file is and read just that

            cb = GetFileSize((HANDLE)HDOSFromHST((HANDLE)w2, BPCur()), NULL);
            }

        if (buf + cb >= vi.cbRAM[0])
            {
            // still reading way too much, clip it

            cb = vi.cbRAM[0] - buf;
            }

        if (cb < vi.cbRAM[0])
            {
            pv = malloc(cb);
            }

        if (pv && ((cb == 0) || 
            ReadFile((HANDLE)HDOSFromHST((HANDLE)w2, BPCur()),
            pv, cb, &cbRead, NULL)) &&
            HostToGuestDataCopy(buf, pv, ACCESS_WRITE, cb))
            {
            vpregs->D0 = cbRead;
            }
        else
            {
            int l = GetLastError();

            vpregs->D0 = -36;
            }

        if (pv)
            free(pv);
        }
        break;

      case 64:                // Fwrite
        // check for one of the default file handles

        if ((short)w2 <= 5)
            break;

        fHandled = TRUE;

        {
        unsigned long cb  = WW2L(w3,w4);
        unsigned long buf = WW2L(w5,w6);
        unsigned long cbRead;
        void *pv = NULL;

        if (cb < vi.cbRAM[0])
            {
            pv = malloc(cb);
            }

        if (pv &&
             GuestToHostDataCopy(buf, pv, ACCESS_READ, cb) &&
             WriteFile((HANDLE)HDOSFromHST((HANDLE)w2, BPCur()),
              pv, cb, &cbRead, NULL))
            {
            vpregs->D0 = cbRead;
            }
        else
            {
            vpregs->D0 = -36;
            }

        free(pv);
        }
        break;

    case 65:                // Fdelete
        fHandled = TRUE;

        Str68ToStr(WW2L(w2,w3), rgch);
        vpregs->D0 = (DeleteFile(rgch) ? 0 : -34);
        break;

      case 66:                // Fseek
        if ((short)w4 <= 5)
            break;

        fHandled = TRUE;
        vpregs->D0 = SetFilePointer((HANDLE)HDOSFromHST((HANDLE)w4, BPCur()),
              WW2L(w2,w3), NULL, w5);
        if (vpregs->D0 == 0xFFFFFFFF)
            vpregs->D0 = -64;
        break;

    case 67:                // Fattrib
        fHandled = TRUE;

        {
        DWORD attr;

        Str68ToStr(WW2L(w2,w3), rgch);
        if ((attr = GetFileAttributes(rgch)) != 0xFFFFFFFF)
            {
            vpregs->D0 = attr & 63;

            w4 &= 63;
            w5 &= 63;

            if (w4 != 0)
                {
                attr = (attr & ~w4) | (w5 & w4);
                SetFileAttributes(rgch, attr);
                }
            }
        else
            vpregs->D0 = -36;
        }
        break;

    case 69:                // Fdup
        if ((short)w2 <= 5)
            break;

        fHandled = TRUE;
        vpregs->D0 = -37;
        Assert(FALSE);
        break;

    case 70:                // Fforce
        if ((short)w2 <= 5)
            break;

        fHandled = TRUE;
        vpregs->D0 = -37;
        Assert(FALSE);
        break;

    case 71:                // Dgetpath
        fHandled = TRUE;

        {
        int cch = GetCurrentGEMDOSDirectory(sizeof(rgch), rgch);

        if ((cch == 3) && rgch[2] == '\\')
            {
            rgch[2] = 0;
            }
        }

        if ((w4 == 0) || (w4 == (CurDrive+1)) )
            StrToStr68(rgch+2,WW2L(w2,w3));
        else
            StrToStr68("",WW2L(w2,w3));
        vpregs->D0 = 0;
        break;

    case 75:                // Pexec
        // only trap the "load" cases of Pexec

//        if (w2 != 0)
        if ((w2 != 0) && (w2 != 3))
            break;

        fHandled = TRUE;

        Str68ToStr(WW2L(w3,w4), rgch);
        savedSP = vpregs->A7;
        savedMode = w2;
        savedFile = WW2L(w3,w4);

        if (w2 == 0)
            {
            // load and go mode

            if ((stricmp(rgch+strlen(rgch)-4,".EXE") == 0) &&
                (stricmp(rgch+strlen(rgch)-11,"NEODESK.EXE") != 0) )
                {
                // Pexecing an .EXE file

                STARTUPINFO si =
                    {
                    sizeof(STARTUPINFO),
                    NULL,
                    NULL,
                    NULL,     // title
                    0, 0,      // dx, dy
                    0, 0,     // dwx, dwy
                    0, 0,     // dwX, dwY
                    0,        // flags
                    0,
                    0, 
                    0,
                    NULL,
                    NULL,
                    NULL,
                    };

                PROCESS_INFORMATION pi;

                if (CreateProcess(
                    rgch,     // module name
                    NULL,     // command line
                    NULL,     // process security descriptor
                    NULL,     // thread security descriptor
                    FALSE,    // inheritance
                    CREATE_DEFAULT_ERROR_MODE |  // flags
                    NORMAL_PRIORITY_CLASS,
                    NULL,     // environment
                    NULL,     // current directory
                    &si,
                    &pi) ||

                    // if CreateProcess didn't work (maybe it wasa DOS app)
                    // try the older WinExec

                    (WinExec(rgch, SW_SHOWDEFAULT) > 31) )
                    {
                    // spawn worked, not much to do

                    }
                vpregs->D0 = 0;
                break;
                }
            }

        hPexec = (HANDLE)OpenFile(rgch, &ofstruct, OF_READ);
        DebugStr("opened handle = %d\n", hPexec);

        if (hPexec == (HANDLE)HFILE_ERROR)
            {
            // tried to Pexec a file we couldn't open

            vpregs->D0 = -33;
            break;
            }

        mdPexec = 1;
        break;

    case 76:                // Pterm
        CloseFilesProcess(BPCur());
        break;

    case 78:                // Fsfirst
        fHandled = TRUE;
        fInFsfirst = TRUE;

        Str68ToStr(WW2L(w2,w3), rgch);
        bFind = (BYTE)w4;

        hFind = HFromDTA(pDTA);
        if (hFind)
            {
            FindClose(hFind);
            hFind = NULL;
            FDToDTA(&finddata, pDTA, hFind);
            }

        if (bFind == 8)
            {
            char szName[12], sz[12];
            int i;

            if (GetVolumeInformation(rgch, szName, 12, NULL, &i, &i, sz, 12))
                {
                vmPokeB(pDTA+0,  0x2A);
                vmPokeB(pDTA+1,  0x2E);
                vmPokeB(pDTA+2,  0x2A);
                vmPokeB(pDTA+20, 0x31);

                vmPokeB(pDTA+21, 0x08);

                // time stamp

                vmPokeW(pDTA+22, 0);

                // date stamp

                vmPokeW(pDTA+24, 0);
    
                // file size

                vmPokeL(pDTA+26, 0);

                StrToStr68(szName, pDTA+30);

                vpregs->D0 = 0;
                hFind = 0;
                break;
                }
            }

        if ((hFind = FindFirstFile(rgch, &finddata)) == INVALID_HANDLE_VALUE)
            {
            vpregs->D0 = -33; // file not found
            break;
            }

        goto Lfoundnext;

    case 79:                // Fsnext
        fHandled = TRUE;

        hFind = HFromDTA(pDTA);

Lfindnext:
        if (hFind && FindNextFile(hFind, &finddata))
            {
Lfoundnext:
            FDToDTA(&finddata, pDTA, hFind);

            if (bFind == 0)
                {
                // don't find directories, hidden files, or volume labels

                if (finddata.dwFileAttributes & 0x1E)
                    goto Lfindnext;
                }

            else if ((bFind & 0x08) == 0)
                {
                // don't find volume labels

                if (finddata.dwFileAttributes & 0x08)
                    goto Lfindnext;
                }

            else if ((finddata.dwFileAttributes & bFind) == 0)
                goto Lfindnext;

            DebugStr("returning a found file\n");
            FDToDTA(&finddata, pDTA, hFind);
            vpregs->D0 = 0;
            }
        else
            {
            DebugStr("No more files!\n");
            if (hFind)
                FindClose(hFind);
            hFind = NULL;
            FDToDTA(&finddata, pDTA, hFind);
            vpregs->D0 = fInFsfirst ? -33 : -49; // file not found : no more files
            }
        break;

     case 86:                // Frename
        fHandled = TRUE;

        Str68ToStr(WW2L(w3,w4), rgch);
        GetCurrentDirectory(sizeof(rgch2), rgch2);

        if (!strcmp(rgch, rgch2))
            {
            // can't rename current diretory! move up
            SetCurrentDirectory("..");
            }

        Str68ToStr(WW2L(w5,w6), rgch2);

        if (MoveFile(rgch, rgch2))
            vpregs->D0 = 0;
        else
            vpregs->D0 = -33;
        break;

    case 87:                // Fdatime
        fHandled = TRUE;
        {
        WORD DOSdate, DOStime;
        FILETIME ftDummy, ftUTC, ft;

        vpregs->D0 = 0;

        if (w5)
            {
            // change file time

            DOSdate = PeekW(WW2L(w2,w3));
            DOStime = PeekW(WW2L(w2,w3)+2);
            DosDateTimeToFileTime(DOSdate, DOStime, &ft);
            LocalFileTimeToFileTime(&ft, &ftUTC);
            if (!SetFileTime((HANDLE)HDOSFromHST((HANDLE)w4, BPCur()), NULL, NULL, &ftUTC))
                vpregs->D0 = -37;
            }
        else
            {
            // read file time

            if (GetFileTime((HANDLE)HDOSFromHST((HANDLE)w4, BPCur()), &ftDummy, &ftDummy, &ftUTC))
                {
                FileTimeToLocalFileTime(&ftUTC, &ft);
                FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
                vmPokeW(WW2L(w2,w3), DOSdate);
                vmPokeW(WW2L(w2,w3)+2, DOStime);
                }
            else
                vpregs->D0 = -37;
            }

        break;
        }

    case 292: // DPathConf
        fHandled = TRUE;

        switch(w4)
            {
        default:
            fHandled = FALSE;
            break;

        case 3:    // maximum size of filename
            vpregs->D0 = 12;
            break;

        case 5: // file name truncation type
            vpregs->D0 = 2;
            break;

        case 6: // case sensitivity type
            vpregs->D0 = 2;
            break;
            }

        break;

    case 296: // Dopendir
        Str68ToStr(WW2L(w2,w3), rgch);
        DebugStr("Dopendir(\"%s\", %d)", rgch, w4);
        fHandled = TRUE;
        {
        MINT_DIR *pmd = (void *)vpregs->D0 = (long)LocalAlloc(0, sizeof(MINT_DIR));
        int i;
        HANDLE hFind; // handle for FindNextFile

        // Append \*.* or *.* to the path name
        if (rgch[0] && (rgch[strlen(rgch)-1] != '\\'))
            strcat(rgch,"\\");
        strcat(rgch,"*.*");

        for (i = 0, hFind = INVALID_HANDLE_VALUE; i < 1000; i++)
            {
            if (i == 0)
                {
                if ((hFind = FindFirstFile(rgch, &finddata)) == INVALID_HANDLE_VALUE)
                    break;
                }
            else if (!FindNextFile(hFind, &finddata))
                {
                break;
                }

            pmd->rgfd[i] = finddata;
            }

        pmd->iCur = 0;
        pmd->iMac = i;
        pmd->lMagic = 0x12345678;
        pmd->fCompat = w4;
        }
        break;

    case 297: // Dreaddir
        fHandled = TRUE;
        vpregs->D0 = 0;
        break;

    case 298: // Drewinddir
        fHandled = TRUE;
        vpregs->D0 = 0;
        break;

    case 299: // Dclosedir
        fHandled = TRUE;
        __try
            {
            MINT_DIR *pmd = (void *)WW2L(w2,w3);

            if (pmd->lMagic == 0x12345678)
                LocalFree(pmd);
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        vpregs->D0 = 0;
        break;

    case 300: // Fxattr
        fHandled = TRUE;
        vpregs->D0 = -33; // assume the worst

        __try
            {
            unsigned lxa = WW2L(w5,w6);
            WORD DOSdate, DOStime;
            FILETIME ft;
            WIN32_FIND_DATA fd, *pFD = &fd;
            char *pch;

            Str68ToStr(WW2L(w3,w4), rgch);

            if ((hFind = FindFirstFile(rgch, pFD)) == INVALID_HANDLE_VALUE)
                {
                vpregs->D0 = -33; // file not found
                break;
                }

            if (pFD->cAlternateFileName[0])
                pch = (unsigned char *)&pFD->cAlternateFileName;
            else
                pch = (unsigned char *)&pFD->cFileName;

            if (pFD->dwFileAttributes & 4)
                vmPokeW(lxa+0,  0x41FF);    // mode
            else
                vmPokeW(lxa+0,  0x01FF);    // mode
            vmPokeL(lxa+2,  2);        // index
            vmPokeL(lxa+6,  5);        // dev
            vmPokeW(lxa+8,  0);        // reserved
            vmPokeW(lxa+10, 1);        // nlink
            vmPokeW(lxa+12, 0);        // uid
            vmPokeW(lxa+14, 0);        // gid
            vmPokeL(lxa+16, pFD->nFileSizeLow);        // size
            vmPokeL(lxa+20, 2048);    // blksize
            vmPokeL(lxa+24, 0);        // nblocks

            FileTimeToLocalFileTime(&pFD->ftLastWriteTime, &ft);
            FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
            vmPokeW(lxa+28, DOStime);    // mtime
            vmPokeW(lxa+30, DOSdate);    // mdata

            FileTimeToLocalFileTime(&pFD->ftLastAccessTime, &ft);
            FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
            vmPokeW(lxa+32, DOStime);    // atime
            vmPokeW(lxa+34, DOSdate);    // adata

            FileTimeToLocalFileTime(&pFD->ftCreationTime, &ft);
            FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
            vmPokeW(lxa+36, DOStime);    // mtime
            vmPokeW(lxa+38, DOSdate);    // mdata

            vmPokeW(lxa+40, pFD->dwFileAttributes);
            vmPokeW(lxa+42, 0);        // reserved
            vmPokeL(lxa+44, 0);        // reserved
            vmPokeL(lxa+48, 0);        // reserved

            vpregs->D0 = 0;
            FindClose(hFind);
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        break;

    case 322: // Magic 4 Dxreadaddr
        fHandled = TRUE;
        vpregs->D0 = -33; // assume the worst
//        vpregs->D0 = -32; // invalid function
//        break;
        {
        __try
            {
            MINT_DIR *pmd = (void *)WW2L(w3,w4);
            unsigned lsz = WW2L(w5,w6);
            unsigned lxa = WW2L(w7,w8);
            WORD DOSdate, DOStime;
            FILETIME ft;

            if (pmd->lMagic == 0x12345678)
                {
                if (pmd->iCur < pmd->iMac)
                    {
                    WIN32_FIND_DATA *pFD = &pmd->rgfd[pmd->iCur];
                    char *pch;

                    if (pFD->cAlternateFileName[0])
                        pch = (unsigned char *)&pFD->cAlternateFileName;
                    else
                        pch = (unsigned char *)&pFD->cFileName;

                    if (!pmd->fCompat)
                        {
                        vmPokeL(lsz, (long)pmd + pmd->iCur); // index??
                        StrToStr68(pch, lsz+4);
                        }
                    else
                        {
                        StrToStr68(pch, lsz);
                        }

                    if (pFD->dwFileAttributes & 4)
                        vmPokeW(lxa+0,  0x41FF);    // mode
                    else
                        vmPokeW(lxa+0,  0x01FF);    // mode
                    vmPokeL(lxa+2,  2);        // index
                    vmPokeL(lxa+6,  5);        // dev
                    vmPokeW(lxa+8,  0);        // reserved
                    vmPokeW(lxa+10, 1);        // nlink
                    vmPokeW(lxa+12, 0);        // uid
                    vmPokeW(lxa+14, 0);        // gid
                    vmPokeL(lxa+16, pFD->nFileSizeLow);        // size
                    vmPokeL(lxa+20, 2048);    // blksize
                    vmPokeL(lxa+24, 0);        // nblocks

                    FileTimeToLocalFileTime(&pFD->ftLastWriteTime, &ft);
                    FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
                    vmPokeW(lxa+28, DOStime);    // mtime
                    vmPokeW(lxa+30, DOSdate);    // mdata

                    FileTimeToLocalFileTime(&pFD->ftLastAccessTime, &ft);
                    FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
                    vmPokeW(lxa+32, DOStime);    // atime
                    vmPokeW(lxa+34, DOSdate);    // adata

                    FileTimeToLocalFileTime(&pFD->ftCreationTime, &ft);
                    FileTimeToDosDateTime(&ft, &DOSdate, &DOStime);
                    vmPokeW(lxa+36, DOStime);    // mtime
                    vmPokeW(lxa+38, DOSdate);    // mdata

                    vmPokeW(lxa+40, pFD->dwFileAttributes);
                    vmPokeW(lxa+42, 0);        // reserved
                    vmPokeL(lxa+44, 0);        // reserved
                    vmPokeL(lxa+48, 0);        // reserved

                    pmd->iCur++;
                    vpregs->D0 = 0;
                    }
                }
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
        break;

    case 338: // Magic 4 DreadLabel
        {
        char szName[MAX_PATH], sz[MAX_PATH];
        int i;

        fHandled = TRUE;
        Str68ToStr(WW2L(w2,w3), rgch);
        if (GetVolumeInformation(rgch, szName, w6, NULL, &i, &i, sz, 12))
            {
            StrToStr68(szName, WW2L(w4,w5));
            vpregs->D0 = 0;
            }
        else
            vpregs->D0 = -64; // ENAMETOOULONG
        break;
        }

    case 51:  // Sconfig
//        fHandled = TRUE;
//        vpregs->D0 = 0;
        break;
        }

    SetErrorMode(emOld);

    // Pexec post processing

    switch (mdPexec)
        {
    case 0:
        break;

    case 1:
        // we have received a Pexec command.
        // First step is to create a basepage

    DebugStr("BP=%08X ", BPCur());
    DebugStr("in case 1\n");
        vmPokeW(savedSP+2, 5);  // change to Pexec(5)
        mdPexec = 2;
        return -1;              // execute trap and rewind PC

    case 2:
        // we've created a basepage, now load in the program.
        // If the program load fails for whatever reason, we
        // do an Mfree and force ourselves into stage 3

        {
        unsigned long cb = 28;
        unsigned long bp = vpregs->D0;
        unsigned long ptext = bp + 256;
        unsigned long papply;
        unsigned long cbRead;
        unsigned long b;
        void *pv = NULL;

    DebugStr("BP=%08X ", BPCur());
    DebugStr("in case 2\n");
        DebugStr("address of new basepage = %08X\n", bp);

        pv = malloc(cb);

        if (pv && ReadFile(hPexec, pv, cb, &cbRead, NULL) &&
            HostToGuestDataCopy(ptext, pv, ACCESS_WRITE, cb))
            {
            free(pv);
            }
        else
            {
            free(pv);
            GetLastError();
            goto PexecError;
            }

        PokeL(bp+8,  ptext);                        // start of code
        PokeL(bp+12, PeekL(ptext+2));                // code size
        PokeL(bp+16, bp+256 + PeekL(bp+12));        // start of data
        PokeL(bp+20, PeekL(ptext+6));                // size of data
        PokeL(bp+24, PeekL(bp+16)+PeekL(bp+20));    // start of bss
        PokeL(bp+28, PeekL(ptext+10));                // size of bss

        DebugStr("Basepage+0  = %08X\n", PeekL(bp+0));
        DebugStr("Basepage+4  = %08X\n", PeekL(bp+4));
        DebugStr("Basepage+8  = %08X\n", PeekL(bp+8));
        DebugStr("Basepage+12 = %08X\n", PeekL(bp+12));
        DebugStr("Basepage+16 = %08X\n", PeekL(bp+16));
        DebugStr("Basepage+20 = %08X\n", PeekL(bp+20));
        DebugStr("Basepage+24 = %08X\n", PeekL(bp+24));
        DebugStr("Basepage+28 = %08X\n", PeekL(bp+28));
        DebugStr("Basepage+32 = %08X\n", PeekL(bp+32));
        DebugStr("Basepage+36 = %08X\n", PeekL(bp+36));
        DebugStr("Basepage+40 = %08X\n", PeekL(bp+40));
        DebugStr("Basepage+44 = %08X\n", PeekL(bp+44));

        // read text and data and symbols sections

        cb = PeekL(ptext+2) + PeekL(ptext+6) + PeekL(ptext+14);

        if (cb < vi.cbRAM[0])
            {
            pv = malloc(cb);
            }
        else
            {
            pv = NULL;
            }

        if (pv && ReadFile(hPexec, pv, cb, &cbRead, NULL) &&
                HostToGuestDataCopy(ptext, pv, ACCESS_WRITE, cb))
            {
            free(pv);
            }
        else
            {
            free(pv);
            goto PexecError;
            }

        ReadFile(hPexec, ((char *)&b)+3, 1, &cbRead, NULL);
        ReadFile(hPexec, ((char *)&b)+2, 1, &cbRead, NULL);
        ReadFile(hPexec, ((char *)&b)+1, 1, &cbRead, NULL);

        if (ReadFile(hPexec, &b, 1, &cbRead, NULL))
            {
            papply = ptext + b;

            while(b)
                {
                if (b != 1)
                    vmPokeL(papply, PeekL(papply)+ptext);

                b = 0;
                if (ReadFile(hPexec, &b, 1, &cbRead, NULL))
                    {
                    papply += ((b == 1) ? 254 : b);
                    }
                else
                    goto PexecError;
                }
            }
        else
            goto PexecError;

        CloseHandle(hPexec);

        CloseFilesProcess(bp);

        {
        unsigned long ea = PeekL(bp+24);
        unsigned long cb = PeekL(bp+28); // clear BSS only
//        unsigned long cb = PeekL(bp+4) - ea; // clear whole TPA

        while (cb >= 4)
            {
            vmPokeL(ea,0);
            ea += 4;
            cb -= 4;
            }
        }

        if (savedMode == 0)
            {
            // load and go

        DebugStr("setting Pexec(4)\n");
            mdPexec = 0;
            vmPokeW(savedSP+2, 6);  // change to Pexec(4)

            if (v.rgosinfo[vmCur.iOS].osType == osTOS1X_D)
                vmPokeW(savedSP+2, 4);  // change to Pexec(4)

            vmPokeL(savedSP+8, bp);
            return 1;
            }
        else
            {
            // load, don't go

        DebugStr("doing a \"load, no go\"\n");
            mdPexec = 0;
            return 0;
            }

PexecError:
    DebugStr("setting Mfree\n");
        CloseHandle(hPexec);
        mdPexec = 3;
        vmPokeW(savedSP, 73);                // Mfree
        vmPokeL(savedSP+2, vpregs->D0);
        vpregs->D0 = -33;
        return -1;
        }

    case 3:
        // we had an error, just return an error code

    DebugStr("BP=%08X ", BPCur());
    DebugStr("in case 3\n");
        mdPexec = 0;
        vpregs->D0 = 0xFFFFFFFF;
        return 0;
        }

    if (fHandled)
        DebugStr("GEMDOS returning %d\n", vpregs->D0);
    else
        DebugStr("Not handled\n", vpregs->D0);
    return (fHandled ? 0 : 1);
}

#endif // ATARIST

