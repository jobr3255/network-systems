#ifndef _CLIENT_CONNECTIONS_H_
#define _CLIENT_CONNECTIONS_H_

#define BUFSIZE 65535

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

// struct ClientConnectionInfo createClientConnection(int portno, int sockfd);
// void sendToClient(struct ClientConnectionInfo connection, void *buf, int size);

// void sendError400(int fd, char *version);
// void sendError403(int fd, char *version);
// int sendResponseToClient(int fd, char *header, char *contentType, void *body, int contentLength);

#endif
