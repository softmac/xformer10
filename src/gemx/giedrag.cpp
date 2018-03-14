/* giedrag.cpp */


#include "precomp.h"
#include "giedrag.h"
#include "strlist.h"


#define CF_IDLIST              RegisterClipboardFormat(CFSTR_SHELLIDLIST)
#define CF_FILEGROUPDESCRIPTOR RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR)
#define CF_FILECONTENTS        RegisterClipboardFormat(CFSTR_FILECONTENTS)


class GieFileDropTarget : public IDropTarget
{
    public:
        GieFileDropTarget(HWND hwndProcessDrop, HWND hwndTarget);
        ~GieFileDropTarget();

    public:
        HWND GetTargetHwnd() const;

    public: /* IUnknown */
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
    public: /* IDropTarget */
        STDMETHODIMP DragEnter(LPDATAOBJECT pIDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
        STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
        STDMETHODIMP DragLeave();
        STDMETHODIMP Drop(LPDATAOBJECT pIDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

    private:
        HSTRLIST     ExtractFilesFromDataObject(LPDATAOBJECT pIDataObject);
        HSTRLIST     ExtractFilesFromHGlobal(HGLOBAL h);
        LPITEMIDLIST CopyShellInfo(LPITEMIDLIST piilDest, LPITEMIDLIST piilSrc);
        BOOL         FCanDropFiles(HSTRLIST hsl);
        BOOL         FDropFiles(HSTRLIST hsl);

    private:
        UINT m_cRef;
        HWND m_hwndProcessDrop;
        HWND m_hwndTarget;
        BOOL m_fCanDropFiles;
}; /* GieFileDropTarget */


class GieFileDropSource : public IDropSource
{
    public:
        GieFileDropSource();
        ~GieFileDropSource();

    public: /* IUnknown */
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

    public: /* IDropSource */
        STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
        STDMETHODIMP GiveFeedback(DWORD dwEffect);

    private:
        UINT m_cRef;
}; /* GieFileDropSource */


class GieFileDataObject : public IDataObject
{
    public:
        GieFileDataObject(HWND hwnd);
        ~GieFileDataObject();

    public: /* IUnknown */
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

    public: /* IDataObject */
        STDMETHODIMP GetData(LPFORMATETC pfe, LPSTGMEDIUM psm);
        STDMETHODIMP GetDataHere(LPFORMATETC pfe, LPSTGMEDIUM psm);
        STDMETHODIMP QueryGetData(LPFORMATETC pfe);
        STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pfeIn, LPFORMATETC pfeOut);
        STDMETHODIMP SetData(LPFORMATETC pfe, LPSTGMEDIUM psm, BOOL fRelease);
        STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnum);
        STDMETHODIMP DAdvise(LPFORMATETC pfe, DWORD grfAdvise, LPADVISESINK pAdvSink, LPDWORD pdwConnection);
        STDMETHODIMP DUnadvise(DWORD dwConnection);
        STDMETHODIMP EnumDAdvise(LPENUMSTATDATA* ppEnum);

    private:
        UINT m_cRef;
		HWND m_hwnd;
}; /* GieFileDataObject */


class GieEnumFormatEtc : public IEnumFORMATETC
{
    public:
        GieEnumFormatEtc();
        ~GieEnumFormatEtc();

    public: /* IUnknown */
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

    public: /* IEnumFORMATETC */
        STDMETHODIMP Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
        STDMETHODIMP Skip(ULONG celt);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(IEnumFORMATETC** ppenum);

    private:
        UINT m_cRef;
        UINT m_iFormat;
        UINT m_cFormat;
        FORMATETC m_rgfe[2];
}; /* GieEnumFormatEtc */




GieFileDropTarget::GieFileDropTarget(HWND hwndProcessDrop, HWND hwndTarget)
{
    ASSERT(hwndProcessDrop != NULL);
    ASSERT(IsWindow(hwndProcessDrop));
    ASSERT(hwndTarget != NULL);
    ASSERT(IsWindow(hwndTarget));

    m_cRef = 0;
    m_hwndProcessDrop = hwndProcessDrop;
    m_hwndTarget = hwndTarget;
    m_fCanDropFiles = FALSE;
}


GieFileDropTarget::~GieFileDropTarget()
{
}


HWND GieFileDropTarget::GetTargetHwnd() const
{
    ASSERT(m_hwndTarget != NULL);
    ASSERT(IsWindow(m_hwndTarget));

    return m_hwndTarget;
}


STDMETHODIMP GieFileDropTarget::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IDropTarget)
    {
        *ppvObj = (LPVOID)(IDropTarget*)this;
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) GieFileDropTarget::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}


STDMETHODIMP_(ULONG) GieFileDropTarget::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP GieFileDropTarget::DragEnter(LPDATAOBJECT pIDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    HSTRLIST hsl;

    hsl = ExtractFilesFromDataObject(pIDataObject);
    if (hsl != NULL)
    {
        m_fCanDropFiles = FCanDropFiles(hsl);
        StrListDestroy(hsl);
    }
    else
    {
        m_fCanDropFiles = FALSE;
    }

    *pdwEffect = m_fCanDropFiles ? DROPEFFECT_COPY : DROPEFFECT_NONE;
    return NOERROR;
}


STDMETHODIMP GieFileDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    *pdwEffect = m_fCanDropFiles ? DROPEFFECT_COPY : DROPEFFECT_NONE;
    return NOERROR;
}


STDMETHODIMP GieFileDropTarget::DragLeave()
{
    m_fCanDropFiles = FALSE;
    return NOERROR;
}


STDMETHODIMP GieFileDropTarget::Drop(LPDATAOBJECT pIDataObject, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    BOOL fDropped;
    HSTRLIST hsl;

    m_fCanDropFiles = FALSE;

    fDropped = FALSE;
    hsl = ExtractFilesFromDataObject(pIDataObject);
    if (hsl != NULL)
    {
        if (FCanDropFiles(hsl))
            fDropped = FDropFiles(hsl);
        StrListDestroy(hsl);
    }

    *pdwEffect = fDropped ? DROPEFFECT_COPY : DROPEFFECT_NONE;
    return NOERROR;
}


HSTRLIST GieFileDropTarget::ExtractFilesFromDataObject(LPDATAOBJECT pIDataObject)
{
    FORMATETC fe;
    STGMEDIUM sm;
    HSTRLIST hsl;

    ASSERT(pIDataObject !=  NULL);

    FillMemory(&fe, sizeof(FORMATETC), 0);
    FillMemory(&sm, sizeof(STGMEDIUM), 0);

    fe.cfFormat = CF_IDLIST;
    fe.ptd      = NULL;
    fe.dwAspect = DVASPECT_CONTENT;
    fe.lindex   = -1;
    fe.tymed    = TYMED_HGLOBAL;

    if (FAILED(pIDataObject->GetData(&fe, &sm)))
        return NULL;

    ASSERT(TYMED_HGLOBAL == sm.tymed);

    hsl = ExtractFilesFromHGlobal(sm.hGlobal);
    ReleaseStgMedium(&sm);
    return hsl;
}


HSTRLIST GieFileDropTarget::ExtractFilesFromHGlobal(HGLOBAL h)
{
    HSTRLIST hsl;
    LPITEMIDLIST piilPath, piilFilePart;
    LPIDA pi;
    DWORD iFile;
    char szFullPath[MAX_PATH];

    ASSERT(h != NULL);

    hsl = StrListCreate();
    if (hsl == (HSTRLIST)NULL)
        return (HSTRLIST)NULL;
    
    piilPath = (LPITEMIDLIST)malloc(GlobalSize(h));
    if (piilPath == (LPITEMIDLIST)NULL)
    {
        StrListDestroy(hsl);
        return (HSTRLIST)NULL;
    }

    pi = (LPIDA)GlobalLock(h);
    piilFilePart = CopyShellInfo(piilPath, (LPITEMIDLIST)((char*)pi + pi->aoffset[0]));

    for (iFile = 1; iFile <= pi->cidl; iFile++)
    {
        CopyShellInfo(piilFilePart, (LPITEMIDLIST)((char*)pi + pi->aoffset[iFile]));
        if (SHGetPathFromIDList(piilPath, szFullPath))
            StrListAdd(hsl, szFullPath);
    }

    free(piilPath);
    GlobalUnlock(h);
    return hsl;
}


LPITEMIDLIST GieFileDropTarget::CopyShellInfo(LPITEMIDLIST piilDest, LPITEMIDLIST piilSrc)
{
    DWORD cb;

    while (*(USHORT *)piilSrc != 0)
    {
        cb = *(USHORT *)piilSrc;
        CopyMemory(piilDest, piilSrc, cb);
        piilDest = (LPITEMIDLIST)((char*)piilDest + cb);
        piilSrc = (LPITEMIDLIST)((char*)piilSrc + cb);
    }

    *(USHORT *)piilDest = 0;
    return piilDest;
}


BOOL GieFileDropTarget::FCanDropFiles(HSTRLIST hsl)
{
    ASSERT(hsl != NULL);
    ASSERT(m_hwndProcessDrop != NULL);
    ASSERT(IsWindow(m_hwndProcessDrop));

    return SendMessage(m_hwndProcessDrop, WM_GIE_CANDROPFILES, (WPARAM)this, (LPARAM)hsl);
}


BOOL GieFileDropTarget::FDropFiles(HSTRLIST hsl)
{
    ASSERT(hsl != NULL);
    ASSERT(m_hwndProcessDrop != NULL);
    ASSERT(IsWindow(m_hwndProcessDrop));

    return SendMessage(m_hwndProcessDrop, WM_GIE_FILESDROPPED, (WPARAM)this, (LPARAM)hsl);
}


#if 0
BOOL IsFileDrag(LPDATAOBJECT pIDataObject)
{
    ASSERT(NULL != pIDataObject);

    FORMATETC fe;
    STGMEDIUM sm;
    LPIDA pi;
    TCHAR szPath[256];
    char szBuffer[1024];
    char* pch;
    USHORT* pcb;

    FillMemory(&fe, sizeof(FORMATETC), 0);
    FillMemory(&sm, sizeof(STGMEDIUM), 0);

    fe.cfFormat = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    fe.ptd      = NULL;
    fe.dwAspect = DVASPECT_CONTENT;
    fe.lindex   = -1;
    fe.tymed    = TYMED_HGLOBAL;
    if(FAILED(pIDataObject->GetData(&fe, &sm)))
        return FALSE;
    ASSERT(TYMED_HGLOBAL == sm.tymed);
    pi = (LPIDA)GlobalLock(sm.hGlobal);

    pch = szBuffer;
    pcb = (USHORT*)((char*)pi + pi->aoffset[0]);
    while(*pcb)
    {
        DWORD cb = *pcb;
        
        CopyMemory(pch, pcb, cb);
        pch += cb;
        pcb = (USHORT*)((char*)pcb + cb);
    }
    pcb = (USHORT*)((char*)pi + pi->aoffset[1]);
    while(*pcb)
    {
        DWORD cb = *pcb;
        
        CopyMemory(pch, pcb, cb);
        pch += cb;
        pcb = (USHORT*)((char*)pcb + cb);
    }
    *pch++ = 0;
    *pch++ = 0;

    SHGetPathFromIDList((LPCITEMIDLIST)szBuffer, szPath);

    GlobalUnlock(sm.hGlobal);
    ReleaseStgMedium(&sm);

    return TRUE;

/*
    LPENUMFORMATETC pIEnumFormatEtc;
    FORMATETC fe;
    DWORD c;
    
    if(SUCCEEDED(pIDataObject->EnumFormatEtc(DATADIR_GET, &pIEnumFormatEtc)))
    {
        while(SUCCEEDED(pIEnumFormatEtc->Next(1, &fe, &c)) && 1 == c)
        {
            fe.cfFormat = fe.cfFormat;
        }

        pIEnumFormatEtc->Release();
    }
*/
}
#endif // 0






GieFileDropSource::GieFileDropSource()
{
    m_cRef = 0;
}


GieFileDropSource::~GieFileDropSource()
{
}


STDMETHODIMP GieFileDropSource::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IDropSource)
    {
        *ppvObj = (LPVOID)(IDropSource*)this;
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) GieFileDropSource::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}


STDMETHODIMP_(ULONG) GieFileDropSource::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP GieFileDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed)
        return ResultFromScode(DRAGDROP_S_CANCEL);

    if (0 == (grfKeyState & MK_LBUTTON))
        return ResultFromScode(DRAGDROP_S_DROP);

    return NOERROR;
}


STDMETHODIMP GieFileDropSource::GiveFeedback(DWORD dwEffect)
{
    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}






GieFileDataObject::GieFileDataObject(HWND hwnd)
{
	ASSERT(NULL != hwnd);
	ASSERT(IsWindow(hwnd));

    m_cRef = 0;
	m_hwnd = hwnd;
}


GieFileDataObject::~GieFileDataObject()
{
}


STDMETHODIMP GieFileDataObject::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IDataObject)
    {
        *ppvObj = (LPVOID)(IDataObject*)this;
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) GieFileDataObject::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}


STDMETHODIMP_(ULONG) GieFileDataObject::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP GieFileDataObject::GetData(LPFORMATETC pfe, LPSTGMEDIUM psm)
{
    if (pfe->cfFormat == CF_FILEGROUPDESCRIPTOR)
    {
        if (pfe->ptd == NULL && (pfe->dwAspect & DVASPECT_CONTENT) != 0 && pfe->lindex == -1 && (pfe->tymed & TYMED_HGLOBAL) != 0)
        {
            psm->hGlobal = (HGLOBAL)SendMessage(m_hwnd, WM_GIE_GETFILEGROUPDESCRIPTOR, 0, 0);
			if (psm->hGlobal == NULL)
				return E_INVALIDARG; // OOM

            psm->tymed = TYMED_HGLOBAL;
            psm->pUnkForRelease = NULL;
            return NOERROR;

/*
            FILEGROUPDESCRIPTOR* pfgd;
            
            pfgd = (FILEGROUPDESCRIPTOR*)GlobalAlloc(GMEM_FIXED, sizeof(FILEGROUPDESCRIPTOR));
            pfgd->fgd[0].dwFlags = FD_ATTRIBUTES|FD_WRITESTIME|FD_FILESIZE;
            pfgd->fgd[0].dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
            GetSystemTimeAsFileTime(&pfgd->fgd[0].ftLastWriteTime);
            pfgd->fgd[0].nFileSizeHigh = 0;
            pfgd->fgd[0].nFileSizeLow  = 12;
            lstrcpy(pfgd->fgd[0].cFileName, "dummy12.txt");

            psm->tymed = TYMED_HGLOBAL;
            psm->hGlobal = (HGLOBAL)pfgd;
            psm->pUnkForRelease = NULL;

            return NOERROR;
*/
        }

        return E_INVALIDARG;
    }    

    if (pfe->cfFormat == CF_FILECONTENTS)
    {
        if (pfe->ptd == NULL && (pfe->dwAspect & DVASPECT_CONTENT) != 0 && (pfe->tymed & TYMED_HGLOBAL) != 0)
        {
			psm->hGlobal = (HGLOBAL)SendMessage(m_hwnd, WM_GIE_GETFILECONTENTS, 0, pfe->lindex);
			if (psm->hGlobal == NULL)
				return E_INVALIDARG; // OOM or invalid index

            psm->tymed = TYMED_HGLOBAL;
            psm->pUnkForRelease = NULL;
            return NOERROR;

/*
            if (pfe->lindex >= 1) // # of items
                return E_INVALIDARG;

            char* pch;

            pch = (char*)GlobalAlloc(GMEM_FIXED, 12);
            lstrcpy(pch, "fuck you901");

            psm->tymed = TYMED_HGLOBAL;
            psm->hGlobal = (HGLOBAL)pch;
            psm->pUnkForRelease = NULL;

            return NOERROR;
*/
        }

        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::GetDataHere(LPFORMATETC pfe, LPSTGMEDIUM psm)
{
    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::QueryGetData(LPFORMATETC pfe)
{
    if (pfe->cfFormat == CF_FILEGROUPDESCRIPTOR)
    {
        if (pfe->ptd == NULL && (pfe->dwAspect & DVASPECT_CONTENT) != 0 && pfe->lindex == -1 && (pfe->tymed & TYMED_HGLOBAL) != 0)
        {
            return S_OK;
        }
        return E_INVALIDARG;
    }

    if (pfe->cfFormat == CF_FILECONTENTS)
    {
        if (pfe->ptd == NULL && (pfe->dwAspect & DVASPECT_CONTENT) != 0 && (pfe->tymed & TYMED_HGLOBAL) != 0)
        {
            return S_OK;
        }
        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::GetCanonicalFormatEtc(LPFORMATETC pfeIn, LPFORMATETC pfeOut)
{
    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::SetData(LPFORMATETC pfe, LPSTGMEDIUM psm, BOOL fRelease)
{
    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnum)
{
    GieEnumFormatEtc* pgefe;
    HRESULT hr;

    pgefe = new GieEnumFormatEtc;
    pgefe->AddRef();
    hr = pgefe->QueryInterface(IID_IEnumFORMATETC, (LPVOID*)ppEnum);
    pgefe->Release();

    return hr;
}


STDMETHODIMP GieFileDataObject::DAdvise(LPFORMATETC pfe, DWORD grfAdvise, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
{
    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::DUnadvise(DWORD dwConnection)
{
    return E_NOTIMPL;
}


STDMETHODIMP GieFileDataObject::EnumDAdvise(LPENUMSTATDATA* ppEnum)
{
    return E_NOTIMPL;
}





GieEnumFormatEtc::GieEnumFormatEtc()
{
    m_cRef = 0;
    m_iFormat = 0;
    m_cFormat = 2;
	
    m_rgfe[0].cfFormat = CF_FILEGROUPDESCRIPTOR;
    m_rgfe[0].ptd      = NULL;
    m_rgfe[0].dwAspect = DVASPECT_CONTENT;
    m_rgfe[0].lindex   = -1;
    m_rgfe[0].tymed    = TYMED_HGLOBAL;

    m_rgfe[1].cfFormat = CF_FILECONTENTS;
    m_rgfe[1].ptd      = NULL;
    m_rgfe[1].dwAspect = DVASPECT_CONTENT;
    m_rgfe[1].lindex   = -1;
    m_rgfe[1].tymed    = TYMED_HGLOBAL;
}


GieEnumFormatEtc::~GieEnumFormatEtc()
{
}


STDMETHODIMP GieEnumFormatEtc::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (riid == IID_IUnknown || riid == IID_IEnumFORMATETC)
    {
        *ppvObj = (LPVOID)(IEnumFORMATETC*)this;
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) GieEnumFormatEtc::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}


STDMETHODIMP_(ULONG) GieEnumFormatEtc::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP GieEnumFormatEtc::Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched)
{
    UINT cFetch;
    HRESULT hr;

    hr = S_FALSE;
    if (m_iFormat < m_cFormat)
    {
        cFetch = m_cFormat - m_iFormat;
        if (cFetch >= celt)
        {
            cFetch = celt;
            hr = S_OK;
        }
        CopyMemory(rgelt, &m_rgfe[m_iFormat], cFetch * sizeof(FORMATETC));
        m_iFormat += celt;
    }
    else
    {
        cFetch = 0;
    }

    if (pceltFetched != NULL)
        *pceltFetched = cFetch;

    return hr;
}


STDMETHODIMP GieEnumFormatEtc::Skip(ULONG celt)
{
    m_iFormat += celt;
    if (m_iFormat > m_cFormat)
    {
        m_iFormat = m_cFormat;
        return S_FALSE;
    }

    return S_OK;
}


STDMETHODIMP GieEnumFormatEtc::Reset()
{
    m_iFormat = 0;
    return NOERROR;
}


STDMETHODIMP GieEnumFormatEtc::Clone(IEnumFORMATETC** ppenum)
{
    return E_NOTIMPL;
}


HFILEDROPTGT RegisterFileDropTarget(HWND hwndProcessDrop, HWND hwndTarget)
{
    LPDROPTARGET pIDropTarget;

    ASSERT(hwndProcessDrop != NULL);
    ASSERT(IsWindow(hwndProcessDrop));
    ASSERT(hwndTarget != NULL);
    ASSERT(IsWindow(hwndTarget));

    pIDropTarget = new GieFileDropTarget(hwndProcessDrop, hwndTarget);
    if (pIDropTarget == NULL)
        return NULL;

    pIDropTarget->AddRef();
    CoLockObjectExternal(pIDropTarget, TRUE, FALSE);
    RegisterDragDrop(hwndTarget, pIDropTarget);

    return (HFILEDROPTGT)pIDropTarget;
}


void UnregisterFileDropTarget(HFILEDROPTGT hfdt)
{
    LPDROPTARGET pIDropTarget;
    
    if (hfdt == NULL)
        return;

    pIDropTarget = (LPDROPTARGET)hfdt;

    RevokeDragDrop(((GieFileDropTarget*)pIDropTarget)->GetTargetHwnd());
    CoLockObjectExternal(pIDropTarget, FALSE, TRUE);
    pIDropTarget->Release();
}


BOOL DoDrag(HWND hwnd)
{
    ASSERT(hwnd != NULL);
    ASSERT(IsWindow(hwnd));

	LPDROPSOURCE pIDropSource;
	LPDATAOBJECT pIDataObject;
	DWORD dwEffect;

	pIDropSource = new GieFileDropSource;
	if (pIDropSource == NULL)
		return FALSE;
	pIDropSource->AddRef();

	pIDataObject = new GieFileDataObject(hwnd);
	if (pIDataObject == NULL)
	{
		pIDropSource->Release();
		return FALSE;
	}
	pIDataObject->AddRef();

	DoDragDrop(pIDataObject, pIDropSource, DROPEFFECT_COPY, &dwEffect);

	pIDropSource->Release();
	pIDataObject->Release();

	return TRUE;
}
