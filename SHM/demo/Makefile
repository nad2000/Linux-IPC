CC=clang
CFLAGS=-g

all: reader writer

# shm_demo.o: shm_demo.c
# 	clang -g -c shm_demo.c -o shm_demo.o

# reader.o: reader.c
# 	clang -g -c reader.c -o reader.o

# writer.o: writer.c
# 	clang -g -c writer.c -o writer.o 

reader: shm_demo.o reader.o
	clang -g reader.o shm_demo.o -o reader -lrt

writer: shm_demo.o writer.o
	clang -g writer.o shm_demo.o -o writer -lrt

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core
