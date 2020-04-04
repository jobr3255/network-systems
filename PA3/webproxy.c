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

#define MAX_CONNECTIONS 10
void *getInAddr(struct sockaddr *sa);
int getListenerSocket(char *port);

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

	int newfd;
	struct sockaddr_storage clientAddrInfo;	// connector's address information
	char clientIP[INET6_ADDRSTRLEN];
	memset(clientIP, 0, INET6_ADDRSTRLEN);

	// Get a listening socket
	int listenfd = getListenerSocket(port);
	pid_t pid;

	if (listenfd < 0) {
		fprintf(stderr, "webproxy: fatal error getting listening socket\n");
		exit(1);
	}

	printf("webproxy: waiting for connections on port %s...\n", port);

	while(1) {
		socklen_t sin_size = sizeof clientAddrInfo;
		// newfd is a new socket descriptor for the new connection.
		newfd = accept(listenfd, (struct sockaddr *)&clientAddrInfo, &sin_size);
		if (newfd == -1) {
			perror("accept");
			continue;
		}
		// Print out a message that we got the connection
		inet_ntop(clientAddrInfo.ss_family,
							getInAddr((struct sockaddr *)&clientAddrInfo),
							clientIP, sizeof clientIP);
		// printf("server: got connection from %s\n", clientIP);
		// handleRequest(newfd);
		if ( (pid = fork()) == 0 ) {
			// struct sockaddr_storage clientAddrInfoTmp;	// connector's address information
			// char clientIPTmp[INET6_ADDRSTRLEN];
			// memset(clientIPTmp, 0, INET6_ADDRSTRLEN);
			// memcpy(clientIPTmp, clientIP, sizeof clientIP);
			// memcpy(&clientAddrInfoTmp, &clientAddrInfo, sizeof clientAddrInfo);
			handleRequest(newfd);
      close(newfd);
      exit(0); // child terminates
    }
		close(newfd);
	}

	// Unreachable code

	return 0;
}

/**
 * This gets an Internet address, either IPv4 or IPv6
 *
 * Helper function to make printing easier.
 */
void *getInAddr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * Return the main listening socket
 *
 * Returns -1 or error
 */
int getListenerSocket(char *port)
{
	int sockfd;
	struct addrinfo hints, *serverInfo, *potential;
	int yes = 1;
	int errorNo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;	// IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP (SOCK_STREAM)
	hints.ai_flags = AI_PASSIVE;	// use my IP
	if ((errorNo = getaddrinfo(NULL, port, &hints, &serverInfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errorNo));
		return -1;
	}

	/*
	 * Loop through potential interfaces and try to set up a socket on each.
	 * Quit looping on first success
	 */
	for(potential = serverInfo; potential != NULL; potential = potential->ai_next) {
		// Try to make a socket based on this candidate interface
		sockfd = socket(potential->ai_family, potential->ai_socktype, potential->ai_protocol);
		if (sockfd == -1) {
			// failed so try next potential
			continue;
		}

		/* setsockopt: Handy debugging trick that lets
		 * us rerun the server immediately after we kill it;
		 * otherwise we have to wait about 20 secs.
		 * Eliminates "ERROR on binding: Address already in use" error.
		 */
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			close(sockfd);
			freeaddrinfo(serverInfo);	// free serverInfo
			return -2;
		}

		/*
		 * Try to bind this socket to this local IP address.
		 */
		if (bind(sockfd, potential->ai_addr, potential->ai_addrlen) == -1) {
			close(sockfd);
			//perror("server: bind");
			continue;
		}
		// If we got here, we got a bound socket and we're done
		break;
	}
	// free serverInfo
	freeaddrinfo(serverInfo);

	// If potential is NULL, we don't have a good socket.
	if (potential == NULL)  {
		fprintf(stderr, "webserver: failed to find local address\n");
		return -3;
	}

	// Start listening
	if (listen(sockfd, MAX_CONNECTIONS) == -1) {
		// failed to listen
		close(sockfd);
		return -4;
	}
	return sockfd;
}
