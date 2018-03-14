# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=gemul8r - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to gemul8r - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "gemul8r - Win32 Release" && "$(CFG)" !=\
 "gemul8r - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "gemul8r.mak" CFG="gemul8r - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gemul8r - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "gemul8r - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "gemul8r - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "gemul8r - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\gemul8r.sys" "$(OUTDIR)\gemul8r.bsc"

CLEAN : 
	-@erase "$(INTDIR)\GEMUL8R.OBJ"
	-@erase "$(INTDIR)\GEMUL8R.res"
	-@erase "$(INTDIR)\GEMUL8R.SBR"
	-@erase "$(OUTDIR)\gemul8r.bsc"
	-@erase "$(OUTDIR)\gemul8r.sys"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G4 /Gz /W3 /Oy /Gf /Gy /D WINVER=0x030A /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D try=__try /D leave=__leave /D except=__except /D finally=__finally /D _CRTAPI1=__cdecl /D _CRTAPI2=__cdecl /D itoa=_itoa /D strcmpi=_strcmpi /D stricmp=_stricmp /D wcsicmp=_wcsicmp /D wcsnicmp=_wcsnicmp /D DBG=0 /D DEVL=1 /D FPO=1 /D "_IDWBUILD" /FR /YX /Zel /Oxs /QI6 /c
CPP_PROJ=/nologo /G4 /Gz /ML /W3 /Oy /Gf /Gy /D WINVER=0x030A /D _X86_=1 /D\
 i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D\
 NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D try=__try /D leave=__leave\
 /D except=__except /D finally=__finally /D _CRTAPI1=__cdecl /D _CRTAPI2=__cdecl\
 /D itoa=_itoa /D strcmpi=_strcmpi /D stricmp=_stricmp /D wcsicmp=_wcsicmp /D\
 wcsnicmp=_wcsnicmp /D DBG=0 /D DEVL=1 /D FPO=1 /D "_IDWBUILD" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/gemul8r.pch" /YX /Fo"$(INTDIR)/" /Zel /Oxs /QI6 /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/GEMUL8R.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/gemul8r.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\GEMUL8R.SBR"

"$(OUTDIR)\gemul8r.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ntoskrnl.lib hal.lib int64.lib /base:0x10000 /version:3.51 /entry:"DriverEntry@8" /pdb:none /machine:I386 /out:"release\gemul8r.sys" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -RELEASE -FULLBUILD -FORCE:MULTIPLE -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -debug:notmapped,MINIMAL -osversion:3.51 -MERGE:.rdata=.text -align:0x20 -subsystem:native,3.51
LINK32_FLAGS=ntoskrnl.lib hal.lib int64.lib /base:0x10000 /version:3.51\
 /entry:"DriverEntry@8" /pdb:none /machine:I386 /out:"$(OUTDIR)/gemul8r.sys"\
 -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -RELEASE\
 -FULLBUILD -FORCE:MULTIPLE -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089\
 -debug:notmapped,MINIMAL -osversion:3.51 -MERGE:.rdata=.text -align:0x20\
 -subsystem:native,3.51 
LINK32_OBJS= \
	"$(INTDIR)\GEMUL8R.OBJ" \
	"$(INTDIR)\GEMUL8R.res"

"$(OUTDIR)\gemul8r.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "gemul8r - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\gemul8r.exe"

CLEAN : 
	-@erase "$(INTDIR)\GEMUL8R.OBJ"
	-@erase "$(INTDIR)\GEMUL8R.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\gemul8r.exe"
	-@erase "$(OUTDIR)\gemul8r.ilk"
	-@erase "$(OUTDIR)\gemul8r.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/gemul8r.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/GEMUL8R.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/gemul8r.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/gemul8r.pdb" /debug /machine:I386 /out:"$(OUTDIR)/gemul8r.exe" 
LINK32_OBJS= \
	"$(INTDIR)\GEMUL8R.OBJ" \
	"$(INTDIR)\GEMUL8R.res"

"$(OUTDIR)\gemul8r.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "gemul8r - Win32 Release"
# Name "gemul8r - Win32 Debug"

!IF  "$(CFG)" == "gemul8r - Win32 Release"

!ELSEIF  "$(CFG)" == "gemul8r - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\GEMUL8R.RC
NODEP_RSC_GEMUL=\
	".\common.ver"\
	

"$(INTDIR)\GEMUL8R.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\GEMUL8R.C
DEP_CPP_GEMUL8=\
	".\gemioctl.h"\
	".\GEMUL8R.H"\
	"c:\ddk\inc\alpharef.h"\
	{$(INCLUDE)}"\bugcodes.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\exlevels.h"\
	{$(INCLUDE)}"\ntddk.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntiologc.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	

!IF  "$(CFG)" == "gemul8r - Win32 Release"


"$(INTDIR)\GEMUL8R.OBJ" : $(SOURCE) $(DEP_CPP_GEMUL8) "$(INTDIR)"

"$(INTDIR)\GEMUL8R.SBR" : $(SOURCE) $(DEP_CPP_GEMUL8) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gemul8r - Win32 Debug"


"$(INTDIR)\GEMUL8R.OBJ" : $(SOURCE) $(DEP_CPP_GEMUL8) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
