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
  ExecShell "" "$INSTDIR\Play Bitfighter.lnk"
FunctionEnd


;--------------------------------
; See  <<http://nsis.sourceforge.net/More_advanced_replace_text_in_file>>  for usage details
; This function allows you to replace pieces of text in a file. Instead of replacing all text found in the file, you have the extra 
; option of replacing text after the x times of it occurring, and x times to replace the text after that occurrence. 
Function AdvReplaceInFile
Exch $0 ;file to replace in
Exch
Exch $1 ;number to replace after
Exch
Exch 2
Exch $2 ;replace and onwards
Exch 2
Exch 3
Exch $3 ;replace with
Exch 3
Exch 4
Exch $4 ;to replace
Exch 4
Push $5 ;minus count
Push $6 ;universal
Push $7 ;end string
Push $8 ;left string
Push $9 ;right string
Push $R0 ;file1
Push $R1 ;file2
Push $R2 ;read
Push $R3 ;universal
Push $R4 ;count (onwards)
Push $R5 ;count (after)
Push $R6 ;temp file name
 
  GetTempFileName $R6
  FileOpen $R1 $0 r ;file to search in
  FileOpen $R0 $R6 w ;temp file
   StrLen $R3 $4
   StrCpy $R4 -1
   StrCpy $R5 -1
 
loop_read:
 ClearErrors
 FileRead $R1 $R2 ;read line
 IfErrors exit
 
   StrCpy $5 0
   StrCpy $7 $R2
 
loop_filter:
   IntOp $5 $5 - 1
   StrCpy $6 $7 $R3 $5 ;search
   StrCmp $6 "" file_write1
   StrCmp $6 $4 0 loop_filter
 
StrCpy $8 $7 $5 ;left part
IntOp $6 $5 + $R3
IntCmp $6 0 is0 not0
is0:
StrCpy $9 ""
Goto done
not0:
StrCpy $9 $7 "" $6 ;right part
done:
StrCpy $7 $8$3$9 ;re-join
 
IntOp $R4 $R4 + 1
StrCmp $2 all loop_filter
StrCmp $R4 $2 0 file_write2
IntOp $R4 $R4 - 1
 
IntOp $R5 $R5 + 1
StrCmp $1 all loop_filter
StrCmp $R5 $1 0 file_write1
IntOp $R5 $R5 - 1
Goto file_write2
 
file_write1:
 FileWrite $R0 $7 ;write modified line
Goto loop_read
 
file_write2:
 FileWrite $R0 $R2 ;write unmodified line
Goto loop_read
 
exit:
  FileClose $R0
  FileClose $R1
 
   SetDetailsPrint none
  Delete $0
  Rename $R6 $0
  Delete $R6
   SetDetailsPrint both
 
Pop $R6
Pop $R5
Pop $R4
Pop $R3
Pop $R2
Pop $R1
Pop $R0
Pop $9
Pop $8
Pop $7
Pop $6
Pop $5
Pop $0
Pop $1
Pop $2
Pop $3
Pop $4
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
; Installer Section

Section "Install"

  SetOutPath "$INSTDIR"
  File "..\..\..\exe\Bitfighter.exe"
  File "..\..\..\exe\joystick_presets.ini"
  
  CreateShortCut "$INSTDIR\Play Bitfighter.lnk" "$INSTDIR\Bitfighter.exe" "-rootdatadir $\"$DOCUMENTS\Bitfighter$\""

  File "..\..\..\lib\OpenAL32.dll"
  File "..\..\..\lib\ALURE32.dll"
  File "..\..\..\lib\libogg.dll"
  File "..\..\..\lib\libspeex.dll"
  File "..\..\..\lib\libvorbis.dll"
  File "..\..\..\lib\libvorbisfile.dll"
  File "..\..\..\lib\SDL.dll"
  File "..\..\..\lib\zlib1.dll"
  File "..\..\..\lib\libpng14.dll"

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

  SetOutPath "$DOCUMENTS\Bitfighter\music"
  File /r "..\..\..\resource\music\*.ogg"
  
  SetOutPath "$DOCUMENTS\Bitfighter\editor_plugins"
  File /r "..\..\..\resource\editor_plugins\*.*"
  
  SetOutPath "$DOCUMENTS\Bitfighter\scripts"
  File /r "..\..\..\resource\scripts\*.lua"

  SetOutPath "$DOCUMENTS\Bitfighter\levels"
  File /r "..\..\..\resource\levels\*.level"  
  File /r "..\..\..\resource\levels\*.levelgen"

  SetOutPath "$DOCUMENTS\Bitfighter\robots"
  File /r "..\..\..\resource\robots\*.bot" 

  
  ;Insert datadir path into twoplayers.bat
  Push "@@ROOT_DATA_DIR@@" ; text to be replaced
  Push $\"$DOCUMENTS\Bitfighter$\"  ; replace with
  Push all   ; replace all occurrences
  Push all   ; replace all occurrences
  Push "$INSTDIR\twoplayers.bat" ; file to replace in
  
  Call AdvReplaceInFile
  
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
      CreateShortCut "$SMPROGRAMS\Bitfighter\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe" "-rootdatadir $\"$DOCUMENTS\Bitfighter$\""
   NoStartMenu:
   
   MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to add a desktop icon for Bitfighter?" IDNO NoDesktopIcon
      SetOutPath $INSTDIR
      CreateShortCut "$DESKTOP\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe" "-rootdatadir $\"$DOCUMENTS\Bitfighter$\""
   NoDesktopIcon:
     
   SetOutPath $INSTDIR     
SectionEnd

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
