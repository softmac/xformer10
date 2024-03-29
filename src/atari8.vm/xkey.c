
/***************************************************************************

    XKEY.C

    - Atari keyboard emulation

    Copyright (C) 1986-2021 by Darek Mihocka and Danny Miller. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    This file is part of the Xformer project and subject to the MIT license terms
    in the LICENSE file found in the top-level directory of this distribution.
    No part of Xformer, including this file, may be copied, modified, propagated,
    or distributed except according to the terms contained in the LICENSE file.

    11/30/2008  darekm      open source release

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

// CheckKey() is called after the vertical blank (calling before the VBI breaks unpausing MULE) to process the keyboard buffer
// which is filled by KeyAtari() in response to windows messages for keys, mouse, joystick
//
// fDoShift means look at the shift key, otherwise use the value in myShift (for text pasting)
//
void CheckKey(void *candy, BOOL fDoShift, WORD myShift)
{
    BYTE scan = 0;
    BYTE ch = 0;
    WORD shift = 0;
    //BOOL fForceCtrl = FALSE;

    extern BYTE rgbMapScans[1024];

    //printf("in CheckKey\n");

    // a key has gone up or down
    if (pvmin->keyhead != pvmin->keytail)
    {
        scan = pvmin->rgbKeybuf[pvmin->keytail++];
        ch = pvmin->rgbKeybuf[pvmin->keytail++];
        pvmin->keytail &= 1023;

        if ((pvmin->keytail & 1) || (pvmin->keyhead & 1))
            pvmin->keytail = pvmin->keyhead = 0;

        // We get repeating scan/ch codes while the key is held down, and sometimes multiple ups. Ignore those.

        if ((scan || ch) && (wLastScanCh == ((scan << 8) | ch)))
            return;

        wLastScanCh = ((scan << 8) | ch);

        fKeyPressed = (scan || ch);
    }

    // nothing has happened
    else
        return;

#if 0 // never happens, and shouldn't ever set kbcode to 255 anyway
    if (scan == 255)
    {
        KBCODE = 255;
        fKeyPressed = 0;
        return;
    }
#endif

    if (fDoShift)
        shift = bshftByte & (wScrlLock | wCapsLock | wCtrl | wAnyShift);
    else
        shift = myShift;
    
    shift &= ~wScrlLock;

    //ODS("CheckKey: scan = %02X, ch = %02X, shift = %02x\n", scan, ch, shift);

    if (shift & 3)
    {
        //ODS("SHIFT DOWN\n");
        SKSTAT &= ~0x08;  // shift key pressed  // !!! not in sync with bit 2, but that seems to be OK
    }

    else
    {
        //ODS("SHIFT UP\n");
        SKSTAT |= 0x08;      // shift key not pressed
    }

    if (shift & wCapsLock)
    {
        if (!scan && !ch)
            {
            // Caps Lock pressed, no other key pressed,
            // so fake a caps key keystroke

            scan = 0x3A;
            bshftByte &= ~wCapsLock;
            }

        shift &= ~wCapsLock;
    }

    if (!(scan || ch))
    {
        SKSTAT |= 0x04; // notify that the key is up
        IRQST |= 0x40;  // no longer assert the key down interrupt
        IRQST |= 0x80;  // !!! supposed to stop asserting it if it's not handled right away, not on key up
                        // but it has to remain asserted for the duration of the interrupt handler

        //ODS("Upstroke @ %03x:%02x\n", wScan, wCycle);
        return;
    }

    shift &= 7;    // mask out Alt bit to not mess up table look up

    if (ch == 0xE0)
    {
        switch (scan)
        {
            case 0x47:
                // Home (not on numeric keypad) - Clear
                goto lookitup;

            case 0x4F:
                // End (not on numeric keypad) - break key
                //ODS("BREAK KEY\n");
                if (IRQEN & 0x80)
                    IRQST &= ~0x80;
                return;

            case 0x52:
                // Ins (not on numeric keypad) - Insert a character (Ctrl Ins)

                if (shift & 3)
                    goto lookitup;

                KBCODE = 0xB7;
                goto lookit2;

            case 0x53:
                // Del (not on numeric keypad) - Delete a character (Ctrl Del)

                if (shift & 3)
                    goto lookitup;

                KBCODE = 0xB4;
                goto lookit2;
        }
    }

    else if (scan == 0xE0)
    {
        switch (ch)
            {
        case 0x0D:
            // Numeric Enter
            scan = 0x72;
            goto lookitup;

        case 0x2F:
            // Numeric /
            scan = 0x65;
            goto lookitup;
            }
    }

    else if ((ch >= 0x2E) && (ch <= 0x39))
    {
        // Numeric keypad with Num Lock on returns same
        // scan codes as without Num Lock, but Ascii code
        // is returned instead of 0. So ignore the Ascii
        // code and go switch on scan code.

        // goto Lbigswitch;
    }
    else if ((ch == 0x2A) && (scan == 0x37))
    {
        // Numeric *

        scan = 0x66;
        goto lookitup;
    }
    else if (scan == 0x56)
    {
        // remap the '\' key on U.K. keyboards
        scan = 0x60;
        goto lookitup;
    }

    switch (scan)
    {
        default:
            // function keys with Ctrl or Alt pressed
            if ((scan >= 0x54) && (scan <= 0x5D))
                scan -= 0x19;
            else if ((scan >= 0x5E) && (scan <= 0x67))
                scan -= 0x23;
            break;

        case 0x77: // Control + Home
            scan = 0x47;
            break;

        case 0x75: // Control + End
            scan = 0x4F;
            break;

        case 0x8D: // Control + Up
            scan = 0x48;
            break;

        case 0x73: // Control + Left
            scan = 0x4B;
            break;

        case 0x74: // Control + Right
            scan = 0x4D;
            break;

        case 0x91: // Control + Down
            scan = 0x50;
            break;

        case 0x92: // Insert
        case 0x93: // Delete
            // Insert/Delete with Contrl pressed
            // need to be changed to their un-Controled scan codes
            scan -= 0x40;
            break;

        case 0x97: // Home
        case 0x98: // Up arrow
        case 0x9B: // Left arrow
        case 0x9D: // Right arrow
        case 0x9F: // End
        case 0xA0: // Down arrow
        case 0xA2: // Insert
        case 0xA3: // Delete
            // cursor keys, Home, etc with Alt pressed
            // need to be changed to their un-Alted scan codes
            //fForceCtrl = TRUE;    // it's annoying to need to press CTRL to get these, but games break using cursors as joystick otherwise
            scan -= 0x50;
            break;

        case 0x47: // numeric 7 (Home)
            if (ch == 0)                    // numlock off
                {
                rPADATA &= ~5;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x67;
            break;

        case 0x48: // numeric 8 (if numeric keypad present, those cursor keys also serve as joystick)
            if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
                {
                rPADATA &= ~1;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x68;
            break;

        case 0x49: // numeric 9
            if (ch == 0)                    // numlock off
                {
                rPADATA &= ~9;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x69;
            break;

        case 0x4B: // numeric 4
            if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
                {
                rPADATA &= ~4;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x6A;
            break;

        case 0x4C: // numeric 5
            if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
                {
                rPADATA |= 15;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x6B;
            break;

        case 0x4D: // numeric 6
            if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
                {
                rPADATA &= ~8;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x6C;
            break;

        case 0x4F: // numeric 1
            if (ch == 0)                    // numlock off
                {
                rPADATA &= ~6;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x6D;
            break;

        case 0x50: // numeric 2
            if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
                {
                rPADATA &= ~2;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x6E;
            break;

        case 0x51: // numeric 3
            if (ch == 0)                    // numlock off
                {
                rPADATA &= ~10;
                UpdatePorts(candy);
                return;
                }
            else
                scan = 0x6F;
            break;

        case 0x52: // numeric 0
            if (ch == 0)                    // numlock off
                {
                TRIG0 &= ~1;                // JOY 0 fire button down
                return;
                }

            scan = 0x70;
            break;

        case 0x53: // numeric .
            scan = 0x71;
            break;
    }

lookitup:

    //if (fForceCtrl) sh = shift | 4;
    
    BYTE kb = rgbMapScans[(scan*4 + (shift >> 1)) | (shift & 1)];    // bit 1 for CTRL, bit 0 for any SHIFT
    
    // if only shift or control is pressed, but no real key, do not report it
    if (kb == 0xff)
        return;

    KBCODE = kb;
    
lookit2:
    
    //ODS("Downstroke %02x @ %03x:%02x\n", KBCODE, wScan, wCycle);
    
    if (IRQEN & 0x40)
        IRQST &= ~0x40;

    // this must be in sync with the moment the IRQ actually fires (Eryus), but if it won't fire, we need to do it now
    // !!! unlikely but possible timing issue
    if ((regP & IBIT) || !(IRQEN & 0x40))
        SKSTAT &= ~0x04;
}


// Helper function for KeyAtari() to process a Windows keystroke. The parameter lParam is broken up as follows:
//
// bits 0..15  - repeat count
// bits 16..23 - scan code
// bit  24       - extended key flag
// bit  29     - context code
// bit  30     - previous key state. 0 = up, 1 = down
// bit  31     - transition state. 0 = down, 1 = up
//
// Decide whether or not to pass the key to ATARI
// RETURN TRUE to hide the key from Windows, FALSE to let Windows see it

BOOL FKeyMsg800(void *candy, HWND hwnd, UINT message, WPARAM uParam, LPARAM lParam)
{
    //ODS("(KM %04x %04x) ", uParam, lParam);
    message;

    int ch, scan;
    MSG msg;
    BOOL fDown =  (lParam & 0x80000000) == 0;

    scan = (lParam >> 16) & 0xFF;
    ch = uParam & 0xFF;

    // the VK codes for volume and brightness, etc. happen to have the same scan codes as ordinary letters
    // and the ATARI will type them, so return early
    if ((ch >= 0xa4 && ch <= 0xb7) || ch == 0xff)    // ff is both brightness keys, or possibly other future expansion keys too?
        return FALSE;

#if 0
    // eat repeating keystrokes
    if ((lParam & 0xC0000000) == 0x40000000)
        return TRUE;
#endif

    //printf("First KeyMsg800: keydown:%d  l:%08X  scan:%04X, ch:%02X\n", fDown, lParam, scan, ch);

    ch = 0;

    bshftByte ^= wShiftChanged;
    wShiftChanged=0;

    switch ((lParam >> 16) & 0x1FF)
        {
    default:

        // FOREIGN KEYBOARD SUPPORT!
        // Remap ASCII characters to U.S. scan codes

        if (PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR + 1, PM_REMOVE))
        {
            if ((msg.lParam & 0x80000000) == 0)
            {
                // key being pressed down

                //DebugStr("2nd   KeyMsg800: keydown:%d  l:%08X  scan:%04X, ch:%02X\n", fDown, lParam, scan, msg.wParam);

                wShiftChanged = (bshftByte & wAnyShift);

                // this would prevent graphics characters from being typed
                //bshftByte &= ~wCtrl; // on German keyboards, right Alt is same as Ctrl+Alt

                switch (msg.wParam)
                {
                case '=':  scan = 0x0D; bshftByte &= ~wAnyShift; break;
                case '+':  scan = 0x0D; bshftByte |= wAnyShift; break;
                case '-':  scan = 0x0C; bshftByte &= ~wAnyShift; break;
                case '_':  scan = 0x0C; bshftByte |= wAnyShift; break;
                case '[':  scan = 0x1A; bshftByte &= ~wAnyShift; break;
                case ']':  scan = 0x1B; bshftByte &= ~wAnyShift; break;
                case ';':  scan = 0x27; bshftByte &= ~wAnyShift; break;
                case ':':  scan = 0x27; bshftByte |= wAnyShift; break;
                case '\'': scan = 0x28; bshftByte &= ~wAnyShift; break;
                case '"':  scan = 0x28; bshftByte |= wAnyShift; break;
                case ',':  scan = 0x33; bshftByte &= ~wAnyShift; break;
                case '<':  scan = 0x33; bshftByte |= wAnyShift; break;
                case '.':  scan = 0x34; bshftByte &= ~wAnyShift; break;
                case '>':  scan = 0x34; bshftByte |= wAnyShift; break;
                case '/':  scan = 0x35; bshftByte &= ~wAnyShift; break;
                case '?':  scan = 0x35; bshftByte |= wAnyShift; break;
                case '\\': scan = 0x2B; bshftByte &= ~wAnyShift; break;
                case '|':  scan = 0x2B; bshftByte |= wAnyShift; break;
                case '!':  scan = 0x02; bshftByte |= wAnyShift; break;
                case '@':  scan = 0x03; bshftByte |= wAnyShift; break;
                case '#':  scan = 0x04; bshftByte |= wAnyShift; break;
                case '$':  scan = 0x05; bshftByte |= wAnyShift; break;
                case '%':  scan = 0x06; bshftByte |= wAnyShift; break;
                case '^':  scan = 0x07; bshftByte |= wAnyShift; break;
                case '&':  scan = 0x08; bshftByte |= wAnyShift; break;
                case '*':  scan = 0x09; bshftByte |= wAnyShift; break;
                case '(':  scan = 0x0A; bshftByte |= wAnyShift; break;
                case ')':  scan = 0x0B; bshftByte |= wAnyShift; break;
                case '1':  scan = 0x02; bshftByte &= ~wAnyShift; break;
                case '2':  scan = 0x03; bshftByte &= ~wAnyShift; break;
                case '3':  scan = 0x04; bshftByte &= ~wAnyShift; break;
                case '4':  scan = 0x05; bshftByte &= ~wAnyShift; break;
                case '5':  scan = 0x06; bshftByte &= ~wAnyShift; break;
                case '6':  scan = 0x07; bshftByte &= ~wAnyShift; break;
                case '7':  scan = 0x08; bshftByte &= ~wAnyShift; break;
                case '8':  scan = 0x09; bshftByte &= ~wAnyShift; break;
                case '9':  scan = 0x0A; bshftByte &= ~wAnyShift; break;
                case '0':  scan = 0x0B; bshftByte &= ~wAnyShift; break;

                    // remap letters without affecting shift state

                case 'A': case 'a': scan = 0x1E; break;
                case 'B': case 'b': scan = 0x30; break;
                case 'C': case 'c': scan = 0x2E; break;
                case 'D': case 'd': scan = 0x20; break;
                case 'E': case 'e': scan = 0x12; break;
                case 'F': case 'f': scan = 0x21; break;
                case 'G': case 'g': scan = 0x22; break;
                case 'H': case 'h': scan = 0x23; break;
                case 'I': case 'i': scan = 0x17; break;
                case 'J': case 'j': scan = 0x24; break;
                case 'K': case 'k': scan = 0x25; break;
                case 'L': case 'l': scan = 0x26; break;
                case 'M': case 'm': scan = 0x32; break;
                case 'N': case 'n': scan = 0x31; break;
                case 'O': case 'o': scan = 0x18; break;
                case 'P': case 'p': scan = 0x19; break;
                case 'Q': case 'q': scan = 0x10; break;
                case 'R': case 'r': scan = 0x13; break;
                case 'S': case 's': scan = 0x1F; break;
                case 'T': case 't': scan = 0x14; break;
                case 'U': case 'u': scan = 0x16; break;
                case 'V': case 'v': scan = 0x2F; break;
                case 'W': case 'w': scan = 0x11; break;
                case 'X': case 'x': scan = 0x2D; break;
                case 'Y': case 'y': scan = 0x15; break;
                case 'Z': case 'z': scan = 0x2C; break;
                }

                wShiftChanged ^= (bshftByte & wAnyShift);
            }
        }

        // don't pass ALT-ed keys to ATARI, but pass them to Windows for menuing
        if (bshftByte & wAlt)
            return FALSE;

        break;

    case 0x1c:    // normal ENTER
    case 0x11C: // keypad ENTER?

        if (bshftByte & wAlt)
            return FALSE;    // exit early to avoid passing the FULLSCREEN Alt-ENTER key to the ATARI

        scan = 0x72;    // Enter -> Enter
        break;

    case 0x152:
        ch = 0xE0;      // Insert
        break;

    case 0x153:
        ch = 0xE0;      // Delete
        break;

    case 0x148:
    case 0x14B:
    case 0x14D:
    case 0x150:
        {
        // plus joystick emulation

        static BYTE mpJoyBit[9] = { 1, 1, 1, 4, 4, 8, 8, 8, 2 };

        if (fDown)
            rPADATA &= ~mpJoyBit[scan - 0x48];
        else
            rPADATA |= mpJoyBit[scan - 0x48];

        UpdatePorts(candy);

        // Unfortunately, we can't have the arrow keys work without CTRL being pressed, as some games (Bruce Lee, Archon, etc.)
        // pause the game on any keystroke! Playing without a joystick and using the arrow keys would
        // continuously pause/unpause the game. Let's NOT pass
        // the arrow keys to Atari if RIGHT control is pressed. That will always work for joystick fire without
        // interfering (although you have to use the left control for the cursor keys)
        if (bshftByte & wCtrl && !(bshftByte & wRCtrl))
        {
            scan += 0x50;
            break;    // pass to ATARI
        }

        return FALSE;    // exit early so the ATARI doesn't see keystrokes when cursor used as joystick.
                        // some games pause or react badly.
        }
    case 0x147:
        ch = 0xE0;      // Home
        break;

    case 0x14F:
        ch = 0xE0;      // End
        break;

    case 0x149:         // Page Up - Time Travel

        if (fDown)
        {
#if 0
            // toggle higher clock speed

            clockMult++;

            if (clockMult > 64)
                clockMult = 1;

            printf("clock multiplier = %u\n", clockMult);
            //SendMessage(hwnd, WM_COMMAND, IDM_TURBO, 0); // toggle real time or fast as possible
#endif
            TimeTravel(candy);
        }
        return FALSE;    // don't let ATARI see this

    case 0x151:         // Page Dn - toggle using a fixed point in time or saving periodically
        if (fDown)
            TimeTravelFixPoint(candy);
        return FALSE;   // don't let ATARI see this either

    case 0x85:
        scan = 0x60;    // U.K. backslash
        break;

    case 0x47: // numeric 7
        scan = 0x67;
        break;

    case 0x48: // numeric 8
        scan = 0x68;
        break;

    case 0x49: // numeric 9
        scan = 0x69;
        break;

    case 0x4B: // numeric 4
        scan = 0x6A;
        break;

    case 0x4C: // numeric 5
        scan = 0x6B;
        break;

    case 0x4D: // numeric 6
        scan = 0x6C;
        break;

    case 0x4F: // numeric 1
        scan = 0x6D;
        break;

    case 0x50: // numeric 2
        scan = 0x6E;
        break;

    case 0x51: // numeric 3
        scan = 0x6F;
        break;

    case 0x52: // numeric 0
        scan = 0x70;
        break;

    case 0x53: // numeric .
        scan = 0x71;
        break;

    case 0x37: // numeric *
        ch = 0x2A;
        break;

        // some function key handling is done in the .rc file. The rest is done here.
        // Only fn keys that make keystrokes that need to be processed in the 800's keyboard buffer are passed to CheckKey

    case 0x3e:    // F4
        if (bshftByte & wAlt)
        {
            PostMessage(vi.hWnd, WM_CLOSE, 0, 0);    // be polite and close
        }
        return TRUE; // don't let ATARI nor Windows see special function key presses (Windows might react)

    case 0x40: // F6 - HELP key for XL/XE
        if (fDown && mdXLXE != md800)
        {
            if (bshftByte & wAnyShift)    // !!! HELP + CTRL + SHIFT supposedly doesn't do anything, I register it as SHIFT-HELP
                rgbMem[0x02dc] = 81;
            else if (bshftByte & wCtrl)
                rgbMem[0x02dc] = 145;
            else
                rgbMem[0x02dc] = 17;
        }
        else if (mdXLXE != md800)
            rgbMem[0x02dc] = 0;

        return TRUE; // dont' let ATARI see special function key presses

    case 0x41: // F7 - START key
        if (fDown)
            CONSOL &= ~0x1;
        else
            CONSOL |= 0x01;
        return TRUE; // don't let ATARI see special function key presses

    case 0x42: // F8 - SELECT key
        if (fDown)
            CONSOL &= ~0x2;
        else
            CONSOL |= 0x02;
        return TRUE; // don't let ATARI see special function key presses

    case 0x43: // F9 - OPTION key
        if (fDown)
            CONSOL &= ~0x4;
        else
            CONSOL |= 0x04;
        return TRUE; // don't let ATARI see special function key presses

#if 0
    // not implemented, and anyway, a VM thread can't call ColdStart!
    case 0x58:
        // ctrl F12 - next cartridge
        if (fDown && (bshftByte & wCtrl))
        {
            NextCart();
            ColdStart();
        }
        return;
#endif

    case 0x2A: // left shift
    case 0x36: // right shift
        if (fDown)
        {
            bshftByte |= wAnyShift;
        }
        else
        {
            bshftByte &= ~wAnyShift;
        }break;

    // MAME uses Left control for joystick fire, but that generates keystrokes with the arrows and some games (archon) pause.
    // We can't use ALT because that works the menus, we can't use shift as that activates sticky keys.
    // If your game pauses, use the RIGHT control key, we won't pass that to ATARI (you won't be able to move the arrows with it)
    // (Altirra makes you toggle a menu item to enable joystick or not, but that's annoying too)
    case 0x1D: // left control
    case 0x11D: // right control
        if (fDown)
        {
            if (((lParam >> 16) & 0x1FF) == 0x11D)
                bshftByte |= wRCtrl;        // it's the RIGHT control
            bshftByte |= wCtrl;
            
            // one thing we can't do well automatically is differentiate between FIRE and Left Control and a keypress.
            // Mag SCAT 078_A Monopoly processes both (Ctrl-U). The L-CTRL specifically is a great fire button, I don't want
            // to disallow it just because of one app.
            if (!v.fDisableLCTRLFire)
                TRIG0 &= ~1;
        }
        else
        {
            if (((lParam >> 16) & 0x1FF) == 0x11D)
                bshftByte &= ~wRCtrl;        // it's the RIGHT control
            bshftByte &= ~wCtrl;
            TRIG0 |= 1;
        }
        break;

    case 0x38: // left alt
    case 0x138: // right alt
        if (fDown)
        {
            bshftByte |= wAlt;
        }
        else
        {
            bshftByte &= ~wAlt;
        }
        return FALSE; // return EARLY (don't let ATARI see it) and FALSE (let windows see it)

    case 0x46: // Scrl Lock
        if (fDown)
            bshftByte ^= wScrlLock;
        //else
        //    DisplayStatus();

        return FALSE; // don't let ATARI see special key presses

    case 0x3A: // Caps Lock
        if (fDown)
            bshftByte |= wCapsLock;
        else
            bshftByte &= ~wCapsLock;
        break;
        }

    AddToPacket(candy, fDown ? scan : 0);
    AddToPacket(candy, fDown ? ch   : 0);

    return FALSE;    // by default, let windows see the key - eg. to operate menus
}

// Process Thread messages for keys and joystick, and Windows messages for keybaord, mouse, and joystick
// Return FALSE if we still want Windows to handle the key, TRUE if we'd like to eat it
//
BOOL __cdecl KeyAtari(void *candy, HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam)
{
    switch (message)
        {
    default:
        //Assert(FALSE);
        break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
#if 0
    case WM_CHAR:
#endif
        return FKeyMsg800(candy, hWnd, message, uParam, lParam);

// we don't use these custom thread messages
#if 0
    case TM_JOY0FIRE:
        if (uParam)
            TRIG0 &= ~1;                // JOY 0 fire button down
        else
            TRIG0 |= 1;                 // JOY 0 fire button up

        return TRUE;

    case TM_JOY0MOVE:
printf("joy0move %d %d\n", uParam, lParam);
        if (uParam)
            rPADATA &= ~lParam;         // JOY 0 direction pushed (arrow down)
        else
            rPADATA |= lParam;          // JOY 0 direction released (arrow up)

        UpdatePorts();
        return TRUE;
#endif

    // mouse moves will send these joystick messages, even though we don't use joystick callbacks
    case MM_JOY1BUTTONDOWN:
    case MM_JOY1BUTTONUP:
        if (uParam & JOY_BUTTON1)
            TRIG0 &= ~1;                // JOY 0 fire button down
        else
            TRIG0 |= 1;                 // JOY 0 fire button up
        return TRUE;

#if 0
    case MM_JOY1MOVE:
    {
        int dir = (LOWORD(lParam) - (vi.rgjc[0].wXmax - vi.rgjc[0].wXmin) / 2);
        dir /= (int)((vi.rgjc[0].wXmax - vi.rgjc[0].wXmin) / wJoySens);
        // printf("X dir = %d\n", dir);

        rPADATA |= 12;                  // assume joystick centered

        if (dir < 0)
            rPADATA &= ~4;              // left
        else if (dir > 0)
            rPADATA &= ~8;              // right

        dir = (HIWORD(lParam) - (vi.rgjc[0].wYmax - vi.rgjc[0].wYmin) / 2);
        dir /= (int)((vi.rgjc[0].wYmax - vi.rgjc[0].wYmin) / wJoySens);
        // printf("Y dir = %d\n", dir);

        rPADATA |= 3;                   // assume joystick centered

        if (dir < 0)
            rPADATA &= ~1;              // up
        else if (dir > 0)
            rPADATA &= ~2;              // down

        UpdatePorts();
        return TRUE;
    }

    case MM_JOY2MOVE:
        {
        int dir = (LOWORD(lParam) - (vi.rgjc[1].wXmax - vi.rgjc[1].wXmin) / 2);
        dir /= (int)((vi.rgjc[1].wXmax - vi.rgjc[1].wXmin) / wJoySens);
// printf("X dir = %d\n", dir);

        rPBDATA |= 12;                  // assume joystick centered

        if (dir < 0)
            rPBDATA &= ~4;              // left
        else if (dir > 0)
            rPBDATA &= ~8;              // right

        dir = (HIWORD(lParam) - (vi.rgjc[1].wYmax - vi.rgjc[1].wYmin) / 2);
        dir /= (int)((vi.rgjc[1].wYmax - vi.rgjc[1].wYmin) / wJoySens);
// printf("Y dir = %d\n", dir);

        rPBDATA |= 3;                   // assume joystick centered

        if (dir < 0)
            rPBDATA &= ~1;              // up
        else if (dir > 0)
            rPBDATA &= ~2;              // down

        UpdatePorts();
        return TRUE;
        }

    case MM_JOY2BUTTONDOWN:
    case MM_JOY2BUTTONUP:
        if (uParam & JOY_BUTTON1)
            TRIG1 &= ~1;                 // JOY 1 fire button down
        else
            TRIG1 |= 1;                 // JOY 1 fire button up
        return TRUE;
#endif

// mouse simulates (poorly) a joystick and just confuses apps because mouse moves to start GEM get passed to the app
// and get stuck not centred.
#if 0
    case WM_MOUSEMOVE:
    {
        int dir = (LOWORD(lParam) - (vsthw.xpix * vi.fXscale) / 2) / (vsthw.xpix / 4);

        rPADATA |= 12;                  // assume joystick centered

        if (dir < 0)
            rPADATA &= ~4;              // left
        else if (dir > 0)
            rPADATA &= ~8;              // right

        dir = (HIWORD(lParam) - (vsthw.ypix * vi.fYscale) / 2) / (vsthw.ypix / 4);

        rPADATA |= 3;                   // assume joystick centered

        if (dir < 0)
            rPADATA &= ~1;              // up
        else if (dir > 0)
            rPADATA &= ~2;              // down

        UpdatePorts();
        return TRUE;
    }

    case WM_LBUTTONDOWN:
        TRIG0 &= ~1;                // JOY 0 fire button down
        return TRUE;

    case WM_LBUTTONUP:
        TRIG0 |= 1;                 // JOY 0 fire button up
        return TRUE;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        // return FMouseMsgST(hWnd, message, uParam, LOWORD(lParam), HIWORD(lParam));
        break;
#endif
        }

    return FALSE;
}

// without this:
// if we closed using ALT-F4 it will have saved the state that ALT is down
// if we cold start, it will remember the time travel anchor point had the control key down
//
void ControlKeyUp8(void *candy)
{
    bshftByte &= ~wCtrl;
    bshftByte &= ~wAlt;
    bshftByte &= ~wAnyShift;
}

#endif // XFORMER

