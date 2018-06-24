[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{D5059A46-541F-4E8C-8110-63F3D42E5F8F}}
AppName=XF10
AppVersion=10.0
AppPublisher=Emulators Inc.
AppPublisherURL=http://www.emulators.com
AppSupportURL=http://www.emulators.com
AppUpdatesURL=http://www.emulators.com
ChangesAssociations=yes
DefaultDirName={pf}\XF10
DefaultGroupName=XF10
DisableProgramGroupPage=yes
OutputBaseFilename=setup
PrivilegesRequired=admin
SetupIconFile=src\res\gemul8r.ico
UninstallDisplayIcon={app}\XF10.exe
Compression=lzma2 
SolidCompression=yes
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Dirs]
;or non-admins who try and change options or data files will end up making secret local copies that never get deleted and crash 
;when a new version is released
Name: "{commonappdata}"; 
Name: "{commonappdata}\XF10"; Permissions: everyone-full

[Files]
Source: "Release\Gemul8r.exe"; DestDir: "{app}"; DestName: "XF10.exe"; Flags: ignoreversion

[InstallDelete]

[UninstallDelete]
Type: files; Name: "{commonappdata}\XF10\*.gem"

[Icons]
Name: "{group}\XF10"; Filename: "{app}\XF10.exe"
Name: "{commondesktop}\XF10"; Filename: "{app}\XF10.exe"; Tasks: "desktopicon"
Name: "{commonstartmenu}\XF10"; Filename: "{app}\XF10.exe"

[Registry]
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\XF10.exe"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\XF10.exe"; ValueType: string; ValueData: "{app}\XF10.exe"

Root: HKCR; Subkey: ".xfd"; ValueType: string; ValueName: ""; ValueData: "XFDiskImage"; Flags: uninsdeletevalue; Tasks: associateXFD; 
Root: HKCR; Subkey: "XFDiskImage"; ValueType: string; ValueName: ""; ValueData: "XF Disk Image"; Flags: uninsdeletekey; Tasks: associateXFD; 
Root: HKCR; Subkey: "XFDiskImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\XF10.EXE,0"; Tasks: associateXFD; 
Root: HKCR; Subkey: "XFDiskImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\XF10.EXE"" ""%1"""; Tasks: associateXFD; 

[Tasks]
Name: associateXFD; Description: "&Associate XFD files"; GroupDescription: "Other tasks:"; Flags: unchecked

[Run]
Filename: "{app}\XF10.exe"; Description: "{cm:LaunchProgram,XF10}"; Flags: nowait postinstall skipifsilent
