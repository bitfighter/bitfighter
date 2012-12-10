# Script grabs version number from game.h
!searchparse /file "../../../zap/version.h" `ZAP_GAME_RELEASE "` versionNumber `"`
!define curVersion "${versionNumber}"  
;--------------------------------
; Include Modern UI

   !include "MUI2.nsh"

;--------------------------------
; General

;Name and file
Name    "Bitfighter"                               ; App name
!ifdef PORTABLE
  OutFile "Bitfighter-Portable-${curVersion}.exe"    ; Name of the installer file to write
!else
  OutFile "Bitfighter-Installer-${curVersion}.exe"   ; Name of the installer file to write
!endif


SetCompressor /SOLID lzma


;Default installation folder
!ifdef PORTABLE
  RequestExecutionLevel user    ; Don't escalate for portable install

!else  
  
  InstallDir "$PROGRAMFILES\Bitfighter"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Bitfighter" ""

  ; Request application privileges for Windows Vista
  RequestExecutionLevel admin


  ;--------------------------------
  ; Overall Interface Settings

  !define MUI_ABORTWARNING
  BrandingText " "

  ;--------------------------------
  ; Welcome Page Settings

  !define MUI_WELCOMEFINISHPAGE_BITMAP WelcomePageBanner.bmp
  !define MUI_WELCOMEPAGE_TEXT "\
     This will install Bitfighter version ${curVersion} on your computer. If you previously installed an older version, this will overwrite it.  There is no need to uninstall."


  ; For moving folders:
  ;--------------------------------
  !include 'FileFunc.nsh'
  !insertmacro Locate

  Var /GLOBAL switch_overwrite
  !include 'MoveFileFolder.nsh'
  ;--------------------------------


  ;--------------------------------
  ; Finish page settings

  ; Create checkbox to run game post installation
  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_TEXT "Run Bitfighter"
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"


  ;--------------------------------
  ; Uninstall page settings

  !define MUI_UNCONFIRMPAGE_TEXT_TOP "\
     This will remove Bitfighter from your system.  It will not remove your data in $APPDATA\Bitfighter.$\n$\n\
     \
     Click Uninstall to continue.$\n $\n"

  ;--------------------------------
  ; Pages

     !insertmacro MUI_PAGE_WELCOME
     !insertmacro MUI_PAGE_LICENSE "..\..\..\End-User License.txt"
     !insertmacro MUI_PAGE_DIRECTORY
     !insertmacro MUI_PAGE_INSTFILES
     !insertmacro MUI_PAGE_FINISH		

     !insertmacro MUI_UNPAGE_CONFIRM
     !insertmacro MUI_UNPAGE_INSTFILES
    
  ;--------------------------------
  ; Languages
   
     !insertmacro MUI_LANGUAGE "English"


  ;--------------------------------
  Function LaunchLink
    ExecShell "" "$INSTDIR\Play Bitfighter.lnk"
  FunctionEnd
!endif



;--------------------------------
Function .onInit
    !ifdef PORTABLE
      SetSilent silent    ; Suppress output for portable install
    !endif
FunctionEnd



;--------------------------------
; Installer Section

Section "Install"

    !ifdef PORTABLE
       ; Get directory from which the exe was called, put it into $0
        System::Call "kernel32::GetCurrentDirectory(i ${NSIS_MAX_STRLEN}, t .r0)"
        StrCpy $INSTDIR  "$0\Bitfighter"
    !endif


  SetOutPath "$INSTDIR"
  File "..\..\..\exe\Bitfighter.exe"
  File "..\..\..\exe\joystick_presets.ini"

  File "..\..\..\lib\OpenAL32.dll"
  File "..\..\..\lib\ALURE32.dll"
  File "..\..\..\lib\libogg.dll"
  File "..\..\..\lib\libspeex.dll"
  File "..\..\..\lib\libvorbis.dll"
  File "..\..\..\lib\libvorbisfile.dll"
  File "..\..\..\lib\SDL2.dll"
  File "..\..\..\lib\zlib1.dll"
  File "..\..\..\lib\libpng14.dll"
  File "..\..\..\lib\libmodplug.dll"

  File "..\..\..\doc\readme.txt"
  File "..\..\..\End-User License.txt"
  File ".\twoplayers.bat"
  File "..\..\..\resource\bficon.bmp"

  SetOutPath "$INSTDIR\updater"
  File /r ".\updater\bfup.exe"
  File /r ".\updater\bfup.xml"
  File /r ".\updater\libcurl.dll"
  File /r ".\updater\License.txt"
  File /r ".\updater\readme.txt"

  SetOutPath "$INSTDIR\sfx"
  File /r "..\..\..\resource\sfx\*.wav"

  SetOutPath "$INSTDIR\music"
  File /r "..\..\..\resource\music\*.*"

  SetOutPath "$INSTDIR\editor_plugins"
  File /r "..\..\..\resource\editor_plugins\*.*"

  SetOutPath "$INSTDIR\scripts"
  File /r "..\..\..\resource\scripts\*.lua"

  SetOutPath "$INSTDIR\levels"
  File /r "..\..\..\resource\levels\*.level"  
  File /r "..\..\..\resource\levels\*.levelgen"

  SetOutPath "$INSTDIR\robots"
  File /r "..\..\..\resource\robots\*.bot" 


  CreateShortCut "$INSTDIR\Play Bitfighter.lnk" "$INSTDIR\Bitfighter.exe"


  !ifdef PORTABLE
    ; Portable installs need this marker file
    FileOpen $0 "$INSTDIR\.standalone" w
    FileClose $0
  !else
      ; Store installation folder
      WriteRegStr HKCU "Software\Bitfighter" "" $INSTDIR
      WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bitfighter" "DisplayName" "Bitfighter (remove only)"
      WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bitfighter" "UninstallString" '"$INSTDIR\uninstall-bitfighter.exe"'

      ; Create uninstaller
      WriteUninstaller "$INSTDIR\uninstall-bitfighter.exe"
      SetOutPath $SMPROGRAMS\Bitfighter
      WriteINIStr "$SMPROGRAMS\Bitfighter\Bitfighter Home Page.url" "InternetShortcut" "URL" "http://www.bitfighter.org/"
      CreateShortCut "$SMPROGRAMS\Bitfighter\Uninstall Bitfighter.lnk" "$INSTDIR\uninstall-bitfighter.exe"
      SetOutPath $INSTDIR
      CreateShortCut "$SMPROGRAMS\Bitfighter\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe"

      call MigrateUserData
       
       MessageBox MB_YESNO|MB_ICONQUESTION "Bitfighter has been installed.  Would you like to add a desktop icon for Bitfighter?" IDNO NoDesktopIcon
          SetOutPath $INSTDIR
          CreateShortCut "$DESKTOP\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe"
       NoDesktopIcon:
         
       SetOutPath $INSTDIR

  !endif

SectionEnd

!ifndef PORTABLE

  ; Migrate old settings folder, if it exists
  ; Put in a function so as to not screw up our progress bar... in NSIS, functions are excluded from progress bars
  Function MigrateUserData
    
    ${If} ${FileExists} `$DOCUMENTS\Bitfighter\*.*`
      ${AndIf} ${FileExists} `$DOCUMENTS\Bitfighter\bitfighter.ini`
      ${AndIf} ${FileExists} `$DOCUMENTS\Bitfighter\robots\*.*`
      ${AndIfNot} ${FileExists} `$APPDATA\Bitfighter\*.*`
          StrCpy $switch_overwrite 0
          !insertmacro MoveFolder "$DOCUMENTS\Bitfighter" "$APPDATA\Bitfighter" "*.*"
    ${EndIf}
  FunctionEnd    

  ;--------------------------------
  ;Uninstaller Section

  Section "Uninstall"

    ; Always delete uninstaller first
    Delete $INSTDIR\uninstall-bitfighter.exe

    ; And purge our install dirs, along with everything in them...
    RMDir /r $INSTDIR\updater
    RMDir /r $INSTDIR\sfx
    RMDir /r $INSTDIR\levels
    RMDir /r $INSTDIR\robots
    RMDir /r $INSTDIR\screenshots
    RMDir /r $INSTDIR\cache
    RMDir /r $INSTDIR\lua
    RMDir /r $INSTDIR\editor_plugins
    RMDir /r $INSTDIR
    
    ; Remove the links from the start menu and desktop
    Delete $SMPROGRAMS\Bitfighter\*.*
    RMDir $SMPROGRAMS\Bitfighter  # Delete folder on Start menu
    Delete "$DESKTOP\Bitfighter.lnk"
     
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Bitfighter"
    DeleteRegKey HKCU "Software\Bitfighter"
     
        IfFileExists $INSTDIR 0 Removed 
        MessageBox MB_OK|MB_ICONEXCLAMATION "Note: I did my best, but I could not remove $INSTDIR."
    Removed: 


  SectionEnd
!endif