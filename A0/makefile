# Makefile de exemplo (Manual do GNU Make)
CFLAGS = -Wall -g # flags de compilacao
CC = gcc

all: main.o jukefila.o pedido.o
	$(CC) -o main main.o jukefila.o pedido.o

jukefila.o: jukefila.c
	$(CC) -c $(CFLAGS) jukefila.c

main.o: main.c
	$(CC) -c $(CFLAGS) main.c

pedido.o: pedido.c
	$(CC) -c $(CFLAGS) pedido.c

clean:
	rm -f *.o main
