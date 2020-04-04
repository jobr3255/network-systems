#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "client-connections.h"
#include "helpers.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 65535

struct ClientConnectionInfo createClientConnection(int portno, int sockfd) {
	struct ClientConnectionInfo connection;
	connection.portno = portno;
	connection.sockfd = sockfd;

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	connection.optval = 1;
	setsockopt(connection.sockfd, SOL_SOCKET, SO_REUSEADDR,
						 (const void *)&connection.optval, sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &connection.serveraddr, sizeof(connection.serveraddr));
	connection.serveraddr.sin_family = AF_INET;
	connection.serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	connection.serveraddr.sin_port = htons((unsigned short)connection.portno);

	/*
	 * bind: associate the parent socket with a port
	 */
	if (bind(connection.sockfd, (struct sockaddr *) &connection.serveraddr,
					 sizeof(connection.serveraddr)) < 0)
		error("ERROR on binding");
	return connection;
}

/**
 *  Sends buffer to client
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 */
// void sendToClient(struct ClientConnectionInfo connection, void *buf, int size) {
// 	// printf("Response size: %ld\n", strlen(buf));
// 	connection.n = sendto(connection.sockfd, buf, size, 0,
// 												(struct sockaddr *) &connection.clientaddr, connection.clientlen);
// 	if (connection.n < 0)
// 		error("ERROR in sendto");
// 	// printf("buf: '%s', %ld\n", buf, strlen(buf));
// 	memset(buf,0, BUFSIZE);
// }



/**
 *  Send a 400 Bad Request response to client
 */
// void sendError400(int fd, char *version) {
// 	printf("fd %d\n", fd);
// 	printf("sending 400 to client\n");
// 	char *message = "Bad Request\n";
// 		printf("here\n");
// 	char tmp[42];
// 	memcpy( tmp, &version[0], 8);
// 		printf("here\n");
// 	memcpy( &tmp[8], " 400 Bad Request ", 18);
// 	char header[24];
// 	memcpy(header, tmp, 24);
// 		printf("here\n");
// 	sendResponseToClient(fd, header, "text/plain", message, strlen(message));
// 		printf("here\n");
// 	// close(fd);
// 	// exit(1);
// }
//
// /**
//  *  Send a 403 Forbidden response to client
//  */
// void sendError403(int fd, char *version) {
// 	printf("sending 403 to client\n");
// 	char *message = "Forbidden\n";
// 	char tmp[40];
// 	memcpy( tmp, &version[0], 8);
// 	memcpy( &tmp[8], " 403 Forbidden ", 16);
// 	char header[24];
// 	memcpy(header, tmp, 24);
// 	sendResponseToClient(fd, header, "text/plain", message, strlen(message));
// 	// close(fd);
// 	// exit(1);
// }
//
// /**
//  * Send an HTTP response
//  *
//  * header:       "HTTP/1.1 400 Bad Request" or "HTTP/1.1 200 Ok", etc.
//  * contentType: "text/plain", etc.
//  * body:         the data to send.
//  * contentLength: body length
//  */
// int sendResponseToClient(int fd, char *header, char *contentType, void *body, int contentLength) {
// 	printf("sending to client\n");
// 	// Build HTTP response and store it in tempResponse
// 	char tempResponse[6553600];
// 	int pos = 0;
// 	pos += sprintf(&tempResponse[pos], "%s\r\n", header);
// 	pos += sprintf(&tempResponse[pos], "Content-Type: %s\r\n", contentType);
// 	pos += sprintf(&tempResponse[pos], "Content-Length: %d\r\n\r\n", contentLength);
// 	// pos += sprintf(&tempResponse[pos], "Connection: %s\r\n", "Closed");
// 	// pos += sprintf(&tempResponse[pos], "\r\n%s", body);
// 	int responseLength = pos + contentLength;
// 	if(contains(contentType, "text")) {
// 		pos += sprintf(&tempResponse[pos], "%s", body);
// 		// printf ("pos: %d responseLength: %d\n", pos, responseLength);
// 		responseLength = pos;
// 	}else{
// 		// responseLength = pos + contentLength;
// 		memcpy(&tempResponse[pos], body, contentLength);
// 	}
//
// 	// Copy tempResponse into response which has the correct size to fit all the data
// 	// Solves "Excess found in a non pipelined read" error
// 	// char response[responseLength];
// 	char *response;
// 	response = (char *) malloc(responseLength);
// 	memcpy(response, &tempResponse[0], responseLength);
// 	// strcpy(response, tempResponse);
// 	// printf ("body size: %ld total size: %ld\n", contentLength, responseLength);
// 	// printf("Content-Type: %s\n", contentType);
// 	// Send it all!
// 	int error = send(fd, response, responseLength, 0);
// 	free(response);
// 	if (error < 0) {
// 		perror("send");
// 	}
// 	return error;
// }
