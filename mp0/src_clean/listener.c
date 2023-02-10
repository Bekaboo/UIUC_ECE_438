/*
** listener.c -- a datagram sockets server demo
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

#define PORT "4950"
#define MAXBUFLEN 100


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


int main(void)
{

	/* bind to socket */

	int sockfd, rv;
	char addr[INET6_ADDRSTRLEN];
	struct addrinfo hints, *infos, *info;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &infos)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (info = infos; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (info == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(infos);

	/* bound */

	int numbytes;
	char buf[MAXBUFLEN];
	struct sockaddr_storage their_addr;

	printf("listener: waiting to recvfrom...\n");

	socklen_t addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		return 1;
	}
	buf[numbytes] = '\0';

	inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), addr, sizeof addr);
	printf("listener: got packet from %s\n", addr);
	printf("listener: packet is %d bytes long\n", numbytes);
	printf("listener: packet contains \"%s\"\n", buf);

	close(sockfd);
	return 0;
}

