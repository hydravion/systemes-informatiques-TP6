CC=gcc
CFLAGS=-Wall -g --pedantic -std=c99

all: SMTPclient

# common.o: common.c common.h
# 	$(CC) $(CFLAGS) -c -o common.o common.c

SMTPclient.o: SMTPclient.c #common.h
	$(CC) $(CFLAGS) -c -o SMTPclient.o SMTPclient.c

SMTPclient: SMTPclient.o #common.o
	$(CC) $(CFLAGS) -o SMTPclient SMTPclient.o #common.o

clean:
	rm -rf *.o SMTPclient
