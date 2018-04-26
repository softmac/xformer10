
/****************************************************************************

    SOUND.C

    - Mono and stereo sound routines

    Copyright (C) 1991-2018 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    08/24/2001  darekm      Update for SoftMac XP
    03/2018     dannymi     Support VOLUME ONLY speech synth + DISTORTION

****************************************************************************/

#include "gemtypes.h" // main include file
#ifdef XFORMER
#include "atari8.vm/atari800.h"
#endif

#define CNT_ANGLES 64

CHAR const mpxsine[CNT_ANGLES+1] =
{
      0,  13,  25,  38,  49,  60,  70,  80,  90, 98, 106, 112, 117, 121, 125, 126, 127, 126, 125, 121, 117, 112, 106, 98,  90,  80, 70, 60, 49, 38, 25, 13,
      0, -13, -25, -38, -49, -60, -70, -80, -90,-98,-106,-112,-117,-121,-125,-126,-127,-126,-125,-121,-117,-112,-106,-98, -90, -80,-70,-60,-49,-38,-25,-13,
      0,
};

HWAVEOUT hWave;
WAVEFORMATEX pcmwf;
//static FILE *fp;



// These are globals, not per-instance. Only 1 instance is controlling sound at a time, even tiled, and it only switches when all
// threads are asleep.
typedef struct
{
    ULONG frequency;
    ULONG volume;        // 0..15
    ULONG distortion;
    ULONG phase;    // not used in 8 bit
} VOICE;

#ifdef XFORMER

static int sCurBuf = -1;    // which buffer is the current one we're filling
static int sOldSample = 0; // how much of the buffer is full already

typedef struct {
    int pos;    // how far along the width of the pulse are we?
    int width[2];    // width of the two consecutive pulses (1/2 period pulses) for better resolution eg. 17,18,17,18 simulates 17.5
    int wi;    // width index
    BOOL phase;    // are we doing the top (or bottom) of a square wave? (may be random, doesn't necessarily alternate)
} PULSE;
static PULSE pulse[4] = { {0 },{0},{0},{0} };    // only need to init pos

// the old values to use to catch up to the present, when they have just been changed
static VOICE rgvoice[4] = { { 0,0,0 },{ 0,0,0 },{ 0,0,0 },{ 0,0,0 } };
static ULONG sAUDCTL = 0;
#endif


//
// convert a phase angle (0..65535) to a sine (-127..+127)

__inline int SineOfPhase(ULONG x)
{
    int i, w;

    return ((mpxsine[(x >> 16) & (CNT_ANGLES - 1)]));

    //    return mpxsine[(((x) * CNT_ANGLES) >> 16) & (CNT_ANGLES-1)];

    i = (((x)* CNT_ANGLES) >> 16) & (CNT_ANGLES - 1);
    w = ((x)* CNT_ANGLES & 65535) >> 14;

    return ((mpxsine[i] * (15 - w)) + (mpxsine[i + 1] * w)) / 2;
}

//
// convert phase angle into square wave
//

__inline int SqrOfPhase(ULONG x)
{
    return ((x >> 22) & 1) ? 127 : -127;
}

void CALLBACK MyWaveOutProc(
    HWAVEOUT  hwo,
    UINT      uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2)
{
    hwo; uMsg; dwInstance; dwParam1; dwParam2;
#ifndef NDEBUG
    if (uMsg == WOM_DONE) {
        int cx = 0;
        for (int i = 0; i < SNDBUFS; i++)
        {
            if (vi.rgwhdr[i].dwFlags & WHDR_DONE) cx++;
        }
        if (cx == SNDBUFS)
        {
            //ODS("AUDIO GLITCH - EMPTY\n");
        }
        //ODS("DONE %08x (%d free)\n", dwParam1, cx);
    }
#endif
}

// WRITE SOME AUDIO TO THE WAVE BUFFER
// This is OK not being thread safe, because only one thread is allowed in at a time, and we never switch which
// thread is allowed in unless all threads are asleep
//
void SoundDoneCallback(int iVM, LPWAVEHDR pwhdr, int iCurSample)
{
    // Only do sound for the tiled VM in focus, this code is not thread safe
    // We only switch VMs when all threads are asleep
    if (v.fTiling && sVM != (int)iVM)
        return;

    // 8 bit code
    if (!FIsAtari68K(rgvm[iVM].bfHW)) {
#ifdef XFORMER
        // write from wherever we left off(sOldSample) to now(iCurSample). If it becomes full, play it.
        assert(iCurSample <= SAMPLES_PER_VOICE);

        // find a buffer to use if we haven't already
        if (sCurBuf == -1)
        {
            for (int i = 0; i < SNDBUFS; i++)
            {
                if (pwhdr[i].dwFlags & WHDR_DONE) {
                    sCurBuf = i;
                    sOldSample = 0;    // start writing at the beginning of it
                }
            }
        }

        // uh oh, no free buffers to write into (this will happen naturally when tiling or without brakes)
        if (sCurBuf == -1)
        {
            //if (!v.fTiling && fBrakes) ODS("AUDIO GLITCH - FULL\n");
            return; // whatever you do, don't call waveoutWrite!
        }

        // we should never try to go back in time
        assert(iCurSample >= sOldSample);

        //fprintf(fp, "SOUND[0] %d-%d f=%d v=%d d=%d\n", sOldSample, iCurSample, AUDF1, AUDC1 & 0x0f, AUDC1 >> 4);

        // nothing to write (make sure sOldSample is reset to 0 when iCurSample == SAMPLES_PER_VOICE)
        if (iCurSample == sOldSample) {
            //ODS("Wow, Sound changed so fast we dropped a sample!\n");
            goto SaveAud;    // at least remember the new values
        }

        if (pwhdr[sCurBuf].lpData == NULL)
        {
            return;    // no sound buffer (usually due to no sound card or sound disabled in remote session)
        }

        // where in the buffer to start writing
        signed char *pb = pwhdr[sCurBuf].lpData + sOldSample * 2;    // mono

        if (iCurSample < SAMPLES_PER_VOICE && AUDCTL == sAUDCTL &&
            AUDF1 == rgvoice[0].frequency && AUDC1 == (rgvoice[0].volume | (rgvoice[0].distortion << 4)) &&
            AUDF2 == rgvoice[1].frequency && AUDC2 == (rgvoice[1].volume | (rgvoice[1].distortion << 4)) &&
            AUDF3 == rgvoice[2].frequency && AUDC3 == (rgvoice[2].volume | (rgvoice[2].distortion << 4)) &&
            AUDF4 == rgvoice[3].frequency && AUDC4 == (rgvoice[3].volume | (rgvoice[3].distortion << 4)))
        {
            return;    // nothing has changed and this doesn't complete a buffer
        }

        if (iCurSample - sOldSample == 1 && iCurSample != SAMPLES_PER_VOICE &&
            (AUDC4 & 0x0f) == 0 && rgvoice[3].volume > 0)
        {
            rgvoice[3].volume = AUDC4 & 0x0f;
            return;    // !!! app hack for MULE, which sends a single sample of noise every frame which needs to be ignored
        }

        if (iCurSample == SAMPLES_PER_VOICE && sOldSample == SAMPLES_PER_VOICE - 1 && (AUDC4 & 0x0f) > 0 && rgvoice[3].volume > 0
            // stereo version && *(pb - 4) == 0 && *(pb - 3) == 0)
            && *(pb - 2) == 0)    // mono version
        {
            rgvoice[3].volume = 0;    // !!! app hack for MULE, the single bad sample is at the end of the buffer
        }

        if (rgvm[iVM].fSound) {

            // figure out the freq and pulse width of each voice
            int freq[4];
            for (int i = 0; i < 4; i++) {

                // frequency isn't relevant for volume only sound or if sound will be silent
                // otherwise, figure out how wide to make the pulses for this voice's frequency

                // I proceed whenever freq = 0 because you can do fake volume only sound without setting the bit (see bin1\WOOFER.obj)

                if ((rgvoice[i].distortion & 0x01) == 0 && /* rgvoice[i].frequency != 0 && */ rgvoice[i].volume != 0) {

                    // actual frequency is the current clock divided by (n+1) (x 100 for the precision necessary to count poly clocks)
                    // An additional /2 will happen because this frequency is how often the square wave toggles

                    // Channels 1 and 3 can be clocked with 1.78979 MHz, otherwise the clock is either 15700 or 63921Hz
                    if ((i == 0 && (sAUDCTL & 0x40)) || (i == 2 && (sAUDCTL & 0x20))) {
                        freq[i] = 1789790 * 100 / (rgvoice[i].frequency + 1);

                        // Channels 2 and 4 can be combined with channels 1 and 3 for 16 bit precision, clocked by any possible clock
                        // Divide by (n+4) for 64K or (n+7) for 1.79M
                    }
                    else if (i == 1 && (sAUDCTL & 0x10)) {
                        freq[i] = ((sAUDCTL & 0x40) ? 178979000 : ((sAUDCTL & 0x01) ? 1570000 : 6392100)) /
                            ((rgvoice[i].frequency << 8) + rgvoice[i - 1].frequency + ((sAUDCTL & 0x40) ? 7 : 4));
                    }
                    else if (i == 3 && (sAUDCTL & 0x08)) {
                        freq[i] = ((sAUDCTL & 0x20) ? 178979000 : ((sAUDCTL & 0x01) ? 1570000 : 6392100)) /
                            ((rgvoice[i].frequency << 8) + rgvoice[i - 1].frequency + ((sAUDCTL & 0x40) ? 7 : 4));
                    }
                    else {
                        freq[i] = ((sAUDCTL & 0x01) ? 1570000 : 6392100) / (rgvoice[i].frequency + 1);
                    }

                    // recalculate the pulse width as the freq may have changed, rounded to the nearest half sample.
                    // and possibly alternate 2 values to achieve that (16.5 is 16 17 16 17), otherwise sound 21 = sound 22
                    // if the widths of the positive pulses are not all the same in pure tones, it sounds distorted, so you can't
                    // allow higher resoltions like 16.1 by doing 9 16's and a 17.
                    static const int sr = SAMPLE_RATE * 1000;    // save a couple of cycles
                    pulse[i].width[0] = ((sr / freq[i] * 10 + 25) << 1) / 100;
                    pulse[i].width[1] = pulse[i].width[0] / 2;
                    pulse[i].width[0] -= pulse[i].width[1]; // this may be one more than the other one
                }
            }

            // determine the amplitude for each sample in the buffer we need to fill now
            for (int i = sOldSample; i < iCurSample; i++) {

                int bLeft = 0;    //left channel amplitude
                //int bRight = 0;

                // figure out each voice for this sample
                for (int voice = 0; voice < 4; voice++) {

                    // Not in focus, or this voice is silent and won't contribute
                    if (!vi.fHaveFocus ||
                        ((rgvoice[voice].distortion & 0x01) == 0 && (/* rgvoice[voice].frequency == 0 || */ rgvoice[voice].volume == 0)))
                        continue;

                    // apply distortion on the edge transition (only relevant for non-volume only)
                    if ((rgvoice[voice].distortion & 0x01) == 0 && pulse[voice].pos == 0) {

                        // How many times would the poly counters have clocked since the last pulse width? We need the accuracy of
                        // using the freq of the note * 10, not the pulse width, which is only rounded to the nearest 1/2 integer
                        int r = 178979000 / freq[voice];

                        // poly5's cycle is 31
                        if (!(rgvoice[voice].distortion & 8))    // only if it's being used
                        {
                            poly5pos[voice] = (poly5pos[voice] + r) % 0x1f;
                        }

                        // when bit 7 of AUDCx is off (distortion bit 3), that pulse is gated by the low bit of the 5-bit poly
                        // ie. only allow the following poly counters/pure tone to do their thing if
                        // poly5 is inactive or if active, if poly5 is currently high. Otherwise keep the same amplitude as the last pulse.
                        BOOL fAllow = (rgvoice[voice].distortion & 8) || (poly5[poly5pos[voice]] & 1);

                        // !!! poly 5 combined with 4 sounds wrong! (Distortion 4) Why?

                        //bit 5 chooses whether to use another counter (the 17/9 or 4-bit counter)
                        if ((rgvoice[voice].distortion & 2) == 0) {

                            //bit 6 chooses between 17/9-bit and 4-bit counter
                            if ((rgvoice[voice].distortion & 4) == 0) {

                                // AUDCTL bit 7 chooses 9 or 17
                                if ((sAUDCTL & 0x80) && fAllow)
                                {
                                    poly9pos[voice] = (poly9pos[voice] + r) % 0x1ff;
                                    pulse[voice].phase = (poly9[poly9pos[voice]] & 1);
                                }
                                if (!(sAUDCTL & 0x80) && fAllow)
                                {
                                    poly17pos[voice] = (poly17pos[voice] + r) % 0x1fff;
                                    pulse[voice].phase = (poly17[poly17pos[voice]] & 1);
                                }
                            }
                            else if (fAllow)
                            {
                                poly4pos[voice] = (poly4pos[voice] + r) % 0xf;
                                pulse[voice].phase = (poly4[poly4pos[voice]] & 1);
                            }
                        }

                        // pure tones invert the phase every pulse width
                        else if (fAllow)
                        {
                            pulse[voice].phase = !pulse[voice].phase;
                        }
                    }

                    // !!! support high pass filter

                    // volume only
                    if (rgvoice[voice].distortion & 1)
                    {
                        //bLeft += ((65535 * (int)rgvoice[voice].volume / 15) - 32768);
                        bLeft += ((rgvoice[voice].volume << 12) - 32768);    // much faster
                        pulse[voice].pos = 0;    // start a new pulse from the beginning when we're done with volume only

                    // output the correct sample given what part of the pulse we're in, or if we're skipping the positive section
                    }
                    else {
                        bLeft += ((pulse[voice].phase ? 32768 : -32768) * (int)rgvoice[voice].volume >> 4);
                        pulse[voice].pos++;

                        // may be strictly bigger if the note changed on us
                        if (pulse[voice].pos >= pulse[voice].width[pulse[voice].wi & 1])
                        {
                            // the next pulse may have a different width than this one if we're alternating 17,16,17,16 to simulate 16.5
                            pulse[voice].pos = 0;
                            pulse[voice].wi = (~pulse[voice].wi) & 1;
                        }
                    }
                }

                bLeft = (bLeft >> 2);        // we won't let you clip, unlike the real POKEY
                //bRight = bLeft;    // mono for now
                *((WORD *)pb)++ = (bLeft & 0xffff);
                //*pb++ = (char)(bRight & 0xff);
                //*pb++ = (char)((bRight >> 8) & 0xff);

            } // for each sample in the buffer

        SaveAud:
            // next time we write audio, we'll use the new values that just got set (and triggered us being called)
            sAUDCTL = AUDCTL;
            rgvoice[0].frequency = AUDF1;
            rgvoice[1].frequency = AUDF2;
            rgvoice[2].frequency = AUDF3;
            rgvoice[3].frequency = AUDF4;
            rgvoice[0].volume = AUDC1 & 0x0f;
            rgvoice[1].volume = AUDC2 & 0x0f;
            rgvoice[2].volume = AUDC3 & 0x0f;
            rgvoice[3].volume = AUDC4 & 0x0f;
            rgvoice[0].distortion = AUDC1 >> 4;
            rgvoice[1].distortion = AUDC2 >> 4;
            rgvoice[2].distortion = AUDC3 >> 4;
            rgvoice[3].distortion = AUDC4 >> 4;

        }
        else {
            memset(pb, 0x00, (iCurSample - sOldSample) * 2);    // 16 bit signed MONO silence for this portion
        }

        sOldSample = iCurSample; // next time start writing here

#if 0
        int bL;    // create some pure sound to test for audio glitches with known good data in the buffers, assumes 800 samples
        for (int z1 = 0; z1 < 20; z1++) {
            for (int z2 = 0; z2 < 20; z2++) {
                bL = 32767;
                *pb++ = (char)(bL & 0xff);
                *pb++ = (char)((bL >> 8) & 0xff);
                //*pb++ = (char)(bL & 0xff);
                //*pb++ = (char)((bL >> 8) & 0xff);
            }
            for (int z2 = 0; z2 < 20; z2++) {
                bL = -32768;
                *pb++ = (char)(bL & 0xff);
                *pb++ = (char)((bL >> 8) & 0xff);
                //*pb++ = (char)(bL & 0xff);
                //*pb++ = (char)((bL >> 8) & 0xff);
            }
        }
#endif

#if !defined(_M_ARM)
        // we have now filled the buffer
        if (iCurSample == SAMPLES_PER_VOICE)
        {
            pwhdr[sCurBuf].dwFlags &= ~WHDR_DONE;
            waveOutWrite(hWave, &pwhdr[sCurBuf], sizeof(WAVEHDR));

            //ODS("Write %d %08x (t=%lld)\n", sCurBuf, &pwhdr[sCurBuf],GetJiffies());

            //unsigned char *bb = pwhdr[sCurBuf].lpData;
            //for (int zz = 0; zz < 800; zz++)
            //{
            //    fprintf(fp, "%02x%02x ", bb[zz * 4 + 1], bb[zz * 4]);
            //}
            //fprintf(fp, "\n\n");

            sCurBuf = -1;    // need to find a new buffer to write to next time
        }
#endif
#endif // #ifdef XFORMER
    }

    // 16 bit ATARI ST Code
    else
    {
#ifdef ATARIST
        signed char *pb;
        int voice;
        int iHdr;

        // ATARI 16 bit code must call us once per frame with this value
        assert(iCurSample == SAMPLES_PER_VOICE);

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
        DebugStr("SoundDoneCallback: pwhdr = %08X, hdr # = %d\n", pwhdr, pwhdr - vi.rgwhdr);
#endif

        pwhdr->dwFlags &= ~WHDR_DONE;

#if 0
        pwhdr->dwLoops = 99;
        pwhdr->dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
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

                    for (; i < 512; i++)
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
                    char bLeft = PeekB(ii + (i << 1)) + 128;
                    char bRight = bLeft;

                    cs = 7 - (vmachw.rgvia[0].vBufA & 7);
#if 0
                    * pb++ = 128 + (bLeft >> cs);
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
                    if ((pb >= vi.whdr2.lpData) && (pb <= vi.whdr2.lpData + SAMPLES_PER_VOICE * 2))
                        bLeft /= 2;
#endif
                    bLeft /= 4;
                    //bRight = bLeft;    // mono for now

                    *pb++ = 128 + bLeft;
                    *pb++ = 128 + bRight;

#if 1
                    // Whitewater sound buffer
                    vi.pbAudioBuf[i] = bLeft;
#endif
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
                ULONG end = (vsthw.rgbSTESound[0x0F] << 16) | (vsthw.rgbSTESound[0x11] << 8) | (vsthw.rgbSTESound[0x13]);

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
                        bRight = PeekB(cntr + 1) ^ 0x80;
                        cntr += 2;
                    }
                    else
                    {
                        bRight = bLeft;
                        cntr++;
                    }

                    for (j = 0; j < step; j++)
                    {
                        *pb++ = (*pb + bLeft) / 2;
                        *pb++ = (*pb + bRight) / 2;
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
        DebugStr("Calling waveOutWrite: pwhdr = %08X, hdr # = %d\n", pwhdr, pwhdr - vi.rgwhdr);
#endif

#if !defined(_M_ARM)
        waveOutWrite(hWave, pwhdr, sizeof(WAVEHDR));
#endif


#endif// ATARIST
    }
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
        //fclose(fp);
        }
#endif

    hWave = NULL;
    vi.fWaveOutput = FALSE;
}

//
void InitSound()
{
    WAVEOUTCAPS woc;
    int i, iMac = 0;

    UninitSound();

// figure out a real way to turn sound on/off if you want to, probably not per-vm?

#if !defined(_M_ARM)
    iMac = waveOutGetNumDevs();

    for (i = 0; i < iMac; i++)
        {
        //DebugStr("querying output device %d\n", i);

        if (!waveOutGetDevCaps(i, &woc, sizeof(woc)))
            {

            // !!! surely everybody supports 48K Stereo 16bit these days
            // I count on that much resolution to make the pitches accurate
            if (woc.dwFormats & WAVE_FORMAT_48M16)
                {
                if (!vi.fWaveOutput)
                    {
                    vi.fWaveOutput = TRUE;
                    vi.iWaveOutput = i;
                    vi.woc = woc;
                    }
                }
            }
        }

    if (vi.fWaveOutput)
        {
        //printf("Selected wave device #%d\n", vi.iWaveOutput);
        }
    else
        {
        return;
        }

    if ((vi.woc.wChannels != 1))
        vi.woc.wChannels = 1;    // don't use any fancy surround sound or multi-channel

    pcmwf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.nChannels  = vi.woc.wChannels;
    pcmwf.nSamplesPerSec = SAMPLE_RATE;
    pcmwf.wBitsPerSample = 16;
    pcmwf.nAvgBytesPerSec = SAMPLE_RATE * vi.woc.wChannels * pcmwf.wBitsPerSample / 8;
    pcmwf.nBlockAlign = vi.woc.wChannels * pcmwf.wBitsPerSample / 8;

    pcmwf.cbSize = 0;

    if (!waveOutOpen(&hWave, vi.iWaveOutput, &pcmwf, (DWORD_PTR)MyWaveOutProc, 0, CALLBACK_FUNCTION))
    {
        int iHdr;

#ifndef NDEBUG
        //printf("opened wave device, handle = %08x\n", hWave);
        //printf("Device name %s %d %08X\n", vi.woc.szPname, vi.woc.wChannels, vi.woc.dwFormats);
#endif

        for (iHdr = 0; iHdr < SNDBUFS; iHdr++)
        {
            vi.rgwhdr[iHdr].lpData = vi.rgbSndBuf[iHdr];
            vi.rgwhdr[iHdr].dwBufferLength = SAMPLES_PER_VOICE * vi.woc.wChannels * pcmwf.wBitsPerSample / 8;
            vi.rgwhdr[iHdr].dwBytesRecorded = 0;
            vi.rgwhdr[iHdr].dwFlags = 0;
            vi.rgwhdr[iHdr].dwLoops = 0;

            waveOutPrepareHeader(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));
            if (iHdr < 2)
            {
                waveOutWrite(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));    // start with 2 buffers of silence to prevent glitching
            }
            else
            {
                vi.rgwhdr[iHdr].dwFlags |= WHDR_DONE; // OK to use these now
            }
        }
        //fp = fopen("c:\\danny\\8bit\\out.txt", "w");
    } else GetLastError();
#endif
}

// do we really want to allow each VM to independently decide to do MIDI or not?
void InitMIDI(int iVM)
{
    MIDIINCAPS  mic;
    MIDIOUTCAPS moc;
    int i, iMac = 0;

    if (!rgvm[iVM].fMIDI)
        return;

#if !defined(_M_ARM)
    iMac = midiOutGetNumDevs();

    //DebugStr("number of MIDI output devices = %d\n", iMac);

    if (iMac == 0)
        rgvm[iVM].fMIDI = FALSE;

    for (i = 0; i < iMac; i++)
        {
        //DebugStr("querying output device %d\n", i);

        if (!midiOutGetDevCaps(i, &moc, sizeof(moc)))
            {
            //DebugStr("Device responded!\n");
            }
        }

    iMac = midiInGetNumDevs();

    //DebugStr("number of MIDI input devices = %d\n", iMac);

    if (iMac == 0)
        rgvm[iVM].fMIDI = FALSE;

    for (i = 0; i < iMac; i++)
        {
        //DebugStr("querying input device %d\n", i);

        if (!midiInGetDevCaps(i, &mic, sizeof(mic)))
            {
            //DebugStr("Device responded!\n");

            }
        }
#endif
}

void InitJoysticks()
{
#if !defined(_M_ARM)
    int i, j = 0, nj;
    MMRESULT mm;
    JOYCAPS jc;

    ReleaseJoysticks();

    if ((nj = joyGetNumDevs()) == 0)
        return;

    // 16 seems to be common, and the 2nd and subsequent joysticks might be a sparse matrix so check thenm all
    nj = min(nj, NUM_JOYDEVS);

    memset (&vi.rgji, 0, sizeof(vi.rgji));

    for (i = 0; i < nj; i++)
    {
        vi.rgji[i].dwSize = sizeof(JOYINFOEX);
        vi.rgji[i].dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;

        if (joyGetPosEx(JOYSTICKID1 + i, &vi.rgji[i]) != JOYERR_UNPLUGGED)
        {
            mm = joyGetDevCaps(JOYSTICKID1 + i, &jc, sizeof(JOYCAPS));
            if (!mm && jc.wNumButtons > 0)
            {
                vi.rgjc[j] = jc;    // joy caps
                vi.rgjn[j++] = i;    // which joystick ID this is
            }
        }
    }
    vi.njc = j;    // this is how many actual joysticks are plugged in, and we compacted the array of them
#endif
}

// globals, each warm and cold start will glitch the other VMs
//
void CaptureJoysticks()
{
#if !defined(_M_ARM)
    int i;
    MMRESULT mm;

    // find a way to disable joystick if they want? (probably not per-VM)?
    if (joyGetNumDevs() == 0)
        return;

    for (i = 0; i < NUM_JOYDEVS; i++)
        {
        mm = joySetThreshold(JOYSTICKID1 + i, (vi.rgjc[i].wXmax - vi.rgjc[i].wXmin) / 8);
        //mm = joySetCapture(vi.hWnd, JOYSTICKID1+i, 100, TRUE); // doesn't work, so we just poll
    }
#endif
}

void ReleaseJoysticks()
{
#if !defined(_M_ARM)
    for (int z = 0; z < NUM_JOYDEVS; z++)
        //joyReleaseCapture(JOYSTICKID1 + z)
        ;

#endif
}
