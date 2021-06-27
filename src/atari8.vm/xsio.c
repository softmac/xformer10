
/***************************************************************************

    XSIO.C

    - Atari serial port emulation

    Copyright (C) 1986-2021 by Darek Mihocka and Danny Miller. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release
    09/27/2000  darekm      last update

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

#define DEBUGCOM 0

//
// SIO stuff
//

/* SIO return codes */
#define SIO_OK      0x01
#define SIO_TIMEOUT 0x8A
#define SIO_NAK     0x8B
#define SIO_DEVDONE 0x90

#define MD_OFF  0
#define MD_SD   1
#define MD_ED   2
#define MD_DD   3            // normally, the first 3 sectors are 128 bytes long, then 768 bytes of blank, and then sector 4
#define MD_DD_OLD_ATR1 4    // early .ATR files have the first half of the first three 256 byte sectors filled
#define MD_DD_OLD_ATR2 5    // other early .ATR files sector 4 start at offset 384 w/o blank space
#define MD_QD   6
#define MD_HD   7
#define MD_RD   8
#define MD_35   9
#define MD_EXT  10
#define MD_FILE 11          // an unknown PC File mounted as an ATARI DOS File for them to see
#define MD_FILEBIN 12       // a PC file that is an ATARI binary, mounted as an ATARI DOS file and also as an auto-boot
#define MD_FILEBAS 13       // a PC file that is maybe an ATARI BASIC file, mounted in ATARI DOS and auto-booting

// !!! scary globals that don't seem to be used anymore
int wCOM;
int fXFCable;

// This code is borrowed from an old exe loader, but doesn't show the menu and let you select an exe, it auto-loads
// the first one, and doesn't alter the display list since some apps don't make their own DL but assume GR.0 is
// already set up for them.
//
// Also, there was a bug, if a very short segment was loaded, two different initializations would happen during
// the same sector JMP ($2E2) and each would DEC CRITIC, so CRITIC was $FF and interrupts were disabled and could hang 
// the app during the 2nd code portion (Ridiculous Reality).
//
// Since there is no source code to this program (I typed it in in hex) I should probably say a few things about it.
// Some apps (Capt. Sticky) set DOSVEC and return from ($2E0) but aren't supposed to, so I have to jmp ($a) if they do that.
// This loader only occupies $700 to $87f, it loads all sectors into $a00, which seems dangerous ($400 is the safest place)
// but some stupid games (gunfighter) seem to only work under emulation expecting an SIO hack to not use $400 for sector 
// loading, as they put data there they expect to stick around after more sectors are loaded! In other words, so many
// programs expect the binary loader to occupy $700-$87F and to load sectors in $a00, that's what I have to do as my first try.
// !!! Also, to support quad density binary files, I treat the sector number as 12 bits, even though DOS only support 10 bits.
// I borrow the bottom two bits of the file #. These files can't be easily copied to an ATR image either for this emulator
// or for real hardware (the image is too big plus this hack breaks DOS 2.0)

// Some apps expect the jump vector $c to be initialized to $e4c0 but loading our fake loader changes it to our run address,
// so we have to put it back (@ $858) (AT_ARTIS)
// Also, @$720 we do a SEC because even though we have hacked our SIO hook to set the carry after it finished, my loader might
// clear it again before jumping through ($2e0) which isn't supposed to happen (Barahir.xex)

const BYTE Bin1[128] = {
    0x00, 0x03, 0x00, 0x07, 0x58, 0x08, 0x18, 0x60, 0xA9, 0x00, 0x8D, 0x44, 0x02, 0xA8, 0x99, 0x80,
    0x08, 0x88, 0xD0, 0xFA, 0xC8, 0x84, 0x09, 0x8C, 0x01, 0x03, 0xCE, 0x06, 0x03, 0x4c, 0x2c, 0x07,
    0x38, 0x20, 0xf9, 0x07, 0x80, 0x00, 0xe6, 0x42, 0x60, 0xea, 0xea, 0xea, 0xa9, 0x69, 0x85, 0x18,
    0xA5, 0x18, 0x8D, 0x0A, 0x03, 0xA9, 0x01, 0x8D, 0x0B, 0x03, 0xE6, 0x18, 0x20, 0x0D, 0x08, 0xB9,
    0x00, 0x0a, 0xF0, 0x47, 0x30, 0x3B, 0xA6, 0x47, 0xB9, 0x03, 0x0a, 0x95, 0x5A, 0xB9, 0x04, 0x0a,
    0x95, 0x6E, 0x8A, 0x18, 0x69, 0x91, 0xA6, 0x48, 0x9D, 0x80, 0x08, 0xA9, 0x0B, 0x85, 0x49, 0xB9,
    0x05, 0x0a, 0x38, 0xE9, 0x20, 0x9D, 0x82, 0x08, 0xC8, 0xE8, 0xC6, 0x49, 0xD0, 0xF1, 0x98, 0x38,
    0xE9, 0x0B, 0xA8, 0x8A, 0x18, 0x69, 0x09, 0x85, 0x48, 0xE6, 0x47, 0xA5, 0x47, 0xC9, 0x09, 0xF0 };

const BYTE Bin2[128] = { 
    0x0A, 0x98, 0x18, 0x69, 0x10, 0xA8, 0x0A, 0x90, 0xB6, 0xB0, 0xA5, 0xC6, 0x42, 0x85, 0x14, 0xC5,
    0x14, 0xF0, 0xFC, 0xA9, 0x31, 0xEA, 0x38, 0xE9, 0x31, 0xC5, 0x47, 0xB0, 0xF6, 0xAA, 0xea, 0xea,
    0xea, 0xB5, 0x5A, 0x8D, 0x0A, 0x03, 0xB5, 0x6E, 0x8D, 0x0B, 0x03, 0x20, 0x21, 0x08, 0xCA, 0x20,
    0xFC, 0x07, 0x85, 0x43, 0x20, 0xFC, 0x07, 0x85, 0x44, 0x25, 0x43, 0xC9, 0xFF, 0xF0, 0xF0, 0x20,
    0xFC, 0x07, 0x85, 0x45, 0x20, 0x6c, 0x08, 0xea, 0xea, 0x20, 0xFC, 0x07, 0x91, 0x43, 0xE6, 0x43,
    0xD0, 0x02, 0xE6, 0x44, 0xA5, 0x45, 0xC5, 0x43, 0xA5, 0x46, 0xE5, 0x44, 0xB0, 0xEB, 0xAD, 0xE2,
    0x02, 0x0D, 0xE3, 0x02, 0xF0, 0xc9, 0x86, 0x19, 0xC6, 0x42, 0x20, 0x20, 0x07, 0xA6, 0x19, 0xA0,
    0x00, 0x8C, 0xE2, 0x02, 0x8C, 0xE3, 0x02, 0xF0, 0xB6, 0x6C, 0xE2, 0x02, 0xE0, 0x7D, 0xD0, 0x4A };

const BYTE Bin3[128] = {
    0xAD, 0x0A, 0x03, 0x0D, 0x0B, 0x03, 0xD0, 0x19, 0xC6, 0x42, 0x4C, 0x63, 0x08, 0xA9, 0x31, 0x8D,
    0x00, 0x03, 0xA9, 0x52, 0x8D, 0x02, 0x03, 0xA9, 0x0a, 0x8D, 0x05, 0x03, 0xA9, 0x80, 0x8D, 0x08,
    0x03, 0xA9, 0x40, 0x8D, 0x03, 0x03, 0x20, 0x59, 0xE4, 0x30, 0xF6, 0xE6, 0x42, 0xAD, 0x7D, 0x0a,
    0x29, 0x0f, 0x8D, 0x0B, 0x03, 0xAD, 0x7E, 0x0a, 0x8D, 0x0A, 0x03, 0xea, 0xea, 0xea, 0xAD, 0x7F,
    0x0a, 0x29, 0x7F, 0x8D, 0xFD, 0x07, 0xA0, 0x00, 0xA2, 0x00, 0xBD, 0x00, 0x0a, 0xE8, 0x60, 0xAD,
    0x25, 0xE4, 0x48, 0xAD, 0x24, 0xE4, 0x48, 0x60, 0xa9, 0xc0, 0x85, 0x0c, 0xa9, 0xe4, 0x85, 0x0d,
    0x4c, 0x08, 0x07, 0x20, 0x69, 0x08, 0x6c, 0x0a, 0x00, 0x6c, 0xe0, 0x02, 0x20, 0xfc, 0x07, 0x85,
    0x46, 0x80, 0x00, 0x60, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea };

// The $d680 version of the loader, residing safely in ROM where it can't possibly interfere with 
// loading. Unfortunately, something needing this loader cannot be easily copied into an ATR image
// and used on real hardware, but it couldn't anyway. It loads sectors into $d600 because any part
// of RAM, even $400, might be used by the app (Highway Duel)
// (needed for Lords.xex, etc.)

const BYTE BinA1[128] = {
    0x00, 0x03, 0x80, 0xd6, 0xd8, 0xd7, 0x18, 0x60, 0xA9, 0x00, 0x8D, 0x44, 0x02, 0xA8, 0x99, 0x80,
    0x08, 0x88, 0xD0, 0xFA, 0xC8, 0x84, 0x09, 0x8C, 0x01, 0x03, 0xCE, 0x06, 0x03, 0x4c, 0xac, 0xD6,
    0x38, 0x20, 0x79, 0xD7, 0x80, 0x00, 0xe6, 0x42, 0x60, 0xea, 0xea, 0xea, 0xa9, 0x69, 0x85, 0x18,
    0xA5, 0x18, 0x8D, 0x0A, 0x03, 0xA9, 0x01, 0x8D, 0x0B, 0x03, 0xE6, 0x18, 0x20, 0x8D, 0xD7, 0xB9,
    0x00, 0xd6, 0xF0, 0x47, 0x30, 0x3B, 0xA6, 0x47, 0xB9, 0x03, 0xd6, 0x95, 0x5A, 0xB9, 0x04, 0xd6,
    0x95, 0x6E, 0x8A, 0x18, 0x69, 0x91, 0xA6, 0x48, 0x9D, 0x80, 0x08, 0xA9, 0x0B, 0x85, 0x49, 0xB9,
    0x05, 0xd6, 0x38, 0xE9, 0x20, 0x9D, 0x82, 0x08, 0xC8, 0xE8, 0xC6, 0x49, 0xD0, 0xF1, 0x98, 0x38,
    0xE9, 0x0B, 0xA8, 0x8A, 0x18, 0x69, 0x09, 0x85, 0x48, 0xE6, 0x47, 0xA5, 0x47, 0xC9, 0x09, 0xF0 };

const BYTE BinA2[128] = {
    0x0A, 0x98, 0x18, 0x69, 0x10, 0xA8, 0x0A, 0x90, 0xB6, 0xB0, 0xA5, 0xC6, 0x42, 0x85, 0x14, 0xC5,
    0x14, 0xF0, 0xFC, 0xA9, 0x31, 0xEA, 0x38, 0xE9, 0x31, 0xC5, 0x47, 0xB0, 0xF6, 0xAA, 0xea, 0xea,
    0xea, 0xB5, 0x5A, 0x8D, 0x0A, 0x03, 0xB5, 0x6E, 0x8D, 0x0B, 0x03, 0x20, 0xa1, 0xD7, 0xCA, 0x20,
    0x7C, 0xD7, 0x85, 0x43, 0x20, 0x7C, 0xD7, 0x85, 0x44, 0x25, 0x43, 0xC9, 0xFF, 0xF0, 0xF0, 0x20,
    0x7C, 0xD7, 0x85, 0x45, 0x20, 0xec, 0xd7, 0xea, 0xea, 0x20, 0x7C, 0xd7, 0x91, 0x43, 0xE6, 0x43,
    0xD0, 0x02, 0xE6, 0x44, 0xA5, 0x45, 0xC5, 0x43, 0xA5, 0x46, 0xE5, 0x44, 0xB0, 0xEB, 0xAD, 0xE2,
    0x02, 0x0D, 0xE3, 0x02, 0xF0, 0xc9, 0x86, 0x19, 0xC6, 0x42, 0x20, 0xa0, 0xd6, 0xA6, 0x19, 0xA0,
    0x00, 0x8C, 0xE2, 0x02, 0x8C, 0xE3, 0x02, 0xF0, 0xB6, 0x6C, 0xE2, 0x02, 0xE0, 0x7D, 0xD0, 0x4A };

const BYTE BinA3[128] = {
    0xAD, 0x0A, 0x03, 0x0D, 0x0B, 0x03, 0xD0, 0x19, 0xC6, 0x42, 0x4C, 0xe3, 0xd7, 0xA9, 0x31, 0x8D,
    0x00, 0x03, 0xA9, 0x52, 0x8D, 0x02, 0x03, 0xA9, 0xd6, 0x8D, 0x05, 0x03, 0xA9, 0x80, 0x8D, 0x08,
    0x03, 0xA9, 0x40, 0x8D, 0x03, 0x03, 0x20, 0x59, 0xE4, 0x30, 0xF6, 0xE6, 0x42, 0xAD, 0x7D, 0xd6,
    0x29, 0x0f, 0x8D, 0x0B, 0x03, 0xAD, 0x7E, 0xd6, 0x8D, 0x0A, 0x03, 0xea, 0xea, 0xea, 0xAD, 0x7F,
    0xd6, 0x29, 0x7F, 0x8D, 0x7d, 0xd7, 0xA0, 0x00, 0xA2, 0x00, 0xBD, 0x00, 0xd6, 0xE8, 0x60, 0xAD,
    0x25, 0xE4, 0x48, 0xAD, 0x24, 0xE4, 0x48, 0x60, 0xa9, 0xc0, 0x85, 0x0c, 0xa9, 0xe4, 0x85, 0x0d,
    0x4c, 0x88, 0xd6, 0x20, 0xe9, 0xd7, 0x6c, 0x0a, 0x00, 0x6c, 0xe0, 0x02, 0x20, 0x7c, 0xd7, 0x85,
    0x46, 0x80, 0x00, 0x60, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea };

// BASIC loader

const BYTE Bas1[128] = {    // ca fe @$08 is our signature so we know to use BASIC
    0x00, 0x02, 0x00, 0x07, 0x0a, 0x07, 0x18, 0x60, 0xca, 0xfe, 0x20, 0x23, 0x07, 0xA2, 0x0D, 0x18,
    0xBD, 0xF2, 0x07, 0x69, 0x07, 0x95, 0x80, 0xCA, 0xBD, 0xF2, 0x07, 0x95, 0x80, 0xCA, 0x10, 0xEF,
    0x4C, 0x98, 0x07, 0xA9, 0x31, 0x8D, 0x00, 0x03, 0xA9, 0x01, 0x8D, 0x01, 0x03, 0xA9, 0x52, 0x8D,
    0x02, 0x03, 0xA9, 0x80, 0x8D, 0x08, 0x03, 0xA9, 0x00, 0x8D, 0x09, 0x03, 0x8D, 0x0B, 0x03, 0xA9,
    0xF2, 0x8D, 0x04, 0x03, 0xA9, 0x07, 0x8D, 0x05, 0x03, 0xA9, 0x04, 0x8D, 0x0A, 0x03, 0x20, 0xd0,
    0x07, 0xea, 0xea, 0xea, 0xea, 0xea, 0xA9, 0x40, 0x8D, 0x03, 0x03, 0x20, 0x59, 0xE4, 0xAD, 0x03,
    0x03, 0xC9, 0x01, 0xF0, 0x06, 0x20, 0x3E, 0xC6, 0x4C, 0x68, 0x07, 0x18, 0xAD, 0x04, 0x03, 0x85,
    0xCB, 0x69, 0x7D, 0x8D, 0x04, 0x03, 0xAD, 0x05, 0x03, 0x85, 0xCC, 0x69, 0x00, 0x8D, 0x05, 0x03 };

const BYTE Bas2[128] = {
    0xA0, 0x7E, 0xB1, 0xCB, 0x8D, 0x0A, 0x03, 0x88, 0xB1, 0xCB, 0x29, 0x03, 0x8D, 0x0B, 0x03, 0x0D,
    0x0A, 0x03, 0xD0, 0x01, 0x60, 0x4C, 0x56, 0x07, 0xA2, 0xFF, 0x9A, 0xA9, 0x0A, 0x85, 0xC9, 0xA9,
    0x00, 0x85, 0x09, 0x8D, 0xF8, 0x03, 0xA9, 0xFD, 0x8D, 0x01, 0xD3, 0xA9, 0xA9, 0x48, 0xA9, 0x79,
    0x48, 0xA9, 0xB7, 0x48, 0xA9, 0x54, 0x48, 0xA5, 0x6A, 0x10, 0x02, 0xA9, 0xA0, 0x85, 0x6A, 0x4C,
    0x94, 0xEF, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xa9, 0x40, 0x8d, 0x03, 0x03, 0x20, 0x59, 0xe4, 0xad, 0x04, 0x03, 0x18, 0x6d, 0xf4, 0x07, 0x8d,
    0x04, 0x03, 0xad, 0x05, 0x03, 0x6d, 0xf5, 0x07, 0x38, 0xe9, 0x01, 0x8d, 0x05, 0x03, 0x60, 0xea,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA };

const BYTE Bas3[128] = {
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA,
    0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA };

// is this 128 byte buffer empty?
BOOL IsEmpty(char *cc)
{
    for (int i = 0; i < 128; i++)
    {
        if (cc[i])
            return FALSE;
    }
    return TRUE;
}

BOOL GetWriteProtectDrive(void *candy, int i)
{
    return rgDrives[i].fWP;
}

BOOL SetWriteProtectDrive(void *candy, int i, BOOL fWP)
{
    WORD mode = rgDrives[i].mode;
    // you can't un-write protect a binary file masquerading as a disk image, bad things will happen
    if ((mode != MD_FILE && mode != MD_FILEBIN && mode != MD_FILEBAS) || fWP)
        rgDrives[i].fWP = (WORD)fWP;
    return rgDrives[i].fWP;
}

// make an ATARI compatible filename - 8.3 without the dot
void AtariFNFromPath(void *candy, int i, BYTE *spec)
{
    // start with every filename character a space
    int l, d, s, m;
    for (l = 0; l < 11; l++)
        rgDrives[i].name[l] = ' ';
    rgDrives[i].name[11] = 0;

    // look backwards for a '.' or '\'

    d = lstrlen((LPCSTR)spec);  // if no . found, assume dot is after the end
    l = d - 1;
    m = d;

    while (l > 0 && spec[l] != '.' && spec[l] != '\\')
        l--;
    
    // . means we found an extension for positions 8-10
    if (l > 0 && spec[l] == '.')
    {
        d = l;
        for (s = 1; s < 4 && l + s < m; s++)
        {
            if (spec[l + s] >= 'a' && spec[l + s] <= 'z')
                rgDrives[i].name[7 + s] = spec[l + s] - 'a' + 'A';    // only upper case allowed
            else if ((spec[l + s] >= 'A' && spec[l + s] <= 'Z') || (spec[l + s] >= '0' && spec[l + s] <= '9'))
                rgDrives[i].name[7 + s] = spec[l + s];
            else
                l++;    // illegal character, skip it
        }
    }

    // if not already there, go backwards to '\' (start of filename)
    while (l > 0 && spec[l] != '\\')
        l--;

    if (l > 0)
    {
        s = l + 1;
        for (l++; l < d && l - s < 8; l++)
        {
            if (spec[l] >= 'a' && spec[l] <= 'z')
                rgDrives[i].name[l - s] = spec[l] - 'a' + 'A';
            else if ((spec[l] >= 'A' && spec[l] <= 'Z') || (spec[l] >= '0' && spec[l] <= '9' && s != l))
                rgDrives[i].name[l - s] = spec[l];
            else
                s++;    // illegal character, or starts with number, skip it
        }
    }
}

void CloseDrive(void *candy, int i)
{
    if (rgDrives[i].h && rgDrives[i].h != -1)
        _close(rgDrives[i].h);
    rgDrives[i].h = 0;
}

void DeleteDrive(void *candy, int i)
{
    CloseDrive(candy, i);
    rgDrives[i].mode = MD_OFF;
}

BOOL OpenDrive(void *candy, int i)
{
    if (i < 0 || i >= MAX_DRIVES)
        return FALSE;

    if ((rgDrives[i].h > 0) && (rgDrives[i].h != -1))
        return TRUE;

    BYTE *pchPath = (BYTE *)pvm->rgvd[i].sz;
    HFILE h = _open((LPCSTR)pchPath, _O_BINARY | _O_RDWR);

    // do not alter write protect status of drive when swapping disks

    // try again read only
    if (h == -1)
    {
        h = _open((LPCSTR)pchPath, _O_BINARY | _O_RDONLY);
        if (h == -1)
        {
            rgDrives[i].mode = MD_OFF;
            return FALSE;
        }

        rgDrives[i].fWP = 1;  // file is read-only or in use
    }

    rgDrives[i].h = h;
    return TRUE;
}

BOOL AddDrive(void *candy, int i, BYTE *pchPath)
{
    int sc=0;

    if (pchPath[0] == 'd' || pchPath[0] == 'D')
        {
        if (fXFCable && pchPath[2] == ':')
            {
            rgDrives[i].mode = MD_EXT;
            return TRUE;
            }
        }

    if (!OpenDrive(candy, i))
        return FALSE;

    rgDrives[i].wSectorMac = 720;
    rgDrives[i].mode = MD_SD;
    strcpy(rgDrives[i].path, (const char *)pchPath);

    if (_read(rgDrives[i].h, &sc,2) == 2)
    {
        ULONG l;

        l = _lseek(rgDrives[i].h,0,SEEK_END);
        rgDrives[i].cb = l;
        _lseek(rgDrives[i].h,2,SEEK_SET);

        if (sc == 0x296)
        {
            // it's an SIO2PC created image

            rgDrives[i].ofs = 16;
            _read(rgDrives[i].h,&l,2);                     // size lo
            _read(rgDrives[i].h,&sc,2);                     // bytes/sec
            _read(rgDrives[i].h,(((char *)&l) + 2),2);     // size hi

            l = l << 4;
            rgDrives[i].wSectorMac = (WORD)(l / sc);

            if (sc == 256)
            {
                char cc[128];

                // Normally, the first three sectors are packed into the first 384 bytes. Then there is 384 blank bytes.
                // Then starts sector 4 at offset $300
                // !!! I don't support noticing that it's really MD_QD, but nobody seems to care
                rgDrives[i].mode = MD_DD;

                // Some old broken .ATR files have the 1st 3 sectors as the first half of the first 3 256 byte sectors
                // 128-256 will be empty only in this version. Also make sure sector 1 is not empty. We want a blank disk
                // to register as a normal DD disk, not this.
                if (_lseek(rgDrives[i].h, 128 + 16, SEEK_SET) == 128 + 16)
                    if (_read(rgDrives[i].h, cc, 128) == 128)
                        if (IsEmpty(cc))
                            if (_lseek(rgDrives[i].h, 16, SEEK_SET) == 16)
                                if (_read(rgDrives[i].h, cc, 128) == 128)
                                    if (!IsEmpty(cc))
                                        rgDrives[i].mode = MD_DD_OLD_ATR1;

                // Other old broken .ATR files have sector 4 start right at offset 384 w/o any blank space
                // 384-512 will not be blank only in this version. But if sector 4 is blank, we won't realize it's this kind of file,
                // but we can go by the fact that it's shorter than the other types of files
                if (_lseek(rgDrives[i].h, 384 + 16, SEEK_SET) == 384 + 16)
                    if (_read(rgDrives[i].h, cc, 128) == 128)
                        if (!IsEmpty(cc) || l == 0x2ce80)
                        {
                            rgDrives[i].mode = MD_DD_OLD_ATR2;
                            rgDrives[i].wSectorMac = 720;  // it looked like 718 before because it's shorter
                        }
            }

            // 128 byte sectors is either SD or ED. Some ATR files are truncated, some padded with tons of extra 0's
            else
            {
                if (rgDrives[i].wSectorMac > 720)
                    rgDrives[i].mode = MD_ED;
            }
        }
        else
        {
            // assume it's a Xformer Cable created image
            // so just check for density
       
            rgDrives[i].ofs = 0;

            char *path = rgDrives[i].path;
            int pl = (int)strlen(path);

            if (l == 368640)
            {
                rgDrives[i].mode = MD_QD;
                rgDrives[i].wSectorMac = 1440;
            }
            else if (l == 184320)
            {
                rgDrives[i].mode = MD_DD;
                rgDrives[i].wSectorMac = 720;
            }
            else if (l == 133120)
            {
                rgDrives[i].mode = MD_ED;
                rgDrives[i].wSectorMac = 1040;
            }
            
            // somehow, there are XFD files out there not the right size. We better work with them, that's our format!
            // raw disk images often have a 3 sector loader at first, seeing 0x300 means we're a raw image w/o a header
            // and therefore a good candidate for an XFD file. Also if our extension is XFD.
            else if (l == 92160 || (l < 133120 && sc == 0x300) || (pl >=3 && !_stricmp(&path[pl - 3], "xfd")))
            {
                rgDrives[i].mode = MD_SD;
                rgDrives[i].wSectorMac = 720;
            }
            
            else if ((l > 0) && (l <= 368640 /* was 88375 */))
            {
                // it might be a BASIC or EXE file that we fake up as a virtual disk

                if (sc == 0xffff)
                    rgDrives[i].mode = MD_FILEBIN;     // ATARI binary file
                else if (sc == 0x0000)
                {
                    rgDrives[i].mode = MD_FILEBAS;     // ATARI BASIC tokenized file (I hope)
                    ramtop = 0xa000;                        // we'll need the basic cartridge
                }
                else
                    rgDrives[i].mode = MD_FILE;        // who knows?

                // how many 125 byte sectors fit in a file this big, plus 3 boot sectors plus 9 directory sectors + 1 partial
                rgDrives[i].wSectorMac = (WORD)max(720, l / 125 + 13);
                rgDrives[i].fWP = 1;  // force read-only for fake disks that can't be written to
                rgDrives[i].cb = l;

                AtariFNFromPath(candy, i, pchPath);  // make an ATARI compatible filename

#if 0
                // REVIEW: why did I do this?

                while ((pchT = strstr(pch,":")) != NULL)
                    pch = pchT+1;

                while ((pchT = strstr(pch,"\\")) != NULL)
                    pch = pchT+1;
#endif

            }
            else
            {
                // invalid disk image
                rgDrives[i].mode = MD_OFF;
            }
        }
    }
    else
        rgDrives[i].ofs = 0;

    CloseDrive(candy, i);  // don't keep 64,000 file handles open all the time, we'll be clever about when to open it again
    return TRUE;
}

#define NO_TRANSLATION    32
#define LIGHT_TRANSLATION 0
#define HEAVY_TRANSLATION 16


// SIO Bare bones, get some information that's private to this file
//
void SIOGetInfo(void *candy, int drive, BOOL *psd, BOOL *ped, BOOL *pdd, BOOL *pfwp, BOOL *pfb)
{
    WORD md = rgDrives[drive].mode;

    if (psd)
        *psd = (md == MD_SD);
    if (ped)
        *ped = (md == MD_ED);
    if (pdd)
        *pdd = (md == MD_QD) || (md == MD_DD) || (md == MD_DD_OLD_ATR1) || (md == MD_DD_OLD_ATR2) || (md == MD_HD) || (md == MD_RD);
    if (pfwp)
        *pfwp = rgDrives[drive].fWP;
    if (pfb)
        *pfb = (md == MD_FILEBIN || md == MD_FILEBAS);  // we need to know if we are faking a disk image in some way
}


// SIO Bare bones read a sector, return checksum for those rolling their own and not using SIOV
// Returns FALSE if something went wrong
//
BOOL SIOReadSector(void *candy, int wDrive, BYTE *pchk)
{
    WORD wDev, wCom, wStat, wSector;
    WORD  wBytes;
    WORD wTimeout;
    WORD md;
    DRIVE *pdrive;
    ULONG lcbSector;

    wDev = (WORD)(wDrive + 0x31);
    wCom = 0x52;
    wStat = 0x40;
    wTimeout = 0x1f;
    wSector = rgSIO[2] | ((WORD)rgSIO[3] << 8);

    // don't blow up looking for the string
    if (wDrive < 0 || wDrive >= MAX_DRIVES)
        return FALSE;

    // make sure we're open, sometimes we try to be clever and close it when it's not being used for a while
    if (!OpenDrive(candy, wDrive))
        return FALSE;

    pdrive = &rgDrives[wDrive];

    md = pdrive->mode;
    
    if (pdrive->h <= 0)
        return FALSE;

    int cbSIO2PCFudge = pdrive->ofs;

    if (wSector < 1)
    {
        ODS("SIO bare read invalid sector < 1\n");
        return FALSE;
    }
    else if (wSector > pdrive->wSectorMac)
    {
        ODS("SIO bare read invalid sector > %d\n", pdrive->wSectorMac);
        return FALSE;
    }

    // The first 3 sectors of DD are SD
    if (wSector < 4 || md == MD_SD || md == MD_ED)
        wBytes = 128;
    else
        wBytes = 256;

    if (md == MD_SD || md == MD_ED)
        lcbSector = 128L;
    else if ((wSector < 4) && pdrive->ofs)  // SIO2PC disk image
    {
        lcbSector = 128L;
        // the data is in the first half of a 256 byte sector
        if (md == MD_DD_OLD_ATR1)
            lcbSector = 256;
    }
    else if (pdrive->ofs)
    {
        lcbSector = 256L;
        if (pdrive->cb == 184720)    // !!! 400 byte header instead of 16, what is this?
            cbSIO2PCFudge += 384;

        // the first 3 sectors were compacted
        else if (md == MD_DD_OLD_ATR2)
        {
            wSector -= 1;
            cbSIO2PCFudge -= 128;
        }
    }
    else
        lcbSector = 256L;

    _lseek(pdrive->h, (ULONG)((wSector - 1) * lcbSector) + cbSIO2PCFudge, SEEK_SET);

    if (_read(pdrive->h, sectorSIO, wBytes) < wBytes)
        return 0;

    // now do the checksum
    WORD ck = 0;
    for (int i = 0; i < wBytes; i++)
    {
        ck += sectorSIO[i];
        if (ck > 0xff)
        {
            ck = ck & 0xff;
            ck++;    // add carry back in after every addition
        }
    }
    ck = ck & 0xff;

    if (pchk)
        *pchk = (BYTE)ck;
    return TRUE;
}


// ATARI 800 serial I/O handler
//
void SIOV(void *candy)
{
    WORD wDev, wDrive, wCom, wStat, wBuff, wSector, bAux1, bAux2;
    WORD  wBytes;
    WORD wTimeout;
    WORD fDD;
    WORD wRetStat = SIO_OK;
    WORD md;
    DRIVE *pdrive;
    ULONG lcbSector;
    BYTE rgb[256];
    WORD i;

#if 0   // interferes with binary loader patch, see SIOCheck
    // we're the 800 SIO routine. Otherwise, BUS1 is the XL/XE version
    if (regPC != 0xE459 && regPC != 0xE959)
    {
        BUS1(candy);
        return;
    }
#endif

#if 0
    printf("Device ID = %2x\n", cpuPeekB(0x300));
    printf("Drive # = %2x\n", cpuPeekB(0x301));
    printf("Command = %2x\n", cpuPeekB(0x302));
    printf("SIO Command = %2x\n", cpuPeekB(0x303));
    printf("Buffer = %2x\n", cpuPeekW(0x304));
    printf("Timeout = %2x\n", cpuPeekW(0x306));
    printf("Byte count = %2x\n", cpuPeekW(0x308));
    printf("Sector = %2x\n", cpuPeekW(0x30A));
    printf("Aux1 = %2x\n", cpuPeekB(0x30A));
    printf("Aux2 = %2x\n", cpuPeekB(0x30B));
#endif

    wDev = cpuPeekB(candy, 0x300);
    wDrive = cpuPeekB(candy, 0x301)-1;
    wCom = cpuPeekB(candy, 0x302);
    wStat = cpuPeekB(candy, 0x303);
    wBuff = cpuPeekW(candy, 0x304);
    wTimeout = cpuPeekW(candy, 0x306);
    wBytes = cpuPeekW(candy, 0x308);
    wSector = cpuPeekW(candy, 0x30A);
    bAux1 = cpuPeekB(candy, 0x30A);
    bAux2 = cpuPeekB(candy, 0x30B);

    // make sure we're open, sometimes we try to be clever and close it when it's not being used for a while
    if (!OpenDrive(candy, wDrive))
        goto lCable;    // returns error code SIO_TIMEOUT

#if 0
    if (wCom == 0x52)
        ODS("SIOV: Read SECTOR %d into %04x\n", wSector, wBuff);
    else
        ODS("SIOV: Command %02x\n", wCom);
#endif

    if (wDev == 0x31)       /* disk drives */
    {
#if 0
        // SIO is supposed to copy the drive and aux bytes of the last command into CDEVIC (Alternate Reality - Dungeon)
        rgbMem[0x23a] = wDev + wDrive;
        rgbMem[0x23b] = wCom;
        rgbMem[0x23c] = bAux1;
        rgbMem[0x23d] = bAux2;
#endif

        if ((wDrive < 0) || (wDrive >= MAX_DRIVES))
            goto lCable;

        pdrive = &rgDrives[wDrive];

        md = pdrive->mode;

        if (md == MD_OFF)           /* drive is off */
            {
            wRetStat = 138;
            goto lExit;
            }

        if (md == MD_EXT)
            goto lCable;

        if (md == MD_35)            /* 3.5" 720K disk */
            {
lNAK:
            wRetStat = SIO_NAK;
            goto lExit;
            }

        fDD = (md==MD_QD) || (md==MD_DD) || (md==MD_DD_OLD_ATR1) || (md==MD_DD_OLD_ATR2) || (md==MD_HD) || (md==MD_RD);

        if (pdrive->h == -1)
            goto lNAK;

        /* the disk # is valid, the sector # is valid, # bytes is valid */

        switch(wCom)
        {
        default:
         /*   printf("SIO command %c\n", wCom); */
            wRetStat = SIO_NAK;
            break;

        /* format enhanced density, we don't support that */
        case '"':
            if (md != MD_ED)
                wRetStat = SIO_NAK;
            break;

        case '!':
            if (pdrive->fWP)       /* is drive write-protected? */
                {
                wRetStat = SIO_DEVDONE;
                break;
                }

            /* "format" disk, just zero all sectors */

            memset(rgb,0,sizeof(rgb));

            if ((md == MD_SD) || (md == MD_ED))
                lcbSector = 128L;
            else
                lcbSector = 256L;

            _lseek(pdrive->h,(ULONG)pdrive->ofs,SEEK_SET);

            for (i = 0; i < pdrive->wSectorMac; i++)
                {
                if (_write(pdrive->h,(void *)&rgb,(WORD)lcbSector) < (WORD)lcbSector)
                    {
                    wRetStat = SIO_DEVDONE;
                    break;
                    }
                }
            break;

        /* status request */
        case 'S':
          /*  printf("SIO command 'S'\n"); */

            /* b7 = enhanced   b5 = DD/SD  b4 = motor on   b3 = write prot */
            BOOL fMotorOn = (wLastSectorFrame && wFrame - wLastSectorFrame < 180);  // ~3 seconds until the motor spins down
            cpuPokeB (candy, wBuff++, ((md == MD_ED) ? 128 : 0) + (fDD ? 32 : 0) + (fMotorOn ? 16 : 0) + (pdrive->fWP ? 8 : 0));

            cpuPokeB (candy, wBuff++, 0xFF);         /* controller */
            cpuPokeB (candy, wBuff++, 0xE0);         /* format timeout */
            cpuPokeB (candy, wBuff, 0x00);           /* unused */
            break;

        /* get configuration */
        case 'N':
          /*  printf("SIO command 'N'\n"); */
            if (md == MD_HD)
                {
                pdrive->wSectorMac--;
                cpuPokeB (candy, wBuff++, 1);   /* tracks */
                cpuPokeB (candy, wBuff++, 0);    /* ?? */
                cpuPokeB (candy, wBuff++, pdrive->wSectorMac & 255);   /* ?? */
                cpuPokeB (candy, wBuff++, pdrive->wSectorMac >> 8);    /* sectors/track */
                cpuPokeB (candy, wBuff++, 0x00);         /* ?? */
                pdrive->wSectorMac++;
                }
            else
                {
                cpuPokeB (candy, wBuff++, 0x28);         /* tracks */
                cpuPokeB (candy, wBuff++, 0x02);         /* ?? */
                cpuPokeB (candy, wBuff++, 0x00);         /* ?? */
                cpuPokeB (candy, wBuff++, (md == MD_ED) ? 0x1A : 0x12); /* secs/track */
                cpuPokeB (candy, wBuff++, 0x00);         /* ?? */
                }

            if (fDD)
                {
                cpuPokeB (candy, wBuff++, 0x04);     /* density: 4 = dbl  0 = sng */
                cpuPokeB (candy, wBuff++, 0x01);     /* bytes/sector hi */
                cpuPokeB (candy, wBuff++, 0x00);     /* bytes/sector lo */
                }
            else
                {
                cpuPokeB (candy, wBuff++, 0x00);     /* density: 4 = dbl  0 = sng */
                cpuPokeB (candy, wBuff++, 0x00);     /* bytes/sector hi */
                cpuPokeB (candy, wBuff++, 0x80);     /* bytes/sector lo */
                }
            cpuPokeB (candy, wBuff++, 0xFF);         /* unused */
            cpuPokeB (candy, wBuff++, 0xFF);         /* unused */
            cpuPokeB (candy, wBuff++, 0xFF);         /* unused */
            cpuPokeB (candy, wBuff, 0xFF);           /* unused */
            break;

        /* set configuration - we don't support it */
        case 'O':
          /*  printf("SIO command 'O'\n"); */
            wRetStat = SIO_OK;
            break;
            goto lNAK;

        case 'P':
        case 'W':
        case 'R':
            {
            int cbSIO2PCFudge = pdrive->ofs;

            if (wSector < 1)            /* invalid sector # */
                goto lNAK;

            if ((wSector < 4) || (md == MD_FILE) || (md == MD_FILEBIN) || (md == MD_FILEBAS) || (md == MD_SD) || (md == MD_ED))
                {
                if (wBytes != 128)
                    goto lNAK;
                }
            else if (md == MD_HD)
                wBytes = 256;
            else if (wBytes != 256)
                goto lNAK;

            if (wSector > pdrive->wSectorMac)   /* invalid sector # */
                goto lNAK;

            if ((md == MD_FILE) || (md == MD_FILEBIN) || (md == MD_FILEBAS) || (md == MD_SD) || (md == MD_ED))
                lcbSector = 128L;
            else if ((wSector < 4) && pdrive->ofs)  // SIO2PC disk image
            {
                lcbSector = 128L;
                // the data is in the first half of a 256 byte sector
                if (md == MD_DD_OLD_ATR1)
                    lcbSector = 256;
            }
            else if (pdrive->ofs)
                {
                lcbSector = 256L;
                if (pdrive->cb == 184720)    // !!! 400 byte header instead of 16, what is this?
                    cbSIO2PCFudge += 384;

                // the first 3 sectors were compacted
                else if (md == MD_DD_OLD_ATR2)
                {
                    wSector -= 1;
                    cbSIO2PCFudge -= 128;
                }
            }
            else
                lcbSector = 256L;

            // !!! Arena is mean and uses a custom SEROUT needed interrupt with SIOV to change the sector # to load
            // after SIOV is called from a certain point on
            if (wLastSIOSector == 9 && wSector == 9 && rgbMem[0xbb8a] == 0x3c)
            {
                wSector = 10;           // start the hack
                wLastSIOSector = -1;
            }
            else if (wLastSIOSector == -1)
            {
                if (wSector > 9)
                    wSector++;
                else
                    wLastSIOSector = 0; // stop the hack
            }

            _lseek(pdrive->h,(ULONG)((wSector-1) * lcbSector) + cbSIO2PCFudge,SEEK_SET);

            if ((wCom == 'R'))    // wStat is only checked for cassette I/O not disk I/O, breaks apps // && (wStat == 0x40))
            {
#if 0
                printf("Read: sector = %d  wBuff = $%4x  wBytes = %d  lcbSector = %ld  md = %d\n",
                    wSector, wBuff, wBytes, lcbSector, md);
#endif
                // now that SIO is not instantaneous, mute the audio so it doesn't hold a high pitch (MULE).
                // Since I/O makes sounds, I presume it's OK to alter the sounds on the user like this
                //AUDF1 = AUDF2 = AUDF3 = AUDF4 = 0;    // doesn't work by itself!
                AUDC1 = AUDC2 = AUDC3 = AUDC4 = 0;

                // POKEY Mag 95_May needs to not read the sector if the direction isn't read, or it will trash page 3.
                if ((wStat & 0xc0) != 0x40)
                {
                    wStat = SIO_OK; // Altirra returns NO ERROR, though, so that's probably proper
                    break;
                }

                // create a virtual whole disk image out of a single ATARI file
                if ((md == MD_FILE) || (md == MD_FILEBIN) || (md == MD_FILEBAS))
                {
                    memset(rgb, 0, sizeof(rgb));
                    _fmemset(&rgbMem[wBuff], 0, 128);

                    if (wSector == 360)
                    {
                        // FAT sector

                        // subtract size of our file to get free space
                        BYTE bl = (BYTE)((pdrive->cb + 124L) / 125L);
                        BYTE bh = (BYTE)((pdrive->cb + 124L) / 32000L);
                        WORD wUsed = (bh << 8) | bl;
                        WORD wUnused = (WORD)(707 - wUsed);

                        rgbMem[wBuff + 0] = 0x02;       // VTOC
                        rgbMem[wBuff + 1] = 0xC3;
                        rgbMem[wBuff + 2] = 0x02;       // # of sectors on disk
                        rgbMem[wBuff + 3] = wUnused & 0xff;
                        rgbMem[wBuff + 4] = wUnused >> 8;     // # of free sectors

                        _fmemset(&rgbMem[wBuff + 10], 0xff, 90);   // 1 = unused sector (90 * 8 = 720)
                        rgbMem[wBuff + 10] = (BYTE)~0x70;   // mark first 3 sectors used
                        rgbMem[wBuff + 10 + 45] = 0;        // mark VTOC etc used
                        rgbMem[wBuff + 10 + 46] = 0x7f;        // mark VTOC etc used

                        // mark all the right sectors as used
                        for (int ss = 0; ss < wUsed; ss++)
                        {
                            int sec = ss + 4;   // file starts at sector 4
                            if (sec >= 360)     // file skips sectors 360-368
                                sec += 9;
                            if (sec > 719)
                                sec = 719;      // max. possible DOS sector

                            int s1 = sec & 7;
                            int s2 = sec >> 3;
                            
                            rgbMem[wBuff + 10 + s2] &= ~(0x80 >> s1);   // mark sector used (0)
                        }
                    }
                    else if (wSector == 361)
                    {
                        // directory sector
                        rgbMem[wBuff + 0] = 0x42;
                        rgbMem[wBuff + 1] = (BYTE)((pdrive->cb + 124L) / 125L);    // length
                        rgbMem[wBuff + 2] = (BYTE)((pdrive->cb + 124L) / 32000L);
                        rgbMem[wBuff + 3] = 4;
                        rgbMem[wBuff + 4] = 0x00;
                        memcpy(&rgbMem[wBuff + 5], (BYTE *)pdrive->name, 11);
                    }
                    else if (wSector >= 4)
                    {
                        // data sector

                        if (wSector > 368)   // skip FAT/directory sectors
                            wSector -= 9;

                        if (wSector >= 4)
                        {
                            _lseek(pdrive->h, (ULONG)((wSector - 4) * 125L), SEEK_SET);

                            if ((rgbMem[wBuff + 127] = (BYTE)_read(pdrive->h, &rgbMem[wBuff], 125)) < 125)
                            {
                                rgbMem[wBuff + 125] = 0x00;     // FILE # 0 in bits 7-2, high bits of sector # in bits 1-0
                                rgbMem[wBuff + 126] = 0x00;     // low bits of next sector #
                            }
                            else
                            {
                                wSector++;

                                if (wSector >= 360)
                                    wSector += 9;

                                // !!! DOS only allows 2 high bits for sector number, and large XEX files fill >1024 sectors
                                // Squeeze the full sector # in there anyway, and our loader will handle it but it won't be
                                // a completely valid DOS image, but who cares
                                rgbMem[wBuff + 125] = (BYTE)(wSector >> 8);    // FILE # 0 in bits 7-2, hi 2 bits of sector # in bits 1-0
                                rgbMem[wBuff + 126] = (BYTE)wSector;
                            }
                        }
                    }
                    
                    // sector 1-3, provide boot disk data for binary and BASIC files to be auto-loaded
                    else
                    {
                        if (md == MD_FILEBIN)
                        {
                            if (wSector == 1)
                            {
                                // which of the 2 load routines do we want for this binary?
                                if (fAltBinLoader)
                                    memcpy(&rgbMem[wBuff], BinA1, 128);
                                else
                                    memcpy(&rgbMem[wBuff], Bin1, 128);
                            }
                            else if (wSector == 2)
                            {
                                if (fAltBinLoader)
                                    memcpy(&rgbMem[wBuff], BinA2, 128);
                                else
                                    memcpy(&rgbMem[wBuff], Bin2, 128);
                            }
                            else
                            {
                                if (fAltBinLoader)
                                    memcpy(&rgbMem[wBuff], BinA3, 128);
                                else
                                    memcpy(&rgbMem[wBuff], Bin3, 128);
                            }
                        }
                        else if (md == MD_FILEBAS)
                        {
                            if (wSector == 1)
                                memcpy(&rgbMem[wBuff], Bas1, 128);
                            else if (wSector == 2)
                                memcpy(&rgbMem[wBuff], Bas2, 128);
                            else
                                memcpy(&rgbMem[wBuff], Bas3, 128);
                        }
                    }
                }

                // read from a PC disk file
                else
                {
                    // don't read directly into memory, it might be partially in himem which should be ignored,
                    // so use PokeBAtari (SAG mag #114)
                    BYTE c128[256]; // largest sector size
                    int wBX = _read(pdrive->h, c128, wBytes);
                    for (int iw = 0; iw < wBX; iw++)
                        PokeBAtari(candy, wBuff + iw, c128[iw]);

                    if (wBX < wBytes)
                        wRetStat = SIO_DEVDONE;

                    else
                    {
                        // now pretend some jiffies elapsed for apps that time disk sector reads
                        // (Repton is not that fussy, but mag OVERMIND.atr needs same sector then different sector combo
                        // in same track to take < 13 jiffies
                        // !!! Do all this for _FILEBIN and _FILEBAS and for writing too?
                
                        BYTE jif = 4;
                        // reading the same sector as last time takes twice as long
                        if (wSector == wLastSIOSector)
                            jif = 8;
                        BYTE oldjif = rgbMem[20];
                        rgbMem[20] = oldjif + jif;
                        if (oldjif >= (256 - jif))
                        {
                            oldjif = rgbMem[19];
                            rgbMem[19]++;
                            if (oldjif == 255)
                                rgbMem[18]++;
                        }

                        wRetStat = SIO_OK;
                        if (wLastSIOSector >= 0)
                            wLastSIOSector = wSector;
                        wLastSectorFrame = wFrame;  // note what time it is
                    }
                }
            }
            else if ((wCom == 'W') || (wCom == 'P'))
            {
                // !!! Which apps break? Am I sure? Because the disk handler does check for $40 for read
                //if ((wStat & 0xc0) != 0x80)    // only the cassette handler checks this, not disk I/O, this would break apps
                //{
                //    wStat = SIO_OK; // was goto lNAK, was that the problem?
                //    break;
                //}

                if (pdrive->fWP)
                    {
                    wRetStat = SIO_DEVDONE;
                    break;
                    }
                if (_write(pdrive->h, (LPCCH)&rgbMem[wBuff],wBytes) < wBytes)
                    wRetStat = SIO_DEVDONE;
                else
                    wRetStat = SIO_OK;
            }
            break;
            }
        }
    }

    else if (!fXFCable && wDev == 0x40)      /* printer */
    {
        int timeout = 32000;

#ifdef xDEBUG
        printf("printer SIO: wDev = %2x  wCom = %2x  wBytes = %2x\n",
            wDev, wCom, wBytes);
        printf("SIO: wBuff = %4x  bAux1 = %2x  bAux2 = %2x\n",
            wBuff, bAux1, bAux2);
#endif

        switch(wCom)
            {
        /* status request */
        case 'S':
            while (timeout--)
                {
                if (FPrinterReady(0)) // !!! not thread safe, needs VM #
                    {
                    wRetStat = SIO_OK;
                    break;
                    }

                if (timeout == 0)
                    {
                    wRetStat = SIO_TIMEOUT;
                    break;
                    }
                }
            break;

        case 'W':
            /* print line */
            {
            BYTE *pchBuf = &rgbMem[wBuff];
            int  cch = wBytes;
            BYTE ch;

            while ((pchBuf[cch-1] == ' ') && (cch>0))
                cch--;

            while ((cch-- > 0) && (wRetStat == SIO_OK))
                {
                ch = *pchBuf++;
#if 0
                printf("printing: %02X\n", ch);
                fflush(stdout);
#endif

                while (timeout--)
                    {
                    if (FPrinterReady(0))         // !!! not thread safe, needs VM #
                        {
                        if (ch != 155)
                            ByteToPrinter(0, ch); // !!! not thread safe, needs VM #
                        else
                            {
                            ByteToPrinter(0, 13); // !!! not thread safe, needs VM #
                            ByteToPrinter(0, 10); // !!! not thread safe, needs VM #
                            }

                        wRetStat = SIO_OK;
                        break;
                        }

                    if (timeout == 0)
                        {
                        wRetStat = SIO_TIMEOUT;  // else status = timeout error
                        break;
                        }
                    }
                } // while
            }
        } // switch
    }

    else
    {
lCable:
        {
            wRetStat = SIO_TIMEOUT;
            goto lExit;
        }
    }

#ifdef xDEBUG
    printf("wRetStat = %d\n", wRetStat);
#endif

lExit:
    //ODS("SIOV %02x into %04x aux %04x returns %02x\n", wCom, wBuff, wSector, wRetStat);
    cpuPokeB (candy, 0x303,(BYTE)wRetStat);
    regY = (BYTE)wRetStat;
    regP = (regP & ~ZBIT) | ((wRetStat == 0) ? ZBIT : 0);
    regP = (regP & ~NBIT) | ((wRetStat & 0x80) ? NBIT : 0);

    // !!! At least when a sector read is completed, C is set, and some apps looks for this! (Barahir, Mag Overmind.atr)
    if (wRetStat == SIO_OK)
        regP |= CBIT;

    // SIO turns CRITIC on while executing (ship.xex needs to avoid copying the DMACTL shadow and turning off the screen),
    // and then in SIOCheck() we turn critic off again when the SIO delay is done or much of the VBI will never run again,
    // (Shamus loaded from Preppie II disk needs that to turn the screen on again)
    rgbMem[0x42] = 1;
    
    // Cosmic balance hangs without this as the timer 4 interrupt swallows it whole
    PokeBAtari(candy, 0xd20f, 0x13); // SKCTL - SIO also leaves POKEY in async receive mode stopping timers 3 and 4

    // The real SIO also copies the IRQEN shadow to IRQEN, that's not the VBI that does it, so we need to do this
    // or the keyboard IRQ is left turned off and the keyboard stops working (Floyd of the Jungle)
    PokeBAtari(candy, 0xd20e, rgbMem[0x10]);
   
    // SIO also uses TIMER0 and leaves the following values in it, (which 081_A.atr jumps to)
    if (mdXLXE == md800)
    {
        rgbMem[0x226] = 0xec;
        rgbMem[0x227] = 0xeb;
    }
    else
    {
        rgbMem[0x226] = 0x11;
        rgbMem[0x227] = 0xec;
    }

    //regPC = cpuPeekW(candy, regSP + 1) + 1;        // do an RTS
    //regSP = (regSP + 2) & 255 | 256;

    // the stack might wrap!
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    regPC = rgbMem[regSP];
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    regPC |= (WORD)rgbMem[regSP] << 8;
    regPC++;

    //printf("SIO: returning to PC = %04X, SP = %03X, stat = %02X\n", regPC, regSP, regY);

}

#if 0
// !!! What is this?
// XL/XE serial I/O handler
//
void BUS1(void *candy)
{
    WORD wRetStat = 1;
    WORD wStat = 0;

    // !!! statics. CIO won't work!
    static BYTE oldstat = 0;
    static BYTE mdTranslation = NO_TRANSLATION;
    static BYTE fConcurrent = FALSE;

    //    printf("in bus handler, regPC = %04X\n", regPC);

    switch (regPC & 0x00F0)
    {
    default:
        break;

    case 0x00:    // CIO open
        FSetBaudRate(0, 0, 64, 1);
        break;

    case 0x10:  // CIO close
        fConcurrent = FALSE;
        break;

    case 0x20:  // CIO get byte

#if DEBUGCOM
        printf("g%04X", wStat); fflush(stdout);
#endif

        if (0x0100 & wStat)
        {
            wStat = 0;
            CchSerialRead((char *)&wStat, 1);

#if DEBUGCOM
            printf("G%04X", wStat); fflush(stdout);
#endif
            if (wStat & 0xFF00)
                wRetStat = 136;

            regA = (BYTE)wStat;

            if (mdTranslation != NO_TRANSLATION)
            {
                if (regA == 13)
                    regA = 155;
                else
                    regA &= 0x7F;
            }
        }
        else
            wRetStat = 136;
        break;

    case 0x30:  // CIO put byte
        if (mdTranslation != NO_TRANSLATION)
        {
            if (regA == 155)
                regA = 13;
            else
                regA &= 0x7F;
        }

        FWriteSerialPort(regA);
        break;

    case 0x40:  // CIO status
        if (CchSerialPending())
            wStat = 0x4030;
        else
            wStat = 0x4130;

        if (fConcurrent)
        {
            cpuPokeB(candy, 746, 0);   // error bits
            cpuPokeB(candy, 748, 0);   // unknown
            cpuPokeB(candy, 749, 0);   // characters to be sent

                                       // do character pending

#if DEBUGCOM
            printf("s%04X", wStat); fflush(stdout);
#endif

            if (0x0100 & wStat)
                cpuPokeB(candy, 747, 1);
            else
                cpuPokeB(candy, 747, 0);
            //    printf("%d",cpuPeekB(747)); fflush(stdout);
        }
        else
        {
            // block mode status

            cpuPokeB(candy, 746, 0);   // error bits

                                       // BIOS returns CD in bit 7, DSR in bit 5, CTS in bit 4
                                       // Atari needs CD in bit 3, DSR in bit 7, CTS in bit 5

            wStat = ((wStat & 0x80) >> 4) | ((wStat & 0x20) << 2) | ((wStat & 0x10) << 1);

            wStat |= oldstat;
            oldstat = (BYTE)(wStat >> 1) & 0x54;

            cpuPokeB(candy, 747, (BYTE)wStat);   // handshake bits
        }

        break;

    case 0x50:  // CIO special
#if DEBUGCOM
        printf("XIO %d, aux1 = %d, aux2 = %d\n", cpuPeekB(0x22), cpuPeekB(0x2A), cpuPeekB(0x2B));
#endif

        switch (cpuPeekB(candy, 0x22))
        {
        default:
            break;

        case 32:     // flush output buffer
            break;

        case 34:     // handshake
            break;

        case 36:     // Baud, stop bits, ready monitoring
        {
            int baud = 300;

            switch (cpuPeekB(candy, 0x2A) & 15)
            {
            default:
                break;

            case 2:    //  50 baud
            case 4:    //  75 baud
            case 5:    // 110 baud
                baud = 110;
                break;

            case 6:    // 134 baud
            case 7:    // 150 baud
                baud = 150;
                break;

            case 9:    // 600 baud
                baud = 600;
                break;

            case 10:   // 1200 baud
                baud = 1200;
                break;

            case 11:    // 1800 baud
            case 12:    // 2400 baud
                baud = 2400;
                break;

            case 13:    // 4800 baud
                baud = 4800;
                break;

            case 14:    // 9600 baud
            case 15:    // 19200 baud
                baud = 9600;
                break;
            }

            FSetBaudRate(0, 0, 19200 / baud, 1);

#if DEBUGCOM
            printf("setting baud rate to %d\n", baud);
#endif
            }
        break;

        case 38:     // translation and parity

            mdTranslation = cpuPeekB(candy, 0x2A) & 48;
#if DEBUGCOM
            printf("mdTranslation set to %d\n", mdTranslation);
#endif
            break;

        case 40:     // set concurrent mode
            fConcurrent = TRUE;
#if DEBUGCOM
            printf("concurrent mode set\n");
#endif
            break;
            }
        break;

    case 0x60:  // init vector
        if ((wCOM != 0) && (wCOM != 1))
            break;

        // !!! WTF? This breaks Sparta Dos SP32D, because this is the external PIA H/W device bank #
        //cpuPokeB (candy, 583, cpuPeekB(candy, 247) | 1); // tell OS that device 1 is a CIO device

        cpuPokeB(candy, 797, 'R');
        cpuPokeB(candy, 798, 0x8F);
        cpuPokeB(candy, 799, 0xE4);
        break;

    case 0x70:  // SIO vector   !!! What to do?
        break;

    case 0x80:  // interrupt vector
        break;
    }

    cpuPokeB(candy, 0x343, (BYTE)wRetStat);
    regY = (BYTE)wRetStat;
    regP = (regP & ~ZBIT) | ((wRetStat & 0x80) ? 0 : ZBIT);
    regP = (regP & ~NBIT) | ((wRetStat & 0x80) ? NBIT : 0);

    regP |= CBIT; // indicate that command completed successfully

                  //regPC = cpuPeekW(candy, regSP + 1) + 1;        // do an RTS
                  //regSP = (regSP + 2) & 255 | 256;

                  // the stack might wrap!
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    regPC = rgbMem[regSP];
    regSP = 0x100 | ((regSP + 1) & 0xFF);
    regPC |= (WORD)rgbMem[regSP] << 8;
    regPC++;
}
#endif

#endif // XFORMER


#if 0
void InitSIOV(void *candy, int argc, char **argv)
{
    int i, iArgv = 0;

#ifdef HDOS16ORDOS32
    _bios_printer(_PRINTER_INIT, 0, 0);
#endif

    wCOM = -1;

    while ((argv[iArgv][0] == '-') || (argv[iArgv][0] == '/'))
    {
        switch (argv[iArgv][1])
        {
        case 'A':
        case 'a':
            fAutoStart = 1;
            break;

        case 'D':
        case 'd':
            fDebugger = 1;
            break;

        case 'N':
        case 'n':
            ramtop = 0xC000;
            break;

        case '8':
            mdXLXE = md800;
            break;

        case 'S':
        case 's':
#ifndef NDEBUG
            printf("sound activated\n");
#endif
            fSoundOn = TRUE;
#ifndef HWIN32
            InitSoundBlaster();
#endif
            break;

        case 'T':
        case 't':
            bshftByte &= ~wScrlLock;
            break;

        case 'C':
        case 'c':
            wCOM = argv[iArgv][2] - '1';
#ifndef NDEBUG
            printf("setting modem to COM%d:\n", wCOM + 1);
#endif
            break;

#ifndef HWIN32
        case 'X':
        case 'x':
            fXFCable = 1;
            if (argv[iArgv][2] == ':')
            {
                _SIO_Init();
                sscanf(&argv[iArgv][3], "%d", &uBaudClock);
            }
            else
            {
                _SIO_Calibrate();
            }
            if (uBaudClock == 0)
            {
                fXFCable = 0;
            }
            else
            {
            }
            break;
#endif // !HWIN32

        case 'K':
        case 'k':
            if (argv[iArgv][2] == ':')
                ReadCart(candy, &argv[iArgv][3]);
            break;

#ifndef HWIN32
        case 'J':
        case 'j':
#ifndef NDEBUG
            printf("joystick activated\n");
#endif
            MyReadJoy();

            if ((wJoy0X || wJoy1X))
            {
                fJoy = TRUE;

                wJoy0XCal = wJoy0X;
                wJoy0YCal = wJoy0Y;
                wJoy1XCal = wJoy1X;
                wJoy1YCal = wJoy1Y;
            }

#if 0
            for (;;)
            {
                MyReadJoy();
                printf("0X: %04X  0Y: %04X  1X: %04X  1Y: %04X  BUT: %02X\n",
                    wJoy0X, wJoy0Y, wJoy1X, wJoy1Y, bJoyBut);
            }
#endif

            break;
#endif // !HWIN32
        };

        iArgv++;
    }

    for (i = 0; i < MAX_DRIVES; i++)
    {
        if (i < argc)
        {
            AddDrive(candy, i, argv[i + iArgv]);
        }
        else
            rgDrives[i].mode = MD_OFF;
    }
}
#endif
