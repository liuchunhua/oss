

-include ../sources.mk ../objects.mk

all: test
	echo "run test"

test:$(OBJ) test.o ../liboss.a
	gcc -g -o $@ test.o $(LIBS) 
	
%.o:%.c
	$(CC) -g -Wall -c -o $@ $<  $(CFLAGS) -I../
	
clean:
	rm -f test test.o
