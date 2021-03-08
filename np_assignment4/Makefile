CC = gcc
CC_FLAGS = -w -g



all: client server

client.o: client.c
	$(CC) -Wall -I. -c client.c

server.o: server.c
	$(CC) -Wall -I. -c server.c

client: client.o
	$(CC) -Wall -pthread -o sspgame client.o

server: server.o
	$(CC) -Wall -pthread -o sspd server.o


clean:
	rm *.o sspd sspgame
