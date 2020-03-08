#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "connections.h"
#include "helpers.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONNECTIONS 10
#define BUFSIZE 65535
#define SERVER_FILES "www"

/**
 * Handle HTTP request and send response
 */
void handleRequest(int fd) {
	char request[BUFSIZE];
  memset(request, 0, BUFSIZE);
	// Read request
	int bytesRecvd = recv(fd, request, BUFSIZE - 1, 0);
	if (bytesRecvd < 0) {
		perror("recv");
		return;
	}
	// printf("%s\n", request);
	char tmpReq[BUFSIZE];
  memset(tmpReq, 0, BUFSIZE);
	memcpy(tmpReq, request, strlen(request));
	char delim[] = "\n";
	char *temp;
	char method[BUFSIZE], uri[BUFSIZE], version[BUFSIZE];
  memset(method, 0, BUFSIZE);
  memset(uri, 0, BUFSIZE);
  memset(version, 0, BUFSIZE);
	temp = strtok (request, delim);
	printf("%s\n", temp);
	extractCommand(temp, method, uri, version);

	if(strcmp(method,"GET") == 0) {
		handleGet(fd, uri, version);
	}else if(strcmp(method,"POST") == 0) {
		char data[BUFSIZE];
    memset(data, 0, BUFSIZE);
		extractPostData(tmpReq, data);
		handlePost(fd, version, data);
	}else{
		printf("[ERROR]: Received unknown request: %s\n\n", method);
		sendError(fd, version);
	}
}

void handleGet(int fd, char *uri, char *version) {
	char path[BUFSIZE];
	if(strcmp(uri,"/") == 0) {
		sprintf(&path[0], "%s/index.html", SERVER_FILES);
	}else{
		sprintf(&path[0], "%s%s", SERVER_FILES, uri);
	}
	printf("[GET] %s\n\n", path);
	if( fileExists(path) ) {
		FILE *file;
		file = fopen(path,"rb");
		int size = getFileSize(file);
		// printf("file size: %d\n", size);
		// Read binary
		char ch;
		char buf[size];
		for (int i = 0; i < size; i++) {
			ch = fgetc(file);
			buf[i] = ch;
		}
		fclose(file);
		// Build header
		char header[16];
		memcpy( header, &version[0], 8);
		memcpy( &header[8], " 200 Ok ", 8);

		char *type;
		type = getMimeType(path);
		// Send response
		sendResponse(fd, header, type, buf, size);
	} else {
		printf("[ERROR] File '%s' does not exist on server. Sent error\n", uri);
		if(contains(path, "htm")) {
			sendError404Page(fd, version);
		}else{
			sendError404(fd, version);
		}
	}
}

void handlePost(int fd, char *version, char* data) {
	printf("[POST]: %s\n\n", data);
	char tempResponse[BUFSIZE];
	int pos = 0;
	pos += sprintf(&tempResponse[pos], "<html>\n<body>\n<h1>Post Data</h1>\n<pre>%s</pre>\n</body>\n</html>", data);
  char response[pos];
	memcpy(response, &tempResponse[0], pos);
  // Build header
  char header[16];
  memcpy( header, &version[0], 8);
  memcpy( &header[8], " 200 Ok ", 8);
  sendResponse(fd, header, "text/html; charset=UTF-8", response, pos);
}

void sendError(int fd, char *version) {
	// HTTP/1.X 500 Internal Server Error
	char *message = "Internal Server Error\n";
	char tmp[40];
	memcpy( tmp, &version[0], 8);
	memcpy( &tmp[8], " 500 Internal Server Error", 26);
	char header[34];
	memcpy(header, tmp, 34);
	sendResponse(fd, header, "text/plain", message, strlen(message));
}

void sendError404(int fd, char *version) {
	char *message = "Not Found\n";
	char tmp[40];
	memcpy( tmp, &version[0], 8);
	memcpy( &tmp[8], " 404 Not Found ", 16);
	char header[24];
	memcpy(header, tmp, 24);
	sendResponse(fd, header, "text/plain", message, strlen(message));
}

void sendError404Page(int fd, char *version) {
	char path[BUFSIZE];
	sprintf(&path[0], "%s/404.html", SERVER_FILES);
	if( fileExists(path) ) {
		FILE *file;
		file = fopen(path,"rb");
		int size = getFileSize(file);
		// Read binary
		char ch;
		char buf[size];
		for (int i = 0; i < size; i++) {
			ch = fgetc(file);
			buf[i] = ch;
		}
		fclose(file);

		char header[24];
		memcpy( header, &version[0], 8);
		memcpy( &header[8], " 404 Not Found ", 16);

		char *type;
		type = getMimeType(path);
		sendResponse(fd, header, type, buf, size);
	} else {
		printf("[ERROR] Sent 500 error\n");
		sendError(fd, version);
	}
}


/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * contentType: "text/plain", etc.
 * body:         the data to send.
 * contentLength: body length
 */
int sendResponse(int fd, char *header, char *contentType, void *body, int contentLength) {

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
	char response[responseLength];
	memcpy(response, &tempResponse[0], pos);
	// printf ("body size: %ld total size: %ld\n", contentLength, responseLength);
	// printf("Content-Type: %s\n", contentType);
	// Send it all!
	int error = send(fd, response, responseLength, 0);
	if (error < 0) {
		perror("send");
	}
	return error;
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
