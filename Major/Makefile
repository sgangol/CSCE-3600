CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-pthread

all: client server

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

client: cliMajor.c common.o
	$(CC) $(CFLAGS) cliMajor.c common.o -o client $(LDFLAGS)

server: svrMajor.c common.o
	$(CC) $(CFLAGS) svrMajor.c common.o  -o server $(LDFLAGS)

clean:
	rm client server common.o
