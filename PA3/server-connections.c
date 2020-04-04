#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "server-connections.h"
#include "helpers.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 65535

/***************************
   Helper functions to create connection to server
***************************/

/* socket: create the socket */
void createServerSocket(struct ServerConnectionInfo *connection) {
	connection->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connection->sockfd < 0)
		error("ERROR opening socket");
}
/* gethostbyname: get the server's DNS entry */
void setServerDNSEntry(struct ServerConnectionInfo *connection) {
	connection->server = gethostbyname(connection->hostname);
	if (connection->server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", connection->hostname);
		exit(0);
	}
}
/* build the server's Internet address */
void buildServerInternetAddress(struct ServerConnectionInfo *connection) {
	bzero((char *) &connection->serveraddr, sizeof(connection->serveraddr));
	connection->serveraddr.sin_family = AF_INET;
	bcopy((char *)connection->server->h_addr,
				(char *)&connection->serveraddr.sin_addr.s_addr, connection->server->h_length);
	connection->serveraddr.sin_port = htons(connection->portno);
}

/**
 *  @param char *hostname
 *      Hostname of server
 *  @param int portno
 *      Port number to connect on
 *  @return int
 */
struct sockaddr_in createServerConnection(int sockfd, char *hostname, int portno) {
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr= inet_addr(hostname);
	serveraddr.sin_port = htons(portno);

	//Connection of the client to the socket
	if (connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		perror("Problem in connecting to the server");
		exit(3);
	}
	return serveraddr;
}

/**
 *  Sends request to server
 *
 *  @param char *url
 *      HTTP request url
 *  @param char *version
 *      HTTP request version
 *  @param char *request
 *      Request minus the first line
 */
int sendToServer(int sockfd, char *path, char *version, char *request) {
	// Build HTTP response and store it in tempResponse
	char tempReq[BUFSIZE];
	int pos = 0;
	pos += sprintf(&tempReq[pos], "GET %s %s\n%s", path, version, request);

	char *req;
	req = (char *) malloc(pos);
	memcpy(req, tempReq, pos);
	printf("pos: %d Request: \n%s\n", pos, req);
	free(req);

	return send(sockfd, req, pos, 0);
}

/**
 *  Sends buffer to server
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
// void getServerResponse(struct ServerConnectionInfo connection) {
// 	char buf[BUFSIZE];
// 	memset(buf,0, BUFSIZE);
// 	// printf("buf: '%s', %ld \n", buf, strlen(buf));
// 	int n = recvfrom(connection.sockfd, buf, BUFSIZE, 0, &connection.serveraddr, &connection.serverlen);
// 	if (n < 0)
// 		error("ERROR in recvfrom");
// 	// printf("buf: '%s', %ld \n", buf, strlen(buf));
// 	printf("[Server]: %s \n", buf);
// 	memset(buf,0, BUFSIZE);
// }

/**
 *  Sends get request to server and processes response
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param char *variable
 *      Variable to go with command
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
// void getFromServer(struct ServerConnectionInfo connection, char *buf, char *variable) {
// 	if(strlen(variable) == 0) {
// 		printf("'get' needs a variable!\n");
// 		return;
// 	}
// 	sendToServer(connection, buf, strlen(buf));
// 	/* print the server's reply */
// 	int n = recvfrom(connection.sockfd, buf, BUFSIZE, 0, &connection.serveraddr, &connection.serverlen);
// 	if (n < 0)
// 		error("ERROR in recvfrom");
//
// 	if(contains(buf, "ERROR")) {
// 		printf("%s \n", buf);
// 		return;
// 	}
// 	printf("Successfully received '%s' \n", variable);
// }
