# Bitfighter Master Server Makefile
#
# This makefile is for Linux 


# These flags are for use with both database stats and phpbb integration (both use mysql)
MYSQLFLAGS=-I/usr/include/mysql -I../mysql++

LDFLAGS=../mysql++/libmysqlpp.so 

# If you get an error about not being able to find the mysql headers, try removing MYSQLPP_MYSQL_HEADERS_BURIED flag
CFLAGS_DB_STATS=-DBF_WRITE_TO_MYSQL
CFLAGS_PHPBB=-DVERIFY_PHPBB3


CC=g++ -g -I../tnl -I../libtomcrypt -W #-O2

OBJECTS_MASTER=\
   main.o\
   masterInterface.o\
   config.o\
   ../zap/gameStats.o\
   ../zap/stringUtils.o\
   database.o

OBJECTS_DB_STATS=\

OBJECTS_PHPBB=\
   authenticator.o\
   phpbbhash.o

.cpp.o: 
	$(CC) $(MYSQLFLAGS) -c $(CFLAGS) -o $@ $<


# Compile default with all options on

default: master


# Compile without database statistics or phpbb integration

minimal: MYSQLFLAGS=
minimal: LDFLAGS=
minimal: CFLAGS=
minimal: $(OBJECTS_MASTER)
minimal: 
	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm
 

# Compile with database stats off; include phpbb integration

nodbstats: CFLAGS=$(CFLAGS_PHPBB)
nodbstats: $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
nodbstats: 
	@$(MAKE) -C ../mysql++
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


# Compile with no PHPBB integration; include database stats integration

nophpbb: CFLAGS=$(CFLAGS_DB_STATS)
nophpbb: $(OBJECTS_MASTER) $(OBJECTS_DB_STATS)
nophpbb:
	@$(MAKE) -C ../mysql++
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_DB_STATS) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS) 


# Compile all options on (with db stats and phpbb)

master: CFLAGS=$(CFLAGS_DB_STATS) $(CFLAGS_PHPBB)
master: $(OBJECTS_MASTER) $(OBJECTS_DB_STATS) $(OBJECTS_PHPBB)
master: 
	@$(MAKE) -C ../mysql++
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_DB_STATS) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


# Cleaning

clean:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB) ../exe/master
cleano:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB)
