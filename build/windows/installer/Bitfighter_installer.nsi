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

; Request application privileges for Windows Vista
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


;--------------------------------
; Installer Section

Section "Install"

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
   
   MessageBox MB_YESNO|MB_ICONQUESTION "Bitfighter has been installed.  Would you like to add a desktop icon for Bitfighter?" IDNO NoDesktopIcon
      SetOutPath $INSTDIR
      CreateShortCut "$DESKTOP\Bitfighter.lnk" "$INSTDIR\Bitfighter.exe"
   NoDesktopIcon:
     
   SetOutPath $INSTDIR


  ; Migrate old settings folder, if it exists
  ${If} ${FileExists} `$DOCUMENTS\Bitfighter\*.*`
    ${AndIf} ${FileExists} `$DOCUMENTS\Bitfighter\bitfighter.ini`
    ${AndIf} ${FileExists} `$DOCUMENTS\Bitfighter\robots\*.*`
    ${AndIfNot} ${FileExists} `$APPDATA\Bitfighter\*.*`
        StrCpy $switch_overwrite 0
        !insertmacro MoveFolder "$DOCUMENTS\Bitfighter" "$APPDATA\Bitfighter" "*.*"
  ${EndIf}

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


; GetParameters function required by GetParameterValue (below)
; From: http://nsis.sourceforge.net/Get_command_line_parameters
;
; GetParameters
; input, none
; output, top of stack (replaces, with e.g. whatever)
; modifies no other variables.

!include "StrFunc.nsh"

Function GetParameters
 
  Push $R0
  Push $R1
  Push $R2
  Push $R3
 
  StrCpy $R2 1
  StrLen $R3 $CMDLINE
 
  ;Check for quote or space
  StrCpy $R0 $CMDLINE $R2
  StrCmp $R0 '"' 0 +3
    StrCpy $R1 '"'
    Goto loop
  StrCpy $R1 " "
 
  loop:
    IntOp $R2 $R2 + 1
    StrCpy $R0 $CMDLINE 1 $R2
    StrCmp $R0 $R1 get
    StrCmp $R2 $R3 get
    Goto loop
 
  get:
    IntOp $R2 $R2 + 1
    StrCpy $R0 $CMDLINE 1 $R2
    StrCmp $R0 " " get
    StrCpy $R0 $CMDLINE "" $R2
 
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0
 
FunctionEnd



Function StrStr ;${UN}StrStr
   Exch $R1 ; st=haystack,old$R1, $R1=needle
   Exch    ; st=old$R1,haystack
   Exch $R2 ; st=old$R1,old$R2, $R2=haystack
   Push $R3
   Push $R4
   Push $R5
   StrLen $R3 $R1
   StrCpy $R4 0
   ; $R1=needle
   ; $R2=haystack
   ; $R3=len(needle)
   ; $R4=cnt
   ; $R5=tmp
   loop:
     StrCpy $R5 $R2 $R3 $R4
     StrCmp $R5 $R1 done
     StrCmp $R5 "" done
     IntOp $R4 $R4 + 1
     Goto loop
 done:
   StrCpy $R1 $R2 "" $R4
   Pop $R5
   Pop $R4
   Pop $R3
   Pop $R2
   Exch $R1
 FunctionEnd
;!macroend


; Requires GetParameters function (above)
; From: http://nsis.sourceforge.net/Get_command_line_parameter_by_name
;
; GetParameterValue
; Chris Morgan<cmorgan@alum.wpi.edu> 5/10/2004
; -Updated 4/7/2005 to add support for retrieving a command line switch
;  and additional documentation
;
; Searches the command line input, retrieved using GetParameters, for the
; value of an option given the option name.  If no option is found the
; default value is placed on the top of the stack upon function return.
;
; This function can also be used to detect the existence of just a
; command line switch like /OUTPUT  Pass the default and "OUTPUT"
; on the stack like normal.  An empty return string "" will indicate
; that the switch was found, the default value indicates that
; neither a parameter or switch was found.
;
; Inputs - Top of stack is default if parameter isn't found,
;  second in stack is parameter to search for, ex. "OUTPUT"
; Outputs - Top of the stack contains the value of this parameter
;  So if the command line contained /OUTPUT=somedirectory, "somedirectory"
;  will be on the top of the stack when this function returns
;
; Register usage
;$R0 - default return value if the parameter isn't found
;$R1 - input parameter, for example OUTPUT from the above example
;$R2 - the length of the search, this is the search parameter+2
;      as we have '/OUTPUT='
;$R3 - the command line string
;$R4 - result from StrStr calls
;$R5 - search for ' ' or '"'
 
Function GetParameterValue
  Exch $R0  ; get the top of the stack(default parameter) into R0
  Exch      ; exchange the top of the stack(default) with
            ; the second in the stack(parameter to search for)
  Exch $R1  ; get the top of the stack(search parameter) into $R1
 
  ;Preserve on the stack the registers used in this function
  Push $R2
  Push $R3
  Push $R4
  Push $R5
 
  Strlen $R2 $R1+2    ; store the length of the search string into R2
 
  Call GetParameters  ; get the command line parameters
  Pop $R3             ; store the command line string in R3
 
  # search for quoted search string
  StrCpy $R5 '"'      ; later on we want to search for a open quote
  Push $R3            ; push the 'search in' string onto the stack
  Push '"/$R1='       ; push the 'search for'
  Call StrStr         ; search for the quoted parameter value
  Pop $R4
  StrCpy $R4 $R4 "" 1   ; skip over open quote character, "" means no maxlen
  StrCmp $R4 "" "" next ; if we didn't find an empty string go to next
 
  # search for non-quoted search string
  StrCpy $R5 ' '      ; later on we want to search for a space since we
                      ; didn't start with an open quote '"' we shouldn't
                      ; look for a close quote '"'
  Push $R3            ; push the command line back on the stack for searching
  Push '/$R1='        ; search for the non-quoted search string
  Call StrStr
  Pop $R4
 
  ; $R4 now contains the parameter string starting at the search string,
  ; if it was found
next:
  StrCmp $R4 "" check_for_switch ; if we didn't find anything then look for
                                 ; usage as a command line switch
  # copy the value after /$R1= by using StrCpy with an offset of $R2,
  # the length of '/OUTPUT='
  StrCpy $R0 $R4 "" $R2  ; copy commandline text beyond parameter into $R0
  # search for the next parameter so we can trim this extra text off
  Push $R0
  Push $R5            ; search for either the first space ' ', or the first
                      ; quote '"'
                      ; if we found '"/output' then we want to find the
                      ; ending ", as in '"/output=somevalue"'
                      ; if we found '/output' then we want to find the first
                      ; space after '/output=somevalue'
  Call StrStr         ; search for the next parameter
  Pop $R4
  StrCmp $R4 "" done  ; if 'somevalue' is missing, we are done
  StrLen $R4 $R4      ; get the length of 'somevalue' so we can copy this
                      ; text into our output buffer
  StrCpy $R0 $R0 -$R4 ; using the length of the string beyond the value,
                      ; copy only the value into $R0
  goto done           ; if we are in the parameter retrieval path skip over
                      ; the check for a command line switch
 
; See if the parameter was specified as a command line switch, like '/output'
check_for_switch:
  Push $R3            ; push the command line back on the stack for searching
  Push '/$R1'         ; search for the non-quoted search string
  Call StrStr
  Pop $R4
  StrCmp $R4 "" done  ; if we didn't find anything then use the default
  StrCpy $R0 ""       ; otherwise copy in an empty string since we found the
                      ; parameter, just didn't find a value
 
done:
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Pop $R1
  Exch $R0 ; put the value in $R0 at the top of the stack
FunctionEnd