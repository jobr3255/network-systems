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
bool isInFile(char *hostname, char *filename){
	FILE *fp;
	char temp[512];
	fp = fopen(filename, "r");
	while(fgets(temp, 512, fp) != NULL) {
		if((strstr(temp, hostname)) != NULL) {
			fclose(fp);
			return true;
		}
	}
	//Close the file if still open.
	if(fp) {
		fclose(fp);
	}
 	return false;
}
