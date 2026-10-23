#ifndef BuildInputs
  #error Build with tools/package_setup.py to verify and snapshot the payload
#endif
#ifndef PackageId
  #error PackageId must be the verified SHA256 manifest digest
#endif
#ifndef InstallId
  #define InstallId "HustMCL.LocalTrial"
#endif
#ifndef AppDisplayName
  #define AppDisplayName "HustMCL Local Trial"
#endif
#ifndef InstallDirName
  #define InstallDirName "HustMCL-Trial"
#endif
#ifndef ShortcutName
  #define ShortcutName "HustMCL Trial"
#endif
#ifndef ShortcutRoot
  #define DesktopRoot "{userdesktop}"
  #define ProgramsRoot "{userprograms}"
#else
  #define DesktopRoot ShortcutRoot + "\desktop"
  #define ProgramsRoot ShortcutRoot + "\programs"
#endif
[Setup]
AppId={#InstallId}
AppName={#AppDisplayName}
AppVersion=0.1.0
DefaultDirName={localappdata}\Programs\{#InstallDirName}
PrivilegesRequired=lowest
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableWelcomePage=yes
DisableReadyPage=yes
DisableFinishedPage=yes
OutputDir={#BuildInputs}
OutputBaseFilename=HustMCL-Setup-Windows-x64
Compression=lzma2/fast
SolidCompression=yes
SetupIconFile=..\resources\brand\hustmcl.ico
UninstallDisplayIcon={app}\hustmcl_desktop.exe
CloseApplications=no
RestartApplications=no
SetupMutex={#InstallId}.Setup
[Files]
Source: "{#BuildInputs}\payload.sha256"; Flags: dontcopy
#include BuildInputs + "\files.iss"
Source: "{#BuildInputs}\payload.sha256"; DestDir: "{app}"; DestName: ".hustmcl-package"; Flags: ignoreversion
[Icons]
Name: "{#DesktopRoot}\{#ShortcutName}"; Filename: "{app}\hustmcl_desktop.exe"; WorkingDir: "{app}"
Name: "{#ProgramsRoot}\{#ShortcutName}"; Filename: "{app}\hustmcl_desktop.exe"; WorkingDir: "{app}"
[Run]
Filename: "{app}\hustmcl_desktop.exe"; Flags: nowait skipifsilent
[Code]
var
  ReusePayload: Boolean;

function GetFileAttributesW(Name: String): LongWord;
  external 'GetFileAttributesW@kernel32.dll stdcall';

function SafePath(Path: String): Boolean;
var
  Attributes: LongWord;
  Parent: String;
begin
  Result := False;
  while Length(Path) > 3 do begin
    Attributes := GetFileAttributesW(Path);
    if (Attributes <> $FFFFFFFF) and ((Attributes and $400) <> 0) then exit;
    Parent := ExtractFileDir(Path);
    if Parent = Path then exit;
    Path := Parent;
  end;
  Result := True;
end;

function NeedsPayload(): Boolean;
begin
  Result := not ReusePayload;
end;

function ShortcutAvailable(Path: String): Boolean;
var
  Shell, Link: Variant;
begin
  Result := False;
  if not SafePath(Path) or DirExists(Path) then exit;
  if not FileExists(Path) then begin
    Result := True;
    exit;
  end;
  try
    Shell := CreateOleObject('WScript.Shell');
    Link := Shell.CreateShortcut(Path);
    Result := CompareText(Link.TargetPath, ExpandConstant('{app}\hustmcl_desktop.exe')) = 0;
  except
    Result := False;
  end;
end;

function VerifiedInstallation(): Boolean;
var
  Lines: TArrayOfString;
  I: Integer;
  Path, RegisteredRoot: String;
begin
  Result := False;
  if not SafePath(ExpandConstant('{app}\.hustmcl-package')) then exit;
  if FileExists(ExpandConstant('{app}\.hustmcl-package')) then begin
    if GetSHA256OfFile(ExpandConstant('{app}\.hustmcl-package')) <> '{#PackageId}' then exit;
  end else begin
    { Adopt the first installer only when registration and every payload byte match. }
    if not RegQueryStringValue(HKCU64,
        'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#InstallId}_is1',
        'Inno Setup: App Path', RegisteredRoot) then exit;
    if CompareText(RegisteredRoot, ExpandConstant('{app}')) <> 0 then exit;
    if not FileExists(ExpandConstant('{app}\unins000.exe')) or
       not FileExists(ExpandConstant('{app}\unins000.dat')) then exit;
    Log('Checking registered legacy installation before adding manifest marker.');
  end;
  ExtractTemporaryFile('payload.sha256');
  if not LoadStringsFromFile(ExpandConstant('{tmp}\payload.sha256'), Lines) then exit;
  for I := 0 to GetArrayLength(Lines) - 1 do begin
    Path := ExpandConstant('{app}\') + Copy(Lines[I], 66, Length(Lines[I]));
    if not SafePath(Path) or not FileExists(Path) then exit;
    if GetSHA256OfFile(Path) <> Copy(Lines[I], 1, 64) then exit;
  end;
  Result := GetArrayLength(Lines) > 0;
end;

function InitializeSetup(): Boolean;
begin
  Result := IsX64OS;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  ReusePayload := False;
  if not SafePath(ExpandConstant('{app}')) or FileExists(ExpandConstant('{app}')) then begin
    Result := 'Installation directory already exists. Open the installed HustMCL shortcut. This build does not support upgrades.';
    exit;
  end;
  if DirExists(ExpandConstant('{app}')) then begin
    try
      ReusePayload := VerifiedInstallation();
    except
      ReusePayload := False;
    end;
    if not ReusePayload then begin
      Result := 'Installation directory already exists. Open the installed HustMCL shortcut. This build does not support upgrades.';
      Log('Existing installation is unrecognized, different or damaged; refusing changes.');
      exit;
    end;
    Log('Verified existing payload; skipping payload writes.');
  end;
  if not ShortcutAvailable(ExpandConstant('{#DesktopRoot}\{#ShortcutName}.lnk')) or
     not ShortcutAvailable(ExpandConstant('{#ProgramsRoot}\{#ShortcutName}.lnk')) then
    Result := '{#ShortcutName} shortcut already exists. Existing shortcuts are preserved.';
end;
