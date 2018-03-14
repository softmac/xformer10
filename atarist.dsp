# Microsoft Developer Studio Project File - Name="GEMUL8R" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=GEMUL8R - Win32 (80x86) Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "atarist.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "atarist.mak" CFG="GEMUL8R - Win32 (80x86) Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GEMUL8R - Win32 (80x86) Release" (based on "Win32 (x86) Application")
!MESSAGE "GEMUL8R - Win32 (80x86) Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GEMUL8R - Win32 (80x86) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WinRel"
# PROP BASE Intermediate_Dir ".\WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\build\ship"
# PROP Intermediate_Dir ".\build\ship"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /YX /c
# ADD CPP /nologo /G6 /Gz /W3 /Gm /GX /Zi /O2 /I ".\SRC" /I ".\SRC\COMMON" /I ".\SRC\RES" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN32" /D "HWIN32" /FAcs /FD /GF /Gy /GA /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib mfc30.lib mfco30.lib mfcd30.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib kernel32.lib /map /debug /machine:IX86 /force /largeaddressaware:no /debugtype:cv,fixup -opt:ref -optidata -fixed -swaprun:CD -swaprun:NET
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GEMUL8R - Win32 (80x86) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WinDebug"
# PROP BASE Intermediate_Dir ".\WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\build\debug"
# PROP Intermediate_Dir ".\build\debug"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /Od /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /Zi /Ot /Oi /I ".\SRC" /I ".\SRC\COMMON" /I ".\SRC\RES" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN32" /D "HWIN32" /FAcs /Fr /FD /GF /GZ /Gy /GA /c
# SUBTRACT CPP /Ox /Oa /Ow /Og /Os /YX
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib mfc30d.lib mfco30d.lib mfcd30d.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 version.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib kernel32.lib /incremental:no /map /debug /machine:IX86 /force
# SUBTRACT LINK32 /verbose /profile

!ENDIF 

# Begin Target

# Name "GEMUL8R - Win32 (80x86) Release"
# Name "GEMUL8R - Win32 (80x86) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\Src\Atari16.vm\68k.c
# End Source File
# Begin Source File

SOURCE=.\src\680x0.cpu\68k.h
# End Source File
# Begin Source File

SOURCE=.\src\680x0.cpu\68kcpu.c
# End Source File
# Begin Source File

SOURCE=.\SRC\680x0.CPU\68kdis.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\atari800.c
# End Source File
# Begin Source File

SOURCE=.\Src\Atari16.vm\Blitter.c
# End Source File
# Begin Source File

SOURCE=.\src\ddlib.c
# End Source File
# Begin Source File

SOURCE=.\Src\Atari16.vm\Gemdos.c
# End Source File
# Begin Source File

SOURCE=.\src\gemtypes.h
# End Source File
# Begin Source File

SOURCE=.\Src\Gemul8r.c
# End Source File
# Begin Source File

SOURCE=.\SRC\gemul8r.h
# End Source File
# Begin Source File

SOURCE=.\Src\Res\Gemul8r.ico
# End Source File
# Begin Source File

SOURCE=.\Src\Res\gemul8r.rc
# ADD BASE RSC /l 0x409 /i "Src\Res"
# ADD RSC /l 0x409 /i "Src\Res" /i ".\Src\Res"
# End Source File
# Begin Source File

SOURCE=.\Src\Atari16.vm\Ikbd.c
# End Source File
# Begin Source File

SOURCE=.\src\mac.vm\macdisk.c
# End Source File
# Begin Source File

SOURCE=.\src\mac.vm\machw.c
# End Source File
# Begin Source File

SOURCE=.\src\mac.vm\maclinea.c
# End Source File
# Begin Source File

SOURCE=.\src\mac.vm\macrtc.c
# End Source File
# Begin Source File

SOURCE=.\src\680x0.cpu\mon.c
# End Source File
# Begin Source File

SOURCE=.\src\printer.c
# End Source File
# Begin Source File

SOURCE=.\SRC\PROPS.C
# End Source File
# Begin Source File

SOURCE=.\SRC\RES\RESOURCE.H
# End Source File
# Begin Source File

SOURCE=.\Src\Romcard.c
# End Source File
# Begin Source File

SOURCE=.\src\serial.c
# End Source File
# Begin Source File

SOURCE=.\src\sound.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xatari.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xfcable.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xkey.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xlxeram.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xmon.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xsb.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xsio.c
# End Source File
# Begin Source File

SOURCE=.\src\atari8.vm\xvideo.c
# End Source File
# Begin Source File

SOURCE=.\Src\Atari16.vm\Yamaha.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\src\atari8.vm\atari800.h
# End Source File
# Begin Source File

SOURCE=.\src\ddlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Src\Res\bitmap1.bmp
# End Source File
# End Group
# End Target
# End Project
