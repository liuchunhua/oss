CFLAGS=-I/mnt/data/git/OSS/OSS_C 

all:hello.o cache.o
	gcc -Wall -o hello  `pkg-config fuse  --libs`   -lssl -lcrypto -lcurl -lxml2  $^ /mnt/data/git/OSS/OSS_C/liboss.a 
	
.o:.c
	$(CC) -g -DDEBUG -Wall -c -o $@ $<  $(CFLAGS)
clean:
	rm -f *.o
	rm -f hello;