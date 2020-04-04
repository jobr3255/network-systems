CC=gcc
CFLAGS=-g -Wall -Wextra

OBJS=webproxy.o connections.o helpers.o server-connections.o

all: webproxy

webproxy: $(OBJS)
	gcc -o $@ $^

webproxy.o: webproxy.c connections.h

connections.o: connections.c connections.h

server-connections.o: server-connections.c server-connections.h

helpers.o: helpers.c helpers.h

clean:
	rm -f $(OBJS)
	rm -f webproxy
