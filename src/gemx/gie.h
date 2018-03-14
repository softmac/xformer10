#ifndef __GEI_H__
#define __GEI_H__

// Disk and directory level calls

int  EnumeratePhysicalDrives();
BOOL GetIthPhysicalDrive(int id, char *pch, int cch);
int  EnumerateDirectory(int id, const char *szPath);
BOOL GetIthDirectory(int iFile, char *pch, int cch, BOOL *pfDir);
BOOL GetIthDirectoryInfo(int iFile, long *plSize, FILETIME *pft, char *szType, char *szCreator);
int  AddDiskImage(const char *szPathName);
BOOL ExtractFile(const char *szFile, const char *szDestFile);
BOOL ShowFileProperties(const char *szFile);

// Disk image calls

BOOL IsBlockDevice(int id);
BOOL CopyDiskImage(HWND hwnd, BOOL fWrite, int id, PDI pdi, BOOL fWholeDisk);

// File copy/drag calls

BOOL    AddDragDescriptor(int iFile, HGLOBAL *phFileGroupDescriptor);
HGLOBAL DropFileContents(int iFileGroupDescriptor);
HGLOBAL GetIthFileText(int iFile);

// Progress dialog

BOOL StartProgress(char *szTitle);
BOOL SetProgress(int i, char *sz);
BOOL EndProgress();

#endif // !__GEI_H__
