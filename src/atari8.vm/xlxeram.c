
/***************************************************************************

    XLXERAM.C

    - Atari XL/XE bank switching

    Copyright (C) 1988-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    09/27/2000  darekm      last update

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

void DumpBlocks(void);


//
// XL/XE Bank flipping bits
//

#define OS_IN       1
#define OS_OUT      0
#define OS_MASK     1

#define BASIC_IN    0
#define BASIC_OUT   2
#define BASIC_MASK  2

#define iBankFromPortB(b) ((((b) & BANK_MASK2) >> 3) | (((b) & BANK_MASK1) >> 2))
#define BANK_MASK   108
#define BANK_MASK1  12
#define BANK_MASK2  96

#define EXTCPU_IN   0
#define EXTCPU_OUT  16
#define EXTCPU_MASK 16

#define EXTVID_IN   0
#define EXTVID_OUT  32
#define EXTVID_MASK 32

#define SELF_IN     0
#define SELF_OUT    128
#define SELF_MASK   128


//
// Cartridge stuff
//

#define CART_8K     0
#define CART_16K    1
#define CART_OSS    2   // Mac65 etc.

typedef struct _cart
{
    char szName[20];
    BYTE FAR *pbData;  // pointer to cart data
    int  cbData;        // actual amount of data on the cart
    int  cPages;        // # of 4K pages of memory footprint
} CART, *PCART;

#define MAX_CART 8


// globals

char FAR rgbSwapSelf[2048];
char FAR rgbSwapCart[16384];
char FAR rgbSwapC000[4096];
char FAR rgbSwapD800[10240];

#if XE
char HUGE rgbXEMem[16][16384];
char FAR rgbSwapXEMem[16384];
#endif // XE

BYTE oldflags;


CART rgcart[MAX_CART];

int  iCartCur;  // current cartridge running
int  iCartMac;  // count of cartridges


void InitBanks()
{
    static BYTE fFirst = TRUE;

    _fmemset(rgbSwapSelf, 0, sizeof(rgbSwapSelf));
    _fmemset(rgbSwapCart, 0, sizeof(rgbSwapCart));
    _fmemset(rgbSwapC000, 0, sizeof(rgbSwapC000));
    _fmemset(rgbSwapD800, 0, sizeof(rgbSwapD800));

    if (fFirst && (mdXLXE == mdXE))
        {
#if XE
        int i;

        for (i = 0; i < 16; i++)
            {
            _fmemset(&rgbXEMem[i], 0, 16384);
            }

        _fmemset(rgbSwapXEMem, 0, 16384);
#endif

        fFirst = FALSE;
        }

#if XE
    else if (mdXLXE == mdXE)
        {
        // swap out any extended memory that may be in

        if ((wPBDATA & EXTCPU_MASK) == EXTCPU_OUT)
            {
            // enable main CPU memory at $4000

            int iBank = iBankFromPortB(wPBDATA);

            _fmemcpy(rgbXEMem[iBank], &rgbMem[0x4000], 16384);
            }
        }
#endif // XE

    // enable OS ROMs

    if (mdXLXE != md800)
        {
        _fmemcpy(&rgbMem[0xC000], rgbXLXEC000, 4096);
        _fmemcpy(&rgbMem[0xD800], rgbXLXED800, 10240);
        }
    else
        {
        _fmemcpy(&rgbMem[0xD800], rgbAtariOSB, 10240);
        }

    // enable BASIC ROMs

    _fmemcpy(&rgbMem[0xA000], rgbXLXEBAS, 8192);
    oldflags = (ramtop == 0xC000) ? 0xFF : 0xFD;

    if ((iCartMac > 1) && (ramtop != 0xC000))
        {
        // swap in the current cartridge

        InitCart();
        }

#if 0
    printf("InitBanks:    ");
    DumpBlocks();
#endif
}


//
// SwapMem(flags)
//
// Routine to swap the Atari ROMs in and out of address space.
// Parameters passed refer to the bits in the XE's PORTB register.
//
// Function maintains a static state of the memory banks.
//

void DumpBlocks()
{
#if 0
    printf("1: %02X %02X %02X 4: %02X %02X %02X %02X 8: %02X %02X %02X %02X C: %02X %02X %02X %02X\n",
        cpuPeekB(0x1000), cpuPeekB(0x2000), cpuPeekB(0x3000),cpuPeekB(0x4000), cpuPeekB(0x5000), cpuPeekB(0x6000), cpuPeekB(0x7000), cpuPeekB(0x8000), cpuPeekB(0x9000), cpuPeekB(0xA000), cpuPeekB(0xB000), cpuPeekB(0xC000), cpuPeekB(0xD000), cpuPeekB(0xE000), cpuPeekB(0xF000));
#endif
}

void __cdecl SwapMem(BYTE xmask, BYTE flags, WORD pc)
{
#ifdef NEWSWAP
    BYTE mask = oldflags ^ flags;
#else
    BYTE oldflags = xmask ^ flags;
    BYTE mask = xmask;
#endif

#if 0
    printf("SwapMem: mask = %02X, flags = %02X, pc = %04X\n", mask&255, flags&255, pc);
    printf("SwapMem entr: ");
    DumpBlocks();
#endif

    if (mask & OS_MASK)
        {
        if ((flags & OS_MASK) == OS_IN)
            {
            // enable OS ROMs

            _fmemcpy(rgbSwapC000, &rgbMem[0xC000], 4096);
            _fmemcpy(rgbSwapD800, &rgbMem[0xD800], 10240);

            if (mdXLXE != md800)
                {
                _fmemcpy(&rgbMem[0xC000], rgbXLXEC000, 4096);
                _fmemcpy(&rgbMem[0xD800], rgbXLXED800, 10240);
                }
            else
                {
                _fmemcpy(&rgbMem[0xD800], rgbAtariOSB, 10240);
                }

            }
        else
            {
            // disable OS ROMs

            _fmemcpy(&rgbMem[0xC000], rgbSwapC000, 4096);
            _fmemcpy(&rgbMem[0xD800], rgbSwapD800, 10240);
            }
        }

    if (mask & BASIC_MASK)
        {
        int cb = rgcart[iCartCur].cbData;

        if ((flags & BASIC_MASK) == BASIC_IN)
            {
            // enable BASIC ROMs


            ramtop = 0xC000 - cb;
            _fmemcpy(rgbSwapCart, &rgbMem[ramtop], cb);
            _fmemcpy(&rgbMem[ramtop], rgbXLXEBAS, cb);
            }
        else
            {
            // disable BASIC ROMs

            ramtop = 0xC000 - cb;
            _fmemcpy(&rgbMem[ramtop], rgbSwapCart, cb);
            ramtop = 0xC000;
            }
        }

    if (mdXLXE == md800)
        {
        oldflags = flags;
        return;
        }

    if ((oldflags & SELF_MASK) == SELF_IN)
        {
        // disable Self Test ROMs

        _fmemcpy(&rgbMem[0x5000], rgbSwapSelf, 2048);
        }

#if XE
    if (mask & EXTCPU_MASK)
        {
        // XE's extended RAM got banked on or off

        if ((flags & EXTCPU_MASK) == EXTCPU_OUT)
            {
            // enable main CPU memory at $4000

            int iBank = iBankFromPortB(oldflags);

            _fmemcpy(rgbXEMem[iBank], &rgbMem[0x4000], 16384);
            _fmemcpy(&rgbMem[0x4000], rgbSwapXEMem,    16384);
            }
        else
            {
            // enable extended CPU memory at $4000

            int iBank = iBankFromPortB(flags);

            _fmemcpy(rgbSwapXEMem,    &rgbMem[0x4000], 16384);
            _fmemcpy(&rgbMem[0x4000], rgbXEMem[iBank], 16384);
            }
        }
    else if (mask & BANK_MASK)
        {
        if ((flags & EXTCPU_MASK) == EXTCPU_IN)
            {
            // XE's extended RAM changed banks

            int iBankNew = iBankFromPortB(flags);
            int iBankOld = iBankFromPortB(oldflags);

            _fmemcpy(rgbXEMem[iBankOld], &rgbMem[0x4000], 16384);
            _fmemcpy(&rgbMem[0x4000], rgbXEMem[iBankNew], 16384);
            }
        }
#endif // XE

    if ((flags & SELF_MASK) == SELF_IN)
        {
        // enable Self Test ROMs

        _fmemcpy(rgbSwapSelf, &rgbMem[0x5000], 2048);
        _fmemcpy(&rgbMem[0x5000], rgbXLXE5000, 2048);
        }

    oldflags = flags;

#if 0
    printf("SwapMem exit: ");
    DumpBlocks();
#endif
}


//
// Helper routines
//

#ifdef DUMPOUTROMS
void DumpROMs()
    {
    int h;

    h = _open("ATARIOSB.ROM", _O_BINARY | _O_RDWR);
    _write(h, rgbAtariOSB, 10240);
    _close(h);

    h = _open("ATARIBAS.ROM", _O_BINARY | _O_RDWR);
    _write(h, rgbXLXEBAS, 8192);
    _close(h);

    h = _open("ATARIXL.ROM", _O_BINARY | _O_RDWR);
    _write(h, rgbXLXEC000, 16384);
    _close(h);
    }
#endif

void ReadROMs()
    {
    int h;

    h = _open("ATARIOSB.ROM", _O_BINARY | _O_RDONLY);
    if (h != NULL)
        {
#ifndef NDEBUG
        printf("reading ATARIOSB.ROM\n");
#endif
        _read(h, rgbAtariOSB, 10240);
        _close(h);
        }

    h = _open("ATARIBAS.ROM", _O_BINARY | _O_RDONLY);
    if (h != NULL)
        {
#ifndef NDEBUG
        printf("reading ATARIBAS.ROM\n");
#endif
        _read(h, rgbXLXEBAS, 8192);
        _close(h);
        }

    // BASIC is a special first cartridge

    strcpy(rgcart[0].szName, "Atari 8K BASIC");
    rgcart[0].pbData = rgbXLXEBAS;
    rgcart[0].cbData = 8192;
    rgcart[0].cPages = 2;
    iCartMac = 1;
    iCartCur = 0;

    h = _open("ATARIXL.ROM", _O_BINARY | _O_RDONLY);
    if (h != NULL)
        {
#ifndef NDEBUG
        printf("reading ATARIXL.ROM\n");
#endif
        _read(h, rgbXLXEC000, 16384);
        _close(h);
        }
    }


void ReadCart(char *pch)
{
    int h;
    int cb = 0;
    long l;

    h = _open(pch, _O_BINARY | _O_RDONLY);
    if (h != NULL)
        {
#ifndef NDEBUG
        printf("reading %s\n", pch);
#endif

        l = _lseek(h, 0L, SEEK_END);

        cb = (unsigned int)l;

//      printf("size of %s is %d bytes\n", pch, cb);

#ifndef HWIN32
        if (rgcart[iCartMac].pbData = malloc(cb))
#else
        if (rgcart[iCartMac].pbData = GlobalAlloc(GMEM_FIXED, cb))
#endif
            {
//          printf("pb = %04X\n", rgcart[iCartMac].pbData);

            _read(h, rgcart[iCartMac].pbData, cb);

            strcpy(rgcart[iCartMac].szName, pch);
            rgcart[iCartMac].cbData = cb;
            rgcart[iCartMac].cPages = cb >> 12;
            iCartMac++;
            iCartCur = 1;
            }
        }
    _close(h);
}


void NextCart()
{
    iCartCur = (iCartCur+1) % iCartMac;
}

void InitCart()
{
    PCART pcart = &rgcart[iCartCur];
    unsigned int cb = pcart->cbData;

//printf("pcart = %08X\n", pcart);
//printf("pb    = %08X\n", pcart->pbData);

    if (ramtop == 0xC000)
        return;

    if (!pcart->pbData)
        return;

    if (cb <= 8192)
        {
        // simple 4K or 8K case

        _fmemcpy(&rgbMem[0xC000 - (pcart->cPages << 12)], pcart->pbData,
             (pcart->cPages << 12));
        return;
        }

    if (cb == 16384)
        {
        // Super cart??

        unsigned int i;

        BYTE FAR *pb = pcart->pbData;

        for (i = 0; i != cb; i+= 4096)
            {
            if ((pb[i+4095] >= 0xA0) && (pb[i+4095] <= 0xBF))
                {
                // This is the 'fixed' bank

                _fmemcpy(&rgbMem[0xB000], pb+i, 4096);
//printf("fixed bank = %04X\n", i);
//gets("  ");
                }

            if (pb[i+4095] == 0x00)
                {
                // This is the default bank

                _fmemcpy(&rgbMem[0xA000], pb+i, 4096);
//printf("default bank = %04X\n", i);
//gets("  ");
                }
            }
        }
}


void BankCart(int iBank)
{
    PCART pcart = &rgcart[iCartCur];
    unsigned int cb = pcart->cbData;

//    printf("bank select = %d\n", iBank);

    if (ramtop == 0xC000)
        return;

    if (!pcart->pbData)
        return;

    if (cb <= 8192)
        {
        return;
        }

    if (cb == 16384)
        {
        // Super cart??

        unsigned int i;

        BYTE FAR *pb = pcart->pbData;

        for (i = 0; i != cb; i+= 4096)
            {
            if (pb[i+4095] == i)
                {
                // This is the selected bank

                _fmemcpy(&rgbMem[0xA000], pb+i, 4096);
                }
            else
                {
                }
            }
        }
}

#else  // !XFORMER

void __cdecl SwapMem(BYTE xmask, BYTE flags, WORD pc)
{
}

#endif // XFORMER

