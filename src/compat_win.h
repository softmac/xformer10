/* compat_win.h - Win32 type/macro shims for non-Windows builds */
#ifndef COMPAT_WIN_H
#define COMPAT_WIN_H

#ifndef _WIN32

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>

/* Calling convention no-ops */
#define __cdecl
#define __fastcall
/* __inline without static is C99 "external linkage inline" which requires an external def elsewhere.
   Use static inline so each TU has its own copy — correct for header-defined and same-TU helpers. */
#define __inline static inline
#define __forceinline static inline
#define FAR
#define IN
#define OUT
#define WINAPI
#define APIENTRY
#define CALLBACK
#define _cdecl
#define __stdcall
#define __declspec(x)

/* Basic types */
typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint64_t  QWORD;
typedef int       BOOL;
typedef char      CHAR;
typedef uint16_t  WCHAR;
typedef char     *LPSTR;
typedef const char *LPCSTR;
typedef void     *LPVOID;
typedef void     *HANDLE;
typedef void     *HWND;
typedef void     *HDC;
typedef void     *HBITMAP;
typedef void     *HMENU;
typedef void     *HINSTANCE;
typedef void     *HICON;
typedef void     *HCURSOR;
typedef uint32_t  ULONG;
typedef int32_t   LONG;
typedef int64_t   LONGLONG;
typedef uint64_t  ULONGLONG;
typedef int16_t   SHORT;
typedef uint16_t  USHORT;
typedef int       INT;
typedef unsigned int UINT;
typedef float     FLOAT;
typedef double    DOUBLE;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t ULONG_PTR;

/* Message/pointer-sized types */
typedef uintptr_t WPARAM;
typedef intptr_t  LPARAM;
typedef intptr_t  LRESULT;
typedef intptr_t  LONG_PTR;
typedef uintptr_t UINT_PTR;

/* COM error type */
typedef int32_t   HRESULT;
#define S_OK         ((HRESULT)0)
#define S_FALSE      ((HRESULT)1)
#define E_FAIL       ((HRESULT)0x80004005)
#define E_INVALIDARG ((HRESULT)0x80070057)
#define E_NOTIMPL    ((HRESULT)0x80004001)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr)    ((HRESULT)(hr) < 0)

/* Function pointer / string types */
typedef intptr_t (*FARPROC)(void);
typedef const char *PCSTR;
typedef const uint16_t *PCWSTR;
typedef uint16_t *PWSTR;
typedef BYTE     *PBYTE;
typedef DWORD    *LPDWORD;

/* MSVC 64-bit integer compat */
typedef int64_t  __int64;

typedef union _LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; };
    LONGLONG QuadPart;
} LARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct { DWORD LowPart; DWORD HighPart; };
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* GDI geometry types */
typedef struct tagPOINT { LONG x; LONG y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagRECT  { LONG left; LONG top; LONG right; LONG bottom; } RECT, *PRECT, *LPRECT;
typedef struct tagSIZE  { LONG cx; LONG cy; } SIZE, *PSIZE;

/* GDI color/bitmap types */
typedef struct tagRGBQUAD {
    BYTE rgbBlue; BYTE rgbGreen; BYTE rgbRed; BYTE rgbReserved;
} RGBQUAD;
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize; LONG biWidth; LONG biHeight;
    WORD  biPlanes; WORD biBitCount;
    DWORD biCompression; DWORD biSizeImage;
    LONG  biXPelsPerMeter; LONG biYPelsPerMeter;
    DWORD biClrUsed; DWORD biClrImportant;
} BITMAPINFOHEADER;
typedef void *HPALETTE;

/* DirectDraw stub (forward declaration for pointer use in gemtypes.h) */
typedef struct IDirectDrawSurface IDirectDrawSurface;

/* Multimedia result / audio types (stub structs for INST storage) */
typedef UINT MMRESULT;
#define MMSYSERR_NOERROR 0
typedef struct tagWAVEOUTCAPS {
    WORD  wMid; WORD wPid;
    DWORD vDriverVersion;
    char  szPname[32];
    DWORD dwFormats;
    WORD  wChannels; WORD wReserved1;
    DWORD dwSupport;
} WAVEOUTCAPS;
#define WHDR_DONE     0x00000001u
#define WHDR_PREPARED 0x00000002u
typedef struct tagWAVEHDR {
    char  *lpData;
    DWORD  dwBufferLength;
    DWORD  dwBytesRecorded;
    DWORD_PTR dwUser;
    DWORD  dwFlags;
    DWORD  dwLoops;
    struct tagWAVEHDR *lpNext;
    DWORD_PTR reserved;
} WAVEHDR;

/* Joystick types (stub structs for INST storage) */
#define JOY_RETURNALL 0x000000FFul
typedef struct joyinfoex_tag {
    DWORD dwSize; DWORD dwFlags;
    DWORD dwXpos; DWORD dwYpos; DWORD dwZpos;
    DWORD dwRpos; DWORD dwUpos; DWORD dwVpos;
    DWORD dwButtons; DWORD dwButtonNumber;
    DWORD dwPOV; DWORD dwReserved1; DWORD dwReserved2;
} JOYINFOEX;
typedef struct tagJOYCAPS {
    WORD wMid; WORD wPid;
    char szPname[32];
    UINT wXmin; UINT wXmax; UINT wYmin; UINT wYmax;
    UINT wZmin; UINT wZmax; UINT wNumButtons;
    UINT wPeriodMin; UINT wPeriodMax;
    UINT wRmin; UINT wRmax; UINT wUmin; UINT wUmax;
    UINT wVmin; UINT wVmax;
    UINT wCaps; UINT wMaxAxes; UINT wNumAxes; UINT wMaxButtons;
    char szRegKey[32];
    char szOEMVxD[260];
} JOYCAPS;

/* System info stub */
typedef struct _SYSTEM_INFO {
    DWORD     dwOemId;
    DWORD     dwPageSize;
    void     *lpMinimumApplicationAddress;
    void     *lpMaximumApplicationAddress;
    DWORD_PTR dwActiveProcessorMask;
    DWORD     dwNumberOfProcessors;
    DWORD     dwProcessorType;
    DWORD     dwAllocationGranularity;
    WORD      wProcessorLevel;
    WORD      wProcessorRevision;
} SYSTEM_INFO;

/* INT_PTR / UINT_PTR signed/unsigned pointer-sized integer */
typedef intptr_t  INT_PTR;

/* FILETIME and SYSTEMTIME (used by mac_hfs.c and WIN32_FIND_DATA) */
typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME;
typedef struct _SYSTEMTIME {
    WORD wYear; WORD wMonth; WORD wDayOfWeek; WORD wDay;
    WORD wHour; WORD wMinute; WORD wSecond; WORD wMilliseconds;
} SYSTEMTIME;
static inline BOOL SystemTimeToFileTime(const SYSTEMTIME *st, FILETIME *ft)
    { (void)st; if (ft) { ft->dwLowDateTime = 0; ft->dwHighDateTime = 0; } return TRUE; }
static inline BOOL FileTimeToSystemTime(const FILETIME *ft, SYSTEMTIME *st)
    { (void)ft; if (st) { st->wYear=1904; st->wMonth=1; st->wDay=1; st->wHour=0; st->wMinute=0; st->wSecond=0; st->wMilliseconds=0; st->wDayOfWeek=0; } return TRUE; }

/* Drive type constants (used by PdiOpenDisk in blockapi.c) */
#define DRIVE_UNKNOWN         0
#define DRIVE_NO_ROOT_DIR     1
#define DRIVE_REMOVABLE       2
#define DRIVE_FIXED           3
#define DRIVE_REMOTE          4
#define DRIVE_CDROM           5
#define DRIVE_RAMDISK         6
static inline UINT GetDriveType(const char *path) { (void)path; return DRIVE_UNKNOWN; } // LATER:

/* Dynamic library stubs (ASPI not available on Linux) */
static inline HANDLE LoadLibrary(const char *name) { (void)name; return NULL; } // LATER:
static inline FARPROC GetProcAddress(HANDLE h, const char *name) { (void)h; (void)name; return NULL; } // LATER:
static inline BOOL FreeLibrary(HANDLE h) { (void)h; return TRUE; } // LATER:

/* Heap compact stub */
static inline DWORD HeapCompact(HANDLE h, DWORD flags) { (void)h; (void)flags; return 0; }

/* VirtualAlloc/VirtualFree stubs (disk I/O buffers; map to malloc/free) */
#define MEM_COMMIT     0x1000
#define MEM_RESERVE    0x2000
#define MEM_DECOMMIT   0x4000
#define MEM_RELEASE    0x8000
#define PAGE_READWRITE 0x04
#define PAGE_READONLY  0x02
static inline void *VirtualAlloc(void *addr, size_t sz, DWORD type, DWORD prot)
    { (void)addr; (void)type; (void)prot; return malloc(sz); } // LATER:
static inline BOOL VirtualFree(void *p, size_t sz, DWORD type)
    { (void)sz; if (type & MEM_RELEASE) free(p); return TRUE; } // LATER:

/* SetErrorMode stub */
#define SEM_FAILCRITICALERRORS 0x0001
static inline DWORD SetErrorMode(DWORD mode) { (void)mode; return 0; } // LATER:

/* FSCTL constants (from winioctl.h; all disk I/O stubbed to FALSE) */
#define FSCTL_LOCK_VOLUME      0x00090018
#define FSCTL_UNLOCK_VOLUME    0x0009001C
#define FSCTL_DISMOUNT_VOLUME  0x00090020

/* MEDIA_TYPE enum value used in sectorio.c */
#define F3_720_512  6

/* DeviceIoControl stub (raw disk I/O not needed in Phase 1) */
typedef void *LPOVERLAPPED;
static inline BOOL DeviceIoControl(HANDLE h, DWORD code, void *in, DWORD inSz,
                                    void *out, DWORD outSz, DWORD *ret, LPOVERLAPPED ov)
    { (void)h; (void)code; (void)in; (void)inSz; (void)out; (void)outSz; (void)ret; (void)ov; return FALSE; } // LATER:
#define IOCTL_DISK_GET_DRIVE_GEOMETRY 0x00070000
#define IOCTL_DISK_GET_PARTITION_INFO  0x00074004

/* Disk geometry stub (for blockdev.h GetDiskGeometry) */
typedef struct _DISK_GEOMETRY {
    LONGLONG Cylinders;
    DWORD    MediaType;
    DWORD    TracksPerCylinder;
    DWORD    SectorsPerTrack;
    DWORD    BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

/* MessageBox flags and stub (used in #ifndef NDEBUG _gem_assert in gemtypes.h) */
#define MB_OK                  0x00000000u
#define MB_ABORTRETRYIGNORE    0x00000002u
#define MB_APPLMODAL           0x00000000u
#define MB_ICONERROR           0x00000010u
#define MB_ICONINFORMATION     0x00000040u
static inline int MessageBox(void *hwnd, const char *text, const char *caption, unsigned flags)
    { (void)hwnd; (void)flags; fprintf(stderr, "%s: %s\n", caption, text); return 3; } // LATER:
static inline void *GetFocus(void) { return NULL; }
#define wsprintf sprintf
#ifndef __debugbreak
#define __debugbreak() __builtin_trap()
#endif

/* VOID alias (used in old Win32 headers like wnaspi32.h) */
#ifndef VOID
#define VOID void
#endif

/* Near-memory copy/set aliases */
#define _fmemcpy  memcpy
#define _fmemset  memset

/* POSIX I/O aliases and flag names */
#define _read   read
#define _write  write
#define _open   open
#define _close  close
#define _lseek  lseek
#define _O_RDONLY  O_RDONLY
#define _O_WRONLY  O_WRONLY
#define _O_RDWR    O_RDWR
#define _O_CREAT   O_CREAT
#define _O_TRUNC   O_TRUNC
#define _O_APPEND  O_APPEND
#define _O_BINARY  0
#define _O_TEXT    0

/* Old-style OpenFile constants (used by atari800.h macro remapping) */
#define OF_READ       O_RDONLY
#define OF_READWRITE  O_RDWR
#define OF_SHARE_COMPAT 0

/* HFILE: old-style file handle (int fd) */
typedef int HFILE;

/* Additional string pointer types */
typedef const char *LPCCH;
typedef const char *LPCTSTR;
typedef char       *LPTSTR;
typedef WORD       *LPWORD;

/* MessageBox icon aliases */
#ifndef MB_ICONHAND
#define MB_ICONHAND  MB_ICONERROR
#endif

/* Bit-field extraction macros */
#define LOWORD(l)      ((WORD)((DWORD_PTR)(l) & 0xFFFF))
#define HIWORD(l)      ((WORD)(((DWORD_PTR)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)      ((BYTE)((DWORD_PTR)(w) & 0xFF))
#define HIBYTE(w)      ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xFF))
#define MAKELONG(lo,hi) ((LONG)(((WORD)(lo)) | (((DWORD)((WORD)(hi))) << 16)))
#define MAKEWORD(lo,hi) ((WORD)(((BYTE)(lo)) | (((WORD)((BYTE)(hi))) << 8)))
#define MAKELPARAM(lo,hi) ((LPARAM)MAKELONG(lo,hi))
#define MAKELRESULT(lo,hi) ((LRESULT)MAKELONG(lo,hi))

/* min/max (not defined by standard C — Windows defines them in <windef.h>) */
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* Heap allocation stubs (map to malloc/calloc/free) */
#define HEAP_NO_SERIALIZE  0x00000001u
#define HEAP_ZERO_MEMORY   0x00000008u
#define HEAP_GENERATE_EXCEPTIONS 0x00000004u
static inline HANDLE GetProcessHeap(void) { return (HANDLE)1; }
static inline void *HeapAlloc(HANDLE h, DWORD flags, size_t size)
    { (void)h; return (flags & HEAP_ZERO_MEMORY) ? calloc(1, size) : malloc(size); }
static inline BOOL HeapFree(HANDLE h, DWORD flags, void *p)
    { (void)h; (void)flags; free(p); return TRUE; }
static inline void *HeapReAlloc(HANDLE h, DWORD flags, void *p, size_t size)
    { (void)h; (void)flags; return realloc(p, size); }

/* Console handle constants and stubs */
#define STD_INPUT_HANDLE  ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)
static inline HANDLE GetStdHandle(DWORD n) { (void)n; return (HANDLE)(intptr_t)0; }
static inline BOOL ReadConsole(HANDLE h, void *buf, DWORD n, DWORD *read, void *res)
    { (void)h; (void)buf; (void)n; (void)res; if (read) *read = 0; return FALSE; } // LATER:
extern SHORT sdl_get_async_key_state(int vk);
static inline SHORT GetAsyncKeyState(int vk) { return sdl_get_async_key_state(vk); }

/* Windows message constants */
#define WM_NULL           0x0000
#define WM_CREATE         0x0001
#define WM_DESTROY        0x0002
#define WM_SIZE           0x0005
#define WM_ACTIVATE       0x0006
#define WM_SETFOCUS       0x0007
#define WM_KILLFOCUS      0x0008
#define WM_PAINT          0x000F
#define WM_CLOSE          0x0010
#define WM_QUIT           0x0012
#define WM_TIMER          0x0113
#define WM_COMMAND        0x0111
#define WM_KEYDOWN        0x0100
#define WM_KEYUP          0x0101
#define WM_CHAR           0x0102
#define WM_SYSKEYDOWN     0x0104
#define WM_SYSKEYUP       0x0105
#define WM_MOUSEMOVE      0x0200
#define WM_LBUTTONDOWN    0x0201
#define WM_LBUTTONUP      0x0202
#define WM_RBUTTONDOWN    0x0204
#define WM_RBUTTONUP      0x0205
#define WM_USER           0x0400

/* PeekMessage/PostMessage flags */
#define PM_NOREMOVE 0x0000
#define PM_REMOVE   0x0001

/* MSG structure */
typedef struct tagMSG {
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
} MSG, *PMSG, *LPMSG;

/* Message queue stubs */
static inline BOOL PeekMessage(MSG *msg, HWND hwnd, UINT min, UINT max, UINT remove)
    { (void)msg; (void)hwnd; (void)min; (void)max; (void)remove; return FALSE; } // LATER:
static inline BOOL PostMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    { (void)hwnd; (void)msg; (void)wp; (void)lp; return FALSE; } // LATER:
static inline BOOL TranslateMessage(const MSG *msg)
    { (void)msg; return FALSE; }
static inline LRESULT DispatchMessage(const MSG *msg)
    { (void)msg; return 0; }
static inline void PostQuitMessage(int code) { (void)code; }

/* Virtual key codes */
#define VK_LBUTTON   0x01
#define VK_RBUTTON   0x02
#define VK_MBUTTON   0x04
#define VK_BACK      0x08
#define VK_TAB       0x09
#define VK_RETURN    0x0D
#define VK_SHIFT     0x10
#define VK_CONTROL   0x11
#define VK_MENU      0x12
#define VK_PAUSE     0x13
#define VK_CAPITAL   0x14
#define VK_ESCAPE    0x1B
#define VK_SPACE     0x20
#define VK_PRIOR     0x21
#define VK_NEXT      0x22
#define VK_END       0x23
#define VK_HOME      0x24
#define VK_LEFT      0x25
#define VK_UP        0x26
#define VK_RIGHT     0x27
#define VK_DOWN      0x28
#define VK_INSERT    0x2D
#define VK_DELETE    0x2E
#define VK_F1        0x70
#define VK_F2        0x71
#define VK_F3        0x72
#define VK_F4        0x73
#define VK_F5        0x74
#define VK_F6        0x75
#define VK_F7        0x76
#define VK_F8        0x77
#define VK_F9        0x78
#define VK_F10       0x79
#define VK_F11       0x7A
#define VK_F12       0x7B
#define VK_NUMPAD0   0x60
#define VK_NUMPAD1   0x61
#define VK_NUMPAD2   0x62
#define VK_NUMPAD3   0x63
#define VK_NUMPAD4   0x64
#define VK_NUMPAD5   0x65
#define VK_NUMPAD6   0x66
#define VK_NUMPAD7   0x67
#define VK_NUMPAD8   0x68
#define VK_NUMPAD9   0x69
#define VK_MULTIPLY  0x6A
#define VK_ADD       0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT  0x6D
#define VK_DECIMAL   0x6E
#define VK_DIVIDE    0x6F
#define VK_NUMLOCK   0x90
#define VK_SCROLL    0x91
#define VK_LSHIFT    0xA0
#define VK_RSHIFT    0xA1
#define VK_LCONTROL  0xA2
#define VK_RCONTROL  0xA3
#define VK_LMENU     0xA4
#define VK_RMENU     0xA5

/* Multimedia joystick message constants */
#define MM_JOY1MOVE       0x03A1
#define MM_JOY2MOVE       0x03A3
#define MM_JOY1BUTTONDOWN 0x03B5
#define MM_JOY1BUTTONUP   0x03B6
#define MM_JOY2BUTTONDOWN 0x03B7
#define MM_JOY2BUTTONUP   0x03B8
#define JOY_BUTTON1       0x0001
#define JOY_BUTTON2       0x0002
#define JOY_BUTTON3       0x0004
#define JOY_BUTTON4       0x0008

/* Timing stubs */
static inline void Sleep(DWORD ms) { usleep((unsigned long)ms * 1000); }
static inline DWORD GetTickCount(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (DWORD)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
static inline BOOL QueryPerformanceCounter(LARGE_INTEGER *li) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    if (li) li->QuadPart = (LONGLONG)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return TRUE;
}
static inline BOOL QueryPerformanceFrequency(LARGE_INTEGER *li) {
    if (li) li->QuadPart = 1000000000LL; return TRUE;
}

/* Debug output stub */
static inline void OutputDebugString(const char *s) { (void)s; }
#define OutputDebugStringA OutputDebugString

/* Directory stubs */
static inline DWORD GetCurrentDirectory(DWORD sz, char *buf) {
    return getcwd(buf, (size_t)sz) ? (DWORD)strlen(buf) : 0;
}
static inline BOOL SetCurrentDirectory(const char *path) { return chdir(path) == 0; }
static inline DWORD GetWindowsDirectory(char *buf, DWORD sz) { if (buf && sz > 0) buf[0] = 0; return 0; }
static inline BOOL CreateDirectory(const char *path, void *sa) { (void)sa; mkdir(path, 0755); return TRUE; }

/* System metrics stub */
#define SM_CXSCREEN         0
#define SM_CYSCREEN         1
#define SM_CXVIRTUALSCREEN  78
#define SM_CYVIRTUALSCREEN  79
#define SM_CXSIZEFRAME      32
#define SM_CYSIZEFRAME      33
#define SM_CYCAPTION        4
#define SM_CXFULLSCREEN     16
#define SM_CYFULLSCREEN     17
static inline int GetSystemMetrics(int idx) {
    if (idx == SM_CXSCREEN || idx == SM_CYSCREEN) {
        SDL_DisplayMode m;
        if (SDL_GetDisplayMode(0, 0, &m) == 0)
            return (idx == SM_CXSCREEN) ? m.w : m.h;
    }
    return 0;
}

/* Window show state constants */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_RESTORE          9

/* Window style constants */
#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CAPTION          0x00C00000L
#define WS_SYSMENU          0x00080000L
#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L
#define WS_SIZEBOX          0x00040000L
#define CS_HREDRAW          0x0002
#define CS_VREDRAW          0x0001
#define CS_OWNDC            0x0020

/* Shell/SHFolder stubs */
#define CSIDL_APPDATA       0x001a
#define CSIDL_WINDOWS       0x0024
static inline HRESULT SHGetFolderPath(HWND h, int f, HANDLE t, DWORD fl, char *buf)
    { (void)h; (void)f; (void)t; (void)fl; if (buf) buf[0]=0; return E_NOTIMPL; } // LATER:
typedef void *LPITEMIDLIST;
static inline LPITEMIDLIST SHBrowseForFolder(void *bi)
    { (void)bi; return NULL; } // LATER:
static inline BOOL SHGetPathFromIDList(LPITEMIDLIST id, char *buf)
    { (void)id; if (buf) buf[0]=0; return FALSE; } // LATER:
#define SHGFI_DISPLAYNAME 0x200
#define SHGFI_USEFILEATTRIBUTES 0x10
typedef struct { void *hIcon; int iIcon; DWORD dwAttributes; char szDisplayName[260]; char szTypeName[80]; } SHFILEINFO;
static inline DWORD_PTR SHGetFileInfo(const char *p, DWORD a, SHFILEINFO *fi, UINT sz, UINT fl)
    { (void)p; (void)a; (void)fi; (void)sz; (void)fl; return 0; } // LATER:

/* Thread/synchronization — pthreads-backed Win32 handles */
#define INFINITE               0xFFFFFFFFu
#define WAIT_OBJECT_0          0u
#define WAIT_TIMEOUT           0x00000102u
#define WAIT_FAILED            0xFFFFFFFFu
#define MAXIMUM_WAIT_OBJECTS   64
#define THREAD_PRIORITY_LOWEST         (-2)
#define THREAD_PRIORITY_BELOW_NORMAL   (-1)
#define THREAD_PRIORITY_NORMAL         0
#define THREAD_PRIORITY_ABOVE_NORMAL   1
#define THREAD_PRIORITY_HIGHEST        2
#define THREAD_PRIORITY_TIME_CRITICAL  15
/* Magic type tags for heap-allocated handles; values >> any valid fd */
#define LH_EVENT   0x45564E54u  /* 'EVNT' */
#define LH_THREAD  0x54485244u  /* 'THRD' */
typedef struct { unsigned int type; pthread_mutex_t m; pthread_cond_t c; volatile int set; int manual; } LinuxEvent;
typedef struct { unsigned int type; pthread_t tid; } LinuxThread;
typedef struct { DWORD (*fn)(void *); void *arg; } LinuxThArgs;
static __attribute__((unused)) void *lh_thread_entry(void *p) {
    LinuxThArgs *a = (LinuxThArgs *)p;
    DWORD (*fn)(void *) = a->fn; void *arg = a->arg; free(a); fn(arg); return NULL;
}
static inline HANDLE CreateEvent(void *sa, BOOL manual, BOOL init, const char *name) {
    (void)sa; (void)name;
    LinuxEvent *e = (LinuxEvent *)calloc(1, sizeof(LinuxEvent));
    if (!e) return NULL;
    e->type = LH_EVENT; e->manual = manual ? 1 : 0; e->set = init ? 1 : 0;
    pthread_mutex_init(&e->m, NULL); pthread_cond_init(&e->c, NULL);
    return (HANDLE)e;
}
static inline BOOL SetEvent(HANDLE h) {
    LinuxEvent *e = (LinuxEvent *)h;
    pthread_mutex_lock(&e->m); e->set = 1; pthread_cond_broadcast(&e->c);
    pthread_mutex_unlock(&e->m); return TRUE;
}
static inline BOOL ResetEvent(HANDLE h) {
    LinuxEvent *e = (LinuxEvent *)h;
    pthread_mutex_lock(&e->m); e->set = 0; pthread_mutex_unlock(&e->m); return TRUE;
}
static inline DWORD WaitForSingleObject(HANDLE h, DWORD ms) {
    LinuxEvent *e = (LinuxEvent *)h;
    pthread_mutex_lock(&e->m);
    if (ms == INFINITE) {
        while (!e->set) pthread_cond_wait(&e->c, &e->m);
    } else {
        struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ms / 1000; ts.tv_nsec += (long)(ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
        while (!e->set) {
            if (pthread_cond_timedwait(&e->c, &e->m, &ts) != 0) {
                pthread_mutex_unlock(&e->m); return WAIT_TIMEOUT;
            }
        }
    }
    if (!e->manual) e->set = 0;
    pthread_mutex_unlock(&e->m); return WAIT_OBJECT_0;
}
static inline DWORD WaitForMultipleObjects(DWORD n, const HANDLE *h, BOOL all, DWORD ms) {
    (void)all;
    for (DWORD i = 0; i < n; i++) { DWORD r = WaitForSingleObject(h[i], ms); if (r != WAIT_OBJECT_0) return r; }
    return WAIT_OBJECT_0;
}
static inline HANDLE CreateThread(void *sa, size_t stack, DWORD (*fn)(void *), void *arg, DWORD flags, DWORD *id) {
    (void)sa; (void)stack; (void)flags; (void)id;
    LinuxThread *t = (LinuxThread *)calloc(1, sizeof(LinuxThread));
    LinuxThArgs *a = (LinuxThArgs *)malloc(sizeof(LinuxThArgs));
    if (!t || !a) { free(t); free(a); return NULL; }
    t->type = LH_THREAD; a->fn = fn; a->arg = arg;
    if (pthread_create(&t->tid, NULL, lh_thread_entry, a) != 0) { free(a); free(t); return NULL; }
    return (HANDLE)t;
}
static inline BOOL TerminateThread(HANDLE h, DWORD code) {
    (void)code; pthread_cancel(((LinuxThread *)h)->tid); return TRUE;
}
/* Unified CloseHandle: detects pthread handles by magic tag; falls back to fd close.
   Guard prevents redefinition from compat_winfile.h in TUs that include both headers. */
#define LINUX_LH_CLOSEHANDLE 1
static inline BOOL CloseHandle(HANDLE h) {
    if (!h || h == (HANDLE)(uintptr_t)-1) return FALSE;
    uintptr_t hval = (uintptr_t)h;
    /* Heap pointers are well above the fd range; fds stored as (HANDLE)(intptr_t)fd are small */
    if (hval > 65535u && (hval & 3u) == 0u) {
        unsigned int tag = *(unsigned int *)h;
        if (tag == LH_EVENT) {
            LinuxEvent *e = (LinuxEvent *)h;
            pthread_mutex_destroy(&e->m); pthread_cond_destroy(&e->c); free(e); return TRUE;
        } else if (tag == LH_THREAD) { free(h); return TRUE; }
    }
    return close((int)(intptr_t)h) == 0 ? TRUE : FALSE;
}
static inline BOOL GetExitCodeThread(HANDLE h, DWORD *c) { (void)h; if (c) *c = 0; return TRUE; }
static inline HANDLE GetCurrentThread(void) { return (HANDLE)(uintptr_t)pthread_self(); }
static inline BOOL SetThreadPriority(HANDLE h, int prio) { (void)h; (void)prio; return TRUE; }

/* System info */
static inline void GetSystemInfo(SYSTEM_INFO *si) {
    if (si) { memset(si, 0, sizeof(*si)); si->dwNumberOfProcessors = (DWORD)sysconf(_SC_NPROCESSORS_ONLN); }
}

/* GetLastError */
static inline DWORD GetLastError(void) { return 0; }
static inline void SetLastError(DWORD e) { (void)e; }

/* MessageBox flags additions */
#define MB_YESNO           0x00000004u
#define MB_YESNOCANCEL     0x00000003u
#define IDOK               1
#define IDCANCEL           2
#define IDABORT            3
#define IDRETRY            4
#define IDIGNORE           5
#define IDYES              6
#define IDNO               7

/* File permission flag aliases (MSVC uses _S_IREAD/_S_IWRITE) */
#ifndef _S_IREAD
#define _S_IREAD  S_IRUSR
#endif
#ifndef _S_IWRITE
#define _S_IWRITE S_IWUSR
#endif

/* Wave audio types and stubs */
typedef void *HWAVEOUT;
typedef struct tagWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX;
typedef WAVEHDR *LPWAVEHDR;
#define WOM_DONE            0x3BDu
#define WAVE_FORMAT_PCM     1u
#define WAVE_FORMAT_48M16   0x00000800u
#define CALLBACK_FUNCTION   0x00030000u
static inline UINT waveOutGetNumDevs(void) { return 0; }
static inline MMRESULT waveOutGetDevCaps(UINT d, WAVEOUTCAPS *c, UINT sz)
    { (void)d; (void)c; (void)sz; return 1; }
static inline MMRESULT waveOutOpen(HWAVEOUT *h, UINT d, WAVEFORMATEX *f,
    DWORD_PTR cb, DWORD_PTR ci, DWORD fl)
    { (void)h; (void)d; (void)f; (void)cb; (void)ci; (void)fl; return 1; }
static inline MMRESULT waveOutClose(HWAVEOUT h) { (void)h; return 0; }
static inline MMRESULT waveOutReset(HWAVEOUT h) { (void)h; return 0; }
static inline MMRESULT waveOutPrepareHeader(HWAVEOUT h, WAVEHDR *wh, UINT sz)
    { (void)h; (void)wh; (void)sz; return 0; }
static inline MMRESULT waveOutUnprepareHeader(HWAVEOUT h, WAVEHDR *wh, UINT sz)
    { (void)h; (void)wh; (void)sz; return 0; }
static inline MMRESULT waveOutWrite(HWAVEOUT h, WAVEHDR *wh, UINT sz)
    { (void)h; (void)wh; (void)sz; return 1; }

/* MIDI stubs */
typedef struct { WORD wMid; WORD wPid; DWORD vDriverVersion; char szPname[32]; DWORD dwSupport; } MIDIOUTCAPS;
typedef struct { WORD wMid; WORD wPid; DWORD vDriverVersion; char szPname[32]; DWORD dwSupport; } MIDIINCAPS;
static inline UINT midiOutGetNumDevs(void) { return 0; }
static inline MMRESULT midiOutGetDevCaps(UINT d, MIDIOUTCAPS *c, UINT sz)
    { (void)d; (void)c; (void)sz; return 1; }
static inline UINT midiInGetNumDevs(void) { return 0; }
static inline MMRESULT midiInGetDevCaps(UINT d, MIDIINCAPS *c, UINT sz)
    { (void)d; (void)c; (void)sz; return 1; }

/* Joystick additions */
typedef struct { UINT wXpos; UINT wYpos; UINT wZpos; UINT wButtons; } JOYINFO;
#define JOY_RETURNBUTTONS   0x80u
#define JOY_RETURNX         0x01u
#define JOY_RETURNY         0x02u
#define JOYSTICKID1         0u
static inline MMRESULT joyConfigChanged(DWORD f) { (void)f; return 0; }
static inline UINT joyGetNumDevs(void) { return 0; }
static inline MMRESULT joyGetPosEx(UINT id, JOYINFOEX *pji) { (void)id; (void)pji; return 1; }
static inline MMRESULT joyGetDevCaps(UINT id, JOYCAPS *jc, UINT sz)
    { (void)id; (void)jc; (void)sz; return 1; }
static inline MMRESULT joyGetPos(UINT id, JOYINFO *pji) { (void)id; (void)pji; return 1; }

/* ROP codes for BitBlt */
#define SRCCOPY     0x00CC0020
#define SRCPAINT    0x00EE0086
#define SRCAND      0x008800C6
#define SRCINVERT   0x00660046
#define SRCERASE    0x00440328
#define BLACKNESS   0x00000042
#define WHITENESS   0x00FF0062
#define PATCOPY     0x00F00021

/* SW_MAXIMIZE alias */
#define SW_MAXIMIZE SW_SHOWMAXIMIZED

/* SHBrowseForFolder callback messages */
#define BFFM_INITIALIZED    1
#define BFFM_SELCHANGED     2
#define BFFM_SETSELECTION   0x0467
#define BFFM_SETSTATUSTEXT  0x0464

/* CONST qualifier alias */
#ifndef CONST
#define CONST const
#endif

/* GDI bitmap compression */
#define BI_RGB  0
#define BI_RLE8 1
#define BI_RLE4 2
#define DIB_RGB_COLORS 0

/* BITMAPINFO (header + palette) */
typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;

/* _MAX_PATH: MSVC alias */
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

/* CreateThread custom stack flag */
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000

/* Menu item state/type flags */
#define MF_ENABLED    0x00000000u
#define MF_GRAYED     0x00000001u
#define MF_DISABLED   0x00000002u
#define MF_UNCHECKED  0x00000000u
#define MF_CHECKED    0x00000008u
#define MF_STRING     0x00000000u
#define MF_SEPARATOR  0x00000800u

/* InsertMenuItem / GetMenuItemInfo mask */
#define MIIM_STATE    0x00000001u
#define MIIM_ID       0x00000002u
#define MIIM_SUBMENU  0x00000004u
#define MIIM_STRING   0x00000040u
#define MIIM_DATA     0x00000020u

typedef struct tagMENUITEMINFO {
    UINT      cbSize;
    UINT      fMask;
    UINT      fType;
    UINT      fState;
    UINT      wID;
    HMENU     hSubMenu;
    HBITMAP   hbmpChecked;
    HBITMAP   hbmpUnchecked;
    ULONG_PTR dwItemData;
    LPTSTR    dwTypeData;
    UINT      cch;
    HBITMAP   hbmpItem;
} MENUITEMINFO, *LPMENUITEMINFO;

static inline BOOL InsertMenuItem(HMENU m, UINT i, BOOL byPos, const MENUITEMINFO *mi)
    { (void)m; (void)i; (void)byPos; (void)mi; return FALSE; } // LATER:
static inline BOOL GetMenuItemInfo(HMENU m, UINT i, BOOL byPos, MENUITEMINFO *mi)
    { (void)m; (void)i; (void)byPos; (void)mi; return FALSE; } // LATER:
static inline BOOL SetMenuItemInfo(HMENU m, UINT i, BOOL byPos, const MENUITEMINFO *mi)
    { (void)m; (void)i; (void)byPos; (void)mi; return FALSE; } // LATER:
static inline int CheckMenuItem(HMENU m, UINT i, UINT f) { (void)m; (void)i; (void)f; return -1; } // LATER:
static inline BOOL EnableMenuItem(HMENU m, UINT i, UINT f) { (void)m; (void)i; (void)f; return FALSE; } // LATER:
static inline int GetMenuItemCount(HMENU m) { (void)m; return 0; } // LATER:
static inline BOOL DeleteMenu(HMENU m, UINT i, UINT f) { (void)m; (void)i; (void)f; return FALSE; } // LATER:
static inline BOOL AppendMenu(HMENU m, UINT f, UINT_PTR id, const char *s) { (void)m; (void)f; (void)id; (void)s; return FALSE; } // LATER:
static inline HMENU GetSubMenu(HMENU m, int i) { (void)m; (void)i; return NULL; } // LATER:

/* Common file dialog stubs */
#define OFN_EXPLORER        0x00080000u
#define OFN_HIDEREADONLY    0x00000004u
#define OFN_PATHMUSTEXIST   0x00000800u
#define OFN_FILEMUSTEXIST   0x00001000u
#define OFN_OVERWRITEPROMPT 0x00000002u
#define OFN_NOREADONLYRETURN 0x00008000u

typedef struct tagOFN {
    DWORD        lStructSize;
    HWND         hwndOwner;
    HINSTANCE    hInstance;
    const char  *lpstrFilter;
    char        *lpstrCustomFilter;
    DWORD        nMaxCustFilter;
    DWORD        nFilterIndex;
    char        *lpstrFile;
    DWORD        nMaxFile;
    char        *lpstrFileTitle;
    DWORD        nMaxFileTitle;
    const char  *lpstrInitialDir;
    const char  *lpstrTitle;
    DWORD        Flags;
    WORD         nFileOffset;
    WORD         nFileExtension;
    const char  *lpstrDefExt;
    LPARAM       lCustData;
    void        *lpfnHook;
    const char  *lpTemplateName;
} OPENFILENAME, *LPOPENFILENAME;

static inline BOOL GetOpenFileName(OPENFILENAME *ofn) { (void)ofn; return FALSE; } // LATER:
static inline BOOL GetSaveFileName(OPENFILENAME *ofn) { (void)ofn; return FALSE; } // LATER:

/* GDI object handle types */
typedef void *HGDIOBJ;
typedef void *HFONT;
typedef void *HPEN;
typedef void *HBRUSH;
typedef void *HGLOBAL;
typedef void *HLOCAL;

/* GDI object stubs */
static inline HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hobj)
    { (void)hdc; (void)hobj; return NULL; }
static inline BOOL DeleteObject(HGDIOBJ hobj) { free(hobj); return TRUE; }
static inline BOOL DeleteDC(HDC hdc) { (void)hdc; return TRUE; }
static inline HDC CreateCompatibleDC(HDC hdc) { (void)hdc; return (HDC)1; }
static inline HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO *bmi, UINT usage,
    void **ppvBits, HANDLE hSec, DWORD off) {
    (void)hdc; (void)usage; (void)hSec; (void)off;
    if (!ppvBits) return NULL;
    size_t sz = (size_t)bmi->bmiHeader.biWidth
              * (size_t)(bmi->bmiHeader.biHeight < 0 ? -bmi->bmiHeader.biHeight : bmi->bmiHeader.biHeight)
              * (size_t)(bmi->bmiHeader.biBitCount < 8 ? 1 : bmi->bmiHeader.biBitCount / 8);
    *ppvBits = calloc(sz, 1);
    return (HBITMAP)*ppvBits;
}
static inline UINT SetDIBColorTable(HDC hdc, UINT start, UINT count, const RGBQUAD *colors)
    { (void)hdc; (void)start; (void)count; (void)colors; return 0; }
static inline BOOL BitBlt(HDC dst, int x, int y, int w, int h, HDC src, int sx, int sy, DWORD rop)
    { (void)dst; (void)x; (void)y; (void)w; (void)h; (void)src; (void)sx; (void)sy; (void)rop; return TRUE; }
static inline BOOL PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop)
    { (void)hdc; (void)x; (void)y; (void)w; (void)h; (void)rop; return TRUE; }
static inline BOOL StretchBlt(HDC dst, int dx, int dy, int dw, int dh,
    HDC src, int sx, int sy, int sw, int sh, DWORD rop)
    { (void)dst; (void)dx; (void)dy; (void)dw; (void)dh;
      (void)src; (void)sx; (void)sy; (void)sw; (void)sh; (void)rop; return TRUE; }

/* Window geometry stubs */
void linux_get_client_rect(RECT *r);
static inline BOOL GetClientRect(HWND hwnd, RECT *r)
    { (void)hwnd; if (r) linux_get_client_rect(r); return TRUE; }
static inline BOOL GetCursorPos(POINT *pt) { (void)pt; return FALSE; } // LATER:
static inline BOOL ScreenToClient(HWND hwnd, POINT *pt) { (void)hwnd; (void)pt; return FALSE; } // LATER:
static inline BOOL ClientToScreen(HWND hwnd, POINT *pt) { (void)hwnd; (void)pt; return FALSE; } // LATER:
static inline BOOL SetWindowPos(HWND hwnd, HWND ins, int x, int y, int cx, int cy, UINT fl)
    { (void)hwnd; (void)ins; (void)x; (void)y; (void)cx; (void)cy; (void)fl; return TRUE; } // LATER:
/* Window show/cursor stubs */
static inline BOOL ShowWindow(HWND hwnd, int cmd) { (void)hwnd; (void)cmd; return FALSE; } // LATER:
static inline int ShowCursor(BOOL show) { (void)show; return 0; } // LATER:
static inline BOOL ClipCursor(const RECT *r) { (void)r; return TRUE; } // LATER:

/* Message stubs */
static inline LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    { (void)hwnd; (void)msg; (void)wp; (void)lp; return 0; } // LATER:

/* Keyboard state stub */
static inline SHORT GetKeyState(int vk) { return sdl_get_async_key_state(vk); }

void linux_set_window_title(const char *s);
static inline BOOL SetWindowText(HWND hwnd, const char *s) { (void)hwnd; linux_set_window_title(s); return TRUE; }

/* Console stubs */
static inline BOOL AllocConsole(void) { return FALSE; } // LATER:
static inline HWND GetConsoleWindow(void) { return NULL; } // LATER:
static inline BOOL SetConsoleTitle(const char *s) { (void)s; return TRUE; } // LATER:
static inline BOOL FreeConsole(void) { return TRUE; } // LATER:
static inline BOOL FlushConsoleInputBuffer(HANDLE h) { (void)h; return FALSE; } // LATER:

/* CRT compatibility aliases */
#define _stricmp  strcasecmp
#define _strnicmp strncasecmp
#define lstrlen   strlen
#define lstrcpy   strcpy
#define lstrcpyn  strncpy
#define lstrcat   strcat

/* Bit rotate (MSVC intrinsic not available on GCC/Linux) */
#define _rotl(x,n)  (((unsigned)(x) << (n)) | ((unsigned)(x) >> (32-(n))))
#define _rotr(x,n)  (((unsigned)(x) >> (n)) | ((unsigned)(x) << (32-(n))))

#endif /* !_WIN32 */
#endif /* COMPAT_WIN_H */
