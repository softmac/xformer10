
/****************************************************************************

    PROPS.C

    - Dialog handler for Properties, load and save properties
    - Also handles the Disks Properties dialog.

    Copyright (C) 1991-2018 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    03/03/2008  darekm      silent first time autoconfig for EmuTOS

****************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include "gemtypes.h"
#include <sys/stat.h>

// private globals

#ifdef XFORMER
#ifdef ATARIST
char const szIniFile[] = "GEM2000.INI";     // Atari ST, Atari 8-bit, maybe Mac
#else
char const szIniFile[] = "XF2000.INI";      // Atari 8-bit only
#endif // ATARIST

#else  // !XFORMER
#ifdef SOFTMAC
#ifdef ATARIST
char const szIniFile[] = "GEM2000.INI";     // Atari ST, Macintosh
#else
char const szIniFile[] = "SOFTMAC.INI";     // Macintosh only
#endif // ATARIST
#else
char const szIniFile[] = "GEM2000.INI";     // Atari ST only
#endif // SOFTMAC
#endif // XFORMER


// This should be the ONLY CODE allowed to have secret knowledge about the type of VM being used (calling FIs...)
// the code that gives you the VMINFO describing it, based on the VM type # (0, 1, 2 are ATARI 800, etc.)
// which is all you should need to know from now on
// Returns a pointer to a global structure - no alloc or free needed
//
PVMINFO DetermineVMType(int type)
{
    PVMINFO pvmi = NULL;

#ifdef XFORMER
    if (FIsAtari8bit(1 << type))
        pvmi = (PVMINFO)&vmi800;
#endif

#ifdef ATARIST
    else if (FIsAtari68K(type))
        pvmi = (PVMINFO)&vmiST;
#endif

#ifdef SOFTMAC
    if (FIsMac16(type))
        pvmi = (PVMINFO)&vmiMac;
#endif

#ifdef SOFTMAC2
    if (FIsMac68020(type))
        pvmi = (PVMINFO)&vmiMacII;

    if (FIsMac68030(type))
        pvmi = (PVMINFO)&vmiMacQdra;

    if (FIsMac68040(type))
        pvmi = (PVMINFO)&vmiMacQdra;
#ifdef POWERMAC
    if (FIsMacPPC(type))
        pvmi = (PVMINFO)&vmiMacPowr;
#endif // POWERMAC
#endif // SOFTMAC2

    return pvmi;
}


//
// Add a new virtual machine to rgvm - return which instance it found
// It will Install only... the caller needs to set a disk or cartridge image after this
// before calling Init, ColdStart and CreateNewBitmap
//

int AddVM(int type)
{
    int iVM;

    // first try to find an empty slot in the rgvm array
    for (iVM = 0; iVM < MAX_VM; iVM++)
    {
        if (!rgvm[iVM].fValidVM)
            break;
    }

    // did not find an empty slot!
    if (iVM == MAX_VM)
    {
        return -1;
    }

    memset(&rgvm[iVM], 0, sizeof(VM));    // clear it out fresh
    rgvm[iVM].cbSize = sizeof(VM);    // init the size for validity

    // get the VMINFO all about this type of VM, incl. our jump table needed to call InstallVM
    rgvm[iVM].pvmi = DetermineVMType(type);

    BOOL f = FALSE;
    if (rgvm[iVM].pvmi)
        f = FInstallVM(iVM, rgvm[iVM].pvmi, type);

    if (f)
    {
        // Make a thread for this VM and tell it which VM it is
        iThreadVM[iVM] = iVM;
        fKillThread[iVM] = FALSE;

        hGoEvent[iVM] = CreateEvent(NULL, FALSE, FALSE, NULL);
        hDoneEvent[iVM] = CreateEvent(NULL, FALSE, FALSE, NULL);

#pragma warning(push)
#pragma warning(disable:4152) // function/data pointer conversion
        // default stack size of 1M wastes tons of memory and limit us to a few VMS only - smallest possible is 64K
        if (!hGoEvent[iVM] || !hDoneEvent[iVM] ||
#ifdef NDEBUG
                    !CreateThread(NULL, 65536, (void *)VMThread, (LPVOID)&iThreadVM[iVM], STACK_SIZE_PARAM_IS_A_RESERVATION, NULL))
#else   // debug needs twice the stack
                    !CreateThread(NULL, 65536 * 2, (void *)VMThread, (LPVOID)&iThreadVM[iVM], STACK_SIZE_PARAM_IS_A_RESERVATION, NULL))
#endif
        {
            if (hGoEvent[iVM])
            {
                CloseHandle(hGoEvent[iVM]);
                hGoEvent[iVM] = NULL;
            }
            if (hDoneEvent[iVM])
            {
                CloseHandle(hDoneEvent[iVM]);
                hDoneEvent[iVM] = NULL;
            }
            FUnInstallVM(iVM);
            f = FALSE;
        }
#pragma warning(pop)
    }

    if (f)
    {
        v.cVM++;

        rgvm[iVM].fValidVM = f;

        // always enable these, why not?
        rgvm[iVM].fSound = TRUE;
        rgvm[iVM].fJoystick = TRUE;

        // if this is our first VM...
        if (v.cVM == 1)
            fNeedTiledBitmap = TRUE;    // it might be too early to create it without a DC
    }    
    else
    {
        Assert(rgvm[iVM].fValidVM == FALSE);
        iVM = -1;   // return error
    }

    return iVM;
}

// This will Uninit and delete the VM, freeing it's bitmap, etc.
//
void DeleteVM(int iVM, BOOL fFixMenus)
{
    if (!rgvm[iVM].fValidVM)
        return;

    Assert(hGoEvent[iVM] != NULL);
    Assert(hDoneEvent[iVM] != NULL);

    // kill the thread executing this event before deleting any of its objects
    fKillThread[iVM] = TRUE;
    SetEvent(hGoEvent[iVM]);
    WaitForSingleObject(hDoneEvent[iVM], INFINITE);

    hGoEvent[iVM] = NULL;
    hDoneEvent[iVM] = NULL;

    if (vrgvmi[iVM].hdcMem)
    {
        SelectObject(vrgvmi[iVM].hdcMem, vrgvmi[iVM].hbmOld);
        DeleteDC(vrgvmi[iVM].hdcMem);
        vrgvmi[iVM].hdcMem = NULL;
    }

    if (vrgvmi[iVM].hbm)
    {
        DeleteObject(vrgvmi[iVM].hbm);
        vrgvmi[iVM].hbm = NULL;
    }

    FUnInitVM(iVM);
    FUnInstallVM(iVM);
    rgvm[iVM].fValidVM = FALSE;    // the next line will do this anyway, but let's be clear

    v.cVM--;    // one fewer valid instance now

    sWheelOffset = 0;    // we may be scrolled further than is possible given we have fewer of them now
    sVM = -1;    // the one in focus may be gone

    if (v.cVM)
    {
        SelectInstance(v.iVM);    // better find a new valid current instance
    }
    else
    {
        v.iVM = -1;
        RenderBitmap(); // draw black to make the last image go away
    }

    // OK, we're not doing a bunch of deletes in a row, take the possibly slow step of fixing the menus
    if (fFixMenus)
        FixAllMenus();
}

//
// First time initialization of global properties
//

void InitProperties()
{
    memset(&v, 0, sizeof(v));

    //v.fPrivate = fTrue;         // new style settings

    v.cb = sizeof(PROPS);
    v.wMagic = 0x82201233; // some unique value
    v.iVM = -1;     // no valid current instance

    // all fields default to 0 or fFalse except for these...

    v.ioBaseAddr = 0x240;
    v.cCards = 1;
    v.fNoDDraw = 2;      // on NT 4.0 some users won't be able to load DirectX
    v.fZoomColor = FALSE; // make the window large
    v.fNoMono = TRUE;  // in case user is running an ATI video card
    v.fSaveOnExit = TRUE;  // automatically save settings on exit

#if defined (ATARIST) || defined (SOFTMAC)
    // Initialize default ROM directory to current directory
    strcpy((char *)&v.rgchGlobalPath, (char *)&vi.szDefaultDir);
#endif

    return;
}

// make one of everything. Returns TRUE if it made at least one thing
// Another function that is allowed special knowledge of VMs
//
BOOL CreateAllVMs()
{
    int vmNew;
    BOOL f = FALSE, fSelected = FALSE;

    // make one of everything at the same time
    for (int zz = 0; zz < 32; zz++)
    {
        // is this a kind of VM we can handle?
        PVMINFO pvmi = DetermineVMType(zz);

        if (pvmi)
        {
            if ((vmNew = AddVM(zz)) == -1)
                return FALSE;

            f = FALSE;
            if (FInitVM(vmNew))
            {
                f = ColdStart(vmNew);
                if (f && vi.hdc)
                    f = CreateNewBitmap(vmNew);    // we might not have a window yet, we'll do it when we do
            }
            if (!f)
                DeleteVM(vmNew, TRUE);
            else
            {
                if (!fSelected)
                {
                    SelectInstance(vmNew);    // select the first one we make as current
                    fSelected = TRUE;
                }
            }
        }
    }

    FixAllMenus();

    return f;
}


//
// Read properties from INI file and/or save .GEM file
//
// Returns TRUE if at least some valid data was loaded, but maybe not all
//
// NULL = save to INI file that becomes the auto-start up file
//
// fPropsOnly means we are not auto-loading the previous session, so get the global props, like window size, but no VM data
// All existing VM data is always erased
//
// !fPropsOnly means get just the VM data, but DON'T get the global properties, we'll use what we already have
//
BOOL LoadProperties(char *szIn, BOOL fPropsOnly)
{
    PROPS vTmp = { 0 };
    char rgch[MAX_PATH];
    char *sz;
    BOOL f = FALSE;

    sz = szIn;

    // load from an INI
    if (!szIn)
    {
        GetCurrentDirectory(sizeof(rgch), rgch);
        SetCurrentDirectory(vi.szWindowsDir);    // first try to load from "\users\xxxx\appdata\roaming\emulators", for example
        sz = (char *)szIniFile;
    }

    vTmp.cb = 0;

    int h = _open(sz, _O_BINARY | _O_RDONLY);

    // don't hurt the current session if this load fails
    int l;
    if (h != -1)
    {
        _lseek(h, 0L, SEEK_SET);
        l = sizeof(vTmp);
        l = _read(h, &vTmp, l);
    }

    // if INI file contained valid data, use what we can of it, otherwise fail
    if ((vTmp.cb == sizeof(vTmp)) && (vTmp.wMagic == v.wMagic)) // check for version difference
    {

        // looks like something valid can be read, now it's safe to delete the old stuff
        // BEFORE we set v
        for (int z = 0; z < MAX_VM; z++)
        {
            if (rgvm[z].fValidVM)
                DeleteVM(z, FALSE);
        }
        FixAllMenus();
        assert(v.cVM == 0);

        //The important part about a saved .GEM file is the VMs in it, not the global state at the time.
        // If we're loading a saved .GEM file, do NOT replace our global state, just load in the VMs
        // It would be rude to jump you into fullscreen or tiled mode all of a sudden when you choose File/Load...
        // Also, drag/drop a .GEM file and you'll get the last saved INI global state, but the VMs from the GEM file
        if (!szIn)
            v = vTmp;

        f = TRUE;
        v.cVM = 0;    // no matter what they say, assume they're bad until we see they're good

        // we're loading our .ini file and they didn't want the last session restored, so don't
        if (!szIn && !v.fSaveOnExit)
            fPropsOnly = TRUE;

        // non-persistable pointers need to be refreshed
        // and only use VM's that we can handle in this build

        char *pPersist = malloc(65536);

        for (int i = 0; i < MAX_VM; i++)
        {
            // actually, don't go further
            if (fPropsOnly)
                break;

            // read the per-instance VM persistable structure
            l = _read(h, &rgvm[i], sizeof(VM));
            if (l != sizeof(VM) || rgvm[i].cbSize != sizeof(VM))
            {
                memset(&rgvm[i], 0, sizeof(VM));    // clean it
                rgvm[i].cbSize = sizeof(VM);    // init the size for validity
                break;
            }

            // what index, 0 based, is this bit?
            int c = -1, t = rgvm[i].bfHW;
            assert(t);
            while (t)
            {
                t >>= 1;
                c++;
            }

            // Get the VMINFO that tells us all about this type of VM being loaded
            rgvm[i].pvmi = DetermineVMType(c);

            // we just loaded a valid instance off disk. Install and Init it
            if (f && rgvm[i].fValidVM)
            {
                f = FInstallVM(i, rgvm[i].pvmi, c);

                if (f)
                {
                    // get the size of the persisted data
                    DWORD cb;
                    l = _read(h, &cb, sizeof(DWORD));

                    // either restore from the persisted data, or just Init a blank VM if something goes wrong
                    f = FALSE;
                    if (l)    // that's a L, not a 1.
                    {
                        pPersist = realloc(pPersist, cb);
                        l = 0;
                        if (pPersist)
                            l = _read(h, pPersist, cb);
                        if (l == (int)cb) {
                            if (FLoadStateVM(i, pPersist, cb))
                                f = TRUE;
                        }
                    }

                    if (!f)
                    {
                        FUnInitVM(i);
                        FUnInstallVM(i);
                    }
                }

                // make the screen buffer. If it's too soon (initial start up instead of Load through menu)
                // it will happen when our window is created
                if (f)
                {
                    // create a thread to execute this VM
                    iThreadVM[i] = i;
                    fKillThread[i] = FALSE;

                    hGoEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
                    hDoneEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

#pragma warning(push)
#pragma warning(disable:4152) // function/data pointer conversion
                    // default stack size of 1M wastes tons of memory and limit us to a few VMS only - smallest possible is 64K
                    if (!hGoEvent[i] || !hDoneEvent[i] ||
#ifdef NDEBUG
                        !CreateThread(NULL, 65536, (void *)VMThread, (LPVOID)&iThreadVM[i], STACK_SIZE_PARAM_IS_A_RESERVATION, NULL))
#else   // debug needs twice the stack
                        !CreateThread(NULL, 65536 * 2, (void *)VMThread, (LPVOID)&iThreadVM[i], STACK_SIZE_PARAM_IS_A_RESERVATION, NULL))
#endif
                    {
                        if (hGoEvent[i])
                        {
                            CloseHandle(hGoEvent[i]);
                            hGoEvent[i] = NULL;
                        }
                        if (hDoneEvent[i])
                        {
                            CloseHandle(hDoneEvent[i]);
                            hDoneEvent[i] = NULL;
                        }
                        FUnInitVM(i);
                        FUnInstallVM(i);
                        f = FALSE;
                    }
#pragma warning(pop)
                    v.cVM++;
                    if (f && vi.hdc)
                        f = CreateNewBitmap(i);
                    if (!f)
                        // with v.cVM upated, fValidVM set and the bitmap created, this function will now work
                        // and it's needed to destroy the thread properly
                        DeleteVM(i, TRUE);
                }
            }
            if (!f)
            {
                memset(&rgvm[i], 0, sizeof(VM));    // clean it
                rgvm[i].cbSize = sizeof(VM);    // init the size for validity
            }
        }

        if (pPersist)
            free(pPersist);

        // now select the instance that was current when we saved (or find something good)
        if (v.cVM > 0)
            SelectInstance(v.iVM >= 0 ? v.iVM : 0);
        else
            v.iVM = -1;    // make sure everybody knows there is no valid instance
    }

    if (h != -1)
        _close(h);

#if defined (ATARIST) || defined (SOFTMAC)
    if (strlen((char *)&v.rgchGlobalPath) == 0)
        strcpy((char *)&v.rgchGlobalPath, (char *)&vi.szDefaultDir);
#endif

    if (!szIn)
        SetCurrentDirectory(rgch);

#if 0
    // If we just imported an old style INI file with global VM settings
    // initialize the private VM settings to default values

    if (!v.fPrivate)
        {
        unsigned i;

        for (i = 0; i < MAX_VM; i++)
            {
            // All other settings default to 0 or fFalse
            // but this flag needs to be set by default

            rgvm[i].fCPUAuto = fTrue;
            }

        v.fPrivate = fTrue;
        }
#endif

    // if we only partially loaded, but failed, free the stuff that did load
    // !!! We could keep some loaded, but then re-saving it will lose some info and they might not realize.
    if (!f && v.cVM)
    {
        for (int z = 0; z < MAX_VM; z++)
        {
            if (rgvm[z].fValidVM)
                DeleteVM(z, FALSE); // don't fix menus each time, that's painfully slow
        }
        FixAllMenus();
        return FALSE;
    }

    // we only need to have loaded VMs to succeed, if we wanted to.
    return f && (fPropsOnly || v.cVM);
}

//
// NULL = save global props to INI file, look to flag to see whether or not to also save session data
// NON-NULL = save to a specific .GEM file, in which case always save the session data
//
BOOL SaveProperties(char *szIn)
{
    char rgch[MAX_PATH];
    int h;
    BOOL f = FALSE;

    if (szIn == NULL)
    {
        GetCurrentDirectory(sizeof(rgch), rgch);
        SetCurrentDirectory(vi.szWindowsDir);    // saves to "users\xxxxx\appdata\roaming\emulators", for example
        h = _open(szIniFile, _O_BINARY | _O_CREAT | O_WRONLY | _O_TRUNC, _S_IREAD | _S_IWRITE);
    }
    else
    {
        h = _open(szIn, _O_BINARY | _O_RDONLY);
        if (h != -1)
        {
            _close(h);
            int h2 = MessageBox(vi.hWnd, "File exists. Overwrite?", "File Exists", MB_YESNO);
            if (h2 == IDNO)
                return FALSE;    // we don't differentiate between file exists real failure
        }
        h = _open(szIn, _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC, _S_IREAD | _S_IWRITE);
    }

    if (h != -1)
    {
        // write our GLOBAL PERSISTABLE - PROPS
        _lseek(h, 0L, SEEK_SET);
        int l = _write(h, &v, sizeof(v));

        // we want to auto-save our .ini or we're saving to a particular file
        if ((szIn || v.fSaveOnExit) && l == sizeof(v))
        {
            f = TRUE;

            for (int i = 0; i < MAX_VM && f; i++)
            {
                if (rgvm[i].fValidVM)
                {
                    f = FALSE;

                    // save the VM pesistable struct for this instance
                    l = _write(h, &rgvm[i], sizeof(VM));
                    if (l == sizeof(VM))
                    {
                        DWORD cb;
                        char *pPersist;
                        if (FSaveStateVM(i, &pPersist, (int *)&cb))
                        {
                            // save the state of this VM - size first to prevent reading garbage and thinking its valid
                            l = _write(h, &cb, sizeof(DWORD));
                            if (l == sizeof(DWORD))
                            {
                                l = _write(h, pPersist, cb);
                                if (l == (int)cb)
                                    f = TRUE;    // it all worked! Something was saved
                            }
                        }
                    }
                }
            }
        }
        _close(h);
    }

    if (szIn == NULL)
        SetCurrentDirectory(rgch);

    return f;
}

#if 0
#if 0
BOOL OpenTheINI(HWND hWnd, char *psz)
{
    OPENFILENAME OpenFileName;

    // Fill in the OPENFILENAME structure to support a template and hook.
    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hWnd;
    OpenFileName.hInstance = NULL;
    OpenFileName.lpstrFilter =
        "Configuration Files (*.cfg;*.ini)\0*.cfg;*.ini\0All Files\0*.*\0\0";
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter = 0;
    OpenFileName.nFilterIndex = 0;
    OpenFileName.lpstrFile = psz;
    OpenFileName.nMaxFile = MAX_PATH;
    OpenFileName.lpstrFileTitle = NULL;
    OpenFileName.nMaxFileTitle = 0;
    OpenFileName.lpstrInitialDir = NULL;
    OpenFileName.lpstrTitle = "Select Configuration File";
    OpenFileName.nFileOffset = 0;
    OpenFileName.nFileExtension = 0;
    OpenFileName.lpstrDefExt = NULL;
    OpenFileName.lCustData = 0;
    OpenFileName.lpfnHook = NULL;
    OpenFileName.lpTemplateName = NULL;
    OpenFileName.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST \
        | OFN_CREATEPROMPT;

    // Call the common dialog function.
    if (GetOpenFileName(&OpenFileName))
    {
        strcpy(psz, OpenFileName.lpstrFile);
        return TRUE;
    }

    return FALSE;
}
#endif

BOOL SaveState(BOOL fSave)
{
    return TRUE;
}

void ListBootDrives(HWND hDlg)
{
    // Stuff description of all detected ROMs to given dialog box

    int i;
    char rgch[8];

    SendDlgItemMessage(hDlg, IDC_BOOTLIST, CB_RESETCONTENT, 0, 0);

    if (FIsMac(vmCur.bfHW))
        {
        SendDlgItemMessage(hDlg, IDC_BOOTLIST, CB_ADDSTRING, 0, (LPARAM)"Default");

        rgch[0] = 'S';
        rgch[1] = 'C';
        rgch[2] = 'S';
        rgch[3] = 'I';
        rgch[4] = ' ';
        rgch[6] = '\0';

        for (i = 0; i < 7; i++)
            {
            rgch[5] = '0' + i;
            SendDlgItemMessage(hDlg, IDC_BOOTLIST, CB_ADDSTRING, 0, (LPARAM)rgch);
            }
        return;
        }

    rgch[1] = ':';
    rgch[2] = '\0';

    for (i = 0; i < 26; i++)
        {
        rgch[0] = 'A' + i;
        SendDlgItemMessage(hDlg, IDC_BOOTLIST, CB_ADDSTRING, 0, (LPARAM)rgch);
        }
}


/****************************************************************************

    FUNCTION: Properties(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Properties" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT CALLBACK Properties(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    VM vmOld = vmCur;
    BOOL fSaveSettings = FALSE;

    vmOld.bfCPU = (vi.fFake040 ? cpu68040 : vmCur.bfCPU);

    switch (message)
        {
        case WM_INITDIALOG:
            // initialize dialog box

            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

            SetDlgItemText(hDlg, IDC_EDITNAME, vmCur.szModel);

            CheckDlgButton(hDlg, IDC_JOYSTICK,   vmCur.fJoystick);
            EnableWindow(GetDlgItem(hDlg, IDC_JOYSTICK), !FIsMac(vmCur.bfHW));

            CheckDlgButton(hDlg, IDC_STSOUND,    vmCur.fSound);

            // initialize the chipset / model entry

            SendDlgItemMessage(hDlg, IDC_COMBOCHIPSET, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_COMBOCHIPSET, CB_ADDSTRING, 0, (long)"Auto");
            SendDlgItemMessage(hDlg, IDC_COMBOCHIPSET, CB_SETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_COMBOCHIPSET), 0);

            // initialize Memory

            {
            ULONG bf = 1;
            int i = 0, iSel = 0;

            SendDlgItemMessage(hDlg, IDC_COMBOMEM, CB_RESETCONTENT, 0, 0);

            while (bf)
                {
                if (bf & vmCur.pvmi->wfRAM)
                    {
                    if (bf == vmCur.bfRAM)
                        iSel = i;

                    SendDlgItemMessage(hDlg, IDC_COMBOMEM, CB_ADDSTRING, 0,
                         (long)PchFromBfRgSz(bf, rgszRAM));
                    i++;
                    }
                bf <<= 1;
                }

            SendDlgItemMessage(hDlg, IDC_COMBOMEM, CB_SETCURSEL, iSel, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_COMBOMEM), !FIsAtari8bit(vmCur.bfHW));
            }

            // initialize Hard Disk

            CheckDlgButton(hDlg, IDC_VHD,       !vmCur.fUseVHD);
            EnableWindow(GetDlgItem(hDlg, IDC_VHD), !FIsAtari8bit(vmCur.bfHW));

            ListBootDrives(hDlg);
            SendDlgItemMessage(hDlg, IDC_BOOTLIST, CB_SETCURSEL, max(0,vmCur.iBootDisk), 0);
            EnableWindow(GetDlgItem(hDlg, IDC_BOOTLIST), !FIsAtari8bit(vmCur.bfHW));

            // initialize Serial and Parallel ports

            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_ADDSTRING, 0, (long)"None");
            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_ADDSTRING, 0, (long)"COM1:");
            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_ADDSTRING, 0, (long)"COM2:");
            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_ADDSTRING, 0, (long)"COM3:");
            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_ADDSTRING, 0, (long)"COM4:");

            SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_ADDSTRING, 0, (long)"None");
            SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_ADDSTRING, 0, (long)"LPT1:");
            SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_ADDSTRING, 0, (long)"LPT2:");
            SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_ADDSTRING, 0, (long)"LPT3:");

            CheckDlgButton(hDlg, IDC_SHARE, vmCur.fShare);
            SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_SETCURSEL, vmCur.iCOM, 0);
            SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_SETCURSEL, vmCur.iLPT, 0);

            CheckDlgButton(hDlg, IDC_SWAPUKUS,  vmCur.fSwapKeys);
            EnableWindow(GetDlgItem(hDlg, IDC_SWAPUKUS), FIsMac(vmCur.bfHW));

            // initialize Monitor dropdown

            {
            ULONG bf = 1;
            int i = 0, iSel = 0;

            SendDlgItemMessage(hDlg, IDC_COMBOMONITOR, CB_RESETCONTENT, 0, 0);

            while (bf)
                {
                if (bf & vmCur.pvmi->wfMon)
                    {
                    if (bf == vmCur.bfMon)
                        iSel = i;

                    SendDlgItemMessage(hDlg, IDC_COMBOMONITOR, CB_ADDSTRING, 0,
                         (long)PchFromBfRgSz(bf, rgszMon));
                    i++;
                    }
                bf <<= 1;
                }

            SendDlgItemMessage(hDlg, IDC_COMBOMONITOR, CB_SETCURSEL, iSel, 0);
            }

            // initialize CPU dropdown

            {
            ULONG bf = 1;
            int i = 0, iSel = 0;

            SendDlgItemMessage(hDlg, IDC_COMBOCPU, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_COMBOCPU, CB_ADDSTRING, 0, (long)"Auto");

            // count the number of valid CPU choices so that we can determine
            // whether to enable the CPU selector, and also determine the list
            // box offset of the current CPU mode (if it isn't Auto (-1))

            while (bf)
                {
                if (bf & vmCur.pvmi->wfCPU)
                    {
                    if (bf == (vi.fFake040 ? cpu68040 : vmCur.bfCPU))
                        iSel = i;

                    SendDlgItemMessage(hDlg, IDC_COMBOCPU, CB_ADDSTRING, 0,
                         (long)PchFromBfRgSz(bf, rgszCPU));
                    i++;
                    }
                bf <<= 1;
                }

            if (!vmCur.fCPUAuto)
                SendDlgItemMessage(hDlg, IDC_COMBOCPU, CB_SETCURSEL, iSel+1, 0);
            else
                SendDlgItemMessage(hDlg, IDC_COMBOCPU, CB_SETCURSEL, 0, 0);

            EnableWindow(GetDlgItem(hDlg, IDC_COMBOCPU), i > 1);
            }

            // initialize the OS dropdown listing only the OSes appropriate for this VM

            ListROMs(hDlg, vmCur.pvmi->wfROM);
            {
            int i, j = -1;

            for (i = 0; i <= vmCur.iOS; i++)
                {
                if (v.rgosinfo[i].osType & vmCur.pvmi->wfROM)
                    j++;
                }

            SendDlgItemMessage(hDlg, IDC_COMBOTOSLIST, CB_SETCURSEL, j, 0);
            }

            return (TRUE);

        case WM_COMMAND:
            // item notifications

            switch (LOWORD(uParam))
                {
            default:
                break;

            case IDDISKPROPS:
                // disable the caller

                EnableWindow(hDlg,FALSE);

                DialogBox(vi.hInst,
                    MAKEINTRESOURCE(IDD_DISKS),
                    hDlg,
                    DisksDlg);

                // re-enable the caller

                EnableWindow(hDlg,TRUE);
                return (TRUE);
                break;

            case IDSAVE:

                fSaveSettings = TRUE;

                // fall through

            case IDOK:
                DebugStr("ID_OK\n");

                // Now read the status of the check boxes and radio buttons

                vmCur.fJoystick = IsDlgButtonChecked(hDlg, IDC_JOYSTICK);
                vmCur.fMIDI     = 0; //IsDlgButtonChecked(hDlg, IDC_MIDI);
//                v.fSTESound = 0; //IsDlgButtonChecked(hDlg, IDC_STESOUND);

                vmCur.bfMon = SendDlgItemMessage(hDlg, IDC_COMBOMONITOR, CB_GETCURSEL, 0, 0);
                vmCur.bfMon = BfFromWfI(vmCur.pvmi->wfMon, vmCur.bfMon);

                if (!FIsMac(vmCur.bfHW) || (vmCur.bfHW < vmMacII))
                    vsthw.fMono    = FMonoFromBf(vmCur.bfMon);

                vmCur.bfRAM = SendDlgItemMessage(hDlg, IDC_COMBOMEM, CB_GETCURSEL, 0, 0);
                vmCur.bfRAM = BfFromWfI(vmCur.pvmi->wfRAM, vmCur.bfRAM);
                vi.cbRAM[0] = CbRamFromBf(vmCur.bfRAM);

                vmCur.fUseVHD = !IsDlgButtonChecked(hDlg, IDC_VHD);

                vmCur.iBootDisk = (BYTE)SendDlgItemMessage(hDlg, IDC_BOOTLIST, CB_GETCURSEL, 0, 0);

                vmCur.iCOM = (BYTE)SendDlgItemMessage(hDlg, IDC_COMBOCOM, CB_GETCURSEL, 0, 0);
                vmCur.iLPT = (BYTE)SendDlgItemMessage(hDlg, IDC_COMBOLPT, CB_GETCURSEL, 0, 0);
                vmCur.bfCPU = (BYTE)SendDlgItemMessage(hDlg, IDC_COMBOCPU, CB_GETCURSEL, 0, 0);

                vmCur.fCPUAuto = (vmCur.bfCPU == 0);

                if (vmCur.fCPUAuto)
                    vmCur.bfCPU = BfFromWfI(vmCur.pvmi->wfCPU, 0);
                else
                    vmCur.bfCPU = BfFromWfI(vmCur.pvmi->wfCPU, vmCur.bfCPU - 1);

                vmCur.fShare = IsDlgButtonChecked(hDlg, IDC_SHARE);
                vmCur.fSound  = IsDlgButtonChecked(hDlg, IDC_STSOUND);

                vmCur.fSwapKeys = IsDlgButtonChecked(hDlg, IDC_SWAPUKUS);

                vmCur.iOS = SendDlgItemMessage(hDlg, IDC_COMBOTOSLIST, CB_GETCURSEL, 0, 0);
                if ((vmCur.iOS < 0) || (vmCur.iOS >= (int)v.cOS))
                    vmCur.iOS = -1;
                else
                    {
                    // We do the opposite here of how we generated the dropdown of OSes.
                    // Walk rgosinfo and find the ith entry that is valid for this VM.

                    unsigned i;

                    for (i = 0; i < v.cOS; i++)
                        {
                        if (v.rgosinfo[i].osType & vmCur.pvmi->wfROM)
                            if (vmCur.iOS-- == 0)
                                {
                                vmCur.iOS = i;
                                break;
                                }
                        }
                    }

                GetDlgItemText(hDlg, IDC_EDITNAME, &vmCur.szModel,
                    sizeof(rgvm[0].szModel));

                if (fSaveSettings)
                    SaveProperties(NULL);

                   EndDialog(hDlg, TRUE);

                // certain settings don't require rebooting, such as
                // DirectX, Fast Refresh, and Zoom

                vmOld.fSound    = vmCur.fSound;
                vmOld.fJoystick = vmCur.fJoystick;
                vmOld.fShare    = vmCur.fShare;
                vmOld.iBootDisk = vmCur.iBootDisk;
                vmOld.fSwapKeys = vmCur.fSwapKeys;
                vmOld.fUseVHD   = vmCur.fUseVHD;

                memcpy(vmOld.szModel, vmCur.szModel, sizeof(vmOld.szModel));

                if (memcmp(&vmCur, &vmOld, sizeof(VM)))
                    {
                    if (FVerifyMenuOption())
                        {
                        vmCur.fColdReset = TRUE;
                        }
                    else
                        {
                        vmCur = vmOld;
                        break;
                        }
                    }

                CreateNewBitmap();

                return (TRUE);
                break;

            case IDCANCEL:
                DebugStr("ID_CANCEL\n");

                // restore global settings

                vmCur = vmOld;

                EndDialog(hDlg, TRUE);
                return (TRUE);
                break;
                }

            break;
        }

    return (FALSE); // Didn't process the message

    lParam; // This will prevent 'unused formal parameter' warnings
}


void ListVirtualDrives(HWND hDlg)
{
    static char const * const rgszDiskStatus[] =
        {
        "is disabled",
        "is mapped to floppy disk ",
        "is mapped to SCSI device ",
        "is mapped to disk file ",
        "is mapped to disk "
        };

    // Stuff description of all detected ROMs to given dialog box

    int i;
    char rgch[256];
    static PVD pvd;

    SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_RESETCONTENT, 0, 0);

    for (i = 0; i < vmCur.ivdMac; i++)
        {
        rgch[0] = 0;

        pvd = &vmCur.rgvd[i];

        if (FIsMac(vmCur.bfHW))
            {
            // Mac has two floppies and up to 7 SCSI devices

            if (i == 0)
                sprintf(rgch, "Mac internal floppy ");
            else if (i == 1)
                sprintf(rgch, "Mac external floppy ");
            else
                sprintf(rgch, "Mac CD-ROM or SCSI disk %d ", i - 2);
            }
        else if (FIsAtari68K(vmCur.bfHW))
            {
            // Atari has two floppies, 4 hard disk partitions on SCSI 0
            // and up to three more SCSI drives

            if (i == 0)
                sprintf(rgch, "Atari ST internal floppy ");
            else if (i == 1)
                sprintf(rgch, "Atari ST external floppy ");
            else
                sprintf(rgch, "Atari ST hard disk or SCSI device %d ", i - 2);

            }
        else
            {// Atari 8-bit has 9 drives
            // only 8 now, don't execute this and crash
            sprintf(rgch, "Atari 8-bit disk drive D%c: ", '1' + i);
            }

        strcat(rgch, rgszDiskStatus[pvd->dt]);

        if (pvd->dt == DISK_IMAGE)
            {
            strcat(rgch, pvd->sz);
            }
        else if ((pvd->dt == DISK_WIN32) || (pvd->dt == DISK_FLOPPY))
            {
            sprintf(rgch + strlen(rgch), "drive %c:", pvd->id + 'A');
            }
        else if (pvd->dt == DISK_SCSI)
            {
            PDI pdi = PdiOpenDisk(DISK_SCSI,
                 LongFromScsi(pvd->ctl, pvd->id), DI_QUERY);

            if (pdi)
                {
                strcat(rgch, pdi->szVolume);
                CloseDiskPdi(pdi);
                }
            }

        SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_ADDSTRING, 0, (LPARAM)rgch);
        }
}


/****************************************************************************

    FUNCTION: DiskModeDlg(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Disk Mode" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

void RefreshDiskProps(HWND hDlg, int iDisk)
{
    char rgch[MAX_PATH];
    PVD pvd;
    VD vd;
    PDI pdi;
    char rgch2[40];

#if 0
    static ULONG const rglDiskSize[] =
        {
        360, 400, 720, 800, 1440, 2880,
        16*1024, 32*1024, 64*1024, 100*1024,
        128*1024, 256*1024, 512*1024, 1024*1024, 2000*1024,
        0 };
#endif

    pvd = &vmCur.rgvd[iDisk];
    vd = *pvd;

    CheckRadioButton(hDlg, IDC_DISABLED, IDC_IMAGE,
        IDC_DISABLED + ((pvd->dt == DISK_WIN32) ? DISK_FLOPPY : pvd->dt));

    EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DISKRW2), !FIsAtari8bit(vmCur.bfHW));
    EnableWindow(GetDlgItem(hDlg, IDC_RADIO_DISKRW3), FIsMac(vmCur.bfHW));

    CheckRadioButton(hDlg, IDC_RADIO_DISKRW, IDC_RADIO_DISKRW3,
        IDC_RADIO_DISKRW + pvd->mdWP);

    SetDlgItemText(hDlg, IDC_EDITIMAGEPATH, pvd->sz);

    // set up disk sizes combo

    SendDlgItemMessage(hDlg, IDC_DISKSIZE, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(hDlg, IDC_DISKSIZE, CB_ADDSTRING, 0, (LPARAM)"Auto");
    SendDlgItemMessage(hDlg, IDC_DISKSIZE, CB_SETCURSEL, 0, 0);

#if 0
    for (i = 0; i < sizeof(rglDiskSize) / sizeof(ULONG) - 1; i++)
        {
        BYTE rgch[16];

        if (rglDiskSize[i] < 16*1024)
            sprintf(rgch, "%dK", rglDiskSize[i]);
        else
            sprintf(rgch, "%dM", rglDiskSize[i] / 1024);

        SendDlgItemMessage(hDlg, IDC_DISKSIZE, CB_ADDSTRING, 0,
             (LPARAM)rgch);
        }
#endif

    // SCSI is only enabled is any adapters found

    if (!v.fNoSCSI && !FIsAtari8bit(vmCur.bfHW))
        {
        EnableWindow(GetDlgItem(hDlg, IDC_SCSI), (NumAdapters > 0));
        EnableWindow(GetDlgItem(hDlg, IDBROWSESCSI), (NumAdapters > 0));

        rgch2[0] = '\0';
        pdi = PdiOpenDisk(DISK_SCSI, LongFromScsi(vd.ctl, vd.id), DI_QUERY);
        if (pdi)
            {
            strcpy(rgch2, pdi->szVolume);
            CloseDiskPdi(pdi);
            }
        sprintf(rgch, "Host %d Id %d %s", vd.ctl, vd.id, rgch2);
        SetDlgItemText(hDlg, IDC_EDITSCSI, rgch);
        }
    else
        {
        EnableWindow(GetDlgItem(hDlg, IDC_SCSI), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDBROWSESCSI), FALSE);
        SetDlgItemText(hDlg, IDC_EDITSCSI, "");
        }

    // On Atari ST, disable floppies if disk index >= 2

    if (FIsAtari68K(vmCur.bfHW) && (iDisk > 2))
        {
        EnableWindow(GetDlgItem(hDlg, IDC_FLOPPY), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDBROWSEFLOPPY), FALSE);
        }
    else
        {
        EnableWindow(GetDlgItem(hDlg, IDC_FLOPPY), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDBROWSEFLOPPY), TRUE);
        }

    sprintf(rgch, "Drive %c:", vd.id + 'A');
    SetDlgItemText(hDlg, IDC_EDITFLOPPY, rgch);

}


/****************************************************************************

    FUNCTION: DisksDlg(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages for "Disk Properties" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT CALLBACK DisksDlg(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    int i;
    PVD pvd;
    PDI pdi;
    char rgch2[40];
    char rgch[80];

    switch (message)
        {
        case WM_INITDIALOG:
            // initialize dialog box

            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

            // initialize drive list

            ListVirtualDrives(hDlg);
            SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_SETCURSEL, 0, 0);
            RefreshDiskProps(hDlg, 0);
            return (TRUE);

        case WM_COMMAND:
            // item notifications

            i = SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_GETCURSEL, 0, 0);
            pvd = &vmCur.rgvd[i];

            switch (LOWORD(uParam))
                {
            default:
                break;

            case IDC_DISKLIST:
                if (HIWORD(uParam) == LBN_DBLCLK)
                    goto Ldiskimg;

                if (HIWORD(uParam) != LBN_SELCHANGE)
                    break;

                i = SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_GETCURSEL, 0, 0);

                ListVirtualDrives(hDlg);
                SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_SETCURSEL, i, 0);
                RefreshDiskProps(hDlg, i);
                SetFocus(GetDlgItem(hDlg, IDC_DISKLIST));
                break;

            case IDBROWSEFLOPPY:
                if (FIsAtari8bit(vmCur.bfHW))
                    break;

                DebugStr("IDBROWSEFLOPPY\n");

                // force floppy to be selected

                CheckRadioButton(hDlg, IDC_DISABLED, IDC_IMAGE,
                    IDC_DISABLED + DISK_FLOPPY);

Lnextdrive:
                pvd->id = (pvd->id + 1) % 26;

                if (vi.fWinNT && (pvd->id >= 2))
                    {
                    char sz[4];
                    int type;

                    sz[0] = 'A' + pvd->id;
                    sz[1] = ':';
                    sz[2] = '\\';
                    sz[3] = 0;

                    type = GetDriveType(sz);

                    if (type != DRIVE_CDROM)
                        goto Lnextdrive;
                    }
                else
                    {
                    pvd->id &= 1;
                    }

                sprintf(rgch, "Drive %c:", pvd->id + 'A');
                SetDlgItemText(hDlg, IDC_EDITFLOPPY, rgch);

                goto Lupdlistbox;

            case IDBROWSESCSI:
                if (!v.fNoSCSI && FIsAtari8bit(vmCur.bfHW))
                    break;

                DebugStr("IDBROWSESCSI\n");

                // force SCSI to be selected

                CheckRadioButton(hDlg, IDC_DISABLED, IDC_IMAGE,
                    IDC_DISABLED + DISK_SCSI);

                {
                int ctl = pvd->ctl, id = pvd->id;

                rgch2[0] = '\0';

                do
                    {
                    if (++id >= 16)
                        {
                        id = 0;

                        if (++ctl >= NumAdapters)
                            ctl = 0;
                        }

                    pdi = PdiOpenDisk(DISK_SCSI,
                         LongFromScsi(ctl, id), DI_QUERY);
                    if (pdi)
                        {
                        strcpy(rgch2, pdi->szVolume);
                        CloseDiskPdi(pdi);
                        pvd->ctl = ctl;
                        pvd->id = id;
                        break;
                        }
                    } while ((ctl != pvd->ctl) || (id != pvd->id));
                }

                sprintf(rgch, "Host %d Id %d %s", pvd->ctl, pvd->id, rgch2);
                SetDlgItemText(hDlg, IDC_EDITSCSI, rgch);

                goto Lupdlistbox;

            case IDBROWSEDISK:
                DebugStr("IDCHANGEDISK\n");

Ldiskimg:
                // force disk image to be selected

                CheckRadioButton(hDlg, IDC_DISABLED, IDC_IMAGE,
                    IDC_DISABLED + DISK_IMAGE);

                FUnmountDiskVM(i);

                // disable the caller

                EnableWindow(hDlg,FALSE);

                OpenTheFile(hDlg, (void *)pvd->sz, fFalse);

                // re-enable the caller

                EnableWindow(hDlg,TRUE);

                FMountDiskVM(i);

                ListVirtualDrives(hDlg);
                SetDlgItemText(hDlg, IDC_EDITIMAGEPATH, (void*)pvd->sz);

Lupdlistbox:
                if (IsDlgButtonChecked(hDlg, IDC_IMAGE))
                    pvd->dt = DISK_IMAGE;
                else if (IsDlgButtonChecked(hDlg, IDC_FLOPPY))
                    pvd->dt = (pvd->id >= 2) ? DISK_WIN32 : DISK_FLOPPY;
                else if (IsDlgButtonChecked(hDlg, IDC_SCSI))
                    pvd->dt = DISK_SCSI;
                else
                    pvd->dt = DISK_NONE;

                {
                HCURSOR h = SetCursor(LoadCursor(NULL, IDC_WAIT));

                FUnmountDiskVM(i);
                FMountDiskVM(i);

                ListVirtualDrives(hDlg);
                SendDlgItemMessage(hDlg, IDC_DISKLIST, LB_SETCURSEL, i, 0);
                }
                break;

            case IDC_DISABLED:
            case IDC_FLOPPY:
            case IDC_SCSI:
            case IDC_IMAGE:
                goto Lupdlistbox;
                break;

            case IDC_RADIO_DISKRW:      // read-write
            case IDC_RADIO_DISKRW2:     // read-only
            case IDC_RADIO_DISKRW3:     // CD-ROM
                pvd->mdWP = LOWORD(uParam) - IDC_RADIO_DISKRW;
                break;

            case IDOK:
                DebugStr("ID_OK\n");

#ifdef SOFTMAC
                vmachw.fDisk1Dirty = fTrue;
                vmachw.fDisk2Dirty = fTrue;
#endif
                // vsthw.fMediaChange = fTrue;

                   EndDialog(hDlg, TRUE);
                return (TRUE);
                break;

            case IDCANCEL:
                DebugStr("ID_CANCEL\n");

#ifdef SOFTMAC
                vmachw.fDisk1Dirty = fTrue;
                vmachw.fDisk2Dirty = fTrue;
#endif
                // vsthw.fMediaChange = fTrue;

                EndDialog(hDlg, TRUE);
                return (TRUE);
                break;
                }

            break;
        }

    return (FALSE); // Didn't process the message

    lParam; // This will prevent 'unused formal parameter' warnings
}

void HandleDiskEject(int i)
{
#if 0
    iDisk = i;

    // disable the caller

    EnableWindow(vi.hWnd,FALSE);

#if 0
    DialogBox(vi.hInst,
        MAKEINTRESOURCE(IDD_DISKMODE),
        vi.hWnd,
        DiskModeDlg);
#endif

    // re-enable the caller

    EnableWindow(vi.hWnd,TRUE);

    // Open the new disk images

    FUnInitDisksVM();
    FInitDisksVM();
#endif
}

int AutoConfigure(BOOL fShowDlg)
{
    PROPS vTmp = v;
    char rgch[MAX_PATH];
    char sz[MAX_PATH];
    BOOL f = fFalse;

    sz[0] = 0;

    if (fShowDlg)
    if (IDOK != MessageBox (GetFocus(),
            "WARNING: Auto Configure will create default "
            "profilies. All settings will be reset!\n\n "
            "Select OK to proceed."
            , vi.szAppName, MB_OKCANCEL))
        return 0;

    InitProperties();           // clear everything

#if 0
    // preserve the ROM path if there was one

    if (*(char *)(&vTmp.rgchGlobalPath))
        strcpy((char *)(&v.rgchGlobalPath), (char *)(&vTmp.rgchGlobalPath));
#endif

    SaveProperties(NULL);       // save it to the INI file
    LoadProperties(NULL);       // load the INI file so that it sets up globals
    vpvm = vmCur.pvmi;

#if defined(ATARIST) || defined(SOFTMAC)
    if (!fShowDlg)
        {
        WScanROMs(FALSE, TRUE, FALSE, FALSE);

        SaveProperties(NULL);      // save it to the INI file again
        return TRUE;
        }

    // Do First Time Setup (now called ROM BIOS Setup)

    DialogBox(vi.hInst,       // current instance
        MAKEINTRESOURCE(IDD_FIRSTTIME), // dlg resource to use
        vi.hWnd,              // parent handle
        FirstTimeProc);

    SaveProperties(NULL);      // save it to the INI file again

    if (!v.fNoSCSI)
        {
        PDI pdi;
        int ctl = 0, id = -1;
        int cDrives = 0;

        for(;;)
            {
            if (++id >= 16)
                {
                id = 0;

                if (++ctl >= NumAdapters)
                    {
                    id = -1;
                    break;
                    }
                }

            pdi = PdiOpenDisk(DISK_SCSI,
                 LongFromScsi(ctl, id), DI_QUERY);

            if (pdi)
                {
                int i;
                ULONG cb = pdi->cbSector;

                CloseDiskPdi(pdi);

                if (cb <= 512)
                    continue;

                if ((cDrives == 0) && (IDOK != MessageBox (GetFocus(),
                    "Do you wish to add your CD-ROM drive(s) to all "
                    "Macintosh modes?",
                    vi.szAppName, MB_OKCANCEL)))
                    {
                    break;
                    }

                for (i = 0; i < v.cVM; i++)
                    {
                    // 24-bit Macs only get one CD-ROM drive because ROMs suck

                    if (!FIsMac32(rgvm[i].bfHW) && (cDrives > 0))
                        continue;

                    if (FIsMac(rgvm[i].bfHW))
                        {
                        rgvm[i].rgvd[8-cDrives].mdWP = 2;
                        rgvm[i].rgvd[8-cDrives].dt = DISK_SCSI;
                        rgvm[i].rgvd[8-cDrives].ctl = ctl;
                        rgvm[i].rgvd[8-cDrives].id = id;
                        }
                    }

                cDrives++;

                // ok, 4 drives is enough!

                if (cDrives == 4)
                    break;

                }
            }
        }

#endif // ATARIST

    PostMessage(vi.hWnd, WM_COMMAND, IDM_COLDSTART, 0);

    return TRUE;
}


/****************************************************************************

    FUNCTION: ChooseProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Displays a list box to choose a selection

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT CALLBACK ChooseProc(
        HWND hDlg,       // window handle of the dialog box
        UINT message,    // type of message
        WPARAM uParam,       // message-specific information
        LPARAM lParam)
{
    switch (message)
        {
        case WM_INITDIALOG:
            // initialize dialog box

            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

            SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_RESETCONTENT, 0, 0);

            switch(lParam)
                {
            default:
            case 0:
            case 1:
                SetWindowText(hDlg, "Select the size of disk image to create");

                SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_ADDSTRING, 0,
                     (long)"1.44 megabytes (size of floppy disk)");
                SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_ADDSTRING, 0,
                     (long)"30 megabytes (size of small hard disk)");
                SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_ADDSTRING, 0,
                     (long)"100 megabytes (size of ZIP disk)");
                SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_ADDSTRING, 0,
                     (long)"250 megabytes (size of typical hard disk)");

                if (lParam == 1)
                    {
                    SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_ADDSTRING, 0,
                         (long)"650 megabytes (roughly the size of a CD-ROM)");
                    SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_ADDSTRING, 0,
                         (long)"2 gigabytes (maximum size of older hard disks)");
                    }
                break;
                }

            SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_SETCURSEL, 0, 0);

            return (TRUE);

        case WM_COMMAND:
            // item notifications

            switch (LOWORD(uParam))
                {
            default:
                break;

            case IDOK:
                switch(SendDlgItemMessage(hDlg, IDC_LBCHOOSE, CB_GETCURSEL, 0, 0))
                    {
                default:
                case 0:
                    lParam = 1440;
                    break;

                case 1:
                    lParam = 30 * 1024;
                    break;

                case 2:
                    lParam = 96 * 1024;
                    break;

                case 3:
                    lParam = 250 * 1024;
                    break;

                case 4:
                    lParam = 640 * 1024;
                    break;

                case 5:
                    lParam = 2000 * 1024;
                    break;
                    }

                   EndDialog(hDlg, lParam);
                return (TRUE);
                break;

            case IDCANCEL:
                EndDialog(hDlg, FALSE);
                return (TRUE);
                break;
                }

            break;
        }

    return (FALSE); // Didn't process the message
}
#endif


