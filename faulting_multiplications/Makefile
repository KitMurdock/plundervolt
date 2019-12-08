CFLAGS =  -Wall -O0
LFLAGS = -lncurses

all: operation 

operation: operation.o
	gcc $(CFLAGS) -o operation operation.o -lpthread $(LFLAGS)

operation.o: operation.c
	gcc $(CFLAGS) -c -o operation.o operation.c -mrdrnd


clean:
	rm *.o
