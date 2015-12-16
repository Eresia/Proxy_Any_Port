CC=gcc
CFLAGS=-Wall -DDEBUG
OUTFLAGS= -lpthread -g

all: launch_server.out

launch_server.out: launch_server.o server.o
	$(CC) -o launch_server.out launch_server.o server.o $(OUTFLAGS)

launch_server.o: launch_server.c *.h
	$(CC) $(CFLAGS) -o launch_server.o -c launch_server.c

server.o: server.c *.h
	$(CC) $(CFLAGS) -o server.o -c server.c
