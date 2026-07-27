#define MyAppVersion "1.0.0"
#define MyAppPublisher "Jiugang He"
#define MyServiceName "IPAssistantSvc"
#ifndef BuildDir
#define BuildDir "..\build"
#endif

[Setup]
AppId={{8A7D5C7D-0E57-4F6E-B9E0-4A7C010A9F25}
AppName={cm:MyAppName}
AppVersion={#MyAppVersion}
AppVerName={cm:MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\IPAssistant
DefaultGroupName={cm:MyAppName}
UninstallDisplayIcon={app}\IPAssistant.exe
OutputDir=Output
OutputBaseFilename=IPAssistantSetup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin
UsedUserAreasWarning=no
MinVersion=10.0
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
SetupIconFile=..\src\resources\app.ico
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#MyAppVersion}.0
VersionInfoDescription=IP Assistant Installer
VersionInfoProductName=IP Assistant
VersionInfoCompany={#MyAppPublisher}
VersionInfoOriginalFileName=IPAssistantSetup-{#MyAppVersion}.exe
VersionInfoProductVersion={#MyAppVersion}.0
VersionInfoProductTextVersion={#MyAppVersion}.0
VersionInfoTextVersion={#MyAppVersion}.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chansim"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "thai"; MessagesFile: "compiler:Languages\Thai.isl"
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"

[CustomMessages]
english.MyAppName=IP Assistant
english.LaunchApp=Launch IP Assistant
english.InstallService=Installing IP Assistant Service...

chansim.MyAppName=IP助理
chansim.LaunchApp=启动IP助理
chansim.InstallService=正在安装 IP Assistant 服务...

japanese.MyAppName=IP Assistant
japanese.LaunchApp=IP Assistant を起動
japanese.InstallService=IP Assistant サービスをインストールしています...

korean.MyAppName=IP Assistant
korean.LaunchApp=IP Assistant 실행
korean.InstallService=IP Assistant 서비스를 설치하는 중...

german.MyAppName=IP Assistant
german.LaunchApp=IP Assistant starten
german.InstallService=IP Assistant-Dienst wird installiert...

french.MyAppName=IP Assistant
french.LaunchApp=Lancer IP Assistant
french.InstallService=Installation du service IP Assistant...

spanish.MyAppName=IP Assistant
spanish.LaunchApp=Iniciar IP Assistant
spanish.InstallService=Instalando el servicio IP Assistant...

brazilianportuguese.MyAppName=IP Assistant
brazilianportuguese.LaunchApp=Iniciar o IP Assistant
brazilianportuguese.InstallService=Instalando o serviço IP Assistant...

russian.MyAppName=IP Assistant
russian.LaunchApp=Запустить IP Assistant
russian.InstallService=Установка службы IP Assistant...

italian.MyAppName=IP Assistant
italian.LaunchApp=Avvia IP Assistant
italian.InstallService=Installazione del servizio IP Assistant...

turkish.MyAppName=IP Assistant
turkish.LaunchApp=IP Assistant'ı başlat
turkish.InstallService=IP Assistant hizmeti yükleniyor...

polish.MyAppName=IP Assistant
polish.LaunchApp=Uruchom IP Assistant
polish.InstallService=Instalowanie usługi IP Assistant...

thai.MyAppName=IP Assistant
thai.LaunchApp=เปิด IP Assistant
thai.InstallService=กำลังติดตั้งบริการ IP Assistant...

arabic.MyAppName=IP Assistant
arabic.LaunchApp=تشغيل IP Assistant
arabic.InstallService=جارٍ تثبيت خدمة IP Assistant...

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Dirs]
Name: "{app}\data"

[Files]
Source: "{#BuildDir}\IPAssistant.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\IPAssistantService.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\iconengines\*"; DestDir: "{app}\iconengines"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
Source: "{#BuildDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
Source: "{#BuildDir}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
Source: "{#BuildDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "{#BuildDir}\generic\*"; DestDir: "{app}\generic"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
Source: "{#BuildDir}\styles\*"; DestDir: "{app}\styles"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
Source: "{#BuildDir}\tls\*"; DestDir: "{app}\tls"; Flags: recursesubdirs createallsubdirs ignoreversion skipifsourcedoesntexist
Source: "{#BuildDir}\translations\qt_*.qm"; DestDir: "{app}\translations"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{cm:MyAppName}"; Filename: "{app}\IPAssistant.exe"
Name: "{userdesktop}\{cm:MyAppName}"; Filename: "{app}\IPAssistant.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\IPAssistantService.exe"; Parameters: "--install"; Flags: runhidden waituntilterminated; StatusMsg: "{cm:InstallService}"
Filename: "{app}\IPAssistant.exe"; Description: "{cm:LaunchApp}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{cmd}"; Parameters: "/C taskkill /IM IPAssistant.exe /F >nul 2>nul & exit /B 0"; Flags: runhidden waituntilterminated; RunOnceId: "StopIPAssistant"
Filename: "{app}\IPAssistantService.exe"; Parameters: "--stop"; Flags: runhidden waituntilterminated; RunOnceId: "StopIPAssistantSvc"
Filename: "{app}\IPAssistantService.exe"; Parameters: "--uninstall"; Flags: runhidden waituntilterminated; RunOnceId: "UninstallIPAssistantSvc"

[Code]
procedure StopExistingService;
var
  ResultCode: Integer;
begin
  Exec(
    ExpandConstant('{sys}\sc.exe'),
    'stop {#MyServiceName}',
    '',
    SW_HIDE,
    ewWaitUntilTerminated,
    ResultCode);

  if ResultCode = 0 then
  begin
    { The service allows up to three seconds for its pipe thread to exit. }
    Sleep(3500);
  end;

  Exec(
    ExpandConstant('{sys}\sc.exe'),
    'delete {#MyServiceName}',
    '',
    SW_HIDE,
    ewWaitUntilTerminated,
    ResultCode);
  if ResultCode = 0 then
    Sleep(500);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then
    StopExistingService;
end;
