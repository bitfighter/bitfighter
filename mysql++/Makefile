# Makefile for minimum mysql++ compilation
#
# Adapted from the sources of mysql++ 3.1.0
#
# This makefile is for Linux 


CC=g++ -g -c -I/usr/include/mysql -O2 -fPIC -DPIC

OBJECTS=beemutex.o\
cmdline.o\
connection.o\
cpool.o\
datetime.o\
dbdriver.o\
field_names.o\
field_types.o\
manip.o\
myset.o\
mysql++.o\
mystring.o\
null.o\
options.o\
qparms.o\
query.o\
result.o\
row.o\
scopedconnection.o\
sql_buffer.o\
sqlstream.o\
ssqls2.o\
stadapter.o\
tcp_connection.o\
transaction.o\
type_info.o\
uds_connection.o\
utility.o\
vallist.o\
wnp_connection.o


.cpp.o: 
	$(CC) -o $@ $<

default: static


static: $(OBJECTS)
	ar rcv libmysqlpp.a $(OBJECTS)


shared: $(OBJECTS)
	g++ -shared -o libmysqlpp.so *.o -Wl,-soname,libmysqlpp.so.3 -L/usr/lib/mysql -lmysqlclient
	rm -f libmysqlpp.so.3 2>/dev/null && ln -s libmysqlpp.so libmysqlpp.so.3

clean:
	rm -f *.o *.so* *.a
cleano:
	rm -f *.o
