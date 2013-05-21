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
      * libmodplug

   These dependencies are provided for you on Windows and OSX.

   On Linux, you must install them using your distribution's preferred method.

      
2) COMPILING

   The CMake build system is used for compiling on Windows and Linux.  You
   will need to download and install CMake for these platforms.  
   
   On OSX, there is an Xcode project available.

 a) Linux

   You will need the development headers of the above dependencies installed 
   as well as the following software to compile:
      * cmake
      * make
      * gcc / g++

   Open a terminal and change to the 'build' directory within the extracted
   bitfighter sources.  Then run the following commands:

   1) cmake ..
   2) make

   To set up a debugging build do:  cmake -DCMAKE_BUILD_TYPE=Debug ..
   
   To build a dedicated server run 'make bitfighterd'.


 b) Windows

   Bitfighter can be built with at least the following build systems:
      * Visual Studio C++ IDE 
      * MingW/MSYS

   To generate the proper project files with CMake, go into the 'build' directory
   and type:
      * cmake -G "Visual Studio 10" ..
    OR
      * cmake -G "MSYS Makefiles" ..
      
   To see a list of generators for CMake, see here:
   
    http://www.cmake.org/cmake/help/v2.8.11/cmake.html#section_Generators
    
   Different versions of Visual Studio are known to work, but you need to use the
   correct generator with CMake.
   
   
   Microsoft Visual Studio:
   
   Start Visual C++.  Select Open Project, then navigate to the 'build' folder, 
   and open the bitfighter solution file.

   Right-click on the 'bitfighter' target and select 'build' to compile the game.  This 
   will take a few minutes the first time.

   To run:

   Click Debug > Start Debugging (or press F5) to run the game.

   
   MinGW / MSYS

   After generating the "MSYS Makefiles".  All you have to do is run 'make' to compile


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

