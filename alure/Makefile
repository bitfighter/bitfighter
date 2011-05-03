# 
# Alure Makefile
#

# GENERAL FLAGS
LFLAGS=-lstdc++ -lopenal #-lvorbis
EXTRA_FLAGS=-D_GNU_SOURCE -DALURE_STATIC_LIBRARY -DDYNLOAD=1 -DHAVE_CONFIG_H -fPIC

CC=g++ -I. -I./include -W -O2
AR=ar cr

OBJECTS=\
./src/buffer.o\
./src/streamdec.o\
./src/stream.o\
./src/streamplay.o\
./src/istream.o\
./src/codec_wav.o\
./src/codec_aiff.o\
./src/alure.o#\
#./src/codec_vorbisfile.o

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
