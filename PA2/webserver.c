/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "connections.h"

int main(int argc, char **argv) {
	/*
	 * check command line arguments
	 */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	char *port;
	port = argv[1];

	int newfd;			// listen on sock_fd, new connection on newfd
	struct sockaddr_storage clientAddrInfo;			// connector's address information
	char clientIP[INET6_ADDRSTRLEN];

	// Get a listening socket
	int listenfd = getListenerSocket(port);

	if (listenfd < 0) {
		fprintf(stderr, "webserver: fatal error getting listening socket\n");
		exit(1);
	}

	printf("webserver: waiting for connections on port %s...\n", port);

	while(1) {
		socklen_t sin_size = sizeof clientAddrInfo;

		// accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);
		newfd = accept(listenfd, (struct sockaddr *)&clientAddrInfo, &sin_size);
		if (newfd == -1) {
			perror("accept");
			continue;
		}

		// Print out a message that we got the connection
		inet_ntop(clientAddrInfo.ss_family,
							getInAddr((struct sockaddr *)&clientAddrInfo),
							clientIP, sizeof clientIP);
		printf("server: got connection from %s\n", clientIP);

		// newfd is a new socket descriptor for the new connection.
		handleRequest(newfd);

		close(newfd);
	}

	// Unreachable code

	return 0;
}
