# TNL Makefile
# (c) 2003 GarageGames
#
# This makefile is for gcc-based projects
# Works with MinGW on Windows as per instructions below


# 
# Configuration
#

#######################################
# For Linux/Mac, use the following:
#CC=g++ -g -DTNL_DEBUG -DTNL_ENABLE_LOGGING -I../libtomcrypt/src/headers -I../libtomcrypt # -O2
#######################################
# For Windows/MinGW, use the following line
CC=g++ -g -DTNL_DEBUG -DTNL_ENABLE_LOGGING -DWIN32 -I../libtomcrypt/src/headers -I../libtomcrypt # -O2
#######################################


OBJECTS=\
	assert.o\
	asymmetricKey.o\
	bitStream.o\
	byteBuffer.o\
	certificate.o\
	clientPuzzle.o\
	connectionStringTable.o\
	dataChunker.o\
	eventConnection.o\
	ghostConnection.o\
	huffmanStringProcessor.o\
	log.o\
	netBase.o\
	netConnection.o\
	netInterface.o\
	netObject.o\
	netStringTable.o\
	platform.o\
	random.o\
	rpc.o\
	symmetricCipher.o\
	tnlMethodDispatch.o\
	journal.o\
	udp.o\
	vector.o\

CFLAGS=

.cpp.o : 
	$(CC) -c $(CFLAGS) $<

default: $(OBJECTS)
	@echo Building libtnl.a ...
	ar rcv libtnl.a $(OBJECTS)

clean:
	rm -f $(OBJECTS) libtnl.a
