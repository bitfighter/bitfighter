Bitfighter README

   Many resources are found at:

    http://bitfighter.org/wiki/index.php?title=Main_Page

   Most is still valid...   ;-)

1) DEPENDENCIES

   Bitfighter's dependencies have changed drastically from version 015a to 
   016.  The following are needed for 016 and later.

   Bitfighter has several common, open source dependencies:
      * SDL
      * zlib
      * libpng
      * openal
      * libvorbis
      * libspeex
      * readline (Linux-only)
      * ncurses (Linux-only)

   These dependencies are provided for you on Windows/Mac, with the exception
   of SDL on Mac which can be obtained from:

    http://www.libsdl.org/download-1.2.php

   On Linux, you must install them using your distribution's preferred method.

      
2) COMPILING

 a) Linux

   You will need the development headers of the above dependencies installed 
   as well as the following software to compile:
      * gcc / g++
      * make

   To compile run 'make' in the root directory of the project.  

   To build a dedicated server run 'make dedicated'.

   More options available in the Makefile


 b) Windows

   Bitfighter is built with the Visual Studio 2010 IDE

   Install Visual C++ 2010 Express  -- this is a free download from Microsoft.

   Start Visual C++.  Select Open Project, then navigate to the source folder, and open
   the Bitfighter solution file.

   Select Debug > Build Solution (or press F7) to compile the game.  This will take a 
   few minutes the first time.

   To run:
   
   Select bitfighter from the Solution Explorer pane on the left of theGUI, right click, 
   and select Properties > Debugging. Under Command Arguments add "-rootdatadir XXX" 
   (without quotes) where XXX is the folder containing your levels, sounds, etc., 
   perhaps pointing to an existing install of Bitfighter.

   Click Debug > Start Debugging (or press F5) to run the game.

 c) Windows Mingw

   Bitfighter can also be compiled using Mingw for windows.

   Install Mingw, available as free download at www.mingw.org

   Run "cmd" to open command, then enter these command:

   PATH = C:\MinGW\bin;%PATH%
   cd C:\Program Files\Bitfighter
   mingw32-make


 d) MacOS X (10.4 - 10.7)

   Bitfighter is built with the Xcode IDE.  The Xcode project is found in the directory:

    build/osx/xcode

   To compile, select 'Release' mode and build the Bitfighter target


3) INSTALLATION AND PACKAGING

 a) Linux

   There is no 'make install'.

   After running 'make', the bitfighter executable is put into the directory
   'exe/'.  Copy everything from the 'resources/' directory into the 'exe/'
   directory, keeping the folders intact (like sfx, scripts, etc.)

   Bitfighter can now be run from the 'exe/' folder.

   For distribution packaging, Bitfighter is built for a few distributions
   using the Open Build Service at:

    https://build.opensuse.org/package/show?package=bitfighter&project=games


 b) Windows

   An NSIS build script is found in the following directory:

    build/windows/installer

   This will build a self-extracting installer for Bitfighter.  You will need to 
   build in 'Release' mode as the installer is looking for an executable named 
   'bitfighter.exe'


 c) MacOS X

   Run the target called "Create Bitfighter Game Release" to build a 
   distributable DMG.


4) CRASHES & PROBLEMS

   If you are building off of 'tip' then expect crashes and problems.
   Please don't report them via Google Code; instead join the IRC channel
   #bitfighter on freenode to ask a developer about it - we probably are
   already aware.

   If you are building off of a release version, please make sure you can
   consistently reproduce the problem, then post it, with the steps to
   reproduce at:

    http://code.google.com/p/bitfighter/issues/list

   For feature requests or similar, please post in the forums at:

    http://bitfighter.org/forums/viewforum.php?f=4


5) COMPATIBILITY

   TODO


6) OTHER NOTES

   Come join us in the forums at:

    http://bitfighter.org/forums/

   Enjoy Bitfighter!

