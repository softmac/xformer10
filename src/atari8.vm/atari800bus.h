/***************************************************************************

    ATARI800BUS.H

    - The 6502 memory bus: PeekBAtari/PokeBAtari switch on the address and
      hand register, cartridge and ROM accesses to the slow paths in atari800.c.

    MSVC defines these in atari800.c (which includes this file) and inlines
    them into x6502.c through __forceinline and the Assert(pfnPeekB ==
    PeekBAtari) hint there. Other compilers have no such hint (Assert is a
    no-op under LINUX_PORT), so atari800.h includes this file instead and
    every translation unit gets its own inline copy; x6502.c then calls them
    directly rather than through pfnPeekB/pfnPokeB.

    Copyright (C) 1986-2021 by Darek Mihocka and Danny Miller. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.

***************************************************************************/

#pragma once

// the non-jump table version of the code just switches the proper routine
BYTE BUSDISPATCH __fastcall PeekBAtari(void *candy, ADDR addr)
{
    switch (addr >> 8)
    {
    default:
        return cpuPeekB(candy, addr);

    case 0xc0:
    case 0xd0:
    case 0xd2:
    case 0xd3:
    case 0xd4:
        return PeekBAtariHW(candy, addr);

    case 0xd5:
        return PeekBAtariBS(candy, addr);

    case 0x8f:
    case 0x9f:
        if (bCartType == CART_BOB)
            return PeekBAtariBB(candy, addr);
        return cpuPeekB(candy, addr);
    }
}

// the non-jump table version just switches to the right function
BOOL BUSDISPATCH __fastcall PokeBAtari(void *candy, ADDR addr, BYTE b)
{
    BYTE ba = (BYTE)(addr >> 8);
    switch (ba)
    {
    default:
        if (addr < ramtop)
        {
            // writes to screen memory being drawn right now need to be trapped (Turmoil)
            if (addr >= wAddr && addr < (WORD)(wAddr + cbWidth) && rgbMem[addr] != b)
                ProcessScanLine(candy);
            return cpuPokeB(candy, addr, b);
        }
        else if (bCartType == CART_BOB && (ba == 0x8f || ba == 0x9f))
            return PokeBAtariBB(candy, addr, b);
        else
            return TRUE;

    case 0xc0:
    case 0xc1:
    case 0xc2:
    case 0xc3:
    case 0xc4:
    case 0xc5:
    case 0xc6:
    case 0xc7:
    case 0xc8:
    case 0xc9:
    case 0xca:
    case 0xcb:
    case 0xcc:
    case 0xcd:
    case 0xce:
    case 0xcf:
        return PokeBAtariOS(candy, addr, b);

    case 0xd0:
    case 0xd1:
    case 0xd2:
    case 0xd3:
    case 0xd4:
        return PokeBAtariHW(candy, addr, b);

    case 0xd5:
        return PokeBAtariBS(candy, addr, b);

    case 0xd6:
    case 0xd7:
        return PokeBAtariHW(candy, addr, b);  // was TRUE; $d680 binary loader hack

    case 0xd8:
    case 0xd9:
    case 0xda:
    case 0xdb:
    case 0xdc:
    case 0xdd:
    case 0xde:
    case 0xdf:
    case 0xe0:
    case 0xe1:
    case 0xe2:
    case 0xe3:
    case 0xe4:
    case 0xe5:
    case 0xe6:
    case 0xe7:
    case 0xe8:
    case 0xe9:
    case 0xea:
    case 0xeb:
    case 0xec:
    case 0xed:
    case 0xee:
    case 0xef:
    case 0xf0:
    case 0xf1:
    case 0xf2:
    case 0xf3:
    case 0xf4:
    case 0xf5:
    case 0xf6:
    case 0xf7:
    case 0xf8:
    case 0xf9:
    case 0xfa:
    case 0xfb:
    case 0xfc:
    case 0xfd:
    case 0xfe:
    case 0xff:
        return PokeBAtariOS(candy, addr, b);
    }
}
