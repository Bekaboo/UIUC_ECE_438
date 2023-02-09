/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAX_REQUEST_LEN 1024
#define MAX_FILENAME_LEN 256
#define RESPONSE_HEADER_LEN 32


void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


char* read_file(const char *filename)
{
	FILE *fptr = fopen(filename, "r");
	if (fptr == NULL) {
		printf("Cannot open file\n");
		return NULL;
	}

	fseek(fptr, 0, SEEK_END);
	unsigned long len = (unsigned long)ftell(fptr);
	fseek(fptr, 0, SEEK_SET);
	char *content = malloc(len + 1);
	fread(content, len, 1, fptr);
	content[len] = 0;
	fclose(fptr);

	return content;
}


int main(int argc, char *argv[])
{
	int sockfd, new_fd, rv, yes = 1;  // listen on sock_fd, new connection on new_fd
	char addr[INET6_ADDRSTRLEN];
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;

	if (argc != 2) {
		fprintf(stderr, "Port number not specified\n");
		return 1;
	}

	/* bind to socket */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			return 1;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}
	freeaddrinfo(servinfo); // all done with this structure

	/* bound */

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		return 1;
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	printf("server: waiting for connections...\n");

	while (1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), addr, sizeof addr);
		printf("server: got connection from %s\n", addr);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			/*  HTTP-compatible server
			
				Request format:
					GET /<filename> HTTP/1.1\r\r

				Response format:
					HTTP/1.1 <statcode> <statmsg>\r\r<content>
			*/

			char request[MAX_REQUEST_LEN];
			char filename[MAX_FILENAME_LEN];
			char response_header[RESPONSE_HEADER_LEN];
			char *response;
			int valid = 1;

			recv(new_fd, request, MAX_REQUEST_LEN, 0);

			// check if request starts with "GET /"
			if (strncmp(request, "GET /", 5) != 0) {
				valid = 0;
				strcpy(response_header, "HTTP/1.1 400 Bad Request\r\r");
				goto READY_FOR_RESPONSE;
			} else {
				request += 5 * sizeof(char);
			}

			// extract filename from request
			int space_pos = strcspn(request, " ");
			strncpy(filename, request, space_pos);
			filename[space_pos] = 0;
			request += space_pos * sizeof(char);

			// check if request ends with " HTTP/1.1\r\r"
			if (strncmp(request, " HTTP/1.1\r\r", 11) != 0) {
				valid = 0;
				strcpy(response_header, "HTTP/1.1 400 Bad Request\r\r");
				goto READY_FOR_RESPONSE;
			}

			char *content = read_file(filename);
			if (!content) {
				valid = 0;
				strcpy(response_header, "HTTP/1.1 404 Not Found\r\r");
			} else {
				strcpy(response_header, "HTTP/1.1 200 OK\r\r");
			}

READY_FOR_RESPONSE:
			if (valid) {
				response = malloc(strlen(response_header) + strlen(content) + 1);
				strcpy(response, response_header);
				strcat(response, content);
			} else {
				response = malloc(strlen(response_header) + 1);
				strcpy(response, response_header);
			}

			if (send(new_fd, response, strlen(response), 0) == -1)
				perror("send");

			/* HTTP-compatible server END */

			close(new_fd);
			return 0;
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

