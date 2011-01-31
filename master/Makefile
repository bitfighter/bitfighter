# Bitfighter Master Server Makefile
#
# This makefile is for Linux 


# 
# Configuration
#

# These flags are for use with both mysql integration and phpbb integration (both use mysql)
MYSQLFLAGS=-I/usr/include/mysql -Imysql++ -I../mysql++
LDFLAGS= ../mysql++/libmysqlpp.so 

# If you get an error about not being able to find the mysql headers, try removing MYSQLPP_MYSQL_HEADERS_BURIED flag
CFLAGS_STATS=-DBF_STATS -DMYSQLPP_MYSQL_HEADERS_BURIED
CFLAGS_PHPBB=-DVERIFY_PHPBB3


CC=g++ -g -I../tnl -I../libtomcrypt -W #-O2

OBJECTS_MASTER=\
   main.o\
   masterInterface.o\
   config.o\
   ../zap/gameStats.o\
   ../zap/stringUtils.o

OBJECTS_STATS=\
   database.o

OBJECTS_PHPBB=\
   authenticator.o\
   phpbbhash.o

.cpp.o: 
	$(CC) $(MYSQLFLAGS) -c $(CFLAGS) -o $@ $<


default: master

minimal: MYSQLFLAGS=
minimal: LDFLAGS=
minimal: CFLAGS=
minimal: $(OBJECTS_MASTER)
minimal: 
	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)
 

nostats: CFLAGS=$(CFLAGS_PHPBB)
nostats: $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
nostats: 
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


nophpbb: CFLAGS=$(CFLAGS_STATS)
nophpbb: $(OBJECTS_MASTER) $(OBJECTS_STATS)
nophpbb:
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_STATS) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS) 


master: CFLAGS=$(CFLAGS_STATS) $(CFLAGS_PHPBB)
master: $(OBJECTS_MASTER) $(OBJECTS_STATS) $(OBJECTS_PHPBB)
master: 
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_STATS) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


# For windows, uncomment out the line below, and comment out the line above
#   $(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_MYSQL)  $(OBJECTS_PHPBB) ../tnl/libtnl.a   ../libtomcrypt/libtomcrypt.a   C:/MinGW/lib/libwsock32.a  -lstdc++ -lm  

cleano:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB)
clean:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB) ../exe/master
