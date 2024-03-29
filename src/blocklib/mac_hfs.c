
/****************************************************************************

    MAC_HFS.C

    File manipulation code for Macintosh HFS disks

    Copyright (C) 1998-2021 by Darek Mihocka and Danny Miller. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

    11/09/1998  darekm
    04/27/2018  darekm      cleanup for 10.0 release for Windows 10

****************************************************************************/

#include "precomp.h"

#pragma warning (disable:4201) // nameless struct/union

#pragma pack(1)

typedef struct node_descriptor
{
    long    ndFLink;
    long    ndBLink;
    unsigned char    ndType;
    unsigned char    ndNHeight;
    short   ndNRecs;
    short   ndRes;
} ND;

typedef struct Btree_header_node
{
    ND      nd;
    unsigned short   bthDepth;
    long    bthRoot;
    long    bthNRecs;
    long    bthFNode;
    long    bthLNode;
    unsigned short   bthNodeSize;
    unsigned short   bthKeyLen;
    long    bthNNodes;
    long    bthFree;
    unsigned char    rgb[468];
} BTHN;


typedef union B_node
{
        ND      nd;
        unsigned char    *rgbX;

        unsigned char rgb[512];
        unsigned short   *rgofs;
} NODE;

typedef struct B_key
{
    unsigned char cb;
    char    res1;
    long    dirID;
    unsigned char cbName;
    unsigned char    *sz;
} KEY;

typedef struct B_rec
{
    unsigned char bType;
    char    res1;

    union
        {
            unsigned char filFlag;
            unsigned char filType;
            unsigned long l[4];
            unsigned long filID;
            unsigned short dataStart;
            unsigned long  cbData;
            unsigned long  cbDataPhys;
            unsigned short rsrcStart;
            unsigned long  cbRsrc;
            unsigned long  cbRsrcPhys;
            unsigned long filCrDat;
            unsigned long filMdDat;
            unsigned long filBkDat;
            unsigned char rgbFinder[16];
            unsigned short filClumpSize;
            unsigned char rgbDataExtent[12];
            unsigned char rgbRsrcExtent[12];
        
            unsigned short dirFlag;
            unsigned short dirVal;
            unsigned long dirID;
            unsigned long dirCrDat;
            unsigned long dirMdDat;
            unsigned long dirBkDat;
        };
} REC;


typedef struct mfs_rec
{
    unsigned char flFlags;
    unsigned char flTyp;
    unsigned long l[4];
    unsigned long flFlNum;
    unsigned short dataStart;
    unsigned long cbData;
    unsigned long cbDataPhys;
    unsigned short rsrcStart;
    unsigned long cbRsrc;
    unsigned long cbRsrcPhys;
    unsigned long filCrDat;
    unsigned long filMdDat;
    unsigned char cbName;
    unsigned char     *sz;
} MREC;


typedef struct ataridos_rec
{
    unsigned char bFlags;
    unsigned short wCount;
    unsigned short wStart;
    unsigned char filename[11];
} AREC;


typedef struct msdos_rec
{
    unsigned char filename[11];
    unsigned char bFlags;
    unsigned char rgbRes[10];
    unsigned short wTime;
    unsigned short wDate;
    unsigned short wStart;
    unsigned long lSize;
} MSREC;


//
// Helper routines
//

void SzFrom8_3(char *szTo, char *szFrom)
{
    int i;

    for (i = 0; i < 12; i++)
        {
        char sz[2];
        sz[0] = szFrom[i - (i > 8)];
        sz[1] = 0;

        if (i == 8)
            sz[0] = '.';

        if (sz[0] != ' ')
            strcat(szTo, sz);
        }

    if (szTo[strlen(szTo)-1] == '.')
        szTo[strlen(szTo)-1] = 0;
}

BOOL AddEntryToPpfd(WIN32_FIND_DATA **ppfd, WIN32_FIND_DATA *pfd, ULONG count)
{
    if (ppfd)
        {
#if USEHEAP
        void *p1 = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            *ppfd, sizeof(WIN32_FIND_DATA) * (count+1));
#else
        void *p1 = realloc(*ppfd, sizeof(WIN32_FIND_DATA) * (count+1));
#endif
        if (!p1)
        {
            free(*ppfd);
            *ppfd = NULL;
            return FALSE;
        }
        *ppfd = p1;
        (*ppfd)[count] = *pfd;

        return TRUE;
        }

    return FALSE;
}


//
// Main disk directory reading code
//

int __stdcall CntReadDiskDirectory(DISKINFO *pdi, char *szDir, WIN32_FIND_DATA **ppfd)
{
    unsigned char rgb[512];
    int count = 0;
    WIN32_FIND_DATA fd;
    int  i, j;

    if (pdi == NULL)
        return 0;

    if (szDir[0] == 0)
        {
        pdi->cfd = 0;
        }

    if (pdi->fst == FS_HFS)
        {
        // oh boy, here we go people, a B* tree.

        NODE node;
        BTHN bthn;
        int  lFirstCatBlock;
        int  lBlockSize;

        if (szDir[0] != 0)
            goto Lfindit;

        // Read the Master Directory Block to find the start of
        // the Catalog file.

        pdi->count = 1;
        pdi->sec   = 2;
        pdi->lpBuf = rgb;

        if (FRawDiskRWPdi(pdi, 0))
            {
            lBlockSize = (rgb[22] << 8) + rgb[23];

            lFirstCatBlock = (rgb[150] << 8) + rgb[151];
            lFirstCatBlock *= lBlockSize / 512;
            lFirstCatBlock += rgb[29];

#if TRACEDISK
            printf("Block size = %d\n", lBlockSize);
            printf("Catalog starts at block %d\n", lFirstCatBlock);
#endif
            }
        else
            return 0;

        // Read the header node

        pdi->count = 1;
        pdi->sec   = lFirstCatBlock;
        pdi->lpBuf = (BYTE *)&bthn;

        if (FRawDiskRWPdi(pdi, 0))
            {
            // display the node header

            bthn.nd.ndFLink = SwapL(bthn.nd.ndFLink);
            bthn.nd.ndBLink = SwapL(bthn.nd.ndBLink);
            bthn.nd.ndNRecs = (short)SwapW(bthn.nd.ndNRecs);

#if TRACEDISK
            printf("B-Tree Header Node  F link    = %d\n", bthn.nd.ndFLink);
            printf("B-Tree Header Node  B link    = %d\n", bthn.nd.ndBLink);
            printf("B-Tree Header Node  # records = %d\n", bthn.nd.ndNRecs);
            printf("B-Tree Header Node  node type = %d\n", bthn.nd.ndType);
#endif

            // display the B-Tree header record

            bthn.bthDepth    = (short)SwapW(bthn.bthDepth);
            bthn.bthRoot     = SwapL(bthn.bthRoot);
            bthn.bthNRecs    = SwapL(bthn.bthNRecs);
            bthn.bthFNode    = SwapL(bthn.bthFNode);
            bthn.bthLNode    = SwapL(bthn.bthLNode);
            bthn.bthNodeSize = (short)SwapW(bthn.bthNodeSize);
            bthn.bthKeyLen   = (short)SwapW(bthn.bthKeyLen);
            bthn.bthNNodes   = SwapL(bthn.bthNNodes);
#if TRACEDISK
            printf("B-Tree Header Node  depth     = %d\n", bthn.bthDepth);
            printf("B-Tree Header Node  root node = %d\n", bthn.bthRoot);
            printf("B-Tree Header Node  # records = %d\n", bthn.bthNRecs);
            printf("B-Tree Header Node  frst node = %d\n", bthn.bthFNode);
            printf("B-Tree Header Node  last node = %d\n", bthn.bthLNode);
            printf("B-Tree Header Node  node size = %d\n", bthn.bthNodeSize);
            printf("B-Tree Header Node  key len   = %d\n", bthn.bthKeyLen);
            printf("B-Tree Header Node  # nodes   = %d\n", bthn.bthNNodes);
            printf("87654321 backwards = %08X\n", SwapL(0x87654321));
#endif
            }
        else
            return 0;

        for (i = 0; i <= bthn.bthNNodes; i++)
            {
            // Read the header node

            pdi->count = 1;
            pdi->sec   = lFirstCatBlock + i;
            pdi->lpBuf = (BYTE *)&node;
    
            if (FRawDiskRWPdi(pdi, 0))
                {
                if (node.nd.ndType != 0xFF)
                    continue;

                // display the node header
    
                node.nd.ndFLink = SwapL(node.nd.ndFLink);
                node.nd.ndBLink = SwapL(node.nd.ndBLink);
                node.nd.ndNRecs = (short)SwapW(node.nd.ndNRecs);
#if TRACEDISK
                printf("\n\nNode #%d\n", i);
                printf("Node  F link    = %d\n", node.nd.ndFLink);
                printf("Node  B link    = %d\n", node.nd.ndBLink);
                printf("Node  # records = %d\n", node.nd.ndNRecs);
#endif
    
                for (j = 0; j < node.nd.ndNRecs; j++)
                    {
                    int cbKey, cbName;
                    unsigned char sz[256];
                    KEY *pKey;
                    REC *pRec;
                    int type;
                    ULONG lTime;
                    __int64 llTime;
                    ULONG lSize;
                    FILETIME ft;
                    SYSTEMTIME st;
                    long parentID, fileID;

                    node.rgofs[-1-j] = (short)SwapW(node.rgofs[-1-j]);
                    pKey = (KEY *)&node.rgb[node.rgofs[-1-j]];

                    cbKey = pKey->cb;
                    cbName = pKey->cbName;
                    strncpy((char *)sz, (const char *)pKey->sz, cbName);
                    sz[cbName] = '\0';
                    parentID = SwapL(pKey->dirID);

                    pRec = (REC *)(void *)&pKey->sz[cbName + ((cbName & 1) == 0)];
Assert((((ULONG)(LONGLONG)pRec) & 1) == 0);
                    type = pRec->bType;

                    if (cbKey < 7)
                        continue;

                    if (type == 1)
                        {
                        lTime = SwapL(pRec->dirMdDat);
                        lSize = 0;
                        fileID = SwapL(pRec->dirID);
                        }
                    else if (type == 2)
                        {
                        lTime = SwapL(pRec->filMdDat);
                        lSize = SwapL(pRec->cbRsrc) + SwapL(pRec->cbData);
                        fileID = SwapL(pRec->filID);
                        }
                    else
                        {
                        lTime = 0;
                        lSize = 0;
                        fileID = 0;
                        }

                    memset(&st, 0, sizeof(st));
                    st.wYear = 1904;
                    st.wDay = 1;
                    st.wMonth = 1;
                    SystemTimeToFileTime(&st, &ft);
                    llTime = ((__int64)lTime) * 10000000;
                    llTime += *(__int64 *)&ft;
                    memcpy(&ft, &llTime, 8);
                    FileTimeToSystemTime(&ft, &st);

#if TRACEDISK
                    printf("\n");
                    printf("&node = %08X, &key = %08X\n",
                        &node, pKey);
                    printf("offset %d = %d\n", j, node.rgofs[-1-j]);

                    printf("Key length = %d, cbName = %d\n",
                        cbKey, cbName);
                    printf("Parent ID = %d, file/dir ID = %d\n",
                        parentID, fileID);
                    printf("Entry name = %s, type = %d\n",
                        sz, type);
                    printf("Time stamp = %08X, %02d/%02d/%04d %02d:%02d:%02d\n",
                        lTime, st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
#endif

                    if (cbName < 1)
                        continue;

                    if ((type != 1) && (type != 2))
                        continue;

#if 0
                    if (parentID != pdi->dirID)
                        continue;
#endif

#if TRACEDISK
                    printf("Keeping entry name = %s, type = %d\n",
                        sz, type);
#endif

                    memset(&fd, 0, sizeof(fd));
                    strcpy(fd.cFileName, (const char *)sz);
                    memcpy(&fd.ftCreationTime, &pRec->l, 8);
                    fd.dwReserved0 = parentID;
                    fd.dwReserved1 = fileID;
                    fd.nFileSizeLow = lSize;
                    fd.nFileSizeHigh = SwapL(pRec->cbData);
                    fd.ftLastWriteTime = ft;
                    fd.dwFileAttributes = (type == 1)
                        ? FILE_ATTRIBUTE_DIRECTORY : 0;
                    if (SwapL(pRec->cbData))
                        memcpy(fd.cAlternateFileName, &pRec->rgbDataExtent, 12);
                    else
                        memcpy(fd.cAlternateFileName, &pRec->rgbRsrcExtent, 12);

                    pdi->cfd += AddEntryToPpfd(&pdi->pfd, &fd, pdi->cfd);
                    }
                }
            else
                {
#if TRACEDISK
                printf("DISK ERROR on node %d\n", i);
#endif
                }
            }

        // We now have the whole Mac disk directory cached in the pdi

Lfindit:
        // At this point the entire Mac disk directory is cached
        // Walk through it, starting at the root, until we find
        // the directory matching szDir, then get its directory ID

        if (szDir[0] == 0)
            {
            // Mac root directory ID is 2

            pdi->dirID = 1;
            }
        else if (strncmp(szDir, pdi->szCwd, strlen(pdi->szCwd)))
            {
            // current directory is not a parent of szDir so
            // start searching from the root

            pdi->szCwd[0] = 0;
            pdi->dirID = 1;
            }

        // Keep looping deeper and deeper until szCwd matches szDir

        for (;;)
            {
            int cchCwd = (int)strlen(pdi->szCwd);
            int cchDir = (int)strlen(szDir);
            unsigned char sz[MAX_PATH];

            if (cchCwd == cchDir)
                break;

            strcpy((char *)sz, &szDir[cchCwd]);

            if (strstr((const char *)sz, ":"))
                *strstr((const char *)sz, ":") = 0;

            for (i = 0; i < pdi->cfd; i++)
                {
                long parentID, fileID;
                
                parentID = (pdi->pfd)[i].dwReserved0;
                fileID = (pdi->pfd)[i].dwReserved1;

                if ((parentID == pdi->dirID) && 
                   ((pdi->pfd)[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                    if (!strcmp((const char *)sz, (pdi->pfd)[i].cFileName))
                        {
                        strcat(pdi->szCwd, (const char *)sz);
                        strcat(pdi->szCwd, ":");
                        pdi->dirID = fileID;
                        break;
                        }
                    }
                }
            }

        // We're at the right directory now, pick off all files with
        // that same parent ID

        for (i = 0; i < pdi->cfd; i++)
            {
            long parentID, fileID;
                
            parentID = (pdi->pfd)[i].dwReserved0;
            fileID = (pdi->pfd)[i].dwReserved1;

            if (parentID == pdi->dirID)
                {
                if (ppfd)
                    count += AddEntryToPpfd(ppfd, &pdi->pfd[i], count);
                }
            }
        }
    else if (pdi->fst == FS_MFS)
        {
        // 400K/800K Mac disk with flat file directory

        int  lFirstCatBlock;
        int  lBlockSize;
        int  cCatBlocks;

        // this is flat directory!

        if (szDir[0] != 0)
            return 0;

        // Read the Master Directory Block to find the start of
        // the Catalog file.

        pdi->count = 1;
        pdi->sec   = 2;
        pdi->lpBuf = rgb;

        if (FRawDiskRWPdi(pdi, 0))
            {
            lBlockSize = (rgb[22] << 8) + rgb[23];

#if 0
            lFirstCatBlock = (rgb[150] << 8) + rgb[151];
            lFirstCatBlock *= lBlockSize / 512;
#endif
            lFirstCatBlock = rgb[15];

            cCatBlocks = (rgb[16] << 8) + rgb[17];

#if TRACEDISK
            printf("Block size = %d\n", lBlockSize);
            printf("Catalog starts at block %d\n", lFirstCatBlock);
            printf("Number of Catalog blocks = %d\n", cCatBlocks);
#endif
            }
        else
            return 0;

        for (i = 0; i < cCatBlocks; i++)
            {
            // Read the header node

            pdi->count = 1;
            pdi->sec   = lFirstCatBlock + i;
            pdi->lpBuf = rgb;
    
            if (FRawDiskRWPdi(pdi, 0))
                {
                for (j = 0;;)
                    {
                    int cbName;
                    unsigned char sz[256];
                    MREC *pMRec;
                    ULONG lTime;
                    __int64 llTime;
                    ULONG lSize;
                    FILETIME ft;
                    SYSTEMTIME st;

                    pMRec = (MREC *)(void *)&rgb[j];

                    if ((pMRec->flFlags & 0x80) == 0)
                        break;

                    cbName = pMRec->cbName;
                    strncpy((char *)sz, (const char *)pMRec->sz, cbName);
                    sz[cbName] = '\0';

                    lTime = SwapL(pMRec->filMdDat);
                    lSize = SwapL(pMRec->cbRsrc) + SwapL(pMRec->cbData);

                    memset(&st, 0, sizeof(st));
                    st.wYear = 1904;
                    st.wDay = 1;
                    st.wMonth = 1;
                    SystemTimeToFileTime(&st, &ft);
                    llTime = ((__int64)lTime) * 10000000;
                    llTime += *(__int64 *)&ft;
                    memcpy(&ft, &llTime, 8);
                    FileTimeToSystemTime(&ft, &st);

#if TRACEDISK
                    printf("\n");

                    printf("Entry name = %s, cbName = %d\n",
                        sz, cbName);
                    printf("Time stamp = %08X, %02d/%02d/%04d %02d:%02d:%02d\n",
                        lTime, st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
#endif

                    if (cbName < 1)
                        break;

#if TRACEDISK
                    printf("Keeping entry name = %s\n", sz);
#endif

                    memset(&fd, 0, sizeof(fd));
                    strcpy(fd.cFileName, (const char *)sz);
                    memcpy(&fd.ftCreationTime, &pMRec->l, 8);
                    fd.nFileSizeLow = lSize;
                    fd.nFileSizeHigh = SwapL(pMRec->cbData);
                    fd.ftLastWriteTime = ft;
                    fd.dwFileAttributes = 0;

                    pdi->cfd += AddEntryToPpfd(&pdi->pfd, &fd, pdi->cfd);
                    count += AddEntryToPpfd(ppfd, &fd, count);

                    j += sizeof(MREC) + cbName + ((cbName & 1) == 0);
                    }
                }
            else
                {
#if TRACEDISK
                printf("DISK ERROR on node %d\n", i);
#endif
                }
            }
        }
    else if (pdi->fst == FS_ATARIDOS || pdi->fst == FS_MYDOS)
    {
        ULONG lSize = GetFileSize(pdi->h, NULL);
        int cbSector = 128;
        //int vtocSec  = 360;
        int rootSec  = 361;
        BYTE rgbA[8 * 256];

        if (lSize >= (180*1024))
            cbSector = 256;

        // read in the entire Atari disk directory

        pdi->count = 8 * cbSector / 512;
        pdi->sec   = (rootSec-1) * cbSector / 512;
        pdi->lpBuf = rgbA;

        if (FRawDiskRWPdi(pdi, 0))
        {
            for (j = 0; j < 8*cbSector; j+=sizeof(AREC))
            {
                AREC *parec = (AREC *)&rgbA[j];

#if TRACEDISK
                printf("flags byte = %02X\n", parec->bFlags);
                printf("sector count = %d\n", parec->wCount);
                printf("sector start = %d\n", parec->wStart);
                printf("name = %11s\n", &parec->filename);
#endif

                if (parec->bFlags == 0x42 || parec->bFlags == 0x62)
                {
                    memset(&fd, 0, sizeof(fd));
                    SzFrom8_3((char *)&fd.cFileName, (char *)&parec->filename);
                    memcpy(fd.cAlternateFileName, &parec->wStart, 2);
                    fd.nFileSizeHigh =
                    fd.nFileSizeLow = parec->wCount * cbSector;
                    memset(&fd.ftLastWriteTime, 0, sizeof(FILETIME));
                    fd.dwFileAttributes = 0;

                    pdi->cfd += AddEntryToPpfd(&pdi->pfd, &fd, pdi->cfd);

                    if (ppfd)
                        count += AddEntryToPpfd(ppfd, &fd, count);
                }
            }
        }
    }

#if 0   // uses 512K of stack space as written
    else if (pdi->fst == FS_FAT12 || pdi->fst == FS_FAT16)
    {
        int rootSec;    // start of root directory sectors
        int croot;      // count of root directory sectors
        int cbSec;      // size of sectors (should be 512, larger for BGM)
        unsigned char szSoFar[MAX_PATH];

        szSoFar[0] = 0;

        // read in the boot sector

        pdi->count = 1;
        pdi->sec   = 0;
        pdi->lpBuf = rgb;

        if (FRawDiskRWPdi(pdi, 0))
            {
            cbSec = rgb[11] + (rgb[12] << 8);
#if TRACEDISK
            printf("sector size = %d bytes!\n", cbSec);
#endif

            // directory starts at sector 1 + (num FAT) * (size FAT)
            rootSec = rgb[16] * (rgb[22] + (rgb[23] << 8)) + 1;

            croot = rgb[17] + (rgb[18] << 8);

            for (i = 0; i < (int)(croot/(512/sizeof(MSREC))); i++)
                {
                unsigned char rgbBig[512*1024]; // !!! on the stack ?!?
                int cbBig = 512;

                pdi->count = 1;
                pdi->sec   = (rootSec*cbSec/512) + i;
                pdi->lpBuf = rgbBig;

                if (!FRawDiskRWPdi(pdi, 0))
                    continue;

Lnextdir:
                for (j = 0; j < cbBig; j+=sizeof(MSREC))
                    {
                    MSREC *pmsrec = (MSREC *)&rgbBig[j];

                    // unused directory entry

                    if (pmsrec->filename[0] == 0)
                        continue;

                    // deleted directory entry

                    if (pmsrec->filename[0] == 0xE5)
                        continue;

                    // LFN directory entry

                    if (pmsrec->bFlags == 0x0F)
                        continue;

                    // $05 maps to $E5

                    if (pmsrec->filename[0] == 0x05)
                        pmsrec->filename[0] = 0xE5;

#if TRACEDISK
                    printf("name = %11s\n", &pmsrec->filename);
                    printf("flags byte = %02X\n", pmsrec->bFlags);
                    printf("file size = %d\n", pmsrec->lSize);
                    printf("cluster start = %d\n", pmsrec->wStart);
                    printf("file time = %04X\n", pmsrec->wTime);
                    printf("file date = %04X\n", pmsrec->wDate);
#endif

                    memset(&fd, 0, sizeof(fd));
                    SzFrom8_3((char *)&fd.cFileName, (char *)&pmsrec->filename);
                    memcpy(fd.cAlternateFileName, &pmsrec->wStart, 2);
                    fd.nFileSizeHigh =
                    fd.nFileSizeLow = pmsrec->lSize;
                    DosDateTimeToFileTime(pmsrec->wDate, pmsrec->wTime,
                         &fd.ftLastWriteTime);
                    fd.dwFileAttributes = pmsrec->bFlags;

                    if (!(pmsrec->bFlags & 0x10))
                        {
                        // REVIEW: anything to do with a file?

                        // CbReadFileContents(pdi, &rgbBig, &fd);
                        }
                    else
                        {
                        // Check if this directory is what we're looking for

                        unsigned char szTmp[MAX_PATH];

                        if (!strcmp(fd.cFileName, "."))
                            continue;

                        strcpy((char *)szTmp, (const char *)szSoFar);
                        strcat((char *)szTmp, fd.cFileName);
                        strcat((char *)szTmp, "\\");

                        if (!strncmp((const char *)szTmp, (const char *)szDir, strlen((const char *)szTmp)))
                            {
                            // file size for a directory is set to 0
                            fd.nFileSizeLow = sizeof(rgbBig);
                            memset((unsigned char *)rgbBig, 0, sizeof(rgbBig));
                            cbBig = CbReadFileContents(pdi, (unsigned char *)&rgbBig, &fd);
                            fd.nFileSizeLow = 0;

                            strcpy((char *)szSoFar, (const char *)szTmp);
                            pdi->cfd = 0;
                            count = 0;
                            croot = 0;
                            goto Lnextdir;
                            }
                        }

                    pdi->cfd += AddEntryToPpfd(&pdi->pfd, &fd, pdi->cfd);

                    if (ppfd)
                        count += AddEntryToPpfd(ppfd, &fd, count);
                    }
                }
            }
    }
#endif

    strcpy(pdi->szCwd, szDir);

    return count;
}


// call with pb == NULL to just get size requirement
// !!! that feature is only supported by ATARI for now, others will crash or fail
//
ULONG CbReadFileContents(DISKINFO *pdi, unsigned char *pb, WIN32_FIND_DATA *pfd)
{
    unsigned char rgb[512];
    int cb = 0;
    int  i;

    if (pdi->fst == FS_HFS)
        {
        int  lFirstBlock;
        int  lBlockSize;

        // Read the Master Directory Block to find the start of
        // the Catalog file.

        pdi->count = 1;
        pdi->sec   = 2;
        pdi->lpBuf = rgb;

        if (FRawDiskRWPdi(pdi, 0))
            {
            lBlockSize = (rgb[22] << 8) + rgb[23];
            lFirstBlock = rgb[29];

#if TRACEDISK
            printf("Block size = %d\n", lBlockSize);
            printf("Data starts at block %d\n", lFirstBlock);
#endif
            }
        else
            return 0;

        // 3 extent records stored in directory entry

        for (i = 0; i < 3; i++)
            {
            pdi->count = lBlockSize / 512 * SwapW(*(WORD *)&pfd->cAlternateFileName[i*4 + 2]);
            pdi->sec   = lFirstBlock + lBlockSize / 512 * SwapW(*(WORD *)&pfd->cAlternateFileName[i*4]);
            pdi->lpBuf = pb;

            if (!FRawDiskRWPdi(pdi, 0))
                break;

            cb += lBlockSize * SwapW(*(WORD *)&pfd->cAlternateFileName[i*4 + 2]);
            pb += lBlockSize * SwapW(*(WORD *)&pfd->cAlternateFileName[i*4 + 2]);
            }
        }
    else if (pdi->fst == FS_MFS)
        {
        }
    else if (pdi->fst == FS_ATARIDOS || pdi->fst == FS_MYDOS)
    {
        ULONG count = pfd->nFileSizeHigh * 4 / pdi->cbSector;    // # of sectors in this file
        WORD sec = *(WORD *)(pfd->cAlternateFileName);    // starting sector, 1 based

        for (unsigned int j = 0; j < count; j++)
        {
            pdi->count = 1;
            pdi->sec = (sec - 1) * 128 / pdi->cbSector;    // which 512 byte sector number would this be found in?
            pdi->lpBuf = rgb;

            if (!FRawDiskRWPdi(pdi, 0))
            {
                cb = 0;
                break;
            }

            int pos = ((sec - 1) % 4) << 7;    // where in the buffer did the 128 bytes we're interested in go?

            unsigned int cbT = rgb[pos + 127];
            if (cbT != 125 && j != count - 1)    // only the last sector should be short, this is a corrupted file
                return 0;

            // just provide the memory requirement if NULL
            if (pb)
            {
                memcpy(pb, &rgb[pos], cbT);    // copy the 125 bytes actually used
                pb += cbT;
            }
            cb += cbT;

            sec = rgb[pos + 126] | ((rgb[pos + 125] & 3) << 8);    // next sector
        }
    }
    else if (pdi->fst == FS_FAT12 || pdi->fst == FS_FAT16)
        {
        int rootSec;    // start of root directory sectors
        int dataSec;    // start of data sectors
        int croot;      // count of root directory sectors
        int cbSec;      // size of logical sector (usually 512, larger for BGM)
        int cbClust;    // size of cluster (cbSec * sec/clust)
        int cluster, cbFile;
        int cnFatEntry = (pdi->fst == FS_FAT12) ? 3 : 4; // count of nibbles

#if TRACEDISK
        printf("FAT entry size = %d bytes!\n", cnFatEntry);
#endif

        // read in the boot sector

        pdi->count = 1;
        pdi->sec   = 0;
        pdi->lpBuf = rgb;

        if (FRawDiskRWPdi(pdi, 0))
            {
            unsigned char rgbFAT[1024];  // two sectors worth of FAT
            int secFAT = -1;
            int clusterMac = (pdi->fst == FS_FAT16) ? 0xFFFF : 0x0FFF;

            cbSec = rgb[11] + (rgb[12] << 8);
            cbClust = rgb[13] * cbSec;
#if TRACEDISK
            printf("sector size = %d bytes!\n", cbSec);
            printf("cluster size = %d bytes!\n", cbClust);
#endif

            // FAT starts at logical sector 1
            // directory starts at sector 1 + (num FAT) * (size FAT)

            rootSec = rgb[16] * (rgb[22] + (rgb[23] << 8)) + 1;
            croot = rgb[17] + (rgb[18] << 8);

            // Data starts at start of root directory + size of root directory

            dataSec = rootSec + (croot*sizeof(MSREC))/cbSec;

#if TRACEDISK
            printf("count of root dir:    %d\n", croot);
            printf("FAT  starts at sector %d\n", cbSec/512);
            printf("DIR  starts at sector %d\n", rootSec);
            printf("Data starts at sector %d\n", dataSec);
#endif

            cluster = *(unsigned short *)&pfd->cAlternateFileName;
            cbFile = pfd->nFileSizeLow;

            while ((cb < cbFile) && cluster && (cluster != clusterMac))
                {
                int clusterOld;

                // read the current cluster's worth of data

                pdi->count = cbClust/512;
                pdi->sec   = dataSec*cbSec/512 + (cluster-2)*cbClust/512;
                pdi->lpBuf = pb;
#if TRACEDISK
            printf("reading cluster %d / sector %d\n", cluster, pdi->sec);
#endif

#if 0
                if (pdi->sec >= 49172)
                    pdi->sec += 2;
#endif

                if (!FRawDiskRWPdi(pdi, 0))
                    break;
        
#if TRACEDISK
            printf("file data = %02X %02X %02X %02X\n",
                pb[0], pb[1], pb[2], pb[3]);
#endif
                cb += cbClust;
                pb += cbClust;

                if (cb >= cbFile)
                    {
                    cb = cbFile;
                    break;
                    }

                // now follow the link in the FAT to the next sector

                pdi->count = 2;
                pdi->sec   = (cbSec/512) + cluster*cnFatEntry/(2*512);
                pdi->lpBuf = rgbFAT;

#if TRACEDISK
                printf("FAT sector # = %d, cluster = %d, cbSec = %d\n", pdi->sec, cluster, cbSec);
#endif
                // only read a new FAT sector if it changed

                if ((pdi->sec != secFAT) && !FRawDiskRWPdi(pdi, 0))
                    break;

#if TRACEDISK
                printf("read in new FAT sector\n");
#endif
                secFAT = pdi->sec;

                clusterOld = cluster;
                cluster = *(unsigned short *)&rgbFAT[(cluster * cnFatEntry / 2) & 511 /*(cbSec-1)*/];

                if (cnFatEntry == 3)
                    {
                    if (clusterOld & 1)
                        cluster >>= 4;

                    cluster &= 0x0FFF;
                    }

#if TRACEDISK
                printf("next cluster = %d\n", cluster);
#endif
                }
            }
        }

    return cb;
}


