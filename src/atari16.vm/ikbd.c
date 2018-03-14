
/***************************************************************************

    IKDB.C

    - Atari ST mouse/keyboard routines
    
    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    08/24/2001  darekm      last update

***************************************************************************/

#include "gemtypes.h"

#ifdef ATARIST

#define X_CTR 160
#define Y_CTR 100

enum
    {
    MOUSE_OFF = 0,
    MOUSE_ABS = 1,
    MOUSE_REL = 2,
    MOUSE_KEY = 3,
    };

enum
    {
    JOY_MAN  = 0,           // joystick must be read manually
    JOY_AUTO = 1,           // send joystick packet when joystick moves
    JOY_FIRE = 2,           // continuously returns joystick packets
    JOY_TRIG = 3,           // continuously returns file button vector bits
    };

enum
    {
    IKBDIdle = 0,
    IKBDReset = 0x80,
    IKBDSetMouseAction = 0x07,
    IKBDSetMouseRel,
    IKBDSetMouseAbs,
    IKBDSetMouseKey,
    IKBDSetMouseThresh,
    IKBDSetMouseScale,
    IKBDGetMousePos,
    IKBDSetMousePos,
    IKBDSetOriginBottom,
    IKBDSetOriginTop,
    IKBDResume,
    IKBDDisableMouse,
    IKBDPauseOutput,
    IKBDSetJoystickEvent,
    IKBDSetJoystickInter,
    IKBDGetJoystick,
    IKBDSetJoystickMon,
    IKBDSetFireMon,
    IKBDSetJoystickKey,
    IKBDDisableJoysticks,
    IKBDSetClock,
    IKBDGetClock,
    IKBDSetMemory = 0x20,
    IKBDGetMemory,
    IKBDExecute,
    IKBDInqMouseButAct = 0x87,
    IKBDInqMouseModeRel,
    IKBDInqMouseModeAbs,
    IKBDInqMouseModeKey,
    IKBDInqMouseThresh,
    IKBDInqMouseScale,
    IKBDInqMouseOrigin,
    IKBDInqMouseEnabled,
    IKBDInqJoystickMode,
    IKBDInqJoystickEnabled,
    };


WORD IntToBCD(WORD w)
{
    return ((w/10)<<4) | (w%10);
}


WORD BCDToInt(WORD w)
{
    return ((w>>4)*10) + (w&15);
}


BOOL FValidBCD(WORD w)
{
    if ((w & 0x00F0) > 0x90)
        return FALSE;

    if ((w & 0x000F) > 0x09)
        return FALSE;

    return TRUE;
}


BOOL FResetIKBD()
{
    if (FIsMac(vmCur.bfHW))
        {
        vsthw.mdMouse = MOUSE_REL;
        return TRUE;
        }

    vsthw.mdMouse = MOUSE_OFF;
    vsthw.mdIKBD  = IKBDIdle;
    vsthw.xMouse = vsthw.yMouse = 0;
    vsthw.fLeft = vsthw.fRight = 0;

    return TRUE;
}


unsigned short const mpWinMac[512] =
{
    0xFF, 0x6B, 0x25, 0x27, 0x29, 0x2B, 0x2F, 0x2D,     // 00..07
    0x35, 0x39, 0x33, 0x3B, 0x37, 0x31, 0x67, 0x61,     // 08..0F
    0x19, 0x1B, 0x1D, 0x1F, 0x23, 0x21, 0x41, 0x45,     // 10..17
    0x3F, 0x47, 0x43, 0x3D, 0x49, 0x75, 0x01, 0x03,     // 18..1F

    0x05, 0x07, 0x0B, 0x09, 0x4D, 0x51, 0x4B, 0x53,     // 20..27
    0x4F, 0x65, 0x71, 0x55, 0x0D, 0x0F, 0x11, 0x13,     // 28..2F
    0x17, 0x5B, 0x5D, 0x57, 0x5F, 0x59, 0x71, 0x05,     // 30..37
    0x6F, 0x63, 0x73, 0xF5, 0xF1, 0xC7, 0xED, 0xC1,     // 38..3F

    0xC3, 0xC5, 0xC9, 0xCB, 0xDB, 0x0F, 0xFF, 0x33,     // 40..47
    0x37, 0x39, 0xFF, 0x2D, 0x2F, 0x31, 0x0D, 0x27,     // 48..4F
    0x29, 0x2B, 0x5D, 0x57, 0x5F, 0x59, 0x71, 0xFF,     // 50..57
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // 58..5F
};

// Process a Windows keystroke. The parameter lParam is broken up as follows:
//
// bits 0..15  - repeat count
// bits 16..23 - scan code
// bit  24       - extended key flag
// bit  29     - context code
// bit  30     - previous key state. 0 = up, 1 = down
// bit  31     - transition state. 0 = down, 1 = up
//

#define MacScanADBNot(a,n) (FIsMacADB(vmCur.bfHW) ? (a) : (n))
#define MacScanADBAltNot(a,b,n) \
     (FIsMacADB(vmCur.bfHW) ? (vmCur.fSwapKeys ? (b) : (a)) : (n))

BOOL FKeyMsgST(BOOL fDown, DWORD lParam)
{
    int ch, scan;

    scan = (lParam >> 16) & 0xFF;
    ch &= 0xFF;

#if 0
#if !defined(NDEBUG)
    printf("First KeyMsgST: keydown:%d  l:%08X  scan:%04X\n", fDown, lParam, scan);
#endif
#endif

#if 0
    // eat repeating keystrokes
    if ((lParam & 0xC0000000) == 0x40000000)
        return TRUE;
#endif

    scan = (lParam >> 16) & 0xFF;
    ch &= 0xFF;

    if (FIsMac(vmCur.bfHW))
        {
        switch ((lParam >> 16) & 0x1FF)
            {
        case 0x15B:     // Left Windows key
        case 0x15C:     // Right Windows key
        case 0x15D:     // Menu key
            return FALSE;
            break;

        case 0x11C:
            scan = MacScanADBNot(0x99,0x69);    // Enter
            break;

        case 0x1D:
            scan = MacScanADBAltNot(0x75,0x6D,0x75);    // Left Ctrl
            break;

        case 0x11D:
            scan = MacScanADBAltNot(0x6D,0x6F,0x75);    // Right Ctrl
            break;

        case 0x135:
            scan = 0x97;    // Keypad /
            break;

        case 0x037:
            scan = 0x87;    // Keypad *
            break;

        case 0x04A:
            scan = 0x9D;    // Keypad -
            break;

        case 0x04E:
            scan = 0x8B;    // Keypad +
            break;

        case 0x47: // numeric 7
            scan = 0xB3;
            break;
    
        case 0x48: // numeric 8
            scan = 0xB7;
            break;
    
        case 0x49: // numeric 9
            scan = 0xB9;
            break;
    
        case 0x4B: // numeric 4
            scan = 0xAD;
            break;
    
        case 0x4C: // numeric 5
            scan = 0xAF;
            break;
    
        case 0x4D: // numeric 6
            scan = 0xB1;
            break;
    
        case 0x4F: // numeric 1
            scan = 0xA7;
            break;
    
        case 0x50: // numeric 2
            scan = 0xA9;
            break;
    
        case 0x51: // numeric 3
            scan = 0xAB;
            break;
    
        case 0x52: // numeric 0
            scan = 0xA5;
            break;
    
        case 0x53: // numeric .
            scan = 0x83;
            break;
    
        case 0x38:
            scan = MacScanADBAltNot(0x6F,0x75,0x6F);    // Left Alt
            break;

        case 0x138:
            scan = MacScanADBAltNot(0x6F,0x75,0x6F);    // Right Alt
            break;

        case 0x148:
            scan = MacScanADBNot(0x7D,0x9B);    // up arrow
            break;

        case 0x14B:
            scan = MacScanADBNot(0x77,0x8D);    // left arrow
            break;

        case 0x150:
            scan = MacScanADBNot(0x7B,0x91);    // down arrow
            break;

        case 0x14D:
            scan = MacScanADBNot(0x79,0x85);    // right arrow
            break;

        case 0x149:
            scan = 0xE9;    // Page Up
            break;

        case 0x151:
            scan = 0xF3;    // Page Dn
            break;

        case 0x152:
            scan = 0xE5;    // Insert -> Help
            break;

        case 0x147:
            scan = 0xE7;    // Home
            break;

        case 0x153:
            scan = 0xEB;    // Del
            break;

        case 0x14F:
            scan = 0xEF;    // End
            break;

        case 0x145:
            scan = 0x8F;    // Num Lock
            break;

        case 0x45:
            AddToPacket(0x7F); // Break = NMI
            return TRUE;

        default:
            if (scan >= 0x60)
                scan = 0x7B;
            else
                scan = mpWinMac[scan];
            break;
            }

#if 0
#if !defined(NDEBUG)
        printf("Mac KeyMsgST: keydown:%d  l:%08X  scan:%02X\n", fDown, lParam,
            scan | (fDown ? 0x00 : 0x80));
#endif
#endif

        if (FIsMacADB(vmCur.bfHW))
            AddToPacket((scan >> 1) | (fDown ? 0x00 : 0x80));
        else
            {
            if (scan & 0x80)
                AddToPacket(0x79);
            AddToPacket((scan & 0x7F) | (fDown ? 0x00 : 0x80));
            }
        return TRUE;
        }

    switch ((lParam >> 16) & 0x1FF)
        {
    case 0x11C:
        scan = 0x72;    // Enter -> Enter
        break;

    case 0x149:
        scan = 0x62;    // Page Up -> Help
        break;

    case 0x151:
        scan = 0x61;    // Page Dn -> Undo
        break;

    case 0x85:
        scan = 0x60;    // U.K. backslash
        break;

    case 0x135:         // numeric /
        scan = 0x65;
        break;

    case 0x37: // numeric *
        scan = 0x66;
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

    case 0x57: // F11
    case 0x58: // F12
        return TRUE;
        }

#if 0
#if !defined(NDEBUG)
    printf("KeyMsgST: keydown:%d  l:%08X  scan:%02X\n", fDown, lParam,
        scan | (fDown ? 0x00 : 0x80));
#endif
#endif

    AddToPacket(scan | (fDown ? 0x00 : 0x80));
    
    return TRUE;
}



//
// Process any sort of joystick activity
//

BOOL FJoyMsgST(HWND hWnd, int msg, WPARAM uParam, LPARAM lParam)
{
    int dir;

    switch (msg)
        {
    case MM_JOY1MOVE:
        dir = (LOWORD(lParam) - (vi.rgjc[0].wXmax - vi.rgjc[0].wXmin) / 2);
        dir /= (int)((vi.rgjc[0].wXmax - vi.rgjc[0].wXmin) / wJoySens);
// printf("X dir = %d\n", dir);

        vsthw.wJoy1 |= 12;                  // assume joystick centered

        if (dir < 0)
            vsthw.wJoy1 &= ~4;              // left
        else if (dir > 0)
            vsthw.wJoy1 &= ~8;              // right

        dir = (HIWORD(lParam) - (vi.rgjc[0].wYmax - vi.rgjc[0].wYmin) / 2);
        dir /= (int)((vi.rgjc[0].wYmax - vi.rgjc[0].wYmin) / wJoySens);
// printf("Y dir = %d\n", dir);

        vsthw.wJoy1 |= 3;                   // assume joystick centered

        if (dir < 0)
            vsthw.wJoy1 &= ~1;              // up
        else if (dir > 0)
            vsthw.wJoy1 &= ~2;              // down

        break;

    case MM_JOY1BUTTONDOWN:
    case MM_JOY1BUTTONUP:
        if (uParam & JOY_BUTTON1)
            vsthw.wJoy1 &= ~0x80;                // JOY 0 fire button down
        else
            vsthw.wJoy1 |= 0x80;                 // JOY 0 fire button up
        break;

    case MM_JOY2MOVE:
        dir = (LOWORD(lParam) - (vi.rgjc[1].wXmax - vi.rgjc[1].wXmin) / 2);
        dir /= (int)((vi.rgjc[1].wXmax - vi.rgjc[1].wXmin) / wJoySens);
// printf("X dir = %d\n", dir);

        vsthw.wJoy2 |= 12;                  // assume joystick centered

        if (dir < 0)
            vsthw.wJoy2 &= ~4;              // left
        else if (dir > 0)
            vsthw.wJoy2 &= ~8;              // right

        dir = (HIWORD(lParam) - (vi.rgjc[1].wYmax - vi.rgjc[1].wYmin) / 2);
        dir /= (int)((vi.rgjc[1].wYmax - vi.rgjc[1].wYmin) / wJoySens);
// printf("Y dir = %d\n", dir);

        vsthw.wJoy2 |= 3;                   // assume joystick centered

        if (dir < 0)
            vsthw.wJoy2 &= ~1;              // up
        else if (dir > 0)
            vsthw.wJoy2 &= ~2;              // down

        break;

    case MM_JOY2BUTTONDOWN:
    case MM_JOY2BUTTONUP:
        if (uParam & JOY_BUTTON1)
            vsthw.wJoy2 &= ~1;                 // JOY 1 fire button down
        else
            vsthw.wJoy2 |= 1;                 // JOY 1 fire button up
        break;
        }

    if (vsthw.mdJoys == JOY_AUTO)
        {
        AddToPacket(0xFF);
        AddToPacket(vsthw.wJoy1 ^ 0x8F);
        }
    return TRUE;
}


//
// Process any sort of mouse activity
//

BOOL FMouseMsgST(HWND hWnd, int msg, int flags, int x, int y)
{
    BOOL fLeftDown = FALSE, fRightDown = FALSE;
    static int oldflags = 0;

    if (vsthw.mdMouse == MOUSE_OFF)
        return TRUE;

    if (flags == -1)
        flags = oldflags;

    oldflags = flags;

    switch(msg)
        {
    case WM_MOUSEMOVE:
        fLeftDown = flags & MK_LBUTTON;
        fRightDown = flags & MK_RBUTTON;
        break;

    case WM_LBUTTONDOWN:
        fLeftDown = TRUE;
        fRightDown = flags & MK_RBUTTON;
        break;

    case WM_LBUTTONUP:
        fLeftDown = FALSE;
        fRightDown = flags & MK_RBUTTON;
        break;

    case WM_RBUTTONDOWN:
        fLeftDown = flags & MK_LBUTTON;
        fRightDown = TRUE;
        break;

    case WM_RBUTTONUP:
        fLeftDown = flags & MK_LBUTTON;
        fRightDown = FALSE;
        break;
        }

    if (FIsMacADB(vmCur.bfHW))
        {
        // ADB mouse

        vmachw.ADBMouse.fUp = !fLeftDown;
        AddToPacket(0xFF);
        }
    else
    if (FIsMac(vmCur.bfHW))
        {
        // classic Mac mouse

        if (fLeftDown)
            vmachw.rgvia[0].vBufB &= ~8;
        else
            vmachw.rgvia[0].vBufB |= 8;

//        return TRUE;
        }

    // x and y are already absolute client area co-ordinates
    // but don't send a packet, it will be requested

    vsthw.xMouse = x;
    vsthw.yMouse = y;

    vsthw.fLeft = fLeftDown;
    vsthw.fRight = fRightDown;

    if ((vsthw.mdMouse != MOUSE_ABS) || FIsMac(vmCur.bfHW))
        {
        // relative mode, send relative packet

        int dx, dy;

        dx = x-X_CTR;
        dy = y-Y_CTR;

#if 0
        if (vi.fXzoom)
            dx /= 1;

        if (vi.fYzoom)
            dy /= 1;
#endif

        dx /= 2;
        dy /= 2;

        dx = min(127, max(-127, dx));
        dy = min(127, max(-127, dy));

        if (vi.fVMCapture && (dx || dy))
            {
            // if mouse moved, reposition it to fixed spot

            POINT pt = { X_CTR, Y_CTR };
            ClientToScreen(hWnd, &pt);
            SetCursorPos(pt.x, pt.y);
            }

        if (FIsMac(vmCur.bfHW))
            {
            if (FIsMacADB(vmCur.bfHW))
                {
                if (x > vi.dx)
                    x -= vi.dx;
                else
                    x = 0;

                if (y > vi.dy)
                    y -= vi.dy;
                else
                    y = 0;

                vmachw.ADBMouseX = (x / vi.fXscale);
                vmachw.ADBMouseY = (y / vi.fYscale);

                PokeW(0x828, !vi.fVMCapture ? (y / vi.fYscale) :
                    (PeekW(0x828) + dy));
                PokeB(0x8CE, PeekB(0x8CF));

#if 1
                PokeW(0x82A, !vi.fVMCapture ? (x / vi.fXscale) :
                     (PeekW(0x82A) + dx));
                PokeW(0x82C, PeekW(0x828));
                PokeW(0x82E, PeekW(0x82A));
#endif
                PokeB(0x8CE, PeekB(0x8CF));

                return TRUE;
                }

            if ((dx != 0) && (vmachw.aSCC[SCCA].WR[15] & 8))
                {
                if (x > vi.dx)
                    x -= vi.dx;
                else
                    x = 0;

                // DCD interrupt must be enabled in WR15

#if 0
                // emulate the real Mac hardware

                vmachw.aSCC[SCCB].RR[2] |= 8;
                vmachw.rgvia[0].vBufB &= ~0x10;
                vmachw.rgvia[0].vBufB |= (dx > 0 ? 0x10 : 0);
#else
                // emulate by hacking Mac OS low memory globals

                if (dx < 0)
                    {
                    if (FIsMacADB(vmCur.bfHW))
                        vmachw.ADBMouseX = (x / vi.fXscale);
                    else
                        vmachw.rgvia[0].vBufB &= ~16;
                    }
                else if (dx > 0)
                    {
                    if (FIsMacADB(vmCur.bfHW))
                        vmachw.ADBMouseX = (x / vi.fXscale);
                    else
                        vmachw.rgvia[0].vBufB |= 16;
                    }

                PokeW(0x82A, !vi.fVMCapture ? (x / vi.fXscale) :
                     (PeekW(0x82A) + dx));
                PokeB(0x8CE, PeekB(0x8CF));
#endif
                }

            if ((dy != 0) && (vmachw.aSCC[SCCB].WR[15] & 8))
                {
                if (y > vi.dy)
                    y -= vi.dy;
                else
                    y = 0;

                // DCD interrupt must be enabled in WR15
#if 0
                // emulate the real Mac hardware

                vmachw.aSCC[SCCB].RR[2] |= 8+2;
                vmachw.rgvia[0].vBufB &= ~0x20;
                vmachw.rgvia[0].vBufB |= (dy > 0 ? 0x20 : 0);
#else
                // emulate by hacking Mac OS low memory globals

                if (dy < 0)
                    {
                    if (FIsMacADB(vmCur.bfHW))
                        vmachw.ADBMouseY = (y / vi.fYscale);
                    else
                        vmachw.rgvia[0].vBufB &= ~32;
                    }
                else if (dy > 0)
                    {
                    if (FIsMacADB(vmCur.bfHW))
                        vmachw.ADBMouseY = (y / vi.fYscale);
                    else
                        vmachw.rgvia[0].vBufB |= 32;
                    }

                PokeW(0x828, !vi.fVMCapture ? (y / vi.fYscale) :
                    (PeekW(0x828) + dy));
                PokeB(0x8CE, PeekB(0x8CF));
#endif
                }

            return TRUE;
            }

        if (!vi.fVMCapture && (dx || dy))
            {
            // if mouse moved, reposition it to fixed spot

            POINT pt = { X_CTR, Y_CTR };
            ClientToScreen(hWnd, &pt);
            SetCursorPos(pt.x, pt.y);
            }

        if (!dx && !dy)
            {
            AddToPacket(0xF8 | (fLeftDown ? 2 : 0) | (fRightDown ? 1 : 0));
            AddToPacket(0);
            AddToPacket(0);
            DebugStr("null rel mouse packet: %d, %d\n", dx, dy);
            }
        else
            {        
            AddToPacket(0xF8 | (fLeftDown ? 2 : 0) | (fRightDown ? 1 : 0));
            AddToPacket(dx);
            AddToPacket(dy);
            DebugStr("rel mouse packet: %d, %d\n", dx, dy);
            }
        }

    else if (0)
        {
        // absolute packet

        AddToPacket(0xF7);
        AddToPacket(0);
        AddToPacket(x>>8);
        AddToPacket(x);
        AddToPacket(y>>8);
        AddToPacket(y);
        DebugStr("abs mouse packet: %d, %d\n", x, y);
        }

    return TRUE;
}



//
// Write a byte to the IKBD
//
// Always returns TRUE
//

BOOL FWriteIKBD(int b)
{
    DebugStr("FWriteIKBD: %02X\n", b);

    // if idling, set the new state and set up buffer size
    
    if (vsthw.mdIKBD == IKBDIdle)
        {
        DebugStr("FWriteIKBD: receiving command %02X\n", b);

        vsthw.mdIKBD = b;
        vsthw.cbInIKBD = 0;
        vsthw.cbToIKBD = 0;    // set to 0 so we can check below

        switch(b)
            {
        default:        // invalid command, ignore
            break;

        case IKBDReset:    // reset IKBD
            vsthw.cbToIKBD = 1;
            break;

        case IKBDSetMouseAction:
            vsthw.cbToIKBD = 1;
            break;

        case IKBDSetMouseRel:
            break;

        case IKBDSetMouseAbs:
            vsthw.cbToIKBD = 4;
            break;

        case IKBDSetMouseKey:
            vsthw.cbToIKBD = 2;
            break;

        case IKBDSetMouseThresh:
            vsthw.cbToIKBD = 2;
            break;

        case IKBDSetMouseScale:
            vsthw.cbToIKBD = 2;
            break;

        case IKBDGetMousePos:
            break;

        case IKBDSetMousePos:
            vsthw.cbToIKBD = 5;
            break;

        case IKBDSetOriginBottom:
            break;

        case IKBDSetOriginTop:
            break;

        case IKBDResume:
            break;

        case IKBDDisableMouse:
            // so that benchmark programs have a fresh screen
            ScreenRefresh();
            break;

        case IKBDPauseOutput:
            break;

        case IKBDSetJoystickEvent:
            vsthw.mdJoys = JOY_AUTO;
            break;

        case IKBDSetJoystickInter:
            vsthw.mdJoys = JOY_MAN;
            break;

        case IKBDGetJoystick:
            break;

        case IKBDSetJoystickMon:
            vsthw.cbToIKBD = 1;
            break;

        case IKBDSetFireMon:
            break;

        case IKBDSetJoystickKey:
            vsthw.cbToIKBD = 6;
            break;

        case IKBDDisableJoysticks:
            vsthw.mdJoys = JOY_MAN;
            break;

        case IKBDSetClock:
            vsthw.cbToIKBD = 6;
            break;

        case IKBDGetClock:
            break;

        case IKBDSetMemory:
            vsthw.cbToIKBD = 4;  // NOTE: variable!
            break;

        case IKBDGetMemory:
            vsthw.cbToIKBD = 2;
            break;

        case IKBDExecute:
            vsthw.cbToIKBD = 2;
            break;

        case IKBDInqMouseButAct:
            break;

        case IKBDInqMouseModeAbs:
        case IKBDInqMouseModeRel:
        case IKBDInqMouseModeKey:
            break;

        case IKBDInqMouseThresh:
            break;

        case IKBDInqMouseScale:
            break;

        case IKBDInqMouseOrigin:
            break;

        case IKBDInqMouseEnabled:
            break;

        case IKBDInqJoystickMode:
            break;

        case IKBDInqJoystickEnabled:
            break;
            }

        // If cbToIKBD was set, return and wait for next byte
        
        if (vsthw.cbToIKBD > 0)
            return TRUE;
        }

    // get next byte to put in buffer

    if (vsthw.cbInIKBD < vsthw.cbToIKBD)
        {
        DebugStr("FWriteIKBD: receiving operand %02X\n", b);

        vsthw.rgbIKBD[vsthw.cbInIKBD++] = b;

        if (vsthw.cbInIKBD < vsthw.cbToIKBD)
            return TRUE;
        }

    // buffer has filled up, process command

    DebugStr("FWriteIKBD: processing command %02X\n", vsthw.mdIKBD);

    switch(vsthw.mdIKBD)
        {
    default:
        break;        // ignore command
        
    case IKBDReset:
        if (vsthw.rgbIKBD[0] == 0x01)
            {
            // valid reset command

            FResetIKBD();
            }
        break;

    case IKBDSetMouseRel:
        vsthw.mdMouse = MOUSE_REL;
        break;

    case IKBDSetMouseAbs:
        vsthw.mdMouse = MOUSE_ABS;
        break;

    case IKBDSetMouseKey:
        vsthw.mdMouse = MOUSE_KEY;
        break;

    case IKBDDisableMouse:
        vsthw.mdMouse = MOUSE_OFF;
        break;

    case IKBDSetClock:
        {
        SYSTEMTIME st;

        GetLocalTime(&st);

        // Year 2000 fix! Years > 1999 are encoded as $A0 + year in BCD
        if (vsthw.rgbIKBD[0] >= 0xA0)
            {
            vsthw.rgbIKBD[0] -= 0xA0;
            if (FValidBCD(vsthw.rgbIKBD[0]))
                st.wYear = BCDToInt(vsthw.rgbIKBD[0]) + 2000;
            }
        else if (FValidBCD(vsthw.rgbIKBD[0]))
            st.wYear = BCDToInt(vsthw.rgbIKBD[0]) + 1900;
        if (FValidBCD(vsthw.rgbIKBD[1]))
            st.wMonth = BCDToInt(vsthw.rgbIKBD[1]);
        if (FValidBCD(vsthw.rgbIKBD[2]))
            st.wDay = BCDToInt(vsthw.rgbIKBD[2]);
        if (FValidBCD(vsthw.rgbIKBD[3]))
            st.wHour = BCDToInt(vsthw.rgbIKBD[3]);
        if (FValidBCD(vsthw.rgbIKBD[4]))
            st.wMinute = BCDToInt(vsthw.rgbIKBD[4]);
        if (FValidBCD(vsthw.rgbIKBD[5]))
            st.wSecond = BCDToInt(vsthw.rgbIKBD[5]);

        // To prevent the host time from getting reset back to TOS build time,
        // prevent time from going back before March 3 2008

        if (st.wYear < 2008)
            break;

        if ((st.wYear == 2008) && (st.wMonth < 3))
            break;

        SetLocalTime(&st);
        }
        break;

    case IKBDGetClock:
        {
        SYSTEMTIME st;

        GetLocalTime(&st);

        AddToPacket(0xFC);
        AddToPacket(IntToBCD((WORD)(st.wYear%100 + ((st.wYear >= 2000) ? 100 : 0))));
        AddToPacket(IntToBCD(st.wMonth));
        AddToPacket(IntToBCD(st.wDay));
        AddToPacket(IntToBCD(st.wHour));
        AddToPacket(IntToBCD(st.wMinute));
        AddToPacket(IntToBCD(st.wSecond));
        }
        break;


    case IKBDGetJoystick:
        AddToPacket(0xFD);
        AddToPacket(vsthw.wJoy1 & 0x8F);
        AddToPacket(vsthw.wJoy2 & 0x8F);
        break;
        }

    vsthw.mdIKBD = IKBDIdle;
    return TRUE;
}


//
// Read a byte from the IKBD
//
// High bit of return value is set if error
//

BOOL FReadIKBD()
{
    return 0;
}

//
// Process Windows messages for keybaord, mouse, and joystick

BOOL FWinMsgST(HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam)
{
    switch (message)
        {
    default:
        Assert(FALSE);
        break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        return !FKeyMsgST(TRUE, lParam);
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        return !FKeyMsgST(FALSE, lParam);
        break;

    case MM_JOY1MOVE:
    case MM_JOY1BUTTONDOWN:
    case MM_JOY1BUTTONUP:
    case MM_JOY2MOVE:
    case MM_JOY2BUTTONDOWN:
    case MM_JOY2BUTTONUP:
        return FJoyMsgST(hWnd, message, uParam, lParam);

    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
        return FMouseMsgST(hWnd, message, uParam, LOWORD(lParam), HIWORD(lParam));
        break;
        }

    return FALSE;
}


#endif // ATARIST

