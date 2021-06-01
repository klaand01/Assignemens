CC = gcc
CC_FLAGS = -w -g



all: sspgame sspd


main.o: main.c
	$(CC) -Wall -I. -c main.c


test: main.o
	$(CC) -I./ -Wall -lncurses  -o test main.o 


client: sspgame
	$(CC) -Wall -o sspgame client.o

server: sspd
	$(CC) -Wall -o sspd server.o


clean:
	rm *.o *.a test sspd sspgame
