
/***************************************************************************

    XKEY.C

    - Atari keyboard emulation

    Copyright (C) 1986-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      open source release

***************************************************************************/

#include "atari800.h"

#ifdef XFORMER

// CheckKey() is called during the vertical blank to process the keyboard buffer which is filled
// by KeyAtari() in response to windows messages for keys, mouse, joystick
//
void CheckKey()
{
    BYTE scan = 0;
    BYTE ch = 0;
    WORD shift = 0;
    WORD dshift = 0;

    static WORD oldshift = 0;
    static WORD fKeyPressed = 0;
     
    extern BYTE rgbMapScans[1024];

// printf("in CheckKey\n");

    if (vvmi.keyhead != vvmi.keytail)
        {
        scan = vvmi.rgbKeybuf[vvmi.keytail++];
        ch   = vvmi.rgbKeybuf[vvmi.keytail++];
        vvmi.keytail &= 1023;

		DebugStr("CheckKey: scan = %02X, ch = %02X\n", scan, ch);

        if ((vvmi.keytail & 1) || (vvmi.keyhead & 1))
            vvmi.keytail = vvmi.keyhead = 0;
        }

    if (scan == 255)
        {
        KBCODE = 255;
        fKeyPressed = 0;
        return;
        }

    shift = *pbshift & (wScrlLock | wCapsLock | wCtrl | wAnyShift);
	//fBrakes = (shift & wScrlLock);	// using PGUP since modern keyboards don't have scroll lock anymore
    shift &= ~wScrlLock;

    if (shift & wCapsLock)
        {
        if (!scan && !ch)
            {
            // Caps Lock pressed, no other key pressed,
            // so fake a caps key keystroke

            scan = 0x3A;
            *pbshift &= ~wCapsLock;
            }

        shift &= ~wCapsLock;
        }

    dshift = shift ^ oldshift;
    oldshift = shift;

    if (dshift || scan || ch)
        {
        fKeyPressed = (scan << 8) | ch;

        DebugStr("ch=%02X scan=%02X shift=%02X\n", ch, scan, shift);
        }
    else
        {
#ifndef HWIN32
        if (!fKeyPressed || ((inp(0x60) & 0x80) == 0))
#else
        if (!fKeyPressed)
#endif
            {
#if 0
            DebugStr("no key pressed\n");
#endif
            return;
            }

        SKSTAT |= 0x04;
#ifndef HWIN32
        CONSOL |= 0x7;
#endif

#ifndef HWIN32
        if (!fJoy)
            {
            rPADATA = 255;
            UpdatePorts();
            TRIG0  |= 1;
            }
#endif

        fKeyPressed = 0;
        DebugStr("upstroke!\n");
        return;
        }

    if (shift & 3)
        {
        SKSTAT &= ~0x08;  // shift key pressed
        }
    else
        {
        SKSTAT |= 0x08;      // shift key not pressed
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

	#if 0 // these char's are never put in our buffer
			case 0x49:
				// Page Up (scroll window down) not put in the buffer, this won't execute (GEM special fn key - BRAKES)
			Lpup:
				if (wStartScan > 0)
					wStartScan--;
				ForceRedraw();
				return;

			case 0x51:
				// Page Down (scroll window up) not put in the buffer, this won't execute (GEM special fn key - FIRE)
			Lpdown:
				if (wStartScan < 55)
					wStartScan++;
				ForceRedraw();
				return;
	#endif
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

	#if 0
		case 0x3F:
			// F5 - nothing
			return;

		case 0x58:
			// Shift+F5 - nothing
			return;
	#endif

		case 0x40:
		case 0x59:
			// F6 or Shift+F6 - XL Help key

			KBCODE = 17;
			goto lookit2;

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
			scan -= 0x50;
			break;

		case 0x47: // numeric 7 (Home)
			if (ch == 0)                    // numlock off
				{
				rPADATA &= ~5;
				UpdatePorts();
				return;
				}
			else
				scan = 0x67;
			break;

		case 0x48: // numeric 8
			if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
				{
				rPADATA &= ~1;
				UpdatePorts();
				return;
				}
			else
				scan = 0x68;
			break;

		case 0x49: // numeric 9
			if (ch == 0)                    // numlock off
				{
				rPADATA &= ~9;
				UpdatePorts();
				return;
				}
			else
				scan = 0x69;
			break;

		case 0x4B: // numeric 4
			if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
				{
				rPADATA &= ~4;
				UpdatePorts();
				return;
				}
			else
				scan = 0x6A;
			break;

		case 0x4C: // numeric 5
			if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
				{
				rPADATA |= 15;
				UpdatePorts();
				return;
				}
			else
				scan = 0x6B;
			break;

		case 0x4D: // numeric 6
			if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
				{
				rPADATA &= ~8;
				UpdatePorts();
				return;
				}
			else
				scan = 0x6C;
			break;

		case 0x4F: // numeric 1
			if (ch == 0)                    // numlock off
				{
				rPADATA &= ~6;
				UpdatePorts();
				return;
				}
			else
				scan = 0x6D;
			break;

		case 0x50: // numeric 2
			if ((ch == 0) || (ch == 0xE0))  // numlock off or cursor key
				{
				rPADATA &= ~2;
				UpdatePorts();
				return;
				}
			else
				scan = 0x6E;
			break;

		case 0x51: // numeric 3
			if (ch == 0)                    // numlock off
				{
				rPADATA &= ~10;
				UpdatePorts();
				return;
				}
			else
				scan = 0x6F;
			break;

		case 0x52: // numeric 0
			if (ch == 0)                    // numlock off
				{
//				TRIG0 &= ~1;                // JOY 0 fire button down
				return;
				}

			scan = 0x70;
			break;

		case 0x53: // numeric .
			scan = 0x71;
			break;
    }

lookitup:
    KBCODE = rgbMapScans[scan*4 + (shift>>1) | (shift&1)];

    if (KBCODE == 255)
    {
        return;
    }

lookit2:
    SKSTAT &= ~0x04;

    if (IRQEN & 0x40)
        IRQST &= ~0x40;

    DebugStr("KBCODE = %02X %d\n", KBCODE, KBCODE);
}


#ifdef HWIN32

// Helper function for KeyAtari() to process a Windows keystroke. The parameter lParam is broken up as follows:
//
// bits 0..15  - repeat count
// bits 16..23 - scan code
// bit  24       - extended key flag
// bit  29     - context code
// bit  30     - previous key state. 0 = up, 1 = down
// bit  31     - transition state. 0 = down, 1 = up
//

BOOL FKeyMsg800(HWND hwnd, UINT message, DWORD uParam, DWORD lParam)
{
    int ch, scan;
    MSG msg;
    BOOL fDown =  (lParam & 0x80000000) == 0;

    static BOOL wShiftChanged;

    scan = (lParam >> 16) & 0xFF;
    ch = uParam;

#if 0
    // eat repeating keystrokes
    if ((lParam & 0xC0000000) == 0x40000000)
        return TRUE;
#endif

    DebugStr("First KeyMsg800: keydown:%d  l:%08X  scan:%04X, ch:%02X\n", fDown, lParam, scan, ch);
    printf("First KeyMsg800: keydown:%d  l:%08X  scan:%04X, ch:%02X\n", fDown, lParam, scan, ch);

    ch = 0;

    *pbshift ^= wShiftChanged;
    wShiftChanged=0;

    switch ((lParam >> 16) & 0x1FF)
        {
    default:

        // FOREIGN KEYBOARD SUPPORT!
        // Remap ASCII characters to U.S. scan codes

        if (PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR+1, PM_REMOVE))
            {
            if ((msg.lParam & 0x80000000) == 0)
                {
                // key being pressed down

                DebugStr("2nd   KeyMsg800: keydown:%d  l:%08X  scan:%04X, ch:%02X\n", fDown, lParam, scan, msg.wParam);

                wShiftChanged = (*pbshift & wAnyShift);

                *pbshift &= ~wCtrl; // on German keyboards, right Alt is same as Ctrl+Alt

                switch (msg.wParam)
                    {
                case '=':  scan = 0x0D; *pbshift &= ~wAnyShift; break;
                case '+':  scan = 0x0D; *pbshift |=  wAnyShift; break;
                case '-':  scan = 0x0C; *pbshift &= ~wAnyShift; break;
                case '_':  scan = 0x0C; *pbshift |=  wAnyShift; break;
                case '[':  scan = 0x1A; *pbshift &= ~wAnyShift; break;
                case ']':  scan = 0x1B; *pbshift &= ~wAnyShift; break;
                case ';':  scan = 0x27; *pbshift &= ~wAnyShift; break;
                case ':':  scan = 0x27; *pbshift |=  wAnyShift; break;
                case '\'': scan = 0x28; *pbshift &= ~wAnyShift; break;
                case '"':  scan = 0x28; *pbshift |=  wAnyShift; break;
                case ',':  scan = 0x33; *pbshift &= ~wAnyShift; break;
                case '<':  scan = 0x33; *pbshift |=  wAnyShift; break;
                case '.':  scan = 0x34; *pbshift &= ~wAnyShift; break;
                case '>':  scan = 0x34; *pbshift |=  wAnyShift; break;
                case '/':  scan = 0x35; *pbshift &= ~wAnyShift; break;
                case '?':  scan = 0x35; *pbshift |=  wAnyShift; break;
                case '\\': scan = 0x2B; *pbshift &= ~wAnyShift; break;
                case '|':  scan = 0x2B; *pbshift |=  wAnyShift; break;
                case '!':  scan = 0x02; *pbshift |=  wAnyShift; break;
                case '@':  scan = 0x03; *pbshift |=  wAnyShift; break;
                case '#':  scan = 0x04; *pbshift |=  wAnyShift; break;
                case '$':  scan = 0x05; *pbshift |=  wAnyShift; break;
                case '%':  scan = 0x06; *pbshift |=  wAnyShift; break;
                case '^':  scan = 0x07; *pbshift |=  wAnyShift; break;
                case '&':  scan = 0x08; *pbshift |=  wAnyShift; break;
                case '*':  scan = 0x09; *pbshift |=  wAnyShift; break;
                case '(':  scan = 0x0A; *pbshift |=  wAnyShift; break;
                case ')':  scan = 0x0B; *pbshift |=  wAnyShift; break;
                case '1':  scan = 0x02; *pbshift &= ~wAnyShift; break;
                case '2':  scan = 0x03; *pbshift &= ~wAnyShift; break;
                case '3':  scan = 0x04; *pbshift &= ~wAnyShift; break;
                case '4':  scan = 0x05; *pbshift &= ~wAnyShift; break;
                case '5':  scan = 0x06; *pbshift &= ~wAnyShift; break;
                case '6':  scan = 0x07; *pbshift &= ~wAnyShift; break;
                case '7':  scan = 0x08; *pbshift &= ~wAnyShift; break;
                case '8':  scan = 0x09; *pbshift &= ~wAnyShift; break;
                case '9':  scan = 0x0A; *pbshift &= ~wAnyShift; break;
                case '0':  scan = 0x0B; *pbshift &= ~wAnyShift; break;

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

                wShiftChanged ^= (*pbshift & wAnyShift);
                }
            }
        break;

    case 0x11C:
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
        if (*pbshift & wCtrl)
            scan += 0x50;   // cursor keys
        else 
		{
            // joystick emulation

            static BYTE mpJoyBit[9] = { 1, 1, 1, 4, 4, 8, 8, 8, 2 };

            if (fDown)
                rPADATA &= ~mpJoyBit[scan - 0x48];
            else
                rPADATA |= mpJoyBit[scan - 0x48];
            UpdatePorts();
            return TRUE;
        }
        break;

    case 0x147:
        ch = 0xE0;      // Home
        break;

    case 0x14F:
        ch = 0xE0;      // End
        break;

    case 0x149:         // Page Up
						// toggle higher clock speed

        if (fDown)
        {
#if 0
            clockMult++;

            if (clockMult > 64)
                clockMult = 1;

            printf("clock multiplier = %u\n", clockMult);
#endif
			SendMessage(hwnd, WM_COMMAND, IDM_TURBO, 0); // toggle real time or fast as possible
		}
		return TRUE;

#if 0
    case 0x151:         // Page Dn
        if (fDown)
            TRIG0 &= ~1;                // JOY 0 fire button down
        else
            TRIG0 |= 1;                 // JOY 0 fire button up
        return TRUE;
#endif

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

	case 0x3e:	// F4
		if (*pbshift & wAlt)
		{
			PostMessage(vi.hWnd, WM_CLOSE, 0, 0);	// be polite and close
		}
		return TRUE;

    case 0x41: // F7 - START key
        if (fDown)
            CONSOL &= ~0x1;
        else
            CONSOL |= 0x01;
        return TRUE;

    case 0x42: // F8 - SELECT key
        if (fDown)
            CONSOL &= ~0x2;
        else
            CONSOL |= 0x02;
        return TRUE;

    case 0x43: // F9 - OPTION key
        if (fDown)
            CONSOL &= ~0x4;
        else
            CONSOL |= 0x04;
        return TRUE;

#if 0 // handled by GEM now
	case 0x44: // F10

		// shift F10 - shortcut to toggle ATARI BASIC and reboot
		if (fDown && (*pbshift & wAnyShift))
		{
			if (ramtop == 0xC000)
				ramtop = 0xA000;
			else
				ramtop = 0xC000;
			FColdbootVM(v.iVM);
		}
		return;
#endif

#if 0
	case 0x58:
		// ctrl F12 - next cartridge
		if (fDown && (*pbshift & wCtrl))
		{
			NextCart();
			FColdbootVM();
		}
		return;
#endif

    case 0x2A: // left shift
    case 0x36: // right shift
        if (fDown)
            *pbshift |= wAnyShift;
        else
            *pbshift &= ~wAnyShift;
        break;

	case 0x1D: // left control
    case 0x11D: // right control
		if (fDown)
			*pbshift |= wCtrl;
		else
			*pbshift &= ~wCtrl;
		break;

	// MAME uses Left control for joystick fire, but that interferes with the arrow keys so we can't easily use that
	case 0x38: // left alt
	case 0x138: // right alt
		if (fDown)
		{
			*pbshift |= wAlt;
			TRIG0 &= ~1;
		}
		else
		{
			*pbshift &= ~wAlt;
			TRIG0 |= 1;
		}
		break;
	
	case 0x46: // Scrl Lock
        if (fDown)
            *pbshift ^= wScrlLock;
        else
            DisplayStatus();
        break;

    case 0x3A: // Caps Lock
        if (fDown)
            *pbshift |= wCapsLock;
        else
            *pbshift &= ~wCapsLock;
        break;
        }

    AddToPacket(fDown ? scan : 0);
    AddToPacket(fDown ? ch   : 0);
    
    return TRUE;
}

// Process Thread messages for keys and joystick, and Windows messages for keybaord, mouse, and joystick
//
BOOL __cdecl KeyAtari(HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam)
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
        return !FKeyMsg800(hWnd, message, uParam, lParam);
        break;

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

//!!! WTF?
void ControlKeyUp8()
{
    *pbshift &= ~wCtrl;
}

#endif // HWIN32

#endif // XFORMER

