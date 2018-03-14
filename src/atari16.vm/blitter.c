
/***************************************************************************

    BLITTER.C

    - Atari ST blitter emulation
    
    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    08/08/2007  darekm      last update

***************************************************************************/

#include "gemtypes.h"

#ifdef ATARIST

#pragma pack(2)

typedef struct _blitregs
{
    const WORD fill[16];
    const SHORT sxinc;
    const SHORT syinc;
    ULONG saddr;
    const WORD endm1;
    const WORD endm2;
    const WORD endm3;
    const SHORT dxinc;
    const SHORT dyinc;
    ULONG daddr;
    const WORD xcount;
    WORD ycount;
    const BYTE op;    // byte swapped
    const BYTE hop;
    const BYTE skew;    // byte swapped
    BYTE reg1;
} BLITREGS;

#pragma pack()

#define bfFXSR 0x80
#define bfNFSR 0x40


//
// DoBlitter(pbr)
//
// Handles a blit operation. pbr points to the _blitter structure in
// sthw.asm. All words are already byte swapped.
//
// NOTE: saddr and daddr must first be WORD swapped.
//

void DoBlitter(BLITREGS *pbr)
{
    ULONG saddr = ((pbr->saddr << 16) | (pbr->saddr >> 16)) & 0x00FFFFFE;
    ULONG daddr = ((pbr->daddr << 16) | (pbr->daddr >> 16)) & 0x00FFFFFE;
    ULONG x, y;
    const ULONG xMac = pbr->xcount;
    const ULONG yMac = pbr->ycount;
    ULONG w, wMask;
    ULONG l;
    ULONG line = pbr->reg1 & 15;
    const ULONG daddr0 = daddr;

#ifndef NDEBUG
//    DebugStr("Blitter: saddr:%06X daddr:%06X xMac:%04X yMac:%04X sxinc:%04X dxinc:%04X syinc:%04X dyinc:%04X masks: %04X %04X %04X hop:%d op:%1X skew:%02X\n",
//        saddr, daddr, xMac, yMac, (WORD)pbr->sxinc, (WORD)pbr->dxinc, (WORD)pbr->syinc, (WORD)pbr->dyinc, pbr->endm1, pbr->endm2, pbr->endm3, pbr->hop, pbr->op, pbr->skew);
#endif

    if (!yMac || !xMac)
        {
        pbr->reg1 &= 0x7F;
        return;
        }

#if 0
    pbr->sxinc &= ~1;
    pbr->dxinc &= ~1;
    pbr->syinc &= ~1;
    pbr->dyinc &= ~1;
#endif

    // check for special case blits

    if ((pbr->skew == 0) && (pbr->dxinc >= 0) && (xMac > 2))
        {
        // no shifting or backwards blits

        if ((pbr->hop == 1) && (pbr->op == 3))
            {
            // fill memory with halftone

            for (y = 0; y < yMac; y++)
                {
                w = pbr->fill[line];
                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));
                daddr += pbr->dxinc;

                for (x = 0; x < xMac-2; x++)
                    {
                    vmPokeW(daddr, w);
                    daddr += pbr->dxinc;
                    }

                wMask = pbr->endm3;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));

                saddr += pbr->sxinc * (xMac-1);
                saddr += pbr->syinc;
                daddr += pbr->dyinc;

                line += (pbr->dyinc & 0x8000) ? -1 : 1;
                line &= 15;
                }

            goto done;
            }

        if ((pbr->hop == 1) && (pbr->op == 6))
            {
            // XOR memory with halftone

            for (y = 0; y < yMac; y++)
                {
                w = pbr->fill[line];

                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) ^ (vmPeekW(daddr)));
                daddr += pbr->dxinc;

                for (x = 0; x < xMac-2; x++)
                    {
                    vmPokeW(daddr, vmPeekW(daddr) ^ w);
                    daddr += pbr->dxinc;
                    }

                wMask = pbr->endm3;
                vmPokeW(daddr, (w & wMask) ^ (vmPeekW(daddr)));

                saddr += pbr->sxinc * (xMac-1);
                saddr += pbr->syinc;
                daddr += pbr->dyinc;

                line += (pbr->dyinc & 0x8000) ? -1 : 1;
                line &= 15;
                }

            goto done;
            }

        if ((pbr->hop == 1) && (pbr->op == 7))
            {
            // OR memory with halftone

            for (y = 0; y < yMac; y++)
                {
                w = pbr->fill[line];

                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr)));
                daddr += pbr->dxinc;

                for (x = 0; x < xMac-2; x++)
                    {
                    vmPokeW(daddr, vmPeekW(daddr) | w);
                    daddr += pbr->dxinc;
                    }

                wMask = pbr->endm3;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr)));

                saddr += pbr->sxinc * (xMac-1);
                saddr += pbr->syinc;
                daddr += pbr->dyinc;

                line += (pbr->dyinc & 0x8000) ? -1 : 1;
                line &= 15;
                }

            goto done;
            }

        if ((pbr->hop == 2) && (pbr->op == 3))
            {
            // straight memory to memory blit!

            for (y = 0; y < yMac; y++)
                {
                w = vmPeekW(saddr);
                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));
                saddr += pbr->sxinc;
                daddr += pbr->dxinc;

                for (x = 0; x < xMac-2; x++)
                    {
                    vmPokeW(daddr, vmPeekW(saddr));
                    saddr += pbr->sxinc;
                    daddr += pbr->dxinc;
                    }

                w = vmPeekW(saddr);
                wMask = pbr->endm3;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));

                saddr += pbr->syinc;
                daddr += pbr->dyinc;
                }

            goto done;
            }

        if ((pbr->hop == 2) && (pbr->op == 0))
            {
            // fill memory with 0

            for (y = 0; y < yMac; y++)
                {
                wMask = pbr->endm1;
                vmPokeW(daddr, (vmPeekW(daddr) & ~wMask));
                daddr += pbr->dxinc;

                for (x = 0; x < xMac-2; x++)
                    {
                    vmPokeW(daddr, 0);
                    daddr += pbr->dxinc;
                    }

                w = vmPeekW(saddr);
                wMask = pbr->endm3;
                vmPokeW(daddr, (vmPeekW(daddr) & ~wMask));

                saddr += pbr->sxinc * (xMac-1);
                saddr += pbr->syinc;
                daddr += pbr->dyinc;
                }

            goto done;
            }
        }

    else if ((pbr->skew == 0) && (pbr->dxinc >= 0) && (xMac == 1))
        {
        if ((pbr->hop == 1) && (pbr->op == 6))
            {
            // XOR memory with halftone, used by vertical lines

            for (y = 0; y < yMac; y++)
                {
                w = pbr->fill[line];

                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) ^ (vmPeekW(daddr)));

                saddr += pbr->syinc;
                daddr += pbr->dyinc;

                line += (pbr->dyinc & 0x8000) ? -1 : 1;
                line &= 15;
                }

            goto done;
            }

        if ((pbr->hop == 1) && (pbr->op == 3))
            {
            // fill memory with halftone

            for (y = 0; y < yMac; y++)
                {
                w = pbr->fill[line];
                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));

                saddr += pbr->syinc;
                daddr += pbr->dyinc;

                line += (pbr->dyinc & 0x8000) ? -1 : 1;
                line &= 15;
                }

            goto done;
            }

        if ((pbr->hop == 2) && (pbr->op == 3))
            {
            // straight memory to memory blit!

            for (y = 0; y < yMac; y++)
                {
                w = vmPeekW(saddr);
                wMask = pbr->endm1;
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));

                saddr += pbr->syinc;
                daddr += pbr->dyinc;
                }

            goto done;
            }
        }
    else if ((pbr->skew == 1) && (pbr->dxinc >= 0) && (xMac == 1))
        {
        if ((pbr->hop == 2) && (pbr->op == 3))
            {
            // memory to memory blit with OR operation and 1 pixel shift!
            // used by v_gtext in monochrome

            for (y = 0; y < yMac; y++)
                {
                w = vmPeekW(saddr);
                wMask = pbr->endm1;
                vmPokeW(daddr, ((w>>1) & wMask) | (vmPeekW(daddr) & ~wMask));

                saddr += pbr->syinc;
                daddr += pbr->dyinc;
                }

            goto done;
            }

        if ((pbr->hop == 2) && (pbr->op == 7))
            {
            // memory to memory blit with OR operation and 1 pixel shift!
            // used by v_gtext in monochrome

            for (y = 0; y < yMac; y++)
                {
                w = vmPeekW(saddr);
                wMask = pbr->endm1;
                vmPokeW(daddr, ((w>>1) & wMask) | vmPeekW(daddr));

                saddr += pbr->syinc;
                daddr += pbr->dyinc;
                }

            goto done;
            }
        }


#ifndef NDEBUG
    DebugStr("Blitter: saddr:%06X daddr:%06X xMac:%04X yMac:%04X sxinc:%04X dxinc:%04X syinc:%04X dyinc:%04X masks: %04X %04X %04X hop:%d op:%1X skew:%02X\n",
        saddr, daddr, xMac, yMac, (WORD)pbr->sxinc, (WORD)pbr->dxinc, (WORD)pbr->syinc, (WORD)pbr->dyinc, pbr->endm1, pbr->endm2, pbr->endm3, pbr->hop, pbr->op, pbr->skew);
#endif

    for (y = 0; y < yMac; y++)
        {
        // preload shift register

        if ((pbr->hop & 3) >= 2)
            {
            if (pbr->sxinc < 0)
                l = vmPeekW(saddr) << 16;
            else
                l = vmPeekW(saddr);
            }

        if (pbr->skew & bfFXSR)
            {
            // Force eXtra Source Read

            saddr += pbr->sxinc;
            }

        for (x = 0, wMask = pbr->endm1; x < xMac; x++)
            {
            // if either saddr or daddr is not pointing to RAM, just exit

            if (daddr >= vi.cbRAM[0])
                {
                goto Lnext;
                }

            if ((pbr->hop & 3) < 2)
                {
                l = 0;
                }
            else
                {
                // read source

                if (pbr->sxinc < 0)
                    l = (l >> 16) | (vmPeekW(saddr) << 16);
                else
                    l = (l << 16) | vmPeekW(saddr);
                }

            switch(pbr->hop & 3)
                {
            default:
            case 0:        // all 1 bits
                w = (WORD)~0;
                break;

            case 1:        // half tone RAM only
                w = pbr->fill[line];
                break;

            case 2:        // source only
                w = (WORD)(l >> (pbr->skew & 15));
                break;

            case 3:        // source AND half tone
                w = (WORD)(l >> (pbr->skew & 15));
                w &= pbr->fill[line];
                break;
                }

            // DebugStr("%04X ", w); fflush(stdout);

            switch(pbr->op & 15)
                {
            default:
            case 0:        // all 0 bits
                w = 0;
                break;

            case 1:
                w = vmPeekW(daddr) & w;
                break;

            case 2:
                w = ~vmPeekW(daddr) & w;
                break;

            case 3:            // source
                w = w;
                break;

            case 4:
                w = vmPeekW(daddr) & ~w;
                break;

            case 5:            // dest
                w = vmPeekW(daddr);
                break;

            case 6:            // XOR
                w = vmPeekW(daddr) ^ w;
                break;

            case 7:            // OR
                w = vmPeekW(daddr) | w;
                break;

            case 8:
                w = ~vmPeekW(daddr) & ~w;
                break;

            case 9:
                w = vmPeekW(daddr) ^ ~w;
                break;

            case 10:
                w = ~vmPeekW(daddr);
                break;

            case 11:
                w = ~vmPeekW(daddr) | w;
                break;

            case 12:
                w = ~w;
                break;

            case 13:
                w = vmPeekW(daddr) | ~w;
                break;

            case 14:
                w = ~vmPeekW(daddr) | ~w;
                break;

            case 15:
                w = (WORD)~0;
                break;
                }

            // calc destination mask

            if ((xMac >= 2) && (x > 0))
                {
                wMask = pbr->endm2;

                if (x == (xMac-1))
                    wMask = pbr->endm3;
                }

            if (~wMask)
                vmPokeW(daddr, (w & wMask) | (vmPeekW(daddr) & ~wMask));
            else
                vmPokeW(daddr, w);
Lnext:
            saddr += pbr->sxinc;
            daddr += pbr->dxinc;
            }

        if (pbr->skew & bfNFSR)
            {
            // No Final Source Read

            saddr -= pbr->sxinc;
            }

        // after last word of scan line, don't increment by XINC, use YINC

        saddr -= pbr->sxinc;
        daddr -= pbr->dxinc;

        saddr += pbr->syinc;
        daddr += pbr->dyinc;

        // increment/decrement line count depending on sign of YINC

        line += (pbr->dyinc & 0x8000) ? -1 : 1;
        line &= 15;
        }

done:
    pbr->ycount = 0;
    pbr->saddr = (saddr << 16) | (saddr >> 16);
    pbr->daddr = (daddr << 16) | (daddr >> 16);
    pbr->reg1  = (pbr->reg1 & 0x70) | (BYTE)line;
}

#endif // ATARIST

