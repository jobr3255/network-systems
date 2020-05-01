CC=gcc
CFLAGS=-g -Wall -Wextra

all:
	gcc -g dfc.c -o dfc -lcrypto -lm
	gcc -g dfs.c -o dfs -lpthread

clean:
	$(RM) dfc dfs
