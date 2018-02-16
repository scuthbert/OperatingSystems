# File: Makefile
# By: Andy Sayler <www.andysayler.com>
# Adopted from work by: Chris Wailes <chris.wailes@gmail.com>
# Project: CSCI 3753 Programming Assignment 4
# Creation Date: 2010/04/06
# Modififed Date: 2012/04/12
# Second Modified Date: 2017/03/12
# Description:
#	This is the Makefile for PA5.
# 	Modified for Sam Cuthbertson's submission


CC           = gcc

CFLAGSFUSE   = `pkg-config fuse --cflags`
LLIBSFUSE    = `pkg-config fuse --libs`
LLIBSOPENSSL = -lcrypto

CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

.PHONY: all pa4-encfs clean

all: pa4-encfs

pa4-encfs: pa4-encfs.o aes-crypt.o
	$(CC) $(LFLAGS) $^ -o $@ $(LLIBSFUSE) $(LLIBSOPENSSL)

pa4-encfs.o: pa4-encfs.c
	$(CC) $(CFLAGS) $(CFLAGSFUSE) $<

aes-crypt.o: aes-crypt.c aes-crypt.h
	$(CC) $(CFLAGS) $<

clean:
	rm -f pa4-encfs
	rm -f *.o
	rm -f *~
