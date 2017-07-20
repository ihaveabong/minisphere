; miniSphere installer for Windows releases
; (C) 2015-2017 Fat Cerberus
; Inno Setup 5.5 or later with ISPP is required.
;
; this will build a Windows installer for miniSphere.  before compiling the installer,
; you must first build the complete miniSphere GDK for both x86 and x64 using Visual Studio.
; in practical terms, this means building the full GDK and Redist solution configurations.

#define AppName "miniSphere"
#define AppPublisher "Fat Cerberus"
#define AppVersion3 "4.7.0"

; to create a bundle with Sphere Studio, copy the Sphere Studio binaries
; into msw/ide/ before building the installer.
#ifexist "../msw/ide/Sphere Studio.exe"
#define HAVE_SPHERE_STUDIO
#endif

[Setup]
OutputBaseFilename=miniSphereSetup-{#AppVersion3}
OutputDir=.
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
AppName={#AppName}
AppVerName={#AppName} {#AppVersion3}
AppVersion={#AppVersion3}
AppPublisher=Fat Cerberus
AppUpdatesURL=http://forums.spheredev.org/index.php/topic,1215.0.html
AppCopyright=� 2015-2016 Fat Cerberus
AlwaysShowDirOnReadyPage=yes
ArchitecturesInstallIn64BitMode=x64
ChangesAssociations=yes
ChangesEnvironment=yes
Compression=lzma
DefaultDirName={pf}\{#AppName}
DefaultGroupName=miniSphere GDK
DisableProgramGroupPage=yes
DisableWelcomePage=no
LicenseFile=../LICENSE.txt
SetupIconFile=..\msvs\spherical.ico
SolidCompression=yes
UninstallDisplayName={#AppName} {#AppVersion3}
UninstallDisplayIcon={app}\minisphere.exe,0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "normal"; Description: "Standard Redistributable (engine only)"
Name: "developer"; Description: "Spherical Game Development Kit (miniSphere Console)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "engine"; Description: "{#AppName} {#AppVersion3} JavaScript Game Engine [Required]"; Types: normal developer custom; Flags: fixed
Name: "spherun"; Description: "Spherical GDK / miniSphere Console"; Types: developer; Flags: checkablealone
#ifdef HAVE_SPHERE_STUDIO
Name: "spherun/ide"; Description: "Sphere Studio - the Sphere development environment"; Types: developer
#endif
Name: "spherun/cell"; Description: "Cell - the Sphere packaging compiler"; Types: developer
Name: "spherun/ssj"; Description: "SSj - the Sphere JavaScript debugger"; Types: developer

[Tasks]
Name: "assoc"; Description: "&Associate these file extensions with miniSphere:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/sgm"; Description: ".sgm - Sphere &legacy manifest (game.sgm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/s2gm"; Description: ".s2gm - Sphere game &manifest (game.s2gm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/spk"; Description: ".spk - Sphere &SPK game package"; GroupDescription: "Automatically open Sphere file types:"
#ifdef HAVE_SPHERE_STUDIO
Name: "assoc_ss"; Description: "&Associate these file extensions with Sphere Studio:"; GroupDescription: "Automatically open Sphere file types:"; Components: spherun/ide
Name: "assoc_ss/ssproj"; Description: ".ssproj - Sphere Studio project file"; GroupDescription: "Automatically open Sphere file types:"; Components: spherun/ide
#endif
Name: "path"; Description: "Add the GDK tools to the system %&PATH%"; GroupDescription: "Develop on the command line:"; Components: spherun; Flags: checkedonce unchecked

[Files]
Source: "..\msw\minisphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\msw\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/cell
Source: "..\msw\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/ssj
Source: "..\msw\gdk-cp.bat"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw64\minisphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine; Check: IsWin64
Source: "..\msw64\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun; Check: IsWin64
Source: "..\msw64\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/cell; Check: IsWin64
Source: "..\msw64\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/ssj; Check: IsWin64
Source: "..\msw\documentation\sphere2-core-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun
Source: "..\msw\documentation\sphere2-hl-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun
Source: "..\msw\documentation\cellscript-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun/cell
Source: "..\msw\system\*"; DestDir: "{app}\system"; Components: engine; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\msw\template\*"; DestDir: "{app}\template"; Components: engine; Flags: ignoreversion recursesubdirs createallsubdirs
#ifdef HAVE_SPHERE_STUDIO
Source: "..\msw\ide\*"; DestDir: "{app}\ide"; Flags: ignoreversion recursesubdirs; Components: spherun/ide
#endif

[Registry]
; miniSphere associations
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "miniSphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc/spk
Root: HKCR; Subkey: "miniSphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc/spk
Root: HKCR; Subkey: "miniSphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/spk
Root: HKCR; Subkey: "miniSphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "miniSphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/sgm
Root: HKCR; Subkey: "miniSphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/sgm
Root: HKCR; Subkey: "miniSphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "miniSphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/sgm
Root: HKCR; Subkey: ".s2gm"; ValueType: string; ValueName: ""; ValueData: "miniSphere.S2GM"; Flags: uninsdeletevalue; Tasks: assoc/s2gm
Root: HKCR; Subkey: "miniSphere.S2GM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/s2gm
Root: HKCR; Subkey: "miniSphere.S2GM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/s2gm
Root: HKCR; Subkey: "miniSphere.S2GM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/s2gm
; Sphere Studio associations
#ifdef HAVE_SPHERE_STUDIO
Root: HKCR; Subkey: ".ssproj"; ValueType: string; ValueName: ""; ValueData: "miniSphere.SSPROJ"; Flags: uninsdeletevalue; Tasks: assoc_ss/ssproj
Root: HKCR; Subkey: "miniSphere.SSPROJ"; ValueType: string; ValueName: ""; ValueData: "Sphere Studio Project"; Flags: uninsdeletekey; Tasks: assoc_ss/ssproj
Root: HKCR; Subkey: "miniSphere.SSPROJ\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\ide\Sphere Studio.exe,0"; Tasks: assoc_ss/ssproj
Root: HKCR; Subkey: "miniSphere.SSPROJ\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\ide\Sphere Studio.exe"" ""%1"""; Tasks: assoc_ss/ssproj
#endif
; PATH
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app};{olddata}"; Tasks: path

[Icons]
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\minisphere.exe"
#ifdef HAVE_SPHERE_STUDIO
Name: "{group}\Sphere Studio IDE"; Filename: "{app}\ide\Sphere Studio.exe"; Components: spherun/ide
#endif
Name: "{group}\miniSphere Command Prompt"; Filename: "%comspec%"; Parameters: "/k ""{app}\gdk-cp.bat"""; Components: spherun
Name: "{group}\Sphere v2 Core API Reference"; Filename: "{app}\documentation\sphere2-core-api.txt"; Components: spherun
Name: "{group}\Sphere Runtime API Reference"; Filename: "{app}\documentation\sphere2-hl-api.txt"; Components: spherun
Name: "{group}\Cellscript API Reference"; Filename: "{app}\documentation\cellscript-api.txt"; Components: spherun/cell

[Run]
#ifdef HAVE_SPHERE_STUDIO
Filename: "{app}\ide\Sphere Studio.exe"; Description: "Launch Sphere Studio now"; Flags: postinstall nowait skipifsilent
#endif

[Code]
procedure InitializeWizard;
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;
end;
