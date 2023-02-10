/*
** talker.c -- a datagram client demo
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


int main(int argc, char *argv[])
{

	if (argc != 3) {
		fprintf(stderr, "usage: talker hostname message\n");
		return 1;
	}

	/* bind to socket */

	int sockfd, rv;
	struct addrinfo hints, *infos, *info;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &infos)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (info = infos; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (info == NULL) {
		fprintf(stderr, "talker: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(infos);

	/* bound */

	int numbytes;

	if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
		info->ai_addr, info->ai_addrlen)) == -1) {
		perror("talker: sendto");
		return 1;
	}

	printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);

	close(sockfd);
	return 0;
}

