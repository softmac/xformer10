
/***************************************************************************

    YAMAHA.C

    - Atari ST Yamaha chip routines
    
    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    02/18/1998  darekm      last update

***************************************************************************/

#include "gemtypes.h"

#ifdef ATARIST

//
// Write a byte to the Yamaha chip
//
// Always returns TRUE
//

BOOL FWriteYamaha(int i, int b)
{
    BYTE bChanged;

//    DebugStr("FWriteYamaha: %d, %02X\n", i, b);

    if (i == 0)
        {
        vsthw.iRegSelect = b & 15;
//        DebugStr("FWriteYamaha: register select %d\n", b);
        return TRUE;
        }

    else if (i != 2)
        return FALSE;

    bChanged = vsthw.rgbYamaha[vsthw.iRegSelect] ^ b;
    vsthw.rgbYamaha[vsthw.iRegSelect] = b;

    if (vsthw.iRegSelect == 13)
        {
        // if waveform register gets written to, reset decay values

        vsthw.rgbDecay[0] = 15;
        vsthw.rgbDecay[1] = 15;
        vsthw.rgbDecay[2] = 15;
        }

    if (bChanged == 0)
        return TRUE;

    if (vsthw.iRegSelect == 14)
        {
//        DebugStr("writing to port A: %02X\n", b);
        
        if (bChanged & 0x20)
            {
            // Centronics strobe

            DebugStr("printer port output = %02X\n", vsthw.rgbYamaha[15]);
            if ((b & 0x20) == 0)
                ByteToPrinter(vsthw.rgbYamaha[15]);
            }

        if (bChanged & 0x08)
            {
            DebugStr("RTS output changed to %d\n", b & 0x08);
            SetRTS(b & 0x08);
            }

        if (bChanged & 0x10)
            {
            DebugStr("DTR output changed to %d\n", b & 0x10);
            SetDTR(b & 0x10);
            }

        {
        vsthw.sidereg = (~b) & 1;

        b>>=2;
        vsthw.disksel = (~b) & 1;
        }
        }

    else if (vsthw.iRegSelect == 15)
        {
        // write to printer port

//        DebugStr("writing to printer port: %02X\n", b);
        {
        extern BOOL ByteToPrinter(unsigned char);

//        ByteToPrinter(b);
        }
//        Assert(FALSE);
        }

    else if (vsthw.iRegSelect == 7)
        {
        if ((bChanged & 0x01) && !(b & 0x01))
            vsthw.rgbDecay[0] = 15;

        if ((bChanged & 0x02) && !(b & 0x02))
            vsthw.rgbDecay[1] = 15;

        if ((bChanged & 0x04) && !(b & 0x04))
            vsthw.rgbDecay[2] = 15;
        }
        
    else if (0)
        {
        // one of the sound registers is changing

        void update_voice(int voice, ULONG period, BOOL fTone, BOOL fNoise, BYTE volume);

        update_voice(0, ((vsthw.rgbYamaha[1] & 15) << 8) | vsthw.rgbYamaha[0],    // period
            !(vsthw.rgbYamaha[7] & 0x01),       // voice A on/off
            !(vsthw.rgbYamaha[7] & 0x08),       // noise A on/off
            (vsthw.rgbYamaha[8] & 0x1F));       // voice A volume

        update_voice(1, ((vsthw.rgbYamaha[3] & 15) << 8) | vsthw.rgbYamaha[2],    // period
            !(vsthw.rgbYamaha[7] & 0x02),       // voice B on/off
            !(vsthw.rgbYamaha[7] & 0x10),       // noise B on/off
            (vsthw.rgbYamaha[9] & 0x1F));       // voice B volume

        update_voice(2, ((vsthw.rgbYamaha[5] & 15) << 8) | vsthw.rgbYamaha[4],    // period
            !(vsthw.rgbYamaha[7] & 0x04),       // voice C on/off
            !(vsthw.rgbYamaha[7] & 0x20),       // noise C on/off
            (vsthw.rgbYamaha[10] & 0x1F));      // voice C volume
        }

    if (vsthw.iRegSelect < 14)
        {
        DebugStr("writing %02X to Yahama port %d\n", b, vsthw.iRegSelect);
        }

    return TRUE;
}


void update_voice(int voice, ULONG period, BOOL fTone, BOOL fNoise, BYTE volume)
{
    ULONG frequency;

    period &= 0x0FFF;

    if (!fTone && !fNoise)
        frequency = 0;
    else if (volume == 0)
        frequency = 0;
    else if (period == 0)
        frequency = 0;
    else
        frequency = 125000 / period;

//    DebugStr("voice = %d, frequency = %d\n", voice, frequency);

    if (volume & 16)
        {
        // waveform

        volume = vsthw.rgbDecay[voice];

        if (vsthw.rgbDecay[voice])
            {
            switch(vsthw.rgbYamaha[13] & 15)
                {
            default:
                break;

            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 9:
            case 13:
            case 15:
                if (vsthw.rgbYamaha[12] < 8)
                    vsthw.rgbDecay[voice] -= 2;
                else
                    vsthw.rgbDecay[voice] -= 1;

                if (vsthw.rgbDecay[voice] & 0x80)
                    {
                    vsthw.rgbDecay[voice] = 0;
                    frequency = 0;
                    }
                break;
                }
            }
        }
    else
        {
        // sine wave

        volume &= 15;
        }

    if (fNoise)
        UpdateVoice(voice, vsthw.rgbYamaha[6] << 3, (volume & 16) ? 15 : volume & 15, 8);
    else
        UpdateVoice(voice, frequency, (volume & 16) ? 15 : volume & 15, 10);
//    UpdateVoice(0, 1000, 15, 0);
}


void UpdateSound()
{
    // one of the sound registers is changing

    void update_voice(int voice, ULONG period, BOOL fTone, BOOL fNoise, BYTE volume);

    update_voice(0, ((vsthw.rgbYamaha[1] & 15) << 8) | (vsthw.rgbYamaha[0] & 255),    // period
        !(vsthw.rgbYamaha[7] & 0x01),           // voice A on/off
        !(vsthw.rgbYamaha[7] & 0x08),           // noise A on/off
        (vsthw.rgbYamaha[8] & 0x1F));           // voice A volume

    update_voice(1, ((vsthw.rgbYamaha[3] & 15) << 8) | (vsthw.rgbYamaha[2] & 255),    // period
        !(vsthw.rgbYamaha[7] & 0x02),           // voice B on/off
        !(vsthw.rgbYamaha[7] & 0x10),           // noise B on/off
        (vsthw.rgbYamaha[9] & 0x1F));           // voice B volume

    update_voice(2, ((vsthw.rgbYamaha[5] & 15) << 8) | (vsthw.rgbYamaha[4] & 255),    // period
        !(vsthw.rgbYamaha[7] & 0x04),           // voice C on/off
        !(vsthw.rgbYamaha[7] & 0x20),           // noise C on/off
        (vsthw.rgbYamaha[10] & 0x1F));          // voice C volume
}


//
// Read a byte from the Yamaha chip
//
// High bit of return value is set if error
//

BOOL FReadYamaha()
{
    BYTE b;

    b = vsthw.rgbYamaha[vsthw.iRegSelect];
//    DebugStr("FReadYamaha: returning %02X\n", b);
    return b;
}

#endif // ATARIST

