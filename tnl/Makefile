# TNL Makefile
# From Bitfigher (http://bitfighter.org)

OPTIMIZE=-O2
DEBUG=-g -DTNL_DEBUG
CC=g++ $(DEBUG) -DTNL_ENABLE_LOGGING -I../libtomcrypt/src/headers -I../libtomcrypt $(OPTIMIZE)


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
	thread.o\
	tnlMethodDispatch.o\
	journal.o\
	udp.o\
	vector.o\

CFLAGS=
DFLAGS=
WFLAGS=

.cpp.o : 
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) $<

default: release

release: DEBUG=
release: tnl

debug: OPTIMIZE=
debug: tnl

tnl: $(OBJECTS)
	@echo Building libtnl.a ...
	ar rcv libtnl.a $(OBJECTS)

dedicated: CFLAGS=-DZAP_DEDICATED
dedicated: default

cleano:
	rm -f $(OBJECTS) 

clean:
	rm -f $(OBJECTS) libtnl.a
