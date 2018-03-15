
/****************************************************************************

    GEMUL8R.C

    - Main window code

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    06/17/2012  darekm      Windows 8 / 64-bit / touch fixes

****************************************************************************/

#define _NO_CRT_STDIO_INLINE

#include "gemtypes.h" // main include file

// instance globals

PROPS v;            // properties currently in use
INST vi;
VMINST vrgvmi[MAXOSUsable];
// __declspec(thread) VMINST vmi;
VMINFO *vpvm;
ICpuExec *vpci;

BOOL fDebug;

// forward references
BOOL FToggleMacAtari(unsigned iVM, BOOL fRefresh);

#include "shellapi.h"

//
// Tray handling
//

#define MYWM_NOTIFYICON        (WM_APP+100)

BOOL TrayMessage(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, PSTR pszTip)
{
    BOOL f = FALSE;
    NOTIFYICONDATA tnd;

    memset(&tnd, 0, sizeof(tnd));

    tnd.cbSize        = sizeof(NOTIFYICONDATA);
    tnd.hWnd        = hDlg;
    tnd.uID            = uID;

    tnd.uFlags        = NIF_MESSAGE;
    tnd.uCallbackMessage    = MYWM_NOTIFYICON;

    tnd.hIcon        = hIcon;

    if (hIcon != NULL)
        tnd.uFlags |= NIF_ICON;

    if (pszTip)
        {
        tnd.uFlags |= NIF_TIP;
        lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
        }
    else
        tnd.szTip[0] = '\0';

// !!! We leave a bunch of icons in the tray
#ifdef WINXP
    f = Shell_NotifyIcon(dwMessage, &tnd);
#endif

    if (hIcon)
        DestroyIcon(hIcon);

    return f;
}


static char const * const rgszMode[] =
    {
#if 1
    " Mono",
    "x4",
    "x8",
    "x16",
    "x32",
    "x64",
    "x128",
    "x256",
    "x64K",
    " True Color",
#else
    " Monochrome",
    " 4 colors",
    " 8 colors",
    " 16 colors",
    " 32 colors",
    " 64 colors",
    " 128 colors",
    " 256 colors",
    " 64K colors",
    " True Color",
#endif
    };

// check the cartridge then the first disk for a filename that can be our instance name
CreateInstanceName(int i, LPSTR lpInstName, int cLen)
{
	LPSTR lp = NULL;
	int z;

	// we have a loaded cartridge, find that name
	if (FIsAtari8bit(v.rgvm[i].bfHW) && v.rgvm[i].rgcart.fCartIn)
	{
		lp = v.rgvm[i].rgcart.szName;
	}

	if (lp == NULL && v.rgvm[i].rgvd[0].sz)
	{
		lp = v.rgvm[i].rgvd[0].sz;
	}

	for (z = strlen(lp) - 1; z >= 0; z--)
	{
		if (lp[z] == '\\') break;
	}

	strncpy(lpInstName, &lp[z+1], cLen);
	lpInstName[cLen - 1] = 0;
}

/****************************************************************************

FUNCTION: DisplayStatus()

PURPOSE: updates window title with current status

COMMENTS:

****************************************************************************/

void DisplayStatus()
{
    char rgch0[256], rgch[256];

    rgch[0] = '\0';

#if PERFCOUNT
    {
    unsigned code_miss_rate = cntCodeMiss / ((cntInstr / 10000) + 1);
    unsigned data_miss_rate = cntDataMiss / ((cntAccess / 10000) + 1);

    if (code_miss_rate > 20000)
        {
        cntInstr = 0;
        cntCodeMiss = 0;
        }

    if (data_miss_rate > 20000)
        {
        cntAccess = 0;
        cntDataMiss = 0;
        }

    sprintf(rgch0, "%s - %s C: %dM %dM (%d.%02d%%) D: %dM %dM (%d.%02d%%)",
        vmCur.szModel, vi.szAppName,
        cntInstr/(1024*1024), cntCodeMiss/(1024*1024),
        code_miss_rate / 100, code_miss_rate % 100,
        cntAccess/(1024*1024), cntDataMiss/(1024*1024),
        data_miss_rate / 100, data_miss_rate % 100,
        0 );

    }
#else
	char pInstname[20];
	CreateInstanceName(v.iVM, pInstname, 20);
    sprintf(rgch0, "%s - %s - %s", vi.szAppName, vmCur.szModel, pInstname);
#endif

    if (vi.fExecuting)
        {
#if 0
        if (FIsMac(vmCur.bfHW))
            {

            sprintf(rgch, "\0%d%s, %dx%d%s\0 %d %d",
            (vi.cbRAM[0] < 4194304) ? vi.cbRAM[0]/1024 : vi.cbRAM[0]/1048576,
            (vi.cbRAM[0] < 4194304) ? "K" : "M",
            vsthw.xpix, vsthw.ypix, (vsthw.planes == 1) ? rgszMode[0] : " Color",
            0, 0);
            }
        else if (FIsAtari8bit(vmCur.bfHW))
            {
            extern WORD fBrakes;

            sprintf(rgch, " (%s", fBrakes ? "1.8 MHz" : "Turbo");
            }
        else if (FIsTutor(vmCur.bfHW))
            {
            sprintf(rgch, "\0 68000");
            }
        else
            {
            sprintf(rgch, "\0TOS %d.%02d, %d%s, %dx%d%s\0 %d %d",
            0, 0, // vi.bTOSmajor, vi.bTOSminor,
            (vi.cbRAM[0] < 1048576) ? vi.cbRAM[0]/1024 : vi.cbRAM[0]/1048576,
            (vi.cbRAM[0] < 1048576) ? "K" : "M",
            vsthw.xpix, vsthw.ypix, rgszMode[vsthw.planes-1],
            vi.fHaveFocus, vi.fGEMMouse);
            }
#endif
        }
    else
        {
        sprintf(rgch, " - Not running");
        }

    strcat(rgch0, rgch);

#ifdef XFORMER
    if (FIsAtari8bit(vmCur.bfHW))
        {
        extern WORD fBrakes;

        sprintf(rgch, " (%s)", fBrakes ? "1.8 MHz" : "Turbo");
        strcat(rgch0, rgch);
        }
#endif

    if (v.fZoomColor)
    {
        strcat(rgch0, " - Stretched");
    }

    SetWindowText(vi.hWnd, rgch0);

    TrayMessage(vi.hWnd, NIM_MODIFY, 0,
            LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), rgch0);
}


int UpdateOverlay()
{
#ifdef LATER
    char rgch[256], rgch2[256];

    sprintf(rgch, "RAM:%d%s Video:%dx%dx%d",
    (vi.cbRAM[0] < 4194304) ? vi.cbRAM[0]/1024 : vi.cbRAM[0]/1048576,
    (vi.cbRAM[0] < 4194304) ? "K" : "M",
    vsthw.xpix, vsthw.ypix, vsthw.planes);

    if (vi.eaROM[0] && vi.fExecuting)
        {
        sprintf(rgch2, " ROM:%08X", PeekL(vi.eaROM[0]));
        strcat(rgch,rgch2);
        }

    TextOut(vi.hdc, 16, vi.dy, rgch, strlen(rgch));
#endif

    return 0;
}

#if defined(NDEBUG) && 0

//
// Our custom printf, calls the Windows runtime instead of CRT
//

int __cdecl printf(const char *format, ...)
{
    va_list arglist;
    char rgch[512];
    int retval, i;
    static HANDLE h = NULL;

    va_start(arglist, format);

    retval = wvsprintf(rgch, format, arglist);

    if (v.fDebugMode)
        {
        OFSTRUCT ofs;

        if (h == NULL)
            h = OpenFile("\\GEMUL8R.LOG", &ofs, OF_CREATE);

        if (1 && retval)
            WriteFile(h, rgch, retval, &i, NULL);
        }

    if (1 && retval && vi.pszOutput)
        strcat(vi.pszOutput, rgch);

    if (1 && retval)
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), rgch, retval, &i, NULL);

    return retval;
}

#endif


//
// Given a high resolution timer frequency in vi.llTicks generate a 4 byte
// array of right shift coefficients to generate a lower frequency count
//

void GenerateCoefficients(fE)
{
    ULONG lFreq = fE ? 783330 / 2 : 1000;
    BYTE *pbSh = fE ? (BYTE *)&vi.rgbShiftE : (BYTE *)&vi.rgbShiftMs;
    int i;

    for (i = 0; i < 6; i++)
        pbSh[i] = 63;

    for (i = 0; i < 6; i++)
        {
        while ((fE ? GetEclock() : Getms()) < lFreq)
            {
            pbSh[i]--;
            }

        pbSh[i]++;
        pbSh[i] += 0;
        }

#if !defined(NDEBUG)
    printf("coefficients for mapping %d Hz to %d Hz are %d %d %d %d %d %d\n",
            vi.rglTicks[0], lFreq, pbSh[0], pbSh[1], pbSh[2],
                                   pbSh[3], pbSh[4], pbSh[5]);
#endif
}


// checks to see if running on a Pentium and sets performance frequency

ULONG CheckClockSpeed()
{
    volatile ULONG l;

    QueryPerformanceFrequency((LARGE_INTEGER *)&vi.llTicks);

    GenerateCoefficients(0);
    GenerateCoefficients(1);

    l = vi.rglTicks[0];

    return l;
}

//
// Updates the current high resolution counter ticks in vi.llTicks
//
// Returns numbers of ticks since last call
//

ULONG QueryTickCtr()
{
    ULONG i;

    i = (ULONG)vi.llTicks;

    QueryPerformanceCounter((LARGE_INTEGER *)&vi.llTicks);

    i = (ULONG)vi.llTicks - i;

#if 0
    printf("high frequency tick counter = %08X%08X\n",
         vi.rglTicks[1], vi.rglTicks[0]);
#endif

    return i;
}

ULONGLONG GetCycles()
{
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	qpc.QuadPart -= vi.qpcCold;
	ULONGLONG a = (qpc.QuadPart * 178979ULL);
	ULONGLONG b = (vi.qpfCold / 10ULL);
	ULONGLONG c = a / b;
	return c;
}

#if 0
ULONGLONG GetJiffies()
{
	ULONGLONG c = GetCycles() / 29833;
	return c;
}
#endif

//
// Return time in millisecond increments.
//
// Uses the high resolution timer in vi.llTicks and shifts it by the
// 6 coefficients in vi.rgbShiftMs to approximate the millisecond count
//

ULONG Getms()
{
    vi.lms = (ULONG)(vi.llTicks >> vi.rgbShiftMs[0]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftMs[1]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftMs[2]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftMs[3]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftMs[4]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftMs[5]);

    return vi.lms;
}

//
// Return time in 783330 Hz increments (the Mac "E clock")
//
// Uses the high resolution timer in vi.llTicks and shifts it by the
// 6 coefficients in vi.rgbShiftE to approximate the E clock
//

ULONG GetEclock()
{
    vi.lEclk = (ULONG)(vi.llTicks >> vi.rgbShiftE[0]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftE[1]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftE[2]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftE[3]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftE[4]) +
             (ULONG)(vi.llTicks >> vi.rgbShiftE[5]);

    return vi.lEclk;
}


/* advance pch to point to non-space */

char *PchSkipSpace(pch)
char *pch;
    {
    char ch;

    while (ch = *pch++)
        {
        if ((ch != ' ') && (ch != 8) && (ch != 9) && (ch != 10) && (ch != 13))
            return pch-1;
        }

    return 0;
    }


char *PchGetWord(pch, pu)
char *pch;
ULONG *pu;
{
    register int x;
    int w=0, digit=0 ;

    if (pch == 0)
        return 0;

    if (!(pch = PchSkipSpace(pch)))
        return 0;

    while (1)
        {
        x = *pch++;

        if ((x >= '0') && (x <= '9'))
            {
            w <<= 4;
            w += x - '0';
            }
        else if ((x >= 'a') && (x <= 'f'))
            {
            w <<= 4;
            w += x - 'a' + 10;
            }
        else if ((x >= 'A') && (x <= 'F'))
            {
            w <<= 4;
            w += x - 'A' + 10;
            }
        else
            break;
        }
    *pu = w;
    return pch;
}


/****************************************************************************

    FUNCTION: ShowGEMMouse / ShowWindowsMouse

    PURPOSE:  Show/hide the Windows mouse and activate the GEM mouse

    COMMENTS:

****************************************************************************/

void SetWindowsMouse(int level)
{
    int newlevel;

    newlevel = ShowCursor(FALSE);

    while (newlevel < level)
        newlevel = ShowCursor(TRUE);

    while (newlevel > level)
        newlevel = ShowCursor(FALSE);

}

void ShowGEMMouse()
{
    if (!vi.fVMCapture)
        {
        ClipCursor(NULL);
        return;
        }

    if (vi.fExecuting && !vi.fGEMMouse && vi.fHaveFocus)
        {
        RECT rect;

        Assert(vi.fHaveFocus);

        ShowCursor(FALSE);
        SetWindowsMouse(-1);
        vi.fGEMMouse = TRUE;

        GetClientRect(vi.hWnd, &rect);
        ClientToScreen(vi.hWnd, (LPPOINT)&rect.left);
        ClientToScreen(vi.hWnd, (LPPOINT)&rect.right);
        ClipCursor(&rect);
        }
}


void ShowWindowsMouse()
{
    if (!vi.fVMCapture)
        {
        ClipCursor(NULL);
        return;
        }

    if (vi.fGEMMouse && vi.fHaveFocus)
        {
        Assert(vi.fHaveFocus);

        ShowCursor(TRUE);
        SetWindowsMouse(0);
        vi.fGEMMouse = FALSE;
        ClipCursor(NULL);
        }
}


#if 0
/*
 *  Clear the System Palette so that we can ensure an identity palette
 *  mapping for fast performance. note only needed for a Win31 app...
 */
void ClearSystemPalette(void)
{
    struct
        {
        WORD Version;
        WORD NumberOfEntries;
        PALETTEENTRY aEntries[256];
        } Palette = {0x0300, 256};

    HPALETTE ScreenPalette;
    HDC ScreenDC;
    int Counter;
    HWND hwnd = GetForegroundWindow();

    for(Counter = 0; Counter < 256; Counter++)
        {
        Palette.aEntries[Counter].peRed = 0;
        Palette.aEntries[Counter].peGreen = 0;
        Palette.aEntries[Counter].peBlue = 0;
        Palette.aEntries[Counter].peFlags = PC_NOCOLLAPSE;
        }


    ScreenDC = GetDC(hwnd);
    ScreenPalette = CreatePalette((LOGPALETTE *)&Palette);

    if (ScreenPalette)
        {
        ScreenPalette = SelectPalette(ScreenDC,ScreenPalette,FALSE);
        RealizePalette(ScreenDC);
        ScreenPalette = SelectPalette(ScreenDC,ScreenPalette,FALSE);
        DeleteObject(ScreenPalette);
        }

    ReleaseDC(hwnd, ScreenDC);
}
#endif

//
// This is a dummy thread that runs at lowest priority.
// Its job is to keep the processor busy so that CheckClockSpeed works
// correctly. On SpeedStep processors the counter stops counting when the
// processor is truly idle!
//

long IdleThread(int l)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    while (!vi.fQuitting && (vi.si.dwNumberOfProcessors == 1))
        {
        Sleep(250);

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        }

    return 0;
}


long ScreenThread(int l)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    CheckClockSpeed();

    for (;;)
        {
        if (vi.fQuitting)
            ExitThread(0);

#if defined(ATARIST) || defined(SOFTMAC)
        if (vi.fExecuting && !FIsAtari8bit(vmCur.bfHW))
            {
            if (vi.fRefreshScreen || vi.fVideoWrite)
                {
                ScreenRefresh();
                }
            }
#endif

        // 30 ms refresh rate = ~33 fps

        Sleep(30);

#if 1
        {
        static int count = 0;

        if ((count++ & 31) == 0)
            DisplayStatus();
        }
#endif
        }

    return 0;
}


void CreateDebuggerWindow()
{
    // check for parent process command prompt

    if (vi.fParentCon)
        return;

    // create a console window for debugging

    AllocConsole();
    SetConsoleTitle("Virtual Machine Debugger");

    printf(""); // flush the log file

//    EnableWindow(vi.hWnd,FALSE);
}

void DestroyDebuggerWindow()
{
    // check for parent process command prompt

    if (vi.fParentCon)
        return;

    if (!vi.fInDebugger && !v.fDebugMode)
        {
        FreeConsole();
        }

    // set focus to vi.hWnd

//  EnableWindow(vi.hWnd,TRUE);
    GetFocus();
}

// Fix the FILE menu list of all the valid VM's
void CreateVMMenu()
{
	int z;
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	char mNew[MAX_PATH + 10];

	int iFound = 0;
	for (z = MAX_VM - 1; z >= 0; z--)
	{
		if (v.rgvm[z].fValidVM)
		{
			// the highest VM becomes IDM_VM1
			if (iFound == 0)
			{
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				CreateInstanceName(z, mNew, MAX_PATH + 10);	// get a name for it
				SetMenuItemInfo(vi.hMenu, IDM_VM1, FALSE, &mii);
				iFound++;
			}
			// lower VMs go before IDM_V1 in the menu
			else
			{
				mii.fMask = MIIM_STRING | MIIM_ID;
				mii.wID = IDM_VM1 - iFound;
				mii.dwTypeData = mNew;
				CreateInstanceName(z, mNew, MAX_PATH + 10);	// get a name for it
				DeleteMenu(vi.hMenu, IDM_VM1 - iFound, 0);	// erase the old one
				InsertMenuItem(vi.hMenu, IDM_VM1 - iFound + 1, 0, &mii); // insert before here
				iFound++;
			}
		}
	}
	DeleteMenu(vi.hMenu, IDM_VM1 - v.cVM, 0);	// in case we're fixing the menus because one was deleted, this will still be hanging around
}


/****************************************************************************

    FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/
int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    RECT rect;
    MSG msg;
    BOOL fProps;

	// create the thread's message queue

    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // Get paths

    GetCurrentDirectory(MAX_PATH, (char *)&vi.szDefaultDir);
    GetWindowsDirectory((char *)&vi.szWindowsDir, MAX_PATH);

    // find out basic things about the processor

    GetSystemInfo(&vi.si);

    // It's rude to store settings globally in the WINDOWS directory
    // since there may be multiple users on the machine (i.e. Windows XP).
    // So we check to see that IE4.71 or higher is running and if so use
    // the per-user application directory

    {
    UINT (__stdcall *pfnSG)();
    HMODULE hSh32;
    HRESULT __stdcall SHGetFolderPath(HWND, int, HANDLE, DWORD, LPTSTR);

#if !defined(NDEBUG)
    printf("Windows path is: '%s'\n", vi.szWindowsDir);
#endif

    if ((hSh32 = LoadLibrary("shell32.dll")) &&
       (pfnSG = (void *)GetProcAddress(hSh32, "SHGetFolderPathA")))
        {
        if (S_OK == (*pfnSG)(NULL, CSIDL_APPDATA, NULL,
                0 /* SHGFP_TYPE_CURRENT */ , (char *)&vi.szWindowsDir))
            {
            strcat(vi.szWindowsDir, "\\Emulators");
            CreateDirectory(vi.szWindowsDir,NULL);

#if !defined(NDEBUG)
            printf("Application data path is: '%s'\n", vi.szWindowsDir);
#endif
            }
        }
    }

    // Now that the app data directory is set up, it is ok to read the INI file

    //ClearSystemPalette();
    InitProperties();	// initial our state and create some default VMs if nothing will be loaded

    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.

    vi.hInst = hInstance; // Store instance handle in our global variable

#ifdef XFORMER
#ifdef ATARIST
    vi.szAppName = "Gemulator";
    vi.szTitle   = "Gemulator";
#else
    vi.szAppName = "Xformer";
    vi.szTitle   = "Xformer";
#endif // ATARIST
#else  // !XFORMER
#ifdef SOFTMAC
#ifdef ATARIST
    vi.szAppName = "Gemulator";
    vi.szTitle   = "Gemulator";
#else
    vi.szAppName = "SoftMac";
    vi.szTitle   = "SoftMac";
#endif // ATARIST
#else
    vi.szAppName = "Gemulator";
    vi.szTitle   = "Gemulator";
#endif // SOFTMAC
#endif // XFORMER

    // Set tick resolution to 1ms

#if !defined(_M_ARM)
    timeBeginPeriod(1);
#endif

    // Get the high resolution time frequency
    // The result will be either around 1.193182 MHz
    // or it will be the Pentium clock speed.

    QueryPerformanceFrequency((LARGE_INTEGER *)&vi.llTicks);

    // Generate the shift co-efficients so that llTicks can be shifted by
    // at most 4 coefficients to produce an approximate count of milliseconds

    GenerateCoefficients(0);

    // Generate the shift co-efficients so that llTicks can be shifted by
    // at most 4 coefficients to produce the 783330 Hz E-clock count

    GenerateCoefficients(1);

    // Initialize shared things

    if (!hPrevInstance)
        {
        WNDCLASS  wc;

        // Fill in window class structure with parameters that describe the
        // main window.

        wc.style     = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;// Class style(s).
        wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
        wc.cbClsExtra    = 0;              // No per-class extra data.
        wc.cbWndExtra    = 0;              // No per-window extra data.
        wc.hInstance     = hInstance;          // Owner of this class
        wc.hIcon     = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_APP)); // Icon name from .RC
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);// Cursor
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);// Default color
        wc.lpszMenuName  = MAKEINTRESOURCE(IDR_GEMMENU);	// menu
        wc.lpszClassName = vi.szAppName;          // Name to register as

        // Register the window class and return success/failure code.
        if (!RegisterClass(&wc))
            return (FALSE);     // Exits if unable to initialize
        }

    // initialize menu and menu accelerators

    vi.hAccelTable = LoadAccelerators (hInstance, MAKEINTRESOURCE(IDR_ACCEL));

    vi.hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_GEMMENU));

    // now get handle to first pop-up
	// vi.hMenu = GetSubMenu(vi.hMenu, 0);

    // Try to make use of the existing command line window if present

    {
    // AttachConsole not supported before Windows XP

    UINT (__stdcall *pfnAC)(DWORD);
    HMODULE h;
    DWORD ret = 0;

    if ((h = LoadLibrary("kernel32.dll")) &&
       (pfnAC = (void *)GetProcAddress(h, "AttachConsole")))
        {
        ret = (*pfnAC)(-1);
        }

    if (0 == ret)
        {
        }
    else
        vi.fParentCon = TRUE;

    if (v.fDebugMode && !vi.fParentCon)
        {
        CreateDebuggerWindow();
        }
    }

    {
    OSVERSIONINFO oi;
    char *pch;

    oi.dwOSVersionInfoSize = sizeof(oi);
    GetVersionEx(&oi);  // REVIEW: requires Win32s 1.2+

    switch(oi.dwPlatformId)
        {
    default:
    case VER_PLATFORM_WIN32_WINDOWS:
        pch = "Windows 95";

        if ((oi.dwBuildNumber & 0xFFFF) == 950)
            v.fNoSCSI = TRUE;   // Windows 95 RTM has boned SCSI support

        break;

    case VER_PLATFORM_WIN32s:
#if 1
        vi.fWin32s = TRUE;
#endif
        pch = "Windows 3.1 with Win32s";
        break;

    case VER_PLATFORM_WIN32_NT:
        vi.fWinNT = TRUE;
        pch = "Windows NT";

#if defined(ATARIST) || defined(SOFTMAC)
        vi.hROMCard = CreateFile(
            "\\\\.\\GemulatorDev",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
#endif // ATARIST
           break;
        };
    }

    // Try to load previously saved properties, need to do this to get saved window
    // position and fNoDirectX
	// also initializes the global persistable PROPS structure to the default set of VMs created in InitProperties (not persisted right now)
	// current VM is set to 0
    fProps = LoadProperties(NULL);

#ifdef XFORMER
	// !!! TODO: use the FstIdentifyFileSystem() function in blockapi.c to identify the disk image format
	// and select an existing VM of the appropriate hardware type or create a new VM of that type
	// For now just support for Atari 8-bit VMs and presume the disk image is an .ATR/XFD image

	if (FIsAtari8bit(vmCur.bfHW) && (lpCmdLine && strlen(lpCmdLine) > 3))
	{
		int len = strlen(lpCmdLine);
		lpCmdLine[len - 1] = 0;	// get rid of the trailing "

		if (lstrcmpi(lpCmdLine + len - 4, "atr\0") == 0 || lstrcmpi(lpCmdLine + len - 4, "xfd") == 0)
		{
			strcpy(vmCur.rgvd[0].sz, lpCmdLine + 1); // replace disk 1 image with the argument (sans ")
			vmCur.rgvd[0].dt = DISK_IMAGE;	// we want to use this disk image in VM 0
		}
		else if (stricmp(lpCmdLine + len - 4, "bin") == 0 || stricmp(lpCmdLine + len - 4, "rom") == 0
						|| stricmp(lpCmdLine + len - 4, "car") == 0)
		{
			strcpy(vmCur.rgcart.szName, lpCmdLine + 1); // replace disk 1 image with the argument (sans ")
			vmCur.rgcart.fCartIn = TRUE;
		}
	}
#endif

    // DirectX can fragment address space, so only preload if user wants to

    if (/* !vi.fWin32s && */ !v.fNoDDraw)
        {
        if (!FInitDirectDraw())
            MessageBox (GetFocus(),    "DirectX failed to initialize. Full screen mode unavailable.", vi.szAppName, MB_OK);
        }

#if defined(ATARIST) || defined(SOFTMAC)
    if (!memInitAddressSpace())
        return FALSE;

    vi.pregs = &vregs;

    // Allocate a large SCSI data buffer and commit a little bit

    vi.pvSCSIData = malloc(32*1024*1024);

    if (NULL != vi.pvSCSIData)
        memset(vi.pvSCSIData, 0, 32*1024*1024);

#endif // ATARIST

    // Create a main window for this application instance.

    if (!v.fFullScreen && (v.rectWinPos.right == 0))
        {
        // no saved window position, use default

        v.rectWinPos.left = min(50, (GetSystemMetrics(SM_CXSCREEN)-640));
        v.rectWinPos.top  = min(50, (GetSystemMetrics(SM_CYSCREEN)-420));

        SetRect(&rect, 0, 0, 640, 480);	// reasonable default size (default to minimum?)
        }
    else
        {
        SetRect(&rect, v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right, v.rectWinPos.bottom);
        }

#if !defined(NDEBUG)
    printf("init: client rect = %d, %d, %d, %d\n", rect.top, rect.left, rect.right, rect.bottom);
#endif

    vi.hWnd = CreateWindowEx(0L,
        vi.szAppName,       // See RegisterClass() call.
        vi.szTitle,         // Text for window title bar.
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
        v.rectWinPos.left,    // X posisition
        v.rectWinPos.top,    // Y position
        rect.right-rect.left, // Window width
        rect.bottom-rect.top, // Window height
        NULL,            // Overlapped windows have no parent.
        NULL,            // Use the window class menu.
        hInstance,       // This instance owns this window.
        NULL             // We don't use any data in our WM_CREATE
    );

    // If window could not be created, return "failure"
    if (!vi.hWnd)
        return (FALSE);

	// Initialize the menus
	CheckMenuItem(vi.hMenu, IDM_FULLSCREEN, v.fFullScreen ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_STRETCH, v.fZoomColor ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_TILE, v.fTiling ? MF_CHECKED : MF_UNCHECKED);
	
	// Initialize the virtual disk menu items
	
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	char mNew[MAX_PATH+10];
	mii.dwTypeData = mNew;

	if (FIsAtari8bit(vmCur.bfHW))	// 8 bit drives are called D1:, D2: etc.
	{
		sprintf(mNew, "&D1: %s ...", vi.pvmCur->rgvd[0].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D1, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D1U, (vi.pvmCur->rgvd[0].sz[0]) ? 0 : MF_GRAYED);
		sprintf(mNew, "&D2: %s ...", vi.pvmCur->rgvd[1].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D2, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D2U, (vi.pvmCur->rgvd[1].sz[0]) ? 0 : MF_GRAYED);
		sprintf(mNew, "&D3: %s ...", vi.pvmCur->rgvd[2].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D3, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D3U, (vi.pvmCur->rgvd[2].sz[0]) ? 0 : MF_GRAYED);
		sprintf(mNew, "&D4: %s ...", vi.pvmCur->rgvd[3].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D4, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D4U, (vi.pvmCur->rgvd[3].sz[0]) ? 0 : MF_GRAYED);

		if (vi.pvmCur->rgcart.fCartIn)
		{
			sprintf(mNew, "&Cartridge %s ...", vi.pvmCur->rgcart.szName);
			SetMenuItemInfo(vi.hMenu, IDM_CART, FALSE, &mii);
		}
		EnableMenuItem(vi.hMenu, IDM_NOCART, (vi.pvmCur->rgcart.fCartIn) ? 0 : MF_GRAYED);
	}
	else // TODO prefix other kinds of virtual drives appropriately
	{
		sprintf(mNew, "%s ...", vi.pvmCur->rgvd[0].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D1, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D1U, (vi.pvmCur->rgvd[0].sz[0]) ? 0 : MF_GRAYED);
		sprintf(mNew, "%s ...", vi.pvmCur->rgvd[1].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D2, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D2U, (vi.pvmCur->rgvd[1].sz[0]) ? 0 : MF_GRAYED);
		sprintf(mNew, "%s ...", vi.pvmCur->rgvd[2].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D3, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D3U, (vi.pvmCur->rgvd[2].sz[0]) ? 0 : MF_GRAYED);
		sprintf(mNew, "%s ...", vi.pvmCur->rgvd[3].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D4, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D4U, (vi.pvmCur->rgvd[3].sz[0]) ? 0 : MF_GRAYED);

		EnableMenuItem(vi.hMenu, IDM_CART, MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_NOCART, MF_GRAYED);
	}

	// grey out some things if there are no VMs at all
	EnableMenuItem(vi.hMenu, IDM_DELVM, (v.cVM) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_DELALL, (v.cVM) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_COLDSTART, (v.cVM) ? 0 : MF_GRAYED);

	// List all the valid VMs under the FILE menu
	CreateVMMenu();

	// Now put all the possible different VM types into the menu

	int zz = 0;
#ifdef XFORMER	// offer the 8 bit choices
	for (; zz < 3; zz++)
	{
		// Atari 800 / XL / XE
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = mNew;
		strcpy(mNew, rgszVM[zz + 1]);
		SetMenuItemInfo(vi.hMenu, IDM_ADDVM1 + zz, FALSE, &mii); // give a proper name to the sample menu item
	}
#endif

	// TODO: now do the non-8 bit choices

	for (; zz < 9; zz++)
	{
		DeleteMenu(vi.hMenu, IDM_ADDVM1 + zz, 0); // we don't need these
	}


    // Make the window visible; update its client area; and return "success"

    ShowWindow(vi.hWnd, nCmdShow); // Show the window
    UpdateWindow(vi.hWnd);     // Sends WM_PAINT message

    // Initialize SCSI

    if (!v.fNoSCSI)
        FInitBlockDevLib();

    // Now go and prompt the user with First Time Setup if necessary

    if (!fProps)
        {
#if defined(ATARIST) || defined(SOFTMAC)
        if (!AutoConfigure(FALSE))
        DialogBox(vi.hInst,       // current instance
            MAKEINTRESOURCE(IDD_FIRSTTIME), // dlg resource to use
            NULL,          // parent handle
            FirstTimeProc);

        // This will prevent the First Time Setup from popping up again
        SaveProperties(NULL);
#endif // ATARIST
	}

	// Initialize the current instance (or first valid one found) by asking the current VM to cold start
	// !!! the first time you switch to a VM is when it cold starts, it's our global non-persistable INSTANCE data
	// that knows if we need a cold start or not (fInited)
    if (!FToggleMacAtari(v.iVM, TRUE) && !FToggleMacAtari((unsigned)(-1), TRUE))
        return -1;

	{
    int l;

    vi.hIdleThread  = CreateThread(NULL, 4096, (void *)IdleThread, 0, 0, &l);
#if defined(ATARIST) || defined(SOFTMAC)
    vi.hVideoThread = NULL; // CreateThread(NULL, 4096, (void *)ScreenThread, 0, 0, &l);
#endif // ATARIST
    }

    // create a frame buffer in shared memory

    vi.hFrameBuf = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, 1280*720 + 16 + 3200, "GemulatorFrame");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
        printf("MAPPED FILE ALREADY EXISTS!\n");

    printf("created file mapping object %08p\n", vi.hFrameBuf);

    vi.pbFrameBuf = (BYTE *)MapViewOfFile(vi.hFrameBuf, FILE_MAP_ALL_ACCESS,
        0, 0, 0);

    printf("created view of file map at %08X\n", vi.pbFrameBuf);

    *(DWORD *)&vi.pbFrameBuf[1280*720] = GetCurrentThreadId();

    printf("wrote thread id = %04X\n", GetCurrentThreadId());

    vi.pbAudioBuf = vi.pbFrameBuf + 1280*720 + 16;

    PostMessage(vi.hWnd, WM_COMMAND, IDM_COLDSTART, 0);

#if 0
    // One time un-Hibernate
    if (v.fSkipStartup & 2)
        {
        v.fSkipStartup &= ~2;
        SaveProperties(NULL);

//        SetWindowText(vi.hWnd, "Loading saved state...");
//        PostMessage(vi.hWnd, WM_COMMAND, IDM_LOADSTATE, 0);
        }
#endif

	// !!!
    {
    FLASHWINFO fi;

    fi.cbSize = sizeof(fi);
    fi.hwnd = vi.hWnd;
    fi.dwFlags = FLASHW_TRAY;
    fi.uCount = 1;
    fi.dwTimeout = 0;

    FlashWindowEx(&fi);

    // Allow the screen thread to complete the timing calibration
	// !!! WTF?
    Sleep(900);

    fi.dwFlags = FLASHW_STOP;
    FlashWindowEx(&fi);
    }

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    for (;;)
        {
        while (PeekMessage(&msg, // message structure
           NULL,   // handle of window receiving the message
           0,      // lowest message to examine
           0,      // highest message to examine
           PM_REMOVE)) // remove the message, as if a GetMessage
            {
            if (msg.message == WM_QUIT)
                goto Lquit;

//            Assert(msg.hwnd == vi.hWnd);

            if (msg.hwnd == NULL)
                {
                UINT message = msg.message;
                WPARAM uParam = msg.wParam;
                LPARAM lParam = msg.lParam;

                switch(msg.message)
                    {

				// we don't seem to send these to ourself anymore
#if 0
				case TM_JOY0FIRE:
                case TM_JOY0MOVE:
                    if (vi.fExecuting)
                        FWinMsgVM(vi.hWnd, message, uParam, lParam);
                    break;

                case TM_KEYDOWN:
                    if (vi.fExecuting)
                        FWinMsgVM(vi.hWnd, WM_KEYDOWN,
                            uParam, (lParam << 16) | 1);
                    break;

                case TM_KEYUP:
                    if (vi.fExecuting)
                        FWinMsgVM(vi.hWnd, WM_KEYUP,
                            uParam, (lParam << 16) | 0xC0000001);
                    break;
#endif
				}

                // don't pass thread message to window handler
                continue;
                }

            if (!TranslateAccelerator (msg.hwnd, vi.hAccelTable, &msg))
                {
                // for Xformer, we need special German keys to translate
                if (!FIsAtari8bit(vmCur.bfHW) && (!vi.fExecuting))
                    TranslateMessage(&msg);// Translates virtual key codes
                DispatchMessage(&msg); // Dispatches message to window
                }
            }

        // Check if VM needs to reset
		// !!! Why wait so long to do it?
        if (vmCur.fColdReset)
            {
            void ColdStart();

            vmCur.fColdReset = FALSE;
            ColdStart();

			if (v.fDebugMode)
                vi.fDebugBreak = TRUE;
            }

        // Break into debugger is asked to, or if in debug mode and VM died

        if (vi.fDebugBreak ||
                 ((vi.fInDebugger || v.fDebugMode) && !vi.fExecuting))
            {
            vi.fDebugBreak = FALSE;

            CreateDebuggerWindow();
            SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);

#if defined(ATARIST) || defined(SOFTMAC)
            if (!FIsAtari8bit(vmCur.bfHW))
                {
                Mon();
                }
            else
#endif // ATARIST
                {
#ifdef XFORMER
                vi.fInDebugger = TRUE;
                vi.fExecuting = FALSE;
                mon();
                vi.fExecuting = TRUE;
#endif
                }
            DestroyDebuggerWindow();
            }

        else if (vi.fExecuting)
            {
            vi.fExecuting = (FExecVM(FALSE, TRUE) == 0);

            if (FIsAtari8bit(vmCur.bfHW))
                {
#if 0
				// if tiling, advance to the next VM, but this is not how to do it
				if (v.fTiling)
                    FToggleMacAtari((unsigned)(-1), TRUE);
#endif
                }
            }

        else
            {
            // idle code

            WaitMessage();
            }
        } // forever

    // Reset tick resolution

#if !defined(_M_ARM)
    timeEndPeriod(1);
#endif

Lquit:
{
 int i;

 for (i = 0; i < MAXOSUsable; i++)
 {
 // printf("vmi %2d: pvBits = %08p\n", vrgvmi[i]);
 }
}

    // clear the share memory thread ID
    *(DWORD *)&vi.pbFrameBuf[1280*720] = 0;

    return (msg.wParam); // Returns the value from PostQuitMessage

    lpCmdLine; // This will prevent 'unused formal parameter' warnings
}


/****************************************************************************

    FUNCTION: SetBitmapColors/FCreateOurPalette

    PURPOSE:  Set the current system palette to Atari colors

    COMMENTS: return TRUE if it seems to have worked

****************************************************************************/

BOOL SetBitmapColors()
{
    int i;

    if (vsthw.fMono)
        {
        if (vi.fInDirectXMode)
            return TRUE;

        if (v.fNoMono)
            {
            vsthw.rgrgb[0].rgbRed =
            vsthw.rgrgb[0].rgbGreen =
            vsthw.rgrgb[0].rgbBlue = 0;
            vsthw.rgrgb[0].rgbReserved = 0;

            for (i = 1; i < 256; i++)
                {
                vsthw.rgrgb[i].rgbRed =
                vsthw.rgrgb[i].rgbGreen =
                vsthw.rgrgb[i].rgbBlue = 0xFF;
                vsthw.rgrgb[i].rgbReserved = 0;
                }
            SetDIBColorTable(vvmi.hdcMem, 0, 256, vsthw.rgrgb);
            }
        else
            SetDIBColorTable(vvmi.hdcMem, 0, 2,   vsthw.rgrgb);

        return TRUE;
        }

    if (FIsAtari8bit(vmCur.bfHW))
        {
        extern BYTE rgbRainbow[];

        if (vmCur.bfMon == monGreyTV) for (i = 0; i < 256; i++)
            {
            vsthw.rgrgb[i].rgbRed =
            vsthw.rgrgb[i].rgbGreen =
            vsthw.rgrgb[i].rgbBlue =
             (rgbRainbow[i*3] + (rgbRainbow[i*3] >> 2)) +
             (rgbRainbow[i*3+1]  + (rgbRainbow[i*3+1] >> 2)) +
             (rgbRainbow[i*3+2]  + (rgbRainbow[i*3+2] >> 2));
            vsthw.rgrgb[i].rgbReserved = 0;
            }
        else for (i = 0; i < 256; i++)
            {
            vsthw.rgrgb[i].rgbRed = (rgbRainbow[i*3] << 2) | (rgbRainbow[i*3] >> 5);
            vsthw.rgrgb[i].rgbGreen = (rgbRainbow[i*3+1] << 2) | (rgbRainbow[i*3+1] >> 5);
            vsthw.rgrgb[i].rgbBlue = (rgbRainbow[i*3+2] << 2) | (rgbRainbow[i*3+2] >> 5);
            vsthw.rgrgb[i].rgbReserved = 0;

#if 0
            //
            // test code to dump the Atari 256-colour palette as YUV values
            //

            {
            int Y, U, V;
            int R = vsthw.rgrgb[i].rgbRed;
            int G = vsthw.rgrgb[i].rgbGreen;
            int B = vsthw.rgrgb[i].rgbBlue;

            // from http://en.wikipedia.org/wiki/YUV/RGB_conversion_formulas

            Y = 66*R + 129*G + 25*B;
            U = 112*B - 74*G - 38*R;
            V = 112*R - 94*G - 18*B;

            Y >>= 8;
            U >>= 8;
            V >>= 8;

            Y += 16;
            U += 128;
            V += 128;

            printf("i = %3d, RGB=%3d,%3d,%3d, YUV = %3d,%3d,%3d\n",
                i, R, G, B, Y, U, V);
            }
#endif
            }

		if (vi.fInDirectXMode)
			;//FChangePaletteEntries(0, 256, vsthw.rgrgb);
        else
            SetDIBColorTable(vvmi.hdcMem, 0, 256, vsthw.rgrgb);
        return TRUE;
        }

    if (FIsMac(vmCur.bfHW) && (vsthw.planes == 8))
        {
		if (vi.fInDirectXMode)
			;// FChangePaletteEntries(0, 256, vsthw.rgrgb);
        else
            SetDIBColorTable(vvmi.hdcMem, 0, 256, vsthw.rgrgb);
        return TRUE;
        }

	if (vi.fInDirectXMode)
		;// FChangePaletteEntries(10, 16, vsthw.rgrgb);
    else
        SetDIBColorTable(vvmi.hdcMem, 10, 16, vsthw.rgrgb);
    return TRUE;
}

#if 0
BOOL FCreateOurPalette()
    {
    int         i;

    static struct {
        WORD palVersion;
        WORD palCount;
        PALETTEENTRY ape[256];
    }    pal;

#if 0
    if (vsthw.fMono)
        return TRUE;
#endif

    if (vi.fInDirectXMode)
        return TRUE;

    if (vi.hpal == NULL)
    {
        pal.palVersion = 0x0300;
        pal.palCount = 256;

        // make a identity 1:1 palette
        for (i=0; i<256; i++)
        {
            pal.ape[i].peRed = (BYTE)i;
            pal.ape[i].peGreen = 0;
            pal.ape[i].peBlue = 0;
            pal.ape[i].peFlags = PC_EXPLICIT;
        }

        vi.hpal = CreatePalette((LPLOGPALETTE)&pal);
    }

    if (FIsAtari8bit(vmCur.bfHW) || (vsthw.planes == 8))
        {
        for (i=0; i<256; i++)
            {
            pal.ape[i].peRed = vsthw.rgrgb[i].rgbRed;
            pal.ape[i].peGreen = vsthw.rgrgb[i].rgbGreen;
            pal.ape[i].peBlue = vsthw.rgrgb[i].rgbBlue;
            pal.ape[i].peFlags = PC_NOCOLLAPSE;
            }
        SetPaletteEntries(vi.hpal, 0, 256, pal.ape);
        }
    else
        {
        for (i=0; i<16; i++)
            {
            pal.ape[i].peRed = vsthw.rgrgb[i].rgbRed;
            pal.ape[i].peGreen = vsthw.rgrgb[i].rgbGreen;
            pal.ape[i].peBlue = vsthw.rgrgb[i].rgbBlue;
            pal.ape[i].peFlags = PC_NOCOLLAPSE;
            }
        SetPaletteEntries(vi.hpal, 10, 16, pal.ape);
        }

    SelectPalette(vi.hdc,vi.hpal,FALSE);
    return RealizePalette(vi.hdc) != 0;
    }
#endif

/****************************************************************************

    FUNCTION: CreateNewBitmap

    PURPOSE:  Create a bitmap of the appropriate size and colors

    COMMENTS: Updates globals v

****************************************************************************/

BOOL CreateNewBitmap()
{
    RECT rect;
    ULONG x;
    ULONG y;
    RECT rectSav = v.rectWinPos;

#if !defined(NDEBUG)
PrintScreenStats(); printf("Entering CreateNewBitmap: new x,y = %4d,%4d, fFull=%d, fZoom=%d\n",
        vsthw.xpix, vsthw.ypix, v.fFullScreen, v.fZoomColor);
#endif

    // Interlock to make sure video thread is not touching memory

    while (0 != InterlockedCompareExchange(&vi.fVideoEnabled, FALSE, TRUE))
        Sleep(10);

    MarkAllPagesClean();

    UninitDrawing(!v.fFullScreen);
    vi.fInDirectXMode = FALSE;

    if (vvmi.hbmOld)
        {
        SelectObject(vvmi.hdcMem, vvmi.hbmOld);
        vvmi.hbmOld = NULL;
        }

    if (vvmi.hbm)
        {
        DeleteObject(vvmi.hbm);
        vvmi.hbm = NULL;
        }

    if (vvmi.hdcMem)
        {
        DeleteDC(vvmi.hdcMem);
        vvmi.hdcMem = NULL;
        }

    // initialize to some initial state, will be set further below

    vi.sx = vsthw.xpix;
    vi.sy = vsthw.ypix;
    vi.dx = 0;
    vi.dy = 0;
    vi.fXscale = 1;
    vi.fYscale = 1;

#ifdef ALLOW_DIRECTX
    if (vi.hWnd && v.fFullScreen && vi.fHaveFocus)
        {
        // Try to switch to DirectX mode

        int sx=640, sy=400, bpp=8;

Ltry16bit:

        if (vsthw.xpix >= 640 && vsthw.ypix >= 400)
            {
            sx = vsthw.xpix;
            sy = vsthw.ypix;
            }

        if (FIsMac(vmCur.bfHW) && vsthw.ypix == 342)
            {
            sx = 512;
            sy = 384;
            }

        if (FIsAtari8bit(vmCur.bfHW))
            {
            if (v.fZoomColor)
                {
                sx = 320;
                sy = 240;
                }
            else
                {
                sx = 512;
                sy = 384;
                }
            }

Ltryagain:

#if !defined(NDEBUG)
        printf("trying sx=%d sy=%d bpp=%d\n", sx, sy, bpp);
#endif

        if ((v.fNoDDraw < 2) && InitDrawing(&sx,&sy,&bpp,vi.hWnd,0))
            {
            char *pb;
            int cb;

            vi.sx = sx;
            vi.sy = sy;

            // printf("succeeded sx=%d sy=%d bpp=%d\n", sx, sy, bpp);

            pb = (bpp == 8) && LockSurface(&cb);

            if (pb)
                {
#if !defined(NDEBUG)
                printf("locked sx=%d sy=%d bpp=%d\n", sx, sy, bpp);
#endif

                UnlockSurface();
                ClearSurface();
                vi.fInDirectXMode = TRUE;
                }
            }
        else
            {
            if ((sx <= 320) && (sy < 240))
                {
                sy = 240;
                goto Ltryagain;
                }

            if ((sx <= 512) && (sy < 384))
                {
                sx = 512;
                sy = 384;
                goto Ltryagain;
                }

            if ((sx <= 640) && (sy < 400))
                {
                sx = 640;
                sy = 400;
                goto Ltryagain;
                }

            if ((sx <= 640) && (sy < 480))
                {
                sx = 640;
                sy = 480;
                goto Ltryagain;
                }

            if ((sx <= 800) && (sy < 512))
                {
                sx = 800;
                sy = 512;
                goto Ltryagain;
                }

            if ((sx <= 800) && (sy < 600))
                {
                sx = 800;
                sy = 600;
                goto Ltryagain;
                }

            if ((sx <= 1024) && (sy < 768))
                {
                sx = 1024;
                sy = 768;
                goto Ltryagain;
                }

            if ((sx <= 1152) && (sy < 864))
                {
                sx = 1152;
                sy = 864;
                goto Ltryagain;
                }

            if ((sx <= 1280) && (sy < 1024))
                {
                sx = 1280;
                sy = 1024;
                goto Ltryagain;
                }

            // careful, this is a special case because X grows, y is fixed

            if ((sx < 1600) && (sy <= 1024))
                {
                sx = 1600;
                sy = 1024;
                goto Ltryagain;
                }

            if ((sx <= 1600) && (sy < 1200))
                {
                sx = 1600;
                sy = 1200;
                goto Ltryagain;
                }

            // if we've exhausted all the modes, maybe the card can't do
            // 256 color mode (3Dlabs cards!), try higher bit depths

            if (bpp < 32)
                {
                sx=640;
                sy=400;
                bpp += 8;
                goto Ltry16bit;
                }

            // v.fFullScreen = FALSE;
            }
        }
#endif

    x = GetSystemMetrics(SM_CXSCREEN);
    y = GetSystemMetrics(SM_CYSCREEN);

#if !defined(NDEBUG)
    PrintScreenStats();

    if (vi.fInDirectXMode)
        printf("CreateNewBitmap after DirectX mode change, x = %d, y = %d\n", x, y);
#endif

    if (!vi.fInDirectXMode)
        {
        // very wide (e.g. 640x200) double the height
        // to maintain a proper aspect ratio

        int scan_double = ((vsthw.xpix) >= (vsthw.ypix * 3)) ? 2 : 1;

#if 0
        if (v.fZoomColor || v.fFullScreen)
        {
        // now keep increasing the zoom until it doesn't fit

        while ((vsthw.xpix * (vi.fXscale + 1) <= x) && (vsthw.ypix * scan_double * (vi.fYscale + 1) <= y))
            {
            vi.fXscale++;
            vi.fYscale++;
            }
        }
#endif

        vi.fYscale *= scan_double;

        vi.sx = vsthw.xpix * vi.fXscale;
        vi.sy = vsthw.ypix * vi.fYscale;
        }

    if (vsthw.fMono && !FIsAtari8bit(vmCur.bfHW) && !v.fNoMono)
        {
        // create a monochrome bitmap, Win32s compatible code

        vsthw.bmiHeader.biSize = sizeof(BITMAPINFOHEADER) /* +sizeof(RGBQUAD)*2 */;
        vsthw.bmiHeader.biWidth = vsthw.xpix;
        vsthw.bmiHeader.biHeight = -vsthw.ypix;
        vsthw.bmiHeader.biPlanes = 1;
        vsthw.bmiHeader.biBitCount = 1;
        vsthw.bmiHeader.biCompression = BI_RGB;
        vsthw.bmiHeader.biSizeImage = 0;
        vsthw.bmiHeader.biXPelsPerMeter = 2834;
        vsthw.bmiHeader.biYPelsPerMeter = 2834;
        vsthw.bmiHeader.biClrUsed = 0*2;
        vsthw.bmiHeader.biClrImportant = 0*2;
        vsthw.lrgb[0] = 0;
        vsthw.lrgb[1] = 0x00FFFFFF;

        vi.cbScan = min(x, vsthw.xpix) / 8;
        }
    else
        {
        // create a 256 color bitmap buffer

        vsthw.bmiHeader.biSize = sizeof(BITMAPINFOHEADER) /* +sizeof(RGBQUAD)*2 */;
        vsthw.bmiHeader.biWidth = vsthw.xpix;
        vsthw.bmiHeader.biHeight = -vsthw.ypix;
        vsthw.bmiHeader.biPlanes = 1;
        vsthw.bmiHeader.biBitCount = 8;
        vsthw.bmiHeader.biCompression = BI_RGB;
        vsthw.bmiHeader.biSizeImage = 0;
        vsthw.bmiHeader.biXPelsPerMeter = 2834;
        vsthw.bmiHeader.biYPelsPerMeter = 2834;
        vsthw.bmiHeader.biClrUsed = 256; // FIsAtari8bit(vmCur.bfHW) ? 256 : 26;
        vsthw.bmiHeader.biClrImportant = 0;

        vi.cbScan = min(x, vsthw.xpix);
        }

    // Create offscreen bitmap

    vvmi.hbm = CreateDIBSection(vi.hdc, (CONST BITMAPINFO *)&vsthw.bmiHeader,
        DIB_RGB_COLORS, &vpvmi->pvBits, NULL, 0);
    vvmi.hdcMem = CreateCompatibleDC(vi.hdc);

    vvmi.hbmOld = SelectObject(vvmi.hdcMem, vvmi.hbm);

    SetBitmapColors();
    //FCreateOurPalette();

// PrintScreenStats(); printf("Entering CreateNewBitmap: leaving critical section\n");

    if (v.fFullScreen)
    {
        // Get rid of the title bar and menu and maximize
		ShowWindow(vi.hWnd, SW_MAXIMIZE); // this has to go first or it might not work!
		SetMenu(vi.hWnd, NULL);
        ULONG l = GetWindowLong(vi.hWnd, GWL_STYLE);
		SetWindowLong(vi.hWnd, GWL_STYLE, l & ~(WS_CAPTION | WS_SYSMENU | WS_SIZEBOX));
	}
	else
	{
        // Enable title bar and menu
		ULONG l = GetWindowLong(vi.hWnd, GWL_STYLE);
		SetMenu(vi.hWnd, vi.hMenu);
		SetWindowLong(vi.hWnd, GWL_STYLE, l | (WS_CAPTION | WS_SYSMENU | WS_SIZEBOX));
		ShowWindow(vi.hWnd, SW_RESTORE);	// this has to go last or the next maximize doesn't work!
	}


#if 0
    v.rectWinPos = rectSav; // restore saved window position

    SetWindowPos(vi.hWnd, HWND_NOTOPMOST, v.rectWinPos.left, v.rectWinPos.top, 0, 0,
        SWP_NOACTIVATE | SWP_NOSIZE);
#endif

#if !defined(NDEBUG)
        printf("CreateNewBitmap setting windows position to x = %d, y = %d, r = %d, b = %d\n",
             v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right, v.rectWinPos.bottom);
#endif

    {
    RECT rect;
    GetClientRect(vi.hWnd, &rect);

    if ((rect.right < vsthw.xpix) ||  (rect.bottom < vsthw.ypix))
        {
        // This is in case WM_MIXMAXINFO is not issued on too small a window

        SetWindowPos(vi.hWnd, HWND_NOTOPMOST, 0, 0, vsthw.xpix, vsthw.ypix,
            SWP_NOACTIVATE | SWP_NOMOVE);
        }
    }

    if ((v.rectWinPos.left < -100) || (v.rectWinPos.top < -100))
        if (!IsIconic(vi.hWnd))
            {
#if 1 // def QQQ
            SetWindowPos(vi.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOSIZE);
#endif
            }

    if (vi.fInDirectXMode)
        {
        // if in full screen mode, force the Windows mouse off

        vi.fGEMMouse = vi.fVMCapture;

        // in full screen mode, the window is the whole screen

        vi.sx = GetSystemMetrics(SM_CXSCREEN);
        vi.sy = GetSystemMetrics(SM_CYSCREEN);
        }

    // screen mode is now set, window size is set, zoom is set,
    // so we can determine the x,y offset needed to center the display
    // output in the window.
    // works for both windowed and full screen modes since sx,sy are
    // the window size

    {
    RECT rect;
    GetClientRect(vi.hWnd, &rect);

#if !defined(NDEBUG)
    printf("cnbm: client rect = %d, %d, %d, %d\n", rect.top, rect.left, rect.right, rect.bottom);
#endif

    vi.sx = rect.right;
    vi.sy = rect.bottom;
    }

    vi.dx = (vi.sx - (vsthw.xpix * vi.fXscale)) / 2;
    vi.dy = (vi.sy - (vsthw.ypix * vi.fYscale)) / 2;

    // we may have moved the window, so we need to move the mouse capture rectangle too

    if (vi.fGEMMouse)
        {
        ShowWindowsMouse();
        ShowGEMMouse();
        }

// PrintScreenStats(); printf("Entering CreateNewBitmap: after window adjustments\n");

    DisplayStatus();
    // Sleep(20);   // give other windows a chance to redraw

    InvalidateRect(vi.hWnd, NULL, 0);

    MarkAllPagesDirty();

    UpdateOverlay();

    vi.fVideoEnabled = TRUE;

#ifdef XFORMER
    if (FIsAtari8bit(vmCur.bfHW))
        {
        ForceRedraw();
        }
#endif

// PrintScreenStats(); printf("Entering CreateNewBitmap: done!\n");
    return TRUE;
}

#if defined(ATARIST) || defined(SOFTMAC)

/****************************************************************************

    FUNCTION: EnterMode

    PURPOSE:  Gemulator 2.x/3.x/4.x compatible graphics mode switch

    COMMENTS: Expects $12 for 640x480 and higher for 800x600

****************************************************************************/

void EnterMode(BOOL fGraphics)
{
    if (vsthw.VESAmode == 0x00)
        {
        // standard Atari ST resolutions

        switch (vsthw.shiftmd)
            {
        default:
            Assert(FALSE);
            break;

        case 0:
            vsthw.planes = 4;
            vsthw.xpix = 320;
            vsthw.ypix = 200;
            break;

        case 1:
            vsthw.planes = 2;
            vsthw.xpix = 640;
            vsthw.ypix = 200;
            break;

        case 2:
            vsthw.planes = 1;
            vsthw.xpix = 640;
            vsthw.ypix = 400;
            break;
            }
        }
    else if (vsthw.VESAmode <= 0x12)
        {
        // VGA driver asking for 640x480 mode

        vsthw.xpix = 640;
        vsthw.ypix = 480;
        }
    else if (vsthw.VESAmode < 0xFF)
        {
        // VGA driver asking for 800x600 mode

        vsthw.xpix = 800;
        vsthw.ypix = 600;
        }
    else
        {
        // VGA 4.0 driver asking for custom resolution

        vsthw.xpix = HIWORD(vpregs->A4);
        vsthw.ypix = LOWORD(vpregs->A4);
        }

    CreateNewBitmap();
}

#endif // ATARIST

//
// Return TRUE if we can successfully change to a different monitor
//

BOOL FToggleMonitor()
{
    ULONG bfSav = vmCur.bfMon;
    int i;

    // loop through all the bits in wfMon trying to find a different bit

    for(;;)
        {
        vmCur.bfMon <<= 1;

        if (vmCur.bfMon == 0)
            vmCur.bfMon = 1;

        if (vmCur.bfMon & vmCur.pvmi->wfMon)
            break;
        }

    // if it's the same bit, no other monitors

    if (bfSav == vmCur.bfMon)
        {
#if 0
        MessageBeep(0xFFFFFFFF);
#endif
        return FALSE;
        }

    // We need to guard the video critical section since
    // we are modifying the global vsthw.fMono which both
    // the pallete and redraw routines depend on.

    vsthw.rgrgb[0].rgbRed = 255;
    vsthw.rgrgb[0].rgbGreen = 255;
    vsthw.rgrgb[0].rgbBlue = 255;
    vsthw.rgrgb[0].rgbReserved = 0;

    for (i = 1; i < 256; i++)
        {
        vsthw.rgrgb[i].rgbRed = 0;
        vsthw.rgrgb[i].rgbGreen = 0;
        vsthw.rgrgb[i].rgbBlue = 0;
        vsthw.rgrgb[i].rgbReserved = 0;
        }

    vsthw.fMono = FMonoFromBf(vmCur.bfMon);

#if 1
      SetBitmapColors();
//    FCreateOurPalette();
#endif

    InvalidateRect(vi.hWnd, NULL, 0);
#ifdef XFORMER
    if (FIsAtari8bit(vmCur.bfHW))
        {
        ForceRedraw();
        }
#endif
    MarkAllPagesDirty();

    return TRUE;
}

//
// Return TRUE if we can successfully toggle to VM iVM
// or find the next VM if iVM == -1
//
// Right now hard coded to toggle Atari 8-bit/Atari ST/Mac modes
//

BOOL FToggleMacAtari(unsigned iVM, BOOL fRefresh)
{
    BOOL fLoop = (iVM == -1);

    if (v.cVM == 0)
        return FALSE;

    if (fLoop)
        iVM = v.iVM + 1;

loop:
    // wrap around to first VM

    if (iVM >= v.cVM)
        iVM = 0;

    if (!v.rgvm[iVM].fValidVM)
        {
        iVM++;
        goto loop;
        }

    // there is a valid ROM, go switch VMs

//  FUnInitVM();

    v.iVM = iVM;
    vi.pvmCur = &v.rgvm[v.iVM];    // update vmCur / vmiCur any time v.iVM changes!
    vi.pvmiCur = &vrgvmi[v.iVM];
    vpvm = vmCur.pvmi;

    if (!vvmi.fInited)
        {
        vmCur.fColdReset = TRUE;
        vvmi.fInited = TRUE;
        }

    if (fRefresh)
        CreateNewBitmap();

    return TRUE;
}

//
// Reboot the current VM
//

void ColdStart()
{
	
    ShowWindowsMouse();
    SetWindowsMouse(0);

    // User may have just mucked with Properties

    vsthw.fMono = FMonoFromBf(vmCur.bfMon);

    // make sure RAM setting not corrupted!
    // if it is, select the smaller RAM setting of the VM

    if ((vmCur.bfRAM & vmCur.pvmi->wfRAM) == 0)
        vmCur.bfRAM = BfFromWfI(vmCur.pvmi->wfRAM, 0);

    vi.cbRAM[0] = CbRamFromBf(vmCur.bfRAM);
    vi.eaROM[0] = osCur.eaOS;
    vi.cbROM[0] = osCur.cbOS;

    vvmi.keyhead = vvmi.keytail = 0;
    vi.fVMCapture = FALSE;

    if (FIsMac(vmCur.bfHW))
        {
        vsthw.planes = 1;
        vsthw.xpix = 512;
        vsthw.ypix = 342;
        }
    else if (FIsAtari8bit(vmCur.bfHW))
        {
        vsthw.planes = 8;
        vsthw.xpix = X8;
        vsthw.ypix = Y8;
        }
    else if (vsthw.fMono)
        {
        vsthw.planes = 1;
        vsthw.xpix = 640;
        vsthw.ypix = 400;
        }
    else // color Atari ST defaults to lo rez
        {
        vsthw.planes = 4;
        vsthw.xpix = 320;
        vsthw.ypix = 200;
        }

    PatBlt(vi.hdc, 0, 0, 4096, 2048, BLACKNESS);
    PatBlt(vvmi.hdcMem, 0, 0, 4096, 2048, BLACKNESS);

#if _ENGLISH
        SetWindowText(vi.hWnd, "Rebooting...");
#elif _NEDERLANDS
        SetWindowText(vi.hWnd, "Rebooting...");
#elif _DEUTSCH
        SetWindowText(vi.hWnd, "Neustart...");
#elif _FRANCAIS
        SetWindowText(vi.hWnd, "Redimarrer le GEM...");
#elif
#error
#endif

    // reboots faster if video thread is disabled

    MarkAllPagesClean();

    FUnInitVM();
    vi.fExecuting = FALSE;
    vi.fExecuting = FInitVM() && CreateNewBitmap() && FColdbootVM();

#if !defined(NDEBUG)
    fDebug++;
    printf("RAM size = %dK\n", vi.cbRAM[0] / 1024);
    printf("ROM size = %dK\n", vi.cbROM[0] / 1024);
    printf("ROM addr = $%08X\n", vi.eaROM[0]);

    if (vi.fExecuting)
        {
        switch(vmPeekL(vi.eaROM[0]))
            {
        default:         printf("Unknown ROM!\n");              break;

        case 0x0000AAAA: printf("Atari 8-bit\n");               break;

        case 0x602E0100: printf("Atari ST TOS 1.00\n");         break;
        case 0x602E0102: printf("Atari ST TOS 1.02\n");         break;
        case 0x602E0104: printf("Atari ST TOS 1.04\n");         break;
        case 0x602E0106: printf("Atari STE TOS 1.06\n");        break;
        case 0x602E0162: printf("Atari STE TOS 1.62\n");        break;
        case 0x602E0200: printf("Atari STE TOS 2.00\n");        break;
        case 0x602E0205: printf("Atari STE TOS 2.05\n");        break;
        case 0x602E0206: printf("Atari STE TOS 2.06\n");        break;
        case 0x602E0305: printf("Atari STE TOS 3.05\n");        break;
        case 0x602E0306: printf("Atari STE TOS 3.06\n");        break;

        case 0x064DC91D: printf("Mac LC 580\n");                break;
        case 0x06684214: printf("Mac LC 630\n");                break;
        case 0x28ba4e50: printf("Mac 512 A\n");                 break;
        case 0x28ba61ce: printf("Mac 512 B\n");                 break;
        case 0x3193670e: printf("Mac Classic II\n");            break;
        case 0x350eacf0: printf("Mac LC\n");                    break;
        case 0x35c28f5f: printf("Mac LC II\n");                 break;
        case 0x368cadfe: printf("Mac IIci\n");                  break;
        case 0x36b7fb6c: printf("Mac IIsi\n");                  break;
        case 0x3dc27823: printf("Quadra 950\n");                break;
        case 0x4147dd77: printf("Mac IIfx\n");                  break;
        case 0x420dbff3: printf("Quadra 700 / Quadra 900\n");   break;
        case 0x4957EB49: printf("Mac IIvi / IIvx\n");           break;
        case 0x4d1eeee1: printf("Mac Plus A (Hearts)\n");       break;
        case 0x4d1eeae1: printf("Mac Plus B (Heifers\n");       break;
        case 0x4d1f8172: printf("Mac Plus C (Harmonicas)\n");   break;
        case 0x5bf10fd1: printf("Quadra AV\n");                 break;
        case 0x63abfd3f: printf("Powerbook 5300\n");            break;
        case 0x960e4be9: printf("Power Mac 8600\n");            break;
        case 0x96cd923d: printf("PowerPC Unknown\n");           break;
        case 0x97221136: printf("Mac II (1.3) / IIx / IIcx\n"); break;
        case 0x9779d2c4: printf("Mac II (1.2)\n");              break;
        case 0x97851db6: printf("Mac II (1.0)\n");              break;
        case 0x9a5dc01f: printf("Power Mac 7100\n");            break;
        case 0x9feb69b3: printf("Power Mac 6100\n");            break;
        case 0xa49f9914: printf("Mac Classic\n");               break;
        case 0xb2e362a8: printf("Mac SE 800K\n");               break;
        case 0xb306e171: printf("Mac SE HDFD\n");               break;
        case 0xe33b2724: printf("Powerbook 180\n");             break;
        case 0xecbbc41c: printf("Mac LC III\n");                break;
        case 0xecd99dc0: printf("Mac Color Classic\n");         break;
        case 0xf1a6f343: printf("Quadra 610 / Centris 650\n");  break;
        case 0xf1acad13: printf("Quadra 650\n");                break;
        case 0xff7439ee: printf("Mac LC 475 / Quadra 605\n");   break;
            }

        printf("ROM sig  = $%08X\n", vmPeekL(vi.eaROM[0]));
        printf("ROM ver  = $%04X\n", vmPeekW(vi.eaROM[0]+8));
        printf("ROM subv = $%04X\n", vmPeekW(vi.eaROM[0]+18));
        }
    printf("CPU type = %s\n", (long)PchFromBfRgSz(vi.fFake040 ? cpu68040 : vmCur.bfCPU, rgszCPU));
//    printf("CPU real = %s\n", (long)PchFromBfRgSz(vmCur.bfCPU, rgszCPU));
    fDebug--;
#endif

    if (!vi.fExecuting)
        {
        MessageBox (GetFocus(),
            "Error restarting emulator.",
            vi.szAppName, MB_OK|MB_ICONHAND);
        }
    DisplayStatus();

    if (v.fDebugMode)
        {
        vi.fInDebugger = TRUE;
        }
}


BOOL FVerifyMenuOption()
{
    if (!vi.fExecuting)
        return TRUE;

    ShowWindowsMouse();

#if defined(ATARIST) || defined(SOFTMAC)
    if (1 || IDOK == MessageBox (GetFocus(),
#if _ENGLISH
            "The settings you have changed require rebooting Gemulator.\n"
            "Press OK to reboot or Cancel to discard these changes.",
#elif _NEDERLANDS
             "Door het wijzigen van de instellingen moet GEM opnieuw gestart worden.\n"
              "Kies OK om opnieuw te starten of annuleer om deze wijzigingen ongedaan te maken.",
#elif _DEUTSCH
            "Die geänderteten Einstellungen erfordern einen Neustart von GEM.\n"
            "Wählen Sie OK zum Neustart oder Abbruch um die Änderungen zu verwerfen.",
#elif _FRANCAIS
            "Les réglages que vous modifiez nécessite le redémarrage du GEM.\n"
            "Appuyez sur OK pour redémarrer ou sur Annuler pour ignorer ces modifications.",
#elif
#error
#endif
            vi.szAppName, MB_OKCANCEL))
#endif // ATARIST
        {
        return TRUE;
        }

    return FALSE;
}


void UpdateMenuCheckmarks()
{
    if (!FIsAtari68K(vmCur.bfHW))
        {
        CheckMenuItem(vi.hMenu, 1, MF_BYPOSITION | MF_GRAYED);
        }

    // Display Options popup

    CheckMenuItem(vi.hMenu, IDM_DIRECTX,     v.fFullScreen  ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_ZOOM,        v.fZoomColor ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_DXNORMAL,    (v.fNoDDraw == 0) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_DXDELAY,     (v.fNoDDraw == 1) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_DXDISABLE,   (v.fNoDDraw == 2) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_SAVEONEXIT,  v.fSaveOnExit ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_HYBONEXIT,   v.fHibrOnExit ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(vi.hMenu, IDM_NOSCSI,      v.fNoSCSI     ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_NOTWOBUTTON, v.fNoTwoBut   ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_NOQBOOT,     v.fNoQckBoot  ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_NOJIT,       v.fNoJit      ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_NOMONO,      v.fNoMono     ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(vi.hMenu, IDM_SKIPSTARTUP, v.fSkipStartup ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(vi.hMenu, IDM_DEBUGMODE,   v.fDebugMode ? MF_CHECKED : MF_UNCHECKED);

    EnableMenuItem(vi.hMenu, IDM_ZAPCMOS,    FIsMac(vmCur.bfHW) ? MF_ENABLED : MF_GRAYED);

}

int PrintScreenStats()
{
#if !defined(NDEBUG)
    printf("Screen: %4d,%4d  Full: %4d,%4d  sx,sy: %4d,%4d  dx,dy: %4d,%4d  xpix,ypix: %4d,%4d  scale: %d,%d\n",
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        GetSystemMetrics(SM_CXFULLSCREEN),
        GetSystemMetrics(SM_CYFULLSCREEN),
        vi.sx, vi.sy,
        vi.dx, vi.dy,
        vsthw.xpix, vsthw.ypix,
        vi.fXscale, vi.fYscale
        );

    printf("        windows position x = %d, y = %d, r = %d, b = %d\n",
         v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right, v.rectWinPos.bottom);
#endif

    return 0;
}

//
// The emulated computer shut down, do something!
//

int ShutdownDetected()
{
    if (v.fSkipStartup)
        {
        TrayMessage(vi.hWnd, NIM_DELETE, 0,
            LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), NULL);
        PostMessage(vi.hWnd, WM_QUIT, 0, 0);
        }
    else
        PostMessage(vi.hWnd, WM_COMMAND, IDM_ABOUT, 0);

    return 0;
}


//
// Render the DIB section to the current window's device context
//

void RenderBitmap()
{
	RECT rect;

    GetClientRect(vi.hWnd, &rect);
	
#if !defined(NDEBUG)
//  printf("rnb: rect = %d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
//  PrintScreenStats();
#endif

	if (v.fTiling)
	{
		// Tiling

		int x, y, iVM = -1; // v.iVM;

		for (x = rect.left; x < rect.right; x += vsthw.xpix * vi.fXscale)
		{
			for (y = rect.top; y < rect.bottom; y += vsthw.ypix * vi.fYscale)
			{
				// advance to the next valid bitmap

				do
				{
					iVM = (iVM + 1) % MAXOSUsable;
					// printf("hdcMem[%d] = %p, %p\n", iVM, vrgvmi[iVM].hdcMem, vrgvmi[iVM].pvBits);
				} while (vrgvmi[iVM].hdcMem == NULL);

				StretchBlt(vi.hdc, x, y,
					(vsthw.xpix * vi.fXscale), (vsthw.ypix * vi.fYscale),
					vrgvmi[iVM].hdcMem, 0, 0, vsthw.xpix, vsthw.ypix, SRCCOPY);
			}
		}
	}
	else if (v.fZoomColor)
	{
		// Smart Scaling

		StretchBlt(vi.hdc, rect.left, rect.top, rect.right, rect.bottom,
			vvmi.hdcMem, 0, 0, vsthw.xpix, vsthw.ypix, SRCCOPY);
	}
	else
    {
        // Integer scaling

        int x, y, scale;

        for (scale = 16; scale > 1; scale--)
            {
            x = (rect.right - rect.left) - (vsthw.xpix * vi.fXscale * scale);
            y = (rect.bottom - rect.top) - (vsthw.ypix * vi.fYscale * scale);

            if ((x >= 0) && (y >= 0))
                break;
            }

        // Repeat for the case of scale==1 when rectangle doesn't fit

        x = (rect.right - rect.left) - (vsthw.xpix * vi.fXscale * scale);
        y = (rect.bottom - rect.top) - (vsthw.ypix * vi.fYscale * scale);

		// Why did we not used to have to do this?
		StretchBlt(vi.hdc, 0, 0, x/2, rect.bottom, vvmi.hdcMem, 0, 0, 0, 0, BLACKNESS);
		StretchBlt(vi.hdc, x/2, 0, x/2 + (vsthw.xpix * vi.fXscale * scale), y/2, vvmi.hdcMem, 0, 0, 0, 0, BLACKNESS);
		StretchBlt(vi.hdc, x/2, y/2 + (vsthw.ypix * vi.fYscale * scale), x / 2 + (vsthw.xpix * vi.fXscale * scale), rect.bottom,
				vvmi.hdcMem, 0, 0, 0, 0, BLACKNESS);
		StretchBlt(vi.hdc, x/2 + (vsthw.xpix * vi.fXscale * scale), 0, rect.right, rect.bottom, vvmi.hdcMem, 0, 0, 0, 0, BLACKNESS);

		StretchBlt(vi.hdc, x/2, y/2,
             (vsthw.xpix * vi.fXscale * scale), (vsthw.ypix * vi.fYscale * scale),
             vvmi.hdcMem, 0, 0, vsthw.xpix, vsthw.ypix, SRCCOPY);
        }

    if (vi.pbFrameBuf && vvmi.pvBits)
    {
        int x, y;

        for (y = 0; y < vsthw.ypix; y++)
        {
            BYTE *pb = ((BYTE *)vvmi.pvBits) + y * vi.cbScan;

            for (x = 0; x < vsthw.xpix; x++)
            {
                vi.pbFrameBuf[y*vsthw.xpix + x] = *pb++;
            }
        }
    }
}


/****************************************************************************

    FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages

    MESSAGES:

    WM_COMMAND    - application menu (About dialog box)
    WM_DESTROY    - destroy window

    COMMENTS:

    To process the IDM_ABOUT message, call MakeProcInstance() to get the
    current instance address of the About() function.  Then call Dialog
    box which will create the box according to the information in your
    generic.rc file and turn control over to the About() function.  When
    it returns, free the intance address.

****************************************************************************/

LRESULT CALLBACK WndProc(
        HWND hWnd,     // window handle
        UINT message,      // type of message
        WPARAM uParam,     // additional information
        LPARAM lParam)     // additional information
{
    int wmId, wmEvent;
    Assert((hWnd == vi.hWnd) || (vi.hWnd == NULL));

    switch (message)
        {
    case WM_CREATE:
        vi.hdc = GetDC(hWnd);
        SetTextAlign(vi.hdc, TA_NOUPDATECP);
        SetTextColor(vi.hdc, RGB(0,255,0));  // set green text
        SetBkMode(vi.hdc, TRANSPARENT);

        //FCreateOurPalette();
		
        if (!FIsAtari8bit(vmCur.bfHW))
            {
            // create a default monochrome bitmap

            vsthw.planes = 1;
            vsthw.xpix = 640;
            vsthw.ypix = 400;
            }

        else
            {
            // create a default 256-color bitmap

            vsthw.planes = 8;
            vsthw.xpix = X8;
            vsthw.ypix = Y8;	// was 200
            }

        CreateNewBitmap();

//        SetTimer(hWnd, 0, 100, NULL);

//        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

//        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

        TrayMessage(hWnd, NIM_ADD, 0,
            LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), NULL);

        return 0;
        break;

    case WM_MOVE:
        // Only grab the new window position if we're not zoomed to the hilt
        // otherwise we'll grab a position of (0,0)

#if !defined(NDEBUG)
        printf("WM_MOVE before: x = %d, y = %d\n",
             v.rectWinPos.left, v.rectWinPos.top);
#endif

        if (!v.fFullScreen)
          if ((vsthw.xpix * vi.fXscale) < GetSystemMetrics(SM_CXSCREEN))
          if ((vsthw.ypix * vi.fYscale) < GetSystemMetrics(SM_CYSCREEN))
          if (!IsIconic(vi.hWnd))
            GetWindowRect(hWnd, (LPRECT)&v.rectWinPos);

#if !defined(NDEBUG)
        printf("WM_MOVE after:  x = %d, y = %d\n",
             v.rectWinPos.left, v.rectWinPos.top);
#endif

        break;

    case WM_SIZE:
#if !defined(NDEBUG)
        printf("WM_SIZE x = %d, y = %d\n", LOWORD(lParam), HIWORD(lParam));
#endif

        if (vi.fInDirectXMode)
          {
            RECT Rect;
			// !!! I don't trust this
            SetRect(&Rect, 0, GetSystemMetrics(SM_CYCAPTION), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            AdjustWindowRectEx(&Rect, WS_POPUP | WS_CAPTION, FALSE, 0);
            SetWindowPos(vi.hWnd, NULL, Rect.left, Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, SWP_NOACTIVATE | SWP_NOZORDER);
          }

		if (!v.fFullScreen)
			if ((vsthw.xpix * vi.fXscale) < GetSystemMetrics(SM_CXSCREEN))
				if ((vsthw.ypix * vi.fYscale) < GetSystemMetrics(SM_CYSCREEN))
					if (!IsIconic(vi.hWnd))
						GetWindowRect(hWnd, (LPRECT)&v.rectWinPos);
		
    //    CreateNewBitmap();
    //    return 0;
        break;

    case WM_DISPLAYCHANGE:
#if !defined(NDEBUG)
        printf("WM_DISPLAY\n");
#endif
        printf("WM_DISPLAY x = %d, y = %d\n", LOWORD(lParam), HIWORD(lParam));

        if (vi.fInDirectXMode)
          {
            RECT Rect;

            SetRect(&Rect, 0, GetSystemMetrics(SM_CYCAPTION), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            AdjustWindowRectEx(&Rect, WS_POPUP | WS_CAPTION, FALSE, 0);
            SetWindowPos(vi.hWnd, NULL, Rect.left, Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, SWP_NOACTIVATE | SWP_NOZORDER);
          }
        CreateNewBitmap();
   //   return 0;
        break;

#if !defined(NDEBUG)
    case WM_SIZING:
        {
        LPRECT lprect = (LPRECT)lParam;
        BYTE ratio = vi.fYscale / vi.fXscale;
        int thickX = GetSystemMetrics(SM_CXSIZEFRAME) * 2;
        int thickY = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);

            RECT rect;
            GetClientRect(hWnd, &rect);

        printf("WM_SIZING: lprect = %p\n", lprect);
        printf("WM_SIZING: client rect = %d, %d\n", rect.right, rect.bottom);
        printf("WM_SIZING #1: x = %d, y = %d, w = %d, h = %d\n",
            lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top);

break;
#if 0
        if ((vsthw.xpix == 0) || (vsthw.ypix == 0))
            return 0;

        vi.fXscale = max(1, (lprect->right - lprect->left - thickX) /  vsthw.xpix);
        vi.fYscale = max(1, (lprect->bottom - lprect->top - thickY) /  vsthw.ypix);

        vi.fYscale = vi.fXscale * ratio;

        if (1 /* fSnapping */)
            {
            lprect->right = lprect->left + vsthw.xpix * vi.fXscale + thickX;
            lprect->bottom = lprect->top + vsthw.ypix * vi.fYscale + thickY;
            }

        if (vi.fXscale == 1)
            v.fZoomColor = 0;

        printf("WM_SIZING #2: x = %d, y = %d, w = %d, h = %d\n",
            lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top);
        break;
#endif
	}
#endif

#if 1
    case WM_GETMINMAXINFO:
        {
        LPMINMAXINFO lpmm = (LPMINMAXINFO)lParam;

		RECT rc1, rc2;
		GetWindowRect(hWnd, &rc1);
		GetClientRect(hWnd, &rc2);

		int thickX = rc1.right - rc1.left - (rc2.right - rc2.left); // (SM_CXSIZEFRAME) * 2;
		int thickY = rc1.bottom - rc1.top - (rc2.bottom - rc2.top); // GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        int scaleX = 1, scaleY = ((vsthw.xpix) >= (vsthw.ypix * 3)) ? 2 : 1;

#if !defined(NDEBUG)
        printf("WM_GETMINMAXINFO, size(%d,%d) pos(%d,%d) min(%d,%d) max(%d,%d)\n",
            lpmm->ptMaxSize.x,
            lpmm->ptMaxSize.y,
            lpmm->ptMaxPosition.x,
            lpmm->ptMaxPosition.y,
            lpmm->ptMinTrackSize.x,
            lpmm->ptMinTrackSize.y,
            lpmm->ptMaxTrackSize.x,
            lpmm->ptMaxTrackSize.y);
#endif

		if (FIsAtari8bit(vmCur.bfHW)) {
			lpmm->ptMinTrackSize.x = max(vsthw.xpix * scaleX, X8 * 2) + thickX;
			lpmm->ptMinTrackSize.y = max(vsthw.ypix * scaleY, Y8 * 2) + thickY;
		}
		else
		{
			lpmm->ptMinTrackSize.x = max(vsthw.xpix * scaleX, 640) + thickX;
			lpmm->ptMinTrackSize.y = max(vsthw.ypix * scaleY, 480) + thickY;
		}
        break;

#if 0
        if ((vsthw.xpix == 0) || (vsthw.ypix == 0))
            return 0;

        if (v.fFullScreen && vi.fHaveFocus)
            return 0;

        while (((max(vsthw.xpix * (scaleX + 1), X8) + thickX) < lpmm->ptMaxTrackSize.x) &&
               ((max(vsthw.ypix * (scaleX + 1) * scaleY, 200) + thickY) < lpmm->ptMaxTrackSize.y))
            {
            scaleX++;
            }

        lpmm->ptMaxTrackSize.x = max(vsthw.xpix * scaleX, 320) + thickX;
        lpmm->ptMaxTrackSize.y = max(vsthw.ypix * scaleX * scaleY, 320) + thickY;

#if !defined(NDEBUG)
        printf("returning updated min(%d,%d) max(%d,%d)\n",
            lpmm->ptMinTrackSize.x,
            lpmm->ptMinTrackSize.y,
            lpmm->ptMaxTrackSize.x,
            lpmm->ptMaxTrackSize.y);
#endif

        break;
#endif
		}
        return 0;
#endif

    case WM_NCPAINT:
        if (vi.fInDirectXMode && vi.fHaveFocus)
            return 0;

        break;

    case WM_ERASEBKGND:

// PrintScreenStats(); printf("WM_ERASEBKGND message\n");

        PatBlt(vi.hdc, 0, 0,
            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            BLACKNESS);

        return TRUE; // indicates that background erased

    case WM_PAINT:
#if 0
        // if there is a video thread, don't do redraw here

        if ((vi.fInDirectXMode && vi.fHaveFocus) ||
            (!FIsAtari8bit(vmCur.bfHW) && vi.hVideoThread))
            {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            MarkAllPagesDirty();
            UpdateOverlay();
            EndPaint(hWnd, &ps);
// PrintScreenStats(); printf("WM_PAINT after DirectX mark pages dirty\n");
            return 0;
            }
#endif

         {
        // generic redraw doesn't have to be extremely fast

        PAINTSTRUCT ps;

        BeginPaint(hWnd, &ps);
        RenderBitmap();
        UpdateOverlay();

        EndPaint(hWnd, &ps);
// PrintScreenStats(); printf("WM_PAINT after GDI StretchBlt\n");
        }
        return 0;

#if 0
    case WM_PALETTECHANGED:
        if ((HWND)uParam == vi.hWnd)
            break;
        // fall through

    case WM_QUERYNEWPALETTE:
        if (FCreateOurPalette())
            InvalidateRect(hWnd, NULL, TRUE);
        return 0;
        break;
#endif

       case WM_SETCURSOR:
        if (v.fFullScreen && vi.fHaveFocus)
            {
            // In full screen mode, don't show mouse anywhere
            SetCursor(NULL);
            return TRUE;
            }

		if (vi.fHaveFocus && (LOWORD(lParam) == HTCLIENT))
		{
			if (FIsAtari8bit(vmCur.bfHW))
			{
				// Set cursor to crosshairs for 8 bit
				SetCursor(LoadCursor(NULL, IDC_CROSS));
				return TRUE;
			}
			else if (!vi.fVMCapture)
			{
				// In windowed mode, don't show mouse in client area
				SetCursor(NULL);
				return TRUE;
			}
		}
        break;

//     case WM_ACTIVATEAPP:
       case WM_ACTIVATE:
        DebugStr("WM_ACTIVATE %d\n", LOWORD(uParam));

//        if (LOWORD(uParam))  // fACtivate)
        if (LOWORD(uParam) != WA_INACTIVE)
        {
            CreateNewBitmap();
        }

        else if (vi.fInDirectXMode)
            {
            v.fFullScreen = !v.fFullScreen;

            // release DirectX mode
            MarkAllPagesClean();
            UninitDrawing(TRUE);
            vi.fInDirectXMode = FALSE;

            CreateNewBitmap();
            }

        break;

    case WM_SETFOCUS:
        DebugStr("WM_SETFOCUS\n");

        vi.fHaveFocus = TRUE;
        ShowGEMMouse();

#if !defined(NDEBUG)
        DisplayStatus();
#endif

        if (FIsMac(vmCur.bfHW))
            {
            vi.fRefreshScreen = TRUE;
            AddToPacket(0xF5);  // Ctrl key up
            AddToPacket(0xDF);  // Alt  key up
            }
        else if (FIsAtari68K(vmCur.bfHW))
            {
            vi.fRefreshScreen = TRUE;
            AddToPacket(0x9D);  // Ctrl key up
            AddToPacket(0xB8);  // Alt  key up
            }
        else if (FIsAtari8bit(vmCur.bfHW))
            {
#ifdef XFORMER
            ControlKeyUp8();
            ForceRedraw();
#endif
		}

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        return 0;
        break;

    case WM_KILLFOCUS:
        DebugStr("WM_KILLFOCUS\n");
#if 0
        // release DirectX mode
        MarkAllPagesClean();
        UninitDrawing(TRUE);
        vi.fInDirectXMode = FALSE;
#endif

		ShowWindowsMouse();
        ReleaseCapture();

#if !defined(NDEBUG)
        DisplayStatus();
#endif
        vi.fHaveFocus = FALSE;

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        return 0;
        break;

    case MYWM_NOTIFYICON:
        switch (lParam)
            {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            goto L_about;

        default:
            break;
            }
        break;

#if 0
    case WM_SYSCOMMAND:
        switch(uParam&0xFFF0)
            {
        default:
            break;

        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;

        case SC_MAXIMIZE:
            // for now just toggle zoom state

        //  v.fZoomColor = !v.fZoomColor;
        //  CreateNewBitmap();
        //  return 0;
;
            }
        break;
#endif

    case WM_COMMAND:  // message: command from application menu

        wmId    = LOWORD(uParam);
        wmEvent = HIWORD(uParam);

        switch (wmId)
            {
L_about:
		// bring up our ABOUT MessageBox
		case IDM_ABOUT:

			char rgch[1120], rgch2[64], rgchVer[32];
			OSVERSIONINFO oi;

			oi.dwOSVersionInfoSize = sizeof(oi);
			GetVersionEx(&oi);  // REVIEW: requires Win32s 1.2+

			switch (oi.dwPlatformId)
			{
			default:
			case VER_PLATFORM_WIN32_WINDOWS:
				strcpy(rgchVer, "Windows 95/98/Me");
				break;

			case VER_PLATFORM_WIN32s:
				strcpy(rgchVer, "Win32s");
				break;

			case VER_PLATFORM_WIN32_NT:
				strcpy(rgchVer, "Windows");

				if (oi.dwMajorVersion < 5)
				{
					strcpy(rgchVer, "Windows NT");
				}
				else if (oi.dwMajorVersion == 5)
				{
					if (oi.dwMinorVersion == 0)
					{
						strcpy(rgchVer, "Windows 2000");
					}
					else if (oi.dwMinorVersion == 1)
					{
						strcpy(rgchVer, "Windows XP");
					}
					else
					{
						strcpy(rgchVer, "Windows 2003");
					}
				}
				else if (oi.dwMajorVersion == 6)
				{
					if ((oi.dwBuildNumber & 65535) <= 6000)
					{
						strcpy(rgchVer, "Windows Vista");
					}
					else if (oi.dwMinorVersion == 0)
					{
						strcpy(rgchVer, "Windows Vista / Server 2008");
					}
					else if (oi.dwMinorVersion == 1)
					{
						strcpy(rgchVer, "Windows 7 / Server 2008 R2");
					}
					else if (oi.dwMinorVersion == 2)
					{
						if ((oi.dwBuildNumber & 65535) < 8400)
							strcpy(rgchVer, "Windows 8 Beta");
						else if ((oi.dwBuildNumber & 65535) == 8400)
							strcpy(rgchVer, "Windows 8 Consumer Preview");
						else if ((oi.dwBuildNumber & 65535) < 9200)
							strcpy(rgchVer, "Windows 8 pre-release");
						else
							strcpy(rgchVer, "Windows 8");
					}
					else
					{
						strcpy(rgchVer, "Windows 8 or later");
					}
				}
				else
				{
					strcpy(rgchVer, "Windows 8 or later");
				}
				break;
			};

			sprintf(rgch2, "About %s", vi.szAppName);

			sprintf(rgch, "%s Community Release\n"
				"Darek's Classic Computer Emulator.\n"
				"Version 9.90 - built on %s\n"
				"%2d-bit %s release.\n\n"
				"Copyright (C) 1986-2018 Darek Mihocka.\n"
				"All Rights Reserved.\n\n"

#ifdef XFORMER
				"Atari OS and BASIC used with permission.\n"
				"Copyright (C) 1979-1984 Atari Corp.\n\n"
#endif

				"Many thanks to: "
				"Ignac Kolenko, "
				"Ed Malkiewicz, "
				"Bill Huey, "
				"\n"
#if defined(ATARIST) || defined(SOFTMAC)
				"Christian Bauer, "
				"Simon Biber, "
				"\n"
				"William Condie, "
				"Phil Cummins, "
				"Kyle Fox, "
				"\n"
				"Manuel Perez, "
				"Ray Ruvinskiy, "
				"Joel Walter, "
				"\n"
				"Jim Watters, "
#endif
				"Danny Miller, "
				"Robert Birmingham, "
				"and Derek Yenzer.\n\n"

				"Windows version: %d.%02d (build %d)\n"
				"Windows platform: %s\n"
				,
				vi.szAppName,
				__DATE__,
				sizeof(void *) * 8,
#if defined(_M_AMD64)
				"x64",
#elif defined(_M_IX86)
				"x86",
#elif defined(_M_IX86)
				"ARM",
#else
				"",
#endif
				oi.dwMajorVersion, oi.dwMinorVersion,
				oi.dwBuildNumber & 65535,
				rgchVer,
				0);

			MessageBox(GetFocus(), rgch, rgch2, MB_OK);
			break;

		case IDM_FULLSCREEN:
			v.fFullScreen = !v.fFullScreen;
			CheckMenuItem(vi.hMenu, IDM_FULLSCREEN, v.fFullScreen ? MF_CHECKED : MF_UNCHECKED);
			
			// remove title bar and menus and such
			CreateNewBitmap();
			break; // !!! the VM sees the ENTER keystroke

		case IDM_STRETCH:
			v.fZoomColor = !v.fZoomColor;
			CheckMenuItem(vi.hMenu, IDM_STRETCH, v.fZoomColor ? MF_CHECKED : MF_UNCHECKED);
			CreateNewBitmap();
			break;

		case IDM_TILE:
			v.fTiling = !v.fTiling;
			CheckMenuItem(vi.hMenu, IDM_TILE, v.fTiling ? MF_CHECKED : MF_UNCHECKED);
			CreateNewBitmap();
			break;

		case IDM_COLORMONO:
			if (!FToggleMonitor())
				return 0;

			// don't reboot, let the OS detect it as necessary
			return 0;

			MENUITEMINFO mii;
			char mNew[MAX_PATH + 10];

#ifdef XFORMER
		case IDM_CART:

			if (OpenTheFile(vi.hWnd, vi.pvmCur->rgcart.szName, FALSE, 1))
			{
				ReadCart();

				EnableMenuItem(vi.hMenu, IDM_NOCART, 0);
				
				// fix the name of this menu item
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				sprintf(mNew, "&Cartridge %s ...", vi.pvmCur->rgcart.szName);
				SetMenuItemInfo(vi.hMenu, IDM_CART, FALSE, &mii);

				CreateVMMenu();	// fix the FILE menu
				SendMessage(vi.hWnd, WM_COMMAND, IDM_COLDSTART, 0);
			}
			break;

		case IDM_NOCART:
			vi.pvmCur->rgcart.fCartIn = FALSE;	// unload the cartridge and cold start
			vi.pvmCur->rgcart.szName[0] = 0;
			EnableMenuItem(vi.hMenu, IDM_NOCART, MF_GRAYED);
			
			// fix the name of this menu item
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = mNew;
			sprintf(mNew, "&Cartridge...");
			SetMenuItemInfo(vi.hMenu, IDM_CART, FALSE, &mii);
			
			CreateVMMenu();	// fix the FILE menu
			SendMessage(vi.hWnd, WM_COMMAND, IDM_COLDSTART, 0);
			break;

#endif		

		case IDM_D1:
			if (OpenTheFile(vi.hWnd, vi.pvmCur->rgvd[0].sz, FALSE, 0))
			{
				vi.pvmCur->rgvd[0].dt = DISK_IMAGE; // !!! doesn't support DISK_WIN32, DISK_FLOPPY or DISK_SCSI
				FUnmountDiskVM(0);
				FMountDiskVM(0);

				// Fix the virtual disk menu items
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				if (FIsAtari8bit(vmCur.bfHW))	// 8 bit drives are called D1:, D2: etc.
					sprintf(mNew, "&D1: %s...", vi.pvmCur->rgvd[0].sz);
				else
					sprintf(mNew, "%s ...", vi.pvmCur->rgvd[0].sz);
				SetMenuItemInfo(vi.hMenu, IDM_D1, FALSE, &mii);
				EnableMenuItem(vi.hMenu, IDM_D1U, 0);

				DisplayStatus(); // this could change the title of the window
				CreateVMMenu();	// and also the FILE menu
			}
			break;

		case IDM_D2:
			if (OpenTheFile(vi.hWnd, vi.pvmCur->rgvd[1].sz, FALSE, 0))
			{
				vi.pvmCur->rgvd[1].dt = DISK_IMAGE; // !!! doesn't support DISK_WIN32, DISK_FLOPPY or DISK_SCSI
				FUnmountDiskVM(1);
				FMountDiskVM(1);

				// Initialize the virtual disk menu items
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				if (FIsAtari8bit(vmCur.bfHW))	// 8 bit drives are called D1:, D2: etc.
					sprintf(mNew, "&D2: %s...", vi.pvmCur->rgvd[1].sz);
				else
					sprintf(mNew, "%s ...", vi.pvmCur->rgvd[1].sz);
				SetMenuItemInfo(vi.hMenu, IDM_D2, FALSE, &mii);
				EnableMenuItem(vi.hMenu, IDM_D2U, 0);
			}
			break;

		case IDM_D3:
			if (OpenTheFile(vi.hWnd, vi.pvmCur->rgvd[2].sz, FALSE, 0))
			{
				vi.pvmCur->rgvd[2].dt = DISK_IMAGE; // !!! doesn't support DISK_WIN32, DISK_FLOPPY or DISK_SCSI
				FUnmountDiskVM(2);
				FMountDiskVM(2);

				// Initialize the virtual disk menu items
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				if (FIsAtari8bit(vmCur.bfHW))	// 8 bit drives are called D1:, D2: etc.
					sprintf(mNew, "&D3: %s...", vi.pvmCur->rgvd[2].sz);
				else
					sprintf(mNew, "%s ...", vi.pvmCur->rgvd[2].sz);
				SetMenuItemInfo(vi.hMenu, IDM_D3, FALSE, &mii);
				EnableMenuItem(vi.hMenu, IDM_D3U, 0);
			}
			break;

		case IDM_D4:
			if (OpenTheFile(vi.hWnd, vi.pvmCur->rgvd[3].sz, FALSE, 0))
			{
				vi.pvmCur->rgvd[3].dt = DISK_IMAGE; // !!! doesn't support DISK_WIN32, DISK_FLOPPY or DISK_SCSI
				FUnmountDiskVM(3);
				FMountDiskVM(3);

				// Initialize the virtual disk menu items
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				if (FIsAtari8bit(vmCur.bfHW))	// 8 bit drives are called D1:, D2: etc.
					sprintf(mNew, "&D4: %s...", vi.pvmCur->rgvd[3].sz);
				else
					sprintf(mNew, "%s ...", vi.pvmCur->rgvd[3].sz);
				SetMenuItemInfo(vi.hMenu, IDM_D4, FALSE, &mii);
				EnableMenuItem(vi.hMenu, IDM_D4U, 0);
			}
			break;

		// unmount a drive
		case IDM_D1U:
			strcpy(vi.pvmCur->rgvd[0].sz, "");
			vi.pvmCur->rgvd[0].dt = DISK_NONE;
			FUnmountDiskVM(0);
			
			// Initialize the virtual disk menu items
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = mNew;
			sprintf(mNew, "&D1: %s ...", vi.pvmCur->rgvd[0].sz);
			SetMenuItemInfo(vi.hMenu, IDM_D1, FALSE, &mii);
			EnableMenuItem(vi.hMenu, IDM_D1U, MF_GRAYED);

			DisplayStatus(); // this could change the title of the window
			CreateVMMenu();	// and also the FILE menu
			break;

		case IDM_D2U:
			strcpy(vi.pvmCur->rgvd[1].sz, "");
			vi.pvmCur->rgvd[1].dt = DISK_NONE;
			FUnmountDiskVM(1);
			
			// Initialize the virtual disk menu items
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = mNew;
			sprintf(mNew, "&D2: %s ...", vi.pvmCur->rgvd[1].sz);
			SetMenuItemInfo(vi.hMenu, IDM_D2, FALSE, &mii);
			EnableMenuItem(vi.hMenu, IDM_D2U, MF_GRAYED);
			break;
		
		case IDM_D3U:
			strcpy(vi.pvmCur->rgvd[2].sz, "");
			vi.pvmCur->rgvd[2].dt = DISK_NONE;
			FUnmountDiskVM(2);
			
			// Initialize the virtual disk menu items
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = mNew;
			sprintf(mNew, "&D3: %s ...", vi.pvmCur->rgvd[2].sz);
			SetMenuItemInfo(vi.hMenu, IDM_D3, FALSE, &mii);
			EnableMenuItem(vi.hMenu, IDM_D3U, MF_GRAYED);
			break;
		
		case IDM_D4U:
			strcpy(vi.pvmCur->rgvd[3].sz, "");
			vi.pvmCur->rgvd[3].dt = DISK_NONE;
			FUnmountDiskVM(3);
			
			// Initialize the virtual disk menu items
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = mNew;
			sprintf(mNew, "&D4: %s ...", vi.pvmCur->rgvd[3].sz);
			SetMenuItemInfo(vi.hMenu, IDM_D4, FALSE, &mii);
			EnableMenuItem(vi.hMenu, IDM_D4U, MF_GRAYED);
			break;
				
		
		
		case IDM_LOADSTATE:
            SaveState(FALSE);
            return 0;
            break;

        case IDM_PROPERTIES:
            DialogBox(vi.hInst,
                MAKEINTRESOURCE(IDD_PROPERTIES),
                vi.hWnd,
                Properties);
            if (!FIsAtari8bit(vmCur.bfHW))
                AddToPacket(FIsMac(vmCur.bfHW) ? 0xDF : 0xB8);  // Alt  key up
            break;

        case IDM_COLDSTART:
            // have some default values so as to force a default bitmap

            vmCur.fColdReset = TRUE;
            return 0;
            break;

        case IDM_F11:

            // Use F11 to also clear counters
#if PERFCOUNT
            cntInstr = 0;
            cntCodeMiss = 0;
            cntAccess = 0;
            cntDataMiss = 0;
#endif

            // On the Mac, refresh the Mac floppy disk drives

            if (FIsMac(vmCur.bfHW))
                {
#ifdef SOFTMAC
                vmachw.fDisk1Dirty = fTrue;
                vmachw.fDisk2Dirty = fTrue;
#endif
                // vsthw.fMediaChange = fTrue;

                FUnmountDiskVM(8);
                FMountDiskVM(8);
                }
            else

            // if the mouse is captured in window, release it
            // otherwise capture it

            if (vi.fGEMMouse && !vi.fInDirectXMode)
                {
                ShowWindowsMouse();
                }
            else
                {
                ShowGEMMouse();
                }

            return 0;
            break;

        case IDM_TOGGLEHW:
            FToggleMacAtari((unsigned)(-1), TRUE);
            return 0;
            break;

        case IDM_CTRLF11:
#if 0
            {
            POINT pt;

            vi.fHaveFocus = TRUE;  // HACK

            // FMouseMsgVM(hWnd, 0, 0, 0, 0);
            DisplayStatus();
            ShowWindowsMouse();
            GetCursorPos(&pt);
            UpdateMenuCheckmarks();
            TrackPopupMenu(vi.hMenu, TPM_LEFTALIGN, pt.x+8, pt.y+8, 0, hWnd, NULL);
            if (!FIsAtari8bit(vmCur.bfHW))
                AddToPacket(FIsMac(vmCur.bfHW) ? 0xF5 : 0x9D);  // Ctrl key up
            else
                {
                ControlKeyUp8();
                }
            DisplayStatus();
            }
            return 0;
            break;
#else
            DialogBox(vi.hInst,       // current instance
                MAKEINTRESOURCE(IDD_ABOUTBOX), // dlg resource to use
                hWnd,          // parent handle
                About);
            return 0;
            break;
#endif

        case IDM_DEBUGGER:
            ShowWindowsMouse();
            vi.fDebugBreak = TRUE;
            CreateDebuggerWindow();
            return 0;
            break;

        case IDM_DISKPROPS:
            DialogBox(vi.hInst,       // current instance
                MAKEINTRESOURCE(IDD_DISKS), // dlg resource to use
                hWnd,          // parent handle
                DisksDlg);
            if (!FIsAtari8bit(vmCur.bfHW))
                AddToPacket(FIsMac(vmCur.bfHW) ? 0xF1 : 0xAA);  // Shift key up
            return 0;
            break;

        case IDM_DIRECTX:
            v.fFullScreen = !v.fFullScreen;
            CreateNewBitmap();

            if (FIsMac(vmCur.bfHW))
                {
                AddToPacket(0xF5);  // Ctrl key up
                AddToPacket(0xDF);  // Alt  key up
                }
            else if (FIsAtari68K(vmCur.bfHW))
                {
                AddToPacket(0x9D);  // Ctrl key up
                AddToPacket(0xB8);  // Alt  key up
                }

            return 0;
            break;

        case IDM_EXIT:
#if 0
            if (IDOK != MessageBox (GetFocus(),
#if _ENGLISH
                    "Any unsaved documents will be lost.\n"
                    "Are you sure you want to quit?",
#elif _NEDERLANDS
                     "Alle niet bewaarde Atari-files zullen verloren gaan.\n"
                     "Weet u zeker dat u wilt stoppen?\n",
#elif _DEUTSCH
                    "Alle ungesicherten Atari Daten gehen verloren.\n"
                    "Wirklich beenden?\n",
#elif _FRANCAIS
                    "Tous les fichiers Atari non sauvegardés seront perdus.\n"
                    "Etes vous certain de vouloir quitter?",
#elif
#error
#endif
                    vi.szAppName, MB_OKCANCEL))
                {
                ShowWindowsMouse();
                return 0;
                }
#endif
            DestroyWindow (hWnd);
            return 0;
            break;

        // Here are all the other possible menu options,
        // all of these are currently disabled:

        default:
            return (DefWindowProc(hWnd, message, uParam, lParam));
            }

        return 0;
        break;

    case MM_JOY1MOVE:
    case MM_JOY1BUTTONDOWN:
    case MM_JOY1BUTTONUP:
    case MM_JOY2MOVE:
    case MM_JOY2BUTTONDOWN:
    case MM_JOY2BUTTONUP:
        if (vi.fExecuting)
            return FWinMsgVM(hWnd, message, uParam, lParam);
        break;

    case WM_SYSKEYDOWN:
		if (uParam == VK_RETURN)
		{
#if 0
			// Alt+Enter toggles windowed/full screen mode
			// This also toggles the menu bar on and off
			//
			// Cycles through five modes:
			//
			//  v.fFS v.fZm v.fT
			//  ===== ===== ====
			//    0     0    0    menu bar, center guest in window
			//    0     1    0    menu bar, scale guest to full window
			//    1     0    0    no menu bar, centre guest in window
			//    1     1    0    no menu bar, scale guest to full window
			//    1     0    1    tile window

			if (v.fZoomColor && v.fFullScreen) {
				v.fTiling = TRUE;
				v.fZoomColor = FALSE;
			}
			else if (v.fTiling)
			{
				v.fTiling = FALSE;
				v.fFullScreen = FALSE;
			}
			else
			{
				if (v.fZoomColor)
					v.fFullScreen = !v.fFullScreen;

				v.fZoomColor = !v.fZoomColor;
			}
			CreateNewBitmap();
			return 0;
		}
#endif
		SendMessage(vi.hWnd, WM_COMMAND, IDM_FULLSCREEN, 0);
		}

		// fall through

    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        // Catch the keystroke for Menu key so that it doesn't
        // register as an F10 (and reboot Atari BASIC!)
        // The Menu key still functions as expected.

        if (uParam == VK_APPS)
            break;

#if !defined(NDEBUG)
        printf("WM_KEY*: u = %08X l = %08X\n", uParam, lParam);
#endif
        printf("WM_KEY*: u = %08X l = %08X\n", uParam, lParam);

        if (uParam == VK_PAUSE)
            {
            ShowWindowsMouse();
            vi.fDebugBreak = TRUE;
            return 0;
            }

        vi.fHaveFocus = TRUE;  // HACK
        if (vi.fExecuting)
          if (((lParam & 0xC0000000) != 0x40000000) || FIsAtari8bit(vmCur.bfHW))
            return FWinMsgVM(hWnd, message, uParam, lParam);

        // fall through

    case WM_HOTKEY:
        return 0;
        break;

    case WM_RBUTTONDOWN:
        vi.fHaveFocus = TRUE;  // HACK
        if (vi.fExecuting && vi.fGEMMouse && !vi.fInDirectXMode &&
              !v.fNoTwoBut &&
              (!FIsAtari68K(vmCur.bfHW) || (uParam & MK_LBUTTON) ) )
            {
            // both buttons are being pressed, left first and now right
            // so bring back the Windows mouse cursor after sending
            // an upclick

            // see also code for F11

//            FMouseMsgVM(hWnd, 0, 0, 0, 0);
            ShowWindowsMouse();
            return 0;
            break;
            }

        // if the Windows mouse is visible OR we're in DirectX mode, go straight to the dialog
        if (!v.fNoTwoBut)
        if (!vi.fGEMMouse || (vi.fInDirectXMode && (!FIsAtari68K(vmCur.bfHW) || (uParam & MK_LBUTTON))))
		    {
            // fall through into F11 / both mouse button case

    case WM_CONTEXTMENU:
            DialogBox(vi.hInst,       // current instance
                MAKEINTRESOURCE(IDD_ABOUTBOX), // dlg resource to use
                hWnd,          // parent handle
                About);
            break;
            }

        // fall through into WM_LBUTTONDOWN case

    case WM_LBUTTONDOWN:
        vi.fHaveFocus = TRUE;  // HACK

#ifdef SOFTMAC
        vmachw.fVIA2 = TRUE;
#endif

        if (FIsAtari8bit(vmCur.bfHW))
            {
#if !defined(NDEBUG)
            printf("LBUTTONDOWN\n");
#endif
            return FWinMsgVM(hWnd, MM_JOY1BUTTONDOWN, JOY_BUTTON1, 0);
            return 0;
            }

        if (!vi.fVMCapture)
            return FWinMsgVM(hWnd, message, uParam, lParam);

        if (vi.fExecuting && vi.fVMCapture && !vi.fGEMMouse)
            {
            // either mouse button was pressed
            // so give special message to activate mouse

            ShowGEMMouse();
            return 0;
            break;
            }

        // fall through into generic mouse cases

    case WM_LBUTTONUP:
        if (FIsAtari8bit(vmCur.bfHW))
            {
#if !defined(NDEBUG)
            printf("LBUTTONUP\n");
#endif
            return FWinMsgVM(hWnd, MM_JOY1BUTTONUP, 0, 0);
            return 0;
            }

        if (!vi.fVMCapture)
            return FWinMsgVM(hWnd, message, uParam, lParam);

    case WM_RBUTTONUP:
        vi.fHaveFocus = TRUE;  // HACK
        if (vi.fExecuting && (vi.fGEMMouse || !vi.fVMCapture))
            {
            vi.fGEMMouse = FALSE;
            FWinMsgVM(hWnd, message, uParam, lParam);
            vi.fMouseMoved = FALSE;
            vi.fGEMMouse = vi.fVMCapture;
            }
        return 0;
        break;

    case WM_MOUSEMOVE:
        if (FIsAtari8bit(vmCur.bfHW))
            {
            FWinMsgVM(hWnd, message, uParam, lParam);
            return 0;
            }

        vi.fMouseMoved = TRUE;
        return 0;
        break;

    case WM_POWERBROADCAST:
        // If the power status changes, the CPU clock speed may also
        // change, and that's not good if we're using the CPU clock.
        // So in such an event force the hardware timer

        if (uParam == PBT_APMPOWERSTATUSCHANGE)
            {
            // the fact we got this message means clock speed may have
            // changed so recalibrate always

            CheckClockSpeed();
            }

        break;

    case WM_DESTROY:  // message: window being destroyed

        vi.fQuitting = TRUE;
		
		SetMenu(vi.hWnd, vi.hMenu); // so that it gets freed

		UninitSound();
		ReleaseJoysticks();

        // release DirectX mode
        MarkAllPagesClean();
        UninitDrawing(TRUE);
        vi.fInDirectXMode = FALSE;

        if (vi.fExecuting)
            {
            vi.fExecuting = FALSE;
            KillTimer(hWnd, 0);
            ReleaseCapture();
            SelectObject(vvmi.hdcMem, vvmi.hbmOld);
            DeleteObject(vvmi.hbm);
            DeleteDC(vvmi.hdcMem);
            FUnInitVM();
            }
        FInitSerialPort(0);
        ShowCursor(TRUE);

        if (v.fSaveOnExit)
            SaveProperties(NULL);

        TrayMessage(vi.hWnd, NIM_DELETE, 0,
            LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), NULL);

        Sleep(200); // let other threads finish

        PostQuitMessage(0);
        return 0;
        break;

    case WM_CLOSE:
        if (!FIsAtari8bit(vmCur.bfHW))
        {
#if 0
        // Not currently supported!

        if (v.fHibrOnExit)
            goto Lhib;

        switch(MessageBox (GetFocus(),
                    "Click Yes to Hibernate (save current state).\n"
                    "Click No to Exit (lose unsaved documents).\n"
                    "Click Cancel to return to the emulator.\n",
                vi.szAppName, MB_YESNOCANCEL))
            {
        default:
            ShowWindowsMouse();
            return 0;

        case IDYES:
Lhib:
            SetWindowText(vi.hWnd, "Hibernating...");

            v.fSkipStartup |= 2;

            if (SaveState(TRUE))
                PostQuitMessage(0);

            else
                {
                // if we got here during hibernate it means it failed

                v.fSkipStartup &= ~2;
                SaveProperties(NULL);
                MessageBeep(0xFFFFFFFF);
                }

            DisplayStatus();

            // fall through

        case IDNO:          // do a brute force quit
            TrayMessage(vi.hWnd, NIM_DELETE, 0,
                LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), NULL);
            break;
            }
#endif
        }

        // fall through

    default:      // Passes it on if unprocessed
        return (DefWindowProc(hWnd, message, uParam, lParam));
        }

    return (DefWindowProc(hWnd, message, uParam, lParam));
    return (0);
}

/****************************************************************************

    FUNCTION: CenterWindow (HWND, HWND)

    PURPOSE:  Center one window over another

    COMMENTS:

    Dialog boxes take on the screen position that they were designed at,
    which is not always appropriate. Centering the dialog over a particular
    window usually results in a better position.

****************************************************************************/

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    if (hwndParent == NULL)
        {
        // No parent, assume parent is the whole screen

        rParent.left = 0;
        rParent.top = 0;
        rParent.right = GetSystemMetrics(SM_CXSCREEN);
        rParent.bottom = GetSystemMetrics(SM_CYSCREEN);
        }
    else
        {
        // Get the Height and Width of the parent window
        GetWindowRect (hwndParent, &rParent);
        }
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return SetWindowPos (hwndChild, NULL,
        xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}



LRESULT CALLBACK Info(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    int wmId, wmEvent;

    switch (message)
        {
        case WM_INITDIALOG:

            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
            return (TRUE);

        case WM_COMMAND:              // message: received a command
            wmId    = LOWORD(uParam);
            wmEvent = HIWORD(uParam);

            switch (wmId)
                {
            case IDOK:
                EndDialog(hDlg, TRUE);    // Exit the dialog
                return (TRUE);
                }

            break;
        }
    return (FALSE); // Didn't process the message

    lParam; // This will prevent 'unused formal parameter' warnings
}

#if 1       // use listbox for mode selection

#define XB_RESETCONTENT     LB_RESETCONTENT
#define XB_ADDSTRING        LB_ADDSTRING
#define XB_DELETESTRING     LB_DELETESTRING
#define XB_GETCOUNT         LB_GETCOUNT
#define XB_GETCURSEL        LB_GETCURSEL
#define XB_SETCURSEL        LB_SETCURSEL
#define XB_RESETCONTENT     LB_RESETCONTENT

#else       // use combobox for mode selection

#define XB_RESETCONTENT     CB_RESETCONTENT
#define XB_ADDSTRING        CB_ADDSTRING
#define XB_DELETESTRING     CB_DELETESTRING
#define XB_GETCOUNT         CB_GETCOUNT
#define XB_GETCURSEL        CB_GETCURSEL
#define XB_SETCURSEL        CB_SETCURSEL
#define XB_RESETCONTENT     CB_RESETCONTENT

#endif


//
// Update the text in the Mode selector. This needs to be done after
// editing a VM's name or after loading the properties.
//

void UpdateMode(HWND hDlg)
{
    unsigned i;
    int j = 0, k = 0;

    SendDlgItemMessage(hDlg, IDC_VMSELECT, XB_RESETCONTENT, 0, 0);

    for (i = 0; i < v.cVM; i++)
        {
        if (v.rgvm[i].fValidVM)
            {
            SendDlgItemMessage(hDlg, IDC_VMSELECT,
             XB_ADDSTRING, 0, (long)v.rgvm[i].szModel);

            if (i == v.iVM)
                j = k;

            k++;
            }
        }

    SendDlgItemMessage(hDlg, IDC_VMSELECT, XB_SETCURSEL, j, 0);
}


/****************************************************************************

    FUNCTION: About(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Display version information from the version section of the
    application resource.

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT CALLBACK About(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    int wmId, wmEvent;

    switch (message)
        {
        case WM_CLOSE:
            EndDialog(hDlg, TRUE);    // Exit the dialog
            DestroyWindow (vi.hWnd);
            break;

        case WM_INITDIALOG:

            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
            //vi.hMenu = GetMenu(hDlg);

#if defined(ATARIST) || defined(SOFTMAC)
#if 0
            // Delete the Debug Mode and Debugger menus
            DeleteMenu(vi.hMenu, IDM_DEBUGGER, MF_BYCOMMAND);
            DeleteMenu(vi.hMenu, IDM_DEBUGMODE, MF_BYCOMMAND);
#endif
#else   // Atari 8-bit only, delete the fancy stuff

            // Delete the Tools menu
            DeleteMenu(vi.hMenu, 3, MF_BYPOSITION);

            // Delete the Advanced menu
            DeleteMenu(vi.hMenu, 2, MF_BYPOSITION);

            // Delete First Time Setup
            DeleteMenu(vi.hMenu, IDM_FIRST, MF_BYCOMMAND);
            DeleteMenu(vi.hMenu, IDM_AUTOCONFIG, MF_BYCOMMAND);

            // Delete Load Settings and Save Settings
            DeleteMenu(vi.hMenu, IDM_LOADINI, MF_BYCOMMAND);
            DeleteMenu(vi.hMenu, IDM_SAVEINI, MF_BYCOMMAND);
#endif

#if 1
            // Not supported right now
            // Delete Load/Save/Hibernate State
            DeleteMenu(vi.hMenu, IDM_LOADSTATE, MF_BYCOMMAND);
            DeleteMenu(vi.hMenu, IDM_SAVESTATE, MF_BYCOMMAND);
            DeleteMenu(vi.hMenu, IDM_HYBERNATE, MF_BYCOMMAND);
            DeleteMenu(vi.hMenu, IDM_HYBONEXIT, MF_BYCOMMAND);
#endif

            DrawMenuBar(hDlg);
            UpdateMenuCheckmarks();

            SendDlgItemMessage(hDlg, IDC_VMSELECT, XB_RESETCONTENT, 0, 0);

            {
            unsigned i;
            int j = 0, k = 0;

            for (i = 0; i < v.cVM; i++)
                {
                if (v.rgvm[i].fValidVM)
                    {
                    if (v.rgvm[i].szModel[0] == 0)
                        strcpy (v.rgvm[i].szModel, v.rgvm[i].pvmi->pchModel);

                    SendDlgItemMessage(hDlg, IDC_VMSELECT,
                     XB_ADDSTRING, 0, (long)v.rgvm[i].szModel);

                    if (i == v.iVM)
                        j = k;

                    k++;
                    }
                }

            SendDlgItemMessage(hDlg, IDC_VMSELECT, XB_SETCURSEL, j, 0);

            SetWindowText(hDlg, vi.szAppName);

            SetForegroundWindow(hDlg); // bring dialog to focus
            }

            return (TRUE);

        case WM_COMMAND:              // message: received a command
            wmId    = LOWORD(uParam);
            wmEvent = HIWORD(uParam);

            switch (wmId)
                {
            default:
                if ((ULONG)(wmId - IDM_WEB_EMULATORS) < 8)
                    {
                    // User selected an online help menu item

                    static char const * const rgszWeb[] =
                        {
                        "www.emulators.com",                // 40100
                        "",                                 // update page
                        "",                                 // beta page
                        "www.emulators.com/download.htm",   // download page
                        "www.emulators.com/gemul8r.htm",    // Gemulator page
                        "sourceforge.net/projects/emutos/", // EmuTOS page
                        "",
                        };

                    UINT (__stdcall *pfnSE)();
                    HMODULE hSh32;
                    int ret = 0;

                    if ((hSh32 = LoadLibrary("shell32.dll")) &&
                       (pfnSE = (void *)GetProcAddress(hSh32, "ShellExecuteA")))
                        {
                        char rgch[80];

                        if (strncmp("ftp:", rgszWeb[wmId-IDM_WEB_EMULATORS], 4))
                            strcpy(rgch, "http://");
                        strcat(rgch, rgszWeb[wmId - IDM_WEB_EMULATORS]);

                        ret = (*pfnSE)(NULL,
                                 "open",
                                 rgch,
                                 NULL,
                                 NULL,
                                 SW_SHOWNORMAL);
                        }

                    if (ret < 32)
                        MessageBeep(0xFFFFFFFF);

                    if (hSh32)
                        CloseHandle(hSh32);
                    }

                break;

            case IDC_VMSELECT:
                if ((wmEvent != CBN_SELCHANGE) && (wmEvent != LBN_SELCHANGE))
                    break;

                // fall through

            case IDC_BUTCOPYMODE:
            case IDC_BUTDELMODE:
            case IDC_BUTEDITMODE:
            case IDC_BUTRESTMODE:

                // fall through

            case IDCANCEL:
            case IDOK:
                {
                unsigned i = SendDlgItemMessage(hDlg, IDC_VMSELECT, XB_GETCURSEL, 0, 0);
                unsigned iVM;

                // find the ith valid VM in rgvm

                for (iVM = 0; iVM < v.cVM; iVM++)
                    {
                    if (v.rgvm[iVM].fValidVM && (i-- == 0))
                        break;
                    }

                // has it changed from the current iVM?

                if (iVM != v.iVM)
                    {
                    if (FVerifyMenuOption())
                        FToggleMacAtari(iVM, TRUE);
                    }

                if (wmId == IDC_BUTDELMODE)
                    {
                    ULONG c = SendDlgItemMessage(hDlg, IDC_VMSELECT,
                     XB_GETCOUNT, 0, 0);

                    // Don't delete the first three default VM profiles

                    if ((c > 3) && (iVM >= 3) && FVerifyMenuOption())
                        {
                        i = SendDlgItemMessage(hDlg, IDC_VMSELECT,
                         XB_GETCURSEL, 0, 0);

                        SendDlgItemMessage(hDlg, IDC_VMSELECT,
                         XB_DELETESTRING, i, 0);

                        for (; iVM < v.cVM - 1; iVM++)
                            v.rgvm[iVM] = v.rgvm[iVM+1];

                        memset(&v.rgvm[iVM], 0, sizeof(VM));

                        if (i >= --c)
                            i = c-1;

                        SendDlgItemMessage(hDlg, IDC_VMSELECT,
                         XB_SETCURSEL, i, 0);

                        for (iVM = 0; iVM < v.cVM; iVM++)
                            {
                            if (v.rgvm[iVM].fValidVM && (i-- == 0))
                                break;
                            }

                        FToggleMacAtari(iVM, TRUE);
                        }
                    }

                else if (wmId == IDC_BUTCOPYMODE)
                    {
                    ULONG c = SendDlgItemMessage(hDlg, IDC_VMSELECT,
                     XB_GETCOUNT, 0, 0);

                    if (c < v.cVM)
                        {
                        int i;

                        if (FAddVM(vmCur.pvmi, &i))
                            {
                            v.rgvm[i] = v.rgvm[iVM];
                            v.rgvm[i].szModel[0] = '+';
                            memcpy(&v.rgvm[i].szModel[1],
                                   &v.rgvm[iVM].szModel[0],
                                   sizeof(v.rgvm[iVM].szModel) - 1);

                            v.rgvm[i].szModel[sizeof(v.rgvm[iVM].szModel) - 1]
                                     = 0;

                            SendDlgItemMessage(hDlg, IDC_VMSELECT,
                             XB_ADDSTRING, 0, (long)v.rgvm[i].szModel);

                            SendDlgItemMessage(hDlg, IDC_VMSELECT,
                             XB_SETCURSEL, c, 0);
                            }
                        }
                    }

                else if (wmId == IDC_BUTEDITMODE)
                    {
                    DialogBox(vi.hInst,
                        MAKEINTRESOURCE(IDD_PROPERTIES),
                        hDlg,
                        Properties);
                    }

                else if (wmId == IDC_BUTRESTMODE)
                    {
                    vmCur.fColdReset = TRUE;
                    wmId = IDOK;
                    }
                }

                if ((wmId == IDOK) || (wmId == IDCANCEL))
                    {
                    EndDialog(hDlg, TRUE);    // Exit the dialog
                    CreateNewBitmap();        // restore video mode
                    }

                return (TRUE);

            case IDM_ABOUT:
                {
                char rgch[1120], rgch2[64], rgchVer[32];
                OSVERSIONINFO oi;

                oi.dwOSVersionInfoSize = sizeof(oi);
                GetVersionEx(&oi);  // REVIEW: requires Win32s 1.2+

                switch(oi.dwPlatformId)
                    {
                default:
                case VER_PLATFORM_WIN32_WINDOWS:
                    strcpy(rgchVer,"Windows 95/98/Me");
                    break;

                case VER_PLATFORM_WIN32s:
                    strcpy(rgchVer,"Win32s");
                    break;

                case VER_PLATFORM_WIN32_NT:
                    strcpy(rgchVer,"Windows");

                    if (oi.dwMajorVersion < 5)
                        {
                        strcpy(rgchVer,"Windows NT");
                        }
                    else if (oi.dwMajorVersion == 5)
                        {
                        if (oi.dwMinorVersion == 0)
                            {
                            strcpy(rgchVer,"Windows 2000");
                            }
                        else if (oi.dwMinorVersion == 1)
                            {
                            strcpy(rgchVer,"Windows XP");
                            }
                        else
                            {
                            strcpy(rgchVer,"Windows 2003");
                            }
                        }
                    else if (oi.dwMajorVersion == 6)
                        {
                        if ((oi.dwBuildNumber & 65535) <= 6000)
                            {
                            strcpy(rgchVer,"Windows Vista");
                            }
                        else if (oi.dwMinorVersion == 0)
                            {
                            strcpy(rgchVer,"Windows Vista / Server 2008");
                            }
                        else if (oi.dwMinorVersion == 1)
                            {
                            strcpy(rgchVer,"Windows 7 / Server 2008 R2");
                            }
                        else if (oi.dwMinorVersion == 2)
                            {
                            if ((oi.dwBuildNumber & 65535) < 8400)
                                strcpy(rgchVer,"Windows 8 Beta");
                            else if ((oi.dwBuildNumber & 65535) == 8400)
                                strcpy(rgchVer,"Windows 8 Consumer Preview");
                            else if ((oi.dwBuildNumber & 65535) < 9200)
                                strcpy(rgchVer,"Windows 8 pre-release");
                            else
                                strcpy(rgchVer,"Windows 8");
                            }
                        else
                            {
                            strcpy(rgchVer,"Windows 8 or later");
                            }
                        }
                    else
                        {
                        strcpy(rgchVer,"Windows 8 or later");
                        }
                    break;
                    };

                sprintf(rgch2, "About %s", vi.szAppName);

                sprintf(rgch, "%s Community Release\n"
                    "Darek's Classic Computer Emulator.\n"
                    "Version 9.90 - built on %s\n"
                    "%2d-bit %s release.\n\n"
                    "Copyright (C) 1986-2018 Darek Mihocka.\n"
                    "All Rights Reserved.\n\n"

#ifdef XFORMER
                    "Atari OS and BASIC used with permission.\n"
                    "Copyright (C) 1979-1984 Atari Corp.\n\n"
#endif

                    "Many thanks to: "
                    "Ignac Kolenko, "
                    "Ed Malkiewicz, "
                    "Bill Huey, "
                    "\n"
#if defined(ATARIST) || defined(SOFTMAC)
                    "Christian Bauer, "
                    "Simon Biber, "
                    "\n"
                    "William Condie, "
                    "Phil Cummins, "
                    "Kyle Fox, "
                    "\n"
                    "Manuel Perez, "
                    "Ray Ruvinskiy, "
                    "Joel Walter, "
                    "\n"
                    "Jim Watters, "
#endif
                    "Danny Miller, "
                    "Robert Birmingham, "
                    "and Derek Yenzer.\n\n"

                    "Windows version: %d.%02d (build %d)\n"
                    "Windows platform: %s\n"
                    ,
                    vi.szAppName,
                    __DATE__,
                    sizeof(void *) * 8,
#if defined(_M_AMD64)
                    "x64",
#elif defined(_M_IX86)
                    "x86",
#elif defined(_M_IX86)
                    "ARM",
#else
                    "",
#endif
                    oi.dwMajorVersion, oi.dwMinorVersion,
                    oi.dwBuildNumber & 65535,
                    rgchVer,
                    0);

                MessageBox (GetFocus(),    rgch, rgch2, MB_OK);
                }
                break;

#if defined(ATARIST) || defined(SOFTMAC)
            case IDM_FIRST:
                DialogBox(vi.hInst,       // current instance
                    MAKEINTRESOURCE(IDD_FIRSTTIME), // dlg resource to use
                    hDlg,          // parent handle
                    FirstTimeProc);
                break;

            case IDM_AUTOCONFIG:
                AutoConfigure(TRUE);
                UpdateMode(hDlg);
                UpdateMenuCheckmarks();
                break;
#endif // ATARIST

            case IDM_PROPERTIES:
                DialogBox(vi.hInst,
                    MAKEINTRESOURCE(IDD_PROPERTIES),
                    hDlg,
                    Properties);
                if (!FIsAtari8bit(vmCur.bfHW))
                    AddToPacket(FIsMac(vmCur.bfHW) ? 0xDF : 0xB8);  // Alt  key up
                UpdateMode(hDlg);
                break;

            case IDM_DISKPROPS:
                DialogBox(vi.hInst,       // current instance
                    MAKEINTRESOURCE(IDD_DISKS), // dlg resource to use
                    hDlg,          // parent handle
                    DisksDlg);
                break;

            case IDM_COLORMONO:
                if (!FToggleMonitor())
                    return 0;

                // don't reboot, let the OS detect it as necessary
                return 0;

            case IDM_COLDSTART:
                // have some default values so as to force a default bitmap

                EndDialog(hDlg, TRUE);    // Exit the dialog
                vmCur.fColdReset = TRUE;
                break;

            case IDM_TOGGLEHW:
                if (FToggleMacAtari((unsigned)(-1), TRUE))
                    {
                    EndDialog(hDlg, TRUE);    // Exit the dialog
                    }

                return 0;
                break;

            case IDM_DIRECTX:
                v.fFullScreen = !v.fFullScreen;
                CreateNewBitmap();
                UpdateMenuCheckmarks();
                break;

            case IDM_ZOOM:
                v.fZoomColor = !v.fZoomColor;
				v.fTiling = FALSE;
            //  CreateNewBitmap();
                UpdateMenuCheckmarks();
                break;

            case IDM_SAVEONEXIT:
                v.fSaveOnExit = !v.fSaveOnExit;
                UpdateMenuCheckmarks();
                break;

            case IDM_HYBONEXIT:
                v.fHibrOnExit = !v.fHibrOnExit;
                UpdateMenuCheckmarks();
                break;

			// !!! not in UI
            case IDM_SKIPSTARTUP:
                v.fSkipStartup = !v.fSkipStartup;
                UpdateMenuCheckmarks();
                break;

            case IDM_LOADINI:
                FUnInitVM();
                LoadProperties(hDlg);
                vpvm = vmCur.pvmi;
            //  v.fFullScreen = FALSE; // force full screen to be off
                vmCur.fColdReset = TRUE;
                UpdateMode(hDlg);
                return 0;
                break;

            case IDM_SAVEINI:
                SaveProperties(hDlg);
                break;

#if 0
            case IDM_LOAD:
                LoadProperties(NULL);
                break;
#endif

            case IDM_SAVE:
                SaveProperties(NULL);
                break;

            case IDM_DXNORMAL:
                v.fNoDDraw = 0;
                CreateNewBitmap();
                UpdateMenuCheckmarks();
                break;

            case IDM_DXDELAY:
                v.fNoDDraw = 1;
                CreateNewBitmap();
                UpdateMenuCheckmarks();
                break;

            case IDM_DXDISABLE:
                v.fNoDDraw = 2;
                CreateNewBitmap();
                UpdateMenuCheckmarks();
                break;

            case IDM_HYBERNATE:
                if (!FIsAtari8bit(vmCur.bfHW))
                    v.fSkipStartup |= 2;

            case IDM_SAVESTATE:
                if (!FIsAtari8bit(vmCur.bfHW))
                if (IDOK == MessageBox (GetFocus(),
                            "Overwrite previous saved state?",
                            vi.szAppName, MB_OKCANCEL))
                    {
                    SetWindowText(vi.hWnd, "Hibernating...");

                    if (SaveState(TRUE))
                        {
                        if (wmId == IDM_HYBERNATE)
                            PostQuitMessage(0);
                        }

                    // if we got here during hibernate it means it failed

                    else if (wmId == IDM_HYBERNATE)
                        {
                        v.fSkipStartup &= ~2;
                        SaveProperties(NULL);
                        MessageBeep(0xFFFFFFFF);
                        }
                    }

                v.fSkipStartup &= ~2;

                DisplayStatus();
                return 0;
                break;

            case IDM_LOADSTATE:
                SaveState(FALSE);
                UpdateMode(hDlg);
                return 0;
                break;

            case IDM_ZAPCMOS:
                if (FIsMac(vmCur.bfHW))
                if (IDOK == MessageBox (GetFocus(),
                    "Clearing the PRAM will reset some general Control Panels "
                    "settings such as Disk Cache size and the Startup Disk.",
                    vi.szAppName, MB_OKCANCEL))
                    {
                    memset(&vmachw.rgbCMOS, 0, sizeof(vmachw.rgbCMOS));
                    }
                break;

            case IDM_DEBUGGER:
                EndDialog(hDlg, TRUE);    // Exit the dialog
                ShowWindowsMouse();
                vi.fDebugBreak = TRUE;
                break;

            case IDM_DEBUGMODE:
                v.fDebugMode = !v.fDebugMode;
                UpdateMenuCheckmarks();

                if (v.fDebugMode)
                    {
                    CreateDebuggerWindow();
                    }
                else
                    {
                    vi.fInDebugger = 0;
                    DestroyDebuggerWindow();
                    }

                break;

            case IDM_NOSCSI:
                v.fNoSCSI = !v.fNoSCSI;
                UpdateMenuCheckmarks();
                break;

            case IDM_NOJIT:
                v.fNoJit = !v.fNoJit;
                UpdateMenuCheckmarks();
                break;

            case IDM_NOQBOOT:
                v.fNoQckBoot = !v.fNoQckBoot;
                UpdateMenuCheckmarks();
                break;

            case IDM_NOMONO:
                v.fNoMono = !v.fNoMono;
                CreateNewBitmap();
                UpdateMenuCheckmarks();
                break;

            case IDM_NOTWOBUTTON:
                v.fNoTwoBut = !v.fNoTwoBut;
                UpdateMenuCheckmarks();
                break;

            case IDM_FMTMACFLOP:
                if (IDOK == MessageBox (GetFocus(),
                    "Insert a formatted floppy disk into your PC's A: drive.\n"
                    "The disk will be quick formatted in Macintosh format.",
                    vi.szAppName, MB_OKCANCEL))
                    {
                    PDI pdi;
                    BYTE *pb = VirtualAlloc(NULL, 1440*1024, MEM_COMMIT, PAGE_READWRITE);
                    BOOL f = fFalse;
                    HCURSOR h = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    FUnInitDisksVM();

                    pdi = PdiOpenDisk(DISK_FLOPPY, 0, DI_QUERY);

                    if (pb && pdi)
                        {
                        CreateHFSBootBlocks(pb, 1440, "Untitled");

                        pdi->sec = 0;
                        pdi->count = 16*(1024/512); // 16K is all that's needed
                        pdi->lpBuf = pb;

                        f = FRawDiskRWPdi(pdi, 1);

                        CloseDiskPdi(pdi);
                        }

                    if (pb)
                        VirtualFree(pb, 0, MEM_RELEASE);

                    FInitDisksVM();

                    SetCursor(h);

                    MessageBox (GetFocus(),
                        f ? "Floppy disk formatted successfully!" :
                            "Floppy disk format failed!",
                        vi.szAppName, MB_OK);
                    }
                break;

            case IDM_FLOP2IMG:
                if (IDOK == MessageBox (GetFocus(),
                    "Insert a floppy disk into your PC's A: drive.\n"
                    "The disk will be read and saved as an image.",
                    vi.szAppName, MB_OKCANCEL))
                    {
                    PDI pdi;
                    BYTE *pb = VirtualAlloc(NULL, 1440*1024, MEM_COMMIT, PAGE_READWRITE);
                    BOOL f = fFalse;
                    HCURSOR h = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    BYTE sz[MAX_PATH];

                    FUnInitDisksVM();

                    pdi = PdiOpenDisk(DISK_FLOPPY, 0, DI_QUERY);

                    if (pb && pdi)
                        {
                        pdi->sec = 0;
                        pdi->count = 2880;
                        pdi->lpBuf = pb;

                        f = FRawDiskRWPdi(pdi, 0);

                        CloseDiskPdi(pdi);
                        }

                    if (pb && f && (f = OpenTheFile(hDlg, &sz, fTrue, 0)))
                        {
                        ULONG cbRead;
                        HANDLE hf;

                        hf = CreateFile(sz, GENERIC_READ | GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

                        f = WriteFile(hf, pb, 1440*1024, &cbRead, NULL);

                        CloseHandle(hf);
                        }

                    if (pb)
                        VirtualFree(pb, 0, MEM_RELEASE);

                    FInitDisksVM();

                    SetCursor(h);

                    MessageBox (GetFocus(),
                        f ? "Disk image successfully created!" :
                            "Disk image was not created!",
                        vi.szAppName, MB_OK);
                    }
                break;

            case IDM_IMG2FLOP:
                if (IDOK == MessageBox (GetFocus(),
                    "Insert a blank floppy disk into your PC's A: drive.\n"
                    "Now select a disk image file to copy to the floppy.",
                    vi.szAppName, MB_OKCANCEL))
                    {
                    PDI pdi;
                    BYTE *pb = VirtualAlloc(NULL, 1440*1024, MEM_COMMIT, PAGE_READWRITE);
                    BOOL f = fFalse;
                    HCURSOR h;
                    BYTE sz[MAX_PATH];

                    FUnInitDisksVM();

                    pdi = PdiOpenDisk(DISK_FLOPPY, 0, DI_QUERY);

                    if (pb && (f = OpenTheFile(hDlg, &sz, fFalse, 0)))
                        {
                        PDI pdi2;

                        pdi2 = PdiOpenDisk(DISK_IMAGE, sz, DI_READONLY);

                        if (pdi2)
                            {
                            pdi2->sec = 0;
                            pdi2->count = 2880;
                            pdi2->lpBuf = pb;

                            f = FRawDiskRWPdi(pdi2, 0);

                            CloseDiskPdi(pdi2);
                            }
                        }

                    h = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    if (pb && f && pdi)
                        {
                        pdi->sec = 0;
                        pdi->count = 2880;
                        pdi->lpBuf = pb;

                        f = FRawDiskRWPdi(pdi, 1);

                        CloseDiskPdi(pdi);
                        }

                    if (pb)
                        VirtualFree(pb, 0, MEM_RELEASE);

                    FInitDisksVM();

                    SetCursor(h);

                    MessageBox (GetFocus(),
                        f ? "Disk image successfully copied to floppy!" :
                            "Disk image was not copied to floppy!",
                        vi.szAppName, MB_OK);
                    }
                break;

            case IDM_NEWHFX:
            case IDM_NEWDSK:
                if (IDOK == MessageBox (GetFocus(),
                    "You are about to create a large disk image file that "
                    "will use up to 2 gigabytes of available disk space on "
                    "the selected partition.",
                    vi.szAppName, MB_OKCANCEL))
                    {
                    BYTE *pb = VirtualAlloc(NULL, 1440*1024, MEM_COMMIT, PAGE_READWRITE);
                    ULONG ckb = 0;
                    BOOL f = 0;
                    BYTE sz[MAX_PATH];
                    HCURSOR h;
                    int i;

                    FUnInitDisksVM();

                    i = DialogBoxParam(vi.hInst,       // current instance
                        MAKEINTRESOURCE(IDD_CHOOSE), // dlg resource to use
                        NULL,          // parent handle
                        ChooseProc,
                        (wmId == IDM_NEWDSK));         // do a disk size drop down

                    h = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    sz[0] = 0;

                    if (pb && (i > 0) && (f = OpenTheFile(hDlg, &sz, fTrue, 0)))
                        {
                        ULONG cbRead;
                        HANDLE hf;

                        hf = CreateFile(sz, GENERIC_READ | GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

                        while (f && (ckb < (i * 1024)))
                            {
                            f = WriteFile(hf, pb, 32*1024, &cbRead, NULL);

                            if (f)
                                {
                                ckb += cbRead/1024;

                                if (ckb > 2000)
                                    {
                                    ckb = SetFilePointer(hf,
                                             i*1024, NULL, FILE_BEGIN) / 1024;

                                    SetEndOfFile(hf);
                                    break;
                                    }
                                }
                            }

                        if (ckb >= 1440)
                            {
                            if (wmId == IDM_NEWHFX)
                                {
                                CreateHFSBootBlocks(pb, ckb, "Mac Disk");

                                SetFilePointer(hf, 0, NULL, FILE_BEGIN);

                                WriteFile(hf, pb, 1440*1024, &cbRead, NULL);
                                }
                            }
                        else
                            ckb = 0;

                        CloseHandle(hf);
                        }

                    if (pb)
                        VirtualFree(pb, 0, MEM_RELEASE);

                    FInitDisksVM();

                    SetCursor(h);

                    MessageBox (GetFocus(),
                        ckb ?
                            "Disk image created successfully! "
                            "Use Disk Properties "
                            "to map the disk image to a disk drive. "
                          :
                            "Disk image creation failed!",
                        vi.szAppName, MB_OK);
                    }
                break;

            case IDM_DEBUGINFO:
                {
                char rgch[512];

                sprintf(rgch, "fWin32s:%d  fWinNT:%d  hVideo:%08X\n"
                    "VDH:\n%08X %08X\n%08X %08X\n"
                    "wave device name:%s\nchannels:%d formats:%08X\n"
                    "High speed timer = %d kHz\n"
                    "%d %d %d %d %d %d\n"
                    "%d %d %d %d %d %d\n"
                    "ROM sig: %08X\n"
                    ,
                    /* vi.fWin32s */ 0, vi.fWinNT, vi.hVideoThread,
                    vi.rgpdi[0],vi.rgpdi[1],vi.rgpdi[2],vi.rgpdi[3],
                    vi.woc.szPname,    vi.woc.wChannels, vi.woc.dwFormats,
                    ((CheckClockSpeed()) + 500) / 1000,
                    vi.rgbShiftMs[0], vi.rgbShiftMs[1], vi.rgbShiftMs[2],
                    vi.rgbShiftMs[3], vi.rgbShiftMs[4], vi.rgbShiftMs[5],
                    vi.rgbShiftE[0], vi.rgbShiftE[1], vi.rgbShiftE[2],
                    vi.rgbShiftE[3], vi.rgbShiftE[4], vi.rgbShiftE[5],
                    vi.fExecuting ? vmPeekL(vi.eaROM[0]) : 0
                    );

                MessageBox (GetFocus(),    rgch, "Info", MB_OK);
                }
                break;

#if 0
            case IDM_SNAPSHOT: // write out a memory dump
                MemorySnapShot();
                break;
#endif

            case IDM_EXIT:
#if 0
                if (IDOK != MessageBox (GetFocus(),
    #if _ENGLISH
                        "Any unsaved documents will be lost.\n"
                        "Are you sure you want to quit?",
    #elif _NEDERLANDS
                         "Alle niet bewaarde Atari-files zullen verloren gaan.\n"
                         "Weet u zeker dat u wilt stoppen?\n",
    #elif _DEUTSCH
                        "Alle ungesicherten Atari Daten gehen verloren.\n"
                        "Wirklich beenden?\n",
    #elif _FRANCAIS
                        "Tous les fichiers Atari non sauvegardés seront perdus.\n"
                        "Etes vous certain de vouloir quitter?",
    #elif
    #error
    #endif
                        vi.szAppName, MB_OKCANCEL))
                    {
                    ShowWindowsMouse();
                    return 0;
                    }
#endif

                EndDialog(hDlg, TRUE);    // Exit the dialog
                DestroyWindow (vi.hWnd);
                break;
                }

            break;
        }
    return (FALSE); // Didn't process the message

    lParam; // This will prevent 'unused formal parameter' warnings
}

//
//   FUNCTION: OpenTheFile(HWND hwnd, HWND hwndEdit)
//
//   PURPOSE: Invokes common dialog function to open a file and opens it.
//
//   COMMENTS:
//
//    This function initializes the OPENFILENAME structure and calls
//            the GetOpenFileName() common dialog function.
//
//    RETURN VALUES:
//        TRUE - The file was opened successfully and read into the buffer.
//        FALSE - No files were opened.
//
//
// nType == 0 for the ordinary disk image type you want to load
// nType == 1 for a special type of media for your VM, eg. a cartridge for Atari800
BOOL OpenTheFile(HWND hWnd, char *psz, BOOL fCreate, int nType)
{
    OPENFILENAME OpenFileName;

    // Fill in the OPENFILENAME structure to support a template and hook.
    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = NULL;
    if (FIsMac(vmCur.bfHW))
        OpenFileName.lpstrFilter   =
             "Macintosh Disk Images\0*.dsk;*.ima*;*.img;*.hf*;*.hqx;*.cd\0All Files\0*.*\0\0";
	else if (FIsAtari8bit(vmCur.bfHW))
	{
		if (nType == 0)
			OpenFileName.lpstrFilter = "Xformer/SIO2PC Disks\0*.xfd;*.atr;*.sd;*.dd\0All Files\0*.*\0\0";
		else
			OpenFileName.lpstrFilter = "Xformer Cartridge\0*.bin;*.rom;*.car\0All Files\0*.*\0\0";
	}
    else
        OpenFileName.lpstrFilter   =
             "Atari ST Disk Images\0*.vhd;*.st;*.dsk;*.msa\0All Files\0*.*\0\0";
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = psz;
    OpenFileName.nMaxFile          = MAX_PATH;
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = fCreate ? "Create a Disk Image" : "Select a Disk Image";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = 0;
    OpenFileName.lpfnHook            = NULL;
    OpenFileName.lpTemplateName    = NULL;
    OpenFileName.Flags             = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |
			(fCreate ? 0 : OFN_FILEMUSTEXIST);

    // Call the common dialog function.
    if (GetOpenFileName(&OpenFileName))
        {
        strcpy(psz, OpenFileName.lpstrFile);
        return TRUE;
        }

    return FALSE;
}

#ifndef BROWSEINFO

typedef struct _browseinfo {
    HWND hwndOwner;
    void * /* LPCITEMIDLIST */ pidlRoot;
    LPSTR pszDisplayName;
    LPCSTR lpszTitle;
    UINT ulFlags;
    /* BFFCALLBACK */ void * lpfn;
    LPARAM lParam;
    int iImage;
} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;

void * __stdcall SHGetPathFromIDList(void *, void *);
void * __stdcall SHBrowseForFolder(void *);

#endif

#ifndef BIF_RETURNONLYFSDIRS
#define BIF_RETURNONLYFSDIRS 1
#endif

UINT (__stdcall *pfnCoInitialize)(void *);

BOOL OpenThePath(HWND hWnd, char *psz)
{
    BROWSEINFO OpenFilePath;
    void *pidl; // LPCITEMDLIST pidl;

    HMODULE hOle32;

    if(pfnCoInitialize == NULL)
        {
        if((hOle32=LoadLibrary("ole32.dll"))==NULL ||
            (pfnCoInitialize=(void *)GetProcAddress(hOle32, "CoInitialize"))==NULL)
            return FALSE;

        (*pfnCoInitialize)(NULL);
        }

    // CoInitialize(NULL);

    memset(&OpenFilePath, 0, sizeof(OpenFilePath));

    // Fill in the BROWSEINFO structure
    OpenFilePath.hwndOwner         = hWnd;
    OpenFilePath.pszDisplayName    = psz;
//    OpenFilePath.pidlRoot          = NULL;
    OpenFilePath.lpszTitle         = "Select a path for ROM image files";
//    OpenFilePath.lpfn              = NULL;
//    OpenFilePath.lParam              = 0;
    OpenFilePath.ulFlags           = BIF_RETURNONLYFSDIRS;

    // Call the shell function.

    if (pidl = SHBrowseForFolder(&OpenFilePath))
        {
        if (SHGetPathFromIDList(pidl, psz))
            return TRUE;
        }

    return FALSE;
}


char const * const rgszMon[] =
{
    "Unknown Monitor",
    "Monochrome",
    "Color",
    "Black & White TV",
    "Color TV",
#if defined(ATARIST) || defined(SOFTMAC)
    "Atari ST Monochrome",
    "Atari ST Color",
    "",
    "512x342",
    "640x480",
    "800x512",
    "800x600",
    "832x624",
    "864x600",
    "1024x480",
    "1024x600",
    "1024x768",
    "1152x864",
    "1280x960",
    "1280x1024",
    "1600x1024",
    "1600x1200",
    "1600x1280",
    "1920x1080",
    "1920x1200",
    "1920x1440",
    "Current desktop resolution",
#endif
};

char const * const rgszRAM[] =
{
    "Default",
    "8K",
    "16K",
    "24K",
    "32K",
    "40K",
    "48K",
    "64K",
    "128K",
    "256K",
    "320K",
    "512K",
    "1M",
    "2M",
    "2.5M",
    "4M",
    "8M",
    "10M",
    "12M",
    "14M",
    "16M",
    "20M",
    "24M",
    "32M",
    "48M",
    "64M",
    "128M",
    "192M",
    "256M",
    "384M",
    "512M",
    "768M",
    "1024M",
};

char const * const rgszROM[] =
{
    "Unknown OS",
    "Atari 800 OS",
    "Atari 800XL OS",
    "Atari 130XE OS",
    "",
#if defined(ATARIST) || defined(SOFTMAC)
    "2-chip TOS 1.x",
    "6-chip TOS 1.x",
    "2-chip TOS 2.x",
    "4-chip TOS 3.x",
    "1-chip TOS 4.x",
    "TOS 1.x boot disk",
    "Magic 2.x",
    "Magic 4.x",
    "MagicPC",
    "",
    "",
    "",
    "64K Mac ROMs",
    "128K Mac ROMs",
    "256K Mac ROMs",
    "512K Mac ROMs",
    "1M Mac ROMs",
    "",
    "",
#endif // ATARIST
};

char const * const rgszVM[] =
{
    "",
    "Atari 800",
    "Atari 800XL",
    "Atari 130XE",
#if defined(ATARIST) || defined(SOFTMAC)
    "Simulator",
    "Atari ST",
    "Atari STE",
    "Atari TT",
    "Atari 800 with BASIC",
    "Atari 800XL with BASIC",
    "Macintosh Plus",
    "Macintosh SE",
    "Atari 130XE with BASIC",
    "Macintosh II",
#ifdef POWERMAC
    "Power Mac 6100",
#else
    "",
#endif
    "Macintosh IIcx",
#ifdef POWERMAC
    "Power Mac 7100",
#else
    "",
#endif
    "Macintosh IIci",
    "Macintosh IIsi",
    "Macintosh LC",
    "Macintosh Quadra",
    "Macintosh Quadra 605",
    "Macintosh Quadra 700",
    "Macintosh Quadra 610",
    "Macintosh Quadra 650",
    "Macintosh Quadra 900",
#ifdef POWERMAC
    "Power Mac 900",
    "Power Mac 700",
    "Power Mac 610",
    "Power Mac 650",
    "Power Mac 6400",
    "Power Mac iMac",
    "Power Mac G4",
#endif // POWERMAC
#endif // ATARIST
};

char const * const rgszCPU[] =
{
    "Unknown",
    "6502",
#if defined(ATARIST) || defined(SOFTMAC)
    "68000",
    "68010",
    "68020",
    "68030",
    "68040",
#ifdef POWERMAC
    "PowerPC 601",
    "PowerPC 603",
    "PowerPC 604",
    "PowerPC G3",
    "PowerPC G4",
#else
    "",
    "",
    "",
    "",
    "",
#endif // POWERMAC
    "",
#endif // ATARIST
};


char *PchFromBfRgSz(ULONG bf, char const * const *rgsz)
{
    char **ppch = (char **)rgsz;

    while (bf)
        {
        bf >>= 1;
        ppch++;
        }

    return *ppch;
}


BOOL FMonoFromBf(ULONG bf)
{
    return (vmCur.bfMon == monMono)
         || (vmCur.bfMon == monSTMono)
        ;
}


ULONG CbRamFromBf(ULONG bf)
{
    static ULONG const rgmpbfcb[] =
        {
        0, 8, 16, 32, 24, 40, 48, 64, 128, 256, 320, 512, 1024, 2048, 2560,
        4096, 8192, 10240, 12288, 14336, 16384, 20480, 24576, 32768,
        49152, 65536, 128*1024, 192*1024, 256*1024,
        384*1024, 512*1024, 768*1024, 1024*1024,
        };
    int i = 0;

    while (bf)
        {
        bf >>= 1;
        i++;
        }

    return 1024*rgmpbfcb[i];
}


//
// Return the ith set bit in the bit vector wf
//

ULONG BfFromWfI(ULONG wf, int i)
{
    ULONG bf = 1;

    while (bf)
        {
        // skip any zero bits

        if ((wf & bf) == 0)
            {
            bf <<= 1;
            continue;
            }

        // are we on the ith bit?

        if (i-- == 0)
            break;

        // no, so punt this one too
        bf <<= 1;
        }

    return bf;
}

//
// Add a byte to the IKBD input buffer
//

void AddToPacket(ULONG b)
{
    vvmi.rgbKeybuf[vvmi.keyhead++] = (BYTE)b;
    vvmi.keyhead &= 1023;

#if defined(ATARIST) || defined(SOFTMAC)
    vsthw.gpip &= 0xEF; // to indicate characters available
#endif // ATARIST
}


void MarkAllPagesDirty()
{
    vi.fRefreshScreen = 1;
    vi.fVideoWrite = 1;
}

void MarkAllPagesClean()
{
    memset(vi.rgbwrite, 0, sizeof(vi.rgbwrite));
    vi.fRefreshScreen = 0;
    vi.fVideoWrite = 0;
}

//
// CbReadWholeFile(filename, count, buffer);
//
// Instead of duplicating fopen/fread/fclose code all over the place, this
// just blasts in a file into memory and returns number of bytes read.
//

int CbReadWholeFile(char *sz, int cb, void *buf)
{
    HANDLE h;
    char szDir[MAX_PATH];
    int cbRead = 0;

    GetCurrentDirectory(MAX_PATH, (char *)szDir);

#if !defined(NDEBUG)
    printf("CbReadWholeFile: dir = '%s', file = '%s'\n", szDir, sz);
#endif

    if ((sz[1] == ':') || (sz[0] == '\\') || (sz[1] == '\\'))
        strcpy(szDir,sz);
    else
        {
        strcat(szDir,"\\");
        strcat(szDir,sz);
        }

#if !defined(NDEBUG)
    printf("actual filename: '%s'\n", szDir);
#endif

    h = CreateFile(szDir, GENERIC_READ, FILE_SHARE_READ,
          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h != INVALID_HANDLE_VALUE)
        {
        ReadFile(h, buf, cb, &cbRead, NULL);
        CloseHandle(h);
        }

    return cbRead;
}


#if defined(ATARIST) || defined(SOFTMAC)

//
// CbReadWholeFileToGuest(filename, count, addr);
//
// Read a file directly into guest physical address space.
// Returns number of bytes read.
//

int CbReadWholeFileToGuest(char *sz, int cb, ULONG addr, ULONG access_mode)
{
    void *pv;

    if (cb >= vi.cbRAM[0])
        return 0;

    pv = malloc(cb);

    if (NULL == pv)
        return 0;

    cb = CbReadWholeFile(sz, cb, pv);

    if (cb)
        {
        if (!HostToGuestDataCopy(addr, pv, access_mode, cb))
            cb = 0;
        }

    free(pv);

    return cb;
}

#endif  // ATARIST

DWORD FReadChunkFile(char *sz, int cb, void *buf, DWORD offset)
{
    HANDLE h;
    char szDir[MAX_PATH];

    GetCurrentDirectory(MAX_PATH, (char *)szDir);

#if !defined(NDEBUG)
    printf("FReadChunkFile: dir = '%s', file = '%s'\n", szDir, sz);
#endif

    if ((sz[1] == ':') || (sz[0] == '\\') || (sz[1] == '\\'))
        strcpy(szDir,sz);
    else
        {
        strcat(szDir,"\\");
        strcat(szDir,sz);
        }

#if !defined(NDEBUG)
    printf("actual filename: '%s'\n", szDir);
#endif

    h = CreateFile(szDir, GENERIC_READ, FILE_SHARE_READ,
          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h != INVALID_HANDLE_VALUE)
        {
        int cbRead;

        SetFilePointer(h, offset, NULL, FILE_BEGIN);
        ReadFile(h, buf, cb, &cbRead, NULL);
        CloseHandle(h);

        return (cb == cbRead) ? (offset + cb) : 0;
        }

    return 0;
}

BOOL FWriteWholeFile(char *sz, int cb, void *buf)
{
    HANDLE h;
    char szDir[MAX_PATH];

    GetCurrentDirectory(MAX_PATH, (char *)szDir);

#if !defined(NDEBUG)
    printf("FWriteWholeFile: dir = '%s', file = '%s'\n", szDir, sz);
#endif

    if ((sz[1] == ':') || (sz[0] == '\\') || (sz[1] == '\\'))
        strcpy(szDir,sz);
    else
        {
        strcat(szDir,"\\");
        strcat(szDir,sz);
        }

#if !defined(NDEBUG)
    printf("actual filename: '%s'\n", szDir);
#endif

    h = CreateFile(szDir, GENERIC_READ | GENERIC_WRITE, 0,
          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h != INVALID_HANDLE_VALUE)
        {
        int cbWrite;

        WriteFile(h, buf, cb, &cbWrite, NULL);
        CloseHandle(h);

        return cb == cbWrite;
        }

    return FALSE;
}


BOOL FAppendWholeFile(char *sz, int cb, void *buf)
{
    HANDLE h;
    char szDir[MAX_PATH];

    GetCurrentDirectory(MAX_PATH, (char *)szDir);

#if !defined(NDEBUG)
    printf("FAppendWholeFile: dir = '%s', file = '%s'\n", szDir, sz);
#endif

    if ((sz[1] == ':') || (sz[0] == '\\') || (sz[1] == '\\'))
        strcpy(szDir,sz);
    else
        {
        strcat(szDir,"\\");
        strcat(szDir,sz);
        }

#if !defined(NDEBUG)
    printf("actual filename: '%s'\n", szDir);
#endif

    h = CreateFile(szDir, GENERIC_READ | GENERIC_WRITE, 0,
          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h != INVALID_HANDLE_VALUE)
        {
        int cbWrite;

        SetFilePointer(h, 0, NULL, FILE_END);
        WriteFile(h, buf, cb, &cbWrite, NULL);
        CloseHandle(h);

        return cb == cbWrite;
        }

    return FALSE;
}


#if defined(ATARIST) || defined(SOFTMAC)

//
// ScreenRefresh
//
// Walk each scan line and check if it needs to be redrawn.
// If so, blit from 68000 space to bitmap buffer and then
// and the end send a message to redraw the bounding range of scan lines
//

void __cdecl Blit4Mac(BYTE *pbSrc, BYTE *pbDst, ULONG cbScanSrc, ULONG cbScanDst)
{
    int i = cbScanSrc;
    BYTE *pb = pbDst;

    while (i--)
        {
        BYTE b = *pbSrc--;

        *pb++ = ((b >> 6) & 3) + 0x0A;
        *pb++ = ((b >> 4) & 3) + 0x0A;
        *pb++ = ((b >> 2) & 3) + 0x0A;
        *pb++ = ((b     ) & 3) + 0x0A;
        }
}

void __cdecl Blit16Mac(BYTE *pbSrc, BYTE *pbDst, ULONG cbScanSrc, ULONG cbScanDst)
{
    int i = cbScanSrc;
    BYTE *pb = pbDst;

    while (i--)
        {
        pbSrc--;
        *pb++ = ((*pbSrc >> 4) & 15) + 0x0A;
        *pb++ = ((*pbSrc     ) & 15) + 0x0A;
        }
}

void ScreenRefresh(void)
{
    unsigned scan;
    int  first, last;
    BYTE *pbDst;
    ULONG cbScanSrc = 1, cbScanDst = 1;
    ULONG cbSkip;
    ULONG eaScanLine = vsthw.dbase;
    BYTE *pbSurface = NULL;
    BOOL fRefresh;

    BOOL fMac;

    extern void __cdecl BlitMono(void *, void *, ULONG, ULONG);
    extern void __cdecl BlitMonoInv(void *, void *, ULONG, ULONG);
    extern void __cdecl BlitMonoX(void *, void *, ULONG, ULONG);
    extern void __cdecl BlitMonoXInv(void *, void *, ULONG, ULONG);
    extern void __cdecl Blit4(void *, void *, ULONG, ULONG);
    extern void __cdecl Blit4Mac(BYTE *, BYTE *, ULONG, ULONG);
    extern void __cdecl Blit4X(void *, void *, ULONG, ULONG);
    extern void __cdecl Blit16(void *, void *, ULONG, ULONG);
    extern void __cdecl Blit16Mac(BYTE *, BYTE *, ULONG, ULONG);
    extern void __cdecl Blit16X(void *, void *, ULONG, ULONG);

    typedef void (__cdecl *PFNBLIT) (void *, void *, ULONG, ULONG);

    PFNBLIT pfnBlit;

    if (vvmi.pvBits == NULL)
        return;

    if (!vi.fRefreshScreen && !vi.fVideoWrite)
        return;

    // Interlock to make sure video memory is there

    if (TRUE != InterlockedCompareExchange(&vi.fVideoEnabled, FALSE, TRUE))
        return;

    if (vi.fVideoEnabled != FALSE)
        {
        __asm int 3;
        }

    vsthw.dbase += 0;

    if (vsthw.dbase > (vi.eaRAM[1] + vi.cbRAM[1] - 65536))
        {
        vsthw.dbase = vi.eaRAM[1];
        }

#if !defined(NDEBUG)
//  printf("eaRAM[1] = %08X cbRAM[1] = %08X setting dbase = %08X\n", vi.eaRAM[1], vi.cbRAM[1], vsthw.dbase);
#endif

    // display some debug information about screen refreshes and page faults

    if (0)
        {
        static ULONG l, l0;
        char rgch[256];

        sprintf(rgch,"top:%d bot:%d R:%d W:%d V:%d ticks:%d",
             first, last, vi.countR, vi.countW, vi.countVidW, (l = GetTickCount()) - l0);
        SetWindowText(vi.hWnd, rgch);

        l0 = l;
        }

// PrintScreenStats(); printf("Refresh: entered critical section\n");

    fMac = FIsMac(vmCur.bfHW);

    if (!fMac && !vsthw.fMono && vi.fRefreshScreen)
        {
        // color registers have possibly changed

        int i;

        for (i = 0; i < 16; i++)
            {
            ULONG w = vsthw.wcolor[i];
            ULONG r, g, b;

            if (FIsAtariEhn(vmCur.bfHW))
                {
                w = ((w & 0x0777) << 1) | ((w & 0x0888) >> 3);
                }
            else
                {
                w = w & 0x0777;
                }

            r = (w & 0x0F00) >> 8;
            g = (w & 0x00F0) >> 4;
            b = (w & 0x000F) >> 0;

            if (FIsAtariEhn(vmCur.bfHW))
                {
                // convert 0..15 levels to 0..255 levels by x17

                r *= 17;
                g *= 17;
                b *= 17;
                }
            else
                {
                // convert 0..7 levels to 0.255 by x36
                r = r*36 + (r>>1);
                g = g*36 + (g>>1);
                b = b*36 + (b>>1);
                }

            DebugStr("setting rgrgb[%d] = %d, %d, %d\n", i, r, g, b);

            vsthw.rgrgb[i].rgbRed = (BYTE)r;
            vsthw.rgrgb[i].rgbGreen = (BYTE)g;
            vsthw.rgrgb[i].rgbBlue =(BYTE) b;
            vsthw.rgrgb[i].rgbReserved = 0;
            }

        SetBitmapColors();
        FCreateOurPalette();
        }

    // in case the CPU thread is writing to pages that are already marked
    // we won't get a page fault to tell us. So before any screen refresh
    // pretend like we already refreshed the screen, so that any screen
    // writes that happen will be caught the next time we get called.

    fRefresh = TRUE;
    vi.fRefreshScreen = 0;

    // mark all the visible video pages

    cbScanSrc = (vsthw.xpix/8) * vsthw.planes;
    cbSkip = (FIsAtariEhn(vmCur.bfHW) ? vsthw.scanskip*2 : 0);

    if (eaScanLine == 0)
        goto Ldone;

  // HACK!!

    vi.fVideoWrite = 1;

    first = 9999;    // first scan line of bounding rectangle
    last = -1;        // last scan line of bounding rectangle

    if (vi.fInDirectXMode && vi.fHaveFocus)
        {
        pbSurface = LockSurface(&cbScanDst);
        }

    if (pbSurface)
        {
        pbDst = pbSurface;

        // center the screen

        if (vsthw.xpix == 320)
            pbDst += (((vi.sx/2-vsthw.xpix)/2)) + cbScanDst * ((vi.sy/2-vsthw.ypix)/2);
        else if (vsthw.ypix == 200)
            pbDst += (vi.dx = ((vi.sx-vsthw.xpix)/2)) + cbScanDst * ((vi.sy/2-vsthw.ypix)/2);
        else
            pbDst += vi.dx + cbScanDst * vi.dy;
        }
    else
        {
        pbDst = vvmi.pvBits;
        cbScanDst = vi.cbScan;
        }

    cbScanSrc = vsthw.xpix/8 * vsthw.planes;

    if (vsthw.fMono && (pbSurface || v.fNoMono))
        {
        Assert(vsthw.planes == 1);

        if (fMac || vsthw.wcolor[0] & 1)
            {
            pfnBlit = BlitMonoX;
            }
        else
            {
            pfnBlit = BlitMonoXInv;
            }
        }
    else if (vsthw.fMono)
        {
        Assert(vsthw.planes == 1);

        if (fMac || vsthw.wcolor[0] & 1)
            {
            pfnBlit = BlitMono;
            }
        else
            {
            pfnBlit = BlitMonoInv;
            }
        }
    else if (fMac && (vsthw.planes == 2))
        {
        // 4 color mode Mac II video mode

        pfnBlit = Blit4Mac;
        }
    else if (vsthw.planes == 2)
        {
        // medium rez

        Assert(vsthw.planes == 2);

        // in DirectX mode, pixels are doubled so skip scan
        if (pbSurface && vsthw.ypix == 200)
            {
            pfnBlit = Blit4X;
            }
        else
            {
            pfnBlit = Blit4;
            }
        }
    else if (fMac && (vsthw.planes == 4))
        {
        // 16 color mode Mac II video mode

        pfnBlit = Blit16Mac;
        }
    else if (vsthw.planes == 4)
        {
        // low rez

        Assert(vsthw.planes == 4);

        // in DirectX mode, pixels are doubled so skip scan
        if (pbSurface && vsthw.ypix == 200)
            {
            pfnBlit = Blit16X;
            }
        else
            {
            pfnBlit = Blit16;
            }
        }
    else // if (fMac && (vsthw.planes == 8))
        {
        // 256 color mode Mac II video mode

        pfnBlit = BlitMono;
        }

    if (vi.fVideoEnabled != FALSE)
        {
#if !defined(NDEBUG)
        printf("fVideoEnabled\n");
#endif
        __asm int 3;
        }

  __try
    {
    const void *pvScanLine = _alloca(cbScanSrc);

    for (scan = 0; scan < vsthw.ypix; scan++)
        {
        // convert scan line to min and lim 68000 addresses

        if (fRefresh && pvScanLine &&
              GuestToHostDataCopy(eaScanLine, (BYTE *)pvScanLine,
                 ACCESS_PHYSICAL_READ, cbScanSrc))
            {
            last = scan;
            first = min(scan, first);

            // DebugStr("%08X rendering scan line %d\n", eaScanLine, scan);

            (*pfnBlit)((BYTE *)pvScanLine, pbDst, cbScanSrc, cbScanDst);
            }

        pbDst += cbScanDst;

        if (pbSurface && (vsthw.ypix == 200))
            pbDst += cbScanDst;

        eaScanLine += cbScanSrc+cbSkip;
        }
    }
  __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (pbSurface)
        UnlockSurface();

    if (!pbSurface && last != -1)
        {
        RenderBitmap();
        UpdateOverlay();
        }

    memset(vi.rgbwrite, 0, sizeof(vi.rgbwrite));

Ldone:
    vi.fVideoEnabled = TRUE;
}

#endif  // ATARIST


//
// Linker directives help simplify the various builds
// by not having to muck with the makefile
//

#if _MSC_VER == 1200
#pragma comment(linker, "/defaultlib:src\\atari8.vm\\x6502")
#pragma comment(linker, "/defaultlib:src\\blocklib\\blockdev")
#endif

#if defined(ATARIST) || defined(SOFTMAC)
#pragma comment(linker, "/defaultlib:src\\utiln")
#pragma comment(linker, "/defaultlib:src\\680x0.cpu\\68k")
#pragma comment(linker, "/include:_Go68000")
#pragma comment(linker, "/include:_opcodes")
#pragma comment(linker, "/include:_ops1")
#pragma comment(linker, "/include:_ops2")
#pragma comment(linker, "/include:_ops3")
#pragma comment(linker, "/include:_ops3")
#pragma comment(linker, "/include:_ops4")
#pragma comment(linker, "/include:_ops5")
#pragma comment(linker, "/include:_ops6")
#pragma comment(linker, "/include:_ops7")
#pragma comment(linker, "/include:_ops8")
#pragma comment(linker, "/include:_ops9")
#pragma comment(linker, "/include:_opsA")
#pragma comment(linker, "/include:_opsB")
#pragma comment(linker, "/include:_opsC")
#pragma comment(linker, "/include:_opsD")
#pragma comment(linker, "/include:_opsE")
#pragma comment(linker, "/include:_opsF")
#endif

#pragma comment(linker, "/version:9.21")

#pragma comment(linker, "/defaultlib:comdlg32")
#pragma comment(linker, "/defaultlib:gdi32")
#pragma comment(linker, "/defaultlib:shell32")



