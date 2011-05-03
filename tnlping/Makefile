# 
# tnlping Makefile
#

# GENERAL FLAGS
LFLAGS=-lpthread
CFLAGS=

CC=g++ -I../tnl -W -O2

OBJECTS=\
./tnlping.o

.cpp.o: 
	$(CC) -c $(CFLAGS) -o $@ $<


default: $(OBJECTS)
default: 
	@$(MAKE) -C ../libtomcrypt
	@$(MAKE) -C ../tnl
	$(CC) -o tnlping $(OBJECTS) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a $(LFLAGS) 
 

# Cleaning
clean:
	rm -f $(OBJECTS) tnlping
cleano:
	rm -f $(OBJECTS)
