CC = gcc
CC_FLAGS = -w -g



all: client server


client: sspgame
	$(CC) -Wall -o sspgame client.o

server: sspd
	$(CC) -Wall -o sspd server.o


clean:
	rm *.o *.a sspd sspgame
