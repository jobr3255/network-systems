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
 *  This extracts the cmethod, uri, and version from a command
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

void extractPostData(char *request, char *data){
	printf("Request: \n");
	printf("%s\n\n", request);
	char delim[] = "\r\n";
	char *temp;
	char tempBuf[BUFSIZE];
	memset(tempBuf, 0, BUFSIZE);
	memcpy(tempBuf, request, strlen(request));
	temp = strtok (tempBuf, delim);
	char prevLine[BUFSIZE];
	memset(prevLine, 0, BUFSIZE);
	int size = 0;
	while (temp != NULL) {
		printf("temp: '%s' \n", temp);
		memcpy(prevLine, temp, strlen(temp));
		size = strlen(temp);
		temp = strtok (NULL, delim);
	}
	memcpy(data, &prevLine[0], size);
	// printf("last line: '%s' \n", prevLine);
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
 * Lowercase a string
 */
char *toLower(char *s) {
	for (char *p = s; *p != '\0'; p++) {
		*p = tolower(*p);
	}

	return s;
}

#define DEFAULT_MIME_TYPE "application/octet-stream"

/**
 * Return a MIME type for a given filename
 */
char *getMimeType(char *filename) {
	char *ext = strrchr(filename, '.');

	if (ext == NULL) {
		return DEFAULT_MIME_TYPE;
	}

	ext++;

	toLower(ext);

	if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) { return "text/html"; }
	if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0) { return "image/jpg"; }
	if (strcmp(ext, "css") == 0) { return "text/css"; }
	if (strcmp(ext, "js") == 0) { return "application/javascript"; }
	if (strcmp(ext, "json") == 0) { return "application/json"; }
	if (strcmp(ext, "txt") == 0) { return "text/plain"; }
	if (strcmp(ext, "gif") == 0) { return "image/gif"; }
	if (strcmp(ext, "png") == 0) { return "image/png"; }
	if (strcmp(ext, "ico") == 0) { return "image/x-icon"; }

	return DEFAULT_MIME_TYPE;
}
