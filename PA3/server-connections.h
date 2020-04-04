#ifndef _SERVER_CONNECTIONS_H_
#define _SERVER_CONNECTIONS_H_

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

void createServerSocket(struct ServerConnectionInfo *connection);
void setServerDNSEntry(struct ServerConnectionInfo *connection);
void buildServerInternetAddress(struct ServerConnectionInfo *connection);

struct sockaddr_in createServerConnection(int sockfd, char *hostname, int portno);
int sendToServer(int sockfd, char *path, char *version, char *request);
void getServerResponse(struct ServerConnectionInfo connection);
void getFromServer(struct ServerConnectionInfo connection, char *buf, char *variable);

#endif
