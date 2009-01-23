# Script assumes that there is a /Dversion=curVersion
!define curVersion "Release 010 preview"		###### <<<-----  TODO: Remove this.  For testing only!
;--------------------------------
; Include Modern UI

	!include "MUI2.nsh"

;--------------------------------
; General

;Name and file
Name    "Bitfighter"								; App name
OutFile "Bitfighter-Installer-${curVersion}.exe"	; Name of the installer file to write

;Default installation folder
InstallDir "$PROGRAMFILES\Bitfighter"

;Get installation folder from registry if available
InstallDirRegKey HKCU "Software\Bitfighter" ""

;Request application privileges for Windows Vista
RequestExecutionLevel user

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
; Finish page settings

; Create checkbox to run game post installation
!define MUI_FINISHPAGE_RUN $INSTDIR\Bitfighter.exe		

;--------------------------------
; Uninstall page settings

!define MUI_UNCONFIRMPAGE_TEXT_TOP "\
	This will completely remove Bitfighter from your system.  Any custom levels you created will be deleted unless you have backed them up.$\n$\n\
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
  File ".\Windows specific\OpenAL32.dll"		
  File ".\Windows specific\glut32.dll"
  File "readme.txt"
  File "End-User License.txt"
  File "..\exe\bitfighter.ini.sample"
  File ".\Windows specific\twoplayers.bat"

  SetOutPath "$INSTDIR\sfx"
  File /r ".\sfx\*.wav"

  SetOutPath "$INSTDIR\levels"
  File /r ".\levels\*.level"	
  
  SetOutPath "$INSTDIR\screenshots"
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
  		CreateShortCut "$SMPROGRAMS\Bitfighter\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe"
  	NoStartMenu:
  	
  	MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to add a desktop icon for Bitfighter?" IDNO NoDesktopIcon
  		SetOutPath $INSTDIR
  		CreateShortCut "$DESKTOP\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe"
  	NoDesktopIcon:
  
  	;MessageBox MB_YESNO|MB_ICONQUESTION "Bitfighter installation has completed.  Would you like to play?" IDNO NoPlay
  	;ExecShell open '$INSTDIR\Bitfighter.exe'
  	;NoPlay:
  	
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
  RMDir /r $INSTDIR\screenshots
  RMDir /r $INSTDIR
  
  	; Remove the links from the start menu and desktop
  	Delete $SMPROGRAMS\Bitfighter\*.*
  	RMDir $SMPROGRAMS\Bitfighter	# Delete folder on Start menu
  	Delete "$DESKTOP\Bitfighter.lnk"
  	
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Bitfighter"
  DeleteRegKey HKCU "Software\Bitfighter"
  	
      IfFileExists $INSTDIR 0 Removed 
  		MessageBox MB_OK|MB_ICONEXCLAMATION "Note: I did my best, but I could not remove $INSTDIR."
  	Removed:	


SectionEnd