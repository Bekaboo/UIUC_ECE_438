/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT "3490"
#define MAXDATASIZE 100


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


int main(int argc, char *argv[])
{

	if (argc != 2) {
		fprintf(stderr, "usage: client hostname\n");
		return 1;
	}

	/* bind to socket */

	int sockfd, rv;
	char addr[INET6_ADDRSTRLEN];
	struct addrinfo hints, *infos, *info;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &infos)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (info = infos; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (info == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	freeaddrinfo(infos);

	/* bound */

	inet_ntop(info->ai_family, get_in_addr((struct sockaddr *)info->ai_addr), addr, sizeof addr);
	printf("client: connecting to %s\n", addr);

	int numbytes;
	char buf[MAXDATASIZE];

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
		perror("recv");
		return 1;
	}
	buf[numbytes] = '\0';

	char *pos = strchr(buf, '\n');
	unsigned int str_position = pos - buf;
	printf("client: received %.*s bytes", str_position, buf);
	printf("%s", buf + str_position + 2);

	close(sockfd);
	return 0;
}

