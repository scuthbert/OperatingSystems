CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -pthread -Wall -Wextra

all: multi-lookup

multi-lookup: multi-lookup.o util.o
	$(CC) $(LFLAGS) $^ -o $@

util.o: util.c util.h
	$(CC) $(CFLAGS) $<

multi-lookup.o: multi-lookup.c
	$(CC) $(CFLAGS) $<

clean:
	rm -f multi-lookup
	rm -f *.o
	rm -f *~
