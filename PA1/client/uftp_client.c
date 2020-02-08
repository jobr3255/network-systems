/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "helpers.h"


void menu(char *buf);

struct ServerConnectionInfo createConnection(char **argv);

// void sendToServer(char *buf, struct ServerConnectionInfo connection);
void getFromServer(struct ServerConnectionInfo connection, char *buf, char *variable);
void putToServer(struct ServerConnectionInfo connection, char *buf, char *variable);
void deleteFromServer(struct ServerConnectionInfo connection, char *buf, char *variable);
void listServerFiles(struct ServerConnectionInfo connection, char *buf);
void exitServer(struct ServerConnectionInfo connection, char *buf);


int main(int argc, char **argv) {
	struct ServerConnectionInfo connection;
	char buf[BUFSIZE];
	char command[BUFSIZE];
	char variable[BUFSIZE];
	/* check command line arguments first */
	if (argc != 3) {
		fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	connection = createConnection(argv);

	while(true) {
		// clear variables
		memset(buf,0, BUFSIZE);
		memset(command,0, BUFSIZE);
		memset(variable,0, BUFSIZE);
		menu(buf);
		extractCommandAndVariable(buf, command, variable);
		// printf("buf: '%s' command: '%s' variable: '%s' \n", buf, command, variable);
		if(strcmp(command, "get") == 0) {
			getFromServer(connection, buf, variable);
		}else if(strcmp(command, "put") == 0) {
			putToServer(connection, buf, variable);
		}else if(strcmp(command, "delete") == 0) {
			deleteFromServer(connection, buf, variable);
		}else if(strcmp(command, "ls") == 0) {
			listServerFiles(connection, buf);
		}else if(strcmp(command, "exit") == 0) {
			exitServer(connection, buf);
		}else {
			fprintf(stderr, "Invalid command '%s'\n", command);
		}
	}
	return 0;
}

/**
 *  Displays main menu and gets user input
 *
 *  @param char *buf
 *      Reference to user input variable
 */
void menu(char *buf) {
	printf("\n************************\n");
	// printf("Commands: get [file_name] | put [file_name] | delete [file_name] | ls | exit");
	bzero(buf, BUFSIZE);
	printf("Enter a command: ");
	fgets(buf, BUFSIZE, stdin);
	printf("************************\n\n");
	// buf[indexOf(buf, "\n")] = '\0'; // remove newline
}

/***************************
   Helper functions to create connection to server
***************************/

/* socket: create the socket */
void createSocket(struct ServerConnectionInfo *connection) {
	connection->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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
 *  Builds the ServerConnectionInfo struct
 *
 *  @param char **argv
 *      Command line arguments
 */
struct ServerConnectionInfo createConnection(char **argv) {
	struct ServerConnectionInfo connection;
	connection.hostname = argv[1];
	connection.portno = atoi(argv[2]);
	createSocket(&connection);
	setServerDNSEntry(&connection);
	buildServerInternetAddress(&connection);
	return connection;
}

/**
 *  Sends buffer to server
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
void sendToServer(struct ServerConnectionInfo connection, char *buf, int size) {
	connection.serverlen = sizeof(connection.serveraddr);
	connection.n = sendto(connection.sockfd, buf, size, 0, &connection.serveraddr, connection.serverlen);
	if (connection.n < 0)
		error("ERROR in sendto");
}

/**
 *  Sends buffer to server
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
void getServerResponse(struct ServerConnectionInfo connection) {
	char buf[BUFSIZE];
	memset(buf,0, BUFSIZE);
	// printf("buf: '%s', %ld \n", buf, strlen(buf));
	int n = recvfrom(connection.sockfd, buf, BUFSIZE, 0, &connection.serveraddr, &connection.serverlen);
	if (n < 0)
		error("ERROR in recvfrom");
	// printf("buf: '%s', %ld \n", buf, strlen(buf));
	printf("[Server]: %s \n", buf);
	memset(buf,0, BUFSIZE);
}

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
void getFromServer(struct ServerConnectionInfo connection, char *buf, char *variable) {
	if(strlen(variable) == 0) {
		printf("'get' needs a variable!\n");
		return;
	}
	sendToServer(connection, buf, strlen(buf));
	/* print the server's reply */
	int n = recvfrom(connection.sockfd, buf, BUFSIZE, 0, &connection.serveraddr, &connection.serverlen);
	if (n < 0)
		error("ERROR in recvfrom");

	if(contains(buf, "ERROR")) {
		printf("%s \n", buf);
		return;
	}

	FILE *fptr;
	fptr = fopen(variable,"wb");
	fwrite(buf, 1, n, fptr);
	fclose(fptr);
	printf("Successfully received '%s' \n", variable);
}

/**
 *  Sends put request to server and processes response
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param char *variable
 *      Variable to go with command
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
void putToServer(struct ServerConnectionInfo connection, char *buf, char *variable) {
	if(strlen(variable) == 0) {
		printf("'put' needs a variable!\n");
		return;
	}
	// sendToServer(buf, connection);
	if( fileExists(variable) ) {
		FILE *file;
		file = fopen(variable,"rb");
	  int size = getFileSize(file);
		// Read binary
		char ch;
	  char fileBuf[size];
	  for (int i = 0; i < size; i++) {
	      ch = fgetc(file);
	      fileBuf[i] = ch;
	  }
		fclose(file);

		strcat(buf, fileBuf);
		sendToServer(connection, buf, size);
		getServerResponse(connection);
	} else {
		printf("ERROR: '%s' does not exist locally\n", variable);
	}
}

/**
 *  Sends delete request to server and processes response
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param char *variable
 *      Variable to go with command
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
void deleteFromServer(struct ServerConnectionInfo connection, char *buf, char *variable) {
	if(strlen(variable) == 0) {
		printf("'delete' needs a variable!\n");
		return;
	}
	sendToServer(connection, buf, strlen(buf));
	getServerResponse(connection);
}

/**
 *  Sends ls request to server and processes response
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
void listServerFiles(struct ServerConnectionInfo connection, char *buf) {
	sendToServer(connection, buf, strlen(buf));
	getServerResponse(connection);
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	char *fileName;
	char tempBuf[BUFSIZE] = "";
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			fileName = dir->d_name;
			if(contains(fileName, "foo")) {
				strcat(tempBuf, fileName);
				strcat(tempBuf, " ");
			}
		}
		closedir(d);
	}
	printf("Local: %s\n", tempBuf);
	memset(tempBuf,0, BUFSIZE);
}

/**
 *  Sends exit request to server and processes response
 *
 *  @param char *buf
 *      Buffer message to send
 *  @param ServerConnectionInfo connection
 *      Server connection info object
 */
void exitServer(struct ServerConnectionInfo connection, char *buf) {
	sendToServer(connection, buf, strlen(buf));
	getServerResponse(connection);
}
