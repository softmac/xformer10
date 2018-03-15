
/***************************************************************************

    XSB.C

    - Atari 800 sound emulation for Sound Blaster

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    12/20/2000  darekm      last update

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

#ifndef HWIN32

int delay(int x)
{
    int i;

    for (i = 0; i < 10; i++)
        {
        x += i;
        }

    return x * i;
}

void OutSBPort(BYTE reg, BYTE b)
{
#if 0
    static BYTE rgb[256] = { 0 };

    if ((rgb[reg] == b) && (b != 0))
        return;

    rgb[reg] = b;
#endif

#ifdef HDOS16ORDOS32
    delay(reg);
    
    outp(0x388, reg);
    outp(0x388, reg);

    delay(reg);

    outp(0x389, b);
    outp(0x389, b);
#endif
}


void InitSoundBlaster()
{
    BYTE i;

    // reset  WSE/TEST

    OutSBPort(1, 0x00);

    // reset IRQ

    OutSBPort(4, 0x60);
    OutSBPort(4, 0x80);

    // reset CSM/SEL

    OutSBPort(8, 0x00);

    // AM=0, VIB=0, EG=1, KSR=0, MULTI=4

    for (i = 0x20; i < 0x26; i++)
        OutSBPort(i, 0x24);

    OutSBPort(0x28, 0x24);
    OutSBPort(0x2B, 0x24);

    // KSL=3, TLm=FF

    OutSBPort(0x40, 0xFF);
    OutSBPort(0x41, 0xFF);
    OutSBPort(0x42, 0xFF);
    OutSBPort(0x48, 0xFF);

    // KSL=2, TLc=63

    OutSBPort(0x43, 0x7F);
    OutSBPort(0x44, 0x7F);
    OutSBPort(0x45, 0x7F);
    OutSBPort(0x4B, 0x7F);

    // AR/DR set to max

    for (i = 0x60; i < 0x66; i++)
        OutSBPort(i, 0xFF);

    OutSBPort(0x68, 0xFF);
    OutSBPort(0x6B, 0xFF);

    // SL/RR set to min

    for (i = 0x80; i < 0x86; i++)
        OutSBPort(i, 0x00);

    OutSBPort(0x88, 0x00);
    OutSBPort(0x8B, 0x00);

    // AM-VIB/Rhythm=0

    OutSBPort(0xBD, 0x00);

    // FB=0, C=1

    OutSBPort(0xC0, 0x01);
    OutSBPort(0xC1, 0x01);
    OutSBPort(0xC2, 0x01);
    OutSBPort(0xC3, 0x01);
    OutSBPort(0xC8, 0x01);

    // KEY-ON=0

    OutSBPort(0xB0, 0x00);
    OutSBPort(0xB1, 0x00);
    OutSBPort(0xB2, 0x00);
    OutSBPort(0xB3, 0x00);
    OutSBPort(0xB8, 0x00);

#if 0
    OutSBPort(1, 0x20);
    OutSBPort(0xE0, 3);
    OutSBPort(0xE1, 3);
    OutSBPort(0xE2, 3);
    OutSBPort(0xE8, 3);
#endif

    DoSound(0,500,10,0);
    DoSound(1,500,10,0);
    DoSound(2,500,10,0);
    DoSound(3,500,10,0);
}


void DoSound(BYTE voice, WORD freq, BYTE dist, BYTE vol)
{
    if (voice > 2)
        voice = 8;

    if (freq > 20000)
        freq = 0;

    dist &= 14;
#if 1
    if ((dist != 10) && (dist != 14))
        freq = 0;
#endif

    if (freq == 0)
        vol = 0;

    // KSL=2, TLm=60..0

    if (vol == 0)
        {
        OutSBPort(0x43+voice,0x40 | 63);
        return;
        } 

    OutSBPort(0x43+voice,0x40 | 15-1*vol);

    if (voice > 2)
        voice = 3;

    if (freq > 500)
        {
        freq /= 24;

        // KEY-ON=1, BLOCK=7, F-Number(hi)=ch

        OutSBPort(0xB0+voice, 0x3C | ((freq >> 8) & 0x03));  // freq hi

        // F-Number(lo)

        OutSBPort(0xA0+voice, freq);  // freq lo
        }
    else
        {
        freq /= 3;

        // KEY-ON=1, BLOCK=7, F-Number(hi)=ch

        OutSBPort(0xB0+voice, 0x30 | ((freq >> 8) & 0x03));  // freq hi

        // F-Number(lo)

        OutSBPort(0xA0+voice, freq);  // freq lo
        }
}

#endif

#endif // XFORMER

