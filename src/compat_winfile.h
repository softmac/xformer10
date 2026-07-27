/* compat_winfile.h - Win32 file I/O shims for non-Windows builds */
#ifndef COMPAT_WINFILE_H
#define COMPAT_WINFILE_H

#ifndef _WIN32

#include "compat_win.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* File access constants */
#define GENERIC_READ          0x00000001u
#define GENERIC_WRITE         0x00000002u
#define FILE_SHARE_READ       0x00000001u
#define FILE_SHARE_WRITE      0x00000002u

/* File disposition constants */
#define CREATE_NEW            1
#define CREATE_ALWAYS         2
#define OPEN_EXISTING         3
#define OPEN_ALWAYS           4
#define TRUNCATE_EXISTING     5

/* File attribute/flag constants */
#define FILE_ATTRIBUTE_READONLY   0x00000001u
#define FILE_ATTRIBUTE_HIDDEN     0x00000002u
#define FILE_ATTRIBUTE_SYSTEM     0x00000004u
#define FILE_ATTRIBUTE_DIRECTORY  0x00000010u
#define FILE_ATTRIBUTE_ARCHIVE    0x00000020u
#define FILE_ATTRIBUTE_NORMAL     0x00000080u
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000u
#define FILE_FLAG_RANDOM_ACCESS   0x10000000u

/* Seek method constants */
#define FILE_BEGIN            0
#define FILE_CURRENT          1
#define FILE_END              2

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE  ((HANDLE)(uintptr_t)-1)
#endif

/* CreateFileA: maps disposition+access to open(2) flags */
static inline HANDLE CreateFileA(const char *path, DWORD access, DWORD share,
                                  void *sa, DWORD disp, DWORD flags, HANDLE tmpl)
{
    (void)share; (void)sa; (void)flags; (void)tmpl;
    int oflags;
    if ((access & GENERIC_READ) && (access & GENERIC_WRITE))
        oflags = O_RDWR;
    else if (access & GENERIC_WRITE)
        oflags = O_WRONLY;
    else
        oflags = O_RDONLY;
    if (disp == CREATE_ALWAYS || disp == TRUNCATE_EXISTING)
        oflags |= O_CREAT | O_TRUNC;
    else if (disp == CREATE_NEW)
        oflags |= O_CREAT | O_EXCL;
    else if (disp == OPEN_ALWAYS)
        oflags |= O_CREAT;
    int fd = open(path, oflags, 0644);
    if (fd < 0) return INVALID_HANDLE_VALUE;
    return (HANDLE)(intptr_t)fd;
}

/* ReadFile */
static inline BOOL ReadFile(HANDLE h, void *buf, DWORD n, DWORD *bytesRead,
                             void *overlapped)
{
    (void)overlapped;
    ssize_t r = read((int)(intptr_t)h, buf, n);
    if (r < 0) { if (bytesRead) *bytesRead = 0; return FALSE; }
    if (bytesRead) *bytesRead = (DWORD)r;
    return TRUE;
}

/* WriteFile */
static inline BOOL WriteFile(HANDLE h, const void *buf, DWORD n,
                              DWORD *bytesWritten, void *overlapped)
{
    (void)overlapped;
    ssize_t w = write((int)(intptr_t)h, buf, n);
    if (w < 0) { if (bytesWritten) *bytesWritten = 0; return FALSE; }
    if (bytesWritten) *bytesWritten = (DWORD)w;
    return TRUE;
}

/* CloseHandle — suppressed when compat_win.h provides the pthread-unified version */
#ifndef LINUX_LH_CLOSEHANDLE
static inline BOOL CloseHandle(HANDLE h)
{
    if (h == INVALID_HANDLE_VALUE) return FALSE;
    return close((int)(intptr_t)h) == 0 ? TRUE : FALSE;
}
#endif

/* SetFilePointer: combines low 32-bit dist with optional high 32-bit *high */
static inline DWORD SetFilePointer(HANDLE h, LONG dist, LONG *high, DWORD method)
{
    int whence = (method == FILE_BEGIN)   ? SEEK_SET :
                 (method == FILE_END)     ? SEEK_END  : SEEK_CUR;
    off_t off;
    if (high)
        off = (off_t)((int64_t)((uint64_t)(uint32_t)dist | ((uint64_t)(uint32_t)*high << 32)));
    else
        off = (off_t)(int32_t)dist;
    off_t result = lseek((int)(intptr_t)h, off, whence);
    if (result == (off_t)-1) return (DWORD)-1;
    if (high) *high = (LONG)((uint64_t)result >> 32);
    return (DWORD)(result & 0xFFFFFFFFu);
}

/* GetFileSize */
static inline DWORD GetFileSize(HANDLE h, DWORD *high)
{
    struct stat st;
    if (fstat((int)(intptr_t)h, &st) < 0) return (DWORD)-1;
    if (high) *high = (DWORD)((uint64_t)st.st_size >> 32);
    return (DWORD)((uint64_t)st.st_size & 0xFFFFFFFFu);
}

/* WIN32_FIND_DATA — full struct matching Windows layout */
typedef struct _WIN32_FIND_DATAA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
    DWORD    dwReserved0;
    DWORD    dwReserved1;
    CHAR     cFileName[MAX_PATH];
    CHAR     cAlternateFileName[14];
} WIN32_FIND_DATAA;
#ifndef WIN32_FIND_DATA
#define WIN32_FIND_DATA WIN32_FIND_DATAA
#endif

/* FindFirstFileA/FindNextFileA — stubbed; no directory enumeration in Phase 1 */
static inline HANDLE FindFirstFileA(const char *path, WIN32_FIND_DATAA *fd)
{
    (void)path; (void)fd;
    return INVALID_HANDLE_VALUE; // LATER: directory enumeration not implemented
}
static inline BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA *fd)
{
    (void)h; (void)fd;
    return FALSE; // LATER:
}
static inline BOOL FindClose(HANDLE h)
{
    (void)h;
    return TRUE; // LATER:
}

#define FindFirstFile  FindFirstFileA
#define FindNextFile   FindNextFileA
#define CreateFile     CreateFileA

/* SetEndOfFile: truncate at current position */
static inline BOOL SetEndOfFile(HANDLE h)
{
    off_t pos = lseek((int)(intptr_t)h, 0, SEEK_CUR);
    if (pos == (off_t)-1) return FALSE;
    return ftruncate((int)(intptr_t)h, pos) == 0 ? TRUE : FALSE;
}

/* GetFileAttributes: map to stat(); return INVALID_FILE_ATTRIBUTES on error */
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
static inline DWORD GetFileAttributes(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0) return INVALID_FILE_ATTRIBUTES;
    DWORD attr = FILE_ATTRIBUTE_ARCHIVE;
    if (S_ISDIR(st.st_mode)) attr |= FILE_ATTRIBUTE_DIRECTORY;
    if (!(st.st_mode & S_IWUSR)) attr |= FILE_ATTRIBUTE_READONLY;
    return attr;
}

/* SetFileAttributes: no-op on Linux */
static inline BOOL SetFileAttributes(const char *path, DWORD attr) { (void)path; (void)attr; return TRUE; } // LATER:

/* DeleteFile: map to unlink */
static inline BOOL DeleteFile(const char *path) { return unlink(path) == 0 ? TRUE : FALSE; }

#endif /* !_WIN32 */
#endif /* COMPAT_WINFILE_H */
