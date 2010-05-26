# Script grabs version number from game.h
!searchparse /file "../zap/game.h" `ZAP_GAME_RELEASE "` versionNumber `"`
!define curVersion "${versionNumber}"  
;--------------------------------
; Include Modern UI

   !include "MUI2.nsh"

;--------------------------------
; General

;Name and file
Name    "Bitfighter"                      ; App name
OutFile "Bitfighter-Installer-${curVersion}.exe"   ; Name of the installer file to write

;Default installation folder
InstallDir "$PROGRAMFILES\Bitfighter"

;Get installation folder from registry if available
InstallDirRegKey HKCU "Software\Bitfighter" ""

;Request application privileges for Windows Vista
RequestExecutionLevel admin


SetCompressor /SOLID lzma

;--------------------------------
; Overall Interface Settings

!define MUI_ABORTWARNING
BrandingText " "

;--------------------------------
; Welcome Page Settings

!define MUI_WELCOMEFINISHPAGE_BITMAP WelcomePageBanner.bmp
!define MUI_WELCOMEPAGE_TEXT "\
   This will install Bitfighter version ${curVersion} on your computer. If you previously installed an older version, this will overwrite it.  There is no need to uninstall."

;--------------------------------
Function LaunchLink
  ExecShell "" "$INSTDIR\Bitfighter.lnk"
FunctionEnd

;--------------------------------
; Finish page settings

; Create checkbox to run game post installation
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Run Bitfighter"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"


;--------------------------------
; Uninstall page settings

!define MUI_UNCONFIRMPAGE_TEXT_TOP "\
   This will completely remove Bitfighter from your system.  Any custom levels or robots you created will be deleted unless you have backed them up.$\n$\n\
   \
   Click Uninstall to continue."

;--------------------------------
; Pages

   !insertmacro MUI_PAGE_WELCOME
   !insertmacro MUI_PAGE_LICENSE "End-User License.txt"
   !insertmacro MUI_PAGE_DIRECTORY
   !insertmacro MUI_PAGE_INSTFILES
   !insertmacro MUI_PAGE_FINISH

   !insertmacro MUI_UNPAGE_CONFIRM
   !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
; Languages
 
   !insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Installer Section

Section "Install"

  SetOutPath "$INSTDIR"
  File ..\exe\Bitfighter.exe
  File ..\exe\robot_helper_functions.lua
  File ..\exe\levelgen_helper_functions.lua
  File ..\exe\lua_helper_functions.lua
  
  CreateShortCut "$INSTDIR\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe" "-rootdatadir $\"%LOCALAPPDATA%\Bitfighter$\""

  File ".\Windows specific\OpenAL32.dll"     
  File ".\Windows specific\glut32.dll"
  File ".\Windows specific\lua5.1.dll"
  File ".\Windows specific\lua51.dll"
  File "readme.txt"
  File "End-User License.txt"
  File ".\Windows specific\twoplayers.bat"

  SetOutPath "$INSTDIR\sfx"
  File /r ".\sfx\*.wav"

  SetOutPath "$LOCALAPPDATA\Bitfighter\levels"
  File /r ".\levels\*.level"  
  File /r ".\levels\*.levelgen"

  SetOutPath "$LOCALAPPDATA\Bitfighter\robots"
  File /r ".\robots\*.bot" 

  
  SetOutPath "$LOCALAPPDATA\Bitfighter\screenshots"
  File ".\screenshots\readme.txt"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Bitfighter" "" $INSTDIR
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bitfighter" "DisplayName" "Bitfighter (remove only)"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Bitfighter" "UninstallString" '"$INSTDIR\uninstall-bitfighter.exe"'

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall-bitfighter.exe"
  
   MessageBox MB_YESNO|MB_ICONQUESTION "Bitfighter has been installed.  Would you like to add shortcuts in the start menu?"  IDNO NoStartMenu
      SetOutPath $SMPROGRAMS\Bitfighter
      WriteINIStr "$SMPROGRAMS\Bitfighter\Bitfighter Home Page.url" "InternetShortcut" "URL" "http://www.bitfighter.org/"
      CreateShortCut "$SMPROGRAMS\Bitfighter\Uninstall Bitfighter.lnk" "$INSTDIR\uninstall-bitfighter.exe"
      SetOutPath $INSTDIR
      CreateShortCut "$SMPROGRAMS\Bitfighter\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe" "-rootdatadir $\"%LOCALAPPDATA%\Bitfighter$\""
   NoStartMenu:
   
   MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to add a desktop icon for Bitfighter?" IDNO NoDesktopIcon
      SetOutPath $INSTDIR
      CreateShortCut "$DESKTOP\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe" "-rootdatadir $\"%LOCALAPPDATA%\Bitfighter$\""
   NoDesktopIcon:
     
   SetOutPath $INSTDIR     
SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; Always delete uninstaller first
  Delete $INSTDIR\uninstall-bitfighter.exe

  ; And purge our install dirs, along with everything in them...
  RMDir /r $INSTDIR\sfx
  RMDir /r $INSTDIR\levels
  RMDir /r $INSTDIR\robots
  RMDir /r $INSTDIR\screenshots
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
