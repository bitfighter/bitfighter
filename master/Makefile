# Bitfighter Master Server Makefile
#
# This makefile is for Linux / MinGW on Windows


# 
# Configuration
#
CC=g++ -g -I../tnl #-O2

OBJECTS_MASTER=\
   main.o\
   masterInterface.o\
   config.o

CFLAGS=

.cpp.o : 
	$(CC) -c $(CFLAGS) $<

default: $(OBJECTS_MASTER)
# For linux and Mac, uncomment the following, and comment out the Windows line below

	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm 
# For windows, uncomment out the line below, and comment out the line above
#   $(CC) -o ../exe/master $(OBJECTS_MASTER)  ../tnl/libtnl.a   ../libtomcrypt/libtomcrypt.a   C:/MinGW/lib/libwsock32.a  -lstdc++ -lm  


cleano:
	rm -f $(OBJECTS_MASTER) 
clean:
	rm -f $(OBJECTS_MASTER) ../exe/master
