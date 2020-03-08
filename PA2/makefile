CC=gcc
CFLAGS=-g -Wall -Wextra

OBJS=webserver.o connections.o helpers.o

all: webserver

webserver: $(OBJS)
	gcc -o $@ $^

server.o: webserver.c connections.h

connections.o: connections.c connections.h

helpers.o: helpers.c helpers.h

clean:
	rm -f $(OBJS)
	rm -f webserver
