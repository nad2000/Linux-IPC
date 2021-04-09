CC=clang
CFLAGS=-g -Wall 

server: server.o routing_table.o rtm.h
	$(CC) -lreadline -lrt server.o routing_table.o  -o server

client: client.o routing_table.o rtm.h
	$(CC) client.o routing_table.o -o client -lrt

all: server client

%.o: %.c *.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

