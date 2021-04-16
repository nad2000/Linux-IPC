CC=clang
CFLAGS=-g -Wall

all: server client

server: server.o routing_table.o rtm.h server.c routing_table.c
	$(CC) -lreadline -lrt server.o routing_table.o  -o server

client: client.o routing_table.o rtm.h client.c routing_table.c
	$(CC) client.o routing_table.o -o client -lrt

%.o: %.c *.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

