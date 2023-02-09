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
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_HOSTNAME_LEN 256
#define MAX_PORT_LEN 8
#define MAX_FILENAME_LEN 256
#define MAX_REQUEST_LEN 512
#define MAX_RESPONSE_LEN 1048576

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[])
{
	int sockfd, rv, numbytes;
	char addr[INET6_ADDRSTRLEN];
	struct addrinfo hints, *servinfo, *p;

	if (argc != 2) {
		fprintf(stderr, "usage: client hostname\n");
		return 1;
	}

	/* analyze input argument */

	char hostname[MAX_HOSTNAME_LEN];
	char port[MAX_PORT_LEN];
	char filename[MAX_FILENAME_LEN];
	char *arg = argv[1];

	if strncmp(arg, "http://", 7) != 0 {
		fprintf(stderr, "Not a valid URL\n");
		return 1;
	} else {
		arg = &arg[7];
	}

	int colon_pos = strcspn(arg, ":");
	strncpy(hostname, arg, colon_pos);
	hostname[colon_pos] = '\0';
	arg = &arg[colon_pos + 1];

	int slash_pos = strcspn(arg, "/");
	strncpy(port, arg, slash_pos);
	port[slash_pos] = '\0';
	arg = &arg[slash_pos + 1];

	strcpy(filename, arg);

	/* bind to socket */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	/* bound */

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), addr, sizeof addr);
	printf("client: connecting to %s\n", addr);
	freeaddrinfo(servinfo); // all done with this structure

	// send request
	char request[MAX_REQUEST_LEN];
	sprintf(request, "GET /%s HTTP/1.1\r\n\r\n", filename);
	printf("client: sending '%s'\n", request);
	if (send(sockfd, request, strlen(request), 0) == -1) {
		perror("send");
		return 1;
	}

	char response[MAX_RESPONSE_LEN];
	if ((numbytes = recv(sockfd, response, MAX_RESPONSE_LEN-1, 0)) == -1) {
		perror("recv");
		return 1;
	}
	buf[numbytes] = '\0';

	// parse response
	int content_ptr = strcspn(response, "\r\n\r\n");
	response[content_ptr] = '\0';
	printf("client: received '%s'\n", response);
	char *content = &response[content_ptr + 4];

	// write response to file
	FILE *fp = fopen("output", "w");
	if (fp == NULL) {
		perror("fopen");
		return 1;
	}
	fwrite(content, 1, strlen(content), fp);
	fclose(fp);

	close(sockfd);
	return 0;
}

