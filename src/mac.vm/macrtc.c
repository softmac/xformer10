
/***************************************************************************

	MACRTC.C

	- emulation of the Mac's parameter RAM and real time clock

	08/24/2001

***************************************************************************/

#include "gemtypes.h"

#ifdef SOFTMAC

#define TRACERTC 0

enum
{
    rtcEnb = 0x04,
    rtcCLK = 0x02,
    rtcData = 0x01,
};


enum
{
    RTC_IDLE,
    RTC_WRITECMD,
    RTC_WRITECMD2,
    RTC_WRITEDATA,
    RTC_READDATA,
};


int  cbitRTC;                       // bit counter for RTC input/output
BYTE bRTC;                          // CMOS data byte (in/out)
int  iCMOS;                         // CMOS read/write register select
BYTE mdRTC;


void AbortRTCCmd()
{
#if TRACERTC
    printf("AbortRTCCmd\n");
#endif
    mdRTC = RTC_IDLE;
}


void StartRTCCmd()
{
#if TRACERTC
    printf("StartRTCCmd\n");
#endif
    mdRTC = RTC_WRITECMD;
    bRTC = 0;                       // data byte is inited to 0
    cbitRTC = 8;                    // 8 bits to go
}


#define fWrite vmachw.rgbCMOS[259]

void WriteByteToRTC()
{
#if TRACERTC
    printf("writing %02X to RTC\n", bRTC);
#endif

    if (mdRTC == RTC_WRITEDATA)
        {
        mdRTC = RTC_IDLE;

        if ((iCMOS < 256) && (vmachw.rgbCMOS[256] & 0x80))
            {
#if TRACERTC
            printf("RTC: CMOS IS WRITE PROTECTED!!!\n");
#endif
            return;
            }

#if TRACERTC
        printf("RTC: Writing to CMOS offset %02X value %02X\n", iCMOS, bRTC);
#endif
        vmachw.rgbCMOS[iCMOS] = bRTC;
        return;
        }

    else if (mdRTC == RTC_WRITECMD2)
        {
        iCMOS |= (bRTC >> 2) & 0x1F;
#if TRACERTC
        printf("RTC: XPRAM offset = %02X\n", iCMOS);
#endif
        }

    else if (mdRTC == RTC_WRITECMD)
        {
        fWrite = !(bRTC & 0x80);

        if ((bRTC & 0x78) == 0x38)
            {
            iCMOS = (bRTC << 5) & 0xE0;
#if TRACERTC
            printf("RTC: first byte of XPRAM\n");
#endif
            mdRTC = RTC_WRITECMD2;
            cbitRTC = 8;                    // 8 bits to go
            return;
            }

        if (bRTC == 0x31)
            iCMOS = 257;                    // test register
        else if (bRTC == 0x35)
            iCMOS = 256;                    // write protect register
        else if ((bRTC & 0x6F) == 0x01)
            iCMOS = -4;                     // seconds register 0
        else if ((bRTC & 0x6F) == 0x05)
            iCMOS = -3;                     // seconds register 1
        else if ((bRTC & 0x6F) == 0x09)
            iCMOS = -2;                     // seconds register 2
        else if ((bRTC & 0x6F) == 0x0D)
            iCMOS = -1;                     // seconds register 3
        else if ((bRTC & 0x73) == 0x21)
            iCMOS = ((bRTC & 0x0C) >> 2) + 16 - 8;  // registers $10..$13
        else if ((bRTC & 0x43) == 0x41)
            iCMOS = ((bRTC & 0x3C) >> 2) + 16;   // registers $00..0F
        else
            {
            DebugStr("unknown command byte %02X\n", bRTC);
            iCMOS = 258;                    // unknown
            }
        }

    mdRTC = fWrite ? RTC_WRITEDATA : RTC_READDATA;
    cbitRTC = 8;                    // 8 bits to go

    if (!fWrite)
        {
        if (iCMOS < 0)
            {
            // reading the real time clock!

            ULONG LGetMacTime(void);

            vmachw.lRTC = LGetMacTime();

            // This should match rgbCMOS[iCMOS] but just in case read separately

            bRTC = vmachw.rgbRTC[iCMOS & 3];

            DebugStr("reading from RTC offset %02X value %02X\n", iCMOS & 3, bRTC);
            }
        else
            {
            // some sort of CMOS validation

#if 0
            vmachw.rgbCMOS[12] = 0x4E;
            vmachw.rgbCMOS[13] = 0x75;
            vmachw.rgbCMOS[14] = 0x4D;
            vmachw.rgbCMOS[15] = 0x63;

            // PowerPC ROMs seem to expect this

            vmachw.rgbCMOS[0xB0] = 0x01;

#ifdef DEMO
            // HACK! HACK! HACK! sound is broken, so turn it off

            vmachw.rgbCMOS[8] &= 0xF8; // either this...
            vmachw.rgbCMOS[16] &= 0xF8; // of this set sound volume to 0
#endif
            // Hack for System 7.5 which won't shut down properly when offset 13 is bad

            if ((iCMOS == 0x013) && (vmachw.rgbCMOS[iCMOS] == 0x01))
                vmachw.rgbCMOS[iCMOS] = 0x48;
#endif

            bRTC = vmachw.rgbCMOS[iCMOS];
            }

        DebugStr("reading from CMOS offset %02X value %02X, 20C = %08X\n", iCMOS, bRTC, PeekL(0x20C));
        }
}


void ReadByteFromRTC()
{
}


void HandleMacRTC(BYTE bOld, BYTE bNew)
{
    if ((bOld ^ bNew) & rtcEnb)
        {
        if (bNew & rtcEnb)
            {
            // abort RTC transaction

            AbortRTCCmd();
            return;
            }
        else
            {
            // start RTC transaction

            StartRTCCmd();

            // fall through
            }
        }

    // if rtcEnb is not low, ignore the clock bit

    if (vmachw.rgvia[0].vBufB & rtcEnb)
        return;

    if ((bOld ^ bNew) & rtcCLK)
        {
        // rtcCLK changed while rtcEnb is low

        if ((vmachw.rgvia[0].vBufB & rtcCLK) && (vmachw.rgvia[0].vDirB & rtcData))
            {
            // writing a bit

            bRTC += bRTC + (vmachw.rgvia[0].vBufB & 1);

#if TRACERTC
            printf("RTC: write byte value = %02X, cbitRTC = %d\n", bRTC, cbitRTC);
#endif

            if (--cbitRTC == 0)
                {
                WriteByteToRTC();
                }

            }
        else if (!(vmachw.rgvia[0].vBufB & rtcCLK) && !(vmachw.rgvia[0].vDirB & rtcData))
            {
            // reading a bit

            vmachw.rgvia[0].vBufB &= ~1;
            vmachw.rgvia[0].vBufB |= (bRTC & 0x80) ? 1 : 0;
            bRTC <<= 1;

#if TRACERTC
            printf("RTC: read byte value = %02X, cbitRTC = %d\n", bRTC, cbitRTC);
#endif

            cbitRTC--;
            }
        }
}


#endif // SOFTMAC

