
/****************************************************************************

    SOUND.C

    - Mono and stereo sound routines

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    08/24/2001  darekm      Update for SoftMac XP

****************************************************************************/

#include "gemtypes.h" // main include file 
#include "atari8.vm/atari800.h"

#define CNT_ANGLES 64

// !!! when we support multiple VMs running at the same time, each will need its own static
static int sCurBuf = -1;	// which buffer is the current one we're filling
static int sOldSample = 0; // how much of the buffer is full already

typedef struct {
	int widthTop;	// width of the top of each wave pulse in samples x 1000 for better precision
	int widthBottom;	// width of the bottom of each pulse
	BOOL phase;	// are we doing the top (or bottom) of the square wave?
	int pos;	// how far along the width of the pulse are we?
	BOOL skipping;	// are we skipping this top of the pulse due to poly distortion?
} PULSE;
static PULSE pulse[4] = { { 0, 0, FALSE, 0, FALSE }, { 0, 0, FALSE, 0, FALSE }, { 0, 0, FALSE, 0, FALSE }, { 0, 0, FALSE, 0, FALSE } };
static int poly4, poly5, poly9, poly17;

HWAVEOUT hWave;
WAVEFORMATEX pcmwf;
static FILE *fp;

typedef struct
{
    ULONG frequency;           
    ULONG volume;        // 0..15
    ULONG distortion;
} VOICE;

// the old values to use to catch up to the present, when they have just been changed
static VOICE rgvoice[4] = { { 0,0,0 }, { 0,0,0 }, { 0,0,0 }, { 0,0,0 } };
static ULONG sAUDCTL = 0;

void CALLBACK MyWaveOutProc(
	HWAVEOUT  hwo,
	UINT      uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2)
{
#ifndef NDEBUG
	if (uMsg == WOM_DONE) {
		int cx = 0;
		for (int i = 0; i < SNDBUFS; i++)
		{
			if (vi.rgwhdr[i].dwFlags & WHDR_DONE) cx++;
		}
		if (cx == SNDBUFS)
			OutputDebugString("AUDIO GLITCH - EMPTY\n");

		//char ods[100];
		//sprintf(ods, "DONE %08x (%d free)\n", dwParam1, cx);
		//OutputDebugString(ods);
	}
#endif
}

// WRITE SOME AUDIO TO THE WAVE BUFFER - from wherever we left off (sOldSample) to now (iCurSample). If it becomes full, play it.
//
void SoundDoneCallback(LPWAVEHDR pwhdr, int iCurSample)
{
	assert(iCurSample <= SAMPLES_PER_VOICE);
	
	// find a buffer to use if we haven't already
	if (sCurBuf == -1)
	{
		for (int i = 0; i < SNDBUFS; i++)
		{
			if (pwhdr[i].dwFlags & WHDR_DONE) {
				sCurBuf = i;
				sOldSample = 0;	// start writing at the beginning of it
			}
		}
	}

	// uh oh, no free buffers to write into (this will happen all the time when not running in real time but as fast as possible)
	if (sCurBuf == -1) {
#ifndef NDEBUG
		//OutputDebugString("AUDIO GLITCH - FULL\n");
#endif
		return;
	}

	//fprintf(fp, "SOUND[3] %d-%d f=%d v=%d d=%d\n", sOldSample, iCurSample, AUDF4, AUDC4 & 0x0f, AUDC4 >> 4);
	
	// nothing to write (make sure sOldSample is reset to 0 when iCurSample == 800)
	if (iCurSample == sOldSample && sOldSample > 0) {
#ifndef NDEBUG
		//OutputDebugString("Sound changed so fast we dropped a sample!\n");
#endif
	return;
	}

	// where in the buffer to start writing
	signed char *pb = pwhdr[sCurBuf].lpData + sOldSample * 4;

	if (iCurSample < SAMPLES_PER_VOICE && AUDCTL == sAUDCTL &&
		AUDF1 == rgvoice[0].frequency && AUDC1 == (rgvoice[0].volume | (rgvoice[0].distortion << 4)) &&
		AUDF2 == rgvoice[1].frequency && AUDC2 == (rgvoice[1].volume | (rgvoice[1].distortion << 4)) &&
		AUDF3 == rgvoice[2].frequency && AUDC3 == (rgvoice[2].volume | (rgvoice[2].distortion << 4)) &&
		AUDF4 == rgvoice[3].frequency && AUDC4 == (rgvoice[3].volume | (rgvoice[3].distortion << 4)))
	{
		return;	// nothing has changed and this doesn't complete a buffer
	}

	if (iCurSample - sOldSample == 1 && iCurSample != SAMPLES_PER_VOICE &&
			(AUDC4 & 0x0f) == 0 && rgvoice[3].volume > 0)
	{
		rgvoice[3].volume = AUDC4 & 0x0f;
		return;	// !!! hack for MULE, which sends a single sample of noise every frame which needs to be ignored
	}

	if (iCurSample == SAMPLES_PER_VOICE && sOldSample == SAMPLES_PER_VOICE - 1 && (AUDC4 & 0x0f) > 0 && rgvoice[3].volume > 0
				&& *(pb-4) == 0 && *(pb-3) == 0)
	{
		rgvoice[3].volume = 0;	// !!! hack for MULE, the single bad sample is at the end of the buffer
	}

	if (vmCur.fSound) {	// I'm not sure you *can* turn this off, but whatever

		for (int i = 0; i < 4; i++) {
			int freq;

			// frequency isn't relevant for volume only sound or if sound will be silent
			// otherwise, figure out how wide to make the pulses for this voice's frequency
			if ((rgvoice[i].distortion & 0x01) == 0 && rgvoice[i].frequency != 0 && rgvoice[i].volume != 0) {

				// actual frequency is the current clock divided by 2(n+1) (x 10 for greater precision)
				// pokey does the /n before the polys, then /2, but I don't think it matters and this is way easier

				// Channels 1 and 3 can be clocked with 1.78979 MHz, otherwise the clock is either 15700 or 63921Hz
				if ((i == 0 && (sAUDCTL & 0x40)) || (i == 2 && (sAUDCTL & 0x20))) {
					freq = 1789790 * 10 / (2 * (rgvoice[i].frequency + 1));

					// Channels 2 and 4 can be combined with channels 1 and 3 for 16 bit precision, clocked by any possible clock
					// Divide by 2(n+4) for 64K or 2(n+7) for 1.79M
				}
				else if (i == 1 && (sAUDCTL & 0x10)) {
					freq = ((sAUDCTL & 0x40) ? 1789790 : ((sAUDCTL & 0x01) ? 15700L : 63921L)) * 10 /
						(2 * (rgvoice[i].frequency * 256 + rgvoice[i - 1].frequency + ((sAUDCTL & 0x40) ? 7 : 4)));
				}
				else if (i == 3 && (sAUDCTL & 0x08)) {
					freq = ((sAUDCTL & 0x20) ? 1789790 : ((sAUDCTL & 0x01) ? 15700L : 63921L)) * 10 /
						(2 * (rgvoice[i].frequency * 256 + rgvoice[i - 1].frequency + ((sAUDCTL & 0x40) ? 7 : 4)));
				}
				else {
					freq = ((sAUDCTL & 0x01) ? 15700L : 63921L) * 10 / (2 * (rgvoice[i].frequency + 1));
				}

				// recalculate the pulse width as the freq may have changed.
				// round the total pulse width, then split the top and bottom sections into possibly different numbers (off by 1)
				// we need this precision and 48K sampling or sound 57 == sound 58
				// we cannot fix this by making every 10th pulse different in width by 1 for the best possible precision
				// because that has horrible artifacting when you're doing square waves!
				pulse[i].widthTop = (SAMPLE_RATE * 100 / freq + 5) / 10;
				pulse[i].widthBottom = pulse[i].widthTop / 2;
				pulse[i].widthTop -= pulse[i].widthBottom; // this may be one more than top
			}
		}

		// determine the amplitude for each sample in the buffer we need to fill now
		for (int i = sOldSample; i < iCurSample; i++) {

			int bLeft = 0;	//left channel amplitude
			int bRight = 0;

			// !!! I broke ST sound
			if (!FIsAtari68K(vmCur.bfHW)) {
				
				// 1.79MHz / 44.1KHz = 40.5 shifts per audio sample. Do it 41 cuz that's a prime
				BYTE r = RANDOM;	// randomly add an extra shift so we're not so periodic
				r = (r >> 7) + 41;
				// !!! this is slow
				for (int j = 0; j < 1; j++) {
					poly4 = ((poly4 >> 1) | ((~(poly4 ^ (poly4 >> 1)) & 0x01) << 3)) & 0x000F;
					poly5 = ((poly5 >> 1) | ((~(poly5 ^ (poly5 >> 3)) & 0x01) << 4)) & 0x001F;
					//poly9 = ((poly9 >> 1) | (~(poly9 ^ (poly9 >> 1)) << 8)) & 0x01FF;
					// my 9 counter is worse than Darek's
					poly9 = ((poly9 >> 1) | ((~((~poly9) ^ ~(poly9 >> 5)) & 0x01) << 8)) & 0x01FF;
					poly17 = ((poly17 >> 1) | ((~((~poly17) ^ ~(poly17 >> 5)) & 0x01) << 16)) & 0x1FFFF;
				}
				//printf("poly4 = %02X, poly5 = %02X, poly9 = %03X, poly17 = %05X\n", poly4, poly5, poly9, poly17);
            }

			// figure out each voice for this sample
			for (int voice = 0; voice < 4; voice++) {

				// Not in focus, or this voice is silent and won't contribute
				// !!! Sound still freezes annoyingly if window is dragged! WM_MOVING?
				if (!vi.fHaveFocus ||
							((rgvoice[voice].distortion & 0x01) == 0 && (rgvoice[voice].frequency == 0 || rgvoice[voice].volume == 0)))
					continue;

				// apply distortion on the positive edge transition - skip this pulse if it coincides with the relevant poly counters
				if ((rgvoice[voice].distortion & 0x01) == 0 && pulse[voice].pos == 0 && pulse[voice].phase) {
					
					// when bit 7 is off (distortion bit 3), that pulse is gated by the low bit of the 5-bit poly
					//5 bit poly has the opposite sense of the other counters, 1 means skip
					if (((rgvoice[voice].distortion & 8) == 0) && ((poly5 & 1) == 1)) {
						pulse[voice].skipping = TRUE;
					}

					//bit 5 chooses whether to use another counter (the 17/9 or 4-bit counter)
					if ((rgvoice[voice].distortion & 2) == 0) {

						//bit 6 chooses between 17/9-bit and 4-bit counter
						if ((rgvoice[voice].distortion & 4) == 0) {

							// AUDCTL bit 7 chooses 9 or 17
							if ((sAUDCTL & 0x80) && ((poly9 & 1) == 0)) {
								pulse[voice].skipping = TRUE;
							}
							if (!(sAUDCTL & 0x80) && ((poly17 & 1) == 0)) {
								pulse[voice].skipping = TRUE;
							}
						}
						else if ((poly4 & 1) == 0) {
							pulse[voice].skipping = TRUE;
						}
					}
				}

				// !!! support high pass filter

				// volume only
				if (rgvoice[voice].distortion & 1)
				{
					bLeft += ((65535 * (int)rgvoice[voice].volume / 15) - 32768);
					pulse[voice].pos = 0;	// start a new pulse from the beginning when we're done with volume only
				
				// output the correct sample given what part of the pulse we're in, or if we're skipping the positive section
				} else {
					bLeft += (((pulse[voice].phase && !pulse[voice].skipping) ? 32767 : -32768) * (int)rgvoice[voice].volume / 15);
					pulse[voice].pos++;
					
					// may be strictly bigger if the note changed on us
					if ((pulse[voice].phase && pulse[voice].pos >= pulse[voice].widthTop) ||
								(!pulse[voice].phase && pulse[voice].pos >= pulse[voice].widthBottom)) {
						pulse[voice].pos = 0;
						pulse[voice].phase = !pulse[voice].phase;
						pulse[voice].skipping = FALSE;
					}
				}
            }   
		
            bLeft  /= 4;		// we won't let you clip, unlike the real POKEY
			bRight = bLeft;    // mono for now			
			*pb++ = (char)(bLeft & 0xff);
			*pb++ = (char)((bLeft >> 8) & 0xff);
			*pb++ = (char)(bRight & 0xff);
			*pb++ = (char)((bRight >> 8) & 0xff);

		} // for each sample in the buffer

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

    } else {
        memset(pb, 0x00, (iCurSample - sOldSample) * 2 * 2);	// 16 bit signed silence for this portion
    }

	sOldSample = iCurSample; // next time start writing here

#if 0
	int bL;	// create some pure sound to test for audio glitches with known good data in the buffers, assumes 800 samples
	for (int z1 = 0; z1 < 20; z1++) {
		for (int z2 = 0; z2 < 20; z2++) {
			bL = 32767;
			*pb++ = (char)(bL & 0xff);
			*pb++ = (char)((bL >> 8) & 0xff);
			*pb++ = (char)(bL & 0xff);
			*pb++ = (char)((bL >> 8) & 0xff);
		}
		for (int z2 = 0; z2 < 20; z2++) {
			bL = -32768;
			*pb++ = (char)(bL & 0xff);
			*pb++ = (char)((bL >> 8) & 0xff);
			*pb++ = (char)(bL & 0xff);
			*pb++ = (char)((bL >> 8) & 0xff);
		}
	}
#endif

#if !defined(_M_ARM)
	// we have now filled the buffer
	if (iCurSample == SAMPLES_PER_VOICE)
	{
		pwhdr[sCurBuf].dwBytesRecorded = pwhdr[sCurBuf].dwBufferLength;
		pwhdr[sCurBuf].dwFlags &= ~WHDR_DONE;
		waveOutWrite(hWave, &pwhdr[sCurBuf], sizeof(WAVEHDR));
		
		//char ods[100];
		//sprintf(ods, "Write %d %08x (t=%lld)\n", sCurBuf, &pwhdr[sCurBuf],GetJiffies());
		//OutputDebugString(ods);

		//unsigned char *bb = pwhdr[sCurBuf].lpData;
		//for (int zz = 0; zz < 800; zz++)
		//{
		//	fprintf(fp, "%02x%02x ", bb[zz * 4 + 1], bb[zz * 4]);
		//}
		//fprintf(fp, "\n\n");

		sCurBuf = -1;	// need to find a new buffer to write to next time
	}
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
		//fclose(fp);
        }
#endif

    hWave = NULL;
    vi.fWaveOutput = FALSE;
}

void InitSound()
{
    WAVEOUTCAPS woc;
    int i, iMac = 0;

    UninitSound();

    if (!vmCur.fSound)
        return;

#if !defined(_M_ARM)
    iMac = waveOutGetNumDevs();

    for (i = 0; i < iMac; i++)
        {
        DebugStr("querying output device %d\n", i);

        if (!waveOutGetDevCaps(i, &woc, sizeof(woc)))
            {

            // !!! surely everybody supports 48K Stereo 16bit these days
			// I count on that much resolution to make the pitches accurate
            if (woc.dwFormats & WAVE_FORMAT_48S16)
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
#ifndef NDEBUG
        printf("Selected wave device #%d\n", vi.iWaveOutput);
#endif
        }
    else
        {
        return;
        }

    if ((vi.woc.wChannels != 2))
        vi.woc.wChannels = 2;	// don't use any fancy surround sound or multi-channel

    pcmwf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.nChannels  = vi.woc.wChannels;
    pcmwf.nSamplesPerSec = SAMPLE_RATE;
    pcmwf.nAvgBytesPerSec = SAMPLE_RATE * vi.woc.wChannels * 2;
    pcmwf.nBlockAlign = vi.woc.wChannels * 2;
    pcmwf.wBitsPerSample = 16;
	pcmwf.cbSize = 0;

	if (!waveOutOpen(&hWave, vi.iWaveOutput, &pcmwf, (DWORD_PTR)MyWaveOutProc, 0, CALLBACK_FUNCTION))
	{
		int iHdr;

#ifndef NDEBUG
		printf("opened wave device, handle = %08x\n", hWave);
		printf("Device name %s %d %08X\n",
			vi.woc.szPname, vi.woc.wChannels, vi.woc.dwFormats);
#endif

		for (iHdr = 0; iHdr < SNDBUFS; iHdr++)
		{
			vi.rgwhdr[iHdr].lpData = vi.rgbSndBuf[iHdr];
			vi.rgwhdr[iHdr].dwBufferLength = SAMPLES_PER_VOICE * 4; // = * vi.woc.wChannels * vi.woc.wBitsPerSample / 8;
			vi.rgwhdr[iHdr].dwBytesRecorded = 0xffffffff;
			vi.rgwhdr[iHdr].dwFlags = 0u;
			vi.rgwhdr[iHdr].dwLoops = 0;

			waveOutPrepareHeader(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));
			if (iHdr < 2)
			{
				waveOutWrite(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));	// start with 2 buffers of silence to prevent glitching
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
	MMRESULT mm;
    for (i = 0; i < 2; i++)
        {
        vi.rgji[i].dwSize = sizeof(JOYINFOEX);
        vi.rgji[i].dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;

        if (joyGetPosEx(JOYSTICKID1+i, &vi.rgji[i]) != JOYERR_UNPLUGGED)
            mm = joyGetDevCaps(JOYSTICKID1+i, &vi.rgjc[i], sizeof(JOYCAPS));
        }
#endif
}

void CaptureJoysticks(HWND hwnd)
{
#if !defined(_M_ARM)
    int i;
	MMRESULT mm;

    if (!vmCur.fJoystick || joyGetNumDevs() == 0)
        return;

    for (i = 0; i < 2; i++)
        {
		mm = joySetThreshold(JOYSTICKID1 + i, (vi.rgjc[i].wXmax - vi.rgjc[i].wXmin) / 8);
        //mm = joySetCapture(hwnd, JOYSTICKID1+i, 100, TRUE); // doesn't work, so we just poll
	}
#endif
}

void ReleaseJoysticks()
{
#if !defined(_M_ARM)
    MMRESULT mm = joyReleaseCapture(JOYSTICKID1);
    mm = joyReleaseCapture(JOYSTICKID2);
#endif
}
