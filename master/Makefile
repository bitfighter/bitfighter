# Bitfighter Master Server Makefile
#
# This makefile is for Linux 


# These flags are for use with mysql: both with database stats and phpbb integration
IFLAGS_MYSQL=-I/usr/include/mysql -I../mysql++
# TODO LFLAGS_MYSQL only works for CentOS 32bit right now
LFLAGS_MYSQL=-L/usr/lib/mysql -lmysqlclient
MYSQLPP_LIB=../mysql++/libmysqlpp.a 

# Other CFLAGS
CFLAGS_MYSQL_STATS=-DBF_WRITE_TO_MYSQL
CFLAGS_PHPBB=-DVERIFY_PHPBB3

# GENERAL FLAGS
LFLAGS_EXTRA=-lm -ldl -lstdc++ -lpthread
EXTRA_FLAGS=

CC=gcc -g -I../tnl -I../libtomcrypt -I../boost -Wall -O2

OBJECTS_MASTER=\
   master.o\
   masterInterface.o\
   config.o\
   ../zap/Color.o\
   ../zap/gameStats.o\
   ../zap/IniFile.o\
   ../zap/stringUtils.o\
   ../zap/WeaponInfo.o\
   ../sqlite/sqlite3.o\
   database.o

OBJECTS_PHPBB=\
   authenticator.o\
   phpbbhash.o


.cpp.o: 
	$(CC) $(EXTRA_FLAGS) -c $(CFLAGS) -o $@ $<

.c.o: 
	$(CC) $(EXTRA_FLAGS) -c $(CFLAGS) -o $@ $<


# Compile default with mysql for everything

default: mysql

help:
	@echo
	@echo "    default:       see mysql"
	@echo
	@echo "    minimal:       no phpbb; use default sqlite3 for stats"
	@echo "    nomysqlstats:  phpbb; use default sqlite3 for stats"
	@echo "    nophpbb:       no phpbb; use mysql for DB stats"
	@echo "    mysql:         phpbb; use mysql for DB stats"
	@echo
	@echo "    Note that you may need to build TNL and libtomcrypt befre you build this"
	@echo "    To do that, go to the tnl and libtomcrypt folders and run make"
	@echo


# Compile without mysql integration for phpbb or stats (use sqlite3 for stats)

minimal: $(OBJECTS_MASTER)
minimal: 
	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a $(LFLAGS_EXTRA)
 

# Use default sqlite3 for stats; integrate with phpbb

nomysqlstats: EXTRA_FLAGS=$(CFLAGS_PHPBB) $(IFLAGS_MYSQL)
nomysqlstats: $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
nomysqlstats: 
	@$(MAKE) -C ../mysql++
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a $(MYSQLPP_LIB) $(LFLAGS_EXTRA) $(LFLAGS_MYSQL)


# Compile with no phpbb integration; include mysql database stats integration

nophpbb: EXTRA_FLAGS=$(CFLAGS_MYSQL_STATS) $(IFLAGS_MYSQL)
nophpbb: $(OBJECTS_MASTER)
nophpbb:
	@$(MAKE) -C ../mysql++
	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a $(MYSQLPP_LIB) $(LFLAGS_EXTRA) $(LFLAGS_MYSQL)


# Compile all options on (with db stats and phpbb)

mysql: EXTRA_FLAGS=$(CFLAGS_MYSQL_STATS) $(CFLAGS_PHPBB) $(IFLAGS_MYSQL)
mysql: $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
mysql: 
	@$(MAKE) -C ../mysql++
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a $(MYSQLPP_LIB) $(LFLAGS_EXTRA) $(LFLAGS_MYSQL)


# Cleaning

clean:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../exe/master
cleano:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
