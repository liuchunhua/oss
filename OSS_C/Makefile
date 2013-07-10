
-include sources.mk 

all:List.o HashTable.o authen.o http.o ossutil.o log.o service.o String.o
	#$(MAKE) -C test
liboss.a:oss.o List.o base64.o HashTable.o ossutil.o String.o
	ar rv  liboss.a *.o
%.o:%.c 
	$(CC) -g -DDEBUG -Wall -c -o $@ $<  $(CFLAGS)

clean:
	rm -f *.a
	rm -f *.o
	#$(MAKE) -C test clean