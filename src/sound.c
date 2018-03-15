
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

static BOOL fSoundInit = FALSE; // only play new buffers in the callback when sound is active, or we hang

typedef struct {
	int widthTop;	// width of each clock pulse clocking the audio (1/2 period) x 1000 for better precision
	int widthBottom;	// current error, so it doesn't propagate
	BOOL phase;	// are we doing the top or bottom of the square wave?
	int pos;	// how far along the width of the pulse are we?
	BOOL skipping;	// are we skipping this top of the pulse due to poly distortion?
} PULSE;
static PULSE pulse[4] = { { 0, 0, FALSE, 0, FALSE }, { 0, 0, FALSE, 0, FALSE }, { 0, 0, FALSE, 0, FALSE }, { 0, 0, FALSE, 0, FALSE } };
static int poly4, poly5, poly9, poly17;

HWAVEOUT hWave;
WAVEFORMATEX pcmwf;
void SoundDoneCallback(LPWAVEHDR pwhdr);

//
// defines a voice (frequency, volume, distortion, and current phase)
//

typedef struct
{
    ULONG frequency;           
    ULONG volume;        // 0..15
    ULONG distortion;
} VOICE;

VOICE rgvoice[4];


void UpdateVoice(int iVoice, ULONG new_frequency, BOOL new_distortion, ULONG new_volume)
{
	//OutputDebugString("\nUpdate Voice\n");

	// !!!
    //if (new_frequency < 30)
    //    new_frequency = 0;
    //else if (new_frequency > 20000)
    //    new_frequency = 0;
    //else if (new_frequency > 11000)
    //    new_frequency = 11000;

	rgvoice[iVoice].frequency = new_frequency;
    rgvoice[iVoice].volume     = new_volume;
    rgvoice[iVoice].distortion = new_distortion;
	
#ifndef NDEBUG
//	if (iVoice == 3) {
//		char sp[100];
//		sprintf(sp, "%llu %llu VOICE3 FREQ=%lu DIST=%lu VOL=%lu\n", GetCycles(), GetJiffies(), rgvoice[iVoice].frequency, rgvoice[iVoice].distortion, rgvoice[iVoice].volume);
//		OutputDebugString(sp);
//	}
#endif
}

// old code from inside the sound callback at the top of the fn
#if 0
CHAR const mpxsine[CNT_ANGLES + 1] =
{
	0,  13,  25,  38,  49,  60,  70,  80,  90, 98, 106, 112, 117, 121, 125, 126, 127, 126, 125, 121, 117, 112, 106, 98,  90,  80, 70, 60, 49, 38, 25, 13,
	0, -13, -25, -38, -49, -60, -70, -80, -90,-98,-106,-112,-117,-121,-125,-126,-127,-126,-125,-121,-117,-112,-106,-98, -90, -80,-70,-60,-49,-38,-25,-13,
	0,
};

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
#if 0

//
// convert phase angle into square wave
//

__inline __int16 SqrOfPhase(ULONG x)
{
	return ((x >> 22) & 1) ? 32767 : -32768;
}

int iC = 0, iB;
for (iHdr = 0; iHdr < SNDBUFS; iHdr++) {
	pwhdr = &vi.rgwhdr[iHdr];

	if (vi.rgwhdr[iHdr].dwFlags & WHDR_DONE) {
		iC++;	// break;
		iB = iHdr;
	}
}

#ifndef NDEBUG
if (iC == 0) { //if (iHdr == SNDBUFS) {
			   //if (nLast == 0 && rgvoice[3].volume == 8) {
			   //	nF = 1;
	char sp[100];
	sprintf(sp, "%llu %llu FULL!\n", GetCycles(), GetJiffies());
	OutputDebugString(sp);
	//}
	return;
}

if (iC == SNDBUFS) {
	char sp[100];
	sprintf(sp, "%llu %llu STARVING!\n", GetCycles(), GetJiffies());
	OutputDebugString(sp);
}
#endif

iHdr = iB; // pick a free buffer to use
pwhdr = &vi.rgwhdr[iHdr];
#endif

#if 0
if (nF > 0) {
	if (rgvoice[3].volume == 8) {
		char sp[100];
		sprintf(sp, "%d Phew!\n", GetTickCount());
		//OutputDebugString(sp);
	}
	else {
		nF = 0;
		char sp[100];
		sprintf(sp, "%d Oh Shit!\n", GetTickCount());
		//OutputDebugString(sp);
	}
}

nLast = rgvoice[3].volume;
if (nLast == 8) nF = 0;

#endif

#if 0
DebugStr("%09d ", GetTickCount());
DebugStr("SoundDoneCallback: pwhdr = %08X, hdr # = %d\n", pwhdr, pwhdr - vi.rgwhdr);
#endif


#if 0
pwhdr->dwLoops = 99;
pwhdr->dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
#endif
#if 0
waveOutWrite(hWave, pwhdr, sizeof(WAVEHDR));
#endif

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
#endif

void CALLBACK MyWaveOutProc(
	HWAVEOUT  hwo,
	UINT      uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE && fSoundInit) { // !!! or we hang, I'm cheating, not supposed to call waveOutWrite in callback.
		SoundDoneCallback((LPWAVEHDR)dwParam1);	// !!! we still hang sometimes!
	}
}

// WRITE AUDIO TO THE WAVE BUFFER 
void SoundDoneCallback(LPWAVEHDR pwhdr)
{
	//OutputDebugString("SoundDoneCallback\n");

	if (pwhdr == NULL) return;	// just in case
	pwhdr->dwFlags &= ~WHDR_DONE;
	signed char *pb = pwhdr->lpData;

	if (vmCur.fSound) {

		for (int i = 0; i < 4; i++) {
			int freq;

			// actual frequency is the current clock divided by 2(n+1) (x 10 for greater precision)
			// pokey does the /n before the polys, then /2, but I don't think it matters and this is way easier
			
			// Channels 1 and 3 can be clocked with 1.78979 MHz, otherwise the clock is either 15700 or 63921Hz
			if ((i == 0 && (AUDCTL & 0x40)) || (i == 2 && (AUDCTL & 0x20))) {
				freq = 1789790 * 10 / (2 * (rgvoice[i].frequency + 1));

			// Channels 2 and 4 can be combined with channels 1 and 3 for 16 bit precision, clocked by any possible clock
			// Divide by 2(n+4) for 64K or 2(n+7) for 1.79M
			} else if (i == 1 && (AUDCTL & 0x10)) {
				freq = ((AUDCTL & 0x40) ? 1789790 : ((AUDCTL & 0x01) ? 15700L : 63921L)) * 10 /
						(2 * (rgvoice[i].frequency * 256 + rgvoice[i-1].frequency + ((AUDCTL & 0x40) ? 7 : 4)));
			} else if (i == 3 && (AUDCTL & 0x08)) {
				freq = ((AUDCTL & 0x20) ? 1789790 : ((AUDCTL & 0x01) ? 15700L : 63921L)) * 10 /
						(2 * (rgvoice[i].frequency * 256 + rgvoice[i - 1].frequency + ((AUDCTL & 0x40) ? 7 : 4)));
			} else {
				freq = ((AUDCTL & 0x01) ? 15700L : 63921L) * 10 / (2 * (rgvoice[i].frequency + 1));
			}

			// recalculate the pulse width as the freq may have changed.
			// round the total pulse width, then split the top and bottom sections into possibly different numbers (off by 1)
			// we need this precision and 48K sampling or sound 57 == sound 58
			// !!! we cannot fix this by making every 10th pulse different in width by 1 for the best possible precision
			// because that has horrible artifacting when you're doing square waves!
			pulse[i].widthTop = (SAMPLE_RATE * 100 / freq + 5) / 10;
			pulse[i].widthBottom = pulse[i].widthTop / 2;
			pulse[i].widthTop -= pulse[i].widthBottom; // this may be one more than top
		}

		// determine the amplitude for each sample in the buffer
        for (int i = 0; i < SAMPLES_PER_VOICE; i++) {

			int bLeft = 0;
			int bRight = 0;

			if (!FIsAtari68K(vmCur.bfHW)) {
				
				// 1.79MHz / 44.1KHz = 40.5 shifts per audio sample. Do it 41 cuz that's a prime
				BYTE r = RANDOM;	// randomly add an extra shift so we're not so periodic
				r = (r >> 7) + 41;
				for (int j = 0; j < r; j++) {
					poly4 = ((poly4 >> 1) | ((~(poly4 ^ (poly4 >> 1)) & 0x01) << 3)) & 0x000F;
					poly5 = ((poly5 >> 1) | ((~(poly5 ^ (poly5 >> 3)) & 0x01) << 4)) & 0x001F;
					//poly9 = ((poly9 >> 1) | (~(poly9 ^ (poly9 >> 1)) << 8)) & 0x01FF;
					// my 9 counter is worse than Darek's
					poly9 = ((poly9 >> 1) | ((~((~poly9) ^ ~(poly9 >> 5)) & 0x01) << 8)) & 0x01FF;
					poly17 = ((poly17 >> 1) | ((~((~poly17) ^ ~(poly17 >> 5)) & 0x01) << 16)) & 0x1FFFF;
				}
#if 0
            printf("poly4 = %02X, poly5 = %02X, poly9 = %03X, poly17 = %05X\n",
                    poly4, poly5, poly9, poly17);
#endif
            }

			// figure out each voice for this sample
			for (int voice = 0; voice < 4; voice++) {

				// Not in focus, or Gem menu is up or the sound is supposed to be silent
				// !!! Sound still freezes annoyingly if window is dragged!
				if (!vi.fHaveFocus || rgvoice[voice].frequency == 0 || rgvoice[voice].volume == 0)
					continue;

				// apply distortion on the positive edge transition - skip this pulse if it coincides with the relevant poly counters
				if (pulse[voice].pos == 0 && pulse[voice].phase) {
					
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
							if ((AUDCTL & 0x80) && ((poly9 & 1) == 0)) {
								pulse[voice].skipping = TRUE;
							}
							if (!(AUDCTL & 0x80) && ((poly17 & 1) == 0)) {
								pulse[voice].skipping = TRUE;
							}
						} else if ((poly4 & 1) == 0) {
							pulse[voice].skipping = TRUE;
						}
					}

					// !!! support volume only sound, but we can't just update POKEY every VBLANK if we do!
					if (rgvoice[voice].distortion & 1) {
					}

					// !!! support high pass filter

				}

				// output the correct sample given what part of the pulse we're in
				int lev = (((pulse[voice].phase && !pulse[voice].skipping) ? 32767 : -32768) * (int)rgvoice[voice].volume / 15);
				bLeft += lev;

				//char ods[100];
				//sprintf(ods, "%d ", lev);
				//OutputDebugString(ods);

				pulse[voice].pos++;
				// may be strictly bigger if the note changed on us
				if ((pulse[voice].phase && pulse[voice].pos >= pulse[voice].widthTop) ||
							(!pulse[voice].phase && pulse[voice].pos >= pulse[voice].widthBottom)) {
					//char ods[100];
					//sprintf(ods, "width=%d\n", pulse[voice].pos);
					//OutputDebugString(ods);
					pulse[voice].pos = 0;
					//pulse[voice].widthError = (pulse[voice].width1000 + pulse[voice].widthError) -
					//		(int)((pulse[voice].width1000 + pulse[voice].widthError) / 1000) * 1000;
					pulse[voice].phase = !pulse[voice].phase;
					pulse[voice].skipping = FALSE;
				}
			
				// !!! support ATARI ST?
				// if (FIsAtari68K(vmCur.bfHW))
            }   

            bLeft  /= 4;		// we won't let you clip, unlike the real POKEY
			bRight = bLeft;    // mono for now			
			*pb++ = (char)(bLeft & 0xff);
			*pb++ = (char)((bLeft >> 8) & 0xff);
			*pb++ = (char)(bRight & 0xff);
			*pb++ = (char)((bRight >> 8) & 0xff);

#if 1
            // !!! Whitewater sound buffer (no longer implemented)
            //vi.pbAudioBuf[i] = bLeft;
#endif
        } // for each sample in the buffer
    } else {
        //memset(pb + sOldSample, 0x00, (sCurSample - sOldSample) * 2 * 2);	// 16 bit signed silence for this portion
    }

#if !defined(_M_ARM)
	// we have now filled the buffer
//	if (sCurSample == SAMPLES_PER_VOICE)
	{
		pwhdr->dwBytesRecorded = pwhdr->dwBufferLength;
		waveOutWrite(hWave, pwhdr, sizeof(WAVEHDR));
//		sOldSample = 0;	// start at beginning of next buffer
//		sCurBuf = -1;	// need to find a new buffer to write to next time
	}

#ifndef NDEBUG
//	char sp[100];
//	sprintf(sp, "%llu %llu waveOutWrite (%d)\n", GetCycles(), GetJiffies(), 0 /*iC*/);
//	OutputDebugString(sp);
#endif
#endif
}

void UpdatePokey()
{
	WORD freq;
	BYTE dist, vol;
		
	freq = (ULONG)AUDF1;
	dist = AUDC1 >> 4;
	vol = AUDC1 & 15;

	UpdateVoice(0, freq, dist, vol);

	freq = (ULONG)AUDF2;
	dist = AUDC2 >> 4;
	vol = AUDC2 & 15;

	UpdateVoice(1, freq, dist, vol);
	
	freq = (ULONG)AUDF3;
	dist = AUDC3 >> 4;
	vol = AUDC3 & 15;

	UpdateVoice(2, freq, dist, vol);

	freq = (ULONG)AUDF4;
	dist = AUDC4 >> 4;
	vol = AUDC4 & 15;

	UpdateVoice(3, freq, dist, vol); 
}


void UninitSound()
{
#if !defined(_M_ARM)
    if (hWave)
        {
        int iHdr;

		fSoundInit = FALSE;
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

void InitSound()
{
    WAVEOUTCAPS woc;
    int i, iMac = 0;

    UninitSound();

    if (!vmCur.fSound)
        return;

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
        {
        // Windows 2000 bug!
        //
        // WDM drivers may support up to 64K channels! If anything other
        // than one or two channels, default to 2

        vi.woc.wChannels = 2;
        }

    {
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
		printf("opened wave device, handle = %08X\n", (unsigned int)hWave);
		printf("Device name %s %d %08X\n",
			vi.woc.szPname, vi.woc.wChannels, vi.woc.dwFormats);
#endif

		fSoundInit = TRUE;

		for (iHdr = 0; iHdr < SNDBUFS; iHdr++)
		{
			vi.rgwhdr[iHdr].lpData = vi.rgbSndBuf[iHdr];
			vi.rgwhdr[iHdr].dwBufferLength = SAMPLES_PER_VOICE * 4; // = * vi.woc.wChannels * vi.woc.wBitsPerSample / 8;
			vi.rgwhdr[iHdr].dwBytesRecorded = -1;
			vi.rgwhdr[iHdr].dwFlags = 0u;
			vi.rgwhdr[iHdr].dwLoops = 0;

			waveOutPrepareHeader(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));
			waveOutWrite(hWave, &vi.rgwhdr[iHdr], sizeof(WAVEHDR));	
		}
		
	} else GetLastError();
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
        mm = joySetCapture(hwnd, JOYSTICKID1+i, 100, TRUE);
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
