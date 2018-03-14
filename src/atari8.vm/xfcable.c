
/***************************************************************************

    XFCABLE.C

    - Xformer Cable SIO service routines
    - taken from QTU 1.3 and ST Xformer

    Copyright (C) 1988-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    02/18/1998  darekm      last update
    09/24/1988  darekm      created

***************************************************************************/

#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP

#undef  _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1

#define WindowsSDKDesktopARM64Support 1

#define USETIMER 0

#ifdef HDOS16
#define INXF 1
#include "atari800.h"
#else
#define INXF 1
#include <stdio.h>
#include "atari800.h"
#define FAR
#endif

#ifdef XFORMER

#ifndef HWIN32

/* disk drive status flags */
#define STAT_CONN   0x80
#define STAT_WP     0x40
#define STAT_WARP   0x20
#define STAT_MOTOR  0x10
#define STAT_QD     0x08
#define STAT_DD     0x04
#define STAT_ED     0x02
#define STAT_SD     0x01

#define DENS_NONE   0
#define DENS_SNG    1
#define DENS_1050   2
#define DENS_DBL    3

/* SIO return codes */
#define SIO_OK      0x01
#define SIO_TIMEOUT 0x8A
#define SIO_NAK     0x8B
#define SIO_DEVDONE 0x90

/* bit masks */
#define BIT7    0x80
#define BIT6    0x40
#define BIT5    0x20
#define BIT4    0x10
#define BIT3    0x08
#define BIT2    0x04
#define BIT1    0x02
#define BIT0    0x01

/* timeout delays */
#define TO_BYTE    ((long)uBaudClock << 3)
#define TO_SHORT   ((long)uBaudClock << 6)
#define TO_SEC     ((long)uBaudClock << 17)
#define TO_LONG    ((long)uBaudClock << 18)
#define TO_FORMAT  ((long)uBaudClock << 20)

/*******************************************************************
**  low level timer routines
*******************************************************************/

unsigned uBaudClock = 0;

typedef unsigned short TICK;

// UNUSED, sets the frequency of the timer
//
static void sethertz()
{
    outp(0x21, inp(0x21) & 0xfe);
    outp(0x43, 0x34);
    outp(0x40, 0x01); // lo byte
    outp(0x40, 0x00); // hi byte
}

// set up the timer so its tick count can be read
//
__inline static void init_timer()
{
    outp(0x43, 0x0B);
}

// read the 16-bit tick count
//
__inline static TICK read_timer()
{
    TICK u;

//    outp(0x43, 0x0B);
    u = (inp(0x40) << 8) | inp(0x40);

    return u;
}

// Delay for up to 32767 clock ticks (1 tick ~= 1 us)
//

int foo;
int goo;

__inline static void DelayCus(TICK uDelay)
{
#if USETIMER
    uDelay <<= 1;
#else
    uDelay >>= 0;
    if (uDelay) while (--uDelay)
        {
        foo = uDelay;
        }
    return;
#endif

    init_timer();

//printf("o = %u\n", uDelay);
    uDelay = read_timer() - uDelay; // target count

//printf("c = %u\n", read_timer());
//printf("a = %u\n", uDelay);

    while ((uDelay - read_timer()) & 0x8000)
//    printf("%u\n", read_timer() - uDelay);    
        ;

//    printf("delay_us: error = %u\n\n", uDelay - read_timer());    
}

// Delay the number of milliseconds, approximately
//
static void DelayCms(TICK uDelay)
{
    while (uDelay--)
        DelayCus(uBaudClock << 5);
}

/*******************************************************************
**  low level port routines
*******************************************************************/

#define pwLPT1 ((unsigned int FAR *) 0x00400008)

unsigned int LPTPort = 0x3BC;
unsigned char bDataOut;

#define DataOutPort (LPTPort)
#define DataInPort  (LPTPort+1)

long lTimeout;

//
// Looking at a DB-13 connector face first:
//
//         12  10   8    6    4    2
//       13  11   9   7    5    3    1
//
//    SIO PIN       DB-13   DB-25   PORT AND BIT
//    ----------    -----   -----   ------------
//    DATA IN         3      11     In     0x40
//    DATA OUT        5       5     Out    0x08 (data 3)
//    COMMAND         7       7     Out    0x20 (data 5)
//    CLOCK OUT       2       3     Out    0x02 (data 1)
//    +5V            10       1     Out    0x80 (data 7)
//    GROUND         4,6     18        n/a

#define DATA_IN     0x80
#define DATA_OUT    0x08
#define COMMAND     0x20
#define CLOCK_OUT   0x02
#define _5V         0x80

// One time init
//
void _SIO_Init()
{
    LPTPort = *pwLPT1;
    printf("LPT port = %04X\n", LPTPort);

    outp(DataOutPort, 0xFF); // all data bits high
    bDataOut = 0xFF;
}

// One time uninit
//
void _SIO_UnInit()
{
    outp(DataOutPort, 0xFF); // all data bits high
    bDataOut = 0xFF;
}


// Disable CPU interrupts
//
static void DI() { __asm { cli } }


// Enable CPU interrupts
//
static void EI() { __asm { sti } }


// Tells the SIO bus to stop paying attention
//
__inline static void CommandHi()
    {
    DelayCms(1);
    bDataOut |= COMMAND;
    outp(DataOutPort, bDataOut);
    }

// Tells the SIO bus to start paying attention
//
__inline static void CommandLo()
    {
    bDataOut &= ~COMMAND;
    outp(DataOutPort, bDataOut);
    DelayCms(1);
    }

// Set the CLOCK_OUT line
//
__inline static void SetClockOut(int w)
{
    bDataOut = (bDataOut & ~CLOCK_OUT) | (w ? CLOCK_OUT : 0);
    outp(DataOutPort, bDataOut);

#ifndef NDEBUG
    if (bDataOut & COMMAND)
        printf("error: COMMAND is set in ClockOut\n");
#endif
}

// Set the DATA_OUT line
//
__inline static void SetDataOut(int w)
{
    bDataOut = (bDataOut & ~DATA_OUT) | (w ? DATA_OUT : 0);
    outp(DataOutPort, bDataOut);

#ifndef NDEBUG
    if (bDataOut & COMMAND)
        printf("error: COMMAND is set in DataOut\n");
#endif
}

// Get the DATA_IN line
//
__inline static int GetDataIn()
{
    return (inp(DataInPort) & DATA_IN) == 0;
}

unsigned char rgchCmdFrame[6] = { 0, 0, 0, 0, 0, 0 };

//    send a frame of bytes to DATA OUT

static void SendFrame(pb, cb)
unsigned char *pb;
unsigned int cb;
{
    unsigned int chksum = 0;
    unsigned char *pch = pb;
    unsigned int i, j;
    unsigned char tmp;
    unsigned int w;

    i = cb;
    while (i--)
        {
        chksum += *pch++;
        if (chksum >= 0x100)
            chksum -= 0xFF;
        }
    tmp = *pch;         /* save old value of byte past frame */
    *pch = chksum;

#if 0
    {
    int q;

    printf("SendFrame: cb = %d   ", cb);
    for (q=0; q<5; q++)
        {
        printf("%d:%02x  ", q, pb[q]);
        }
    printf("\n");
    }
#endif

    pch = pb;
    i = cb+1;

    SetClockOut(0);

    while (i--)
        {
        w = *pch++;
        w = 0x200 + w + w;

        for (j = 0; j < 10; j++)
            {
//printf("%c", w & 1 ? '1' : '0');
            SetDataOut(w & 1); 
            SetClockOut(1);
            DelayCus(uBaudClock);
            SetClockOut(0);
            DelayCus(uBaudClock);
            w >>= 1;
            }
//printf("\n");
        }
    pch--;
    *pch = tmp;
}

// get a single byte

unsigned int GetByte()
{
    unsigned int chReturn = 0;
    unsigned int j;
    unsigned long l = lTimeout;

    // wait for start bit (low)

    while (GetDataIn())
        {
        if (l-- == 0)
            return 0xFFFF;
        }

    DelayCus(uBaudClock);       // kill half a cycle

    for (j = 0; j < 8 ; j++)    // 8 data bits
        {
        // kill one clock cycle, then read a bit

        DelayCus(uBaudClock);
        GetDataIn();
        DelayCus(uBaudClock);

        chReturn >>= 1;
        chReturn |= (inp(DataInPort) & DATA_IN) ? 0 : 0x80;
        }

    DelayCus(uBaudClock);       // kill a cycle to get to stop bit
    DelayCus(uBaudClock);

//    printf("GetByte returning %02X\n", chReturn);
    return chReturn;
}

int GetFrame(pb, cb)
unsigned char *pb;
int cb;
{
    unsigned int chksum = 0;
    int i = cb;
    unsigned int chk;

    lTimeout = TO_SEC;

    do
        {
        if ((*pb++ = GetByte()) == 0xFFFF)
            {
            break;
            }
        lTimeout = TO_BYTE;
        } while (--cb);

    chk = GetByte();

    while (i--)
        {
        chksum += *--pb;
        if (chksum >= 0x100)
            chksum -= 0xFF;
        }

/*    print("getframe: %d %d\n", chksum, chk); */

    if (chksum == chk)
        return SIO_OK;
    return SIO_TIMEOUT;
}


/******************************************************************
**
** SIO entry point emulation
**
** return SIO return code
**
** qch  - pointer to buffer (if applicable)
** wDev - deviec ID (this is really wDev + wDrive)
** wCom - SIO command
** wStat - type of command ($00, $40, or $80)
** wBytes - bytes to transfer (if applicable)
** wSector - also known as Aux1 and Aux2
** wTimeout - timeout in seconds
**
******************************************************************/

int _SIOV(char *qch, int wDev, int wCom, int wStat, int wBytes, int wSector, int wTimeout)
{
    int wRetStat;
    char bAux1, bAux2;
    int fWarp = (wCom & 0x80);
    int i,j;

//    DelayCms(20);

    fWarp = 0;

    bAux1 = wSector & 0xFF;
    bAux2 = wSector >> 8;
    
    rgchCmdFrame[0] = wDev;
    rgchCmdFrame[1] = wCom;
    rgchCmdFrame[2] = bAux1;
    rgchCmdFrame[3] = bAux2;

    lTimeout = TO_SHORT;

    if (wStat == 0x00)
        {
        CommandLo();
        DI();
        SendFrame(rgchCmdFrame,4);
        EI();
        CommandHi();

        DI();
        i = GetByte();
        lTimeout = TO_SEC * (long)wTimeout;
        j = GetByte();
        if ((i == 'A') && (j == 'C'))
            wRetStat = SIO_OK;
        else if ((i == 'N') || (j == 'N'))
            wRetStat = SIO_NAK;
        else
            wRetStat = SIO_TIMEOUT;
        EI();
        }

    else if (wStat == 0x40)
        {
        int wRetry = 13;

lRetry40:
        CommandLo();
        DI();
        SendFrame(rgchCmdFrame,4);
        EI();
        CommandHi();
        DI();
        i = GetByte();

        if (i != 'A')
            {
            EI();

            if (--wRetry)
                {
                goto lRetry40;
                }
            else
                goto lTO40;
            }

        lTimeout = TO_SEC * (long)wTimeout;
        j = GetByte();
        if ((i == 'A') && (j == 'C'))
            wRetStat = GetFrame(qch, wBytes);
        else if ((i == 'N') || (j == 'N'))
            wRetStat = SIO_NAK;
        else
            {
lTO40:
            wRetStat = SIO_TIMEOUT;
            }
        EI();
        }
    else if (wStat == 0x80)
        {
        CommandLo();
        DI();
        SendFrame(rgchCmdFrame,4);
        EI();
        CommandHi();

        DI();
        if (GetByte() == 'A')
            {
            DelayCms(1);
            SendFrame(qch, wBytes);
            i = GetByte();
            lTimeout = TO_SEC * (long)wTimeout;
            j = GetByte();
            if ((i == 'A') && (j == 'C'))
                wRetStat = SIO_OK;
            else if ((i == 'N') || (j == 'N'))
                wRetStat = SIO_NAK;
            else
                wRetStat = SIO_TIMEOUT;
            }
        EI();
        }
    else
        wRetStat = SIO_NAK;
    
/*  print("SIOV returning %d\n", wRetStat); */
    /* return status */
    return wRetStat;
}

/*******************************************************************
**  SIO level routines for Quick Transfer Utility
*******************************************************************/

/* first byte of status packet:
** b7 = enhanced   b5 = DD/SD  b4 = motor on   b3 = write prot */

unsigned _SIO_DriveStat(unsigned uDrive)
{
    int i1 = 0, i2 = 0, i3 = 0;
    unsigned int rgfStat = 0;
    char rgch[16];

    rgchCmdFrame[0] = 0x31 + uDrive;
    rgchCmdFrame[1] = 0x00 | 'S';
    rgchCmdFrame[2] = 0x00;
    rgchCmdFrame[3] = 0x00;

    DelayCms(20);

    CommandLo();
    DI();
    SendFrame(rgchCmdFrame,4);
//    EI();
    CommandHi();
    lTimeout = TO_SHORT;
    DI();
    i1 = GetByte();
    if (i1 == 'A')
        i2 = GetByte();
    if ((i1 == 'A') && (i2 == 'C'))
        i3 = GetFrame(rgch, 4);
    EI();
    DelayCms(2);

// printf("DriveStat: i1, i2, i3 = %d, %d, %d\n", i1, i2, i3);
    if (i1 == 'A')
        rgfStat |= STAT_CONN;
    else return 0;

    return rgfStat;

    CommandLo();
    DI();
    rgchCmdFrame[1] = 0x80 | 'S';
    SendFrame(rgchCmdFrame,4);
    CommandHi();
    lTimeout = TO_SHORT;
    i1 = GetByte();
    if (i1 == 'A')
        i2 = GetByte();
    if ((i1 == 'A') && (i2 == 'C'))
        i3 = GetFrame(rgch, 4);
    EI();
    DelayCms(2);

    if ((i1 == 'A') && (i2 == 'C'))
        rgfStat |= STAT_WARP;

    rgfStat |= STAT_SD;
    if (rgch[0] & BIT7)
        rgfStat |= STAT_ED;
    if (rgch[0] & BIT5)
        rgfStat |= STAT_DD;
    if (rgch[0] & BIT3)
        rgfStat |= STAT_WP;

    return rgfStat;
}

int _SIO_DiskIO(uDrive, uSector, cb, pb, fRead)
unsigned int uDrive, uSector, cb;
char *pb;
int fRead;
    {
    int i1;

    if (fRead)
        {
        i1 = _SIOV(pb, 0x31 + uDrive, 'R', 0x40, cb, uSector, 5);
        }
    else
        {
        i1 = _SIOV(pb, 0x31 + uDrive, 'W', 0x80, cb, uSector, 5);
        }

    return i1;
    }

int _SIO_DiskDens(uDrive)
int uDrive;
    {
    int i1;
    char rgch[260];
    
    i1 = _SIOV(rgch, 0x31 + uDrive, 'R', 0x40, 128, 1, 5);
    
    if (i1 == SIO_TIMEOUT)      /* a single density drive */
        {
        return DENS_NONE;
        }
    else if (i1 == SIO_OK)
        {
        i1 = _SIOV(rgch, 0x31 + uDrive, 'R', 0x40, 256, 4, 5);
        if (i1 == SIO_OK)
            return DENS_DBL;
        if (i1 == SIO_NAK)
            return DENS_SNG;
        }
        
    return DENS_NONE;
    }

int _SIO_GetDens(uDrive)
int uDrive;
    {
    int i1;
    char rgch[12];
    
    i1 = _SIOV(rgch, 0x31 + uDrive, 'N', 0x40, 12, 0, 2);
    
    if (i1 == SIO_TIMEOUT)      /* a single density drive */
        {
        return DENS_SNG;
        }
    else if (i1 == SIO_OK)
        {
        return (rgch[5] & 4) ? DENS_DBL : DENS_SNG;
        }
        
    return DENS_NONE;
    }

unsigned char rgchSetDens[12] =
    {
    40, 2, 0, 18, 0, 0, 0, 128, 255, 255, 255, 255
    };

int _SIO_SetDens(uDrive, uDens)
int uDrive, uDens;
    {
    int i1;
    char rgch[4];

    if (uDens == DENS_DBL)
        {
        rgch[5] = 4;
        rgch[6] = 1;
        rgch[7] = 0;
        }
    
    i1 = _SIOV(rgch, 0x31 + uDrive, 'O', 0x80, 12, 0, 2);
    
    if (i1 == SIO_OK)
        {
        return 0;
        }
    return -1;
    }

void UtoPch(u, pch)
unsigned u;
char *pch;
    {
    int ch;

    ch = '0';
    while (u > 99)
        {
        ch++;
        u -= 100;
        }
    *pch++ = ch;

    ch = '0';
    while (u > 9)
        {
        ch++;
        u -= 10;
        }
    *pch++ = ch;

    ch = '0' + u;
    *pch++ = ch;

    *pch = '\0';
    }

void _SIO_Calibrate()
{
    TICK i, iMin;
    unsigned int stat;

    printf("Calibrating Xformer cable...\n");

    _SIO_Init();

    for (i = 20; i < 10000; i += 1 + (i>>6) + (i>>7))
        {
//        printf("%d\n", i);
        uBaudClock = i;

        if (STAT_CONN == (stat =_SIO_DriveStat(0)))
            break;

//        printf("stat = %04X\n", stat);
        }

    if (i >= 10000)
        {
        printf("Unable to calibrate cable\n");
        uBaudClock = 0;
        return;
        }

    for (iMin = i; i < 10000; i++)
        {
#ifndef NDEBUG
        printf("%d\n", i);
#endif
        uBaudClock = i;

        if (STAT_CONN != _SIO_DriveStat(0))
            break;
        }

    uBaudClock = (i + iMin) / 2;

    printf("Xformer cable calibration value = %d\n", uBaudClock);
}

#ifndef INXF

void main()
{
    int i;
    unsigned char rgch[260];

    printf("XF Cable Test\n\n");
#if 0
    printf("10 second delay...\n"); fflush(stdout);
    DelayCms(10000);
#endif

    _SIO_Calibrate();

    _SIO_Init();
//goto ldatain;
#if 1
Lloop:
    for (i = 1; i < 720; i++)
        {
        printf("reading sector %d...\n", i);
#if USETIMER
        uBaudClock = 16 + (i/100);
#else
        uBaudClock = 125+20*i;
        uBaudClock = 1100;
#endif
        if (SIO_OK == _SIO_DiskIO(0, i, 128, rgch, 1))
            {
            int j;

            for (j = 0; j < 128; j++)
                {
           break;
                printf("%02X ", rgch[j]);
                if ((j & 15) == 15)
                    printf("\n");
                }
            }
        else
            {
            printf("Read error\n");
       //     DelayCms(1000);
            }
        }
    if (!kbhit())
        goto Lloop;
return;
#endif
    printf("toggling DATA_OUT\n");
    for (i = 0; i < 10; i++)
        {
        SetDataOut(i & 1);
        printf("%d\n", i & 1);
        DelayCms(5000);
        }

    printf("toggling CLOCK_OUT\n");
    for (i = 0; i < 10; i++)
        {
        SetClockOut(i & 1);
        printf("%d\n", i & 1);
        DelayCms(5000);
        }

    printf("toggling COMMAND\n");
    for (i = 0; i < 5; i++)
        {
        CommandLo();
        printf("0\n");
        DelayCms(5000);
        CommandHi();
        printf("1\n");
        DelayCms(5000);
        }

    printf("reading DATA_IN\n");
    for (i = 0; i < 10; i++)
        {
        printf("DATA_IN = %d\n", GetDataIn());
        DelayCms(1000);
        }

ldatain:
    for (i = 0; i < 10000; i++)
        {
        printf("DATA_IN = %d\n", GetDataIn());
        }

    _SIO_UnInit();

}

#endif

#endif // !HWIN32

#endif // XFORMER

