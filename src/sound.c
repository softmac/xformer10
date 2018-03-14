
/****************************************************************************

    SOUND.C

    - Mono and stereo sound routines

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    08/24/2001  darekm      Update for SoftMac XP

****************************************************************************/

#include "gemtypes.h" // main include file 

#define SAMPLE_RATE 22050
int SAMPLES_PER_VOICE;

#define CNT_ANGLES 64

CHAR const mpxsine[CNT_ANGLES+1] =
{
      0,  13,  25,  38,  49,  60,  70,  80,  90, 98, 106, 112, 117, 121, 125, 126, 127, 126, 125, 121, 117, 112, 106, 98,  90,  80, 70, 60, 49, 38, 25, 13,
      0, -13, -25, -38, -49, -60, -70, -80, -90,-98,-106,-112,-117,-121,-125,-126,-127,-126,-125,-121,-117,-112,-106,-98, -90, -80,-70,-60,-49,-38,-25,-13,
      0,
};

#define SNDBUFS     12

//
// convert a phase angle (0..65535) to a sine (-127..+127)

__inline int SineOfPhase(ULONG x)
{
    int i, w;

    return ((mpxsine[(x >> 16) & (CNT_ANGLES-1)]));

//    return mpxsine[(((x) * CNT_ANGLES) >> 16) & (CNT_ANGLES-1)];

    i = (((x) * CNT_ANGLES) >> 16) & (CNT_ANGLES-1);
    w = ((x) * CNT_ANGLES & 65535) >> 14;

    return ((mpxsine[i] * (15-w)) + (mpxsine[i+1] * w)) / 2;
}


//
// convert phase angle into square wave
//

__inline int SqrOfPhase(ULONG x)
{
    return ((x >> 22) & 1) ? 127 : -127;
}


HWAVEOUT hWave;
PCMWAVEFORMAT pcmwf;

//
// defines a voice (frequency, volume, distortion, and current phase)
//

typedef struct
{
    ULONG frequency;           
    ULONG volume;        // 0..15
    ULONG distortion;
    ULONG phase;
} VOICE;

VOICE rgvoice[4];


void UpdateVoice(int iVoice, ULONG new_frequency, ULONG new_volume, BOOL new_distortion)
{
    if (new_frequency < 30)
        new_frequency = 0;
    else if (new_frequency > 20000)
        new_frequency = 0;
    else if (new_frequency > 11000)
        new_frequency = 11000;

    rgvoice[iVoice].frequency  = (65536 * new_frequency / SAMPLE_RATE) * CNT_ANGLES;
    rgvoice[iVoice].volume     = new_volume;
    rgvoice[iVoice].distortion = new_distortion;
}


void SoundDoneCallback()
{
    WAVEHDR *pwhdr = NULL;
    signed char *pb;
    int voice;
    int iHdr;

    for (iHdr = 0; iHdr < SNDBUFS; iHdr++)
        {
        pwhdr = &vi.rgwhdr[iHdr];

        if (vi.rgwhdr[iHdr].dwFlags & WHDR_DONE)
            break;
        }

    if (iHdr == SNDBUFS)
        return;

#if 0
    DebugStr("%09d ", GetTickCount());
    DebugStr("SoundDoneCallback: pwhdr = %08X, hdr # = %d\n", pwhdr, pwhdr-vi.rgwhdr);
#endif

    pwhdr->dwFlags &= ~WHDR_DONE;

#if 0
    pwhdr->dwLoops = 99;
    pwhdr->dwFlags =  WHDR_BEGINLOOP | WHDR_ENDLOOP;
#endif
#if 0
    waveOutWrite(hWave, pwhdr, sizeof(WAVEHDR));
#endif

    pb = pwhdr->lpData;

#ifdef SOFTMAC
    if (vmCur.fSound && FIsMac(vmCur.bfHW))
        {
        int cs;
        int i;
        int ii;

        if (vmachw.rgvia[0].vBufB & 0x80)
          if (vmCur.bfHW < vmMacII)
            {
            // Mac sound disabled (VIA regB bit 7 only used on Plus/SE)

            goto Lnosound;
            }

        if (vmCur.bfHW == vmMacSE)
            ii = vi.cbRAM[0] - 0x300;
        else
            ii = vi.cbRAM[0] - ((vmachw.rgvia[0].vBufA & 0x08) ? 0x300 : 0x5F00);

        if (vmCur.bfHW >= vmMacII)
            {
            cs = 7 - ((vmachw.asc.bVol >> 2) & 7);
// cs = 2;

            // HACK! fake the startup sound

            if (vmachw.iSndStart)
                {
                cs = 1;

                for (i = 0; i < 512; i++)
                    {
                    pb[0] = PeekB(vi.eaROM[0] + vmachw.iSndStart);
                    pb[1] = PeekB(vi.eaROM[0] + vmachw.iSndStart);
                    pb += vi.woc.wChannels;

                    vmachw.iSndStart++;
                    vmachw.cSndStart--;

                    if (vmachw.cSndStart == 0)
                        {
                        vmachw.iSndStart = 0;
                        break;
                        }
                    }

                for ( ; i < 512; i++)
                    {
                    pb[0] = 0x80;
                    pb[1] = 0x80;
                    pb += vi.woc.wChannels;
                    }
                }

            else if (vmachw.asc.bMode == 2)
                {
                signed char b0 = 0, b1 = 0, b2 = 0, b3 = 0;

// printf("in synth code!\n");

                for (i = 0; i < 512; i++)
                    {
                    if (vmachw.asc.lFreq0)
                        {
                        b0 = vmachw.asc.rgb0[(vmachw.asc.lDisp0 >> 16) & 511] ^ 0x80;
                        vmachw.asc.lDisp0 += vmachw.asc.lFreq0 * 2;
                        }

                    if (vmachw.asc.lFreq1)
                        {
                        b1 = vmachw.asc.rgb1[(vmachw.asc.lDisp1 >> 16) & 511] ^ 0x80;
                        vmachw.asc.lDisp1 += vmachw.asc.lFreq1 * 2;
                        }

                    if (vmachw.asc.lFreq2)
                        {
                        b2 = vmachw.asc.rgb2[(vmachw.asc.lDisp2 >> 16) & 511] ^ 0x80;
                        vmachw.asc.lDisp2 += vmachw.asc.lFreq2 * 2;
                        }

                    if (vmachw.asc.lFreq3)
                        {
                        b3 = vmachw.asc.rgb3[(vmachw.asc.lDisp3 >> 16) & 511] ^ 0x80;
                        vmachw.asc.lDisp3 += vmachw.asc.lFreq3 * 2;
                        }

#if 0
if (i < 4)
    printf("b0 b1 b2 b3 = %d %d %d %d\n", b0, b1, b2, b3);
#endif

                    pb[0] = 0x80 + (b0 >> cs) + (b2 >> cs);
                    pb[1] = 0x80 + (b1 >> cs) + (b3 >> cs);
                    pb += vi.woc.wChannels;
                    }
                }

            else if (vmachw.asc.bMode != 1)
                goto Lnosound;

            else

                {

                for (i = 0; i < 512; i++)
                    {
                    char bLeft = vmachw.rgbASC[i] ^ 0x80;
                    char bRight = vmachw.rgbASC[1024 + i] ^ 0x80;

#if 0
if (i < 4)
    printf("bL bR = %d %d\n", bLeft, bRight);
#endif

                    pb[0] = 0x80 + (bLeft >> cs);
                    pb[1] = 0x80 + (bRight >> cs);
                    pb += vi.woc.wChannels;
                    }

                vmachw.asc.bStat = 5; // empty

                if (vmachw.V2Base)
                    vmachw.rgvia[1].vIFR |= 0x10;
                else
                    vmachw.rgvia[1].vIFR |= 0x10;
                }

#if 0
            printf("CHAN 0: %02X %02X %02X %02X ",
                 (BYTE)vmachw.rgbASC[0],
                 (BYTE)vmachw.rgbASC[1],
                 (BYTE)vmachw.rgbASC[2],
                 (BYTE)vmachw.rgbASC[3]);

            printf("CHAN 1: %02X %02X %02X %02X ",
                 (BYTE)vmachw.rgbASC[0x400],
                 (BYTE)vmachw.rgbASC[0x401],
                 (BYTE)vmachw.rgbASC[0x402],
                 (BYTE)vmachw.rgbASC[0x403]);

            printf("CHAN 2: %02X %02X %02X %02X ",
                 (BYTE)vmachw.rgbASC[0x800],
                 (BYTE)vmachw.rgbASC[0x801],
                 (BYTE)vmachw.rgbASC[0x802],
                 (BYTE)vmachw.rgbASC[0x803]);

            printf("CHAN 3: %02X %02X %02X %02X\n",
                 (BYTE)vmachw.rgbASC[0xC00],
                 (BYTE)vmachw.rgbASC[0xC01],
                 (BYTE)vmachw.rgbASC[0xC02],
                 (BYTE)vmachw.rgbASC[0xC03]);
#endif

#if 0
            printf("TYPE = %02X\n", *(BYTE *)&vmachw.rgbASC[0x800]);
            printf("MODE = %02X\n", *(BYTE *)&vmachw.rgbASC[0x801]);
            printf("CHNL = %02X\n", *(BYTE *)&vmachw.rgbASC[0x802]);
            printf("RST  = %02X\n", *(BYTE *)&vmachw.rgbASC[0x803]);
            printf("STAT = %02X\n", *(BYTE *)&vmachw.rgbASC[0x804]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x805]);
            printf("VOL  = %02X\n", *(BYTE *)&vmachw.rgbASC[0x806]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x807]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x808]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x809]);
            printf("DIR  = %02X\n", *(BYTE *)&vmachw.rgbASC[0x80A]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x80B]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x80C]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x80D]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x80E]);
            printf("???? = %02X\n", *(BYTE *)&vmachw.rgbASC[0x80F]);
#endif

#if 0
            printf("DISP0= %08X\n", *(LONG *)&vmachw.rgbASC[0x810]);
            printf("FREQ0= %08X\n", *(LONG *)&vmachw.rgbASC[0x814]);
            printf("DISP1= %08X\n", *(LONG *)&vmachw.rgbASC[0x818]);
            printf("FREQ1= %08X\n", *(LONG *)&vmachw.rgbASC[0x81C]);
            printf("DISP2= %08X\n", *(LONG *)&vmachw.rgbASC[0x820]);
            printf("FREQ2= %08X\n", *(LONG *)&vmachw.rgbASC[0x824]);
            printf("DISP3= %08X\n", *(LONG *)&vmachw.rgbASC[0x828]);
            printf("FREQ3= %08X\n", *(LONG *)&vmachw.rgbASC[0x82C]);
            printf("RMASK= %08X\n", *(LONG *)&vmachw.rgbASC[0x830]);
            printf("LMASK= %08X\n", *(LONG *)&vmachw.rgbASC[0x834]);
#endif
            }
        else
        for (i = 0; i < 370; i++)
            {
            char bLeft = PeekB(ii + (i<<1)) + 128;
            char bRight = bLeft;

            cs = 7 - (vmachw.rgvia[0].vBufA & 7);
#if 0
            *pb++ = 128 + (bLeft >> cs);
            *pb++ = 128 + (bRight >> cs);
#else
            pb[0] = 128 + (bLeft >> cs);
            pb[1] = 128 + (bRight >> cs);
            pb += 2;
#endif
            }
        }
    else
#endif // SOFTMAC
    if (vmCur.fSound)
        {
        int i;

        for (i = 0; i < SAMPLES_PER_VOICE; i++)
            {
            int bLeft = 0;
            int bRight = 0;
            static int poly4, poly5, poly9, poly17;

            if (!FIsAtari68K(vmCur.bfHW))
            {
            int j;
            for (j = 0; j < 90; j++) // REVIEW: 1.8MHz / 22KHz = 90 but we'll use 9
            {
            poly4  = ((poly4 >> 1) | (~(poly4 ^ (poly4>>1)) << 3)) & 0x000F;
            poly5  = ((poly5 >> 1) | (~(poly5 ^ (poly5>>1)) << 4)) & 0x001F;
            poly9  = ((poly9 >> 1) | (~(poly9 ^ (poly9>>1)) << 8)) & 0x01FF;
            poly17 = ((poly17 >> 1) | (~(poly17 ^ (poly17>>1)) << 16)) & 0x1FFFF;
            }
#if 0
            printf("poly4 = %02X, poly5 = %02X, poly9 = %03X, poly17 = %05X\n",
                    poly4, poly5, poly9, poly17);
#endif

            }

            for (voice = 0; voice < 4;
                    rgvoice[voice].phase += rgvoice[voice].frequency, voice++)
                {
                if (rgvoice[voice].frequency == 0)
                    continue;

                if (rgvoice[voice].volume < 2)
                    continue;

                if (rgvoice[voice].distortion & 1)
                    {
                    // output = DC volume level
                    // Do nothing for now

                    continue;
                    }

                Assert(rgvoice[voice].volume <= 15);

                if ((rgvoice[voice].distortion == 10) ||
                    (rgvoice[voice].distortion == 14))
                    {
                    // pure tone

                    if (FIsAtari68K(vmCur.bfHW))
                        {
                        // sine wave
                        bLeft += (int)(SineOfPhase(rgvoice[voice].phase) * (int)rgvoice[voice].volume) / 15;
                        continue;
                        }
                    else
                        {
                        // square wave
                     //   bLeft += (int)(SqrOfPhase(rgvoice[voice].phase) * (int)rgvoice[voice].volume) / 15;
                        }

                    //continue;
                    }

                // POKEY noise (AUDCTL already shifted down 4 into distortion value
                //
                // The way sound works is in 4 steps:
                //
                // 1 - the divide by N counter produces a pulse which for
                //     pure tones clocks a flip flop to make a square wave

                bLeft += (int)(SqrOfPhase(rgvoice[voice].phase) * rgvoice[voice].volume) / 15;
#if 0
                if (voice == 0)
                    printf("%d %d\n", (int)(SqrOfPhase(rgvoice[voice].phase) * rgvoice[voice].volume) / 15, bLeft);
#endif

                if (SqrOfPhase(rgvoice[voice].phase) >=
                    SqrOfPhase(rgvoice[voice].phase + rgvoice[voice].frequency))
                    {
                    // N counter didn't count down, so no new pulse

                    continue;
                    }

#if 0
            printf("poly4 = %02X, poly5 = %02X, poly9 = %03X, poly17 = %05X\n",
                    poly4, poly5, poly9, poly17);
#endif

                // 2 - when bit 7 is on, that pulse is gated by the low
                //     bit of the 5-bit poly. No pulse, no change of output
                //     which we fake by reversing the phase

                if (((rgvoice[voice].distortion & 8) == 0) && ((poly5 & 1) == 0))
                    {
                    rgvoice[voice].phase ^= (1 << 22);
                    continue;
                    }

                //   - bit 5 chooses whether to feed the 17/4-bit counter

                if (rgvoice[voice].distortion & 2)
                    continue;

                //   - bit 6 chooses between 17-bit and 4-bit counter

                rgvoice[voice].phase = (rgvoice[voice].phase & ~(1 << 22)) |
                   (((rgvoice[voice].distortion & 4) ? poly4 : poly17) & 1) << 22;
                }

#if 0
            if ((pb >= vi.whdr2.lpData) && (pb <= vi.whdr2.lpData + SAMPLES_PER_VOICE*2))
                bLeft /= 2;
#endif
            bLeft  /= 4;
            bRight = bLeft;    // mono for now

            *pb++ = 128 + bLeft;
            *pb++ = 128 + bRight;

            vi.pbAudioBuf[i] = bLeft;
            }
        }
    else
        {
Lnosound:
        memset(pb, 0x80, SAMPLES_PER_VOICE * 2);
        }

#if defined(ATARIST)
    if (/* v.fSTE && vmCur.fSTESound */ 0) // STE sound not supported right now
        {
        pb = pwhdr->lpData;

        if (vsthw.rgbSTESound[0x01] & 1)
            {
            // play a stereo sound

            ULONG base = (vsthw.rgbSTESound[0x03] << 16) | (vsthw.rgbSTESound[0x05] << 8) | (vsthw.rgbSTESound[0x07]);
            ULONG cntr = (vsthw.rgbSTESound[0x09] << 16) | (vsthw.rgbSTESound[0x0B] << 8) | (vsthw.rgbSTESound[0x0D]);
            ULONG end  = (vsthw.rgbSTESound[0x0F] << 16) | (vsthw.rgbSTESound[0x11] << 8) | (vsthw.rgbSTESound[0x13]);

            BOOL fStereo = (vsthw.rgbSTESound[0x20] & 0x80) == 0;
            int i;

            // how many 44 Khz samples we make out of one STE sample
            int  step = 8 >> (vsthw.rgbSTESound[0x20] & 0x03);

            if ((cntr >= end) || (cntr == 0))
                cntr = base;

            for (i = 0; i < SAMPLES_PER_VOICE; i += step)
                {
                int bLeft = PeekB(cntr) ^ 0x80;
                int bRight;
                int j;

                if (fStereo)
                    {
                    bRight = PeekB(cntr+1) ^ 0x80;
                    cntr += 2;
                    }
                else
                    {
                    bRight = bLeft;
                    cntr++;
                    }

                for (j = 0; j < step; j++)
                    {                
                    *pb++ = (*pb + bLeft)/2;
                    *pb++ = (*pb + bRight)/2;
                    }

                if (cntr >= end)
                    {
                    cntr = base;
                    if ((vsthw.rgbSTESound[0x01] & 2) == 0)
                        {
                        // not repeating, stop playback

                        vsthw.rgbSTESound[0x01] = 0;
                        cntr = 0;
                        }
                    }

                } // for i

            // update counter

            vsthw.rgbSTESound[0x09] = (BYTE)(cntr >> 16);
            vsthw.rgbSTESound[0x0B] = (BYTE)(cntr >> 8);
            vsthw.rgbSTESound[0x0D] = (BYTE)(cntr);

            } // if stereo

        } // if ste sound
#endif // ATARIST

#if 0
    DebugStr("%09d ", GetTickCount());
    DebugStr("Calling waveOutWrite: pwhdr = %08X, hdr # = %d\n", pwhdr, pwhdr-vi.rgwhdr);
#endif

#if !defined(_M_ARM)
    waveOutWrite(hWave, pwhdr, sizeof(WAVEHDR));
#endif
}


void UninitSound()
{
#if !defined(_M_ARM)
    if (hWave)
        {
        int iHdr;

        waveOutReset(hWave);

        for (iHdr = 0; iHdr < SNDBUFS; iHdr++)
            {
            waveOutUnprepareHeader(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));
            }
        waveOutClose(hWave);
        }
#endif

    hWave = NULL;
    vi.fWaveOutput = FALSE;
}

void InitSound(int cbSamples)
{
    WAVEOUTCAPS woc;
    int i, iMac = 0;

    UninitSound();

    if (!vmCur.fSound)
        return;

    if (cbSamples == 0)
        cbSamples = SAMPLES_PER_VOICE;
    else
        SAMPLES_PER_VOICE = cbSamples;

#if !defined(_M_ARM)
    iMac = waveOutGetNumDevs();

#ifndef NDEBUG
    printf("number of wave output devices = %d\n", iMac);
#endif

    for (i = 0; i < iMac; i++)
        {
        DebugStr("querying output device %d\n", i);

        if (!waveOutGetDevCaps(i, &woc, sizeof(woc)))
            {
#ifndef NDEBUG
            printf("Device %d responded\n", i);
#endif

            // if we find any device that can do 44 KHz mono, take it
            if (woc.dwFormats & WAVE_FORMAT_2S08)
                {
                if (!vi.fWaveOutput)
                    {
                    vi.fWaveOutput = TRUE;
                    vi.iWaveOutput = i;
                    vi.woc = woc;
                    }
                }

            // if we find a stereo device, we're done
            if ((woc.dwFormats & WAVE_FORMAT_2S08) && (woc.wChannels == 2))
                {
                vi.iWaveOutput = i;
                vi.woc = woc;
                break;
                }
            }
        }

    if (vi.fWaveOutput)
        {
#ifndef NDEBUG
        printf("Selected wave device #%d\n", vi.iWaveOutput);
#endif
        }
    else
        {
        return;
        }

    if ((vi.woc.wChannels != 1) && (vi.woc.wChannels != 2))
        {
        // Windows 2000 bug!
        //
        // WDM drivers may support up to 64K channels! If anything other
        // than one or two channels, default to 2

        vi.woc.wChannels = 2;
        }

    {
    pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.wf.nChannels  = vi.woc.wChannels;
    pcmwf.wf.nSamplesPerSec = SAMPLE_RATE;
    pcmwf.wf.nAvgBytesPerSec = SAMPLE_RATE * vi.woc.wChannels;
    pcmwf.wf.nBlockAlign = vi.woc.wChannels;
    pcmwf.wBitsPerSample = 8;

    if (!waveOutOpen(&hWave, vi.iWaveOutput, &pcmwf, 0 /* SoundDoneCallback */, 0, CALLBACK_NULL))
        {
        int iHdr;

#ifndef NDEBUG
        printf("opened wave device, handle = %08X\n", hWave);
        printf("Device name %s %d %08X\n",
                vi.woc.szPname,    vi.woc.wChannels, vi.woc.dwFormats);
#endif

        for (iHdr = 0; iHdr < SNDBUFS; iHdr++)
            {
            vi.rgwhdr[iHdr].lpData = vi.rgbSndBuf[iHdr];
            vi.rgwhdr[iHdr].dwBufferLength = SAMPLES_PER_VOICE * vi.woc.wChannels;
            vi.rgwhdr[iHdr].dwBytesRecorded = -1;
            vi.rgwhdr[iHdr].dwFlags = 0; // WHDR_BEGINLOOP;
            vi.rgwhdr[iHdr].dwLoops = 0;

            waveOutPrepareHeader(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));
            waveOutWrite(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));
            }
        }
    else
        GetLastError();
    }
#endif
}


void InitMIDI()
{
    MIDIINCAPS  mic;
    MIDIOUTCAPS moc;
    int i, iMac = 0;

    if (!vmCur.fMIDI)
        return;

#if !defined(_M_ARM)
    iMac = midiOutGetNumDevs();

    DebugStr("number of MIDI output devices = %d\n", iMac);

    if (iMac == 0)
        vmCur.fMIDI = FALSE;

    for (i = 0; i < iMac; i++)
        {
        DebugStr("querying output device %d\n", i);

        if (!midiOutGetDevCaps(i, &moc, sizeof(moc)))
            {
            DebugStr("Device responded!\n");
            }
        }

    iMac = midiInGetNumDevs();

    DebugStr("number of MIDI input devices = %d\n", iMac);

    if (iMac == 0)
        vmCur.fMIDI = FALSE;

    for (i = 0; i < iMac; i++)
        {
        DebugStr("querying input device %d\n", i);

        if (!midiInGetDevCaps(i, &mic, sizeof(mic)))
            {
            DebugStr("Device responded!\n");

            }
        }
#endif
}


void InitJoysticks()
{
#if !defined(_M_ARM)
    int i;

    if (joyGetNumDevs() == 0)
        return;

    memset (&vi.rgji, 0, sizeof(vi.rgji));

    for (i = 0; i < 2; i++)
        {
        vi.rgji[i].dwSize = sizeof(JOYINFOEX);
        vi.rgji[i].dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;


        if (joyGetPosEx(JOYSTICKID1+i, &vi.rgji[i]) != JOYERR_UNPLUGGED)
            joyGetDevCaps(JOYSTICKID1+i, &vi.rgjc[i], sizeof(JOYCAPS));
        }
#endif
}


void CaptureJoysticks()
{
#if !defined(_M_ARM)
    int i;

    if (!vmCur.fJoystick || joyGetNumDevs() == 0)
        return;

    for (i = 0; i < 2; i++)
        {
        joySetThreshold(JOYSTICKID1+i, (vi.rgjc[i].wXmax - vi.rgjc[i].wXmin)/8);
        joySetCapture(vi.hWnd, JOYSTICKID1+i, 100, TRUE);
        }
#endif
}


void ReleaseJoysticks()
{
#if !defined(_M_ARM)
    joyReleaseCapture(JOYSTICKID1);
    joyReleaseCapture(JOYSTICKID2);
#endif
}



