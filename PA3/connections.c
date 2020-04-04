#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "server-connections.h"
#include "connections.h"
#include "helpers.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 65535
// 6553600

/**
 * Handle HTTP request and send response
 */
void handleRequest(int fd) {
	char clientRequest[BUFSIZE];
	memset(clientRequest, 0, BUFSIZE);
	// Read request
	int bytesRecvd = recv(fd, clientRequest, BUFSIZE - 1, 0);
	if (bytesRecvd < 0) {
		perror("recv");
		return;
	}
	// printf("bytesRecvd: %d clientRequest: \n%s", bytesRecvd, clientRequest);

	// printf("Request %s\n", request);
	char tmpReq[BUFSIZE];
	memset(tmpReq, 0, BUFSIZE);
	memcpy(tmpReq, clientRequest, strlen(clientRequest));
	char delim[] = "\r\n";
	char *temp;
	char method[BUFSIZE], url[BUFSIZE], version[BUFSIZE];
	memset(method, 0, BUFSIZE);
	memset(url, 0, BUFSIZE);
	memset(version, 0, BUFSIZE);
	// extract first line
	temp = strtok (tmpReq, delim);
	// printf("TEMP: %s\n", temp);
	extractCommand(temp, method, url, version);

	char buildReq[BUFSIZE];
	memset(buildReq, 0, BUFSIZE);
	int pos = 0;
	// printf("before loop\n");
	char *temp2;
	temp2 = strtok (clientRequest, delim);
	while (temp2 != NULL) {
		// printf("%s \n", temp);
		temp2 = strtok (NULL, delim);
		if(temp2 != NULL)
			pos += sprintf(&buildReq[pos], "%s\n", temp2);
	}
	// printf("after loop\n");
	char request[pos];
	memcpy(request, buildReq, pos);
	// printf("pos: %d Request: \n%s\n", pos, request);

	if(strcmp(method,"GET") == 0) {
		printf("%s\n", temp);
		handleGet(fd, url, version, request);
	}else{
		printf("[ERROR] Bad Request: %s %s\n", method, url);
		sendError400(fd, version);
	}
}

void handleGet(int fd, char *url, char *version, char *request) {
	// printf("url: %s\n", url);
	char hostname[BUFSIZE], tmpPath[BUFSIZE], path[BUFSIZE];
	memset(hostname, 0, BUFSIZE);
	memset(tmpPath, 0, BUFSIZE);
	char *temp;
	char tempURL[BUFSIZE];
	memset(tempURL, 0, BUFSIZE);
	memcpy(tempURL, url, strlen(url));
	unsigned tmpSize = 0;
	temp = strtok (tempURL, "//");
	tmpSize = tmpSize + strlen(temp) + 2;
	temp = strtok (NULL, "/");
	tmpSize = tmpSize + strlen(temp) + 1;
	memcpy(hostname, temp, strlen(temp));
	if(tmpSize < strlen(url)) {
		temp = strtok (NULL, "");
		memcpy(tmpPath, temp, strlen(temp));
	}
	sprintf(&path[0], "/%s", tmpPath);
	temp = strtok (hostname, ":");
	temp = strtok (NULL, "");
	int portno = 80;
	if(temp) {
		portno = atoi(temp);
	}
	printf("hostname: %s\n", hostname);
	printf("path: %s\n", path);
	printf("port: %d\n", portno);

	// Check that the requested URL exists
	struct hostent *server;
	server = gethostbyname(hostname);
	printf("hostname: %s\n", server->h_name);
	// printf("hostname: %s\n", server->h_addr);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", url);
		sendError400(fd, version);
		return;
	}
	// printf("hostname: %s\n", server->h_addr_list[0]);

	// Create socket for proxy connection to host
	int sockfd;
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		perror("Problem in creating the socket");
		exit(2);
	}

  // Set host connection hostname and port
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr= inet_addr(server->h_name);
	serveraddr.sin_port = htons(portno);

	//Connection of the client to the socket
	if (connect(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr))<0) {
		perror("Problem in connecting to the server");
		exit(3);
	}

	// Build HTTP response and store it in tempResponse
	char tempReq[BUFSIZE];
	int pos = 0;
	pos += sprintf(&tempReq[pos], "GET %s %s\n%s", path, version, request);

	// Copy tempReq into req which has the correct size to fit all the data
	// Solves "Excess found in a non pipelined read" error
	// char response[responseLength];
	// char req[pos];
	// memcpy(req, tempReq, pos);

	char *req;
	req = (char *) malloc(pos);
	memcpy(req, tempReq, pos);
	printf("pos: %d Request: \n%s\n", pos, req);
	// strcpy(response, tempResponse);
	// printf ("body size: %ld total size: %ld\n", contentLength, responseLength);
	// printf("Content-Type: %s\n", contentType);
	// Send it all!
	// strcpy(response, tempResponse);
	// printf ("body size: %ld total size: %ld\n", contentLength, responseLength);
	// printf("Content-Type: %s\n", contentType);
	// Send it all!
	int error = send(sockfd, req, pos, 0);
	free(req);
	if (error < 0) {
		perror("Error sending to server");
	}

	// send(sockfd, request, strlen(request), 0);
	char buf[6553600];
 	memset(buf, 0, BUFSIZE);
	int n;
  if ((n = recv(sockfd, buf, BUFSIZE,0)) == 0){
   //error: server terminated prematurely
   perror("The server terminated prematurely");
   exit(4);
  }
  // printf("n: %d String received from the server: \n%s", n, buf);
	send(fd, buf, n, 0);

	// int sockfd = createServerSocket();
	// struct sockaddr_in serveraddr;
	// buildServerInternetAddress(serveraddr, server, portno);
	// int n = sendto(sockfd, buf, size, 0, &serveraddr, sizeof(serveraddr));
	// if (n < 0)
	//  error("ERROR in sendto");
	//
	// while(1){
	//  n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);
	// }
}

void sendError(int fd, char *version) {
	// HTTP/1.X 500 Internal Server Error
	char *message = "Internal Server Error\n";
	char tmp[40];
	memcpy( tmp, &version[0], 8);
	memcpy( &tmp[8], " 500 Internal Server Error", 26);
	char header[34];
	memcpy(header, tmp, 34);
	sendResponseToClient(fd, header, "text/plain", message, strlen(message));
}

void sendError400(int fd, char *version) {
	char *message = "Bad Request\n";
	char tmp[42];
	memcpy( tmp, &version[0], 8);
	memcpy( &tmp[8], " 400 Bad Request ", 18);
	char header[24];
	memcpy(header, tmp, 24);
	sendResponseToClient(fd, header, "text/plain", message, strlen(message));
}

/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 400 Bad Request" or "HTTP/1.1 200 Ok", etc.
 * contentType: "text/plain", etc.
 * body:         the data to send.
 * contentLength: body length
 */
int sendResponseToClient(int fd, char *header, char *contentType, void *body, int contentLength) {
	// Build HTTP response and store it in tempResponse
	char tempResponse[6553600];
	int pos = 0;
	pos += sprintf(&tempResponse[pos], "%s\r\n", header);
	pos += sprintf(&tempResponse[pos], "Content-Type: %s\r\n", contentType);
	pos += sprintf(&tempResponse[pos], "Content-Length: %d\r\n\r\n", contentLength);
	// pos += sprintf(&tempResponse[pos], "Connection: %s\r\n", "Closed");
	// pos += sprintf(&tempResponse[pos], "\r\n%s", body);
	int responseLength = pos + contentLength;
	if(contains(contentType, "text")) {
		pos += sprintf(&tempResponse[pos], "%s", body);
		// printf ("pos: %d responseLength: %d\n", pos, responseLength);
		responseLength = pos;
	}else{
		// responseLength = pos + contentLength;
		memcpy(&tempResponse[pos], body, contentLength);
	}

	// Copy tempResponse into response which has the correct size to fit all the data
	// Solves "Excess found in a non pipelined read" error
	// char response[responseLength];
	char *response;
	response = (char *) malloc(responseLength);
	memcpy(response, &tempResponse[0], responseLength);
	// strcpy(response, tempResponse);
	// printf ("body size: %ld total size: %ld\n", contentLength, responseLength);
	// printf("Content-Type: %s\n", contentType);
	// Send it all!
	int error = send(fd, response, responseLength, 0);
	free(response);
	if (error < 0) {
		perror("send");
	}
	return error;
}
