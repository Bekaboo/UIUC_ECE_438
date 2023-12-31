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
#define MAX_REQUEST_LEN 1024
#define MAX_RESPONSE_HEADER_LEN 32
#define MAX_SEGMENT_LEN 4194304
#define DEFAULT_PORT "80"

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
	int sockfd, rv;
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

	if (strncmp(arg, "http://", 7) != 0) {
		fprintf(stderr, "Not a valid URL\n");
		return 1;
	} else {
		arg = &arg[7];
	}

	int colon_pos = strcspn(arg, ":");
	int slash_pos = strcspn(arg, "/");
	if (colon_pos == strlen(arg)) {
		strncpy(hostname, arg, slash_pos);
		hostname[slash_pos] = '\0';
		strcpy(port, DEFAULT_PORT);
	} else {
		slash_pos -= colon_pos + 1;
		strncpy(hostname, arg, colon_pos);
		hostname[colon_pos] = '\0';
		arg = &arg[colon_pos + 1];
		strncpy(port, arg, slash_pos);
		port[slash_pos] = '\0';
	}

	arg = &arg[slash_pos];
	strcpy(filename, arg);

	/* bind to socket */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	printf("client: hostname is '%s' on port %s\n", hostname, port);

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

	/* HTTP-compatible client */

	char request[MAX_REQUEST_LEN];
	char response_header[MAX_RESPONSE_HEADER_LEN];
	char segment[MAX_SEGMENT_LEN];
	int segment_len, file_len = 0;

	// send request
	sprintf(request, "GET %s HTTP/1.1\r\n\r\n", filename);
	printf("client: sending request '''\n%s'''\n", request);
	if (send(sockfd, request, strlen(request), 0) == -1) {
		perror("send");
		return 1;
	}

	// receive response header
	if (recv(sockfd, response_header, MAX_RESPONSE_HEADER_LEN, 0) == -1) {
		perror("recv");
		return 1;
	}
	printf("client: received response header '''\n%s\n'''\n", response_header);

	// check for valid response and open file
	if (strncmp(response_header, "HTTP/1.1 200 OK", 15) == 0) {
		FILE *fptr = fopen("output", "w");
		if (fptr == NULL) {
			perror("fopen");
			return 1;
		}

		// receive and write file segments to disk
		while ((segment_len = recv(sockfd, segment, MAX_SEGMENT_LEN, 0))) {
			if (segment_len == -1) {
				perror("recv");
				return 1;
			}
			fwrite(segment, 1, segment_len, fptr);
			file_len += segment_len;
		}
		printf("client: received %d bytes of file\n", file_len);
		fclose(fptr);
	}

	/* HTTP-compatible client END */

	close(sockfd);
	return 0;
}

