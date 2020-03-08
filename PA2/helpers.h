#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <stdbool.h>

void error(char *msg);
int indexOf(char *str, char *find);
bool contains(char *str, char *find);
void extractCommand(char *command, char *method, char *uri, char *version);
void extractPostData(char *command, char *data);
bool fileExists(char *fileName);
int getFileSize(FILE *file);
char *toLower(char *s);
char *getMimeType(char *filename);

#endif
