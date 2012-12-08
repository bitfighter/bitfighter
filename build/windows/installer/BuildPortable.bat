  @echo off

REM Check to make sure we have the proper arguments

if "%1"=="" (
   echo Usage: BuildPortable ^<installer.exe^>
   echo or drag and drop installer.exe onto this batch file
   echo.
   pause
   exit
)



REM Check for the presence of 7Zip

set FOUND=
for %%e in (%PATHEXT%) do (
  for %%X in (7z%%e) do (
    if not defined FOUND (
      set FOUND=%%~$PATH:X
    )
  )
)

if "%FOUND%"=="" (
  echo For this script to run, you must first install 7Zip, and the 7z.exe must be on your system path!
  pause
  exit
)


REM Make the portable installer file name

set basename=%~n1
set outname=%basename:Installer=Portable%

rem Extract files from the installer, excluding special installer files
7z x -oBitfighter -x!$_OUTDIR -x!$R1 -x!$R1$R2 %1

REM Create a marker file so Bitfighter will treat this as a portable install
echo Bitfighter will run in portable mode if this file is present. > Bitfighter\.portable

REM Build a zip file from the Bitfighter folder
7z a -tzip -r %outname% Bitfighter

REM Cleanup
del /Q /S Bitfighter
rmdir /Q /S Bitfighter

echo Done!