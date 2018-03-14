/* gieui.c */


#include "precomp.h"
#include "giedrag.h"
#include "strlist.h"


// #define NODRAGDROP


#define WM_GIE_INITIALIZE   (WM_USER + 100)

#define LARGEICONSIZE 32
#define SMALLICONSIZE 16

#define FOLDERIMAGEINDEX   0
#define FILEIMAGEINDEX     1
#define HARDDISKIMAGEINDEX 2
#define ARROWIMAGEINDEX    3

#define IDC_TREEVIEW 1001
#define IDC_LISTVIEW 1002

#define GIEWINDOWCLASS "giewindowclass"


HINSTANCE vhinst;

HWND vhwndMain;
HWND vhwndTreeView;
HWND vhwndListView;

int  vdwListViewStyle;
int  vdwListViewSort; /* LOWORD = {NAME, SIZE, DATE}, HIWORD = {ascending, descending} */
BOOL vfEmptyingTreeView; /* ignore TVN_SELCHANGED if this is TRUE */

HIMAGELIST vhilSmall;
HIMAGELIST vhilLarge;

int  viDriveID;
char vszCurrentPath[256];
char vszCurrentDrive[256];

int  vcxSplitter;
BOOL vfTrackingSplitter;
HDC  vhdcSplitter;
RECT vrcSplitter;

HFILEDROPTGT vhfdtTreeView;


//
// Progress Dialog Library
//

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


BOOL CALLBACK ProgressDlg(HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam);

int iProgress;
char szProg[256];
char szProgTitle[256];

HWND hDlgProg;
HANDLE hProgThread;
BOOL fAbort;

long ProgThread(int l)
{
    MSG msg;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	DialogBox(vhinst, MAKEINTRESOURCE(IDD_PROGRESS), vhwndMain, ProgressDlg);
    return 0;
}

BOOL StartProgress(char *szTitle)
{
    int l;

    iProgress = 0;
    strcpy(szProgTitle, szTitle);
    fAbort = FALSE;
    hDlgProg = NULL;

    hProgThread = CreateThread(NULL, 0, (void *)ProgThread, 0, 0, &l);
	SetThreadPriority(hProgThread, THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(hProgThread, THREAD_PRIORITY_HIGHEST);
//	ResumeThread(hProgThread);

    SetProgress(0, "");
    Sleep(100);
    SetProgress(0, "");

    return TRUE;
}

BOOL SetProgress(int i, char *sz)
{
    MSG msg;

    if (i < 0)
        i = 0;
    else if (i > 100)
        i = 100;

    iProgress = i;
    strcpy(szProg, sz);

    while (PeekMessage(&msg, // message structure
       NULL,   // handle of window receiving the message
       0,      // lowest message to examine
       0,      // highest message to examine
       PM_REMOVE)) // remove the message, as if a GetMessage
        {
        if (msg.message == WM_QUIT)
            return msg.wParam;

        TranslateMessage(&msg);// Translates virtual key codes
        DispatchMessage(&msg); // Dispatches message to window
        }
    return !fAbort;
}

BOOL EndProgress()
{
    KillTimer(hDlgProg, 1);
	EndDialog(hDlgProg, IDCANCEL);

    // make sure the progress thread is really dead
    WaitForSingleObject(hProgThread, INFINITE);

    return TRUE;
}

BOOL CALLBACK ProgressDlg(HWND hDlg, UINT message, WPARAM uParam, LPARAM lParam)
{
	switch (message)
    	{
        case WM_INITDIALOG:
            hDlgProg = hDlg;
			SetTimer(hDlg, 1, 250, NULL);
            SetWindowText(hDlg, szProgTitle);

            CenterWindow(hDlg, GetWindow (hDlg, GW_OWNER));
            CenterWindow(hDlg, vhwndMain);
            SetForegroundWindow(hDlg);
            BringWindowToTop(hDlg);
			// break;

		case WM_TIMER:
			SendDlgItemMessage(hDlg, IDC_PROGRESSBAR, PBM_SETPOS, iProgress, 0);
            SetWindowText(GetWindow(hDlg, IDC_PROGRESSDESCRIPTION), szProg);
            SendDlgItemMessage(hDlg, IDC_PROGRESSDESCRIPTION, WM_SETTEXT, 0, szProg);
			break;

		case WM_COMMAND:
			// test for abort here--if so kill timer
			if (LOWORD(uParam) == IDCANCEL)
                fAbort = TRUE;
			break;
		}
	
	return FALSE;
}


//
//
//

void SetWindowTitle(HWND hwnd)
{
    char szLoad[256], szFullTitle[256];
	int cch;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));

	cch = strlen(vszCurrentPath);
	if (cch > 0)
	{
		/* there is a path, use it */
		strcpy(szFullTitle, vszCurrentPath);
		szFullTitle[cch - 1] = '\0'; /* remove last delimiter */
	}
	else
	{
        ASSERT(strlen(vszCurrentDrive) > 0);

		/* we're at the root of a drive */
		if (LoadString(vhinst, IDS_ROOTDRIVETITLE, szLoad, sizeof(szLoad)) > 0)
			wsprintf(szFullTitle, szLoad, vszCurrentDrive);
		else
			szFullTitle[0] = '\0';
	}

    if (LoadString(vhinst, IDS_APPENDTOTITLE, szLoad, sizeof(szLoad)) > 0)
        strcat(szFullTitle, szLoad);

    SetWindowText(hwnd, szFullTitle);
}


BOOL FAddIconToImageLists(int idi)
{
    HICON hIcon;

    hIcon = LoadIcon(vhinst, MAKEINTRESOURCE(idi));
    if (hIcon == NULL)
        return FALSE;
    if (ImageList_AddIcon(vhilSmall, hIcon) == -1 || ImageList_AddIcon(vhilLarge, hIcon) == -1)
        return FALSE;

    return TRUE;
}


BOOL FLoadImages()
{
    ASSERT(vhilSmall == NULL);
    ASSERT(vhilLarge == NULL);

    vhilSmall = ImageList_Create(SMALLICONSIZE, SMALLICONSIZE, ILC_MASK, 4, 0);
    vhilLarge = ImageList_Create(LARGEICONSIZE, LARGEICONSIZE, ILC_MASK, 4, 0);
    if (vhilSmall == NULL || vhilLarge == NULL)
        return FALSE;

    FAddIconToImageLists(IDI_FOLDER);
    FAddIconToImageLists(IDI_FILE);
    FAddIconToImageLists(IDI_HARDDISK);
	FAddIconToImageLists(IDI_ARROW);

    if (ImageList_GetImageCount(vhilSmall) != 4 || ImageList_GetImageCount(vhilLarge) != 4)
        return FALSE;

    return TRUE;
}


BOOL FCreateTreeView(HWND hwndParent)
{
    RECT rc;

    ASSERT(vhwndTreeView == NULL);

    GetClientRect(hwndParent, &rc);
    vhwndTreeView  = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
                         TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES |
                         TVS_SHOWSELALWAYS | WS_VISIBLE | WS_CHILD | WS_BORDER,
                         0, 0, rc.right / 3, rc.bottom, hwndParent, (HMENU)IDC_TREEVIEW,
                         vhinst, NULL);
    if (vhwndTreeView == NULL)
        return FALSE;

    TreeView_SetImageList(vhwndTreeView, vhilSmall, 0);
    return TRUE;
}


BOOL FAddColumnToListView(int ids, int fmt, int cx, int iColumn)
{
    char szText[256];
    LV_COLUMN lvc;

    if (LoadString(vhinst, ids, szText, sizeof(szText)) == 0)
        return FALSE;
    
    FillMemory(&lvc, sizeof(LV_COLUMN), 0);
    lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt      = fmt;
    lvc.cx       = cx;
    lvc.pszText  = szText;
    lvc.iSubItem = 0;

    if (ListView_InsertColumn(vhwndListView, iColumn, &lvc) == -1)
        return FALSE;

    return TRUE;
}


BOOL FCreateListView(HWND hwndParent)
{
    RECT rc;

    ASSERT(vhwndListView == NULL);

    vdwListViewStyle = LVS_LIST; /* default style */
    vdwListViewSort = MAKELONG(0 /*NAME*/, 0 /*ascending*/);
    
    GetClientRect(hwndParent, &rc);
    vhwndListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
                        WS_VISIBLE | WS_CHILD | WS_BORDER | vdwListViewStyle |
                        LVS_AUTOARRANGE | LVS_SHAREIMAGELISTS,
                        0, 0, rc.right, rc.bottom, hwndParent, (HMENU)IDC_LISTVIEW,
                        vhinst, NULL);
    if (vhwndListView == NULL)
        return FALSE;

    ListView_SetImageList(vhwndListView, vhilSmall, LVSIL_SMALL);
    ListView_SetImageList(vhwndListView, vhilLarge, LVSIL_NORMAL);

    rc.right = rc.right * 2 / 3 - 10;
    if (rc.right < 100)
        rc.right = 100;
    if (!FAddColumnToListView(IDS_FILENAMECOLUMN, LVCFMT_LEFT, rc.right / 3, 0))
        return FALSE;
    if (!FAddColumnToListView(IDS_FILESIZECOLUMN, LVCFMT_RIGHT, rc.right / 4 - 5, 1))
        return FALSE;
    if (!FAddColumnToListView(IDS_FILEDATECOLUMN, LVCFMT_LEFT, rc.right / 4 + 5, 2))
        return FALSE;
    if (!FAddColumnToListView(IDS_FILETYPECOLUMN, LVCFMT_LEFT, rc.right / 12, 3))
        return FALSE;
    if (!FAddColumnToListView(IDS_FILECRETCOLUMN, LVCFMT_LEFT, rc.right / 12, 4))
        return FALSE;

    return TRUE;
}


int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    BOOL fDir1, fDir2, fAscending;
    int i;
    char szItem1[256], szItem2[256];
	char szType[256], szCret[256];
	long lSize1, lSize2;
	FILETIME ft1, ft2;

    fDir1 = ((lParam1 & 0xFF000000) != 0);
    fDir2 = ((lParam2 & 0xFF000000) != 0);

    i = fDir2 - fDir1; /* directories always come before files */
    if (i == 0)
    {
        if (LOWORD(vdwListViewSort) != 0 /*SIZE|DATE*/)
        {
            if (GetIthDirectoryInfo(lParam1 & 0x00FFFFFF, &lSize1, &ft1, szType, szCret) &&
                GetIthDirectoryInfo(lParam2 & 0x00FFFFFF, &lSize2, &ft2, szType, szCret))
            {
                if (LOWORD(vdwListViewSort) == 2 /*DATE*/)
                    i = CompareFileTime(&ft2, &ft1);
                else /*SIZE*/
                    i = lSize1 - lSize2;
            }
        }

        if (i == 0)
        {
        	if (GetIthDirectory(lParam1 & 0x00FFFFFF, szItem1, sizeof(szItem1), &fDir1) &&
                GetIthDirectory(lParam2 & 0x00FFFFFF, szItem2, sizeof(szItem2), &fDir2))
            {
                i = strcmpi(szItem1, szItem2);
            }
        }
    }

    fAscending = (HIWORD(vdwListViewSort) == 0);
    return (fAscending ? i : -i);
}


BOOL FAddDrive(int iDrive)
{
	char szDriveName[256];
	TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

	if (!GetIthPhysicalDrive(iDrive, szDriveName, sizeof(szDriveName)))
		return FALSE;

	FillMemory(&tvi, sizeof(TV_ITEM), 0);
	tvi.mask           = TVIF_CHILDREN | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvi.pszText        = szDriveName;
    tvi.cchTextMax     = sizeof(szDriveName);
	tvi.iImage         = HARDDISKIMAGEINDEX;
	tvi.iSelectedImage = HARDDISKIMAGEINDEX;
	tvi.lParam         = iDrive;

    tvins.item         = tvi;
    tvins.hInsertAfter = TVI_LAST;
    tvins.hParent      = TVI_ROOT;

    if (TreeView_InsertItem(vhwndTreeView, &tvins) == NULL)
		return FALSE;

	return TRUE;
}


void EmptyTreeView()
{
    vfEmptyingTreeView = TRUE;
	TreeView_DeleteAllItems(vhwndTreeView);
    vfEmptyingTreeView = FALSE;
}


BOOL FFillTreeView()
{
    HCURSOR hcursor;
	int cDrives, iDrive;

    ASSERT(vhwndTreeView != NULL);
    ASSERT(IsWindow(vhwndTreeView));

    /* hourglass on */
    SetCapture(GetParent(vhwndTreeView));
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    EmptyTreeView();

	cDrives = EnumeratePhysicalDrives();
	for (iDrive = 0; iDrive < cDrives; iDrive++)
	{
		if (!FAddDrive(iDrive))
        {
            EmptyTreeView();

	        /* hourglass off */
            SetCursor(hcursor);
            ReleaseCapture();
            return FALSE;
        }
	}

    /* hourglass off */
    SetCursor(hcursor);
    ReleaseCapture();

	return TRUE;
}


int GetFileIndexFromItem(int iItem)
{
    LV_ITEM lvi;

    FillMemory(&lvi, sizeof(LV_ITEM), 0);
    lvi.mask  = LVIF_PARAM;
    lvi.iItem = iItem;

    if (!ListView_GetItem(vhwndListView, &lvi))
        return 0; /* error! */

    return (int)(lvi.lParam & 0x00FFFFFF);
}


BOOL FFillListViewDetails()
{
	HCURSOR hcursor;
    int cItems, iItem;
	char szSize[256], szDate[256], szTime[256];
	char szType[256], szCret[256];
	long lSize;
	FILETIME ft;
	SYSTEMTIME st;

    ASSERT(vdwListViewStyle == LVS_REPORT);
    ASSERT(vhwndListView != NULL);
    ASSERT(IsWindow(vhwndListView));

    /* hourglass on */
    SetCapture(GetParent(vhwndListView));
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    cItems = ListView_GetItemCount(vhwndListView);
    for (iItem = 0; iItem < cItems; iItem++)
    {
        if (!GetIthDirectoryInfo(GetFileIndexFromItem(iItem),
                 &lSize, &ft, szType, szCret))
        {
            /* hourglass off */
            SetCursor(hcursor);
            ReleaseCapture();
            return FALSE;
        }

		if (lSize == -1)
			wsprintf(szSize, "");
		else
			wsprintf(szSize, "%d bytes", lSize);
		wsprintf(szDate, "foo", 0);

		FileTimeToSystemTime(&ft, &st);
#if 1
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szDate, sizeof(szDate));
		GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, szTime, sizeof(szTime));
#else
		GetDateFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, &st, NULL, szDate, sizeof(szDate));
		GetTimeFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, &st, NULL, szTime, sizeof(szTime));
#endif
		strcat(szDate, " ");
		strcat(szDate, szTime);

        ListView_SetItemText(vhwndListView, iItem, 1, szSize);
        ListView_SetItemText(vhwndListView, iItem, 2, szDate);

        ListView_SetItemText(vhwndListView, iItem, 3, szType);
        ListView_SetItemText(vhwndListView, iItem, 4, szCret);
    }
    
    /* hourglass off */
    SetCursor(hcursor);
    ReleaseCapture();

	return TRUE;
}


BOOL FFillListView()
{
	HCURSOR hcursor;
    int iItem, cItems;
	char szItem[256];
	BOOL fDirectory;
    LV_ITEM lvi;

    ASSERT(vhwndListView != NULL);
    ASSERT(IsWindow(vhwndListView));

    /* hourglass on */
    SetCapture(GetParent(vhwndListView));
    hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    ListView_DeleteAllItems(vhwndListView);

	cItems = EnumerateDirectory(viDriveID, vszCurrentPath);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		int cch;

		if (!GetIthDirectory(iItem, szItem, sizeof(szItem), &fDirectory))
        {
            ListView_DeleteAllItems(vhwndListView);

			/* hourglass off */
            SetCursor(hcursor);
            ReleaseCapture();
            return FALSE;
        }

		cch = strlen(szItem);
		FillMemory(&lvi, sizeof(LV_ITEM), 0);
		lvi.mask        = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.iItem       = iItem;
		lvi.iSubItem    = 0;
		lvi.pszText     = szItem;
        lvi.cchTextMax  = sizeof(szItem);
        lvi.iImage      = fDirectory ? FOLDERIMAGEINDEX : FILEIMAGEINDEX;
		if (strncmp(szItem, "..", 2) == 0)
			lvi.iImage = ARROWIMAGEINDEX;
		lvi.lParam      = (LPARAM)iItem;

        if (fDirectory)
        {
		    lvi.lParam |= ((LPARAM)szItem[cch - 1] << 24); /* last character */
			szItem[cch - 1] = '\0';
        }

		if (ListView_InsertItem(vhwndListView, &lvi) == -1)
        {
            ListView_DeleteAllItems(vhwndListView);

            /* hourglass off */
            SetCursor(hcursor);
            ReleaseCapture();
			return FALSE;
        }
	}

    /* hourglass off */
    SetCursor(hcursor);
    ReleaseCapture();

    ListView_SortItems(vhwndListView, ListViewCompareProc, 0);
    ListView_SetItemState(vhwndListView, 0, /*LVIS_SELECTED |*/ LVIS_FOCUSED, /*LVIS_SELECTED |*/ LVIS_FOCUSED);

    if (vdwListViewStyle == LVS_REPORT)
        return FFillListViewDetails();

	return TRUE;
}


BOOL CopyToClipboard(HWND hwnd, UINT uFormat, HANDLE h)
{
	BOOL fCopied;
	
	ASSERT(hwnd != NULL);
	ASSERT(IsWindow(hwnd));
	ASSERT(h != NULL);

	if (!OpenClipboard(hwnd))
		return FALSE;

	fCopied = (EmptyClipboard() && SetClipboardData(uFormat, h) != NULL);
	CloseClipboard();

	return fCopied;
}


void CopyAsText(HWND hwnd)
{
	int iItem, cItems;
	HGLOBAL h;
    
	h = NULL;
	cItems = ListView_GetItemCount(vhwndListView);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		if (ListView_GetItemState(vhwndListView, iItem, LVIS_SELECTED) != 0)
		{
            h = GetIthFileText(GetFileIndexFromItem(iItem));
			break;
		}
	}

	if (h == NULL)
		return;
	if (!CopyToClipboard(hwnd, CF_TEXT, h))
		GlobalFree(h);
}


BOOL FSwitchListViewStyle(DWORD dwNewStyle)
{
    DWORD dwStyle;

    vdwListViewStyle = dwNewStyle;
    dwStyle = GetWindowLong(vhwndListView, GWL_STYLE);
    SetWindowLong(vhwndListView, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwNewStyle);

    if (vdwListViewStyle == LVS_REPORT)
        return FFillListViewDetails();

    return TRUE;
}


void ListViewRefresh()
{
    if (!FFillListView())
    {
        /* something went wrong! */
        ASSERT(FALSE);
    }
}


void ListViewSelectAll()
{
	int iItem, cItems;

    cItems = ListView_GetItemCount(vhwndListView);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		ListView_SetItemState(vhwndListView, iItem, LVIS_SELECTED, LVIS_SELECTED);
    }

    SetFocus(vhwndListView);
}


BOOL FSelectDrive(HWND hwnd, HTREEITEM hti)
{
	TV_ITEM tvi;
	char szDriveName[256];

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
	
    FillMemory(&tvi, sizeof(TV_ITEM), 0);
    tvi.mask       = TVIF_TEXT | TVIF_PARAM;
    tvi.hItem      = hti;
    tvi.pszText    = szDriveName;
    tvi.cchTextMax = sizeof(szDriveName);

    if (!TreeView_GetItem(vhwndTreeView, &tvi))
		return FALSE;

    viDriveID = (int)tvi.lParam;
    vszCurrentPath[0] = '\0';
    FFillListView();
	strcpy(vszCurrentDrive, szDriveName);
    SetWindowTitle(hwnd);

	return TRUE;
}


void PreviousDirectory()
{
    int cch;
	char chDelim, *pchPrev;

	cch = strlen(vszCurrentPath);
	if (cch > 0)
	{
		chDelim = vszCurrentPath[cch - 1];
		vszCurrentPath[cch - 1] = '\0';
		pchPrev = strrchr(vszCurrentPath, chDelim);
		if (pchPrev != NULL)
			*(pchPrev + 1) = '\0';
		else
			vszCurrentPath[0] = '\0';
	}
}


void ListViewDoubleClick(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    char szItem[256];
    BOOL fDirectory;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));

    FillMemory(&lvi, sizeof(LV_ITEM), 0);
    lvi.mask       = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem      = iItem;
    lvi.pszText    = szItem;
    lvi.cchTextMax = sizeof(szItem);

    if (ListView_GetItem(vhwndListView, &lvi))
    {
        fDirectory = ((lvi.lParam & 0xFF000000) != 0);

        if (fDirectory)
        {
            int cch;

			if (lstrcmp(szItem, "..") == 0)
			{
				PreviousDirectory();
			}
			else
			{
				strcat(vszCurrentPath, szItem);

				cch = strlen(vszCurrentPath);
				vszCurrentPath[cch] = (char)(lvi.lParam >> 24);
				vszCurrentPath[cch + 1] = '\0';
			}

            FFillListView();
			SetWindowTitle(hwnd);
        }
        else
        {
			int iDrive, cDrives;

			cDrives = AddDiskImage((const char*)(lvi.lParam & 0x00FFFFFF));
			iDrive = TreeView_GetCount(vhwndTreeView);
			while (iDrive < cDrives)
			{
				if (!FAddDrive(iDrive))
				{
					/* @@@ error! */
					return;
				}

				iDrive++;
			}
        }
    }
}


void ShowFilePropertiesForItem(HWND hwnd, int iItem)
{
    LV_ITEM lvi;
    char szItem[256];

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));

    FillMemory(&lvi, sizeof(LV_ITEM), 0);
    lvi.mask       = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem      = iItem;
    lvi.pszText    = szItem;
    lvi.cchTextMax = sizeof(szItem);

    if (ListView_GetItem(vhwndListView, &lvi))
        ShowFileProperties(szItem);
}


//
//   FUNCTION: OpenTheFile(HWND hwnd, HWND hwndEdit)
//
//   PURPOSE: Invokes common dialog function to open a file and opens it.
//

BOOL OpenTheFile(HWND hWnd, char *psz, BOOL fWrite)
{
	OPENFILENAME OpenFileName;

	// Fill in the OPENFILENAME structure to support a template and hook.
	OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hWnd;
    OpenFileName.hInstance         = NULL;
    OpenFileName.lpstrFilter       = "Disk Images (*.cd;*.dsk;*.im*)\0*.cd;*.dsk;*.im*\0\0\0";
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = psz;
    OpenFileName.nMaxFile          = MAX_PATH;
    OpenFileName.lpstrFileTitle    = NULL;
    OpenFileName.nMaxFileTitle     = 0;
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = fWrite ? "Select a disk image to write to disk"
                                            : "Disk image file to create or overwrite";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.lCustData         = 0;
	OpenFileName.lpfnHook 		   = NULL;
	OpenFileName.lpTemplateName    = NULL;
    OpenFileName.Flags             = fWrite ? OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
                                            : OFN_EXPLORER | OFN_PATHMUSTEXIST;

	// Call the common dialog function.
    if (GetOpenFileName(&OpenFileName))
        {
        strcpy(psz, OpenFileName.lpstrFile);
		return TRUE;
        }

    return FALSE;
}

void OnDiskImageCopy(HWND hwnd, BOOL fWrite, BOOL fRawDisk)
{
    // fWrite - TRUE = write image to disk, FALSE = read from disk to image

    PDI pdi = NULL;
    char szImageFile[MAX_PATH] = { 0 };

    if (!OpenTheFile(hwnd, szImageFile, fWrite))
        return;

    pdi = PdiOpenDisk(DISK_IMAGE, (long)szImageFile, (fWrite ? DI_READONLY : DI_QUERY | DI_CREATE));

    if (pdi)
        {
        HCURSOR hcursor;

        /* hourglass on */
      //  EnableWindow(hwnd,FALSE);
        SetCapture(GetParent(vhwndTreeView));
        hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

        CopyDiskImage(hwnd, fWrite, 0, pdi, fRawDisk);
        CloseDiskPdi(pdi);

        /* hourglass off */
        SetCursor(hcursor);
        ReleaseCapture();
      //  EnableWindow(hwnd,TRUE);

        // force the window to refresh with the new disk contents
        PostMessage(hwnd, WM_COMMAND, ID_VIEW_REFRESH, 0);
        }
}


void OnFileProperties(HWND hwnd)
{
	int iItem, cItems;

    ASSERT(ListView_GetSelectedCount(vhwndListView) == 1);

    cItems = ListView_GetItemCount(vhwndListView);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		if (ListView_GetItemState(vhwndListView, iItem, LVIS_SELECTED) != 0)
		{
            ShowFilePropertiesForItem(hwnd, iItem);
            break;
        }
    }
}


void OnFileAbout(HWND hwnd)
{
    char sz[1024], szT[256];

    wsprintf(sz, "Gemulator Explorer 2.40 BETA (%s release)\n\n", __DATE__);

    wsprintf(szT, "Copyright (C) 2004 Emulators Inc. All rights reserved.\n\n");
    strcat(sz, szT);

    wsprintf(szT, "Gemulator Explorer is a free utility to read non-DOS ");
    strcat(sz, szT);

    wsprintf(szT, "(Atari ST and Apple Macintosh) disks on your Windows PC.\n");
    strcat(sz, szT);

    wsprintf(szT, "Use it to read floppy disks, CD-ROMs, LS-120 Superdisks, ");
    strcat(sz, szT);

    wsprintf(szT, "Iomega ZIP and Jaz disks, and non-DOS SCSI hard disks.\n");
    strcat(sz, szT);

    wsprintf(szT, "Create disk images of those disks, or drag individual ");
    strcat(sz, szT);

    wsprintf(szT, "files to Windows Explorer to copy your documents over.\n");
    strcat(sz, szT);

    wsprintf(szT, "Even use it to format Macintosh disks on your PC and to ");
    strcat(sz, szT);

    wsprintf(szT, "create Mac OS boot disks from boot disk image files.\n");
    strcat(sz, szT);

    wsprintf(szT, "Visit our web site at http://www.emulators.com for full documentation.\n");
    strcat(sz, szT);

    MessageBox(vhwndMain, sz, "About Gemulator Explorer", MB_OK);
}


BOOL IsItemPreviousDirectory(int iItem)
{
    LV_ITEM lvi;
    char szItem[256];
    BOOL fDirectory;

    FillMemory(&lvi, sizeof(LV_ITEM), 0);
    lvi.mask       = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem      = iItem;
    lvi.pszText    = szItem;
    lvi.cchTextMax = sizeof(szItem);

    if (!ListView_GetItem(vhwndListView, &lvi))
        return FALSE;

    fDirectory = ((lvi.lParam & 0xFF000000) != 0);
    if (fDirectory && lstrcmp(szItem, "..") == 0)
        return TRUE;

    return FALSE;
}


BOOL IsItemDirectory(int iItem)
{
    LV_ITEM lvi;
    char szItem[256];
    BOOL fDirectory;

    FillMemory(&lvi, sizeof(LV_ITEM), 0);
    lvi.mask       = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem      = iItem;
    lvi.pszText    = szItem;
    lvi.cchTextMax = sizeof(szItem);

    if (!ListView_GetItem(vhwndListView, &lvi))
        return FALSE;

    fDirectory = ((lvi.lParam & 0xFF000000) != 0);
	return fDirectory;
}


int GetDropSourceItemCount()
{
	int iItem, cItems, cSourceItems;

	cSourceItems = 0;
    cItems = ListView_GetItemCount(vhwndListView);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		if (ListView_GetItemState(vhwndListView, iItem, LVIS_SELECTED) != 0)
		{
            if (IsItemPreviousDirectory(iItem)) /* don't include ".." */
                continue;
            
            cSourceItems++;
		}
	}

    return cSourceItems;
}


int GetSelectedFileCount()
{
	int iItem, cItems, cFiles;

	cFiles = 0;
    cItems = ListView_GetItemCount(vhwndListView);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		if (ListView_GetItemState(vhwndListView, iItem, LVIS_SELECTED) != 0)
		{
            if (!IsItemDirectory(iItem))
	            cFiles++;
		}
	}

    return cFiles;
}


BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpcs, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    vcxSplitter = GetSystemMetrics(SM_CXEDGE);
    if (vcxSplitter == 0)
        vcxSplitter = 2;

    if (!FLoadImages() || !FCreateTreeView(hwnd) || !FCreateListView(hwnd))
    {
        *plr = -1;
        return TRUE;
    }

    PostMessage(hwnd, WM_GIE_INITIALIZE, 0, 0);

    *plr = 0;
    return TRUE;
}


BOOL OnDestroy(HWND hwnd, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

#ifndef NODRAGDROP
    if (vhfdtTreeView != (HFILEDROPTGT)NULL)
    {
        UnregisterFileDropTarget(vhfdtTreeView);
        vhfdtTreeView = (HFILEDROPTGT)NULL;
    }
#endif /* !NODRAGDROP */

    PostQuitMessage(0);

    *plr = 0;
    return TRUE;
}


BOOL OnGieInitialize(HWND hwnd, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    FFillTreeView();

#ifndef NODRAGDROP
	vhfdtTreeView = RegisterFileDropTarget(hwnd /*hwndProcessDrop*/, vhwndTreeView /*hwndTarget*/);
#endif /* !NODRAGDROP */

    /* @@@ */
    SetFocus(vhwndTreeView);

    *plr = 0;
    return TRUE;
}


BOOL OnGieCanDropFiles(HWND hwnd, HFILEDROPTGT hfdt, HSTRLIST hsl, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(hfdt != (HFILEDROPTGT)NULL);
    ASSERT(hfdt == vhfdtTreeView);
    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));
    ASSERT(StrListGetCount(hsl) > 0);
    ASSERT(plr != NULL);

    *plr = TRUE; /* permit all file drops */
    return TRUE;
}


BOOL OnGieFilesDropped(HWND hwnd, HFILEDROPTGT hfdt, HSTRLIST hsl, LRESULT *plr)
{
    int iItem, cItems;
	int iDrive, cDrives;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(hfdt != (HFILEDROPTGT)NULL);
    ASSERT(hfdt == vhfdtTreeView);
    ASSERT(hsl != (HSTRLIST)NULL);
    ASSERT(StrListIsValid(hsl));
    ASSERT(StrListGetCount(hsl) > 0);
    ASSERT(plr != NULL);

    cItems = StrListGetCount(hsl);
    for (iItem = 0; iItem < cItems; iItem++)
    {
		cDrives = AddDiskImage(StrListGet(hsl, iItem));
    }

	iDrive = TreeView_GetCount(vhwndTreeView);
	while (iDrive < cDrives)
	{
		if (!FAddDrive(iDrive))
		{
			/* @@@ error! */
			return FALSE;
		}

		iDrive++;
	}

	PostMessage(vhwndTreeView, WM_KEYDOWN, VK_END, 0);

    *plr = 0;
    return TRUE;
}


BOOL OnGieGetFileGroupDescriptor(HWND hwnd, LRESULT *plr)
{
	int iItem, cItems, cSelItems;
    HGLOBAL h;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);
	ASSERT(ListView_GetSelectedCount(vhwndListView) > 0);

#if 0
    // uncomment this to allow only a single file copy

	if (ListView_GetSelectedCount(vhwndListView) > 1)
        return TRUE;
#endif

	h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(FILEGROUPDESCRIPTOR));
	if (h == NULL)
	{
		*plr = 0; /* hglobal = NULL */
		return TRUE;
	}

	cSelItems = 0;
    cItems = ListView_GetItemCount(vhwndListView);
	for (iItem = 0; iItem < cItems; iItem++)
	{
		if (ListView_GetItemState(vhwndListView, iItem, LVIS_SELECTED) != 0)
		{
            if (IsItemPreviousDirectory(iItem)) /* don't include ".." */
                continue;
            
            if (!AddDragDescriptor(GetFileIndexFromItem(iItem), (HGLOBAL *)&h))
			{
                cSelItems = 0;
				break;
			}

            cSelItems++;
		}
	}

	if (cSelItems == 0)
	{
		GlobalFree(h);
		h = NULL;
	}

	*plr = (LRESULT)h;
	return TRUE;
}


BOOL OnGieGetFileContents(HWND hwnd, int index, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

	*plr = (LRESULT)DropFileContents(index);
	return TRUE;
}


BOOL OnLButtonDown(HWND hwnd, UINT fwKeys, int xPos, int yPos, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (vfTrackingSplitter)
        return FALSE;
    vhdcSplitter = GetDC(hwnd);
    if (vhdcSplitter == NULL)
        return FALSE;

    vfTrackingSplitter = TRUE;
    SetCapture(hwnd);
    GetClientRect(hwnd, &vrcSplitter);

    vrcSplitter.left = min(max(50, xPos - vcxSplitter / 2), vrcSplitter.right - 50) + 1;
    PatBlt(vhdcSplitter, vrcSplitter.left, 0, vcxSplitter, vrcSplitter.bottom, DSTINVERT);

    *plr = 0;
    return TRUE;
}


BOOL OnMouseMove(HWND hwnd, UINT fwKeys, int xPos, int yPos, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (!vfTrackingSplitter)
        return FALSE;

    PatBlt(vhdcSplitter, vrcSplitter.left, 0, vcxSplitter, vrcSplitter.bottom, DSTINVERT);
    vrcSplitter.left = min(max(50, xPos - vcxSplitter / 2), vrcSplitter.right - 50) + 1;
    PatBlt(vhdcSplitter, vrcSplitter.left, 0, vcxSplitter, vrcSplitter.bottom, DSTINVERT);

    *plr = 0;
    return TRUE;
}


BOOL OnLButtonUp(HWND hwnd, UINT fwKeys, int xPos, int yPos, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (!vfTrackingSplitter)
        return FALSE;

    PatBlt(vhdcSplitter, vrcSplitter.left, 0, vcxSplitter, vrcSplitter.bottom, DSTINVERT);
    vrcSplitter.left = min(max(50, xPos - vcxSplitter / 2), vrcSplitter.right - 50) + 1;

    ReleaseCapture();
    ReleaseDC(hwnd, vhdcSplitter);
    vfTrackingSplitter = FALSE;
    vhdcSplitter = NULL;

    vrcSplitter.left -= vcxSplitter / 2;
    SetWindowPos(vhwndTreeView, NULL, 0, 0, vrcSplitter.left, vrcSplitter.bottom, SWP_NOZORDER);
    vrcSplitter.left += vcxSplitter;
    SetWindowPos(vhwndListView, NULL, vrcSplitter.left, 0, vrcSplitter.right - vrcSplitter.left, vrcSplitter.bottom, SWP_NOZORDER);

    *plr = 0;
    return TRUE;
}


BOOL OnKeyDown(HWND hwnd, int nVirtKey, UINT lKeyData, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (nVirtKey != VK_ESCAPE)
        return FALSE;
    if (!vfTrackingSplitter)
        return FALSE;

    PatBlt(vhdcSplitter, vrcSplitter.left, 0, vcxSplitter, vrcSplitter.bottom, DSTINVERT);

    ReleaseCapture();
    ReleaseDC(hwnd, vhdcSplitter);
    vfTrackingSplitter = FALSE;
    vhdcSplitter = NULL;

    *plr = 0;
    return TRUE;
}


BOOL OnSize(HWND hwnd, UINT fwSizeType, int cx, int cy, LRESULT *plr)
{
    RECT rc;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (vhwndTreeView == NULL || vhwndListView == NULL)
        return FALSE;

    if (fwSizeType != SIZE_MINIMIZED)
    {
        GetWindowRect(vhwndTreeView, &rc);
        ScreenToClient(hwnd, (LPPOINT)&rc.right);
        rc.right = max(50, min(rc.right, cx - 50));
        SetWindowPos(vhwndTreeView, NULL, 0, 0, rc.right, cy, SWP_NOZORDER);
        rc.right += vcxSplitter;
        SetWindowPos(vhwndListView, NULL, rc.right, 0, cx - rc.right, cy, SWP_NOZORDER);
    }

    *plr = 0;
    return TRUE;
}


BOOL OnCommand(HWND hwnd, UINT uNotifyCode, UINT uID, HWND hwndCtl, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    switch (uID)
    {
        case ID_FILE_CREATEIMAGEFROMPART:
            OnDiskImageCopy(hwnd, FALSE, FALSE);
            *plr = 0;
            return TRUE;
        case ID_FILE_CREATEIMAGEFROMDISK:
            OnDiskImageCopy(hwnd, FALSE, TRUE);
            *plr = 0;
            return TRUE;
        case ID_FILE_CREATEDISKFROMIMAGE:
#ifndef NODRAGDROP
            OnDiskImageCopy(hwnd, TRUE, TRUE);
#endif
            *plr = 0;
            return TRUE;
        case ID_FILE_PROPERTIES:
            OnFileProperties(hwnd);
            *plr = 0;
            return TRUE;
        case ID_FILE_ABOUT:
            OnFileAbout(hwnd);
            *plr = 0;
            return TRUE;
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            *plr = 0;
            return TRUE;
		case ID_EDIT_COPYASTEXT:
			CopyAsText(hwnd);
			*plr = 0;
			return TRUE;
        case ID_VIEW_LARGEICONS:
            FSwitchListViewStyle(LVS_ICON);
            *plr = 0;
            return TRUE;
        case ID_VIEW_SMALLICONS:
            FSwitchListViewStyle(LVS_SMALLICON);
            *plr = 0;
            return TRUE;
        case ID_VIEW_LIST:
            FSwitchListViewStyle(LVS_LIST);
            *plr = 0;
            return TRUE;
        case ID_VIEW_DETAILS:
            FSwitchListViewStyle(LVS_REPORT);
            *plr = 0;
            return TRUE;
        case ID_VIEW_REFRESH:
            ListViewRefresh();
            *plr = 0;
            return TRUE;
        case ID_VIEW_SELECTALL:
            ListViewSelectAll();
            *plr = 0;
            return TRUE;
    }

    return FALSE;
}


BOOL OnNotifyTreeView(HWND hwnd, LPNMHDR pnmhdr, LRESULT *plr)
{
    TV_HITTESTINFO tvhti;
	NM_TREEVIEW *pnmtv;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(pnmhdr != NULL);
    ASSERT(plr != NULL);

    if (pnmhdr->code == NM_CLICK)
    {
        FillMemory(&tvhti, sizeof(TV_HITTESTINFO), 0);
        GetCursorPos(&tvhti.pt);
        ScreenToClient(vhwndTreeView, &tvhti.pt);

        if (TreeView_HitTest(vhwndTreeView, &tvhti) != NULL && (tvhti.flags & TVHT_ONITEM) != 0)
            FSelectDrive(hwnd, tvhti.hItem);

        *plr = 0;
        return TRUE;
    }

	if (pnmhdr->code == TVN_SELCHANGED)
	{
		if (!vfEmptyingTreeView)
        {
            pnmtv = (NM_TREEVIEW *)pnmhdr;
            if (pnmtv->action != TVC_BYMOUSE)
		        FSelectDrive(hwnd, pnmtv->itemNew.hItem);
        }

        *plr = 0;
        return TRUE;
	}

    if (pnmhdr->code == TVN_KEYDOWN)
    {
        TV_KEYDOWN *ptvkd;

        ptvkd = (TV_KEYDOWN *)pnmhdr;
        if (ptvkd->wVKey == VK_TAB)
        {
            SetFocus(vhwndListView);
    		*plr = 1;
	    	return TRUE;
        }

        if (ptvkd->wVKey == VK_F5)
        {
            ListViewRefresh();
            *plr = 1;
            return TRUE;
        }
    }

    return FALSE;
}


BOOL OnNotifyListView(HWND hwnd, LPNMHDR pnmhdr, LRESULT *plr)
{
    LV_HITTESTINFO lvhti;

    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(pnmhdr != NULL);
    ASSERT(plr != NULL);

    if (pnmhdr->code == NM_DBLCLK || pnmhdr->code == NM_RCLICK)
    {
        FillMemory(&lvhti, sizeof(LV_HITTESTINFO), 0);
        GetCursorPos(&lvhti.pt);
        ScreenToClient(vhwndListView, &lvhti.pt);

        if (ListView_HitTest(vhwndListView, &lvhti) != -1 && (lvhti.flags & LVHT_ONITEM) != 0)
        {
            if (pnmhdr->code == NM_DBLCLK)
                ListViewDoubleClick(hwnd, lvhti.iItem);
            else
                ShowFilePropertiesForItem(hwnd, lvhti.iItem);
        }

        *plr = 0;
        return TRUE;
    }

    if (pnmhdr->code == LVN_COLUMNCLICK)
    {
        NM_LISTVIEW *pnmlv;
        
        pnmlv = (NM_LISTVIEW *)pnmhdr;
        if (LOWORD(vdwListViewSort) == pnmlv->iSubItem)
            vdwListViewSort = MAKELONG(pnmlv->iSubItem, !HIWORD(vdwListViewSort));
        else
            vdwListViewSort = MAKELONG(pnmlv->iSubItem, 0 /*ascending*/);

        ListView_SortItems(vhwndListView, ListViewCompareProc, 0);
        
        *plr = 0;
        return TRUE;
    }

    if (pnmhdr->code == NM_RETURN)
	{
        BOOL fAltEnter;

		fAltEnter = ((GetKeyState(VK_MENU) & 0x8000) != 0);
		if (ListView_GetSelectedCount(vhwndListView) == 1)
	    {
		    int iItem, cItems;

            cItems = ListView_GetItemCount(vhwndListView);
	        for (iItem = 0; iItem < cItems; iItem++)
	        {
		        if (ListView_GetItemState(vhwndListView, iItem, LVIS_SELECTED) != 0)
		        {
                    if (fAltEnter)
						ShowFilePropertiesForItem(hwnd, iItem);
					else
						ListViewDoubleClick(hwnd, iItem);
                    break;
                }
            }
	    }

		*plr = 0;
		return TRUE;
	}

    if (pnmhdr->code == LVN_KEYDOWN)
    {
        LV_KEYDOWN *plvkd;

        plvkd = (LV_KEYDOWN *)pnmhdr;
        if (plvkd->wVKey == VK_TAB)
        {
            SetFocus(vhwndTreeView);
		    *plr = 1;
		    return TRUE;
        }

        if (plvkd->wVKey == VK_BACK)
        {
            if (strlen(vszCurrentPath) > 0)
            {
                PreviousDirectory();
                FFillListView();
			    SetWindowTitle(hwnd);
            }

			*plr = 1;
			return TRUE;
        }        

        if (plvkd->wVKey == 'A' /*VK_A*/)
        {
            BOOL fCtrl;

		    fCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);
            if (fCtrl)
            {
                ListViewSelectAll();
                *plr = 1;
                return TRUE;
            }
        }

        if (plvkd->wVKey == VK_F5)
        {
            ListViewRefresh();
            *plr = 1;
            return TRUE;
        }
    }

#ifndef NODRAGDROP
	if (pnmhdr->code == LVN_BEGINDRAG)
	{
		if (GetDropSourceItemCount() > 0)
            DoDrag(hwnd);
	}
#endif /* !NODRAGDROP */

    return FALSE;
}


BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmhdr, LRESULT *plr)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (idCtrl == IDC_TREEVIEW)
        return OnNotifyTreeView(hwnd, pnmhdr, plr);
    if (idCtrl == IDC_LISTVIEW)
        return OnNotifyListView(hwnd, pnmhdr, plr);

    return FALSE;
}


BOOL OnInitMenuPopup(HWND hwnd, HMENU hmenu, UINT uPos, BOOL fSystemMenu, LRESULT *plr)
{
    BOOL fOneFileSelected;
    BOOL fBlockDev;
    UINT idCheck;
    
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));
    ASSERT(plr != NULL);

    if (fSystemMenu)
        return FALSE;

    fBlockDev = IsBlockDevice(0);   // REVIEW: pass tree view index
    EnableMenuItem(hmenu, ID_FILE_CREATEIMAGEFROMPART, MF_BYCOMMAND | (fBlockDev ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(hmenu, ID_FILE_CREATEIMAGEFROMDISK, MF_BYCOMMAND | (fBlockDev ? MF_ENABLED : MF_GRAYED));
#ifdef NODRAGDROP
    EnableMenuItem(hmenu, ID_FILE_CREATEDISKFROMIMAGE, MF_BYCOMMAND | MF_GRAYED);
#else
    EnableMenuItem(hmenu, ID_FILE_CREATEDISKFROMIMAGE, MF_BYCOMMAND | (fBlockDev ? MF_ENABLED : MF_GRAYED));
#endif

    /* enable Properties if one item is selected (and it's not ..) */
	fOneFileSelected = (ListView_GetSelectedCount(vhwndListView) == 1 && GetDropSourceItemCount() == 1);
    EnableMenuItem(hmenu, ID_FILE_PROPERTIES, MF_BYCOMMAND | (fOneFileSelected ? MF_ENABLED : MF_GRAYED));

	/* enable Copy as Text if one -file- is selected */
#ifdef NODRAGDROP
	EnableMenuItem(hmenu, ID_EDIT_COPYASTEXT, MF_BYCOMMAND | MF_GRAYED);
#else /* !NODRAGDROP */
    fOneFileSelected = (ListView_GetSelectedCount(vhwndListView) == 1 && GetSelectedFileCount() == 1);
	EnableMenuItem(hmenu, ID_EDIT_COPYASTEXT, MF_BYCOMMAND | (fOneFileSelected ? MF_ENABLED : MF_GRAYED));
#endif /* !NODRAGDROP */

    if (vdwListViewStyle == LVS_ICON)
        idCheck = ID_VIEW_LARGEICONS;
    else if (vdwListViewStyle == LVS_SMALLICON)
        idCheck = ID_VIEW_SMALLICONS;
    else if (vdwListViewStyle == LVS_LIST)
        idCheck = ID_VIEW_LIST;
    else
        idCheck = ID_VIEW_DETAILS;
    CheckMenuRadioItem(hmenu, ID_VIEW_LARGEICONS, ID_VIEW_DETAILS, idCheck, MF_BYCOMMAND);

    *plr = 0;
    return TRUE;
}


LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr;

    switch (uMsg)
    {
        case WM_CREATE:
            if (OnCreate(hwnd, (LPCREATESTRUCT)lParam, &lr))
                return lr;
            break;
        case WM_DESTROY:
            if (OnDestroy(hwnd, &lr))
                return lr;
            break;
        case WM_GIE_INITIALIZE:
            if (OnGieInitialize(hwnd, &lr))
                return lr;
            break;
        case WM_GIE_CANDROPFILES:
            if (OnGieCanDropFiles(hwnd, (HFILEDROPTGT)wParam, (HSTRLIST)lParam, &lr))
                return lr;
            break;
        case WM_GIE_FILESDROPPED:
            if (OnGieFilesDropped(hwnd, (HFILEDROPTGT)wParam, (HSTRLIST)lParam, &lr))
                return lr;
            break;
		case WM_GIE_GETFILEGROUPDESCRIPTOR:
			if (OnGieGetFileGroupDescriptor(hwnd, &lr))
				return lr;
			break;
		case WM_GIE_GETFILECONTENTS:
			if (OnGieGetFileContents(hwnd, (int)lParam, &lr))
				return lr;
			break;
        case WM_LBUTTONDOWN:
            if (OnLButtonDown(hwnd, wParam, LOWORD(lParam), HIWORD(lParam), &lr))
                return lr;
            break;
        case WM_MOUSEMOVE:
            if (OnMouseMove(hwnd, wParam, LOWORD(lParam), HIWORD(lParam), &lr))
                return lr;
            break;
        case WM_LBUTTONUP:
            if (OnLButtonUp(hwnd, wParam, LOWORD(lParam), HIWORD(lParam), &lr))
                return lr;
            break;
        case WM_KEYDOWN:
            if (OnKeyDown(hwnd, (int)wParam, lParam, &lr))
                return lr;
            break;
        case WM_SIZE:
            if (OnSize(hwnd, wParam, LOWORD(lParam), HIWORD(lParam), &lr))
                return lr;
            break;
        case WM_COMMAND:
            if (OnCommand(hwnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam, &lr))
                return lr;
            break;
        case WM_NOTIFY:
            if (OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam, &lr))
                return lr;
            break;
        case WM_INITMENUPOPUP:
            if (OnInitMenuPopup(hwnd, (HMENU)wParam, LOWORD(lParam), HIWORD(lParam), &lr))
                return lr;
            break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


BOOL FInitApplication(int nCmdShow)
{
    WNDCLASSEX wc;
    char szDefaultTitle[256];

    ASSERT(vhwndMain == NULL);

    FillMemory(&wc, sizeof(WNDCLASSEX), 0);
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MainWindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = vhinst;
    wc.hIcon         = LoadIcon(vhinst, MAKEINTRESOURCE(IDI_GIE));
    wc.hCursor       = LoadCursor(vhinst, MAKEINTRESOURCE(IDC_SPLITTER));
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDM_GIE);
    wc.lpszClassName = GIEWINDOWCLASS;
	wc.hIconSm       = LoadImage(vhinst, MAKEINTRESOURCE(IDI_GIE), IMAGE_ICON, 16, 16, 0);

    if (!RegisterClassEx(&wc))
        return FALSE;

	if (LoadString(vhinst, IDS_DEFAULTTITLE, szDefaultTitle, sizeof(szDefaultTitle)) == 0)
		szDefaultTitle[0] = '\0';

    vhwndMain = CreateWindow(GIEWINDOWCLASS, szDefaultTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, vhinst, NULL);
    if (vhwndMain == NULL)
        return FALSE;

    ShowWindow(vhwndMain, nCmdShow);
    UpdateWindow(vhwndMain);
    return TRUE;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    vhinst = hInstance;

    if (CreateMutex(NULL, FALSE, "Gemulator_Exchange") &&
        GetLastError() == ERROR_ALREADY_EXISTS)
        return 1; /* failure because another instance already running */

    if (!FInitApplication(nCmdShow) || FAILED(OleInitialize(NULL)))
        return 1; /* failure */

    OnFileAbout(NULL);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    OleUninitialize();
    return msg.wParam;
}
