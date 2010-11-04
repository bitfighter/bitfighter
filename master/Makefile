# Bitfighter Master Server Makefile
#
# This makefile is for Linux 


# 
# Configuration
#

# The following lines can be commented out if you are not using PHP or PHPBB3 integration
MYSQLFLAGS=-I/usr/include/mysql -Imysql++ 
LDFLAGS= ../mysql++/libmysqlpp.so 

CC=g++ -g -I../tnl -I../libtomcrypt #-O2

OBJECTS_MASTER=\
   main.o\
   masterInterface.o\
   config.o
   #database.o

OBJECTS_PHPBB=\
   authenticator.o\
   phpbbhash.o 

# If you get an error about not being able to find the mysql headers, try removing MYSQLPP_MYSQL_HEADERS_BURIED flag
# To disable PHPBB3 integration, remove VERIFY_PHPBB3 flag
CFLAGS=-DMYSQLPP_MYSQL_HEADERS_BURIED -DVERIFY_PHPBB3


.cpp.o : 
	$(CC) $(MYSQLFLAGS) -c $(CFLAGS) $<


default: $(OBJECTS_PHPBB) master


nophpbb: CFLAGS=
nophpbb: OBJECTS_PHPBB= 
nophpbb: master


master: $(OBJECTS_MASTER)
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


cleano:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
clean:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../exe/master
