#include <stdbool.h>
#include <dirent.h>

#define BUFSIZE 65535
// #define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(0);
}

/**
 *  A struct to hold all the client connection info in one place
 */
struct ClientConnectionInfo
{
	int sockfd;	/* socket */
	int portno;	/* port to listen on */
	int clientlen;/* byte size of client's address */
	struct sockaddr_in serveraddr;/* server's addr */
	struct sockaddr_in clientaddr;/* client addr */
	struct hostent *hostp;/* client host info */
	char *hostaddrp;/* dotted decimal host addr string */
	int optval;	/* flag value for setsockopt */
	int n;/* message byte size */
};

/**
 *  A struct to hold all the client connection info in one place
 */
struct ServerConnectionInfo
{
	int sockfd;	/* socket */
	int portno;	/* port to listen on */
	int n;/* message byte size */
	int serverlen;/* byte size of server's address */
	struct sockaddr_in serveraddr;/* server's addr */
	struct hostent *server;	/* server host info */
	char *hostname;
};

/**
 *  @param char *str
 *      String to search for substring in
 *  @param char *find
 *      The substring we are trying to find
 *  @return int
 *      Returns index of substring or -1 if not found
 */
int indexOf(char *str, char *find) {
	char *result = strstr(str, find);
	int index = result - str;
	return index < 0 ? -1 : index;
}

/**
 *  @param char *str
 *      String to search for substring in
 *  @param char *find
 *      The substring we are searching for
 *  @return bool
 *      Returns true if substring exists inside str, returns false otherwise
 */
bool contains(char *str, char *find) {
	if(strstr(str, find) != NULL) {
		return true;
	}
	return false;
}

/**
 *  This extracts the command and variable passed in
 *
 *  @param char *input
 *      Full string of user input
 *  @param char *command
 *      Pointer reference to command variable
 *  @param char *variable
 *      Pointer reference to variable variable
 */
void extractCommandAndVariable(char *buf, char *command, char *variable) {
	// printf("command: '%s' variable: '%s' \n", command, variable);
	char delim[] = " \n";
	char *temp;
	char tempBuf[BUFSIZE];
	memset(tempBuf,0, BUFSIZE);
	memset(command,0, BUFSIZE);
	memset(variable,0, BUFSIZE);
	memcpy(tempBuf, buf, strlen(buf));
	int wordIndex = 0;
	temp = strtok (tempBuf, delim);
	while (temp != NULL) {
		// printf("temp: '%s' \n", temp);
		if(wordIndex == 0) {
			memcpy(command, temp, strlen(temp));
			command[strlen(temp)] = '\0';
		}else if(wordIndex == 1) {
			memcpy(variable, temp, strlen(temp));
			variable[strlen(temp)] = '\0';
		}
		wordIndex++;
		temp = strtok (NULL, delim);
	}
}

/**
 *  Determines existence of file
 *
 *  @param char *fileName
 *      Name of the file
 *  @return bool
 */
bool fileExists(char *fileName) {
	return (access( fileName, F_OK ) != -1);
}

/**
 *  Determines size of file
 *
 *  @param char *file
 *      Name of the file
 *  @return int
 */
int getFileSize(FILE *file){
  fseek(file, 0, SEEK_END); // seek to end of file
  int size = ftell(file); // get current file pointer
  fseek(file, 0, SEEK_SET); // seek back to beginning of file
  return size;
}
