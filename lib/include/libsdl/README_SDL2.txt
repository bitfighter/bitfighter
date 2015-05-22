The SDL 2.0 headers included here are from teh SDL hg revision 6bb657898f55

To use SDL 2.0 in Bitfighter, you need to change the header include path to
point to '../libsdl/SDL2' instead of '../libsdl/SDL'.  The build should
autodetect the different SDL version and enable the respective blocks in
the Bitfighter code
