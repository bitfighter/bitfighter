In order to build native libraries for Android, the sources must be put into
the jni/ folder.

Right now, with no audio, you must copy the following bitfighter directories 
into the jni/ folder:

libtomcrypt
lua-vec
tnl
zap

In addition you must put in the SDL sources gotten from the latest hg changeset
of the 2.0 branch found at http://hg.libsdl.org/SDL/

On Linux/Mac you can just use soft-links instead of copying the entire folder.
Example:

cd jni/
ln -s ../../../../tnl

Please do NOT commit anything in the jni/ directory!!

Good luck!
