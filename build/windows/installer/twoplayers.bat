
@echo off
IF %2.==. GOTO MissingParams

start bitfighter -name %1 -usestick 1 -window -winwidth 675 -winpos 4   28 -rootdatadir @@ROOT_DATA_DIR@@           %3 %4 %5 %6 %7 %8 %9
start bitfighter -name %2 -usestick 2 -window -winwidth 675 -winpos 687 28 -rootdatadir @@ROOT_DATA_DIR@@  -nomusic %3 %4 %5 %6 %7 %8 %9

GOTO End
1366 - 8
:MissingParams

echo Usage: twoplayers ^<LeftPlayerName^> ^<RightPlayerName^>
echo. 
echo This is a Windows batch file that shows how you might start two game windows,
echo arranged neatly on the screen (these are sized for my 1920px-width display),
echo with each player using their own joystick.  Note that to provide any keyboard
echo input into either game window, that window must have the focus.  Therefore,
echo to start a game, give one window the focus, start hosting a game, then switch
echo the focus to the other window, and join the game there. This method might
echo also work on a dual-monitor system.  I found the proper values for winwidth
echo and winpos through a combination of trial-and error, and positioning the
echo windows where I wanted them, quitting, and examining the INI file.  This is
echo not a supported technique, but I have found it works very well for allowing
echo two players to play the game at once using a single computer with two
echo joysticks attached.

:End

