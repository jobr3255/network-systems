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

#include "helpers.h"


struct ClientConnectionInfo createClientConnection(char **argv);
void sendToClient(struct ClientConnectionInfo connection, void *buf, int size);
void processGet(struct ClientConnectionInfo connection, char *variable);
void processPut(struct ClientConnectionInfo connection, char *buf, char *variable);
void processDelete(struct ClientConnectionInfo connection, char *variable);
void processLS(struct ClientConnectionInfo connection);
void processExit(struct ClientConnectionInfo connection);
void invalidCommand(struct ClientConnectionInfo connection, char *command);

int main(int argc, char **argv) {
	struct ClientConnectionInfo connection;
	char buf[BUFSIZE];
	char command[BUFSIZE];
	char variable[BUFSIZE];
	/*
	 * check command line arguments
	 */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	connection = createClientConnection(argv);

	/*
	 * main loop: wait for a datagram, then echo it
	 */
	connection.clientlen = sizeof(connection.clientaddr);
	while (1) {
		// Clear variables
		memset(buf,0, BUFSIZE);
		memset(command,0, BUFSIZE);
		memset(variable,0, BUFSIZE);
		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		bzero(buf, BUFSIZE);
		connection.n = recvfrom(connection.sockfd, buf, BUFSIZE, 0,
														(struct sockaddr *) &connection.clientaddr, &connection.clientlen);
		if (connection.n < 0)
			error("ERROR in recvfrom");

		/*
		 * gethostbyaddr: determine who sent the datagram
		 */
		connection.hostp = gethostbyaddr((const char *)&connection.clientaddr.sin_addr.s_addr,
																		 sizeof(connection.clientaddr.sin_addr.s_addr), AF_INET);
		if (connection.hostp == NULL)
			error("ERROR on gethostbyaddr");
		connection.hostaddrp = inet_ntoa(connection.clientaddr.sin_addr);
		if (connection.hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");

		// printf("server received datagram from %s (%s)\n", connection.hostp->h_name, connection.hostaddrp);
		// printf("server received %d/%d bytes: %s\n", strlen(buf), connection.n, buf);

		extractCommandAndVariable(buf, command, variable);
		// printf("command: '%s' variable: '%s' \n", command, variable);
		if(strcmp(command, "get") == 0) {
			processGet(connection, variable);
		}else if(strcmp(command, "put") == 0) {
			processPut(connection, buf, variable);
		}else if(strcmp(command, "delete") == 0) {
			processDelete(connection, variable);
		}else if(strcmp(command, "ls") == 0) {
			processLS(connection);
		}else if(strcmp(command, "exit") == 0) {
			processExit(connection);
		}else {
			invalidCommand(connection, command);
		}
	}
}

struct ClientConnectionInfo createClientConnection(char **argv) {
	struct ClientConnectionInfo connection;
	connection.portno = atoi(argv[1]);

	/*
	 * socket: create the parent socket
	 */
	connection.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (connection.sockfd < 0)
		error("ERROR opening socket");

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
void sendToClient(struct ClientConnectionInfo connection, void *buf, int size) {
	// printf("Response size: %ld\n", strlen(buf));
	connection.n = sendto(connection.sockfd, buf, size, 0,
												(struct sockaddr *) &connection.clientaddr, connection.clientlen);
	if (connection.n < 0)
		error("ERROR in sendto");
	// printf("buf: '%s', %ld\n", buf, strlen(buf));
	memset(buf,0, BUFSIZE);
}

/**
 *  Constructs and sends message to client with passed in variables as msg1 + msgVar + msg2
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *msg1
 *      First half of message
 *  @param char *msgVar
 *      The variable to insert
 *  @param char *msg2
 *      Second half of message
 */
void sendMessageToClient(struct ClientConnectionInfo connection, char *msg1, char *msgVar, char *msg2) {
	char buf[BUFSIZE];
	memset(buf,0, BUFSIZE);
	strcpy(buf, msg1);
	strcat(buf, msgVar);
	strcat(buf, msg2);
	// printf("Buf: %s, %ld\n", buf, strlen(buf));
	sendToClient(connection, buf, strlen(buf));
	memset(buf,0, BUFSIZE);
}

/**
 *  Processes get request
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *variable
 *      The file to get
 */
void processGet(struct ClientConnectionInfo connection, char *variable) {
	if(strlen(variable) == 0) {
		char buf[BUFSIZE] = "ERROR: No variable passed for 'get'";
		sendToClient(connection, buf, strlen(buf));
		memset(buf,0, BUFSIZE);
		return;
	}
	printf("Server received get request for '%s'\n", variable);
	// Check if file exists on server
	if( fileExists(variable) ) {
		FILE *file;
		file = fopen(getFilePath(variable),"rb");
		int size = getFileSize(file);
		char *buf;
		buf = fileToBuffer(file, size);
		fclose(file);
		connection.n = size;
		sendToClient(connection, buf, size);
		printf("Sent file of size %d\n", size);
	} else {
		printf("File '%s' does not exist on server. Sent error\n", variable);
		sendMessageToClient(connection, "ERROR: '", variable, "' does not exist on the server!");
	}
}

/**
 *  Processes put request
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *variable
 *      The file to get
 */
void processPut(struct ClientConnectionInfo connection, char *buf, char *variable) {
	printf("Server received put request for '%s'\n", variable);
	int size = connection.n;
	int index = indexOf(buf, "\n") + 2;	// Remove first newline character
	char subbuff[size];
	memcpy( subbuff, &buf[index], size - index);
	subbuff[size - index] = '\0';
	// printf("Put: '%s'\n", subbuff);
	FILE *file;
	// char *path = getFilePath(variable);
	file = fopen(getFilePath(variable),"wb");
	fwrite(subbuff, 1, size, file);
	fclose(file);
	printf("Successfully received '%s' \n", variable);
	sendMessageToClient(connection, "Successfully received '", variable, "'");
}

/**
 *  Processes delete request
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *variable
 *      The file to get
 */
void processDelete(struct ClientConnectionInfo connection, char *variable) {
	printf("Server received delete request for '%s'\n", variable);
	if(fileExists(variable)) {
		remove(getFilePath(variable));
		printf("Server deleted '%s'\n", variable);
		sendMessageToClient(connection, "Successfully deleted '", variable, "'");
	}else{
		printf("Can't delete '%s' because it doesn't exist\n", variable);
		sendMessageToClient(connection, "Can't delete '", variable, "' because it doesn't exist!");
	}
}

/**
 *  Processes ls request
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *variable
 *      The file to get
 */
void processLS(struct ClientConnectionInfo connection) {
	printf("Server received ls request\n");
	DIR *d;
	struct dirent *dir;
	d = opendir(FILES_DIR);
	char *fileName;
	char buf[BUFSIZE] = "";
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			fileName = dir->d_name;
			if(strcmp(fileName, ".") != 0 && strcmp(fileName, "..") != 0) {
				strcat(buf, fileName);
				strcat(buf, " ");
			}
		}
		closedir(d);
	}
	printf("Files: %s\n", buf);
	sendToClient(connection, buf, strlen(buf));
	memset(buf,0, BUFSIZE);
}

/**
 *  Processes exit request
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *variable
 *      The file to get
 */
void processExit(struct ClientConnectionInfo connection) {
	printf("Server received exit request\n");
	char buf[BUFSIZE];
	strcpy(buf, "Server exited gracefully");
	sendToClient(connection, buf, strlen(buf));
	exit(0);
}

/**
 *  Send invalid command message to client
 *
 *  @param ClientConnectionInfo connection
 *      Client connection info object
 *  @param char *command
 *      The invalid command
 */
void invalidCommand(struct ClientConnectionInfo connection, char *command) {
	printf("Received invalid command '%s'\n", command);
	sendMessageToClient(connection, "Invalid command '", command, "'");
}
