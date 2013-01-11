CFLAGS=-I/mnt/data/git/OSS/OSS_C 
test:test.o cache_debug.o 
	gcc -g -o testcase  `pkg-config fuse  --libs`   -lssl -lcrypto -lcurl -lxml2  $^ /mnt/data/git/OSS/OSS_C/liboss.a 
all:hello.o cache.o
	gcc -Wall -o hello  `pkg-config fuse  --libs`   -lssl -lcrypto -lcurl -lxml2  $^ /mnt/data/git/OSS/OSS_C/liboss.a 

cache_debug.o:cache.c
	$(CC) -g -DDEBUG -Wall -c -o $@ $<  $(CFLAGS)
cache.o:cache.c
	$(CC) -g -Wall -c -o $@ $<  $(CFLAGS) 
test.o:test/test.c 
	$(CC) -g -Wall -c -o test.o test/test.c $(CFLAGS) 
clean:
	rm -f *.o
	rm -f hello
	rm -f testcase