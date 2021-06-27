; NSIS installer for Xformer 10
; by Steven Noonan

Unicode true
SetCompressor /solid /final zlib

!include "MUI2.nsh"
!include "Sections.nsh"

Name "Xformer 10"
OutFile "xf10-setup.exe"

InstallDir "$PROGRAMFILES\XF10"
InstallDirRegKey HKLM "Software\XF10" ""
RequestExecutionLevel admin

Var StartMenuFolder

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\XF10" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Xformer 10" SecXF10
  ; Required section
  SectionIn RO
  
  SetOutPath "$INSTDIR"
  File /oname=XF10.exe "Release\Gemul8r.exe"

  SetShellVarContext all
  CreateDirectory $APPDATA\XF10
  ; Requires: https://nsis.sourceforge.io/AccessControl_plug-in
  AccessControl::GrantOnFile "$APPDATA\XF10" "(BU)" "FullAccess"

  WriteRegStr HKLM "Software\Microsoft\Windows\Current Version\App Paths\XF10.exe" "" "$INSTDIR\XF10.exe"

  ;Store installation folder
  WriteRegStr HKLM "Software\XF10" "" "$INSTDIR"

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
	CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Xformer 10.lnk" "$INSTDIR\XF10.exe"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section /o "Associate XFD Files" SecXFD
	WriteRegStr HKCR ".xfd" "" "XFDiskImage"
	WriteRegStr HKCR "XFDiskImage" "" "XF Disk Image"
	WriteRegStr HKCR "XFDiskImage\DefaultIcon" "" "$INSTDIR\XF10.exe,0"
	WriteRegStr HKCR "XFDiskImage\shell\open\command" "" "$\"$INSTDIR\XF10.exe$\" $\"%1$\"" 
SectionEnd

  LangString DESC_SecXF10 ${LANG_ENGLISH} "Xformer 10 main program"
  LangString DESC_SecXFD ${LANG_ENGLISH} "Associate .xfd files with Xformer 10"

  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecXF10} $(DESC_SecXF10)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecXFD} $(DESC_SecXFD)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
  RMDir /r "$INSTDIR"
  SetShellVarContext all
  Delete "$APPDATA\XF10\*.gem"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    
  Delete "$SMPROGRAMS\$StartMenuFolder\Xformer 10.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  
  DeleteRegKey /ifempty HKCU "Software\XF10"
  DeleteRegKey HKLM "Software\Microsoft\Windows\Current Version\App Paths\XF10.exe"
SectionEnd
