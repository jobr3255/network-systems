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
		int error = handleGet(fd, url, version, request);
		if(error == 400){
			sendError400(fd, version);
		}else if(error == 403){
			sendError403(fd, version);
		}
	}else{
		printf("[ERROR] Bad Request: %s %s\n", method, url);
		sendError400(fd, version);
	}
}

int handleGet(int fd, char *url, char *version, char *request) {
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
		return 400;
	}
	// printf("hostname: %s\n", server->h_addr_list[0]);

	if(!isWhitelisted(hostname)){
		printf("%s is not whitelisted. Send 403 error\n", hostname);
		return 403;
	}
	if(isBlacklisted(hostname)){
		printf("%s is blacklisted. Send 403 error\n", hostname);
		return 403;
	}

	// Create socket for proxy connection to host
	int sockfd;
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
		perror("Problem in creating the socket");
		close(fd);
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
		close(fd);
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
		close(fd);
		exit(4);
	}

	// send(sockfd, request, strlen(request), 0);
	char buf[6553600];
 	memset(buf, 0, BUFSIZE);
	int n;
  if ((n = recv(sockfd, buf, BUFSIZE,0)) == 0){
   //error: server terminated prematurely
   perror("The server terminated prematurely");
	 close(fd);
   exit(5);
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
	return 0;
}

/**
 *  Send a 400 Bad Request response to client
 */
void sendError400(int fd, char *version) {
	char tmp[BUFSIZE];
	int pos = 0;
	pos += sprintf(tmp, "%s 400 Bad Request", version);
	char *header;
	header = (char *) malloc(pos);
	memcpy(header, &tmp[0], pos);
	int error = sendResponseToClient(fd, header, "text/plain", "Bad Request", 11);
	if (error < 0) {
		perror("Error sending to client");
	}
	free(header);
}

/**
 *  Send a 403 Forbidden response to client
 */
void sendError403(int fd, char *version) {
	char tmp[BUFSIZE];
	int pos = 0;
	pos += sprintf(tmp, "%s 403 Forbidden", version);
	char *header;
	header = (char *) malloc(pos);
	memcpy(header, &tmp[0], pos);
	int error = sendResponseToClient(fd, header, "text/plain", "Forbidden", 9);
	if (error < 0) {
		perror("Error sending to client");
	}
	free(header);
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
	pos += sprintf(tempResponse, "%s\r\n", header);
	pos += sprintf(&tempResponse[pos], "Content-Type: %s\r\n", contentType);
	pos += sprintf(&tempResponse[pos], "Content-Length: %d\r\n\r\n", contentLength);
	int responseLength = pos + contentLength;
	if(contains(contentType, "text")) {
		pos += sprintf(&tempResponse[pos], "%s", body);
		responseLength = pos;
	}else{
		memcpy(&tempResponse[pos], body, contentLength);
	}
	char *response;
	response = (char *) malloc(responseLength);
	memcpy(response, &tempResponse[0], responseLength);
	int error = send(fd, response, responseLength, 0);
	free(response);
	return error;
}

/**
 * Check if hostname is whitelisted. Returns 1 if whitelist file does not exist
 */
int isWhitelisted(char *hostname){
	if(fileExists("whitelist") ) {
		return isInFile(hostname, "whitelist");
	} else {
		// If no file exists then return true
		// printf("Whitelist file does not exist\n");
		return 1;
	}
	return 0;
}

/**
 * Check if hostname is blacklisted. Returns 0 if blacklist file does not exist
 */
int isBlacklisted(char *hostname){
	if(fileExists("blacklist") ) {
		return isInFile(hostname, "blacklist");
	} else {
		// If no file exists then return false
		// printf("Blacklist file does not exist\n");
		return 0;
	}
	return 0;
}
