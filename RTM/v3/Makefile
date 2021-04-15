CC=clang
CFLAGS=-g -Wall

all: client server

server: server.o routing_table.o rtm.h server.c routing_table.c
	$(CC) -lreadline -lrt server.o routing_table.o  -o server

client: client.o routing_table.o rtm.h client.c routing_table.c
	$(CC) -lrt client.o routing_table.o -o client

%.o: %.c *.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

