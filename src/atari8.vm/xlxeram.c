
/***************************************************************************

    XLXERAM.C

    - Atari XL/XE bank switching

    Copyright (C) 1988-2008 by Darek Mihocka. All Rights Reserved.
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


void InitBanks(int iVM)
{
	if (mdXLXE != md800)
	{
		_fmemset(rgbSwapSelf, 0, SELF_SIZE);
		_fmemset(rgbSwapC000, 0, C000_SIZE);
		_fmemset(rgbSwapD800, 0, D800_SIZE);
	}

#if XE

    if (mdXLXE == mdXE)
	{
		iXESwap = -1;	// never swapped yet
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
BOOL __cdecl SwapMem(int iVM, BYTE xmask, BYTE flags)
{
	if (mdXLXE == md800)
		return TRUE;

    BYTE mask = xmask;

    //printf("SwapMem: mask = %02X, flags = %02X, pc = %04X\n", mask&255, flags&255, pc);
    //printf("SwapMem entr: ");
    //DumpBlocks();

	// We are only called if something changes. We will not, for instance, be told to swap in HELP twice in a row.
	// If we were, for instance, we would erase the saved RAM contents there and overwrite it with the HELP ROM contents

	// SELF-TEST gets swapped out if the OS is being swapped out
	if ((mask & OS_MASK) && (flags & OS_MASK) == OS_OUT)
	{
		// it's only safe to do so if HELP is there right now
		if (!(PORTB & 0x80))
		{
			mask |= SELF_MASK;
			flags |= SELF_OUT;
		}
	}

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

	// don't do any BASIC swapping if a cartridge is present. The Sofware will try, but the hardware doesn't allow it
    if ((mask & BASIC_MASK) && !rgvm[iVM].rgcart.fCartIn)
        {
        int cb = 8192;

		// make space for BASIC. If a real cartridge goes in, UnInit will free and alloc a larger space for a real cartridge
		if (!rgbSwapCart[iVM])
			rgbSwapCart[iVM] = malloc(cb);
		if (!rgbSwapCart)
			return FALSE;

        if ((flags & BASIC_MASK) == BASIC_IN)
            {
            // enable BASIC ROMs
            ramtop = 0xC000 - (WORD)cb;
            _fmemcpy(rgbSwapCart[iVM], &rgbMem[ramtop], cb);
            _fmemcpy(&rgbMem[ramtop], rgbXLXEBAS, cb);
            }
        else
            {
            // disable BASIC ROMs

            // make sure rgbSwapCart is initialized with the cartridge image!
			ramtop = 0xC000 - (WORD)cb;
            _fmemcpy(&rgbMem[ramtop], rgbSwapCart[iVM], cb);
            ramtop = 0xC000;
            }
        }

	if (mask & SELF_MASK)
	{
		if ((flags & SELF_MASK) == SELF_IN)
		{
			// enable Self Test ROMs
			_fmemcpy(rgbSwapSelf, &rgbMem[0x5000], SELF_SIZE);
			_fmemcpy(&rgbMem[0x5000], rgbXLXE5000, SELF_SIZE);
		}
		else
		{
			// disable Self Test ROMs
			_fmemcpy(&rgbMem[0x5000], rgbSwapSelf, SELF_SIZE);
		}
	}

#if XE

	// !!! ANTIC reading from a different bank than the CPU is not supported yet!

	if (mask & EXTCPU_MASK)
    {
		// XE's extended RAM got banked on or off

		// enable main CPU memory at $4000
		if ((flags & EXTCPU_MASK) == EXTCPU_OUT)
		{
			// we have swapped it out, so put it back. Otherwise, don't overwrite it with garbage
			if (iXESwap >= 0)
			{
				char tmp[XE_SIZE];
				_fmemcpy(tmp, &rgbMem[0x4000], XE_SIZE);
				_fmemcpy(&rgbMem[0x4000], rgbXEMem + iXESwap * XE_SIZE, XE_SIZE);
				_fmemcpy(rgbXEMem + iXESwap * XE_SIZE, tmp, XE_SIZE);
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
					char tmp[XE_SIZE];
					_fmemcpy(tmp, &rgbMem[0x4000], XE_SIZE);
					_fmemcpy(&rgbMem[0x4000], rgbXEMem + iXESwap * XE_SIZE, XE_SIZE);
					_fmemcpy(rgbXEMem + iXESwap * XE_SIZE, tmp, XE_SIZE);
					iXESwap = -1;
				}

				// now do the swap we want
				char tmp[XE_SIZE];
				_fmemcpy(tmp, &rgbMem[0x4000], XE_SIZE);
				_fmemcpy(&rgbMem[0x4000], rgbXEMem + iBank * XE_SIZE, XE_SIZE);
				_fmemcpy(rgbXEMem + iBank * XE_SIZE, tmp, XE_SIZE);
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
					char tmp[XE_SIZE];
					_fmemcpy(tmp, &rgbMem[0x4000], XE_SIZE);
					_fmemcpy(&rgbMem[0x4000], rgbXEMem + iXESwap * XE_SIZE, XE_SIZE);
					_fmemcpy(rgbXEMem + iXESwap * XE_SIZE, tmp, XE_SIZE);
					iXESwap = -1;
				}

				// now do the swap we want
				char tmp[XE_SIZE];
				_fmemcpy(tmp, &rgbMem[0x4000], XE_SIZE);
				_fmemcpy(&rgbMem[0x4000], rgbXEMem + iBank * XE_SIZE, XE_SIZE);
				_fmemcpy(rgbXEMem + iBank * XE_SIZE, tmp, XE_SIZE);
				iXESwap = iBank;
			}
		}
	}
#endif // XE

#if 0
	//    oldflags = flags;
	printf("SwapMem exit: ");
    DumpBlocks();
#endif
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
			_read(h, rgbAtariOSB, 10240);	// one copy for all instances
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
			_read(h, rgBasicData, 8192);	// one copy for all instances
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
			_read(h, rgbXLXEC000, 16384);	// one copy for all instances
			_close(h);
		}
	}
}
#endif

#endif // XFORMER
