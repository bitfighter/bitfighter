This folder contains patches applied against the libraries

Current patches:

sdl2-restore-osx-10.4-compat.diff
 - Bring OSX 10.4 compatibility to SDL 2.0.1 (hg b9663c77f5c9)

sdl2-xcode-osx104-configuration.diff
 - Modification of upstream Xcode project to add a 'Release-10.4' configuration that will build i386 and ppc universal library.  This can be joined with the x86_64 build ('Release' configuration) to form a 3-architecture FAT library.  Use 'lipo' like this to do it:
   lipo -create -output SDL2 Release-10.4/SDL2.framework/Versions/A/SDL2 Release/SDL2.framework/Versions/A/SDL2
