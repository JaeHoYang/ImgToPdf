; ============================================================
;  ImgToPdf — Inno Setup 설치 스크립트
;  빌드: Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;  사용법: Inno Setup Compiler 에서 이 파일을 열고 Build > Compile
; ============================================================

#define AppName      "ImgToPdf"
#define AppVersion   "1.0"
#define AppPublisher "jaeho"
#define AppEmail     "jaeho9697@gmail.com"
#define AppExeName   "ImgToPdf.exe"
#define SourceDir    "..\x64\Release"

[Setup]
AppId                    = {{A3F7B2C1-4D5E-4F8A-9B2C-1D3E5F7A9B0C}
AppName                  = {#AppName}
AppVersion               = {#AppVersion}
AppPublisher             = {#AppPublisher}
AppPublisherURL          = ""
AppSupportURL            = "mailto:{#AppEmail}"
AppContact               = {#AppEmail}
DefaultDirName           = {autopf}\{#AppName}
DefaultGroupName         = {#AppName}
AllowNoIcons             = yes
OutputDir                = .
OutputBaseFilename       = ImgToPdf_Setup_v{#AppVersion}
SetupIconFile            = ..\ImgToPdf.ico
Compression              = lzma2/ultra64
SolidCompression         = yes
WizardStyle              = modern
PrivilegesRequired       = lowest
PrivilegesRequiredOverridesAllowed = dialog
ArchitecturesInstallIn64BitMode = x64compatible
MinVersion               = 10.0

; 설치 후 실행 여부 선택 체크박스
UninstallDisplayIcon     = {app}\{#AppExeName}
UninstallDisplayName     = {#AppName} {#AppVersion}

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Tasks]
; 시작 메뉴 등록 (기본 체크)
Name: "startmenu";  Description: "시작 메뉴에 바로가기 추가";  GroupDescription: "바로가기:"; Flags: checkedonce
; 바탕화면 바로가기 (기본 체크)
Name: "desktopicon"; Description: "바탕화면에 바로가기 추가"; GroupDescription: "바로가기:"; Flags: checkedonce

[Files]
; 메인 실행 파일
Source: "{#SourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; ── Visual C++ 2022 런타임 DLL (MFC Dynamic 빌드 의존성) ──────
; 아래 경로는 VS2022 기본 설치 경로. 다를 경우 수정 필요.
; x64 Release 폴더에 직접 복사해 두는 방법도 가능.
Source: "{#SourceDir}\mfc142u.dll";    DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\msvcp140.dll";   DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\vcruntime140.dll";   DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
; 시작 메뉴
Name: "{group}\{#AppName}";        Filename: "{app}\{#AppExeName}"; Tasks: startmenu
Name: "{group}\{#AppName} 제거";   Filename: "{uninstallexe}";      Tasks: startmenu

; 바탕화면
Name: "{autodesktop}\{#AppName}";  Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
; 설치 완료 후 실행 여부 선택
Filename: "{app}\{#AppExeName}"; \
    Description: "{cm:LaunchProgram,{#AppName}}"; \
    Flags: nowait postinstall skipifsilent

[UninstallDelete]
; 제거 시 앱 폴더까지 삭제
Type: filesandordirs; Name: "{app}"
