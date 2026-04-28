; ============================================================
;  ImgToPdf — Inno Setup 설치 스크립트
;  빌드: Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;  사용법: Inno Setup Compiler 에서 이 파일을 열고 Build > Compile
; ============================================================

#define AppName      "ImgToPdf"
#define AppVersion   "2.3"
#define AppPublisher "jaeho"
#define AppEmail     "jaeho9697@gmail.com"
#define AppExeName   "ImgToPdf.exe"
#define SourceDir    "..\x64\Release"
#define RegBase      "Software\jaeho\ImgToPdf\Settings"

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

UninstallDisplayIcon     = {app}\{#AppExeName}
UninstallDisplayName     = {#AppName} {#AppVersion}

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
; ── 바로가기 ──────────────────────────────────────────────────
Name: "startmenu";   Description: "{cm:StartMenuEntry}";   GroupDescription: "{cm:ShortcutGroup}"; Flags: checkedonce
Name: "desktopicon"; Description: "{cm:DesktopEntry}";     GroupDescription: "{cm:ShortcutGroup}"; Flags: checkedonce

; ── 앱 언어 선택 (설치 후 앱이 사용할 언어) ──────────────────
Name: "applang_ko"; Description: "{cm:LangKorean}"; GroupDescription: "{cm:LangGroup}"; Flags: exclusive checkedonce
Name: "applang_en"; Description: "{cm:LangEnglish}"; GroupDescription: "{cm:LangGroup}"; Flags: exclusive

[Files]
Source: "{#SourceDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; ── Visual C++ 2022 런타임 DLL ────────────────────────────────
Source: "{#SourceDir}\mfc142u.dll";         DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\msvcp140.dll";        DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\vcruntime140.dll";    DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\vcruntime140_1.dll";  DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Registry]
; 선택한 앱 언어를 레지스트리에 기록 (0=한국어, 1=English)
Root: HKCU; Subkey: "{#RegBase}"; ValueType: dword; ValueName: "Lang"; ValueData: "0"; Tasks: applang_ko; Flags: createvalueifdoesntexist uninsdeletevalue
Root: HKCU; Subkey: "{#RegBase}"; ValueType: dword; ValueName: "Lang"; ValueData: "1"; Tasks: applang_en; Flags: createvalueifdoesntexist uninsdeletevalue

[Icons]
Name: "{group}\{#AppName}";       Filename: "{app}\{#AppExeName}"; Tasks: startmenu
Name: "{group}\{#AppName} Uninstall"; Filename: "{uninstallexe}"; Tasks: startmenu
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "{cm:LaunchProgram,{#AppName}}"; \
    Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
// ── 이전 버전 제거 헬퍼 ──────────────────────────────────────────
function GetUninstallString(): String;
var
  subKey, s: String;
begin
  subKey := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\'
          + '{A3F7B2C1-4D5E-4F8A-9B2C-1D3E5F7A9B0C}_is1';
  s := '';
  if not RegQueryStringValue(HKCU, subKey, 'UninstallString', s) then
    RegQueryStringValue(HKLM, subKey, 'UninstallString', s);
  Result := s;
end;

// ── Ollama 헬퍼 ──────────────────────────────────────────────────
function FindOllamaExe: String;
var
  candidate: String;
begin
  Result := '';
  candidate := GetEnv('LOCALAPPDATA') + '\Programs\Ollama\ollama.exe';
  if FileExists(candidate) then begin Result := candidate; Exit; end;
  candidate := ExpandConstant('{commonpf64}') + '\Ollama\ollama.exe';
  if FileExists(candidate) then begin Result := candidate; Exit; end;
  candidate := ExpandConstant('{commonpf}') + '\Ollama\ollama.exe';
  if FileExists(candidate) then begin Result := candidate; Exit; end;
end;

function IsLlamaModelInstalled: Boolean;
var
  manifestPath: String;
begin
  // 기본 경로: %USERPROFILE%\.ollama\models\manifests\...
  manifestPath := GetEnv('USERPROFILE') +
    '\.ollama\models\manifests\registry.ollama.ai\library\llama3.2\3b';
  Result := FileExists(manifestPath);
end;

procedure TryPullOllamaModel(OllamaExe: String);
var
  resultCode: Integer;
begin
  if MsgBox(CustomMessage('OllamaDownloadPrompt'), mbConfirmation, MB_YESNO) = IDYES then
  begin
    // SW_SHOW: 콘솔 창에서 다운로드 진행률 표시
    Exec(OllamaExe, 'pull llama3.2:3b', '', SW_SHOW,
         ewWaitUntilTerminated, resultCode);
    if resultCode = 0 then
      MsgBox(CustomMessage('OllamaDownloadDone'), mbInformation, MB_OK)
    else
      MsgBox(CustomMessage('OllamaDownloadFail'), mbError, MB_OK);
  end
  else
    MsgBox(CustomMessage('OllamaDownloadSkip'), mbInformation, MB_OK);
end;

// ── Ollama 설치 프로그램 다운로드 및 실행 ────────────────────────
function DownloadAndInstallOllama: Boolean;
var
  installerPath, psArgs: String;
  resultCode: Integer;
begin
  Result := False;

  if MsgBox(CustomMessage('OllamaInstallPrompt'), mbConfirmation, MB_YESNO) <> IDYES then
  begin
    MsgBox(CustomMessage('OllamaInstallSkip'), mbInformation, MB_OK);
    Exit;
  end;

  installerPath := ExpandConstant('{tmp}') + '\OllamaSetup.exe';

  // 다운로드 시작 전 소요 시간 안내 및 최종 확인
  if MsgBox(CustomMessage('OllamaInstallConfirm'), mbConfirmation, MB_YESNO) <> IDYES then
  begin
    MsgBox(CustomMessage('OllamaInstallSkip'), mbInformation, MB_OK);
    Exit;
  end;

  // TLS 1.2 강제 + 다운로드 진행 메시지 출력 (콘솔 창 표시)
  psArgs := '-NoProfile -Command "'
    + '[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; '
    + 'Write-Host ''Ollama 설치 프로그램 다운로드 중... 잠시 기다려주세요.''; '
    + 'Invoke-WebRequest -Uri ''https://ollama.com/download/OllamaSetup.exe'''
    + ' -OutFile ''' + installerPath + ''' -UseBasicParsing; '
    + 'Write-Host ''다운로드 완료.''"';

  Exec('powershell.exe', psArgs, '', SW_SHOW, ewWaitUntilTerminated, resultCode);

  if not FileExists(installerPath) then
  begin
    MsgBox(CustomMessage('OllamaInstallDownloadFail'), mbError, MB_OK);
    Exit;
  end;

  // Ollama 설치 실행 (사용자가 완료할 때까지 대기)
  if not Exec(installerPath, '', '', SW_SHOW, ewWaitUntilTerminated, resultCode) then
  begin
    MsgBox(CustomMessage('OllamaInstallRunFail'), mbError, MB_OK);
    Exit;
  end;

  // 설치 확인
  if FindOllamaExe() <> '' then
    Result := True
  else
    MsgBox(CustomMessage('OllamaInstallNotDetected'), mbError, MB_OK);
end;

// ── 설치 단계 처리 ───────────────────────────────────────────────
procedure CurStepChanged(CurStep: TSetupStep);
var
  uninstStr, ollamaExe: String;
  resultCode: Integer;
begin
  // 1. 기존 버전 자동 제거
  if CurStep = ssInstall then
  begin
    uninstStr := GetUninstallString();
    if uninstStr <> '' then
      Exec(RemoveQuotes(uninstStr), '/SILENT /NORESTART /SUPPRESSMSGBOXES',
           '', SW_HIDE, ewWaitUntilTerminated, resultCode);
  end

  // 2. 설치 완료 후 Ollama 확인 → 없으면 설치 → 모델 다운로드
  else if CurStep = ssPostInstall then
  begin
    ollamaExe := FindOllamaExe();

    // Ollama 미설치: 다운로드 & 설치 제안
    if ollamaExe = '' then
    begin
      if DownloadAndInstallOllama() then
        ollamaExe := FindOllamaExe();
    end;

    // Ollama 설치 확인 후 모델 다운로드 제안
    if ollamaExe <> '' then
    begin
      if not IsLlamaModelInstalled() then
        TryPullOllamaModel(ollamaExe);
      // 모델 이미 설치된 경우 아무것도 하지 않음
    end;
  end;
end;

[CustomMessages]
; 한국어 메시지
korean.ShortcutGroup=바로가기:
korean.StartMenuEntry=시작 메뉴에 바로가기 추가
korean.DesktopEntry=바탕화면에 바로가기 추가
korean.LangGroup=앱 언어 / App Language:
korean.LangKorean=한국어 (Korean)
korean.LangEnglish=English
korean.OllamaDownloadPrompt=ImgToPdf의 PDF 요약 기능은 로컬 AI 모델(llama3.2:3b)을 사용합니다.%n지금 모델을 다운로드하시겠습니까?%n%n  ■ 모델  : llama3.2:3b (Meta Llama 3.2)%n  ■ 크기  : 약 2.0 GB (디스크 공간 필요)%n  ■ 소요  : 인터넷 속도에 따라 수 분~수십 분%n  ■ 용도  : PDF 내용을 AI가 읽고 한국어/영어로 요약%n  ■ 개인정보 : 모든 처리가 내 PC에서만 이루어집니다%n%n[예] 지금 다운로드 — 콘솔 창에서 진행률이 표시됩니다.%n[아니오] 나중에 직접 설치 — 아래 명령을 실행하세요:%n  ollama pull llama3.2:3b
korean.OllamaDownloadDone=AI 모델 다운로드가 완료되었습니다.%nPDF 도구 탭에서 [요약] 버튼을 사용할 수 있습니다.
korean.OllamaDownloadFail=모델 다운로드에 실패했습니다.%n나중에 직접 실행하세요:%n  ollama pull llama3.2:3b
korean.OllamaDownloadSkip=나중에 아래 명령으로 직접 설치할 수 있습니다:%n  ollama pull llama3.2:3b
korean.OllamaNotFound=Ollama가 설치되어 있지 않습니다.%nPDF 요약 기능을 사용하려면 Ollama를 먼저 설치하세요:%n  https://ollama.com%n%n설치 후 명령 프롬프트에서 실행:%n  ollama pull llama3.2:3b
korean.OllamaInstallPrompt=Ollama가 설치되어 있지 않습니다.%nPDF 요약 기능을 사용하려면 Ollama가 필요합니다.%n%n  ■ 설치 파일 크기 : 약 100~200 MB%n  ■ 다운로드 후 Ollama 설치 마법사가 실행됩니다%n%n[예] 지금 다운로드하여 설치합니다.%n[아니오] 나중에 직접 설치합니다 (https://ollama.com)
korean.OllamaInstallConfirm=Ollama 설치 파일을 다운로드합니다.%n%n  ■ 파일 크기 : 약 100~200 MB%n  ■ 소요 시간 : 인터넷 속도에 따라 수 분~수십 분%n  ■ 다운로드 중에는 검은 콘솔 창이 열립니다 — 닫지 마세요%n%n계속 진행하시겠습니까?
korean.OllamaInstallSkip=나중에 아래 주소에서 Ollama를 설치하세요:%n  https://ollama.com%n%n설치 후 명령 프롬프트에서 실행:%n  ollama pull llama3.2:3b
korean.OllamaInstallDownloadFail=Ollama 설치 파일 다운로드에 실패했습니다.%n인터넷 연결을 확인하거나 아래 주소에서 직접 다운로드하세요:%n  https://ollama.com
korean.OllamaInstallRunFail=Ollama 설치 프로그램을 실행할 수 없습니다.%n다운로드된 파일을 직접 실행하거나 아래 주소에서 다시 시도하세요:%n  https://ollama.com
korean.OllamaInstallNotDetected=Ollama 설치가 완료되지 않은 것 같습니다.%n설치를 마친 후 아래 명령으로 모델을 수동으로 받으세요:%n  ollama pull llama3.2:3b

; 영어 메시지
english.ShortcutGroup=Shortcuts:
english.StartMenuEntry=Add a shortcut to the Start Menu
english.DesktopEntry=Add a shortcut to the Desktop
english.LangGroup=App Language / 앱 언어:
english.LangKorean=한국어 (Korean)
english.LangEnglish=English
english.OllamaDownloadPrompt=ImgToPdf's PDF Summarization feature uses a local AI model (llama3.2:3b).%nWould you like to download the model now?%n%n  ■ Model   : llama3.2:3b (Meta Llama 3.2)%n  ■ Size    : approx. 2.0 GB (disk space required)%n  ■ Time    : a few to tens of minutes depending on your connection%n  ■ Purpose : AI reads the PDF and summarizes it in Korean or English%n  ■ Privacy : all processing happens entirely on your PC%n%n[Yes] Download now — progress will be shown in a console window.%n[No]  Install later — run the following command yourself:%n  ollama pull llama3.2:3b
english.OllamaDownloadDone=AI model downloaded successfully.%nYou can now use the [Summarize] button in the PDF Tools tab.
english.OllamaDownloadFail=Model download failed.%nPlease run it manually later:%n  ollama pull llama3.2:3b
english.OllamaDownloadSkip=You can install it later by running:%n  ollama pull llama3.2:3b
english.OllamaNotFound=Ollama is not installed.%nTo use PDF summarization, install Ollama first:%n  https://ollama.com%n%nThen run in a command prompt:%n  ollama pull llama3.2:3b
english.OllamaInstallPrompt=Ollama is not installed.%nOllama is required for the PDF Summarization feature.%n%n  ■ Installer size : approx. 100–200 MB%n  ■ After download, the Ollama setup wizard will launch%n%n[Yes] Download and install now.%n[No]  Install later manually (https://ollama.com)
english.OllamaInstallConfirm=The Ollama installer will now be downloaded.%n%n  ■ File size    : approx. 100–200 MB%n  ■ Time needed  : a few to tens of minutes depending on your connection%n  ■ A console window will open during download — do not close it%n%nDo you want to continue?
english.OllamaInstallSkip=You can install Ollama later from:%n  https://ollama.com%n%nThen run in a command prompt:%n  ollama pull llama3.2:3b
english.OllamaInstallDownloadFail=Failed to download the Ollama installer.%nCheck your internet connection or download it manually:%n  https://ollama.com
english.OllamaInstallRunFail=Failed to run the Ollama installer.%nTry running the downloaded file manually or visit:%n  https://ollama.com
english.OllamaInstallNotDetected=Ollama installation may not have completed.%nAfter finishing setup, pull the model manually:%n  ollama pull llama3.2:3b
