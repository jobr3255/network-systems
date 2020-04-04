#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

void handleRequest(int fd);
void handleGet(int fd, char *url, char *version, char *request);

void sendError400(int fd, char *version);
void sendError403(int fd, char *version);
int sendResponseToClient(int fd, char *header, char *contentType, void *body, int contentLength);

// int isInFile(char *hostname, char *filename);
// int isWhitelisted(char *hostname);
// int isBlacklisted(char *hostname);

#endif
