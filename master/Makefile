# Bitfighter Master Server Makefile
#
# This makefile is for Linux 


# 
# Configuration
#

# The following lines can be commented out if you are not using PHP or #PHPBB3 integration
#PHPBB_INCLUDES=-I/usr/include/mysql -Imysql++
#PHPBB_CFLAGS= -DVERIFY_PHPBB3 -MYSQLPP_MYSQL_HEADERS_BURIED

# Note, if you install the correct development package you don't need this
#PHPBB_LDFLAGS=../mysql++/libmysqlpp.so 

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
CFLAGS=


.cpp.o : 
	$(CC) $(PHPBB_INCLUDES) -c $(PHPBB_CFLAGS) $<


default: $(OBJECTS_PHPBB) master


nophpbb: PHPBB_CFLAGS= 
nophpbb: PHPBB_INCLUDES= 
nophpbb: PHPBB_LDFLAGS=
nophpbb: OBJECTS_PHPBB=
nophpbb: master 


master: $(OBJECTS_MASTER)
	$(CC) -o ../exe/master $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(PHPBB_LDFLAGS)


cleano:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_PHPBB)
clean:
	rm -f $(OBJECTS_MASTER) $(OBJECTS_PHPBB) ../exe/master
