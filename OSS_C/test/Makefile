

-include ../sources.mk ../objects.mk

all: test
	echo "run test"

test:$(OBJ) test.o
	gcc -g -o $@ $(OBJ) test.o $(LIBS)
	
%.o:%.c
	$(CC) -g -Wall -c -o $@ $<  $(CFLAGS)
	
clean:
	rm $(OBJ) test test.o