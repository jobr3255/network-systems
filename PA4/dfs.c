
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>// For threads
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

// Global variables
// #define BUFSIZE 1024
#define BUFSIZE 65535
int numAuths;
char *validAuths[100];
char *serverFolder;	// server folder name from command line

// Declare thread function
int getListenerSocket(int port);
void *thread(void *vargp);

// Wrapper for perror
void error(char *msg) {
	perror(msg);
}

int main(int argc, char **argv) {
	char *temp;	// temp variable
	pthread_t tid;// thread id
	int port;	// port number for server (command line)
	int sockfd, *clientfd;// listening socket and client socket
	struct sockaddr_in serveraddr;// server addr
	struct sockaddr_in clientaddr;// client addr
	socklen_t clientlen = sizeof(struct sockaddr_in);	// client address length
	numAuths = 0;

	// Check command line arguments
	if (argc != 3) {
		fprintf(stderr, "usage: %s <serverFolder> <port> &\n", argv[0]);
		exit(1);
	}
	serverFolder = argv[1];
	port = atoi(argv[2]);
	// printf("%d\n", port);

	// Get valid authentications
	char filePath[50];
	sprintf(&filePath[0], "./%s/dfs.conf", serverFolder);
	FILE *fptr = fopen(filePath, "rb");
	if (fptr == NULL) {
		error("Couldn't open server configuration file");
		exit(-1);
	} else {
		while(!feof(fptr)) {
			size_t len = 0;
			getline(&temp, &len, fptr);
			temp[strlen(temp)-1] = '\0';
			validAuths[numAuths] = temp;
			numAuths++;
		}
		fclose(fptr);
	}

	// Build listening socket
	sockfd = getListenerSocket(port);
	printf("Server running on port %d... waiting for connections\n", port);

	// Main loop: wait for connections and handle them when they come in
	while (1) {
		clientfd = malloc(sizeof(int));
		*clientfd = accept(sockfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (clientfd < 0) {
			printf("Server accept connection failed");
		}
		else {
			// Create thread to handle request
			pthread_create(&tid, NULL, thread, clientfd);
		}
	}
}

// Thread function
void * thread(void * vargp) {
	char buf[BUFSIZE];// buffer
	char request[BUFSIZE], clientAuthentcation[BUFSIZE];// request and authentication
	char *command, *file;	// parsed request
	int clientfd;	// connection to client
	int size;	// size of message received
	char *trash, username[100], clientDirectory[100];
	char clientACK[100];

	// Detach thread
	clientfd = *(int *)vargp;
	pthread_detach(pthread_self());
	free(vargp);

	// Receive authentication
	memset(buf, '\0', BUFSIZE);
	size = read(clientfd, buf, BUFSIZE);
	if (size < 0) {
		printf("Error in receiving authentication from client\n");
		exit(-1);
	} else {
		strcpy(clientAuthentcation, buf);
		printf("Authentication: '%s'\n", clientAuthentcation);
	}
	trash = strtok(buf, " ");
	strcpy(username, trash);

	// Validate client authentication
	int validated = 0;
	for (int i=0; i<numAuths; i++) {
		if (strcmp(validAuths[i], clientAuthentcation) == 0) {
			validated = 1;
		}
	}
	if(validated == 0) {
		printf("Client not authorized\n");
		char *message = "Invalid Username/Password.";
		write(clientfd, message, strlen(message));
		close(clientfd);
		return 0;
	}

	// Client authenticated
	printf("Valid authentication\n");
	char *message = "Authenticated";
	write(clientfd, message, strlen(message));

	// Receive request
	memset(buf, '\0', BUFSIZE);
	size = read(clientfd, buf, BUFSIZE);
	if (size < 0) {
		printf("Error in receiving message from client\n");
		exit(-1);
	} else {
		strcpy(request, buf);
		// printf("Request: %s\n", request);
	}

	// Make user directory if needed
	sprintf(clientDirectory, "./%s/%s", serverFolder, username);
	if (mkdir(clientDirectory, 0777) < 0) {
		// printf("Could not make directory for the user %s, it may already exist\n", username);
	}

	// Parse request
	command = strtok(buf, " ");

	// Handle different commands
	if (strcmp(command, "list\n") == 0 || strcmp(command, "list") == 0) {
		printf("List command\n");
		DIR *d;
		struct dirent *dir;
		char dirPath[100];
		sprintf(&dirPath[0], "./%s/%s/", serverFolder, username);
		d = opendir(dirPath);
		char *fileName;
		char buf[BUFSIZE] = "";
		int pos = 0;
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				fileName = dir->d_name;
				if(strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0)
					continue;
				pos += sprintf(&buf[pos], "%s\n", fileName);
			}
			closedir(d);
		}
		// printf("Files: \n%s", buf);
		write(clientfd, buf, sizeof(buf));
	} else if (strcmp(command, "put") == 0) {
		// Get file
		char fileName[50];
		file = strtok(NULL, "\n");
		strcpy(fileName, file);
		printf("Put command, %s. Waiting for primer...\n", fileName);
		write(clientfd, "OK\n", strlen("OK\n"));

		// Get primer from client
		bzero(buf, sizeof(buf));
		size = read(clientfd, buf, BUFSIZE);
		if (size < 0) {
			printf("Error receiving from client\n");
			write(clientfd, "ERROR\n", strlen("ERROR\n"));
		} else {
			// Parse primer
			int piece1, piece2;
			int size1, size2;
			piece1 = atoi(strtok(buf, ","));
			size1 = atoi(strtok(NULL, ","));
			piece2 = atoi(strtok(NULL, ","));
			size2 = atoi(strtok(NULL, ","));
			printf("Primer: %d, %d, %d, %d\n", piece1, size1, piece2, size2);

			printf("Send ACK\n");
			// Return acknowledgement
			write(clientfd, "OK\n", strlen("OK\n"));

			// Receive file pieces
			bzero(buf, sizeof(buf));
			size = read(clientfd, buf, size1);
			// printf("Received: %s\n", buf);
			if (size < 0) {
				printf("Error receiving from client\n");
			} else {
				// Put piece in file
				char filePath[50];
				sprintf(filePath, "./%s/%s/%s.%d", serverFolder, username, fileName, piece1+1);
				FILE *fptr = fopen(filePath, "wb");
				int file_success = 1;
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
					file_success = 0;
				}
				else {
					fwrite(buf, 1, size1, fptr);
				}
				fclose(fptr);
			}
			bzero(buf, sizeof(buf));
			size = read(clientfd, buf, size2);
			if (size < 0) {
				printf("Error receiving from client\n");
			} else {
				// Put piece in file
				char filePath[50];
				sprintf(filePath, "./%s/%s/%s.%d", serverFolder, username, fileName, piece2+1);
				FILE *fptr = fopen(filePath, "wb");
				int file_success = 1;
				if (fptr == NULL) {
					printf("Couldn't open file!\n");
					file_success = 0;
				}
				else {
					// Write the buffer to the file
					fwrite(buf, 1, size2, fptr);
				}
				fclose(fptr);
			}
		}

	} else if (strcmp(command, "get") == 0) {
		fd_set set;
		struct timeval timeout;
		int rv;
		// Get file
		file = strtok(NULL, "\n");
		printf("Get command, %s\n", file);
		char filePieceNames[4][100];
		for (int i=0; i<4; i++) {
			sprintf(&filePieceNames[i][0], "%s.%d", file, i+1);
		}

		DIR *d;
		struct dirent *dir;
		char dirPath[100];
		sprintf(&dirPath[0], "./%s/%s/", serverFolder, username);
		d = opendir(dirPath);
		char *fileName;
		char fileName1[100] = "", fileName2[100];
		char filePath1[100], filePath2[100];
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				fileName = dir->d_name;
				for (int i=0; i<4; i++) {
					if(strcmp(filePieceNames[i], fileName) == 0){
						printf("match: %s\n", fileName);
						if(strlen(fileName1) == 0){
							sprintf(filePath1, "./%s/%s/%s", serverFolder, username, fileName);
							sprintf(fileName1, "%s", fileName);
						}else{
							sprintf(filePath2, "./%s/%s/%s", serverFolder, username, fileName);
							sprintf(fileName2, "%s", fileName);
						}
					}
				}
			}
			closedir(d);
		}
		printf("File1: %s File2: %s\n", fileName1, fileName2);
		write(clientfd, fileName1, strlen(fileName1));
		// Wait for OK
		FD_ZERO(&set);
		FD_SET(clientfd, &set);
		timeout.tv_sec = 2;
		bzero(buf, sizeof(buf));
		rv = select(clientfd + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			read(clientfd, clientACK, sizeof(clientACK));
		if (strncmp(clientACK, "OK\n", 2) != 0) {
			return 0;
		}
		// Read in file 1
		FILE *fptr = fopen(filePath1, "rb");
		if (fptr == NULL) {
			error("Couldn't open file");
			return 0;
		}
		fseek(fptr, 0L, SEEK_END);
		float size = ftell(fptr);
		rewind(fptr);
		char file1[BUFSIZE];
		fread(file1, (size_t) size, 1, fptr);
		fclose(fptr);
		// Send file 1
		write(clientfd, file1, strlen(file1));
		// Wait for OK
		FD_ZERO(&set);
		FD_SET(clientfd, &set);
		timeout.tv_sec = 2;
		bzero(buf, sizeof(buf));
		rv = select(clientfd + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			read(clientfd, clientACK, sizeof(clientACK));
		if (strncmp(clientACK, "OK\n", 2) != 0) {
			return 0;
		}
		write(clientfd, fileName2, strlen(fileName2));
		// Wait for OK
		FD_ZERO(&set);
		FD_SET(clientfd, &set);
		timeout.tv_sec = 2;
		bzero(buf, sizeof(buf));
		rv = select(clientfd + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			read(clientfd, clientACK, sizeof(clientACK));
		if (strncmp(clientACK, "OK\n", 2) != 0) {
			return 0;
		}
		// Read in file 2
		printf("Read in file 2\n");
		FILE *fptr2 = fopen(filePath2, "rb");
		if (fptr2 == NULL) {
			error("Couldn't open file");
			return 0;
		}
		fseek(fptr, 0L, SEEK_END);
		float size2 = ftell(fptr2);
		rewind(fptr2);
		char file2[BUFSIZE];
		fread(file2, (size_t) size2, 1, fptr2);
		fclose(fptr2);
		// Send file 2
		write(clientfd, file2, strlen(file2));
	} else {
		printf("Unrecognized command '%s' Error in client code!!\n", command);
		exit(-1);
	}
	// Close connection
	close(clientfd);
}

/**
 * Return the main listening socket
 *
 * Returns -1 or error
 */
int getListenerSocket(int port) {
	int listenfd, optval=1;
	struct sockaddr_in serveraddr;

	// Create socket descriptor
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	// Eliminates addresses already in use error
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0) {
		return -1;
	}

	// listenfd will be an endpoint for all requests to port
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short) port);
	if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
		return -1;
	}

	// Prepare socket to accept connections
	if (listen(listenfd, 20) < 0) {
		return -1;
	}
	return listenfd;
}
