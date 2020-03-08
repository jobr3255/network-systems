#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

void handleRequest(int fd);
void handleGet(int fd, char *uri, char *version);
void handlePost(int fd, char *version, char* data);

void *getInAddr(struct sockaddr *sa);
int getListenerSocket(char *port);

void sendError(int fd, char *version);
void sendError404(int fd, char *version);
void sendError404Page(int fd, char *version);
int sendResponse(int fd, char *header, char *contentType, void *body, int contentLength);

#endif
