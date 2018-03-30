
/***************************************************************************

    XMON.C

    - 6502 debugger (based on original ST Xformer debugger)

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

#pragma optimize ("",off)

/* monitor screen size */
#define XCOMAX  78
#define YCOMAX  22
#define HEXCOLS 16

unsigned int uMemDump, uMemDasm;
char const rgchHex[] = "0123456789ABCDEF";

char const szCR[] = "\n";


char rgchIn[256],     /* monitor line input buffer */
     rgchOut[256],
     ch,              /* character at rgch[ich] */
     fHardCopy;       /* if non-zero dumps to printer */

int  cchIn,           /* cch of buffer */
     ichIn,           /* index into buffer character */
     cchOut;          /* size of output string */

int fMON;       /* TRUE if we are in 6502 monitor */


/*********************************************************************/

/* format of mnemonics: 3 ascii codes of the opcode and the addressing mode:
   00 - implied     01 - immediate     02 - zero page         03 - zero page,x
   04 - zero page,y 05 - (zero page,x) 06 - (zero page),y
   07 - absolute    08 - absolute,x    09 - absolute,y        0A - accumulator
   0B - relative    0C - indirect      0D - absolute indirect

   used by the 6502 monitor for disassembling code
*/

long const rglMnemonics[256] =
 {
 0x42524B00L, 0x4F524105L, 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x4F524102L,
 0x41534C02L, 0x3F3F3F00L, 0x50485000L, 0x4F524101L, 0x41534C0AL, 0x3F3F3F00L,
 0x3F3F3F00L, 0x4F524107L, 0x41534C07L, 0x3F3F3F00L, 0x42504C0BL, 0x4F524106L,
 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x4F524103L, 0x41534C03L, 0x3F3F3F00L,
 0x434C4300L, 0x4F524109L, 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x4F524108L,
 0x41534C08L, 0x3F3F3F00L, 0x4A535207L, 0x414E4405L, 0x3F3F3F00L, 0x3F3F3F00L,
 0x42495402L, 0x414E4402L, 0x524F4C02L, 0x3F3F3F00L, 0x504C5000L, 0x414E4401L,
 0x524F4C0AL, 0x3F3F3F00L, 0x42495407L, 0x414E4407L, 0x524F4C07L, 0x3F3F3F00L,
 0x424D490BL, 0x414E4406L, 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x414E4403L,
 0x524F4C03L, 0x3F3F3F00L, 0x53454300L, 0x414E4409L, 0x3F3F3F00L, 0x3F3F3F00L,
 0x3F3F3F00L, 0x414E4408L, 0x524F4C08L, 0x3F3F3F00L, 0x52544900L, 0x454F5205L,
 0x4C53520AL, 0x3F3F3F00L, 0x4A4D5007L, 0x454F5202L, 0x4C535202L, 0x3F3F3F00L,
 0x50484100L, 0x454F5201L, 0x4C53520AL, 0x3F3F3F00L, 0x4A4D5007L, 0x454F5207L,
 0x4C535207L, 0x3F3F3F00L, 0x4256430BL, 0x454F5206L, 0x3F3F3F00L, 0x3F3F3F00L,
 0x3F3F3F00L, 0x454F5203L, 0x4C535203L, 0x3F3F3F00L, 0x434C4900L, 0x454F5209L,
 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x454F5208L, 0x4C535208L, 0x3F3F3F00L,
 0x52545300L, 0x41444305L, 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x41444302L,
 0x524F5202L, 0x3F3F3F00L, 0x504C4100L, 0x41444301L, 0x524F520AL, 0x3F3F3F00L,
 0x4A4D500CL, 0x41444307L, 0x524F5207L, 0x3F3F3F00L, 0x4256530BL, 0x41444306L,
 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x41444303L, 0x524F5203L, 0x3F3F3F00L,
 0x53454900L, 0x41444309L, 0x3F3F3F00L, 0x3F3F3F00L, 0x3F3F3F00L, 0x41444308L,
 0x524F5208L, 0x3F3F3F00L, 0X3F3F3F00L, 0X53544105L, 0X3F3F3F00L, 0X3F3F3F00L,
 0X53545902L, 0X53544102L, 0X53545802L, 0X3F3F3F00L, 0X44455900L, 0X3F3F3F00L,
 0X54584100L, 0X3F3F3F00L, 0X53545907L, 0X53544107L, 0X53545807L, 0X3F3F3F00L,
 0X4243430BL, 0X53544106L, 0X3F3F3F00L, 0X3F3F3F00L, 0X53545903L, 0X53544103L,
 0X53545804L, 0X3F3F3F00L, 0X54594100L, 0X53544109L, 0X54585300L, 0X3F3F3F00L,
 0X3F3F3F00L, 0X53544108L, 0X3F3F3F00L, 0X3F3F3F00L, 0X4C445901L, 0X4C444105L,
 0X4C445801L, 0X3F3F3F00L, 0X4C445902L, 0X4C444102L, 0X4C445802L, 0X3F3F3F00L,
 0X54415900L, 0X4C444101L, 0X54415800L, 0X3F3F3F00L, 0X4C445907L, 0X4C444107L,
 0X4C445807L, 0X3F3F3F00L, 0X4243530BL, 0X4C444106L, 0X3F3F3F00L, 0X3F3F3F00L,
 0X4C445903L, 0X4C444103L, 0X4C445804L, 0X3F3F3F00L, 0X434C5600L, 0X4C444109L,
 0X54535800L, 0X3F3F3F00L, 0X4C445908L, 0X4C444108L, 0X4C445809L, 0X3F3F3F00L,
 0X43505901L, 0X434D5005L, 0X3F3F3F00L, 0X3F3F3F00L, 0X43505902L, 0X434d5002L,
 0X44454302L, 0X3F3F3F00L, 0X494E5900L, 0X434D5001L, 0X44455800L, 0X3F3F3F00L,
 0X43505907L, 0X434D5007L, 0X44454307L, 0X3F3F3F00L, 0X424E450BL, 0X434D5006L,
 0X3F3F3F00L, 0X3F3F3F00L, 0X3F3F3F00L, 0X434D5003L, 0X44454303L, 0X3F3F3F00L,
 0X434C4400L, 0X434D5009L, 0X3F3F3F00L, 0X3F3F3F00L, 0X3F3F3F00L, 0X434D5008L,
 0X44454308L, 0X3F3F3F00L, 0X43505801L, 0X53424305L, 0X3F3F3F00L, 0X3F3F3F00L,
 0X43505802L, 0X53424302L, 0X494E4302L, 0X3F3F3F00L, 0X494E5800L, 0X53424301L,
 0X4E4F5000L, 0X3F3F3F00L, 0X43505807L, 0X53424307L, 0X494E4307L, 0X3F3F3F00L,
 0X4245510BL, 0X53424306L, 0X3F3F3F00L, 0X3F3F3F00L, 0X3F3F3F00L, 0X53424303L,
 0X494E4303L, 0X3F3F3F00L, 0X53454400L, 0X53424309L, 0X3F3F3F00L, 0X3F3F3F00L,
 0X3F3F3F00L, 0X53424308L, 0X494E4308L, 0X3F3F3F00L
 } ;

char *Blit(pchFrom, pchTo)
char *pchFrom, *pchTo;
    {
    while (*pchFrom)
        *pchTo++ = *pchFrom++;
    return pchTo;
    }

char *Blitb(pchFrom, pchTo, cb)
char *pchFrom, *pchTo;
unsigned cb;
    {
    while (cb--)
        *pchTo++ = *pchFrom++;
    return pchTo;
    }

char* Blitcz(ch, pchTo, cch)
char ch, *pchTo;
int cch;
    {
    while (cch--)
            *pchTo++ = ch;
    return pchTo;
    }


#ifndef HWIN32

int Bconin(dev)
int dev;  /* device, unused */
    {
    int wKey;

       /* If first wKey is 0, then get second extended. */
       wKey = getch();
       if( (wKey == 0) || (wKey == 0xe0) )
               wKey = getch();

    return wKey;
    }

#endif

void Bconout(dev, ch)
int dev;
int ch;
    {
    printf("%c",ch);
#ifndef HWIN32
    fflush(stdout);
#endif
    }

void Cconws(pch)
char *pch;
    {
    while (*pch)
        Bconout(2,*pch++);
    }

void outchar(ch)
char ch ;
    {
#ifdef LATER
    if (ch & 0x80)
        {
        Cconws("\033p");
        Bconout (5,ch & 0x7F);
        Cconws("\033q");
        }
    else
#endif
        Bconout (5,ch);

#ifdef LATER

    /* check if hardcopy on and printer ready */
    if ((fHardCopy) && (Bconstat(0)))
        Bconout (0,ch) ;

    if ((Bconstat(2)!=0) && ((char)(Bconin(2))==' '))
        Bconin(2);

#endif
    }

void OutPchCch(pch, cch)
char *pch;
int cch;
    {
    while (cch--)
        outchar(*pch++);
    }

void GetLine()
    {
    long key;     /* return value of Bconin */
    int wChar;

    cchIn = 0;      /* initialize input line cchIngth to 0 */
    Bconout(2,'>');

#ifdef HWIN32
    ReadConsole(GetStdHandle(STD_INPUT_HANDLE), rgchIn, sizeof(rgchIn), &cchIn, NULL);

    if (cchIn && rgchIn[cchIn-1] == 0x0D)
        cchIn--;
#else
    while (1)
        {
        key = Bconin(2);
        wChar = (int)key;

#ifdef NEVER
        /* convert to uppercase */
        if (wChar>='a' && wChar<='z')
            wChar -= 32;
#endif
        /* if it's printable then print it and store it */
        if (wChar>=' ' && wChar < '~')
            {
            Bconout(2,wChar);
            rgchIn[cchIn++] = wChar;
            }

        /* if Backspace, delete last char */
        else if (wChar == 8 && cchIn>0)
            {           
            Cconws("\b \b");
            cchIn--;
            }

        /* Esc clears line */
        else if (wChar == 27)
            {
            Cconws("\033l>");
            cchIn = 0;
            }

#ifdef LATER
        /* if special key, get scan code */
        if (cchIn==0 && ch==0) {                /* if special key */
      ch = (char) (key>>16) ;             /* get scan code */
      if (ch==0x47) cls() ;                 /* is it Home? */
      if (ch==0x62) help() ;              /* is it Help? */
      break ;                             /* break out of loop */
      }
#endif
        /* stay in loop until rgchInfer full or Return pressed */
        if ((cchIn==XCOMAX) || (wChar==13 && cchIn))
            break;
        }
#endif // HWIN32

    Cconws(szCR);
    /* terminate input line with a space and null */
    rgchIn[cchIn] = ' ';
    rgchIn[cchIn+1] = 0;
    ichIn = 0;
    }

/* advance ichIn to point to non-space */
int FSkipSpace()
    {
    char ch;
    while ((ch = rgchIn[ichIn]) == ' ' && ichIn<cchIn)
        ichIn++ ;
    return (ichIn < cchIn);
    }


/* returns 0-15 if a valid hex character is at rgchIn[ichIn], else -1 */
/* returns -2 if character was a space */
int NextHexChar()
    {
    char ch;

    ch = rgchIn[ichIn++];

    if (ch>='0' && ch<='9')
        return ch-'0';
    if (ch>='A' && ch<='F')
        return ch-'A'+10;
    if (ch>='a' && ch<='f')
        return ch-'a'+10;

    return ((ch == ' ') ? -2 : -1);
    }

/* Get 8 bit value at rgchIn[ichIn]. Returns TRUE if valid number */
unsigned int FGetByte(pu)
unsigned *pu;
    {
    int x;
    int w=0, digit=0 ;

    if (!FSkipSpace())
        return FALSE;
    while (((x = NextHexChar()) >= 0) && digit++ < 2)
        {
        w <<= 4 ;
        w += x;
        }
    *pu = w;
    return (x != -1);
    }


/* Get 16 bit value at rgchIn[ichIn]. Returns TRUE if valid number */
unsigned int FGetWord(pu)
unsigned *pu;
    {
    int x;
    int w=0, digit=0 ;

    if (!FSkipSpace())
        return FALSE;
    while (((x = NextHexChar()) >= 0) && digit++ < 4)
        {
        w <<= 4 ;
        w += x;
        }
    *pu = w;
    return (x != -1);
    }

void XtoPch(pch, u)
char *pch;
unsigned int u;
    {
    *pch++ = rgchHex[(u>>12)&0xF];
    *pch++ = rgchHex[(u>>8)&0xF];
    *pch++ = rgchHex[(u>>4)&0xF];
    *pch++ = rgchHex[u&0xF];
    }

void BtoPch(pch, b)
char *pch;
unsigned int b;
    {
    *pch++ = rgchHex[(b>>4)&0xF];
    *pch++ = rgchHex[b&0xF];
    }

void mon(int iVM)            /* the 6502 monitor */
    {
    char chCom;                 /* command character */
    char ch;
    int cNum, cLines;
    unsigned u1, u2;
    char *pch;

    fMON=TRUE;

    Cconws ("\nPC Xformer debugger commands:\n\n");
    Cconws (" A [addr]          - run until PC at address\n");
    Cconws (" B                 - reboot the Atari 800\n");
    Cconws (" D [addr]          - disassemble at address\n");
    Cconws (" G [addr]          - run at address\n");
    Cconws (" M [addr1] [addr2] - memory dump at address\n");
    Cconws (" S [addr]          - single step one instruction\n");
    Cconws (" T [addr]          - trace instructions\n");
    Cconws (" X                 - exit back to DOS\n");
    Cconws ("\n");
#ifdef HWIN32
    Cconws (" When in Atari mode press Ctrl+Alt+F11 to get back to the debugger");
#else
    Cconws (" When in Atari mode press Shift+F5 to get back to the debugger");
#endif
    Cconws ("\n");

    while(1)
        {
        GetLine();
        if (!FSkipSpace())             /* skip any leading spaces */
            continue;

        chCom = rgchIn[ichIn++] ;      /* get command character */

        if ((chCom >= 'a') && (chCom <= 'z'))
            chCom -= 32;

        if (chCom == 'X')
            {
            break;
            }

        if (chCom == ';')
            continue;

        pch = rgchOut;

        /* Can't use a switch statement because that calls Binsrch */
        if (chCom == 'M')
            {
            if (FGetWord(&u1))
                {
                uMemDump = u1;
                if (FGetWord(&u2))
                    {
                    u2++;
                    }
                else
                    u2 = u1 + HEXCOLS;
                }
            else
                {
                u2 = uMemDump + 16*HEXCOLS;
                }

            do
                {
                Blitcz(' ', rgchOut, XCOMAX);
                rgchOut[0] = ':';
                rgchOut[57] = '\'';
                XtoPch((char *)&rgchOut[1], uMemDump);

                for (cNum=0; cNum<HEXCOLS; cNum++)
                    {
                    BtoPch(&rgchOut[7 + 3*cNum + (cNum>=HEXCOLS/2)],
                          ch = cpuPeekB(iVM, uMemDump++));
                    rgchOut[cNum+58] = ((ch >= 0x20) && (ch < 0x80)) ? ch : '.';
                    if (uMemDump == u2)
                        break;
                    }
                OutPchCch(rgchOut,74);
                Cconws(szCR);
                } while (uMemDump != u2);
            }
        else if (chCom == 'D')
            {
            if (FGetWord(&u1))
                uMemDasm = u1;

            for (cNum=0; cNum<20; cNum++)
                {
                CchDisAsm(iVM, &uMemDasm);
                Cconws(szCR);
                }
            }

        else if (chCom == '.')
            {
            /* dump/modify registers */

            CchShowRegs(iVM);
            }

        else if (chCom == 'H')
            {
            /* set hardcopy on/off flag */
            if (FGetByte(&u1))
                fHardCopy = u1;
            }
        else if (chCom == 'B')
            {
            FColdbootVM(v.iVM);
            FExecVM(v.iVM, FALSE,TRUE);
            CchShowRegs(iVM);
            }
        else if (chCom == ':')
            {
            /* modify memory */
            if (!FGetWord(&u1))
                Cconws("invalid address");
            else while (FGetByte(&u2))
                cpuPokeB(iVM, u1++, u2);
            }
        else if (chCom == 'M')
            {
#ifdef NEVER
            pc = addr1 ;          /* block memory move */
            addr2 = get_addr() ;
            addr3 = get_addr() ;
            while (addr1<=addr2) *(mem+addr3++) = *(mem+addr1++) ;
#endif
            }
        else if (chCom == 'C')
            {
#ifdef NEVER
            pc = addr1 ;          /* block memory compare */
            addr2 = get_addr() ;
            addr3 = get_addr() ;
            while (addr1<=addr2)
                {
                if (*(mem+addr3++) != *(mem+addr1++))
                    {
                    print(" (");
                    showaddr(addr1-1);
                    print(") ");
                    showhex(addr1-1);
                    print("   (");
                    showaddr(addr3-1);
                    print(") ");
                    showhex(addr3-1);
                    Cconws(szCR);
                    }
                }
#endif
            }
        else if (chCom == 'C')
            {
#ifdef NEVER
            show_emul() ;       /* view virtual machine screen */
            getchar() ;
            show_scr() ;
#endif
            }
        else if ((chCom == 'G') ||
                (chCom == 'S') || (chCom == 'T') || (chCom == 'A'))
            {
            unsigned int u;
            WORD bp = 0; 

            cLines = (chCom == 'T') ? 20 : ((chCom == 'A') ? 270000 : 1);
            fTrace = (chCom != 'G');

            if (FGetWord(&u1))
                {
                if (chCom == 'A')
                    bp = u1;
                else
                    regPC = u1;
                }

            if (!fTrace)
                {
                break;
                }

            while ((cLines--) && (regPC >= 0x200) && (regPC != bp))
                {
                u = regPC;
                CchDisAsm(iVM, &u);
                FExecVM(v.iVM, TRUE,FALSE);
                CchShowRegs(iVM);
                }
            }
        else
            Cconws("what??\007\n");
        }

    fMON=FALSE;
    }

void CchShowRegs(int iVM)
    {
    printf("PC:%04X A:%02X X:%02X Y:%02X SP:%02X P:%02X ", 
        regPC, regA, regX, regY, regSP, regP);

    printf("%c%c_%c%c%c%c%c  ",
        (regP & NBIT) ? 'N' : '.',
        (regP & VBIT) ? 'V' : '.',
        (regP & BBIT) ? 'B' : '.',
        (regP & DBIT) ? 'D' : '.',
        (regP & IBIT) ? 'I' : '.',
        (regP & ZBIT) ? 'Z' : '.',
        (regP & CBIT) ? 'C' : '.');

    printf("%02X %02X %02X\n",
        cpuPeekB(iVM, ((regSP + 1) & 255) + 0x100),
        cpuPeekB(iVM, ((regSP + 2) & 255) + 0x100),
        cpuPeekB(iVM, ((regSP + 3) & 255) + 0x100));
    }


/* Disassemble instruction at location uMem to space filled buffer pch. */
/* Returns with puMem incremented appropriate number of bytes. */

void CchDisAsm(int iVM, unsigned int *puMem)
{
    char rgch[32];
    char *pch = rgch;
    unsigned char bOpcode;
    long lPackedOp;
    char *pch0 = pch;
    int md;

    _fmemset(rgch, ' ', sizeof(rgch)-1);
    rgch[sizeof(rgch)-1] = 0;

    *pch++ = ',';
    XtoPch(pch, *puMem);
    pch += 5;

    /* get opcode */
    bOpcode = cpuPeekB(iVM, *puMem);
    BtoPch(pch, bOpcode);

    /* get packed opcode mnemonic and addressing mode */
    lPackedOp = rglMnemonics[bOpcode] ;

    /* addressing mode is LSB of long */
    md = (int)(lPackedOp & 0x0FL);

    /* first hex dump the bytes of the instruction */
    switch (md)
        {
    /* 2 operands */
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0C:
        BtoPch(pch + 6, cpuPeekB(iVM, *puMem + 2));

    /* one operand */
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x0B:
        BtoPch(pch + 3, cpuPeekB(iVM, *puMem + 1));

    /* no operands */
    case 0x00:
    case 0x0A:
        pch += 10;
        break;
        }

    *puMem += 1;

    /* now dump the mnemonic */
    *pch++ = (char) (lPackedOp >>24) & 0x7F;
    *pch++ = (char) (lPackedOp >>16) & 0x7F;
    *pch++ = (char) (lPackedOp >> 8) & 0x7F;
    *pch++ = ' ';

    switch (md)
        {
    case 0x00:
        break;

    case 0x01:
        pch = Blit("#$", pch);
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        *puMem += 1;
        pch += 2;
        break;

    case 0x02:
        *pch++ = '$';
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        *puMem += 1;
        break;

    case 0x03:
        *pch++ = '$';
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        pch = Blit(",X",pch);
        *puMem += 1;
        break;

    case 0x04:
        *pch++ = '$';
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        pch = Blit(",Y",pch);
        *puMem += 1;
        break;

    case 0x05:
        pch = Blit("($", pch);
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        pch = Blit(",X)", pch);
        *puMem += 1;
        break;

    case 0x06:
        pch = Blit("($", pch);
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        pch = Blit("),Y", pch);
        *puMem += 1;
        break;

    case 0x07:
        *pch++ = '$';
        BtoPch(pch, cpuPeekB(iVM, *puMem + 1));
        pch += 2;
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        *puMem += 2;
        break;

    case 0x08:
        *pch++ = '$';
        BtoPch(pch, cpuPeekB(iVM, *puMem + 1));
        pch += 2;
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        *puMem += 2;
        pch = Blit(",X", pch);
        break;

    case 0x09:
        *pch++ = '$';
        BtoPch(pch, cpuPeekB(iVM, *puMem + 1));
        pch += 2;
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        *puMem += 2;
        pch = Blit(",Y", pch);
        break;

    case 0x0A:
        *pch++ = 'A';
        break;

    case 0x0B:
        {
        unsigned uMem;

        *pch++ = '$';
        uMem = (*puMem + 1 + (int)((char)cpuPeekB(iVM, *puMem)));
        BtoPch(pch, uMem>>8);
        pch += 2;
        BtoPch(pch, (char)uMem);
        pch += 2;
        *puMem += 1;
        }
        break;

    case 0x0C:

        pch = Blit("($", pch);
        BtoPch(pch, cpuPeekB(iVM, *puMem + 1));
        pch += 2;
        BtoPch(pch, cpuPeekB(iVM, *puMem));
        pch += 2;
        *pch++ = ')';
        *puMem += 2;
        break;
        }

    printf(rgch);
    }

/***********************************************************************/

#endif // XFORMER

/* end of _XMON.C */

