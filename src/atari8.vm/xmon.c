
/***************************************************************************

    XMON.C

    - 6502 debugger (based on original ST Xformer debugger)

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

// !!! This monitor should be made generic for any processor, right now it's ATARI 6502 specific and so is part of the ATARI VM

#include "gemtypes.h"
#include "atari800.h"

#ifdef XFORMER

#pragma optimize ("",off)

/* monitor screen size */
#define XCOMAX  78
#define YCOMAX  22
#define HEXCOLS 16

// globals are not thread safe, but only one is up at a time

WORD uMemDump, uMemDasm;
char const rgchHex[] = "0123456789ABCDEF";

char const szCR[] = "\n";

char rgchIn[256],     /* monitor line input buffer */
     rgchOut[256],
     //ch,              /* character at rgch[ich] */
     fHardCopy;       /* if non-zero dumps to printer */

int  cchIn,           /* cch of buffer */
     ichIn,           /* index into buffer character */
     cchOut;          /* size of output string */

char chLast;            // the last command

//int fMON;       /* TRUE if we are in 6502 monitor */


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

char *Blit(char *pchFrom, char *pchTo)
{
    while (*pchFrom)
        *pchTo++ = *pchFrom++;
    return pchTo;
}

char *Blitb(char *pchFrom, char *pchTo, unsigned cb)
{
    while (cb--)
        *pchTo++ = *pchFrom++;
    return pchTo;
}

char* Blitcz(char ch, char *pchTo, int cch)
{
    while (cch--)
            *pchTo++ = ch;
    return pchTo;
}

void Bconout(int dev, int ch)
{
    dev;

    printf("%c",ch);
}

void Cconws(char *pch)
{
    while (*pch)
        Bconout(2,*pch++);
}

void outchar(char ch)
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

void OutPchCch(char *pch, int cch)
{
    while (cch--)
        outchar(*pch++);
}

void GetLine()
{
    cchIn = 0;      /* initialize input line cchIngth to 0 */
    Bconout(2,'>');

    ReadConsole(GetStdHandle(STD_INPUT_HANDLE), rgchIn, sizeof(rgchIn), (LPDWORD)&cchIn, NULL);

    if (cchIn && rgchIn[cchIn-1] == 0x0D)
        cchIn--;

    Cconws((char *)szCR);
    /* terminate input line with a space and null */
    rgchIn[cchIn] = ' ';
    rgchIn[cchIn+1] = 0;
    ichIn = 0;
}

/* advance ichIn to point to non-space, return -1 if it runs out of characters, or FALSE if no skipping necessary */
int FSkipSpace()
{
    char ch;
    BOOL f = FALSE;

    while ((ichIn < cchIn) && (ch = rgchIn[ichIn]) == ' ')
    {
        ichIn++;
        f = TRUE;
    }
    return (ichIn < cchIn) ? f : -1;
}


/* returns 0-15 if a valid hex character is at rgchIn[ichIn], else -1 if bad */
/* returns -2 if character was a space or CR/LF */
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

    // un-eat the non-hex char
    ichIn--;

    return ((ch == ' ' || ch == 0x0d || ch == 0x0a) ? -2 : -1);
}

// Get 1 or 2 digit 8 bit value at rgchIn[ichIn].
// Returns FALSE if a bad char was encountered
//
unsigned int FGetByte(unsigned *pu)
{
    int x;
    int w=0, digit=0 ;

    if (FSkipSpace() == -1)
        return FALSE;
    while (((x = NextHexChar()) >= 0) && digit++ < 2)
    {
        w <<= 4 ;
        w += x;
    }
    *pu = w;
    return (x != -1 && digit > 0);
}


/* Get 16 bit value at rgchIn[ichIn]. Returns FALSE if a bad char was encountered */
unsigned int FGetWord(unsigned *pu)
{
    int x;
    int w=0, digit=0 ;

    if (FSkipSpace() == -1)
        return FALSE;
    while (((x = NextHexChar()) >= 0) && digit++ < 4)
        {
        w <<= 4 ;
        w += x;
        }
    *pu = w;
    return (x != -1 && digit > 0);
}

void XtoPch(char *pch, unsigned u)
    {
    *pch++ = rgchHex[(u>>12)&0xF];
    *pch++ = rgchHex[(u>>8)&0xF];
    *pch++ = rgchHex[(u>>4)&0xF];
    *pch++ = rgchHex[u&0xF];
    }

void BtoPch(char *pch, unsigned b)
    {
    *pch++ = rgchHex[(b>>4)&0xF];
    *pch++ = rgchHex[b&0xF];
    }

// return FALSE when somebody quits
//
BOOL __cdecl MonAtari(int iVM)            /* the 6502 monitor */
{
    char chCom;                 /* command character */
    unsigned char ch;
    int cNum, cLines;
    unsigned u1, u2;
    char *pch;
    WORD w;

    //fMON=TRUE;

    char pInst[MAX_PATH];
    pInst[0] = 0;
    CreateInstanceName(iVM, pInst);

    while (1)
    {
        // !!! Why does this printf hang waiting for a RETURN key press if I press BREAK w/o focus then again with focus?
        // or I get two BREAK keys in a row and a mysterious break.
        printf("\nPC Xformer debugger - VM #%d - %s \n", iVM, pInst);
        Cconws("Commands:\n\n");
        Cconws(" A [addr]          - trAce until addr (END to brk)\n");
        Cconws(" B [addr]          - Breakpoint at addr\n");
        Cconws(" C                 - Cold start the Atari 800\n");
        Cconws(" D [addr]          - Disassemble at addr\n");
        Cconws(" G [addr]          - Go from address\n");
        Cconws(" M [addr1] [addr2] - Memory dump at addr\n");
        Cconws(" S [addr]          - Single step from addr\n");
        Cconws(" T [addr]          - Trace one page from addr\n");
        Cconws(" P [addr] [val]    - Poke\n");
        Cconws(" .                 - show registers\n");
        Cconws(" X                 - exit\n");
        Cconws(" ?                 - repeat this menu\n");
        Cconws("\n");
        Cconws(" When in Atari mode press PAUSE to get back to the debugger");
        Cconws("\n");


        w = regPC;
        CchDisAsm(iVM, &w); // don't alter the actual PC
        CchShowRegs(iVM);
        uMemDasm = regPC;    // 'd' and 'm' dumps will start here
        uMemDump = regPC;
        chLast = 0;            // forget whatever the last cmd was

        while (1)
        {
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
            GetLine();
            if (FSkipSpace() == -1)             /* skip any leading spaces */
                continue;

            chCom = rgchIn[ichIn++];      /* get command character */
            
            // blank means repeat the last command letter
            if (chCom == 0x0d && chLast && chLast != 0x0d)
                chCom = chLast;
            chLast = chCom;
                
            if ((chCom >= 'a') && (chCom <= 'z'))
                chCom -= 32;

            pch = rgchOut;

            // all done debugging (for now)
            if (chCom == 'X')
            {
                // clear all the breakpoints, or all VMs will stutter along even w/o the debugger window
                for (int i = 0; i < MAX_VM; i++)
                    if (vrgcandy[i])
                        vrgcandy[i]->m_bp = 0xffff;

                fTrace = FALSE;
                vi.fInDebugger = FALSE;
                return FALSE;    // all done
            }
            
            else if (chCom == '?')
            {
                break;
            }

            /* Can't use a switch statement because that calls Binsrch */
            else if (chCom == 'M')
            {
                if (FGetWord(&u1))
                {
                    uMemDump = u1 & 0xffff;
                    // there was space in between then a valid 2nd number
                    if (FSkipSpace() == TRUE && FGetWord(&u2))
                    {
                        u2 = (u2 & 0xffff);
                        if (u2 < uMemDump)
                            u2 = uMemDump;
                    }
                    else
                    {
                        u2 = uMemDump + 16 * HEXCOLS;
                        if (u2 > 0xffff)
                            u2 = 0xffff;
                    }
                }
                else
                {
                    // continue where we left off last time
                    u2 = uMemDump + 16 * HEXCOLS;
                    if (u2 > 0xffff)
                        u2 = 0xffff;
                }

                do
                {
                    Blitcz(' ', rgchOut, XCOMAX);
                    rgchOut[0] = ':';
                    rgchOut[57] = '\'';
                    XtoPch((char *)&rgchOut[1], uMemDump);

                    // !!! always start a row divisible by $10
                    for (cNum = 0; cNum < HEXCOLS; cNum++)
                    {
                        if (uMemDump <= 0xffff)
                        {
                            BtoPch(&rgchOut[7 + 3 * cNum + (cNum >= HEXCOLS / 2)], ch = PeekBAtari(iVM, uMemDump));
                            rgchOut[cNum + 58] = ((ch >= 0x20) && (ch < 0x80)) ? ch : '.';
                        }

                        if (uMemDump >= u2 || (GetAsyncKeyState(VK_END) & 0x8000))
                            break;
                        uMemDump++;
                    }
                    if (GetAsyncKeyState(VK_END) & 0x8000)
                        break;
                    OutPchCch(rgchOut, 74);
                    Cconws((char *)szCR);
                } while (uMemDump < u2);
            }
            
            else if (chCom == 'D')
            {
                if (FGetWord(&u1))
                    uMemDasm = u1 & 0xffff;

                for (cNum = 0; cNum < 20; cNum++)
                {
                    if (uMemDasm <= 0xffff)
                    {
                        CchDisAsm(iVM, &uMemDasm);
                        Cconws((char *)szCR);
                    }
                    else
                        break;
                }
            }

            // set breakpoint - no arguments will remind you what it is
            else if (chCom == 'B')
            {
                if (FGetWord(&u1))
                    bp = (WORD)u1;
                printf("Breakpoint set at %04x\n", bp);
            }
            
            else if (chCom == '.')
            {
                /* dump/modify registers */

                w = regPC;
                CchDisAsm(iVM, &w); // don't alter the actual PC
                CchShowRegs(iVM);
            }

            else if (chCom == 'C')
            {
                ColdStart(v.iVM);
                w = regPC;
                CchDisAsm(iVM, &w);
                CchShowRegs(iVM);
                uMemDasm = regPC; // is this handy or mean?
            }
            else if (chCom == 'P')
            {
                /* modify memory */
                if (FGetWord(&u1))
                {
                    // if space was next, then another good number
                    while (FSkipSpace() == TRUE && FGetByte(&u2) && u1 < 0x10000)
                        PokeBAtari(iVM, u1++, (BYTE)u2);
                }
            }

            else if ((chCom == 'G') || (chCom == 'S') || (chCom == 'T') || (chCom == 'A'))
            {
                int bpT = bp;    

                cLines = (chCom == 'T') ? 20 : ((chCom == 'A') ? 1000000000 : 1);
                fTrace = (chCom != 'G');

                if (FGetWord(&u1))
                {
                    if (chCom == 'A')
                        bpT = (WORD)u1; // this is a second breakpoint
                    else
                        regPC = (WORD)u1;
                }

                if (!fTrace)
                {
                    break;
                }

                do { // do at least 1 thing, don't get stuck at the breakpoint
                    FExecVM(v.iVM, TRUE, FALSE);// execute it and show the resulting register values
                    RenderBitmap();
                    if (!wSIORTS)   // hide instructions inside our hacky SIO delay
                    {
                        w = regPC;
                        CchDisAsm(iVM, &w);          // show the next instruction that will run
                        CchShowRegs(iVM);            // !!! when interrupted, this will show the results of the first interrupt instruction!
                    }
                    if (GetAsyncKeyState(VK_END) & 0x8000)
                        break;
                } while ((--cLines) && (regPC > 0) && (regPC != bp) && (regPC != bpT));
                
                uMemDasm = regPC;    // future dasms start from here by default
            }

#ifdef NEVER
            else if (chCom == 'H')
            {
                /* set hardcopy on/off flag */
                if (FGetByte(&u1))
                    fHardCopy = (char)u1;
            }

            else if (chCom == 'M1') // duplicate
            {
                pc = addr1;          /* block memory move */
                addr2 = get_addr();
                addr3 = get_addr();
                while (addr1 <= addr2) *(mem + addr3++) = *(mem + addr1++);
            }

            else if (chCom == 'C1')
            {
                pc = addr1;          /* block memory compare */
                addr2 = get_addr();
                addr3 = get_addr();
                while (addr1 <= addr2)
                {
                    if (*(mem + addr3++) != *(mem + addr1++))
                    {
                        print(" (");
                        showaddr(addr1 - 1);
                        print(") ");
                        showhex(addr1 - 1);
                        print("   (");
                        showaddr(addr3 - 1);
                        print(") ");
                        showhex(addr3 - 1);
                        Cconws(szCR);
                    }
                }
            }

            else if (chCom == 'C2')
            {
                show_emul();       /* view virtual machine screen */
                getchar();
                show_scr();
            }
#endif
            else
                Cconws("what??\007\n"); // chime
        }
    
        // show menu again
        if (chCom == '?')
            continue;

        break;

    }

    //fMON=FALSE;
    return TRUE;
}

void CchShowRegs(int iVM)
    {
    
    printf("(%03x/%02x) ", wScan, wCycle);

    printf("PC:%04X A:%02X X:%02X Y:%02X SP:%02X P:%02X ", 
        regPC, regA, regX, regY, regSP, regP);

    printf("%c%c__%c%c%c%c  ",
        (regP & NBIT) ? 'N' : '.',
        (regP & VBIT) ? 'V' : '.',
        //(regP & BBIT) ? 'B' : '.',
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

void CchDisAsm(int iVM, WORD *puMem)
{
    char rgch[32];
    char *pch = rgch;
    unsigned char bOpcode;
    long lPackedOp;
    int md;

    if ((*puMem) > 0xffff)
        return;

    _fmemset(rgch, ' ', sizeof(rgch)-1);
    rgch[sizeof(rgch)-1] = 0;

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
        if ((*puMem) > 0xfffd)
        {
            (*puMem) += 3;
            return;
        }
        BtoPch(pch + 6, cpuPeekB(iVM, *puMem + 2));

    /* one operand */
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x0B:
        if ((*puMem) > 0xfffe)
        {
            (*puMem) += 2;
            return;
        }
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

    printf(rgch);   // must be printf, not puts, because we DON'T want a carriage return
    }

/***********************************************************************/

#endif // XFORMER

/* end of _XMON.C */

