CC=clang
CFLAGS=-g -Wall

all: client server

server: server.o common.h server.c
	$(CC) -lreadline -lrt server.o -o server

client: client.o common.h client.c
	$(CC) -lrt client.o -o client

%.o: %.c *.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
