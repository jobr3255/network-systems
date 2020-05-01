#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include <stdlib.h>
#include <sys/file.h>
#include <math.h>
#include <openssl/md5.h>// For hashing

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

// Global variables
// #define BUFSIZE 1024
#define BUFSIZE 65535
int debug = 0;
int connections[4];
char *authentication;
unsigned char hashValue[MD5_DIGEST_LENGTH];	// Hash output
void handlePut(int hash, int sockets[4], int size1, int size234, char *filePieces[4]);
void handleGet(char *originalFileName, int sockets[4], char *username, char *fullRequest);

void connectServers(int *sockfd, int *port, int *connections, char serverIPs[4][20], struct sockaddr_in serveraddr);
int connectToServer(int *sockfd, struct sockaddr_in serveraddr, int portval, char *serverIP);
int hash(char *piece1, char *piece2, char *piece3, char *piece4);

// Wrapper for perror
void error(char *msg) {
	perror(msg);
}

int main(int argc, char **argv) {
	// Declare all the variables!
	char buf[BUFSIZE];
	char *configFileName;	// name of the configuration file
	char *username, *password;// client's username and password
	char *configLine, *temp;// used to parse config file
	char *fullRequest, *command, *fileName;	// hold which action/file for request
	int sockfd1, sockfd2, sockfd3, sockfd4;	// sockets for connections to servers
	struct sockaddr_in server1addr, server2addr, server3addr, server4addr;// server addresses
	struct hostent *server1, *server2, *server3, *server4;
	char serverNames[4][20];// Hold the server names
	int serverPorts[4];	// Hold the server ports
	char serverIPs[4][20];// IP addresses of server
	int serverlen, port, n, authSuccess;
	int sockets[4];
	connections[0] = 0;
	connections[1] = 0;
	connections[2] = 0;
	connections[3] = 0;

	// Check command line arguments
	if (argc != 2) {
		fprintf(stderr,"usage: %s <dfc.conf>\n", argv[0]);
		exit(1);
	}
	configFileName = argv[1];

	// Load username and password from dfc.conf
	FILE *fptr = fopen(configFileName, "rb");
	if (fptr == NULL) {
		error("Couldn't open configuration file");
		exit(-1);
	} else {
		int i = 0;
		// Get lines from config file
		while(!feof(fptr)) {
			size_t len = 0;
			getline(&configLine, &len, fptr);
			temp = strtok(configLine, " ");
			// If null break while loop
			if(temp == NULL) {
				break;
			}
			// Parse config file
			if (strcmp(temp, "Server") == 0) {
				temp = strtok(NULL, " ");
				memcpy(serverNames[i], temp, sizeof(temp));
				temp = strtok(NULL, ":");
				strcpy(serverIPs[i], temp);
				temp = strtok(NULL, ":");
				port = atoi(temp);
				serverPorts[i] = port;
				i++;
			} else if (strcmp(temp, "Password:") == 0) {
				temp = strtok(NULL, "\n");
				password = temp;
			} else if (strcmp(temp, "Username:") == 0) {
				temp = strtok(NULL, " ");
				username = temp;
				username[strlen(username)-1] = '\0';
			}
		}
		fclose(fptr);
		authentication = username;
		strcat(authentication, " ");
		strcat(authentication, password);

		// While loop to accept user requests
		while (1) {
			authSuccess = 1;

			// Get a request from the user
			bzero(buf, BUFSIZE);
			printf("(dfc) ");
			fgets(buf, BUFSIZE, stdin);

			// Save original request
			fullRequest = calloc(strlen(buf)-1, sizeof(char));
			strcpy(fullRequest, buf);

			// Parse request
			command = strtok(buf, " ");
			if(command == NULL) {
				command[strlen(command)] = '\0';
				printf("Command is null\n");
				continue;
			}
			if (strcmp(command, "exit\n") == 0) {
				exit(0);
			} else if (strcmp(command, "list\n") == 0 || strcmp(command, "put") == 0 || strcmp(command, "get") == 0) {
				// Check server connections
				connectServers(sockets, serverPorts, connections, serverIPs, server1addr);

				// Check if any servers running
				int numServers = 0;
				for (int i=0; i<4; i++) {
					if (connections[i] == 1) {
						numServers++;
					}
					if (connections[i] == -1) {
						numServers = -10;
					}
				}
				// Make sure at least one server is running and that user is authenticated
				if (numServers == 0) {
					printf("No server connections.\n");
					continue;
				}else if (numServers < 0) {
					printf("Not authorized. Check username/password.\n");
					continue;
				}

				// Switch case based on command
				if (strcmp(command, "list\n") == 0) {
					for (int i=0; i<4; i++) {
						if (connections[i] == 1) {
							// Send request
							write(sockets[i], fullRequest, strlen(fullRequest));
						}
					}
					char lines[100][100];
					char files[100][100];
					int matrix[100][4];
					char *temp, *file, *piece, *trash;
					int lineCount = 0;
					int num, rowCount = 0;
					fd_set set;
					struct timeval timeout;
					int rv;
					for (int i=0; i<4; i++) {
						if (connections[i] == 1) {
							lineCount = 0;
							n = -1;
							FD_ZERO(&set);// clear the set
							FD_SET(sockets[i], &set);	// add our socket descriptor to the set
							timeout.tv_sec = 1;
							// Wait until the server acknowledged the first request
							rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
							if(rv == -1)
								perror("select\n");	// an error accured
							else if(rv == 0)
								printf("timeout\n");// a timeout occured
							else
								n = read(sockets[i], buf, BUFSIZE);
							if (n < 0) {
								error("Error receiving directory listing from server");
							} else {
								temp = strtok(buf,"\n");
								while(temp != NULL) {
									strcpy(lines[lineCount], temp);
									trash = strtok(NULL, "\n");
									lineCount++;
									temp = trash;
								}
								for (int k=0; k<lineCount; k++) {
									if (strncmp(lines[k], "listResult.txt\n", 10) != 0) {
										// Extract file name and piece number
										file = strtok(lines[k], ".");
										piece = strtok(NULL, ".");
										num = atoi(piece)-1;
										while (num != 0 && num != 1 && num != 2 && num != 3) {
											piece = strtok(NULL, ".");
											num = atoi(piece)-1;
										}

										// Check if file already found
										int found = -1;
										for (int k=0; k<rowCount; k++) {
											if (strcmp(files[k], file) == 0) {
												found = k;
											}
										}
										if (found == -1) {
											// Add file
											// printf("Add %s in position %d\n", file, rowCount);
											sprintf(files[rowCount], "%s", file);
											matrix[rowCount][0] = 0;
											matrix[rowCount][1] = 0;
											matrix[rowCount][2] = 0;
											matrix[rowCount][3] = 0;

											matrix[rowCount][num] = 1;
											rowCount++;
										} else {
											matrix[found][num] = 1;
										}
									}
								}
							}
						}
					}

					char line[100];
					char response[500] = "";
					for (int k=0; k<rowCount; k++) {
						// Can reconstruct file!
						if (matrix[k][0] == 1 && matrix[k][1] == 1 && matrix[k][2] == 1 && matrix[k][3] == 1) {
							sprintf(line, "%s\n", files[k]);
							strcat(response, line);
						} else {
							// File is incomplete
							sprintf(line, "%s [incomplete]\n", files[k]);
							strcat(response, line);
						}
					}
					printf("Listing:\n%s", response);

				} else if (strcmp(command, "put") == 0) {
					char filePath[50];
					fileName = strtok(NULL, "\n");
					sprintf(&filePath[0], "./Local/%s", fileName);
					FILE *fptr = fopen(filePath, "rb");
					if (fptr == NULL) {
						error("Couldn't open file");
						continue;
					}

					// Read in partitioned file
					fseek(fptr, 0L, SEEK_END);
					float size = ftell(fptr)/4;
					rewind(fptr);
					int size1, size234;	// Sizes for files
					size1 = ceil(size);
					size234 = floor(size);
					char filePieces[4][BUFSIZE];
					fread(filePieces[0], (size_t) size1, 1, fptr);
					fread(filePieces[1], (size_t) size234, 1, fptr);
					fread(filePieces[2], (size_t) size234, 1, fptr);
					fread(filePieces[3], (size_t) size234, 1, fptr);
					fclose(fptr);

					// Get hash of file
					int hashNum = hash(filePieces[0], filePieces[1], filePieces[2], filePieces[3]);
					printf("Hash: %d\n", hashNum);

					char *pieces[4];
					for (int i=0; i<4; i++) {
						pieces[i] = filePieces[i];
					}

					for (int i=0; i<4; i++) {
						if (connections[i] == 1) {
							// Send request
							write(sockets[i], fullRequest, strlen(fullRequest));
						}
					}
					handlePut(hashNum, sockets, size1, size234, pieces);

				} else if (strcmp(command, "get") == 0) {
					fileName = strtok(NULL, "\n");
					handleGet(fileName, sockets, username, fullRequest);
				}
			} else {
				command[strlen(command)] = '\0';
				printf("%s: unrecognized command\n", command);
			}
		}
	}
}

// get the hash number
int hash(char *piece1, char *piece2, char *piece3, char *piece4) {
	MD5_CTX c;
	bzero(&hashValue, sizeof(hashValue));
	bzero(&c, sizeof(&c));
	MD5_Init(&c);
	MD5_Update(&c, piece1, sizeof(piece1));
	MD5_Update(&c, piece2, sizeof(piece2));
	MD5_Update(&c, piece3, sizeof(piece3));
	MD5_Update(&c, piece4, sizeof(piece4));
	MD5_Final(hashValue, &c);
	char md5url[MD5_DIGEST_LENGTH*2 + 1];
	for (int i=0; i<MD5_DIGEST_LENGTH; i++) {
		sprintf(&md5url[i*2], "%02x", (unsigned int) hashValue[i]);
	}
	int hashed_num = atoi(md5url)%4;
	return hashed_num;
}

void handleGet(char *originalFileName, int sockets[4], char *username, char *fullRequest) {
	int n;
	char buf[BUFSIZE];
	char okMsg[20] = "OK\n";
	char errorMsg[20] = "ERROR\n";
	fd_set set;
	struct timeval timeout;
	int rv;

	for (int i=0; i<4; i++) {
		if (connections[i] != 1) {
			continue;
		}
		write(sockets[i], fullRequest, strlen(fullRequest));
		// Wait to get first file name
		FD_ZERO(&set);
		FD_SET(sockets[i], &set);
		timeout.tv_sec = 2;
		n = -1;
		bzero(buf, sizeof(buf));
		rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			n = read(sockets[i], buf, sizeof(buf));
		if (n < 0) {
			error("Error in receiving from server");
			write(sockets[i], errorMsg, sizeof(errorMsg));
			continue;
		}
		char fileName[BUFSIZE];
		strcpy(fileName, buf);
		// Send OK
		write(sockets[i], okMsg, sizeof(okMsg));

		// Wait to get file
		FD_ZERO(&set);
		FD_SET(sockets[i], &set);
		timeout.tv_sec = 2;
		n = -1;
		bzero(buf, sizeof(buf));
		rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			n = read(sockets[i], buf, sizeof(buf));
		if (n < 0) {
			error("Error in receiving from server");
			write(sockets[i], errorMsg, sizeof(errorMsg));
			continue;
		}
		// Write file fragment
		FILE *fptr = fopen(fileName, "wb");
		if (fptr == NULL) {
			printf("Couldn't open file!\n");
		}
		else {
			fwrite(buf, 1, n, fptr);
		}
		fclose(fptr);
		// Send OK
		write(sockets[i], okMsg, sizeof(okMsg));
		// Wait to get second file name
		FD_ZERO(&set);
		FD_SET(sockets[i], &set);
		timeout.tv_sec = 2;
		n = -1;
		bzero(buf, sizeof(buf));
		rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			n = read(sockets[i], buf, sizeof(buf));
		if (n < 0) {
			error("Error in receiving from server");
			write(sockets[i], errorMsg, sizeof(errorMsg));
			continue;
		}
		char fileName2[BUFSIZE];
		strcpy(fileName2, buf);

		// Send OK
		write(sockets[i], okMsg, sizeof(okMsg));

		// Wait to get file
		FD_ZERO(&set);
		FD_SET(sockets[i], &set);
		timeout.tv_sec = 2;
		n = -1;
		bzero(buf, sizeof(buf));
		rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			n = read(sockets[i], buf, sizeof(buf));
		if (n < 0) {
			error("Error in receiving from server");
			write(sockets[i], errorMsg, sizeof(errorMsg));
			continue;
		}
		// Write file fragment
		FILE *fptr2 = fopen(fileName2, "wb");
		if (fptr2 == NULL) {
			printf("Couldn't open file!\n");
		}
		else {
			fwrite(buf, 1, n, fptr2);
		}
		fclose(fptr2);
	}

	char filePieceNames[4][100];
	int fileSizes[4];
	for (int i=0; i<4; i++) {
		sprintf(&filePieceNames[i][0], "%s.%d", originalFileName, i+1);
	}

	int completeFile = 1;
	// Piece 1
	FILE *fptr1 = fopen(filePieceNames[0], "rb");
	if (fptr1 == NULL) {
		printf("Couldn't open file: %s\n", filePieceNames[0]);
		completeFile = 0;
	}else{
		fseek(fptr1, 0, SEEK_END);
	  fileSizes[0] = ftell(fptr1);
	  fseek(fptr1, 0, SEEK_SET);
	}
	// Piece 2
	FILE *fptr2 = fopen(filePieceNames[1], "rb");
	if (fptr2 == NULL) {
		printf("Couldn't open file: %s\n", filePieceNames[1]);
		completeFile = 0;
	}else{
		fseek(fptr2, 0, SEEK_END);
	  fileSizes[1] = ftell(fptr2);
	  fseek(fptr2, 0, SEEK_SET);
	}
	// Piece 3
	FILE *fptr3 = fopen(filePieceNames[2], "rb");
	if (fptr3 == NULL) {
		printf("Couldn't open file: %s\n", filePieceNames[2]);
		completeFile = 0;
	}else{
		fseek(fptr3, 0, SEEK_END);
	  fileSizes[2] = ftell(fptr3);
	  fseek(fptr3, 0, SEEK_SET);
	}
	// Piece 4
	FILE *fptr4 = fopen(filePieceNames[3], "rb");
	if (fptr4 == NULL) {
		printf("Couldn't open file: %s\n", filePieceNames[3]);
		completeFile = 0;
	}else{
		fseek(fptr4, 0, SEEK_END);
	  fileSizes[3] = ftell(fptr4);
	  fseek(fptr4, 0, SEEK_SET);
	}
	//
	// // If all pieces available
	char pieces[4][BUFSIZE];
	if (completeFile == 1) {
		// Reconstruct file
		// Piece 1
		bzero(pieces[0], sizeof(pieces[0]));
		fread(pieces[0], (size_t) fileSizes[0], 1, fptr1);
		// Piece 2
		bzero(pieces[1], sizeof(pieces[1]));
		fread(pieces[1], (size_t) fileSizes[1], 1, fptr2);
		// Piece 3
		bzero(pieces[2], sizeof(pieces[2]));
		fread(pieces[2], (size_t) fileSizes[2], 1, fptr3);
		// Piece 4
		bzero(pieces[3], sizeof(pieces[3]));
		fread(pieces[3], (size_t) fileSizes[3], 1, fptr4);

		char filePath[50];
		sprintf(&filePath[0], "./Local/%s", originalFileName);
		FILE *fptrWhole = fopen(filePath, "a+");
		if (fptr3 == NULL) {
			error("Couldn't create file on client");
		} else {
			fprintf(fptrWhole, "%s%s%s%s", pieces[0], pieces[1], pieces[2], pieces[3]);
		}
		fclose(fptrWhole);

	} else {
		// Throw error message
		printf("File is incomplete.\n");
	}
	//
	// // Delete & close partial files
	if (fptr1 != NULL) {
		fclose(fptr1);
	}
	if (fptr2 != NULL) {
		fclose(fptr2);
	}
	if (fptr3 != NULL) {
		fclose(fptr3);
	}
	if (fptr4 != NULL) {
		fclose(fptr4);
	}
	for (int i=0; i<4; i++) {
		remove(filePieceNames[i]);
	}
}

/**
   hash	DFS1		DFS2		DFS3		 DFS4
   0		(1,2)		(2,3)		(3,4)		(4,1)
   1		(4,1)		(1,2)		(2,3)		(3,4)
   2		(3,4)		(4,1)		(1,2)		(2,3)
   3		(2,3)		(3,4)		(4,1)		(1,2)
 */
void handlePut(int hash, int sockets[4], int size1, int size234, char *filePieces[4]) {
	char primers[4][100];
	char serverACK[100];
	int dfsOffset, p1Index, p2Index;
	fd_set set;
	struct timeval timeout;
	int rv;
	if(hash == 0) {
		dfsOffset = 0;
	}else if(hash == 1) {
		dfsOffset = 3;
	}else if(hash == 2) {
		dfsOffset = 2;
	}else if(hash == 3) {
		dfsOffset = 1;
	}
	for (int i=0; i<4; i++) {
		if (connections[i] != 1) {
			continue;
		}

		FD_ZERO(&set);// clear the set
		FD_SET(sockets[i], &set);	// add our socket descriptor to the set
		timeout.tv_sec = 1;
		// Wait until the server acknowledged the first request
		rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			read(sockets[i], serverACK, sizeof(serverACK));

		if (strncmp(serverACK, "OK\n", 2) != 0) {
			continue;
		}
		p1Index = i + dfsOffset;
		if(p1Index >= 4) {
			p1Index = p1Index - 4;
		}
		p2Index = p1Index + 1;
		if(p2Index >= 4) {
			p2Index = p2Index - 4;
		}


		// Send primers
		snprintf(primers[i], 100, "%d,%d,%d,%d\n", p1Index, size1, p2Index, size234);
		// printf("Sending primer %d,%d,%d,%d\n", p1Index, size1, p2Index, size234);
		write(sockets[i], primers[i], strlen(primers[i]));

		// Wait for response from server
		FD_ZERO(&set);// clear the set
		FD_SET(sockets[i], &set);	// add our socket descriptor to the set
		timeout.tv_sec = 1;
		// Wait until the server acknowledged the first request
		rv = select(sockets[i] + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
			perror("select\n");	// an error accured
		else if(rv == 0)
			printf("timeout\n");// a timeout occured
		else
			read(sockets[i], serverACK, sizeof(serverACK));
		if (strncmp(serverACK, "OK\n", 2) == 0) {
			// Send pieces to servers
			write(sockets[i], filePieces[p1Index], strlen(filePieces[p1Index]));
			write(sockets[i], filePieces[p2Index], strlen(filePieces[p2Index]));
		} else {
			printf("Failed comparison\n");
		}
	}
}

/**
 * Authenticates user with a server
 */
int authenticate(int sockfd) {
	// Send authentication
	int n, result;
	char buf[BUFSIZE];
	n = write(sockfd, authentication, strlen(authentication));
	if (n < 0) {
		return 0;
	}
	// Wait to see if authentication successful
	n = read(sockfd, buf, BUFSIZE);
	if (n < 0) {
		printf("Error receiving message from server\n");
		return 0;
	} else {
		if (strncmp(buf, "Invalid Username/Password.", 18) == 0) {
			return -1;
		}
	}
	return 1;
}

/**
 * Connect all servers
 *
 * Sets connections[i] to:
 *  0 - server down
 * -1 - authentication failed
 *  1 - authentication successful
 */
void connectServers(int *sockets, int *port, int *connections, char serverIPs[4][20], struct sockaddr_in serveraddr) {
	for (int i=0; i<4; i++) {
		if(connectToServer(&sockets[i], serveraddr, port[i], serverIPs[i]) < 0) {
			connections[i] = 0;
			printf("Server %d is down\n", (i+1));
		} else {
			connections[i] = authenticate(sockets[i]);
		}
	}
}

/**
 * Connect to a server
 *
 * Returns 0 on success
 */
int connectToServer(int *sockfd, struct sockaddr_in serveraddr, int port, char *serverIP) {
	// Open socket to server
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*socket < 0) {
		error("ERROR opening socket");
		return -1;
	}

	// Build servers Internet address
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_aton(serverIP,(struct in_addr *)&serveraddr.sin_addr.s_addr);
	serveraddr.sin_port = htons(port);

	if(connect(*sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
		// perror("connect");
		if(debug) {
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
		}
		return -1;
	}
	return 0;
}
