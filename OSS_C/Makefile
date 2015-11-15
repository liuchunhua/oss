
-include sources.mk 

all: liboss.a
	$(MAKE) -C test
liboss.a: List.o HashTable.o authen.o http.o ossutil.o log.o service.o String.o oss_http.o oss.o base64.o
	ar rv  liboss.a *.o
%.o:%.c 
	$(CC) -g -DDEBUG -Wall -c -o $@ $<  $(CFLAGS)
testcase: test.c liboss.a
	gcc -g -o testcase *.c -I/usr/include/libxml2 -lssl -lcrypto -lcurl -lxml2 
ali: ali.c liboss.a
	gcc -g -o ali ali.c -I. -L. -I/usr/include/libxml2 -lssl -lcrypto -lcurl -lxml2 -loss 
clean:
	rm -f *.a
	rm -f *.o
	$(MAKE) -C test clean
