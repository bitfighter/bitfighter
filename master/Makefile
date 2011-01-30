# Bitfighter Master Server Makefile
#
# This makefile is for Linux 


# 
# Configuration
#

# These flags are for use with both mysql integration and phpbb integration (both use mysql)
MYSQLFLAGS=-I/usr/include/mysql -Imysql++ 
LDFLAGS= ../mysql++/libmysqlpp.so 

# If you get an error about not being able to find the mysql headers, try removing MYSQLPP_MYSQL_HEADERS_BURIED flag
CFLAGS_MYSQL=-DMYSQLPP_MYSQL_HEADERS_BURIED
CFLAGS_PHPBB=-DVERIFY_PHPBB3


CC=g++ -g -I../tnl -I../libtomcrypt -W #-O2

OBJECTS_MASTER=\
   main.o\
   masterInterface.o\
   config.o\

OBJECTS_MYSQL=\
   database.o

OBJECTS_PHPBB=\
   authenticator.o\
   phpbbhash.o\

.cpp.o : 
	$(CC) $(MYSQLFLAGS) -c $(CFLAGS) $<


default: CFLAGS=$(CFLAGS_MYSQL) $(CFLAGS_PHPBB)
default: master

minimal: MYSQLFLAGS=
minimal: LDFLAGS=
minimal: CFLAGS=
minimal: OBJECTS_MYSQL=
minimal: OBJECTS_PHPBB=
minimal: master

nomysql: CFLAGS=$(CFLAGS_PHPBB)
nomysql: OBJECTS_MYSQL=
nomysql: master

nophpbb: CFLAGS=$(CFLAGS_MYSQL)
nophpbb: OBJECTS_PHPBB=
nophpbb: master

interface: CFLAGS=
interface: MYSQLFLAGS=
interface: masterInterface.o

master: $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB)
# For linux and Mac, uncomment the following, and comment out the Windows line below

	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


# For windows, uncomment out the line below, and comment out the line above
#   $(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_MYSQL)  $(OBJECTS_PHPBB) ../tnl/libtnl.a   ../libtomcrypt/libtomcrypt.a   C:/MinGW/lib/libwsock32.a  -lstdc++ -lm  

cleano:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB)
clean:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_MYSQL) $(OBJECTS_PHPBB) ../exe/master
