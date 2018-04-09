
/****************************************************************************

    GEMUL8R.C

    - Main window code

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    06/17/2012  darekm      Windows 8 / 64-bit / touch fixes

****************************************************************************/

//
// THEORY OF OPERATION 
//
// The type of VM you want will have a number, 0-2 are ATARI 8-bit types, etc.
//
// DetermineVMType will provide the VMINFO about its capabilities and the fn table to be able to call all the
// functions I'm about to describe.
//
// First we INSTALL a VM.
//
// Then we either INIT it (make a default new one) and eventually COLDBOOT it
// (by calling ColdStart instead of ColdbootVM directly, at least for now)
// OR we just LOADSTATE (to restore its state from disk) but NOT BOTH
//
// CreateNewBitmap creates the screen buffer for the instance. This must be done before EXEC, but after
// our window is created
//
// SelectInstance() makes an instance the current one, only a concept for which tile is showing when not in tiled mode
//
// To change out a cartridge, UnInit, then Init again. To change the VM type (eg. 800 to XL), UnInstall and Install again
//
// While all the VMs are running, the main loop will tell each thread to GO, then wait for them all to be DONE.
// If 1/60s hasn't passed yet and we're not in turbo mode, wait until it has. Repeat.
//
// Each thread simply waits for the GO event, sees if it is to GO or DIE, call EXECVM, signal DONE and repeat.
//
// When you're done with an instance, kill its thread and then UNINIT it, even if you called LoadState to initialize it.
// Finally, UNINSTALL
//

//#define _NO_CRT_STDIO_INLINE
#define _CRT_SECURE_NO_WARNINGS

#include <sys/stat.h>
#include "gemtypes.h" // main include file
#include <VersionHelpers.h>
#include <windowsx.h>

// you remember our main data structures from gemtypes.h, right?

PROPS v;				// global persistable stuff
INST vi;				// global non-persistable stuff
VMINST vrgvmi[MAX_VM];	// per instance not persistable stuff
VM rgvm[MAX_VM];		// per instance persistable stuff

// and our other globals

BOOL fDebug;
int nFirstTile; // at which instance does tiling start?
int sVM = -1;	// the tile with focus

#include "shellapi.h"

// My printf that uses OutputDebugString to send to the VS output tab
// Normal printf will go to the debug monitor, or nowhere if there is none
//
void ODS(char *fmt, ...)
{
	fmt;
#ifndef NDEBUG
	char dest[1024 * 16];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(dest, fmt, argptr);
	va_end(argptr);
	OutputDebugString(dest);
#endif
}


// Come up with a name for our instance - it goes in the title bar and menu.
// Use the name of the machine we're emulating followed by the cartridge name if it has one,
// or if not, the filename being used by the first disk drive.
// String must have enough space for MAX_PATH.
void CreateInstanceName(int i, LPSTR lpInstName)
{
	LPSTR lp = NULL;
	int z;

	// if this VM type suports cartridge and we have a loaded cartridge, find that name
	if (rgvm[i].pvmi->fUsesCart && rgvm[i].rgcart.fCartIn)
	{
		lp = rgvm[i].rgcart.szName;
	}

	// otherwise, use a disk name
	if (lp == NULL && rgvm[i].rgvd[0].sz)
	{
		lp = rgvm[i].rgvd[0].sz;
	}

	// only take the filename, none of the path
	for (z = (int)strlen(lp) - 1; z >= 0; z--)
	{
		if (lp[z] == '\\') break;
	}

	sprintf(lpInstName, "%s - %s", rgvm[i].szModel, lp + z + 1);
}

/****************************************************************************

FUNCTION: DisplayStatus()

PURPOSE: updates window title with current status

COMMENTS:

****************************************************************************/

void DisplayStatus(int iVM)
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

	// put our instance name in the title, which may have changed recently
	char pInst[MAX_PATH];
	pInst[0] = 0;
	
	// no tile in focus? No name for the title bar
	if (iVM >= 0)
	    CreateInstanceName(iVM, pInst);
	
	sprintf(rgch0, "%s - %s", vi.szAppName, pInst);
#endif

#if 0
    if (vi.fExecuting)
        {
        if (FIsMac(vmCur.bfHW))
            {

            sprintf(rgch, "\0%d%s, %dx%d%s\0 %d %d",
            (vi.cbRAM[0] < 4194304) ? vi.cbRAM[0]/1024 : vi.cbRAM[0]/1048576,
            (vi.cbRAM[0] < 4194304) ? "K" : "M",
            vsthw[v.iVM].xpix, vsthw[v.iVM].ypix, (vsthw[v.iVM].planes == 1) ? rgszMode[0] : " Color",
            0, 0);
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
            vsthw[v.iVM].xpix, vsthw[v.iVM].ypix, rgszMode[vsthw[v.iVM].planes-1],
            vi.fHaveFocus, vi.fGEMMouse);
            }
        }
    else
        {
        sprintf(rgch, " - Not running");
        }
#endif

    strcat(rgch0, rgch);

	// are we running at normal speed or turbo speed
    sprintf(rgch, " (%s-%i%%)", fBrakes ? "1.8 MHz" : "Turbo", cEmulationSpeed ? ((int)(100000000 / cEmulationSpeed)) : 0);
    strcat(rgch0, rgch);
		
    if (v.fZoomColor)
    {
		// Is the display stretched to fill the window?
        strcat(rgch0, " - Stretched");
    }

    SetWindowText(vi.hWnd, rgch0);

	// too expensive now that we update the title a lot with the emulation speed
    //TrayMessage(vi.hWnd, NIM_MODIFY, 0, LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), rgch0);
}


// Gets the number of elapsed 6502 cycles, I know that's not machine independent.
ULONGLONG GetCycles()
{
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	qpc.QuadPart -= vi.qpcCold;
	if (qpc.QuadPart == 0)	// wow, you JUST called us!
		return 0;
	ULONGLONG a = (qpc.QuadPart * 178979ULL);
	ULONGLONG b = (vi.qpfCold / 10ULL);
	ULONGLONG c = a / b;
	return c;
}

// This is the ENTIRE CODE for a VM thread.
// Each VM waits its turn, does one frame of execution and video and audio, and signals it's done.
// Rinse and repeat until it is killed
//
DWORD WINAPI VMThread(LPVOID l)
{
	int iVM = *(int *)l;

	while (1)
	{
		WaitForSingleObject(hGoEvent[iVM], INFINITE);

		if (fKillThread[iVM])
			return 0;

		vrgvmi[iVM].fWantDebugger = !FExecVM(iVM, FALSE, TRUE);
		
		if (vrgvmi[iVM].fWantDebugger)
			vi.fExecuting = FALSE;	// we only reset, never set it

		SetEvent(hDoneEvent[iVM]);
	}
}

void CreateDebuggerWindow()
{
    // check for parent process command prompt

    if (vi.fParentCon)
        return;

    // create a console window for debugging

    AllocConsole();
	
	freopen("CONOUT$", "wb", stdout);

    SetConsoleTitle("Virtual Machine Debugger");

    printf(""); // flush the log file

//    EnableWindow(vi.hWnd,FALSE);
}

void DestroyDebuggerWindow()
{
    // check for parent process command prompt

    if (vi.fParentCon)
        return;

	if (!vi.fInDebugger /* && !v.fDebugMode */)
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);	// why is this necessary?
		FreeConsole();
	}

    // set focus to vi.hWnd

//  EnableWindow(vi.hWnd,TRUE);
    GetFocus();
}

// Fix the FILE menu to list of all the valid VM's currently loaded.
// It checkmarks the current one, shows the hotkey for switching instances on the next one
// and allows you to choose one from the list to switch to.
//
void CreateVMMenu()
{
	int z;
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	char mNew[MAX_PATH + 10];
	BOOL fNeedNextKey = FALSE;
	BOOL fNeedPrevKey = FALSE;
	int zLast = -1;

	int iFound = 0;
	for (z = MAX_VM - 1; z >= 0; z--)
	{
		// every valid VM goes in the list
		if (rgvm[z].fValidVM)
		{
			// we're counting backwards, so the first one found goes in as IDM_VM1, the highest identifier.
			if (iFound == 0)
			{
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = mNew;
				CreateInstanceName(z, mNew);	// get a name for it
				SetMenuItemInfo(vi.hMenu, IDM_VM1, FALSE, &mii);
				CheckMenuItem(vi.hMenu, IDM_VM1, (z == (int)v.iVM) ? MF_CHECKED : MF_UNCHECKED);	// check if the current one
				EnableMenuItem(vi.hMenu, IDM_VM1, !v.fTiling ? 0 : MF_GRAYED);	// grey if tiling

				// current inst is bottom of the list, make a note to put the NEXT INST hot key label on the top one
				// and the PREV INST hot key label one above us
				if (z == (int)v.iVM)
				{		
					fNeedNextKey = TRUE;
					fNeedPrevKey = TRUE;
				}
				
				iFound++;
			}

			// subsequent (lower) VMs go before IDM_V1 in the menu
			else
			{
				mii.fMask = MIIM_STRING | MIIM_ID;
				mii.wID = IDM_VM1 - iFound;
				mii.dwTypeData = mNew;
				CreateInstanceName(z, mNew);	// get a name for it

				// this is the previous inst to the current one
				if (fNeedPrevKey) {
					strcat(mNew, "\tShift+Ctrl+F11");
					fNeedPrevKey = FALSE;
				}

				DeleteMenu(vi.hMenu, IDM_VM1 - iFound, 0);	// erase the old one
				InsertMenuItem(vi.hMenu, IDM_VM1 - iFound + 1, 0, &mii); // insert before the one we last did.
				CheckMenuItem(vi.hMenu, IDM_VM1 - iFound, (z == (int)v.iVM) ? MF_CHECKED : MF_UNCHECKED);	// check if the current one
				EnableMenuItem(vi.hMenu, IDM_VM1 - iFound, !v.fTiling ? 0 : MF_GRAYED);	// grey if tiling
				zLast = z;

				if (fNeedPrevKey)
				{
					mii.fMask = MIIM_STRING;
					mii.dwTypeData = mNew;
					mii.cch = sizeof(mNew);
					GetMenuItemInfo(vi.hMenu, IDM_VM1 - iFound + 1, 0, &mii);
					strcat(mNew, "\tCtrl+F11");
					SetMenuItemInfo(vi.hMenu, IDM_VM1 - iFound + 1, FALSE, &mii);

					fNeedPrevKey = TRUE;
				}

				// we-re the current one... add the hotkey "Ctrl+F11" to the menu item after us
				// and note that the next one we find is the prev one
				if (z == (int)v.iVM)
				{
					mii.fMask = MIIM_STRING;
					mii.dwTypeData = mNew;
					mii.cch = sizeof(mNew);
					GetMenuItemInfo(vi.hMenu, IDM_VM1 - iFound + 1, 0, &mii);
					strcat(mNew, "\tCtrl+F11");
					SetMenuItemInfo(vi.hMenu, IDM_VM1 - iFound + 1, FALSE, &mii);

					// don't put PREV KEY hotkey over top of NEXT KEY hotkey, NEXT should get priority
					if (v.cVM > 2)
						fNeedPrevKey = TRUE;
				}
				iFound++;
			}
		}
	}

	// add the NEXT or PREV INST hotkey to the first instance
	if ((fNeedNextKey || fNeedPrevKey) && zLast >= 0)
	{
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = mNew;
		mii.cch = sizeof(mNew);
		GetMenuItemInfo(vi.hMenu, fNeedNextKey ? (IDM_VM1 - iFound + 1) : IDM_VM1, 0, &mii);
		strcat(mNew, fNeedNextKey ? "\tCtrl+F11" : "\tShift+Ctrl+F11");
		SetMenuItemInfo(vi.hMenu, fNeedNextKey ? (IDM_VM1 - iFound + 1) : IDM_VM1, FALSE, &mii);
	}

	// in case we're fixing the menus because one was deleted, this will still be hanging around
	// never delete our anchor!
	if (v.cVM > 0)
		DeleteMenu(vi.hMenu, IDM_VM1 - v.cVM, 0);
	else
	{
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = mNew;
		mii.cch = sizeof(mNew);
		strcpy(mNew, "<NO VMs loaded>");
		SetMenuItemInfo(vi.hMenu, IDM_VM1, FALSE, &mii);
		CheckMenuItem(vi.hMenu, IDM_VM1, MF_UNCHECKED);
	}
}

// Something changed that affects the menus. Set all the menus up right.
//
void FixAllMenus()
{
	// use the active tile or if not tiled, the main instance. If tiled without anything active, don't allow anything
	int inst = (v.fTiling && sVM >= 0) ? sVM : (v.fTiling ? -1 : v.iVM);

	// not implemented yet
	//EnableMenuItem(vi.hMenu, IDM_EXPORTDOS, MF_GRAYED);

	// toggle basic is never a thing outside of XFORMER. If mixed VMs, it's only relevant for an 8bit VM
#ifdef XFORMER
	// toggle BASIC also has to be relevant, only valid for VM type that supports a cartridge
	EnableMenuItem(vi.hMenu, IDM_TOGGLEBASIC, (inst >= 0 && rgvm[inst].pvmi->fUsesCart)	? MF_ENABLED : MF_GRAYED);
#else
	DeleteMenu(vi.hMenu, IDM_TOGGLEBASIC, 0);
#endif

#ifdef NDEBUG
	DeleteMenu(vi.hMenu, IDM_DEBUGGER, 0);
#else
	EnableMenuItem(vi.hMenu, IDM_DEBUGGER, (inst >= 0) ? 0 : MF_GRAYED);
#endif

	// Checkmark if these modes are active
	CheckMenuItem(vi.hMenu, IDM_FULLSCREEN, v.fFullScreen ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_STRETCH, v.fZoomColor ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_TILE, v.fTiling ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_TURBO, fBrakes ? MF_UNCHECKED : MF_CHECKED);
	CheckMenuItem(vi.hMenu, IDM_AUTOLOAD, v.fSaveOnExit ? MF_CHECKED : MF_UNCHECKED);

	EnableMenuItem(vi.hMenu, IDM_STRETCH, !v.fTiling ? 0 : MF_GRAYED);

	// some menu items not appropriate if tiling, but there's no current tile to do anything to
	// it could do it on 
	EnableMenuItem(vi.hMenu, IDM_TIMETRAVEL, (inst >= 0) ? 0 : MF_GRAYED);
    EnableMenuItem(vi.hMenu, IDM_COLORMONO, (inst >= 0) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_COLDSTART, (inst >= 0) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_WARMSTART, (inst >= 0) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_DELVM, (inst >= 0) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_CHANGEVM, (inst >= 0) ? 0 : MF_GRAYED);
	EnableMenuItem(vi.hMenu, IDM_SAVEAS, (inst >= 0) ? 0 : MF_GRAYED);

	// Initialize the virtual disk menu items to show the associated file path with each drive.
	// Grey the unload option for a disk that isn't loaded

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	char mNew[MAX_PATH + 10];
	mii.dwTypeData = mNew;

	if (inst != -1)
	{
		// call them D1: <filespec> - grey an unload choice if nothing is loaded there, and grey them all in tiled mode
		sprintf(mNew, "&D1: %s ...", rgvm[inst].rgvd[0].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D1, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D1, 0);	// might be grey from before
		EnableMenuItem(vi.hMenu, IDM_D1U, (rgvm[inst].rgvd[0].sz[0]) ? 0 : MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_IMPORTDOS1, rgvm[inst].rgvd[0].sz[0] ? 0 : MF_GRAYED);

		sprintf(mNew, "&D2: %s ...", rgvm[inst].rgvd[1].sz);
		SetMenuItemInfo(vi.hMenu, IDM_D2, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D2, 0);
		EnableMenuItem(vi.hMenu, IDM_D2U, (rgvm[inst].rgvd[1].sz[0]) ? 0 : MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_IMPORTDOS2, (rgvm[inst].rgvd[1].sz[0]) ? 0 : MF_GRAYED);
	}

	// no active instance
	else
	{
		sprintf(mNew, "&D1: ...");
		SetMenuItemInfo(vi.hMenu, IDM_D1, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D1, MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_D1U, MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_IMPORTDOS1, MF_GRAYED);
		
		sprintf(mNew, "&D2: ...");
		SetMenuItemInfo(vi.hMenu, IDM_D2, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_D2, MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_D2U, MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_IMPORTDOS2, MF_GRAYED);
	}

	// if this VM type supports cartridges, and there is a valid active instance
	if (inst >= 0 && rgvm[inst].pvmi->fUsesCart)
	{
		// show the name of the current cartridge, if there is one
		if (rgvm[inst].rgcart.fCartIn)
		{
			sprintf(mNew, "&Cartridge %s ...", rgvm[inst].rgcart.szName);
			SetMenuItemInfo(vi.hMenu, IDM_CART, FALSE, &mii);
		}
		else
		{
			sprintf(mNew, "&Cartridge...");
			SetMenuItemInfo(vi.hMenu, IDM_CART, FALSE, &mii);
		}

		// grey "remove cart" if there isn't one
		EnableMenuItem(vi.hMenu, IDM_CART, 0); // might be grey from before
		EnableMenuItem(vi.hMenu, IDM_NOCART, (rgvm[inst].rgcart.fCartIn) ? 0 : MF_GRAYED);
	}
	else
	{
		sprintf(mNew, "&Cartridge...");
		SetMenuItemInfo(vi.hMenu, IDM_CART, FALSE, &mii);
		EnableMenuItem(vi.hMenu, IDM_CART, MF_GRAYED);
		EnableMenuItem(vi.hMenu, IDM_NOCART, MF_GRAYED);
	}

	// Now fix the FILE menu's list of all valid VM's
	CreateVMMenu();

	// populate the menu with all the relevant choices of VM types they can make

	for (int zz = 0; zz < 32; zz++)
	{
		// is this a type of VM we can handle?
		if (DetermineVMType(zz))
		{
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = mNew;
			strcpy(mNew, rgszVM[zz + 1]);
			SetMenuItemInfo(vi.hMenu, IDM_ADDVM1 + zz, FALSE, &mii); // show the name of the type of VM (eg. ATARI 800)
			EnableMenuItem(vi.hMenu, IDM_ADDVM1 + zz, (v.cVM == MAX_VM) ? MF_GRAYED : MF_ENABLED);	// grey it if too many already
		}
		else
			DeleteMenu(vi.hMenu, IDM_ADDVM1 + zz, 0); // we don't use this type in this build

	}

	// something that changed the menus probably changes the title bar
	DisplayStatus((v.fTiling && sVM >= 0) ? sVM : (v.fTiling ? -1 : v.iVM));
}


// Put the next filename in lpCmdLine into sFile. A space is the delimiter, but there can be spaces inside quotes
// return the position of the first character of the next filename, or NULL
// If quotes surround the name, remove them.
// 
// Also return the type of VM that can support this file (or -1 if none) and what kind of media it is, disk or cart (1 or 2)
//
char *GetNextFilename(char *sFile, char *lpCmdLine, int *VMtype, int *MEDIAtype)
{
	char *lpF = sFile;	// where the filename we found will be

	if (!lpCmdLine || !*lpCmdLine)
	{
		*sFile = 0;
		return NULL;
	}

	char c = ' ';		// look for a space

	if (*lpCmdLine == '\"') {
		c = '\"';		// beginning quote? Look for a matching quote instead
		lpCmdLine++;	// filename begins after the quote
	}

	while (*lpCmdLine && *(lpCmdLine) != c)
		*sFile++ = *lpCmdLine++;

	*sFile = 0;	// terminate the filename

	if (c == '\"')
		lpCmdLine++;	// skip the closing quote

	lpCmdLine++;	// skip the space

	// this particular filename is not looking real, move along
	if (!lpF || !*lpF || strlen(lpF) < 5)
		return lpCmdLine;

	// what extension is this?
	char ext[4];
	strcpy (ext, &lpF[strlen(lpF) - 3]);

	// !!! open the file and see if it's actually an ATR by another name and pretend it's an ATR? Hacky and slow

	// look at all the possible VM types to find one that can handle this file
	// use the first that comes along (eg. ATARI 800 will be found before 800XL)

	for (int z = 0; z < 32; z++)
	{
		PVMINFO pvmi;

		// is this a kind of VM we can handle?
		pvmi = DetermineVMType(z);
		if (pvmi)
		{
			char *szF;
			char extV[_MAX_PATH];
			char *pextV;

			// look through all the extensions a virtual disk or cart of that VM type can use
			// this is an OpenFilename string, terminated by double 0, with all the extensions inside the string somewhere

			for (int mt = 1; mt <= 2; mt++)
			{
				// we don't use carts
				if (mt == 2 && pvmi->fUsesCart == FALSE)
					break;

				// look for disk or cart?
				if (mt == 1)
					szF = pvmi->szFilter;
				else
					szF = pvmi->szCartFilter;

				// terminated by double 0
				while (*szF || *(szF + 1))
				{
					pextV = extV;

					// find the next . (beginning of extension)
					while (*szF != '.' && !(!*szF && !*(szF + 1)))
						szF++;

					szF++;	// skip the .

					// get the ext
					while (*szF && ((*szF >= 'A' && *szF <= 'Z') || (*szF >= 'a' && *szF <= 'z')))
						*pextV++ = *szF++;
					*pextV = 0;

					// that's the one we are! (case insensitive)
					if (_stricmp(ext, extV) == 0)
					{
						if (VMtype)
							*VMtype = z;
						if (MEDIAtype)
							*MEDIAtype = mt;

						return lpCmdLine;
					}
				}
			}
		}
	}

	if (VMtype)
		*VMtype = -1;
	if (MEDIAtype)
		*MEDIAtype = 0;

	// don't fail, let them continue to look for more valid disks or carts
	return lpCmdLine;
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
    MSG msg;
	BOOL fProps;

	// init the VM structure sizes so they look valid
	// create the thread's message queue
	for (int ii = 0; ii < MAX_VM; ii++)
		rgvm[ii].cbSize = sizeof(VM);

	// initialize our clock
	LARGE_INTEGER qpc;
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	vi.qpfCold = qpf.QuadPart;
	QueryPerformanceCounter(&qpc);
	vi.qpcCold = qpc.QuadPart;

    // Get paths

    GetCurrentDirectory(MAX_PATH, (char *)&vi.szDefaultDir);
    GetWindowsDirectory((char *)&vi.szWindowsDir, MAX_PATH);

    // find out basic things about the processor

    GetSystemInfo(&vi.si);

    // It's rude to store settings globally in the WINDOWS directory
    // since there may be multiple users on the machine (i.e. Windows XP).
    // So we check to see that IE4.71 or higher is running and if so use
    // the per-user application directory

	if (S_OK == SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (char *)&vi.szWindowsDir))
	{
		strcat(vi.szWindowsDir, "\\Emulators");
		CreateDirectory(vi.szWindowsDir, NULL);
		//printf("Application data path is: '%s'\n", vi.szWindowsDir);
	}

    // Now that the app data directory is set up, it is ok to read the INI file

	// Initialize our global state (PROPS v)
    InitProperties();
	fProps = LoadProperties(NULL, TRUE); // load our basic props only

	fBrakes = TRUE; // always start up in emulated speed mode

    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.
    vi.hInst = hInstance;

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

#if defined(ATARIST) || defined (SOFTMAC)
	// Get the high resolution time frequency
	// The result will be either around 1.193182 MHz
	// or it will be the Pentium clock speed.

	QueryTickCtr();
	
	// Generate the shift co-efficients so that llTicks can be shifted by
    // at most 4 coefficients to produce an approximate count of milliseconds

    GenerateCoefficients(0);

    // Generate the shift co-efficients so that llTicks can be shifted by
    // at most 4 coefficients to produce the 783330 Hz E-clock count

    GenerateCoefficients(1);
#endif

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
	if (!vi.hAccelTable || !vi.hMenu)
		return FALSE;

    vi.fWinNT = TRUE;

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

	// Create all our thread events before starting to load things
	for (int iVM = 0; iVM < MAX_VM; iVM++)
	{
		hGoEvent[iVM] = CreateEvent(NULL, FALSE, FALSE, NULL);
		hDoneEvent[iVM] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	// In case we start up with a parameter on the cmd line
	BOOL fSkipLoad = FALSE;

	//char test[130] = "\"c:\\danny\\8bit\\atari\\XLMule.ATR\"";
	//lpCmdLine = test;
	
	// assume we're loading the default .ini file
	char *lpLoad = NULL;

	// what if more than one VM type supports the same file extension?
	if (lpCmdLine && lpCmdLine[0])
	{
		char sFile[MAX_PATH];
		int iVM, len, VMtype, MEDIAtype;
		BOOL f;

		while (lpCmdLine && strlen(lpCmdLine) > 4)
		{
			// parse the next filename out of the list, removing quotes if there
			// and tell us what VM can handle this kind of file, and what kind of media it is
			lpCmdLine = GetNextFilename(sFile, lpCmdLine, &VMtype, &MEDIAtype);

			len = strlen(sFile);
			
			// a disk
			if (VMtype != -1 && MEDIAtype == 1)
			{
				if ((iVM = AddVM(VMtype)) != -1)	// does the FInstalVM for us
				{
					strcpy(rgvm[iVM].rgvd[0].sz, sFile); // replace disk 1 image with the argument before Init'ing
					rgvm[iVM].rgvd[0].dt = DISK_IMAGE;
					f = FALSE;
					if (FInitVM(iVM))
					{
						f = ColdStart(iVM);
						if (f && vi.hdc)
							CreateNewBitmap(iVM);	// we might not have a window yet, we'll do it when we do
					}
					if (f)
					{
						if (!fSkipLoad)
							SelectInstance(iVM);
						fSkipLoad = TRUE;
					}
					else
						DeleteVM(iVM);

				}	// using drag/drop, just move on with our lives if there's an error
			}
			
			// a cartridge
			else if (VMtype != -1 && MEDIAtype == 2)
			{
				if ((iVM = AddVM(VMtype)) != -1)	// does the FInstalVM for us
				{
					strcpy(rgvm[iVM].rgcart.szName, sFile); // set the cartridge name to the argument
					rgvm[iVM].rgcart.fCartIn = TRUE;
					f = FALSE;
					if (FInitVM(iVM))
					{
						f = ColdStart(iVM);
						if (f && vi.hdc)
							CreateNewBitmap(iVM);	// we might not have a window yet, we'll do it when we do
					}
					if (f)
					{
						if (!fSkipLoad)
							SelectInstance(iVM);
						fSkipLoad = TRUE;
					}
					else
						DeleteVM(iVM);	// bad cartridge? Don't show an empty VM, just kill it

				}	// using drag/drop, just move on with our lives if there's an error
			}

			// found a .gem file. Ignore everything else and just load this

			else if (_stricmp(sFile + len - 3, "gem") == 0)
			{
				fSkipLoad = FALSE;	// changed our mind, loading this .gem file

				// delete all of our VMs we accidently made before we found the .gem file
				for (int z = 0; z < MAX_VM; z++)
				{
					if (rgvm[z].fValidVM)
						DeleteVM(z);
				}
				
				lpLoad = sFile;	// load this .gem file
				break;	// stop loading more files
			}
		}
	}

	// If we drag/dropped more than 1 instance, come up in tiled maximized mode (so you can see the instance names in the title bar)
	// Otherwise, keep the last global settings
	if (v.cVM > 1)
	{
		v.fTiling = TRUE;
		v.fFullScreen = FALSE;
		v.swWindowState = SW_SHOWMAXIMIZED;
	}
	// drag and drop a single file shouldn't be tiled, but re-loading your preference that way should
	else if (fSkipLoad)
		v.fTiling = FALSE;

	// If we were drag and dropped some files, or we didn't save last time, don't load the old VMs
    if (!fSkipLoad && v.fSaveOnExit)
		fProps = LoadProperties(lpLoad, FALSE);
	
	// If we didn't restore/drag any VM's, we need some, so make some
	if (v.cVM == 0)
	{
		v.iVM = -1;
		// !!! TEMP TEST
		//CreateAllVMs();
		
#if defined(ATARIST) || defined(SOFTMAC)
		// Now go and prompt the user with First Time Setup if necessary
		if (!AutoConfigure(FALSE))
			DialogBox(vi.hInst,       // current instance
				MAKEINTRESOURCE(IDD_FIRSTTIME), // dlg resource to use
				NULL,          // parent handle
				FirstTimeProc);

		// This will prevent the First Time Setup from popping up again
		
		erties(NULL);
#endif // ATARIST
	}

	vi.fExecuting = TRUE;	// OK, go! This will get reset when a VM hits a breakpoint, or otherwise fails

	FixAllMenus();

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

    // if we didn't restore a saved position, make a default location for our window
	if (!fProps)
	{
		if (v.iVM >= 0)
		{
			v.rectWinPos.left = min(50, (GetSystemMetrics(SM_CXSCREEN) - 640));
			v.rectWinPos.right = v.rectWinPos.left;
			v.rectWinPos.top = min(50, (GetSystemMetrics(SM_CYSCREEN) - 420));
			v.rectWinPos.bottom = v.rectWinPos.top;

			// add our probable non-client area. !!! I can't get this right so I have to add 20 and 30 at least!
			int thickX = GetSystemMetrics(SM_CXSIZEFRAME) * 2 + 40;
			int thickY = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) + 60;

			// if we're < 640 x 480, make it double sized
			if (rgvm[v.iVM].pvmi->uScreenX < 640 || rgvm[v.iVM].pvmi->uScreenY < 480)
			{
				v.rectWinPos.right += rgvm[v.iVM].pvmi->uScreenX * 2 + thickX;
				v.rectWinPos.bottom += rgvm[v.iVM].pvmi->uScreenY * 2 + thickY;
			}
			else
			{
				v.rectWinPos.right += rgvm[v.iVM].pvmi->uScreenX + thickX;
				v.rectWinPos.bottom += rgvm[v.iVM].pvmi->uScreenY + thickY;
			}
		}
		else
		{
			// default to 640x480 if we're VM-less and stateless
			v.rectWinPos.left = min(50, (GetSystemMetrics(SM_CXSCREEN) - 640));
			v.rectWinPos.right = v.rectWinPos.left + 640;
			v.rectWinPos.top = min(50, (GetSystemMetrics(SM_CYSCREEN) - 420));
			v.rectWinPos.bottom = v.rectWinPos.top + 480;
		}
	}

    //printf("init: client rect = %d, %d, %d, %d\n", v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right, v.rectWinPos.bottom);

	// Create a main window for this application instance.
	vi.hWnd = CreateWindowEx(0L,
        vi.szAppName,       // See RegisterClass() call.
        vi.szTitle,         // Text for window title bar.
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX,
        v.rectWinPos.left,    // X posisition
        v.rectWinPos.top,    // Y position
		v.rectWinPos.right - v.rectWinPos.left, // Window width
		v.rectWinPos.bottom - v.rectWinPos.top, // Window height
        NULL,            // Overlapped windows have no parent.
        NULL,            // Use the window class menu.
        hInstance,       // This instance owns this window.
        NULL             // We don't use any data in our WM_CREATE
    );

    // If window could not be created, return "failure"
    if (!vi.hWnd)
        return (FALSE);

    // Attach the menu, make the window visible; update its client area; and return "success"
	SetMenu(vi.hWnd, vi.hMenu);
	
	// don't ever come up minimized, that's dumb
	ShowWindow(vi.hWnd, (v.swWindowState == SW_SHOWMAXIMIZED) ? SW_SHOWMAXIMIZED: nCmdShow);
    
	UpdateWindow(vi.hWnd);     // Sends WM_PAINT message

#if defined(ATARIST) || defined(SOFTMAC)
	// Initialize SCSI

	if (!v.fNoSCSI)
        FInitBlockDevLib();

	vi.hIdleThread  = CreateThread(NULL, 4096, (void *)IdleThread, 0, 0, &l);
	if (!vi.hIdleThread)
		return FALSE;

	vi.hVideoThread = NULL; // CreateThread(NULL, 4096, (void *)ScreenThread, 0, 0, &l);

	// is this really necessary?
	{
		FLASHWINFO fi;

		fi.cbSize = sizeof(fi);
		fi.hwnd = vi.hWnd;
		fi.dwFlags = FLASHW_TRAY;
		fi.uCount = 1;
		fi.dwTimeout = 0;

		FlashWindowEx(&fi);

#ifndef NDEBUG
		// Allow the screen thread to complete the debug timing calibration
		//Sleep(900);
#endif

		fi.dwFlags = FLASHW_STOP;
		FlashWindowEx(&fi);
	}

#endif // ATARIST
    
#if 0
    // create a frame buffer in shared memory for future cloud use
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

    //PostMessage(vi.hWnd, WM_COMMAND, IDM_COLDSTART, 0);

    // One time un-Hibernate
    if (v.fSkipStartup & 2)
        {
        v.fSkipStartup &= ~2;
        SaveProperties(NULL);

//        SetWindowText(vi.hWnd, "Loading saved state...");
//        PostMessage(vi.hWnd, WM_COMMAND, IDM_LOADSTATE, 0);
        }

#endif

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

#if 0
			//            Assert(msg.hwnd == vi.hWnd);

			if (msg.hwnd == NULL)
			{
				//UINT message = msg.message;
				//WPARAM uParam = msg.wParam;
				//LPARAM lParam = msg.lParam;

				switch (msg.message)
				{

					// we don't seem to send these to ourself anymore
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
				}

				// don't pass thread message to window handler
				continue;
			}
#endif

			if (!TranslateAccelerator(msg.hwnd, vi.hAccelTable, &msg))
			{
				// !!! Does ATARI800 need a special fDontTranslate VMINFO cap?
				// for Xformer, we need special German keys to translate.
				//if (!FIsAtari8bit(vmCur.bfHW) && (!vi.fExecuting))
				TranslateMessage(&msg);// Translates virtual key codes
				DispatchMessage(&msg); // Dispatches message to window
			}
		}

		// =====================
		// THIS IS OUR MAIN LOOP
		// =====================

		HANDLE hVG[MAX_VM];	// handles of threads we want to go
		HANDLE hVD[MAX_VM];
		int iV = 0;

		// Tell the thread(s) to go.
		if (v.cVM && v.fTiling)
		{
			RECT rect;
			GetClientRect(vi.hWnd, &rect);

			// Don't waste time executing tiles we can't see. Pause them for performance.

			// find the VM at the top of the page
			int iVM = nFirstTile;
			while (iVM < 0 || rgvm[iVM].fValidVM == FALSE)
				iVM++;

			int x, y, iDone = iVM;

			// !!! tile sizes are arbitrarily based off the first VM, what about when sizes are mixed?
			int nx = (rect.right * 10 / vvmhw[iVM].xpix + 5) / 10; // how many fit across (if 1/2 showing counts)?

			for (y = sWheelOffset; y < rect.bottom; y += vvmhw[iVM].ypix /* * vi.fYscale*/)
			{
				for (x = 0; x < nx * vvmhw[iVM].xpix; x += vvmhw[iVM].xpix /* * vi.fXscale*/)
				{
					// Don't consider tiles completely off screen
					if (y + vvmhw[iVM].ypix > 0 && y < rect.bottom)
					{
						hVG[iV] = hGoEvent[iVM];	// found one
						hVD[iV++] = hDoneEvent[iVM];
					}

					// advance to the next valid bitmap
					do
					{
						iVM = (iVM + 1) % MAX_VM;
					} while (!rgvm[iVM].fValidVM);

					// we've considered them all
					if (iDone == iVM)
						break;
				}
				if (iDone == iVM)
					break;
			}

			// tell each thread to go
			for (iVM = 0; iVM < iV; iVM++)
				SetEvent(hVG[iVM]);

			// wait for them to complete one frame
			WaitForMultipleObjects(iV, hVD, TRUE, INFINITE);
		}
		else if (v.iVM >= 0)
		{
			assert(v.cVM);
			SetEvent(hGoEvent[v.iVM]);
			WaitForSingleObject(hDoneEvent[v.iVM], INFINITE);
		}

		static ULONGLONG cCYCLES = 0;
		static ULONGLONG cErr = 0;
		static unsigned lastVM = 0;

		// !!! Speed things up by not doing then when in the console, or at least make it actually update the screen
		RenderBitmap();

		// we're emulating its original speed (fBrakes) so slow down to let real time catch up (1/60th sec)
		// don't let errors propogate

		// !!! This number is bogus while tracing

		const ULONGLONG cJif = 29830; // 1789790 / 60
		ULONGLONG cCur = GetCycles() - cCYCLES;

		// report back how long it took
		cEmulationSpeed = cCur * 1000000 / cJif;

		while (fBrakes && ((cJif - cErr) > cCur)) {
			Sleep(1);
			cCur = GetCycles() - cCYCLES;
		}

		cErr = cCur - (cJif - cErr);
		if (cErr > cJif) cErr = cJif;	// don't race forever to catch up if game paused, just carry on (also, it's unsigned)
		cCYCLES = GetCycles();


		// every second, update our clock speed indicator
		// When tiling, only show the name of the one you're hovering over, otherwise it changes constantly
		static ULONGLONG sCJ;
		cCur = GetCycles();
		if (cCur - sCJ >= 1789790)
		{
			int ids = (v.fTiling && sVM >= 0) ? sVM : (v.fTiling ? -1 : v.iVM);
			DisplayStatus(ids);
			sCJ = cCur;
		}

#ifndef NDEBUG
		// Break into debugger is asked to, or if in debug mode and VM died or hit breakpoint
		if (vi.fDebugBreak || ((vi.fInDebugger /*|| v.fDebugMode */) && !vi.fExecuting))
		{
			vi.fInDebugger = TRUE;	// we are currently tracing
			vi.fExecuting = FALSE;	// error or breakpoint, so the VM is asking for debugger

			SetFocus(GetConsoleWindow());	// !!! doesn't work

			// go into the debugger for every VM that needs it
			for (int iVM = 0; iVM < MAX_VM; iVM++)
			{
				// user wants the "current" inst broken into, or a VM wants itself broken into
				// !!! You can probably quickly change "current" VMs before this code executes
				if ((vi.fDebugBreak && iVM == v.iVM) || rgvm[iVM].fValidVM && vrgvmi[iVM].fWantDebugger)
					if (!FMonVM(iVM)) // somebody wants to quit
						break;
			}
			
			SetFocus(vi.hWnd);				// !!! doesn't work

			vi.fDebugBreak = FALSE;	// user wants us in the debugger
			vi.fExecuting = TRUE;
			DestroyDebuggerWindow();
		}
#endif

    } // forever

    // Reset tick resolution

	Lquit:
#if !defined(_M_ARM)
    timeEndPeriod(1);
#endif

#if defined (ATARIST) || defined (SOFTMAC)
#ifndef NDEBUG
	for (int i = 0; i < MAXOSUsable; i++)
	{
		 // printf("vmi %2d: pvBits = %08p\n", vrgvmi[i]);
	}
#endif
#endif

    // cloud support - clear the share memory thread ID
    //*(DWORD *)&vi.pbFrameBuf[1280*720] = 0;

    return (msg.wParam); // Returns the value from PostQuitMessage
}

int PrintScreenStats()
{
#if !defined(NDEBUG)
		//printf("Screen: %4d,%4d  Full: %4d,%4d  sx,sy: %4d,%4d  dx,dy: %4d,%4d  xpix,ypix: %4d,%4d  scale: %d,%d\n",
		//GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), GetSystemMetrics(SM_CXFULLSCREEN),
		//GetSystemMetrics(SM_CYFULLSCREEN), vi.sx, vi.sy, vi.dx, vi.dy, vvmhw[v.iVM].xpix, vvmhw[v.iVM].ypix, vi.fXscale, vi.fYscale);

		//printf("        windows position x = %d, y = %d, r = %d, b = %d\n",
		//v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right, v.rectWinPos.bottom);
#endif

	return 0;
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

	if (/* vi.fExecuting && */ !vi.fGEMMouse && vi.fHaveFocus)
	{
		RECT rect;

		//Assert(vi.fHaveFocus);

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
		//Assert(vi.fHaveFocus);

		ShowCursor(TRUE);
		SetWindowsMouse(0);
		vi.fGEMMouse = FALSE;
		ClipCursor(NULL);
	}
}


/****************************************************************************

    FUNCTION: SetBitmapColors/FCreateOurPalette

    PURPOSE:  Set the current system palette to Atari colors

    COMMENTS: return TRUE if it seems to have worked

****************************************************************************/

BOOL SetBitmapColors(int iVM)
{
    int i;

	// this VM is always or currently in Monochrome mode
    if (rgvm[iVM].pvmi->planes == 1 || vvmhw[iVM].fMono)
    {
        if (vi.fInDirectXMode)
            return TRUE;

        if (v.fNoMono)
            {
            vvmhw[iVM].rgrgb[0].rgbRed =
            vvmhw[iVM].rgrgb[0].rgbGreen =
            vvmhw[iVM].rgrgb[0].rgbBlue = 0;
            vvmhw[iVM].rgrgb[0].rgbReserved = 0;

            for (i = 1; i < 256; i++)
                {
                vvmhw[iVM].rgrgb[i].rgbRed =
                vvmhw[iVM].rgrgb[i].rgbGreen =
                vvmhw[iVM].rgrgb[i].rgbBlue = 0xFF;
                vvmhw[iVM].rgrgb[i].rgbReserved = 0;
                }
            SetDIBColorTable(vrgvmi[iVM].hdcMem, 0, 256, vvmhw[iVM].rgrgb);
            }
        else
            SetDIBColorTable(vrgvmi[iVM].hdcMem, 0, 2,   vvmhw[iVM].rgrgb);

        return TRUE;
    }

	// we are 8 bit colour
	else if (rgvm[iVM].pvmi->planes == 8)
	{
		// but are on a B&W monitor that does shades of grey
		if (rgvm[iVM].bfMon == monGreyTV)

			for (i = 0; i < 256; i++)
            {
				vvmhw[iVM].rgrgb[i].rgbRed =
				vvmhw[iVM].rgrgb[i].rgbGreen =
				vvmhw[iVM].rgrgb[i].rgbBlue =
				 (rgvm[iVM].pvmi->rgbRainbow[i*3] + (rgvm[iVM].pvmi->rgbRainbow[i*3] >> 2)) +
				 (rgvm[iVM].pvmi->rgbRainbow[i*3+1]  + (rgvm[iVM].pvmi->rgbRainbow[i*3+1] >> 2)) +
				 (rgvm[iVM].pvmi->rgbRainbow[i*3+2]  + (rgvm[iVM].pvmi->rgbRainbow[i*3+2] >> 2));
				vvmhw[iVM].rgrgb[i].rgbReserved = 0;
        }
        else for (i = 0; i < 256; i++)
        {
			vvmhw[iVM].rgrgb[i].rgbRed = (rgvm[iVM].pvmi->rgbRainbow[i*3] << 2) | (rgvm[iVM].pvmi->rgbRainbow[i*3] >> 5);
			vvmhw[iVM].rgrgb[i].rgbGreen = (rgvm[iVM].pvmi->rgbRainbow[i*3+1] << 2) | (rgvm[iVM].pvmi->rgbRainbow[i*3+1] >> 5);
			vvmhw[iVM].rgrgb[i].rgbBlue = (rgvm[iVM].pvmi->rgbRainbow[i*3+2] << 2) | (rgvm[iVM].pvmi->rgbRainbow[i*3+2] >> 5);
			vvmhw[iVM].rgrgb[i].rgbReserved = 0;

#if 0
            //
            // test code to dump the 256-colour palette as YUV values
            //

            {
            int Y, U, V;
            int R = vvmhw[iVM].rgrgb[i].rgbRed;
            int G = vvmhw[iVM].rgrgb[i].rgbGreen;
            int B = vvmhw[iVM].rgrgb[i].rgbBlue;

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
			;//FChangePaletteEntries(0, 256, vvmhw[iVM].rgrgb);
        else
            SetDIBColorTable(vrgvmi[iVM].hdcMem, 0, 256, vvmhw[iVM].rgrgb);
        return TRUE;
    }

	// 16 colour VM
	else if (rgvm[iVM].pvmi->planes == 4)
	{
		if (vi.fInDirectXMode)
			;// FChangePaletteEntries(10, 16, vvmhw[iVM].rgrgb);
		else
			SetDIBColorTable(vrgvmi[iVM].hdcMem, 10, 16, vvmhw[iVM].rgrgb);
	}
	
	return TRUE;
}

/****************************************************************************

    FUNCTION: CreateNewBitmap

    PURPOSE:  Create a bitmap of the appropriate size and colors

    COMMENTS: Updates globals v

****************************************************************************/

// This creates the HDC that holds the screen image for the instance
// We create one for every instance right away, so we can tile them!
//
BOOL CreateNewBitmap(int iVM)
{
    RECT rect;
    ULONG x;
    ULONG y;

	// Do we want to force mono? Otherwise use this VM's # of colours
	if (vvmhw[iVM].fMono)
		vvmhw[iVM].planes = 1;
	else
		vvmhw[iVM].planes = rgvm[iVM].pvmi->planes;

	vvmhw[iVM].xpix = rgvm[iVM].pvmi->uScreenX;
	vvmhw[iVM].ypix = rgvm[iVM].pvmi->uScreenY;

	//PrintScreenStats();
	//printf("Entering CreateNewBitmap: new x,y = %4d,%4d, fFull=%d, fZoom=%d\n", vvmhw[iVM].xpix, vvmhw[iVM].ypix, v.fFullScreen, v.fZoomColor);

#if defined(ATARIST) || defined(SOFTMAC)
    // Interlock to make sure video thread is not touching memory
    while (0 != InterlockedCompareExchange(&vi.fVideoEnabled, FALSE, TRUE))
        Sleep(10);

    MarkAllPagesClean();
#endif

    UninitDrawing(!v.fFullScreen);
    vi.fInDirectXMode = FALSE;

    if (vrgvmi[iVM].hbmOld)
        {
        SelectObject(vrgvmi[iVM].hdcMem, vrgvmi[iVM].hbmOld);
        vrgvmi[iVM].hbmOld = NULL;
        }

    if (vrgvmi[iVM].hbm)
        {
        DeleteObject(vrgvmi[iVM].hbm);
        vrgvmi[iVM].hbm = NULL;
        }

    if (vrgvmi[iVM].hdcMem)
        {
        DeleteDC(vrgvmi[iVM].hdcMem);
        vrgvmi[iVM].hdcMem = NULL;
        }

    // initialize to some initial state, will be set further below

    vi.sx = vvmhw[iVM].xpix;
    vi.sy = vvmhw[iVM].ypix;
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

        if (vvmhw.xpix >= 640 && vvmhw.ypix >= 400)
            {
            sx = vvmhw.xpix;
            sy = vvmhw.ypix;
            }

        if (FIsMac(vmCur.bfHW) && vvmhw.ypix == 342)
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

        int scan_double = ((vvmhw[iVM].xpix) >= (vvmhw[iVM].ypix * 3)) ? 2 : 1;

#if 0
        if (v.fZoomColor || v.fFullScreen)
        {
        // now keep increasing the zoom until it doesn't fit

        while ((vvmhw[iVM].xpix * (vi.fXscale + 1) <= x) && (vvmhw[iVM].ypix * scan_double * (vi.fYscale + 1) <= y))
            {
            vi.fXscale++;
            vi.fYscale++;
            }
        }
#endif

        vi.fYscale *= (char)scan_double; // won't overflow?

        vi.sx = vvmhw[iVM].xpix * vi.fXscale;
        vi.sy = vvmhw[iVM].ypix * vi.fYscale;
    }

	// true monochrome?
    if (vvmhw[iVM].fMono && !v.fNoMono)
    {
        vvmhw[iVM].bmiHeader.biSize = sizeof(BITMAPINFOHEADER) /* +sizeof(RGBQUAD)*2 */;
        vvmhw[iVM].bmiHeader.biWidth = vvmhw[iVM].xpix;
        vvmhw[iVM].bmiHeader.biHeight = -vvmhw[iVM].ypix;
        vvmhw[iVM].bmiHeader.biPlanes = 1;
        vvmhw[iVM].bmiHeader.biBitCount = 1;
        vvmhw[iVM].bmiHeader.biCompression = BI_RGB;
        vvmhw[iVM].bmiHeader.biSizeImage = 0;
        vvmhw[iVM].bmiHeader.biXPelsPerMeter = 2834;
        vvmhw[iVM].bmiHeader.biYPelsPerMeter = 2834;
        vvmhw[iVM].bmiHeader.biClrUsed = 0*2;
        vvmhw[iVM].bmiHeader.biClrImportant = 0*2;
        vvmhw[iVM].lrgb[0] = 0;
        vvmhw[iVM].lrgb[1] = 0x00FFFFFF;

        vi.cbScan = (ULONG)(min(x, (ULONG)vvmhw[iVM].xpix) / 8);
    }
    else
    {
        // create a 256 color bitmap buffer

        vvmhw[iVM].bmiHeader.biSize = sizeof(BITMAPINFOHEADER) /* +sizeof(RGBQUAD)*2 */;
        vvmhw[iVM].bmiHeader.biWidth = vvmhw[iVM].xpix;
        vvmhw[iVM].bmiHeader.biHeight = -vvmhw[iVM].ypix;
        vvmhw[iVM].bmiHeader.biPlanes = 1;
        vvmhw[iVM].bmiHeader.biBitCount = 8;
        vvmhw[iVM].bmiHeader.biCompression = BI_RGB;
        vvmhw[iVM].bmiHeader.biSizeImage = 0;
        vvmhw[iVM].bmiHeader.biXPelsPerMeter = 2834;
        vvmhw[iVM].bmiHeader.biYPelsPerMeter = 2834;
		vvmhw[iVM].bmiHeader.biClrUsed = (rgvm[iVM].pvmi->planes == 8) ? 256 : 26; // !!! 4 bit colour gets 10 extra colours?
        vvmhw[iVM].bmiHeader.biClrImportant = 0;

        vi.cbScan = (ULONG)(min(x, (ULONG)vvmhw[iVM].xpix));
        }

    // Create offscreen bitmap

    vrgvmi[iVM].hbm = CreateDIBSection(vi.hdc, (CONST BITMAPINFO *)&vvmhw[iVM].bmiHeader,
        DIB_RGB_COLORS, &(vrgvmi[iVM].pvBits), NULL, 0);
    vrgvmi[iVM].hdcMem = CreateCompatibleDC(vi.hdc);

	if (vrgvmi[iVM].hbm == NULL || vrgvmi[iVM].hdcMem == NULL)
		return FALSE;

    vrgvmi[iVM].hbmOld = SelectObject(vrgvmi[iVM].hdcMem, vrgvmi[iVM].hbm);

    SetBitmapColors(iVM);
    //FCreateOurPalette();

// PrintScreenStats(); printf("Entering CreateNewBitmap: leaving critical section\n");

#if 0
    v.rectWinPos = rectSav; // restore saved window position

    SetWindowPos(vi.hWnd, HWND_NOTOPMOST, v.rectWinPos.left, v.rectWinPos.top, 0, 0,
        SWP_NOACTIVATE | SWP_NOSIZE);
#endif

//        printf("CreateNewBitmap setting windows position to x = %d, y = %d, r = %d, b = %d\n",
//             v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right, v.rectWinPos.bottom);

    GetClientRect(vi.hWnd, &rect);

    if ((rect.right < vvmhw[iVM].xpix) ||  (rect.bottom < vvmhw[iVM].ypix))
    {
        // This is in case WM_MIXMAXINFO is not issued on too small a window

        SetWindowPos(vi.hWnd, HWND_NOTOPMOST, 0, 0, vvmhw[iVM].xpix, vvmhw[iVM].ypix,
            SWP_NOACTIVATE | SWP_NOMOVE);
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

    GetClientRect(vi.hWnd, &rect);

    //printf("cnbm: client rect = %d, %d, %d, %d\n", rect.top, rect.left, rect.right, rect.bottom);

    vi.sx = rect.right;
    vi.sy = rect.bottom;

    vi.dx = (vi.sx - (vvmhw[iVM].xpix * vi.fXscale)) / 2;
    vi.dy = (vi.sy - (vvmhw[iVM].ypix * vi.fYscale)) / 2;

    // we may have moved the window, so we need to move the mouse capture rectangle too

    if (rgvm[iVM].pvmi->fUsesMouse && vi.fGEMMouse)
        {
        ShowWindowsMouse();
        ShowGEMMouse();
        }

	// PrintScreenStats(); printf("Entering CreateNewBitmap: after window adjustments\n");

    // DisplayStatus();
    // Sleep(20);   // give other windows a chance to redraw

    InvalidateRect(vi.hWnd, NULL, 0);

#if defined(ATARIST) || defined(SOFTMAC)
    MarkAllPagesDirty();
	UpdateOverlay();
#endif

    vi.fVideoEnabled = TRUE;

// PrintScreenStats(); printf("Entering CreateNewBitmap: done!\n");
    return TRUE;
}

//
// Return TRUE if we can successfully change to a different monitor
// Basically, this switches between COLOUR and B&W
//
BOOL FToggleMonitor(int iVM)
{
    ULONG bfSav = rgvm[iVM].bfMon;
    int i;

    // loop through all the bits in wfMon trying to find a different bit

    for(;;)
    {
        rgvm[iVM].bfMon <<= 1;

        if (rgvm[iVM].bfMon == 0)
            rgvm[iVM].bfMon = 1;

        if (rgvm[iVM].bfMon & rgvm[iVM].pvmi->wfMon)
            break;
    }

    // if it's the same bit, no other monitors

    if (bfSav == rgvm[iVM].bfMon)
        return FALSE;

    // We need to guard the video critical section since
    // we are modifying the global vvmhw.fMono which both
    // the pallete and redraw routines depend on.

    vvmhw[iVM].rgrgb[0].rgbRed = 255;
    vvmhw[iVM].rgrgb[0].rgbGreen = 255;
    vvmhw[iVM].rgrgb[0].rgbBlue = 255;
    vvmhw[iVM].rgrgb[0].rgbReserved = 0;

    for (i = 1; i < 256; i++)
    {
        vvmhw[iVM].rgrgb[i].rgbRed = 0;
        vvmhw[iVM].rgrgb[i].rgbGreen = 0;
        vvmhw[iVM].rgrgb[i].rgbBlue = 0;
        vvmhw[iVM].rgrgb[i].rgbReserved = 0;
    }

    vvmhw[iVM].fMono = FMonoFromBf(rgvm[iVM].bfMon);

    SetBitmapColors(v.iVM);
//    FCreateOurPalette();

    InvalidateRect(vi.hWnd, NULL, 0);

#if defined(ATARIST) || defined(SOFTMAC)
	MarkAllPagesDirty();
#endif

    return TRUE;
}

//
// Render the DIB section to the current window's device context
//
void RenderBitmap()
{
	RECT rect;
	int iVM = v.iVM;

	GetClientRect(vi.hWnd, &rect);

	if (v.cVM == 0)
	{
		BitBlt(vi.hdc, 0, 0, rect.right, rect.bottom, NULL, 0, 0, BLACKNESS);
		return;
	}

#if !defined(NDEBUG)
	//  printf("rnb: rect = %d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
	//  PrintScreenStats();
#endif

	if (v.fTiling)
	{
		// Tiling

		// find a valid VM we can use
		iVM = nFirstTile; // we're asked to start tiling at this point
		while (iVM < 0 || rgvm[iVM].fValidVM == FALSE)
			iVM++;

		int x, y, fDone = iVM;
		BOOL fBlack = FALSE;

		// !!! tile sizes are arbitrarily based off the first VM, what about when sizes are mixed?
		int nx1 = rect.right / vvmhw[iVM].xpix; // how many fit across entirely?		
		int nx = (rect.right * 10 / vvmhw[iVM].xpix + 5) / 10; // how many fit across (if 1/2 showing counts)?

		// black out the area we'll never draw to
		if (nx == nx1)
			BitBlt(vi.hdc, nx * vvmhw[iVM].xpix, 0, rect.right - (vvmhw[iVM].xpix * nx), rect.bottom, NULL, 0, 0, BLACKNESS);

		for (y = rect.top + sWheelOffset; y < rect.bottom; y += vvmhw[iVM].ypix /* * vi.fYscale*/)
		{
			for (x = 0; x < nx * vvmhw[iVM].xpix; x += vvmhw[iVM].xpix /* * vi.fXscale*/)
			{
				// Tiled mode does not stretch, it needs to be FAST. Don't draw tiles off the top of the screen
				if (y + vvmhw[iVM].ypix > 0 && !fBlack)
				{
					if (sVM == (int)iVM)
					{
						// border around the one we're hovering over
						int xw = vvmhw[iVM].xpix, yw = vvmhw[iVM].ypix;
						BitBlt(vi.hdc, x, y, xw, 5, vrgvmi[iVM].hdcMem, 0, 0, WHITENESS);
						BitBlt(vi.hdc, x, y + 5, 5, yw - 10, vrgvmi[iVM].hdcMem, 0, 0, WHITENESS);
						BitBlt(vi.hdc, x + xw - 5, y + 5, 5, yw - 10, vrgvmi[iVM].hdcMem, 0, 0, WHITENESS);
						BitBlt(vi.hdc, x, y + yw - 5, xw, 5, vrgvmi[iVM].hdcMem, 0, 0, WHITENESS);
						BitBlt(vi.hdc, x + 5, y + 5, xw - 10, yw - 10, vrgvmi[iVM].hdcMem, 5, 5, SRCCOPY);
					}
					else
						BitBlt(vi.hdc, x, y, vvmhw[iVM].xpix, vvmhw[iVM].ypix, vrgvmi[iVM].hdcMem, 0, 0, SRCCOPY);
				}
				else if (fBlack)
					BitBlt(vi.hdc, x, y, vvmhw[iVM].xpix, vvmhw[iVM].ypix, vrgvmi[iVM].hdcMem, 0, 0, BLACKNESS);

				//StretchBlt(vi.hdc, x, y,
				//	(vvmhw[iVM].xpix * vi.fXscale), (vvmhw[iVM].ypix * vi.fYscale),
				//	vrgvmi[iVM].hdcMem, 0, 0, vvmhw[iVM].xpix, vvmhw[iVM].ypix, SRCCOPY);

				// advance to the next valid bitmap
				do
				{
					iVM = (iVM + 1) % MAX_VM;
				} while (vrgvmi[iVM].hdcMem == NULL);

				// we've painted them all, now just black for the rest
				if (fDone == (int)iVM)
					fBlack = TRUE;
			}
		}
	}
	else if (v.fZoomColor)
	{
		// Smart Scaling

		StretchBlt(vi.hdc, rect.left, rect.top, rect.right, rect.bottom,
			vrgvmi[v.iVM].hdcMem, 0, 0, vvmhw[iVM].xpix, vvmhw[iVM].ypix, SRCCOPY);
	}
	else
	{
		// Integer scaling

		int x, y, scale;

		for (scale = 16; scale > 1; scale--)
		{
			x = rect.right - (vvmhw[iVM].xpix * vi.fXscale * scale);
			y = rect.bottom - (vvmhw[iVM].ypix * vi.fYscale * scale);

			if ((x >= 0) && (y >= 0))
				break;
		}

		// Repeat for the case of scale==1 when rectangle doesn't fit

		x = rect.right - (vvmhw[iVM].xpix * vi.fXscale * scale);
		y = rect.bottom - (vvmhw[iVM].ypix * vi.fYscale * scale);

		// Why did we not used to have to do this? Only necessary when coming out of zoom?
		StretchBlt(vi.hdc, 0, 0, x / 2, rect.bottom, vrgvmi[v.iVM].hdcMem, 0, 0, 0, 0, BLACKNESS);
		StretchBlt(vi.hdc, x / 2, 0, x / 2 + (vvmhw[iVM].xpix * vi.fXscale * scale), y / 2, vrgvmi[v.iVM].hdcMem, 0, 0, 0, 0, BLACKNESS);
		StretchBlt(vi.hdc, x / 2, y / 2 + (vvmhw[iVM].ypix * vi.fYscale * scale), x / 2 + (vvmhw[iVM].xpix * vi.fXscale * scale), rect.bottom,
			vrgvmi[v.iVM].hdcMem, 0, 0, 0, 0, BLACKNESS);
		StretchBlt(vi.hdc, x / 2 + (vvmhw[iVM].xpix * vi.fXscale * scale), 0, rect.right, rect.bottom, vrgvmi[v.iVM].hdcMem, 0, 0, 0, 0, BLACKNESS);

		StretchBlt(vi.hdc, x / 2, y / 2,
			(vvmhw[iVM].xpix * vi.fXscale * scale), (vvmhw[iVM].ypix * vi.fYscale * scale),
			vrgvmi[v.iVM].hdcMem, 0, 0, vvmhw[iVM].xpix, vvmhw[iVM].ypix, SRCCOPY);
	}

#if 0 // future cloud stuff, too slow for now
	if (vi.pbFrameBuf && vvmi.pvBits)
	{
		int x, y;

		for (y = 0; y < vvmhw[iVM].ypix; y++)
		{
			BYTE *pb = ((BYTE *)vvmi.pvBits) + y * vi.cbScan;

			for (x = 0; x < vvmhw[iVM].xpix; x++)
			{
				vi.pbFrameBuf[y*vvmhw[iVM].xpix + x] = *pb++;
			}
		}
	}
#endif
}

//
// Change which instance is current and running.
// Start trying at the instance passed to us, and keep looking until we find a valid one
// -1 means go to the previous one
//

void SelectInstance(int iVM)
{
	// there better be some valid ones loaded
	if (v.cVM == 0)
	{
		v.iVM = (unsigned)-1;	// current instance invalid
		return;
	}

	// which way are we looking?
	int dir = 1;
	if (iVM == -1)
	{
		dir = -1;
		iVM = v.iVM - 1;
	}
	
	if (iVM >= MAX_VM)
		iVM = 0;

	int old = iVM;

	while (!rgvm[iVM].fValidVM)
	{
		iVM = (iVM + dir) % MAX_VM;
		if (iVM == old) break;
	}

	assert(rgvm[iVM].fValidVM);

	// This is what you need to do to switch to an instance
	// only the main GEM UI cares about the concept of a current instance now! Everything else is thread safe
	// with no concept of "current", but taking an instance as an argument

	v.iVM = iVM;					// current VM #
	
	// a menu or title bar might need to change.
	if (!v.fTiling)
		FixAllMenus();

	// Enforce minimum window size for this type of VM
	SetWindowPos(vi.hWnd, NULL, v.rectWinPos.left, v.rectWinPos.top, v.rectWinPos.right - v.rectWinPos.left,
		v.rectWinPos.bottom - v.rectWinPos.top, v.swWindowState);

    return;
}

//
// Reboot a particular VM.
// !!! I'm not convinced any of this is necessary, but for now I'll reboot by calling this and letting it call FColdbootVM
//
BOOL ColdStart(int iVM)
{
	printf("COLDBOOT (%d)\n", iVM);

	if (rgvm[iVM].pvmi->fUsesMouse)
	{
		ShowWindowsMouse();
		SetWindowsMouse(0);
	}

#if defined (ATARIST) || defined (SOFTMAC)
    // User may have just mucked with Properties

    vvmhw[iVM].fMono = FMonoFromBf(rgvm[iVM].bfMon);

    // make sure RAM setting not corrupted!
    // if it is, select the smaller RAM setting of the VM

    if ((rgvm[iVM].bfRAM & rgvm[iVM].pvmi->wfRAM) == 0)
        rgvm[iVM].bfRAM = BfFromWfI(rgvm[iVM].pvmi->wfRAM, 0);

    vi.cbRAM[0] = CbRamFromBf(rgvm[iVM].bfRAM);
    vi.eaROM[0] = v.rgosinfo[rgvm[iVM].iOS].eaOS;
    vi.cbROM[0] = v.rgosinfo[rgvm[iVM].iOS].cbOS;

    vrgvmi[iVM].keyhead = vrgvmi[iVM].keytail = 0;
    vi.fVMCapture = FALSE;

	// we no longer require cold start to happen after CreateNewBitmap, so we might not have hdc's now.
	//PatBlt(vi.hdc, 0, 0, 4096, 2048, BLACKNESS);
    //PatBlt(vrgvmi[iVM].hdcMem, 0, 0, 4096, 2048, BLACKNESS);
#endif

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

#if defined(ATARIST) || defined(SOFTMAC)
    // reboots faster if video thread is disabled
    MarkAllPagesClean();
#endif

    BOOL f = FColdbootVM(iVM);

#if !defined(NDEBUG) && 0
    fDebug++;
    printf("RAM size = %dK\n", vi.cbRAM[0] / 1024);
    printf("ROM size = %dK\n", vi.cbROM[0] / 1024);
    printf("ROM addr = $%08X\n", vi.eaROM[0]);

    switch(vmPeekL(iVM, vi.eaROM[0]))
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

    printf("ROM sig  = $%08X\n", vmPeekL(iVM, vi.eaROM[0]));
    printf("ROM ver  = $%04X\n", vmPeekW(iVM, vi.eaROM[0]+8));
    printf("ROM subv = $%04X\n", vmPeekW(iVM, vi.eaROM[0]+18));
    
	printf("CPU type = %s\n", PchFromBfRgSz(vi.fFake040 ? cpu68040 : rgvm[iVM].bfCPU, rgszCPU));
//    printf("CPU real = %s\n", (long)PchFromBfRgSz(rgvm[iVM].bfCPU, rgszCPU));
    fDebug--;

    if (v.fDebugMode)
    {
        vi.fInDebugger = TRUE;
		vi.fDebugBreak = TRUE;
	}
#endif

	return f;
}


// which tile is under the mouse right now? (Or -1 if none)
//
int GetTileFromPos(int xPos, int yPos)
{
	RECT rect;
	
	if (v.cVM == 0)
		return -1;

	GetClientRect(vi.hWnd, &rect);

	int x, y;
	
	int iVM = nFirstTile;
	while (iVM < 0 || !rgvm[iVM].fValidVM)
		iVM = ((iVM + 1) % MAX_VM);
	int fDone = iVM;

	int nx = (rect.right * 10 / vvmhw[iVM].xpix + 5) / 10; // how many fit across (1/2 showing counts)

	for (y = rect.top + sWheelOffset; y < rect.bottom; y += vvmhw[iVM].ypix * vi.fYscale)
	{
		for (x = 0; x < nx * vvmhw[iVM].xpix; x += vvmhw[iVM].xpix /* * vi.fXscale*/)
		{
			if ((xPos >= x) && (xPos < x + vvmhw[iVM].xpix * vi.fXscale) &&
				(yPos >= y) && (yPos < y + vvmhw[iVM].ypix * vi.fYscale))
			{
				return iVM;
			}

			// advance to the next valid bitmap
			do
			{
				iVM = (iVM + 1) % MAX_VM;
			} while (vrgvmi[iVM].hdcMem == NULL);

			// we've reached the end of the tiles
			if (iVM == fDone)
			{
				y = rect.bottom;	// double break
				break;
			}
		}
	}
	return -1;
}

// Scroll the tiles to the point specified by sWheelOffset
//
void ScrollTiles()
{
	RECT rect;
	GetClientRect(vi.hWnd, &rect);

	int iVM = v.iVM;
	while (iVM <= 0 || rgvm[iVM].fValidVM == FALSE)
		iVM++;

	int nx = (rect.right * 10 / vvmhw[iVM].xpix + 5) / 10; // how many fit across (1/2 showing counts)
	int bottom = (v.cVM * 100 / nx - 1) / 100 + 1; // how many rows will it take to show them all?
	bottom = bottom * vvmhw[iVM].ypix - rect.bottom;	// how many pixels past the bottom of the screen is that?

	if (bottom < 0)
		bottom = 0;
	if (sWheelOffset > 0)
		sWheelOffset = 0;
	if (sWheelOffset < bottom * -1)
		sWheelOffset = bottom * -1;

	// now where would our mouse be after this scroll?
	POINT pt;
	if (GetCursorPos(&pt))
		if (ScreenToClient(vi.hWnd, &pt))
		{
			int s = GetTileFromPos(pt.x, pt.y);
			if (s != sVM)
			{
				sVM = s;
				FixAllMenus();
			}
		}
}

// Extract all the DOS files from this disk image and save them to the PC hard drive
//
BOOL SaveATARIDOS(int inst, int drive)
{
	if (inst == -1)
		return TRUE;

	// Any changes you've made to the disk are immediately saved, thankfully
	//FUnmountDiskVM(v.iVM, 0);
	//FMountDiskVM(v.iVM, 0);

	char szDir[MAX_PATH];
	szDir[0] = 0;
	DISKINFO *pdi = PdiOpenDisk(rgvm[inst].rgvd[drive].dt, (long)rgvm[inst].rgvd[drive].sz, DI_READONLY);

	// oops, something wrong with the disk image file
	if (!pdi)
		return TRUE;	// don't show an error, there's no actual disk image

	CntReadDiskDirectory(pdi, szDir, NULL); // not sure how this returns an error, 0 is fine

	BOOL fB, fh = TRUE;

	// save all the files in binary, then text format
	// !!! how to auto detect?
	for (int ij = 0; ij < pdi->cfd * 2; ij++)
	{

		szDir[0] = 0;
		strcat(szDir, rgvm[inst].rgvd[drive].sz);
		strcat(szDir, "_Files");

		// Create a directory named after the disk image for all of its files
		if (ij == 0)
			fB = CreateDirectory(szDir, NULL); // error OK, it may already exist

		strcat(szDir, "\\");					// add '\'
		strcat(szDir, pdi->pfd[ij % pdi->cfd].cFileName);	// add the filename
		if (ij >= pdi->cfd)
			strcat(szDir, ".txt");	// 2nd time around, save everything as text

									// get file size
		ULONG cbSize = CbReadFileContents(pdi, NULL, &pdi->pfd[ij % pdi->cfd]);

		if (cbSize > 0)
		{
			unsigned char *szFile = malloc(cbSize);
			if (!szFile)
			{
				fh = FALSE;
				break;
			}

			cbSize = CbReadFileContents(pdi, szFile, &pdi->pfd[ij % pdi->cfd]);

			int h = _open(szDir, _O_BINARY | _O_RDONLY);
			if (h != -1)
			{
				_close(h);
				char ods[MAX_PATH];
				sprintf(ods, "\"%s\" exists. Overwrite?", szDir);
				int h2 = MessageBox(vi.hWnd, ods, "File Exists", MB_YESNO);
				if (h2 == IDNO)
					continue;
			}
			h = _open(szDir, _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC, _S_IREAD | _S_IWRITE);

			// 2nd time around, convert the buffer to text (ATASCII -> ASCII)
			if (h != -1 && ij >= pdi->cfd)
			{
				// a buffer for the ASCII (maximum 2 characters per character)
				unsigned char *szA = malloc(cbSize * 2);
				unsigned char *szA1 = szA;
				int cbA = 0;
				if (!szA)
				{
					fh = FALSE;
					_close(h);
					free(szFile);
					break;
				}

				for (unsigned int ind = 0; ind < cbSize; ind++)
				{
					// RETURN = CR/LF
					if (szFile[ind] == 0x9b)
					{
						*szA++ = 0x0d;
						*szA++ = 0x0a;
						cbA += 2;
					}
					// TAB
					else if (szFile[ind] == 0x7f)
					{
						*szA++ = 0x09;
						cbA++;
					}
					// BACKSPACE
					else if (szFile[ind] == 0x7e)
					{
						*szA++ = 0x08;
						cbA++;
					}
					else
					{
						*szA++ = szFile[ind];
						cbA++;
					}
				}
				fB = _write(h, szA1, cbA);

				// something in the process failed
				if (fB != cbA)
					fh = FALSE;

				free(szA1);
			}
			else if (h != -1)
			{
				fB = _write(h, szFile, cbSize);

				// something in the process failed
				if (fB != (BOOL)cbSize)
					fh = FALSE;
			}

			_close(h);
			free(szFile);

			// something in the process failed
			if (h == -1)
				fh = FALSE;
		}
	}

	fB = CloseDiskPdi(pdi);

	return fh;
}


void ShowAbout()
{
	char rgch[1120], rgch2[64], rgchVer[32];

	// !!! Windows lies and says we're 8 because we are not "manifested" for 10
	if (IsWindows10OrGreater())
		strcpy(rgchVer, "Windows 10 or Greater");
	else if (IsWindows8Point1OrGreater())
		strcpy(rgchVer, "Windows 8.1");
	else if (IsWindows8OrGreater())
		strcpy(rgchVer, "Windows 8");
	else if (IsWindows7OrGreater())
		strcpy(rgchVer, "Windows 7");
	else if (IsWindowsXPOrGreater())
		strcpy(rgchVer, "Windows XP");
	else
		strcpy(rgchVer, "Windows archaic");

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

//		"Windows version: %d.%02d (build %d)\n"
		"Windows platform: %s\n"
		,
		vi.szAppName,
		__DATE__,
		sizeof(void *) * 8,
#if defined(_M_AMD64)
		"x64",
#elif defined(_M_IX86)
		"x86",
#elif defined(_M_ARM)
		"ARM",
#else
		"",		// !!! ARM64
#endif
//		oi.dwMajorVersion, oi.dwMinorVersion,
//		oi.dwBuildNumber & 65535,
		rgchVer
		);

	MessageBox(GetFocus(), rgch, rgch2, MB_OK);
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

	// which is the "current" VM? (the tiled one with focus, or the main one when not tiled) or -1 if tiled and nothing in focus
	v.iVM = (v.fTiling && sVM >= 0) ? sVM : (v.fTiling ? -1 : v.iVM);	// use the active tile if there is one
	
    switch (message)
    {
    
	case WM_CREATE:
        vi.hdc = GetDC(hWnd);
		SetTextAlign(vi.hdc, TA_NOUPDATECP);
        SetTextColor(vi.hdc, RGB(0,255,0));  // set green text
        SetBkMode(vi.hdc, TRANSPARENT);

    	// now that we have a DC make all the buffers for the screens of all the instances
		for (int ii = 0; ii < MAX_VM; ii++)
		{
			if (rgvm[ii].fValidVM)
			{
				BOOL fC = CreateNewBitmap(ii);
				if (!fC)
					DeleteVM(ii);
			}
		}

		// if we were saved in fullscreen mode, then actually go into fullscreen
		if (v.fFullScreen) {
			v.fFullScreen = FALSE;	// so we think we aren't in it yet
			
			// vi.hWnd won't be set until this WM_CREATE finishes
			// and other stuff isn't set up yet, so don't Send the message immediately, Post it.
			PostMessage(hWnd, WM_COMMAND, IDM_FULLSCREEN, 0);
		}

		// this is slow, so only do it once ever
		InitSound();

        //TrayMessage(hWnd, NIM_ADD, 0, LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), NULL);

        break;

    case WM_MOVE:

#if !defined(NDEBUG)
        //printf("WM_MOVE before: x = %d, y = %d\n", v.rectWinPos.left, v.rectWinPos.top);
#endif

		// MOVE comes before the SIZE so we don't know we're being maximized, so make sure we are >(0,0)
		// remembers our last restored size
		RECT rect;
		GetWindowRect(hWnd, &rect);
        if (!v.fFullScreen)
          if ((vvmhw[v.iVM].xpix * vi.fXscale) < GetSystemMetrics(SM_CXSCREEN))
          if ((vvmhw[v.iVM].ypix * vi.fYscale) < GetSystemMetrics(SM_CYSCREEN))
          if (rect.left > 0 && rect.top > 0)
				GetWindowRect(hWnd, (LPRECT)&v.rectWinPos);

#if !defined(NDEBUG)
        //printf("WM_MOVE after:  x = %d, y = %d\n", v.rectWinPos.left, v.rectWinPos.top);
#endif

        break;

    case WM_SIZE:

#if !defined(NDEBUG)
        //printf("WM_SIZE x = %d, y = %d\n", LOWORD(lParam), HIWORD(lParam));
#endif

		// This code hasn't been enabled in years
        if (vi.fInDirectXMode)
          {
            RECT Rect;
			SetRect(&Rect, 0, GetSystemMetrics(SM_CYCAPTION), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            AdjustWindowRectEx(&Rect, WS_POPUP | WS_CAPTION, FALSE, 0);
            SetWindowPos(vi.hWnd, NULL, Rect.left, Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, SWP_NOACTIVATE | SWP_NOZORDER);
          }

		// remember if we're maximized, minimized or normal
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(vi.hWnd, &wp);
		v.swWindowState = wp.showCmd;

		// take note of our window size (if not full screen nor minimized)
		if (!v.fFullScreen && v.swWindowState == SW_SHOWNORMAL)
			GetWindowRect(hWnd, (LPRECT)&v.rectWinPos);
		
		// If we're bigger, so many tiles might now fit offscreen that all the visible ones vanish
		sWheelOffset = 0;

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

		for (int ii = 0; ii < MAX_VM; ii++)
		{
			if (rgvm[ii].fValidVM)
				CreateNewBitmap(ii);	// fix every instance's bitmap
		}
		//for some reason, in fullscreen, we need this extra push
		if (v.fFullScreen)
		{
			ShowWindow(vi.hWnd, SW_RESTORE);
			ShowWindow(vi.hWnd, SW_MAXIMIZE);
		}
        break;

#if !defined(NDEBUG)
    case WM_SIZING:
        {
        //LPRECT lprect = (LPRECT)lParam;
        //BYTE ratio = vi.fYscale / vi.fXscale;
        //int thickX = GetSystemMetrics(SM_CXSIZEFRAME) * 2;
        //int thickY = GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);

        //    RECT rect;
        //    GetClientRect(hWnd, &rect);

        //printf("WM_SIZING: lprect = %p\n", lprect);
        //printf("WM_SIZING: client rect = %d, %d\n", rect.right, rect.bottom);
        //printf("WM_SIZING #1: x = %d, y = %d, w = %d, h = %d\n",
        //    lprect->left, lprect->top, lprect->right - lprect->left, lprect->bottom - lprect->top);

break;
#if 0
        if ((vvmhw[v.iVM].xpix == 0) || (vvmhw[v.iVM].ypix == 0))
            return 0;

        vi.fXscale = max(1, (lprect->right - lprect->left - thickX) /  vvmhw[v.iVM].xpix);
        vi.fYscale = max(1, (lprect->bottom - lprect->top - thickY) /  vvmhw[v.iVM].ypix);

        vi.fYscale = vi.fXscale * ratio;

        if (1 /* fSnapping */)
            {
            lprect->right = lprect->left + vvmhw[v.iVM].xpix * vi.fXscale + thickX;
            lprect->bottom = lprect->top + vvmhw[v.iVM].ypix * vi.fYscale + thickY;
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

		// !!! shouldn't apply to tiled mode?
		// !!! Switching to a completely different VM needs to trigger this code again

		if (v.iVM >= 0)
		{
			RECT rc1, rc2;
			GetWindowRect(hWnd, &rc1);
			GetClientRect(hWnd, &rc2);

			// calculate the non-client dimensions this window needs
			int thickX = rc1.right - rc1.left - (rc2.right - rc2.left); // (SM_CXSIZEFRAME) * 2; is wrong
			int thickY = rc1.bottom - rc1.top - (rc2.bottom - rc2.top); // (SM_CYSIZEFRAME) * 2 + (SM_CYCAPTION); also wrong

			int scaleX = 1, scaleY = ((vvmhw[v.iVM].xpix) >= (vvmhw[v.iVM].ypix * 3)) ? 2 : 1;

#if !defined(NDEBUG) && 0
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

			// Show a double sized screen of anything < 640x480
			if (rgvm[v.iVM].pvmi->uScreenX < 640 || rgvm[v.iVM].pvmi->uScreenY < 480)
			{
				lpmm->ptMinTrackSize.x = max(vvmhw[v.iVM].xpix * scaleX, (int)rgvm[v.iVM].pvmi->uScreenX * 2) + thickX;
				lpmm->ptMinTrackSize.y = max(vvmhw[v.iVM].ypix * scaleY, (int)rgvm[v.iVM].pvmi->uScreenY * 2) + thickY;
			}
			else
			{
				lpmm->ptMinTrackSize.x = max(vvmhw[v.iVM].xpix * scaleX, (int)rgvm[v.iVM].pvmi->uScreenX) + thickX;
				lpmm->ptMinTrackSize.y = max(vvmhw[v.iVM].ypix * scaleY, (int)rgvm[v.iVM].pvmi->uScreenY) + thickY;
			}
		}
		// default to 640x480 minimum if state-less
		else
		{
			lpmm->ptMinTrackSize.x = 640;
			lpmm->ptMinTrackSize.y = 480;
		}
		break;

#if 0
        if ((vvmhw[v.iVM].xpix == 0) || (vvmhw[v.iVM].ypix == 0))
            return 0;

        if (v.fFullScreen && vi.fHaveFocus)
            return 0;

        while (((max(vvmhw[v.iVM].xpix * (scaleX + 1), X8) + thickX) < lpmm->ptMaxTrackSize.x) &&
               ((max(vvmhw[v.iVM].ypix * (scaleX + 1) * scaleY, 200) + thickY) < lpmm->ptMaxTrackSize.y))
            {
            scaleX++;
            }

        lpmm->ptMaxTrackSize.x = max(vvmhw[v.iVM].xpix * scaleX, 320) + thickX;
        lpmm->ptMaxTrackSize.y = max(vvmhw[v.iVM].ypix * scaleX * scaleY, 320) + thickY;

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

        PAINTSTRUCT ps;

        BeginPaint(hWnd, &ps);
        RenderBitmap();
        //UpdateOverlay();

        EndPaint(hWnd, &ps);
		
		// PrintScreenStats(); printf("WM_PAINT after GDI StretchBlt\n");
		break;

       case WM_SETCURSOR:

        if (!v.fTiling && v.fFullScreen && vi.fHaveFocus)
        {
            // In full screen mode (except when tiling), don't show mouse anywhere
            SetCursor(NULL);
            return TRUE;
        }

		if (vi.fHaveFocus && (LOWORD(lParam) == HTCLIENT))
		{
			if (v.iVM < 0 || !rgvm[v.iVM].pvmi->fUsesMouse)
			{
				// always set cursor to crosshairs if VM doesn't use the mouse
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
        //DebugStr("WM_ACTIVATE %d\n", LOWORD(uParam));

		if (vi.fInDirectXMode)
        {
            v.fFullScreen = !v.fFullScreen;

#if defined(ATARIST) || defined(SOFTMAC)
            // release DirectX mode
            MarkAllPagesClean();
#endif
			UninitDrawing(TRUE);
            vi.fInDirectXMode = FALSE;

            CreateNewBitmap(v.iVM);
        }

        break;

    case WM_SETFOCUS:
        //DebugStr("WM_SETFOCUS\n");

        vi.fHaveFocus = TRUE;
		if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse)
			ShowGEMMouse();

		// If we ALT-TABBED away from the VM, eg., it still thinks ALT is down, send a bunch of UPs 
		// do we need all of these?
		if (v.iVM >= 0)
		{
			// L Shift
			FWinMsgVM(v.iVM, vi.hWnd, WM_KEYUP, 0x10, 0xc02a0001);
			// R Shift
			FWinMsgVM(v.iVM, vi.hWnd, WM_KEYUP, 0x10, 0xc0360001);
			// L CTRL
			FWinMsgVM(v.iVM, vi.hWnd, WM_KEYUP, 0x11, 0xc01d0001);
			// R CTRL
			FWinMsgVM(v.iVM, vi.hWnd, WM_KEYUP, 0x11, 0xc11d0001);
			// L ALT
			FWinMsgVM(v.iVM, vi.hWnd, WM_SYSKEYUP, 0x12, 0xc0380001);
			// R ALT
			FWinMsgVM(v.iVM, vi.hWnd, WM_SYSKEYUP, 0x12, 0xc1380001);

			vi.fRefreshScreen = TRUE;	// do some VMs need this?
		}

        //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        break;

    case WM_KILLFOCUS:
        //DebugStr("WM_KILLFOCUS\n");
#if 0
        // release DirectX mode
        MarkAllPagesClean();
        UninitDrawing(TRUE);
        vi.fInDirectXMode = FALSE;
#endif

		if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse)
		{
			ShowWindowsMouse();
			ReleaseCapture();
		}

        vi.fHaveFocus = FALSE;

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        break;

#if 0
    case MYWM_NOTIFYICON:
        switch (lParam)
            {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            //goto L_about;

        default:
            break;
            }
        break;

	case WM_PALETTECHANGED:
		if ((HWND)uParam == vi.hWnd)
			break;
		// fall through

	case WM_QUERYNEWPALETTE:
		if (FCreateOurPalette())
			InvalidateRect(hWnd, NULL, TRUE);
		return 0;
		break;
	
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

        int wmId    = LOWORD(uParam);

		switch (wmId)
		{
		
		//L_about:

		case IDM_IMPORTDOS1:
		case IDM_IMPORTDOS2:

			if (v.iVM == -1)
				break;

			int drive = wmId - IDM_IMPORTDOS1;
			
			if (!SaveATARIDOS(v.iVM, drive))
				MessageBox(vi.hWnd, "Not every file saved successfully", "Extract ATARI DOS Files", MB_OK);
			
			break;

		// bring up our ABOUT MessageBox
		case IDM_ABOUT:

			ShowAbout();
			break;

		// toggle fullscreen mode
		case IDM_FULLSCREEN:
			v.fFullScreen = !v.fFullScreen;
			
			if (v.fFullScreen)
			{
				// Get rid of the title bar and menu and maximize
				// remember if we didn't used to be maximized before
				BOOL fR = (v.swWindowState == SW_SHOWNORMAL);
				ShowWindow(vi.hWnd, SW_MAXIMIZE); // this has to go first or it might not work!
				SetMenu(vi.hWnd, NULL);
				ULONG l = GetWindowLong(vi.hWnd, GWL_STYLE);
				SetWindowLong(vi.hWnd, GWL_STYLE, l & ~(WS_CAPTION | WS_SYSMENU | WS_SIZEBOX));
				if (fR)
					v.swWindowState = SW_SHOWNORMAL; // several WM_SIZE msgs come from the above code

			}
			else
			{
				// Enable title bar and menu. Remember if we didn't used to be maximized before
				BOOL fR = (v.swWindowState == SW_SHOWNORMAL);
				ULONG l = GetWindowLong(vi.hWnd, GWL_STYLE);
				SetMenu(vi.hWnd, vi.hMenu);
				SetWindowLong(vi.hWnd, GWL_STYLE, l | (WS_CAPTION | WS_SYSMENU | WS_SIZEBOX));
				// put it back the way we found it
				if (fR)
					ShowWindow(vi.hWnd, SW_SHOWNORMAL);	// this has to go last or the next maximize doesn't work!
				else
					ShowWindow(vi.hWnd, SW_SHOWMAXIMIZED);
			}

			FixAllMenus();
			return 0; // return or windows chimes

		// toggle stretch/letterbox mode
		case IDM_STRETCH:
			v.fZoomColor = !v.fZoomColor;
			DisplayStatus(v.iVM); // affects title bar
			FixAllMenus();
			break;

		// toggle tile mode
		case IDM_TILE:
			// which tile appears in the top left? There are often more tiles than fit, so to give everybody a chance,
			// we'll start with the current instance, not always the first one.
			v.fTiling = !v.fTiling;
			if (v.cVM && v.fTiling)
			{
				//nFirstTile = v.iVM;	// show the current VM as the top left one - that's annoying
				//sWheelOffset = 0;	// start at the top - also annoying

				// now where is the mouse? That tile gets focus
				POINT pt;
				if (GetCursorPos(&pt))
					if (ScreenToClient(vi.hWnd, &pt))
					{
						int s = GetTileFromPos(pt.x, pt.y);
						if (s != sVM)
						{
							sVM = s;		// the current tile in focus (may be -1)
							v.iVM = sVM;	// the "current" vm, the main untiled one, or if tiled, the one in focus
							FixAllMenus();	// new active VM changes things
						}
					}
			} else if (v.cVM) {
				SelectInstance(v.iVM >= 0 ? v.iVM : nFirstTile);	// bring the one with focus up if it exists, else the top one
			}
			FixAllMenus();
			break;

		// toggle TURBO mode
		case IDM_TURBO:
			fBrakes = !fBrakes;
			FixAllMenus();
			break;

		case IDM_AUTOLOAD:
			v.fSaveOnExit = !v.fSaveOnExit;
			FixAllMenus();
			break;

		case IDM_TIMETRAVEL:
			
			// until we make a way to communicate this to our VM, the VM has to handle it themselves
			// by seeing the keystroke
			
			// send the to active tile
			if (v.fTiling && sVM >= 0)
			{
				FWinMsgVM(sVM, vi.hWnd, WM_KEYDOWN, 0x21, 0x01490001);	// PAGE UP
			}
			// nobody to send it to
			else if (v.fTiling)
				;

			// send it to our own and only place
			else
				FWinMsgVM(v.iVM, vi.hWnd, WM_KEYDOWN, 0x21, 0x01490001);
				
			break;

		// toggle COLOR/B&W
		case IDM_COLORMONO:

			// toggle the tile in focus (or nothing)
			if (v.fTiling && sVM >= 0)
				FToggleMonitor(sVM);
			
			// toggle the active VM
			else if (!v.fTiling)
				FToggleMonitor(v.iVM);

			break;

		// Delete this instance, and choose another
		case IDM_DELVM:

			assert(v.iVM != -1);
			if (v.iVM != -1)
				DeleteVM(v.iVM);
			break;

		case IDM_NEW:

			// delete all of our VMs
			for (int z = 0; z < MAX_VM; z++)
			{
				if (rgvm[z].fValidVM)
					DeleteVM(z);
			}

			// now try to make the default ones
			CreateAllVMs();
			break;

		// There are 32 types of VMs (it's a LONG bitfield)
		case IDM_ADDVM1:
		case IDM_ADDVM1 + 1:
		case IDM_ADDVM1 + 2:
		case IDM_ADDVM1 + 3:
		case IDM_ADDVM1 + 4:
		case IDM_ADDVM1 + 5:
		case IDM_ADDVM1 + 6:
		case IDM_ADDVM1 + 7:
		case IDM_ADDVM1 + 8:
		case IDM_ADDVM1 + 9:
		case IDM_ADDVM1 + 10:
		case IDM_ADDVM1 + 11:
		case IDM_ADDVM1 + 12:
		case IDM_ADDVM1 + 13:
		case IDM_ADDVM1 + 14:
		case IDM_ADDVM1 + 15:
		case IDM_ADDVM1 + 16:
		case IDM_ADDVM1 + 17:
		case IDM_ADDVM1 + 18:
		case IDM_ADDVM1 + 19:
		case IDM_ADDVM1 + 20:
		case IDM_ADDVM1 + 21:
		case IDM_ADDVM1 + 22:
		case IDM_ADDVM1 + 23:
		case IDM_ADDVM1 + 24:
		case IDM_ADDVM1 + 25:
		case IDM_ADDVM1 + 26:
		case IDM_ADDVM1 + 27:
		case IDM_ADDVM1 + 28:
		case IDM_ADDVM1 + 29:
		case IDM_ADDVM1 + 30:
		case IDM_ADDVM1 + 31:

			// already the maximum, sorry
			if (v.cVM == MAX_VM)
				break;

			// which one did they select? Use that bit
			int vmType = (wmId - IDM_ADDVM1);
		
			int vmNew;

			// create a new VM of the appropriate type
			vmNew = AddVM(vmType);

			BOOL fA = FALSE;
			if (vmNew != -1 && FInitVM(vmNew))
			{
				fA = ColdStart(vmNew);
				if (fA && vi.hdc)
					CreateNewBitmap(vmNew);	// we might not have a window yet, we'll do it when we do
			}
			if (!fA)
				DeleteVM(vmNew);
			else
				// even if we're tiling, what the heck
				SelectInstance(vmNew);

			// !!! what about error?
			assert(v.cVM);
			FixAllMenus();
			break;

		// choose a cartridge to use
		case IDM_CART:

			assert(v.iVM != -1);

			if (v.iVM != -1 && OpenTheFile(v.iVM, vi.hWnd, rgvm[v.iVM].rgcart.szName, FALSE, 1))
			{
				rgvm[v.iVM].rgcart.fCartIn = TRUE;

				// notice the change of cartridge state
				FUnInitVM(v.iVM);

				// If the cartridge is bad, don't kill the current VM, just go back to not using the cartridge
				BOOL f = FInitVM(v.iVM);
				if (!f)
				{
					rgvm[v.iVM].rgcart.fCartIn = FALSE;
					rgvm[v.iVM].rgcart.szName[0] = 0;
					f = FInitVM(v.iVM);
				}

				if (f && ColdStart(v.iVM))
					FixAllMenus();
				else
					DeleteVM(v.iVM);
			}
			break;

		// rip the cartridge out
		case IDM_NOCART:

			assert(v.iVM != -1);

			if (v.iVM != -1)
			{
				rgvm[v.iVM].rgcart.fCartIn = FALSE;	// unload the cartridge
				rgvm[v.iVM].rgcart.szName[0] = 0; // erase the name

				// this will require a reboot, I assume for all types of VMs?
				FUnInitVM(v.iVM);
				BOOL f = FALSE;
				if (FInitVM(v.iVM))
					if (ColdStart(v.iVM))
						f = TRUE;

				if (!f)
					DeleteVM(v.iVM);

				FixAllMenus();
			}

			break;

		// Choose a file to use for the virtual disks
		case IDM_D1:
			assert(v.iVM != -1);
			
			if (OpenTheFile(v.iVM, vi.hWnd, rgvm[v.iVM].rgvd[0].sz, FALSE, 0))
			{
				rgvm[v.iVM].rgvd[0].dt = DISK_IMAGE; // !!! support DISK_WIN32, DISK_FLOPPY or DISK_SCSI for ST/MAC

				// something might be wrong with the disk, take it back out, don't kill the VM or anything drastic
				if (!FMountDiskVM(v.iVM, 0))
					SendMessage(vi.hWnd, WM_COMMAND, IDM_D1U, 0);
				
				FixAllMenus();
			}
			break;

		case IDM_D2:
			assert(v.iVM != -1);
			
			if (OpenTheFile(v.iVM, vi.hWnd, rgvm[v.iVM].rgvd[1].sz, FALSE, 0))
			{
				rgvm[v.iVM].rgvd[1].dt = DISK_IMAGE; // support DISK_WIN32, DISK_FLOPPY or DISK_SCSI for ST/MAC
				 
				// something might be wrong with the disk, take it back out, don't kill the VM or anything drastic
				if (!FMountDiskVM(v.iVM, 1))
					SendMessage(vi.hWnd, WM_COMMAND, IDM_D2U, 0);
				
				FixAllMenus();
			}
			break;


		// unmount a drive

		case IDM_D1U:
			assert(v.iVM != -1);
			
			if (v.iVM != -1)
			{
				FUnmountDiskVM(v.iVM, 0);	// do this before tampering
				strcpy(rgvm[v.iVM].rgvd[0].sz, "");
				rgvm[v.iVM].rgvd[0].dt = DISK_NONE;
				FixAllMenus();
			}
			break;

		case IDM_D2U:
			assert(v.iVM != -1);
			
			if (v.iVM != -1)
			{
				FUnmountDiskVM(v.iVM, 1);	// do this before tampering
				strcpy(rgvm[v.iVM].rgvd[1].sz, "");
				rgvm[v.iVM].rgvd[1].dt = DISK_NONE;
				FixAllMenus();
			}
			break;

		// cycle through the types of machines this VM can handle.. eg. 800, XL, XE
		// keep the same disk and cartridge
		case IDM_CHANGEVM:
			
			if (v.iVM >= 0)
			{
				int type = rgvm[v.iVM].bfHW;
				int otype = 0;
				while (type >>= 1)
					otype++;						// convert bitfield to index
				type = ((otype + 1) & 0x1f);	// go to the next type
				PVMINFO pvmi;

				// find another type that this VM supports
				for (int zz = 0; zz < 32; zz++)
				{
					// there are no alternatives
					if (type == otype)
						break;

					// found one!
					pvmi = DetermineVMType(type);
					if (pvmi)
					{
						BOOL fOK = FALSE;
						FUnInitVM(v.iVM);
						FUnInstallVM(v.iVM);
						if (FInstallVM(v.iVM, pvmi, type))
							if (FInitVM(v.iVM))
								if (ColdStart(v.iVM))
									fOK = TRUE;
						if (!fOK)
						{
							DeleteVM(v.iVM);
						}
						FixAllMenus();
						break;

					}
					type = ((type + 1) & 0x1f);
				}
			}

			break;

#ifdef XFORMER // !!! really hacky support for toggle basic
		case IDM_TOGGLEBASIC:
			assert(v.iVM != -1);

			// It's a keystroke combo, so it's not as simple as sending the right key to the VM
			if (v.iVM != -1)
			{
				char *pCandy;
				int cb;
				if (FSaveStateVM(v.iVM, &pCandy, &cb))
				{
					pCandy += 16 + 65536 + 38;	// yet I think including atari.h is too hacky :-) Works for 32 and x64
					WORD *ramtop = (WORD *)pCandy;
					if (*ramtop == 0xC000)
						*ramtop = 0xA000;
					else
						*ramtop = 0xC000;
				}
			}

			if (!ColdStart(v.iVM))
				DeleteVM(v.iVM);
			// does not affect menus
			break;
#endif

		// Ctrl-F10, must come after IDM_TOGGLEBASIC
        case IDM_COLDSTART:
            
			if (v.iVM >= 0)
				if (!ColdStart(v.iVM))
					DeleteVM(v.iVM);
            break;

		// F10
		case IDM_WARMSTART:

			if (v.iVM >= 0)
				if (!FWarmbootVM(v.iVM))
					DeleteVM(v.iVM);
			break;

		// cycle through all the instances - isn't allowed during tiling
        case IDM_NEXTVM:
            SelectInstance(v.iVM + 1);
            break;

		case IDM_PREVVM:
			SelectInstance(-1);	// go backwards
			break;


		char chFN[MAX_PATH];
		BOOL f;

		case IDM_SAVEAS:
			chFN[0] = 0;	// necessary!
			f = OpenTheFile(v.iVM, vi.hWnd, chFN, TRUE, 2);
			if (f)
				f = SaveProperties(chFN); // !!! Dlg on failure?
			break;

		case IDM_LOAD:
			chFN[0] = 0;	// necessary!
			f = OpenTheFile(v.iVM, vi.hWnd, chFN, FALSE, 2);
			if (f)
				f = LoadProperties(chFN, FALSE); // (we do want the VMs loaded too) 
			
			if (!f)
				f = CreateAllVMs();

			sWheelOffset = 0;	// we may be scrolled further than is possible given we have fewer of them now
			sVM = -1;	// the one in focus may be gone
			FixAllMenus();

			break;


#ifndef NDEBUG
		case IDM_DEBUGGER:

			// PAUSE key enters the debugger in DEBUG
			if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse)
				ShowWindowsMouse();

			vi.fDebugBreak = TRUE;

			// Try to make use of the existing command line window if present
			DWORD ret = AttachConsole((DWORD)-1);
			if (ret)
				vi.fParentCon = TRUE;

			if (/* v.fDebugMode && */ !vi.fParentCon)
				CreateDebuggerWindow();

			SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

			return 0;
#endif

#if 0
		case IDM_PROPERTIES:
			DialogBox(vi.hInst,
				MAKEINTRESOURCE(IDD_PROPERTIES),
				vi.hWnd,
				Properties);
			if (!FIsAtari8bit(vmCur.bfHW))
				AddToPacket(FIsMac(vmCur.bfHW) ? 0xDF : 0xB8);  // Alt  key up
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
			// This msg is disabled now, TODO fix it
			if (FIsMac(vmCur.bfHW))
			{
#ifdef SOFTMAC
				vmachw.fDisk1Dirty = fTrue;
				vmachw.fDisk2Dirty = fTrue;
#endif
				// vvmhw[v.iVM].fMediaChange = fTrue;

				FMountDiskVM(v.iVM, 8);	// TODO make sure that unmounts first
			}
			else

				// if the mouse is captured in window, release it
				// otherwise capture it
				// TODO disabled right now, why was this only for !MAC?
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
            CreateNewBitmap(v.iVM);

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
                    "Tous les fichiers Atari non sauvegard�s seront perdus.\n"
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

        // Here are all the other possible menu options,
        // all of these are currently disabled:

        default:

			// We have asked to switch to a certain VM? Not allowed when tiling

			if (wmId <= IDM_VM1 && wmId > IDM_VM1 - MAX_VM)
			{
				int iVM = IDM_VM1 - wmId;	// we want the VM this far from the end
				int zl;

				// find the VM that far from the end
				for (zl = MAX_VM - 1; zl >= 0; zl--)
				{
					if (rgvm[zl].fValidVM) {
						if (iVM == 0)
							break;
						iVM--;
					}
				}

				assert(zl >= 0);
				SelectInstance(zl);
				
				// stop tiling, tiling mode was to help us choose one
				if (v.fTiling)
					SendMessage(vi.hWnd, WM_COMMAND, IDM_TILE, 0);
			}
		
            return (DefWindowProc(hWnd, message, uParam, lParam));
        
		} // switch WM_COMMAND

        return 0;
        break;

// !!! this didn't work so we poll now inside the VM who is responsible for their own joystick
// We do give hot key joystick button support, though... numeric keypad, page down, mouse button and control keys
#if 0
    case MM_JOY1MOVE:
    case MM_JOY1BUTTONDOWN:
    case MM_JOY1BUTTONUP:
    case MM_JOY2MOVE:
    case MM_JOY2BUTTONDOWN:
    case MM_JOY2BUTTONUP:
        if (vi.fExecuting)
            return FWinMsgVM(hWnd, message, uParam, lParam);
        break;
#endif

    case WM_SYSKEYDOWN:
		if (uParam == VK_RETURN)
		{
			// this magically only executes if a SYS key like ALT is pressed with ENTER
			return SendMessage(vi.hWnd, WM_COMMAND, IDM_FULLSCREEN, 0);
		}

		// fall through

    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:

		//printf("WM_KEY*: u = %08X l = %08X\n", uParam, lParam);
		
		// Do something with the MENU key?
        if (uParam == VK_APPS)
            break;

#ifndef NDEBUG
		// our accelerator doesn't seem to work
        if (uParam == VK_PAUSE)
			SendMessage(vi.hWnd, WM_COMMAND, IDM_DEBUGGER, 0);
#endif

		vi.fHaveFocus = TRUE;  // HACK !!! is this hack that's everywhere still necessary?

		//if (vi.fExecuting)
		{
			if (!v.fTiling)
			{
				// tells us if we need to eat the key, or send it to windows (ALT needs to be sent on for menu activation)
				if (FWinMsgVM(v.iVM, hWnd, message, uParam, lParam))
					return TRUE;
			}

			// send all keys to the tile that is in focus
			else
			{
				if (sVM >= 0)
				{
					BOOL bb = FWinMsgVM(sVM, hWnd, message, uParam, lParam);
					if (bb)
						return TRUE;
				}
			}
		}
		break;

    case WM_HOTKEY:
		break;

	case WM_MOUSEWHEEL:
		short int offset = GET_WHEEL_DELTA_WPARAM(uParam); // must be short to catch the sign
		BOOL fZoom = GET_KEYSTATE_WPARAM(uParam) & MK_CONTROL;

		if (fZoom)
		{
			if (offset < 0)	// zoom in
			{
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				if (v.fTiling && ScreenToClient(vi.hWnd, &pt))
				{
					int iVM = GetTileFromPos(pt.x, pt.y);
					if (iVM >= 0)
					{
						v.iVM = iVM;
						SendMessage(vi.hWnd, WM_COMMAND, IDM_TILE, 0);
						SelectInstance(iVM);
					}
				}
			}
			else if (offset > 0) // zoom out
			{
				if (!v.fTiling)
					SendMessage(vi.hWnd, WM_COMMAND, IDM_TILE, 0);
			}
		}
		else
		{
			sWheelOffset += (offset / 15); // how much did we scroll? Very slow on the surface, but any faster is unusable on normal pads.
			ScrollTiles();
		}
		break;

	case WM_GESTURENOTIFY:
		// !!! why is pan with intertia broken, as well as the GID_PAN and GC_PAN combination?
		GESTURECONFIG gc;
		gc.dwID = 0;
		gc.dwWant = GC_ALLGESTURES;
		gc.dwBlock = 0;

		BOOL bResult = SetGestureConfig(hWnd, 0, 1, &gc, sizeof(GESTURECONFIG));
		break;	// MUST break
	
	case WM_GESTURE:
		GESTUREINFO gi;
		ZeroMemory(&gi, sizeof(GESTUREINFO));
		gi.cbSize = sizeof(GESTUREINFO);
		
		static int iPanBegin; // where we first touched the screen
		static ULONGLONG iZoomBegin;	// how far apart our fingers started out

		bResult = GetGestureInfo((HGESTUREINFO)lParam, &gi);
		if (bResult)
		{
			if (gi.dwID == GID_PAN)
			{
				if (gi.dwFlags & GF_BEGIN)
					iPanBegin = gi.ptsLocation.y;	// where were we when we stated gesturing?
				else if (gi.dwFlags & GF_INERTIA)
					iPanBegin = iPanBegin;	// !!! I'll have to handle this myself, it's broken
				else
				{
					sWheelOffset += (gi.ptsLocation.y - iPanBegin);
					ScrollTiles();
					iPanBegin = gi.ptsLocation.y;
				}
				return 0;	// makes sure we don't get a mouse click on a tile while panning
			}
			else if (gi.dwID == GID_ZOOM)
			{
				if (gi.dwFlags & GF_BEGIN)
				{
					iZoomBegin = gi.ullArguments; // how far apart our fingers start
				}
				else
				{
					int iZoom = (int)gi.ullArguments - (int)iZoomBegin; // make it signed
					
					if (iZoom > 100)	// zoom in
					{
						POINT pt;
						pt.x = gi.ptsLocation.x;
						pt.y = gi.ptsLocation.y;
						if (v.fTiling && ScreenToClient(vi.hWnd, &pt))
						{
							int iVM = GetTileFromPos(pt.x, pt.y);
							if (iVM >= 0)
							{
								v.iVM = iVM;
								SendMessage(vi.hWnd, WM_COMMAND, IDM_TILE, 0);
								SelectInstance(iVM);
							}
						}
					}
					else if (iZoom < -100) // zoom out
					{
						if (!v.fTiling)
							SendMessage(vi.hWnd, WM_COMMAND, IDM_TILE, 0);
					}
				}
				return 0;
			}	// GID_ZOOM
		}

		break;	// must break

    case WM_RBUTTONDOWN:
        vi.fHaveFocus = TRUE;  // HACK again

        if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse /* && vi.fExecuting */ && vi.fGEMMouse && !vi.fInDirectXMode && !v.fNoTwoBut &&
					(!FIsAtari68K(rgvm[v.iVM].bfHW) || (uParam & MK_LBUTTON) ) )	// !!! hack don't use FIs... macros
        {
            // both buttons are being pressed, left first and now right
            // so bring back the Windows mouse cursor after sending
            // an upclick

            // see also old code for F11

//            FMouseMsgVM(hWnd, 0, 0, 0, 0);
			if (rgvm[v.iVM].pvmi->fUsesMouse)
				ShowWindowsMouse();
            return 0;
            break;
        }

#if 0
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
#endif

        // fall through into WM_LBUTTONDOWN case

    case WM_LBUTTONDOWN:
        vi.fHaveFocus = TRUE;  // HACK

#ifdef SOFTMAC
        vmachw.fVIA2 = TRUE;
#endif
		
		// if this VM type supports joystick
		if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesJoystick)
		{
            //printf("LBUTTONDOWN\n");

			// MOUSE LEFT button is joystick FIRE
				FWinMsgVM(v.iVM, hWnd, MM_JOY1BUTTONDOWN, JOY_BUTTON1, 0);
			
			// continue to do Tiling code
        }
		else if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse)
		{
			if (!vi.fVMCapture)
				if (v.iVM >= 0)
					return FWinMsgVM(v.iVM, hWnd, message, uParam, lParam);

			if (rgvm[v.iVM].pvmi->fUsesMouse /* && vi.fExecuting */ && vi.fVMCapture && !vi.fGEMMouse)
			{
				// either mouse button was pressed
				// so give special message to activate mouse

				ShowGEMMouse();

				return 0;
				break;
			}
		}

		// in Tile Mode, click on an instance to bring it up!
		//
		if (v.fTiling && message == WM_LBUTTONDOWN)
		{
			if (sVM != -1)
			{
				v.iVM = sVM;
				SendMessage(vi.hWnd, WM_COMMAND, IDM_TILE, 0);
			}
		}

		break;

    case WM_LBUTTONUP:		
		if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesJoystick)
        {
			// MOUSE button up is JOYSTICK BUTTON UP
			if (v.iVM >= 0)
				return FWinMsgVM(v.iVM, hWnd, MM_JOY1BUTTONUP, 0, 0);
        }
		else if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse)
		{
			if (!vi.fVMCapture)
				if (v.iVM >= 0)
					return FWinMsgVM(v.iVM, hWnd, message, uParam, lParam);
		}

	case WM_RBUTTONUP:
        vi.fHaveFocus = TRUE;  // HACK
		
		if (v.iVM >= 0 && rgvm[v.iVM].pvmi->fUsesMouse /* && vi.fExecuting */ && (vi.fGEMMouse || !vi.fVMCapture))
        {
            vi.fGEMMouse = FALSE;
            if (v.iVM >= 0)
				FWinMsgVM(v.iVM, hWnd, message, uParam, lParam);
            vi.fMouseMoved = FALSE;
            vi.fGEMMouse = vi.fVMCapture;
        }
        return 0;
        
    case WM_MOUSEMOVE:
		// !!! if the VM type supports a mouse, how do we figure out when to capture the mouse inside a tile?
		{
			if (v.iVM >= 0)
				FWinMsgVM(v.iVM, hWnd, message, uParam, lParam);	// give mouse move to the VM !!! not listening to whether to eat it

			// in Tile Mode, make note of which tile we are hovering over
			//
			if (v.fTiling)
			{
				// make sure this works on multimon, GET_X_LPARAM is not defined
				int xPos = LOWORD(lParam);
				int yPos = HIWORD(lParam);

				int s = GetTileFromPos(xPos, yPos);
				if (s != sVM)
				{
					sVM = s;		// the tile in focus
					v.iVM = sVM;	// "current" VM is now this
					FixAllMenus();
				}
			}

			vi.fMouseMoved = TRUE;
		}

		break;	// or return?

    case WM_POWERBROADCAST:
        // If the power status changes, the CPU clock speed may also
        // change, and that's not good if we're using the CPU clock.
        // So in such an event force the hardware timer

        if (uParam == PBT_APMPOWERSTATUSCHANGE)
            {
            // the fact we got this message means clock speed may have
            // changed so recalibrate always

#if defined(ATARIST) || defined (SOFTMAC)
			CheckClockSpeed();
#endif
            }

        break;

    case WM_DESTROY:  // message: window being destroyed

        vi.fQuitting = TRUE;
		
		SetMenu(vi.hWnd, vi.hMenu); // so that it gets freed

		UninitSound();
		ReleaseJoysticks();

#if defined(ATARIST) || defined(SOFTMAC)
        // release DirectX mode
        MarkAllPagesClean();
#endif
        UninitDrawing(TRUE);
        vi.fInDirectXMode = FALSE;

        //if (vi.fExecuting)
        {
            vi.fExecuting = FALSE;
            KillTimer(hWnd, 0);
            ReleaseCapture();
        }

		// save at least some global state, like window position, and maybe the session of VMs if they want
		SaveProperties(NULL);

		// delete all of our VMs and kill their threads
		for (int z = 0; z < MAX_VM; z++)
		{
			if (rgvm[z].fValidVM)
				DeleteVM(z);
		}

		for (int ii = 0; ii < MAX_VM; ii++)
		{
			if (hGoEvent[ii])
				CloseHandle(hGoEvent[ii]);
			if (hDoneEvent[ii])
				CloseHandle(hDoneEvent[ii]);
		}

		FInitSerialPort(0);
        ShowCursor(TRUE);

        //TrayMessage(vi.hWnd, NIM_DELETE, 0, LoadIcon(vi.hInst, MAKEINTRESOURCE(IDI_APP)), NULL);

#if defined(ATARIST) || defined(SOFTMAC)
        Sleep(200); // let other threads finish !!!
#endif

        PostQuitMessage(0);
        return 0;
        break;

    case WM_CLOSE:
#if 0
		if (!FIsAtari8bit(vmCur.bfHW))
        {
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
        }
#endif

        // fall through

    default:      // Passes it on if unprocessed
        return (DefWindowProc(hWnd, message, uParam, lParam));
        }

    return (DefWindowProc(hWnd, message, uParam, lParam));
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
// nType == 1 for a cartridge for VMs that support one
// nType == 2 for loading/saving an entire GEM session
//
BOOL OpenTheFile(int iVM, HWND hWnd, char *psz, BOOL fCreate, int nType)
{
    OPENFILENAME OpenFileName;

    // Fill in the OPENFILENAME structure to support a template and hook.
    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = NULL;

	// we want to save/load our whole session
	if (nType == 0)
		OpenFileName.lpstrFilter = rgvm[iVM].pvmi->szFilter;
	else if (nType == 1)
		OpenFileName.lpstrFilter = rgvm[iVM].pvmi->szCartFilter;
	else
		OpenFileName.lpstrFilter = "Xformer Session\0*.gem\0All Files\0*.*\0\0";

    // !!! TODO - Darek, set the proper VMINFO fields for other VMs like this:
	// MAC: VMINFO.szFilter = "Macintosh Disk Images\0*.dsk;*.ima*;*.img;*.hf*;*.hqx;*.cd\0All Files\0*.*\0\0";
	// ST: VMINFO.szFilter = "Atari ST Disk Images\0*.vhd;*.st;*.dsk;*.msa\0All Files\0*.*\0\0";

	OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = psz;
    OpenFileName.nMaxFile          = MAX_PATH;
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = fCreate ? "Save As..." : "Select a File...";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = (fCreate && nType == 2) ? "gem" : NULL;
    OpenFileName.lCustData         = 0;
    OpenFileName.lpfnHook            = NULL;
    OpenFileName.lpTemplateName    = NULL;
    OpenFileName.Flags             = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |
			(fCreate ? 0 : OFN_FILEMUSTEXIST);

    // Call the common dialog function.
	BOOL fR;
	if (fCreate)
		fR = GetSaveFileName(&OpenFileName);
	else
		fR = GetOpenFileName(&OpenFileName);

    if (fR)
	{
        strcpy(psz, OpenFileName.lpstrFile);
        return TRUE;
    }

    return FALSE;
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
#if defined(ATARIST) || defined (SOFTMAC)
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
#endif
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
    return (bf == monMono) || (bf == monSTMono);
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

void AddToPacket(int iVM, ULONG b)
{
    vrgvmi[iVM].rgbKeybuf[vrgvmi[iVM].keyhead++] = (BYTE)b;
    vrgvmi[iVM].keyhead &= 1023;
	
#if defined(ATARIST) || defined(SOFTMAC)
    vsthw[v.iVM].gpip &= 0xEF; // to indicate characters available
#endif // ATARIST
}


// ==================================
// BELOW THIS POINT IS ST or MAC ONLY
// ==================================

#if defined(ATARIST) || defined(SOFTMAC)

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

	//printf("high frequency tick counter = %08X%08X\n", vi.rglTicks[1], vi.rglTicks[0]);

	return i;
}

//
// Given a high resolution timer frequency in vi.llTicks generate a 4 byte
// array of right shift coefficients to generate a lower frequency count
//

void GenerateCoefficients(int fE)
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
		*pb++ = ((b) & 3) + 0x0A;
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
		*pb++ = ((*pbSrc) & 15) + 0x0A;
	}
}

void ScreenRefresh(void)
{
	unsigned scan;
	int  first, last;
	BYTE *pbDst;
	ULONG cbScanSrc = 1, cbScanDst = 1;
	ULONG cbSkip;
	ULONG eaScanLine = vsthw[v.iVM].dbase;
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

	typedef void(__cdecl *PFNBLIT) (void *, void *, ULONG, ULONG);

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

	vsthw[v.iVM].dbase += 0;

	if (vsthw[v.iVM].dbase > (vi.eaRAM[1] + vi.cbRAM[1] - 65536))
	{
		vsthw[v.iVM].dbase = vi.eaRAM[1];
	}

#if !defined(NDEBUG)
	//  printf("eaRAM[1] = %08X cbRAM[1] = %08X setting dbase = %08X\n", vi.eaRAM[1], vi.cbRAM[1], vsthw[v.iVM].dbase);
#endif

	// display some debug information about screen refreshes and page faults

	if (0)
	{
		static ULONG l, l0;
		char rgch[256];

		sprintf(rgch, "top:%d bot:%d R:%d W:%d V:%d ticks:%d",
			first, last, vi.countR, vi.countW, vi.countVidW, (l = GetTickCount()) - l0);
		SetWindowText(vi.hWnd, rgch);

		l0 = l;
	}

	// PrintScreenStats(); printf("Refresh: entered critical section\n");

	fMac = FIsMac(vmCur.bfHW);

	if (!fMac && !vsthw[v.iVM].fMono && vi.fRefreshScreen)
	{
		// color registers have possibly changed

		int i;

		for (i = 0; i < 16; i++)
		{
			ULONG w = vsthw[v.iVM].wcolor[i];
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
				r = r * 36 + (r >> 1);
				g = g * 36 + (g >> 1);
				b = b * 36 + (b >> 1);
			}

			DebugStr("setting rgrgb[%d] = %d, %d, %d\n", i, r, g, b);

			vsthw[v.iVM].rgrgb[i].rgbRed = (BYTE)r;
			vsthw[v.iVM].rgrgb[i].rgbGreen = (BYTE)g;
			vsthw[v.iVM].rgrgb[i].rgbBlue = (BYTE)b;
			vsthw[v.iVM].rgrgb[i].rgbReserved = 0;
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

	cbScanSrc = (vsthw[v.iVM].xpix / 8) * vsthw[v.iVM].planes;
	cbSkip = (FIsAtariEhn(vmCur.bfHW) ? vsthw[v.iVM].scanskip * 2 : 0);

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

		if (vsthw[v.iVM].xpix == 320)
			pbDst += (((vi.sx / 2 - vsthw[v.iVM].xpix) / 2)) + cbScanDst * ((vi.sy / 2 - vsthw[v.iVM].ypix) / 2);
		else if (vsthw[v.iVM].ypix == 200)
			pbDst += (vi.dx = ((vi.sx - vsthw[v.iVM].xpix) / 2)) + cbScanDst * ((vi.sy / 2 - vsthw[v.iVM].ypix) / 2);
		else
			pbDst += vi.dx + cbScanDst * vi.dy;
	}
	else
	{
		pbDst = vvmi.pvBits;
		cbScanDst = vi.cbScan;
	}

	cbScanSrc = vsthw[v.iVM].xpix / 8 * vsthw[v.iVM].planes;

	if (vsthw[v.iVM].fMono && (pbSurface || v.fNoMono))
	{
		Assert(vsthw[v.iVM].planes == 1);

		if (fMac || vsthw[v.iVM].wcolor[0] & 1)
		{
			pfnBlit = BlitMonoX;
		}
		else
		{
			pfnBlit = BlitMonoXInv;
		}
	}
	else if (vsthw[v.iVM].fMono)
	{
		Assert(vsthw[v.iVM].planes == 1);

		if (fMac || vsthw[v.iVM].wcolor[0] & 1)
		{
			pfnBlit = BlitMono;
		}
		else
		{
			pfnBlit = BlitMonoInv;
		}
	}
	else if (fMac && (vsthw[v.iVM].planes == 2))
	{
		// 4 color mode Mac II video mode

		pfnBlit = Blit4Mac;
	}
	else if (vsthw[v.iVM].planes == 2)
	{
		// medium rez

		Assert(vsthw[v.iVM].planes == 2);

		// in DirectX mode, pixels are doubled so skip scan
		if (pbSurface && vsthw[v.iVM].ypix == 200)
		{
			pfnBlit = Blit4X;
		}
		else
		{
			pfnBlit = Blit4;
		}
	}
	else if (fMac && (vsthw[v.iVM].planes == 4))
	{
		// 16 color mode Mac II video mode

		pfnBlit = Blit16Mac;
	}
	else if (vsthw[v.iVM].planes == 4)
	{
		// low rez

		Assert(vsthw[v.iVM].planes == 4);

		// in DirectX mode, pixels are doubled so skip scan
		if (pbSurface && vsthw[v.iVM].ypix == 200)
		{
			pfnBlit = Blit16X;
		}
		else
		{
			pfnBlit = Blit16;
		}
	}
	else // if (fMac && (vsthw[v.iVM].planes == 8))
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

		for (scan = 0; scan < vsthw[v.iVM].ypix; scan++)
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

			if (pbSurface && (vsthw[v.iVM].ypix == 200))
				pbDst += cbScanDst;

			eaScanLine += cbScanSrc + cbSkip;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
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



//
// Linker directives help simplify the various builds
// by not having to muck with the makefile
//

#if _MSC_VER == 1200
#pragma comment(linker, "/defaultlib:src\\atari8.vm\\x6502")
#pragma comment(linker, "/defaultlib:src\\blocklib\\blockdev")
#endif

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

char *PchSkipSpace(char *pch)
{
	char ch;

	while (ch = *pch++)
	{
		if ((ch != ' ') && (ch != 8) && (ch != 9) && (ch != 10) && (ch != 13))
			return pch - 1;
	}

	return 0;
}


char *PchGetWord(char *pch, ULONG *pu)
{
	register int x;
	int w = 0;

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
        ReadFile(h, buf, cb, (LPDWORD)&cbRead, NULL);
        CloseHandle(h);
        }

    return cbRead;
}


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

BOOL FWriteWholeFile(char *sz, int cb, void *buf)
{
	HANDLE h;
	char szDir[MAX_PATH];

	GetCurrentDirectory(MAX_PATH, (char *)szDir);

#if !defined(NDEBUG)
	printf("FWriteWholeFile: dir = '%s', file = '%s'\n", szDir, sz);
#endif

	if ((sz[1] == ':') || (sz[0] == '\\') || (sz[1] == '\\'))
		strcpy(szDir, sz);
	else
	{
		strcat(szDir, "\\");
		strcat(szDir, sz);
	}

#if !defined(NDEBUG)
	printf("actual filename: '%s'\n", szDir);
#endif

	h = CreateFile(szDir, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (h != INVALID_HANDLE_VALUE)
	{
		int cbWrite;

		WriteFile(h, buf, cb, (LPDWORD)&cbWrite, NULL);
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
		strcpy(szDir, sz);
	else
	{
		strcat(szDir, "\\");
		strcat(szDir, sz);
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
		WriteFile(h, buf, cb, (LPDWORD)&cbWrite, NULL);
		CloseHandle(h);

		return cb == cbWrite;
	}

	return FALSE;
}

/****************************************************************************

FUNCTION: EnterMode

PURPOSE:  Gemulator 2.x/3.x/4.x compatible graphics mode switch

COMMENTS: Expects $12 for 640x480 and higher for 800x600

****************************************************************************/

void EnterMode(BOOL fGraphics)
{
	if (vvmhw[iVM].VESAmode == 0x00)
	{
		// standard Atari ST resolutions

		switch (vvmhw[iVM].shiftmd)
		{
		default:
			Assert(FALSE);
			break;

		case 0:
			vvmhw[iVM].planes = 4;
			vvmhw[iVM].xpix = 320;
			vvmhw[iVM].ypix = 200;
			break;

		case 1:
			vvmhw[iVM].planes = 2;
			vvmhw[iVM].xpix = 640;
			vvmhw[iVM].ypix = 200;
			break;

		case 2:
			vvmhw[iVM].planes = 1;
			vvmhw[iVM].xpix = 640;
			vvmhw[iVM].ypix = 400;
			break;
		}
	}
	else if (vvmhw[iVM].VESAmode <= 0x12)
	{
		// VGA driver asking for 640x480 mode

		vvmhw[iVM].xpix = 640;
		vvmhw[iVM].ypix = 480;
	}
	else if (vvmhw[iVM].VESAmode < 0xFF)
	{
		// VGA driver asking for 800x600 mode

		vvmhw[iVM].xpix = 800;
		vvmhw[iVM].ypix = 600;
	}
	else
	{
		// VGA 4.0 driver asking for custom resolution

		vvmhw[iVM].xpix = HIWORD(vpregs->A4);
		vvmhw[iVM].ypix = LOWORD(vpregs->A4);
	}

	CreateNewBitmap();
}

//
// This is a dummy thread that runs at lowest priority.
// Its job is to keep the processor busy so that CheckClockSpeed works
// correctly. On SpeedStep processors the counter stops counting when the
// processor is truly idle!
//

long IdleThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	while (!vi.fQuitting && (vi.si.dwNumberOfProcessors == 1))
	{
		Sleep(250);

		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	}

	return 0;
}

// do we really need this thread just to call CheckClockSpeed? Don't we know we're on a pentium or higher?
long ScreenThread()
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

#if 0
		{
			static int count = 0;

			if ((count++ & 31) == 0)
				DisplayStatus();
		}
#endif
	}

	return 0;
}

#endif  // ATARIST SOFTMAC


// ===================================
// BELOW THIS POINT IS old #if 0 stuff
// ===================================

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
	} Palette = { 0x0300, 256 };

	HPALETTE ScreenPalette;
	HDC ScreenDC;
	int Counter;
	HWND hwnd = GetForegroundWindow();

	for (Counter = 0; Counter < 256; Counter++)
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
		ScreenPalette = SelectPalette(ScreenDC, ScreenPalette, FALSE);
		RealizePalette(ScreenDC);
		ScreenPalette = SelectPalette(ScreenDC, ScreenPalette, FALSE);
		DeleteObject(ScreenPalette);
	}

	ReleaseDC(hwnd, ScreenDC);
}

ULONGLONG GetJiffies()
{
	ULONGLONG c = GetCycles() / 29833;
	return c;
}

int UpdateOverlay()
{
#ifdef LATER
	char rgch[256], rgch2[256];

	sprintf(rgch, "RAM:%d%s Video:%dx%dx%d",
		(vi.cbRAM[0] < 4194304) ? vi.cbRAM[0] / 1024 : vi.cbRAM[0] / 1048576,
		(vi.cbRAM[0] < 4194304) ? "K" : "M",
		vsthw[v.iVM].xpix, vsthw[v.iVM].ypix, vsthw[v.iVM].planes);

	if (vi.eaROM[0] && vi.fExecuting)
	{
		sprintf(rgch2, " ROM:%08X", PeekL(vi.eaROM[0]));
		strcat(rgch, rgch2);
	}

	TextOut(vi.hdc, 16, vi.dy, rgch, strlen(rgch));
#endif

	return 0;
}

#if defined(NDEBUG) && 0

//
// Our custom printf, calls the Windows runtime instead of CRT, broken since printf is no longer an overridable fn
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


BOOL FCreateOurPalette()
{
	int         i;

	static struct {
		WORD palVersion;
		WORD palCount;
		PALETTEENTRY ape[256];
	}    pal;

#if 0
	if (vvmhw[iVM].fMono)
		return TRUE;
#endif

	if (vi.fInDirectXMode)
		return TRUE;

	if (vi.hpal == NULL)
	{
		pal.palVersion = 0x0300;
		pal.palCount = 256;

		// make a identity 1:1 palette
		for (i = 0; i<256; i++)
		{
			pal.ape[i].peRed = (BYTE)i;
			pal.ape[i].peGreen = 0;
			pal.ape[i].peBlue = 0;
			pal.ape[i].peFlags = PC_EXPLICIT;
		}

		vi.hpal = CreatePalette((LPLOGPALETTE)&pal);
	}

	if (FIsAtari8bit(vmCur.bfHW) || (vvmhw[iVM].planes == 8))
	{
		for (i = 0; i<256; i++)
		{
			pal.ape[i].peRed = vvmhw[iVM].rgrgb[i].rgbRed;
			pal.ape[i].peGreen = vvmhw[iVM].rgrgb[i].rgbGreen;
			pal.ape[i].peBlue = vvmhw[iVM].rgrgb[i].rgbBlue;
			pal.ape[i].peFlags = PC_NOCOLLAPSE;
		}
		SetPaletteEntries(vi.hpal, 0, 256, pal.ape);
	}
	else
	{
		for (i = 0; i<16; i++)
		{
			pal.ape[i].peRed = vvmhw[iVM].rgrgb[i].rgbRed;
			pal.ape[i].peGreen = vvmhw[iVM].rgrgb[i].rgbGreen;
			pal.ape[i].peBlue = vvmhw[iVM].rgrgb[i].rgbBlue;
			pal.ape[i].peFlags = PC_NOCOLLAPSE;
		}
		SetPaletteEntries(vi.hpal, 10, 16, pal.ape);
	}

	SelectPalette(vi.hdc, vi.hpal, FALSE);
	return RealizePalette(vi.hdc) != 0;
}

BOOL FVerifyMenuOption()
{
	if (!vi.fExecuting)
		return TRUE;

	ShowWindowsMouse();

#if defined(ATARIST) || defined(SOFTMAC)
	if (1 || IDOK == MessageBox(GetFocus(),
#if _ENGLISH
		"The settings you have changed require rebooting Gemulator.\n"
		"Press OK to reboot or Cancel to discard these changes.",
#elif _NEDERLANDS
		"Door het wijzigen van de instellingen moet GEM opnieuw gestart worden.\n"
		"Kies OK om opnieuw te starten of annuleer om deze wijzigingen ongedaan te maken.",
#elif _DEUTSCH
		"Die ge�nderteten Einstellungen erfordern einen Neustart von GEM.\n"
		"W�hlen Sie OK zum Neustart oder Abbruch um die �nderungen zu verwerfen.",
#elif _FRANCAIS
		"Les r�glages que vous modifiez n�cessite le red�marrage du GEM.\n"
		"Appuyez sur OK pour red�marrer ou sur Annuler pour ignorer ces modifications.",
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

	CheckMenuItem(vi.hMenu, IDM_DIRECTX, v.fFullScreen ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_ZOOM, v.fZoomColor ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_DXNORMAL, (v.fNoDDraw == 0) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_DXDELAY, (v.fNoDDraw == 1) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_DXDISABLE, (v.fNoDDraw == 2) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_SAVEONEXIT, v.fSaveOnExit ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_HYBONEXIT, v.fHibrOnExit ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(vi.hMenu, IDM_NOSCSI, v.fNoSCSI ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_NOTWOBUTTON, v.fNoTwoBut ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_NOQBOOT, v.fNoQckBoot ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_NOJIT, v.fNoJit ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_NOMONO, v.fNoMono ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(vi.hMenu, IDM_SKIPSTARTUP, v.fSkipStartup ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(vi.hMenu, IDM_DEBUGMODE, v.fDebugMode ? MF_CHECKED : MF_UNCHECKED);

	EnableMenuItem(vi.hMenu, IDM_ZAPCMOS, FIsMac(vmCur.bfHW) ? MF_ENABLED : MF_GRAYED);

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


#if 0
#ifndef BIF_RETURNONLYFSDIRS
#define BIF_RETURNONLYFSDIRS 1
#endif

UINT(__stdcall *pfnCoInitialize)(void *);
BOOL OpenThePath(HWND hWnd, char *psz)
{
	BROWSEINFO OpenFilePath;
	void *pidl; // LPCITEMDLIST pidl;

	HMODULE hOle32;

	if (pfnCoInitialize == NULL)
	{
		if ((hOle32 = LoadLibrary("ole32.dll")) == NULL ||
			(pfnCoInitialize = (void *)GetProcAddress(hOle32, "CoInitialize")) == NULL)
			return FALSE;

		(*pfnCoInitialize)(NULL);
		}

	// CoInitialize(NULL);

	memset(&OpenFilePath, 0, sizeof(OpenFilePath));

	// Fill in the BROWSEINFO structure
	OpenFilePath.hwndOwner = hWnd;
	OpenFilePath.pszDisplayName = psz;
	//    OpenFilePath.pidlRoot          = NULL;
	OpenFilePath.lpszTitle = "Select a path for ROM image files";
	//    OpenFilePath.lpfn              = NULL;
	//    OpenFilePath.lParam              = 0;
	OpenFilePath.ulFlags = BIF_RETURNONLYFSDIRS;

	// Call the shell function.

	if (pidl = SHBrowseForFolder(&OpenFilePath))
	{
		if (SHGetPathFromIDList(pidl, psz))
			return TRUE;
	}

	return FALSE;
}
#endif

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

/****************************************************************************

FUNCTION: CenterWindow (HWND, HWND)

PURPOSE:  Center one window over another

COMMENTS:

Dialog boxes take on the screen position that they were designed at,
which is not always appropriate. Centering the dialog over a particular
window usually results in a better position.

****************************************************************************/

BOOL CenterWindow(HWND hwndChild, HWND hwndParent)
{
	RECT    rChild, rParent;
	int     wChild, hChild, wParent, hParent;
	int     wScreen, hScreen, xNew, yNew;
	HDC     hdc;

	// Get the Height and Width of the child window
	GetWindowRect(hwndChild, &rChild);
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
		GetWindowRect(hwndParent, &rParent);
	}
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	// Get the display limits
	hdc = GetDC(hwndChild);
	wScreen = GetDeviceCaps(hdc, HORZRES);
	hScreen = GetDeviceCaps(hdc, VERTRES);
	ReleaseDC(hwndChild, hdc);

	// Calculate new X position, then adjust for screen
	xNew = rParent.left + ((wParent - wChild) / 2);
	if (xNew < 0) {
		xNew = 0;
	}
	else if ((xNew + wChild) > wScreen) {
		xNew = wScreen - wChild;
	}

	// Calculate new Y position, then adjust for screen
	yNew = rParent.top + ((hParent - hChild) / 2);
	if (yNew < 0) {
		yNew = 0;
	}
	else if ((yNew + hChild) > hScreen) {
		yNew = hScreen - hChild;
	}

	// Set it, and return
	return SetWindowPos(hwndChild, NULL,
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

		CenterWindow(hDlg, GetWindow(hDlg, GW_OWNER));
		return (TRUE);

	case WM_COMMAND:              // message: received a command
		wmId = LOWORD(uParam);
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

	for (i = 0; i < MAX_VM; i++)
	{
		if (rgvm[i].fValidVM)
		{
			SendDlgItemMessage(hDlg, IDC_VMSELECT,
				XB_ADDSTRING, 0, (long)rgvm[i].szModel);

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
		DestroyWindow(vi.hWnd);
		break;

	case WM_INITDIALOG:

		CenterWindow(hDlg, GetWindow(hDlg, GW_OWNER));
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
				if (rgvm[i].fValidVM)
				{
					if (rgvm[i].szModel[0] == 0)
						strcpy(rgvm[i].szModel, rgvm[i].pvmi->pchModel);

					SendDlgItemMessage(hDlg, IDC_VMSELECT,
						XB_ADDSTRING, 0, (long)rgvm[i].szModel);

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
		wmId = LOWORD(uParam);
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

				UINT(__stdcall *pfnSE)();
				HMODULE hSh32;
				int ret = 0;

				if ((hSh32 = LoadLibrary("shell32.dll")) &&
					(pfnSE = (void *)GetProcAddress(hSh32, "ShellExecuteA")))
				{
					char rgch[80];

					if (strncmp("ftp:", rgszWeb[wmId - IDM_WEB_EMULATORS], 4))
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
				if (rgvm[iVM].fValidVM && (i-- == 0))
					break;
			}

			// has it changed from the current iVM?

			if (iVM != v.iVM)
			{
				if (FVerifyMenuOption())
					SelectInstance(iVM);
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
						rgvm[iVM] = rgvm[iVM + 1];

					memset(&rgvm[iVM], 0, sizeof(VM));

					if (i >= --c)
						i = c - 1;

					SendDlgItemMessage(hDlg, IDC_VMSELECT,
						XB_SETCURSEL, i, 0);

					for (iVM = 0; iVM < v.cVM; iVM++)
					{
						if (rgvm[iVM].fValidVM && (i-- == 0))
							break;
					}

					SelectInstance(iVM);
				}
			}

			else if (wmId == IDC_BUTCOPYMODE)
			{
				ULONG c = SendDlgItemMessage(hDlg, IDC_VMSELECT,
					XB_GETCOUNT, 0, 0);

				if (c < v.cVM)
				{
					int i;

					if (AddVM(vmCur.pvmi, &i))
					{
						rgvm[i] = rgvm[iVM];
						rgvm[i].szModel[0] = '+';
						memcpy(&rgvm[i].szModel[1],
							&rgvm[iVM].szModel[0],
							sizeof(rgvm[iVM].szModel) - 1);

						rgvm[i].szModel[sizeof(rgvm[iVM].szModel) - 1]
							= 0;

						SendDlgItemMessage(hDlg, IDC_VMSELECT,
							XB_ADDSTRING, 0, (long)rgvm[i].szModel);

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
			if (SelectInstance(v.iVM + 1))
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
				if (IDOK == MessageBox(GetFocus(),
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
				if (IDOK == MessageBox(GetFocus(),
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
			if (IDOK == MessageBox(GetFocus(),
				"Insert a formatted floppy disk into your PC's A: drive.\n"
				"The disk will be quick formatted in Macintosh format.",
				vi.szAppName, MB_OKCANCEL))
			{
				PDI pdi;
				BYTE *pb = VirtualAlloc(NULL, 1440 * 1024, MEM_COMMIT, PAGE_READWRITE);
				BOOL f = fFalse;
				HCURSOR h = SetCursor(LoadCursor(NULL, IDC_WAIT));

				FUnInitDisksVM();

				pdi = PdiOpenDisk(DISK_FLOPPY, 0, DI_QUERY);

				if (pb && pdi)
				{
					CreateHFSBootBlocks(pb, 1440, "Untitled");

					pdi->sec = 0;
					pdi->count = 16 * (1024 / 512); // 16K is all that's needed
					pdi->lpBuf = pb;

					f = FRawDiskRWPdi(pdi, 1);

					CloseDiskPdi(pdi);
				}

				if (pb)
					VirtualFree(pb, 0, MEM_RELEASE);

				FInitDisksVM();

				SetCursor(h);

				MessageBox(GetFocus(),
					f ? "Floppy disk formatted successfully!" :
					"Floppy disk format failed!",
					vi.szAppName, MB_OK);
			}
			break;

		case IDM_FLOP2IMG:
			if (IDOK == MessageBox(GetFocus(),
				"Insert a floppy disk into your PC's A: drive.\n"
				"The disk will be read and saved as an image.",
				vi.szAppName, MB_OKCANCEL))
			{
				PDI pdi;
				BYTE *pb = VirtualAlloc(NULL, 1440 * 1024, MEM_COMMIT, PAGE_READWRITE);
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

					f = WriteFile(hf, pb, 1440 * 1024, &cbRead, NULL);

					CloseHandle(hf);
				}

				if (pb)
					VirtualFree(pb, 0, MEM_RELEASE);

				FInitDisksVM();

				SetCursor(h);

				MessageBox(GetFocus(),
					f ? "Disk image successfully created!" :
					"Disk image was not created!",
					vi.szAppName, MB_OK);
			}
			break;

		case IDM_IMG2FLOP:
			if (IDOK == MessageBox(GetFocus(),
				"Insert a blank floppy disk into your PC's A: drive.\n"
				"Now select a disk image file to copy to the floppy.",
				vi.szAppName, MB_OKCANCEL))
			{
				PDI pdi;
				BYTE *pb = VirtualAlloc(NULL, 1440 * 1024, MEM_COMMIT, PAGE_READWRITE);
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

				MessageBox(GetFocus(),
					f ? "Disk image successfully copied to floppy!" :
					"Disk image was not copied to floppy!",
					vi.szAppName, MB_OK);
			}
			break;

		case IDM_NEWHFX:
		case IDM_NEWDSK:
			if (IDOK == MessageBox(GetFocus(),
				"You are about to create a large disk image file that "
				"will use up to 2 gigabytes of available disk space on "
				"the selected partition.",
				vi.szAppName, MB_OKCANCEL))
			{
				BYTE *pb = VirtualAlloc(NULL, 1440 * 1024, MEM_COMMIT, PAGE_READWRITE);
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
						f = WriteFile(hf, pb, 32 * 1024, &cbRead, NULL);

						if (f)
						{
							ckb += cbRead / 1024;

							if (ckb > 2000)
							{
								ckb = SetFilePointer(hf,
									i * 1024, NULL, FILE_BEGIN) / 1024;

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

							WriteFile(hf, pb, 1440 * 1024, &cbRead, NULL);
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

				MessageBox(GetFocus(),
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
				vi.rgpdi[0], vi.rgpdi[1], vi.rgpdi[2], vi.rgpdi[3],
				vi.woc.szPname, vi.woc.wChannels, vi.woc.dwFormats,
				((CheckClockSpeed()) + 500) / 1000,
				vi.rgbShiftMs[0], vi.rgbShiftMs[1], vi.rgbShiftMs[2],
				vi.rgbShiftMs[3], vi.rgbShiftMs[4], vi.rgbShiftMs[5],
				vi.rgbShiftE[0], vi.rgbShiftE[1], vi.rgbShiftE[2],
				vi.rgbShiftE[3], vi.rgbShiftE[4], vi.rgbShiftE[5],
				vi.fExecuting ? vmPeekL(vi.eaROM[0]) : 0
			);

			MessageBox(GetFocus(), rgch, "Info", MB_OK);
		}
		break;

#if 0
		case IDM_SNAPSHOT: // write out a memory dump
			MemorySnapShot();
			break;
#endif

		case IDM_EXIT:
#if 0
			if (IDOK != MessageBox(GetFocus(),
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
				"Tous les fichiers Atari non sauvegard�s seront perdus.\n"
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
			DestroyWindow(vi.hWnd);
			break;
		}

		break;
	}
	return (FALSE); // Didn't process the message

	lParam; // This will prevent 'unused formal parameter' warnings
}


//
// Tray handling
//

#define MYWM_NOTIFYICON        (WM_APP+100)

BOOL TrayMessage(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, PSTR pszTip)
{
	BOOL f = FALSE;
	NOTIFYICONDATA tnd;

	memset(&tnd, 0, sizeof(tnd));

	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hDlg;
	tnd.uID = uID;

	tnd.uFlags = NIF_MESSAGE;
	tnd.uCallbackMessage = MYWM_NOTIFYICON;

	tnd.hIcon = hIcon;

	if (hIcon != NULL)
		tnd.uFlags |= NIF_ICON;

	if (pszTip)
	{
		tnd.uFlags |= NIF_TIP;
		lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
	}
	else
		tnd.szTip[0] = '\0';

	//  We leave a bunch of icons in the tray
#ifdef WINXP
	f = Shell_NotifyIcon(dwMessage, &tnd);
#endif

	if (hIcon)
		DestroyIcon(hIcon);

	return f;
}

#endif

#pragma comment(linker, "/version:9.21")
#pragma comment(linker, "/defaultlib:comdlg32")
#pragma comment(linker, "/defaultlib:gdi32")
#pragma comment(linker, "/defaultlib:shell32")
