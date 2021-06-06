CC = gcc
CC_FLAGS = -w -g



all: sspgame sspd


client: client.o
	$(CC) -Wall -o sspgame client.o

server: server.o
	$(CC) -Wall -o sspd server.o


clean:
	rm sspd sspgame
