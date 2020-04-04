#include <stdbool.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

#define BUFSIZE 65535

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(0);
}

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
 *  This extracts the method, uri, and version from a command
 */
void extractCommand(char *command, char *method, char *uri, char *version) {
	char delim[] = " ";
	char *temp;
	char tempBuf[BUFSIZE];
	memset(tempBuf, 0, BUFSIZE);
	memcpy(tempBuf, command, strlen(command));
	temp = strtok (tempBuf, delim);
	memcpy(method, temp, strlen(temp));
	method[strlen(temp)] = '\0';

	temp = strtok (NULL, delim);
	memcpy(uri, temp, strlen(temp));
	uri[strlen(temp)] = '\0';

	temp = strtok (NULL, delim);
	memcpy(version, temp, strlen(temp));
	version[strlen(temp)] = '\0';
	// printf("method: '%s' uri: '%s' version: '%s' \n", method, uri, version);
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
int getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);	// seek to end of file
	int size = ftell(file);	// get current file pointer
	fseek(file, 0, SEEK_SET);	// seek back to beginning of file
	return size;
}

/**
 * Check if hostname is whitelisted. Returns 1 if whitelist file does not exist
 */
int isInFile(char *hostname, char *filename){
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(filename, "r");
	while ((read = getline(&line, &len, fp)) != -1) {
			if(indexOf(line, hostname) == 0){
				fclose(fp);
				return 1;
			}
	}
	fclose(fp);
	return 0;
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

/**
 *  Send a 403 Forbidden response to client
 */
// void sendError403(int fd, char *version) {
// 	char *message = "Forbidden\n";
//
// 	char tmp[40];
// 	memcpy( tmp, &version[0], 8);
// 	memcpy( &tmp[8], " 400 Forbidden ", 16);
// 	char header[24];
// 	memcpy(header, tmp, 24);
// 	sendResponseToClient(fd, header, "text/plain", message, strlen(message));
// }
