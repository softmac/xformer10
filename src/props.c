
/****************************************************************************

    PROPS.C

    - Dialog handler for Properties, load and save properties
    - Also handles the Disks Properties dialog.

    Copyright (C) 1991-2008 by Darek Mihocka. All Rights Reserved.
    Branch Always Software. http://www.emulators.com/

    11/30/2008  darekm      Gemulator 9.0 release
    03/03/2008  darekm      silent first time autoconfig for EmuTOS
    
****************************************************************************/

#include "gemtypes.h"

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


//
// Add a new virtual machine to v.rgvm - return which instance it found
//

BOOL AddVM(PVMINFO pvmi, int *pi, int type)
{
    int i;
    BOOL f;

    // first try to find an empty slot in the rgvm array
    for (i = 0; i < MAX_VM; i++)
    {
        if (!v.rgvm[i].fValidVM)
            break;
    }

    // did not find an empty slot!
	if (i == MAX_VM)
	{
		assert(FALSE);
		return FALSE;
	}

    // i is now an index to an empty VM

	v.cVM++;
    if (pi)
        *pi = i;

	// our jump table for the routines. FInstallVM will crash if we don't do this first
	v.rgvm[i].pvmi = pvmi;

	f = FInstallVM(i, pvmi, type);
	v.rgvm[i].fValidVM = f;
	return f;
}

void DeleteVM(int iVM)
{
	if (!v.rgvm[iVM].fValidVM)
		return;

	vrgvmi[iVM].fInited = FALSE;	// invalidate the non-persistable data
	SelectObject(vrgvmi[iVM].hdcMem, vrgvmi[iVM].hbmOld);
	DeleteObject(vrgvmi[iVM].hbm);
	DeleteDC(vrgvmi[iVM].hdcMem);
	FUnInitVM(iVM);
	memset(&v.rgvm[iVM], 0, sizeof(VM));	// erase the persistable data AFTER UnInit please
	v.rgvm[iVM].fValidVM = FALSE;
	v.cVM--;	// one fewer valid instance now
}

//
// First time initialization of global properties
//

BOOL InitProperties()
{
	memset(&v, 0, sizeof(v));

	v.fPrivate = fTrue;         // new style settings
	v.wMagic = 0x82201233; // some unique value

	// all fields default to 0 or fFalse except for these...

	v.cb = sizeof(PROPS);
	v.ioBaseAddr = 0x240;
	v.cCards = 1;

	v.fNoDDraw = 2;      // on NT 4.0 some users won't be able to load DirectX
	v.fZoomColor = fFalse; // make the window large
	v.fNoMono = fTrue;  // in case user is running an ATI video card
	v.fSaveOnExit = fTrue;  // automatically save settings on exit

	// check for original Windows 95 which had buggy ASPI drivers

	OSVERSIONINFO oi;

	oi.dwOSVersionInfoSize = sizeof(oi);
	GetVersionEx(&oi);

	switch (oi.dwPlatformId)
	{
	default:
		//    case VER_PLATFORM_WIN32_WINDOWS:
		if ((oi.dwBuildNumber & 0xFFFF) == 950)
			v.fNoSCSI = TRUE;   // Windows 95 has boned SCSI support
		break;

	case VER_PLATFORM_WIN32s:
	case VER_PLATFORM_WIN32_NT:
		break;
	}

	// Initialize default ROM directory to current directory

	strcpy((char *)&v.rgchGlobalPath, (char *)&vi.szDefaultDir);

	return TRUE;
}

// Add a menu item to actually do this?
BOOL CreateAllVMs()
{
	int vmNew;

	// make one of everything at the same time
#ifdef XFORMER

    // Create and initialize the default Atari 8-bit VMs

	AddVM(&vmi800, &vmNew, vmAtari48);
	FInitVM(vmNew);

    AddVM(&vmi800, &vmNew, vmAtariXL);
	FInitVM(vmNew);

	AddVM(&vmi800, &vmNew, vmAtariXE);
	FInitVM(vmNew);
#endif

#ifdef ATARIST

    // initialize the default Atari ST VM (68000 mode)

    AddVM(&vmiST, NULL);
    strcpy(v.rgvm[v.cVM-1].szModel, rgszVM[5]);

    // initialize the default Atari ST VM (68030 mode)

    AddVM(&vmiST, NULL);
    strcpy(v.rgvm[v.cVM-1].szModel, rgszVM[5]);
    strcat(v.rgvm[v.cVM-1].szModel, "/030");
    v.rgvm[v.cVM-1].fCPUAuto = FALSE;
    v.rgvm[v.cVM-1].bfCPU = cpu68030;
#endif

#ifdef SOFTMAC
    // initialize the default Mac Plus VM

    AddVM(&vmiMac, NULL);
#endif

#ifdef SOFTMAC2
    // initialize the default color Mac VMs

    AddVM(&vmiMacII, NULL);

    AddVM(&vmiMacQdra, NULL);

#ifdef POWERMAC
    // initialize the default Power Mac VMs

    AddVM(&vmiMacPowr, NULL);
    AddVM(&vmiMacG3, NULL);

#endif  // POWERMAC
#endif  // SOFTMAC2

    return TRUE;
}


BOOL OpenTheINI(HWND hWnd, char *psz)
{
    OPENFILENAME OpenFileName;

    // Fill in the OPENFILENAME structure to support a template and hook.
    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = NULL;
    OpenFileName.lpstrFilter   =
             "Configuration Files (*.cfg;*.ini)\0*.cfg;*.ini\0All Files\0*.*\0\0";
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = psz;
    OpenFileName.nMaxFile          = MAX_PATH;
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = "Select Configuration File";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = 0;
    OpenFileName.lpfnHook            = NULL;
    OpenFileName.lpTemplateName    = NULL;
    OpenFileName.Flags             = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST \
                                                  | OFN_CREATEPROMPT;

    // Call the common dialog function.
    if (GetOpenFileName(&OpenFileName))
        {
        strcpy(psz, OpenFileName.lpstrFile);
        return TRUE;
        }

    return FALSE;
}


//
// Read properties from INI file and/or registry
//
// Returns TRUE if valid data loaded
//

BOOL LoadProperties(HWND hOwner)
{
    PROPS vTmp;
    char rgch[MAX_PATH];
    char sz[MAX_PATH];
    BOOL f = fFalse;
    BOOL fTriedWindows = fFalse;
        
    sz[0] = 0;

    GetCurrentDirectory(sizeof(rgch), rgch);
    SetCurrentDirectory(vi.szWindowsDir);

    if (hOwner != NULL)
        {
        EnableWindow(hOwner,FALSE);
        OpenTheINI(hOwner, sz);
        EnableWindow(hOwner,TRUE);
        }

    vTmp.cb = 0;

LTryAgain:
    f = sizeof(vTmp) == 
        CbReadWholeFile((hOwner != NULL) ? sz : szIniFile, sizeof(vTmp), &vTmp);

    if (!f && !fTriedWindows)
        {
        // try the actual C:\WINDOWS directory

        char sz2[MAX_PATH];

        GetWindowsDirectory(&sz2, MAX_PATH);
        SetCurrentDirectory(sz2);
        fTriedWindows = fTrue;

        goto LTryAgain;
        }

    // if INI file contained valid data, use it
    // otherwise make the user give you data

    if ((vTmp.cb == sizeof(vTmp)) && (vTmp.wMagic == v.wMagic))
        {
        v = vTmp;
        f = fTrue;
        }

    // non-persistable pointers need to be refreshed
	// and only use VM's that we can handle in this build

	int i;
	v.cVM = 0;
	for (i = 0; i < MAX_VM; i++)
	{
		v.rgvm[i].pvmi = NULL;

#ifdef XFORMER
		// old saved PROPERTIES used out of date VMs
		if (v.rgvm[i].bfHW == vmAtari48C)
			v.rgvm[i].bfHW = vmAtari48;
		if (v.rgvm[i].bfHW == vmAtariXLC)
			v.rgvm[i].bfHW = vmAtariXL;
		if (v.rgvm[i].bfHW == vmAtariXEC)
			v.rgvm[i].bfHW = vmAtariXE;

		if (v.rgvm[i].fValidVM && FIsAtari8bit(v.rgvm[i].bfHW)) {
			v.rgvm[i].pvmi = (PVMINFO)&vmi800;
			//			if (i == 0) {
			//				v.rgvm[i].iOS = 0;
			//				v.rgvm[i].bfRAM = ram48K;
			//				v.rgvm[i].bfHW = 1;
			//			}
		}
#endif

#ifdef ATARIST
		if (FIsAtari68K(v.rgvm[i].bfHW))
			v.rgvm[i].pvmi = (PVMINFO)&vmiST;
#endif

#ifdef SOFTMAC
		if (FIsMac16(v.rgvm[i].bfHW))
			v.rgvm[i].pvmi = (PVMINFO)&vmiMac;
#endif

#ifdef SOFTMAC2
		if (FIsMac68020(v.rgvm[i].bfHW))
			v.rgvm[i].pvmi = (PVMINFO)&vmiMacII;

		if (FIsMac68030(v.rgvm[i].bfHW))
			v.rgvm[i].pvmi = (PVMINFO)&vmiMacQdra;

		if (FIsMac68040(v.rgvm[i].bfHW))
			v.rgvm[i].pvmi = (PVMINFO)&vmiMacQdra;
#ifdef POWERMAC
		if (FIsMacPPC(v.rgvm[i].bfHW))
			v.rgvm[i].pvmi = (PVMINFO)&vmiMacPowr;
#endif // POWERMAC
#endif // SOFTMAC2

		v.rgvm[i].ivdMac = sizeof(v.rgvm[0].rgvd) / sizeof(VD);

		// we just loaded an instance off disk. Init it.
		if (v.rgvm[i].fValidVM)
		{
			v.cVM++;
			FInitVM(i);
		}
	}
	
	if (strlen((char *)&v.rgchGlobalPath) == 0)
        strcpy((char *)&v.rgchGlobalPath, (char *)&vi.szDefaultDir);

    SetCurrentDirectory(rgch);

    // If we just imported an old style INI file with global VM settings
    // initialize the private VM settings to default values

    if (!v.fPrivate)
        {
        unsigned i;

        for (i = 0; i < MAX_VM; i++)
            {
            // All other settings default to 0 or fFalse
            // but this flag needs to be set by default

            v.rgvm[i].fCPUAuto    = fTrue;
            }

        v.fPrivate = fTrue;
        }
    
    return f && v.cVM;	// must have an instance loaded to count as success
}

//
// If properties are dirty, prompt user to save properties to INI or registry
//

BOOL SaveProperties(HWND hOwner)
{
    BOOL f;
    char rgch[MAX_PATH];
    char sz[MAX_PATH];

    sz[0] = '\0';
        
    GetCurrentDirectory(sizeof(rgch), rgch);
    SetCurrentDirectory(vi.szWindowsDir);

    if (hOwner != NULL)
        {
        EnableWindow(hOwner,FALSE);
        OpenTheINI(hOwner, sz);
        EnableWindow(hOwner,TRUE);
        }

    f = FWriteWholeFile((hOwner != NULL) ? sz : szIniFile, sizeof(v), &v);

    SetCurrentDirectory(rgch);
    return f;
}

BOOL SaveState(BOOL fSave)
{
	return TRUE;
}


#if 0
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
                    sizeof(v.rgvm[0].szModel));

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
            {
            // Atari 8-bit has 9 drives
			// !!! only 8 now, don't execute this and crash
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

                    if (!FIsMac32(v.rgvm[i].bfHW) && (cDrives > 0))
                        continue;

                    if (FIsMac(v.rgvm[i].bfHW))
                        {
                        v.rgvm[i].rgvd[8-cDrives].mdWP = 2;
                        v.rgvm[i].rgvd[8-cDrives].dt = DISK_SCSI;
                        v.rgvm[i].rgvd[8-cDrives].ctl = ctl;
                        v.rgvm[i].rgvd[8-cDrives].id = id;
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


