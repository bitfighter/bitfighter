This folder contains the project files for creating the libraries in the lib/ folder.  This is for future reference incase we need to build them again.

The sources are not included as they can be downloaded from the various library websites

Xcode project notes:
 - Put the project files in a sub-folder called 'macosx' of the download and extracted sources of which ever project you wish to compile.  Ex.  download and extract libvorbis 1.3.2 and put the Vorbis.xcodeproj file in a subfolder called 'macosx' in the extracted sources.
 - Frameworks are built for three architectures of the libraries:  i386, ppc, x86_64.  Normally, the source for the libraries included Xcode projects that only compiled one or two of the architectures.  SDL upstream already does this.
 - Patches for any libraries are contained in the lib/patches directory. Ex. The patch to passthrough Mac shortcut keys

