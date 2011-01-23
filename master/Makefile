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
   config.o\
   authenticator.o\
   phpbbhash.o\
   database.o

# If you get an error about not being able to find the mysql headers, try removing MYSQLPP_MYSQL_HEADERS_BURIED flag
# To disable PHPBB3 integration, remove VERIFY_PHPBB3 flag
CFLAGS=-DMYSQLPP_MYSQL_HEADERS_BURIED -DVERIFY_PHPBB3


.cpp.o : 
	$(CC) $(MYSQLFLAGS) -c $(CFLAGS) $<

default: $(OBJECTS_MASTER)
# For linux and Mac, uncomment the following, and comment out the Windows line below

	$(CC) -o ../exe/master $(OBJECTS_MASTER) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a -lstdc++ -lm $(LDFLAGS)


# For windows, uncomment out the line below, and comment out the line above
#   $(CC) -o ../exe/master $(OBJECTS_MASTER)  ../tnl/libtnl.a   ../libtomcrypt/libtomcrypt.a   C:/MinGW/lib/libwsock32.a  -lstdc++ -lm  


cleano:
	rm -f $(OBJECTS_MASTER) 
clean:
	rm -f $(OBJECTS_MASTER) ../exe/master
