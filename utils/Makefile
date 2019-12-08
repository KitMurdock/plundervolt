CFLAGS = -Wall 
LFLAGS = -lncurses

all:	read_voltage

read_voltage:	read_voltage.o
	gcc $(CFLAGS) -o read_voltage read_voltage.o $(LFLAGS)

read_voltage.o:	read_voltage.c
	gcc $(CFLAGS) -c -o read_voltage.o read_voltage.c

clean:
	rm *.o
