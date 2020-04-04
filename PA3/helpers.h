#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdbool.h>

void error(char *msg);
int indexOf(char *str, char *find);
bool contains(char *str, char *find);
void extractCommand(char *command, char *method, char *uri, char *version);
bool fileExists(char *fileName);
int getFileSize(FILE *file);
bool isInFile(char *hostname, char *filename);

#endif
