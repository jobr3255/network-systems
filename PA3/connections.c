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
#include <errno.h>

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
	u_short portno = 80;
	if(temp) {
		portno = (u_short)atoi(temp);
	}
	printf("hostname: %s\n", hostname);
	printf("path: %s\n", path);
	printf("port: %d\n", portno);

	if(!isWhitelisted(hostname)){
		printf("%s is not whitelisted. Send 403 error\n", hostname);
		return 403;
	}
	if(isBlacklisted(hostname)){
		printf("%s is blacklisted. Send 403 error\n", hostname);
		return 403;
	}
	struct hostent *server;
	int sockfd;
	struct sockaddr_in sockfdaddr;

	/* connect to sockfd */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 400;
	}
	server = gethostbyname(hostname);
	if(!server) {
		printf("Host %s not found\n", hostname);
		switch(h_errno) {
			case HOST_NOT_FOUND:
				printf("Host could not be identified by the DNS sockfd\n");
				break;
			case TRY_AGAIN:
				printf("Problem with the DNS sockfd. Try again\n");
				break;
			default:
				printf("Unable to resolve this host name to an IP address\n");
		}
		// Hostname failed, send Bad request error
		return 400;
	}
	printf("gethostbyname(%s)=%d.%d.%d.%d", hostname, (unsigned char)server->h_addr[0],(unsigned char)server->h_addr[1],(unsigned char)server->h_addr[2],(unsigned char)server->h_addr[3]);

	printf("Trying to connect to %s:%d...\n",hostname, portno);
	sockfdaddr.sin_family = AF_INET;
	bcopy(server->h_addr,&sockfdaddr.sin_addr,sizeof(struct in_addr)) ;
	sockfdaddr.sin_port = htons(portno) ;
	if(connect(sockfd, (struct sockaddr *) &sockfdaddr, sizeof(struct sockaddr_in)) < 0) {
		perror("connect");
		switch(errno) {
			case ECONNREFUSED:
				printf("Connection refused\n");
				break;
			case ETIMEDOUT:
				printf("Connection timed out\n");
				break;
			case EHOSTUNREACH:
			case ENETUNREACH:
				printf("Unreachable\n");
				break;
			case EHOSTDOWN:
			case ENETDOWN:
				printf("Sorry, this host is down\n");
				break;
			default:
				printf("Host is not responding\n");
		}
		return 0;
	}

	printf("Connected to %s:%d\n", hostname, portno);

	// Build HTTP request and store it in tempReq
	char tempReq[655360];
 	memset(tempReq, 0, BUFSIZE);
	int pos = 0;
	pos += sprintf(&tempReq[pos], "GET %s %s\n%s", path, version, request);
	char *req;
	req = (char *) malloc(pos);
	memcpy(req, tempReq, pos);
	// printf("pos: %d Request: \n%s\n", pos, req);
	int error = send(sockfd, req, pos, 0);
	free(req);
	if (error < 0) {
		perror("Error sending to sockfd");
		close(fd);
		exit(4);
	}
	// printf("sent request %d\n", error);

	char buf[BUFSIZE];
 	memset(buf, 0, BUFSIZE);
	int n;
  if ((n = recv(sockfd, buf, BUFSIZE,0)) == 0){
   //error: sockfd terminated prematurely
   perror("The sockfd terminated prematurely");
	 switch(errno) {
		 case ECONNREFUSED:
			 printf("Connection refused\n");
			 break;
		 case EFAULT:
			 printf("The receive buffer pointer(s) point outside the process's address space.\n");
			 break;
		 case ENOTCONN:
			 printf("The socket is associated with a connection-oriented protocol and has not been connected\n");
			 break;
		 case ENOTSOCK:
			 printf("The file descriptor sockfd does not refer to a socket\n");
			 break;
		 default:
			 printf("Recieved no bytes\n");
			 break;
	 }
	 close(fd);
   exit(5);
  }
  // printf("n: %d Received: \n%s\n", strlen(buf), buf);
  printf("Received: %d bytes\n", strlen(buf));

	char *response;
	n = strlen(buf);
	response = (char *) malloc(n);
	memcpy(response, buf, n);
	// printf("Response: \n%s\n", response);
	error = send(fd, response, n, 0);
	free(response);
	// send(fd, buf, n, 0);
	return error;
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
