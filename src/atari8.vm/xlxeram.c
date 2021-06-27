
/***************************************************************************

    XLXERAM.C

    - Atari XL/XE bank switching

    Copyright (C) 1988-2021 by Darek Mihocka and Danny Miller. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    09/27/2000  darekm      last update

***************************************************************************/
#include <sys/stat.h>
#include "atari800.h"

#ifdef XFORMER

#ifndef NDEBUG
void DumpBlocks()
{
#if 0
    printf("1: %02X %02X %02X 4: %02X %02X %02X %02X 8: %02X %02X %02X %02X C: %02X %02X %02X %02X\n",
        cpuPeekB(0x1000), cpuPeekB(0x2000), cpuPeekB(0x3000), cpuPeekB(0x4000), cpuPeekB(0x5000), cpuPeekB(0x6000), cpuPeekB(0x7000), cpuPeekB(0x8000), cpuPeekB(0x9000), cpuPeekB(0xA000), cpuPeekB(0xB000), cpuPeekB(0xC000), cpuPeekB(0xD000), cpuPeekB(0xE000), cpuPeekB(0xF000));
#endif
}
#endif

//
// XL/XE Bank flipping bits
//

#define OS_IN       1
#define OS_OUT      0
#define OS_MASK     1

#define BASIC_IN    0
#define BASIC_OUT   2
#define BASIC_MASK  2

#define iBankFromPortB(b) (/* (((b) & BANK_MASK2) >> 3) | */ (((b) & BANK_MASK1) >> 2))
#define BANK_MASK   108
#define BANK_MASK1  12

#define EXTCPU_IN   0
#define EXTCPU_OUT  16
#define EXTCPU_MASK 16

#define EXTVID_IN   0
#define EXTVID_OUT  32
#define EXTVID_MASK 32

#define SELF_IN     0
#define SELF_OUT    128
#define SELF_MASK   128

//BYTE oldflags;


void InitBanks(void *candy)
{
    if (mdXLXE != md800)
    {
        _fmemset(rgbSwapSelf, 0, SELF_SIZE);
        _fmemset(rgbSwapC000, 0, C000_SIZE);
        _fmemset(rgbSwapD800, 0, D800_SIZE);
        _fmemset(rgbSwapBASIC, 0, BASIC_SIZE);
    }

#if XE

    if (mdXLXE == mdXE)
    {
        iXESwap = -1;    // never swapped yet
        _fmemset(rgbXEMem, 0, XE_SIZE * 4);
    }
#endif

    // enable OS ROMs

    if (mdXLXE != md800)
        {
        _fmemcpy(&rgbMem[0xC000], rgbXLXEC000, C000_SIZE);
        _fmemcpy(&rgbMem[0xD800], rgbXLXED800, D800_SIZE);
        }
    else
        {
        _fmemcpy(&rgbMem[0xD800], rgbAtariOSB, 10240);
        }

    //printf("InitBanks:    ");
    //DumpBlocks();
}


//
// SwapMem(flags)
//
// Routine to swap the Atari ROMs in and out of address space.
// Parameters passed refer to the bits in the XE's PORTB register.
//
BOOL SwapMem(void *candy, BYTE xmask, BYTE flags)
{   
    if (mdXLXE == md800)
        return TRUE;

    BYTE mask = xmask;

    //printf("SwapMem: mask = %02X, flags = %02X, pc = %04X\n", mask&255, flags&255, pc);
    //printf("SwapMem entr: ");
    //DumpBlocks();

    // We are only called if something changes. We will not, for instance, be told to swap in HELP twice in a row.
    // If we were, for instance, we would erase the saved RAM contents there and overwrite it with the HELP ROM contents

#if 0   // old and broken, we now handle this before calling us
    // SELF-TEST gets swapped out if the OS is being swapped out
    if ((mask & OS_MASK) && (flags & OS_MASK) == OS_OUT)
    {
        // it's only safe to do so if HELP is there right now
        if (!(PORTB & 0x80))
        {
            mask |= SELF_MASK;
            flags |= SELF_OUT;
        }

        // broken, must also avoid swapping HELP in
    }
#endif

    if (mask & OS_MASK)
    {
        if ((flags & OS_MASK) == OS_IN)
        {
            // enable OS ROMs
            _fmemcpy(rgbSwapC000, &rgbMem[0xC000], C000_SIZE);
            _fmemcpy(rgbSwapD800, &rgbMem[0xD800], D800_SIZE);
            _fmemcpy(&rgbMem[0xC000], rgbXLXEC000, C000_SIZE);
            _fmemcpy(&rgbMem[0xD800], rgbXLXED800, D800_SIZE);
        }
        else
        {
            // disable OS ROMs
            _fmemcpy(&rgbMem[0xC000], rgbSwapC000, C000_SIZE);
            _fmemcpy(&rgbMem[0xD800], rgbSwapD800, D800_SIZE);
        }
    }

    // Don't do any BASIC swapping if a cartridge is present. The Sofware will try, but the hardware doesn't allow it
    // But remember, some fancy cartridges can swap themselves out to RAM and then BASIC *can* be swapped in.
    // OK to swap in BASIC if ramtop is high. OK to swap out BASIC if there's no other cartridge, or that cartridge's
    // current bank is maxed out, meaning RAM.
    // !!! What happens if somebody tries to bank a cartridge in when BASIC is already in? I won't allow that.
    if ((mask & BASIC_MASK) && (ramtop == 0xc000 || !pvm->rgcart.fCartIn || (iSwapCart == iNumBanks && iSwapCart > 0)))
    {
        int cb = BASIC_SIZE;

        if ((flags & BASIC_MASK) == BASIC_IN)
        {
            // enable BASIC ROMs
            ramtop = 0xC000 - (WORD)cb;
            _fmemcpy(rgbSwapBASIC, &rgbMem[ramtop], cb);
            _fmemcpy(&rgbMem[ramtop], rgbXLXEBAS, cb);
        }
        else
        {
            // disable BASIC ROMs

            ramtop = 0xC000 - (WORD)cb;
            _fmemcpy(&rgbMem[ramtop], rgbSwapBASIC, cb);
            ramtop = 0xC000;
        }
    }

    // HELP and the XE banks conflict! HELP wins and sits on top no matter what XE bank is there.
    // But GET IT OUT OF THE WAY before doing any XE flipping or you'll corrupt the XE bank
    // And if it belongs in, we have to do that LAST.

    // HELP is currently IN if we're swapping HELP and it wants to go out, or if it's staying the same and wants to go in
    // If so, swap it out.
    if (((mask & SELF_MASK) && (flags & SELF_OUT)) || (!(mask & SELF_MASK) && !(flags & SELF_OUT)))
    {
        // disable Self Test ROMs
        _fmemcpy(&rgbMem[0x5000], rgbSwapSelf, SELF_SIZE);
    }

#if XE

    // If we're not swapping the CPU and ANTIC together
    BYTE m = flags & (EXTVID_MASK | EXTCPU_MASK);
    if (m && m != (EXTVID_MASK | EXTCPU_MASK))
    {
        // ANTIC will need to read from the bank # we are being passed, the CPU will read from rgbMem.
        // Add one to make it 1-based instead of 0-based, because 0 means ANTIC is looking in the same place as the CPU
        ANTICBankDifferent = iBankFromPortB(flags) + 1;
        //ODS("ANTIC DIFFERENT THAN CPU! %02x\n", flags);
    }
    else if (ANTICBankDifferent)
    {
        ANTICBankDifferent = FALSE;
        //ODS("ANTIC now the same as CPU %02x\n", flags);
    }

    if (mask & EXTCPU_MASK)
    {
        // XE's extended RAM got banked on or off

        // enable main CPU memory at $4000
        if ((flags & EXTCPU_MASK) == EXTCPU_OUT)
        {
            // we have swapped it out, so put it back. Otherwise, don't overwrite it with garbage
            if (iXESwap >= 0)
            {
                SwapHelper(candy, &rgbMem[0x4000], rgbXEMem + iXESwap * XE_SIZE, XE_SIZE);
                iXESwap = -1;
            }
        }

        // enable extended CPU memory at $4000
        else
        {
            int iBank = iBankFromPortB(flags);

            // if it's not already there
            if (iXESwap != iBank)
            {
                // swap the current one back to where it belongs
                // we are called such that code won't execute, this case is caught be the next if
                if (iXESwap != -1)
                {
                    SwapHelper(candy, &rgbMem[0x4000], rgbXEMem + iXESwap * XE_SIZE, XE_SIZE);
                    iXESwap = -1;
                }

                // now do the swap we want
                SwapHelper(candy, &rgbMem[0x4000], rgbXEMem + iBank * XE_SIZE, XE_SIZE);
                iXESwap = iBank;
            }
        }
    }

    else if (mask & BANK_MASK)
    {
        if ((flags & EXTCPU_MASK) == EXTCPU_IN)
        {
            // XE's extended RAM changed banks

            //int oldflags = mask ^ flags;
            //int iBankOld = iBankFromPortB(oldflags);

            int iBank = iBankFromPortB(flags);

            // if it's not already there
            if (iXESwap != iBank)
            {
                // swap the current one back to where it belongs
                if (iXESwap != -1)
                {
                    SwapHelper(candy, &rgbMem[0x4000], rgbXEMem + iXESwap * XE_SIZE, XE_SIZE);
                    iXESwap = -1;
                }

                // now do the swap we want
                SwapHelper(candy, &rgbMem[0x4000], rgbXEMem + iBank * XE_SIZE, XE_SIZE);
                iXESwap = iBank;
            }
        }
    }
#endif // XE

    // Lastly, HELP might go in, because it wins over top of a conflicting XE bank.
    // It goes in if it wants to go in now, or if it was already in.
    if (((mask & SELF_MASK) && !(flags & SELF_OUT)) || (!(mask & SELF_MASK) && !(flags & SELF_OUT)))
    {
        // enable Self Test ROMs
        // !!! THIS SHOULD BE READ ONLY
        _fmemcpy(rgbSwapSelf, &rgbMem[0x5000], SELF_SIZE);
        _fmemcpy(&rgbMem[0x5000], rgbXLXE5000, SELF_SIZE);
    }

    return TRUE;
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

#if 0
void ReadROMs()
{
    int h;

    // 800 needs OS B
    if (mdXLXE == md800)
    {
        h = _open("ATARIOSB.ROM", _O_BINARY | _O_RDONLY);
        if (h != NULL)
        {
#ifndef NDEBUG
            printf("reading ATARIOSB.ROM\n");
#endif
            _read(h, rgbAtariOSB, 10240);    // one copy for all instances
            _close(h);
        }
    }

    // XL needs OS XL and built in BASIC (treat it like a special cartridge)
    if (mdXLXE != md800)
    {
        h = _open("ATARIBAS.ROM", _O_BINARY | _O_RDONLY);
        if (h != NULL)
        {
#ifndef NDEBUG
            printf("reading ATARIBAS.ROM\n");
#endif
            _read(h, rgBasicData, 8192);    // one copy for all instances
            _close(h);
        }

        strcpy(rgBasic.szName, "Atari 8K BASIC");
        rgBasic.cbData = 8192;

        h = _open("ATARIXL.ROM", _O_BINARY | _O_RDONLY);
        if (h != NULL)
        {
#ifndef NDEBUG
            printf("reading ATARIXL.ROM\n");
#endif
            _read(h, rgbXLXEC000, 16384);    // one copy for all instances
            _close(h);
        }
    }
}
#endif

#endif // XFORMER
