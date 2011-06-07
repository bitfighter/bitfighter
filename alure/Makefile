# 
# Alure Makefile
#

# GENERAL FLAGS
LFLAGS=-lstdc++ -lopenal -lvorbis
EXTRA_FLAGS=-D_GNU_SOURCE -DALURE_STATIC_LIBRARY -DDYNLOAD=1 -DHAVE_CONFIG_H -fPIC

CC=g++ -I. -I../libvorbis -I../libogg -W -O2
AR=ar cr

OBJECTS=\
buffer.o\
streamdec.o\
stream.o\
streamplay.o\
istream.o\
codec_wav.o\
codec_aiff.o\
alure.o\
codec_vorbisfile.o

.cpp.o: 
	$(CC) $(EXTRA_FLAGS) -c $(CFLAGS) -o $@ $<


# Compile default with mysql for everything

default: $(OBJECTS)
default: 
#	$(CC) -o libalure.a $(OBJECTS) $(LFLAGS)
	$(AR) libalure.a $(OBJECTS)
 

# Cleaning

clean:
	rm -f $(OBJECTS) libalure.a
cleano:
	rm -f $(OBJECTS)
