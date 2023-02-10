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
#define MAX_RESPONSE_HEADER_LEN 32
#define MAX_SEGMENT_LEN 4194304  // requested file is read and sent in 4MB segments


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
					GET /<filename> HTTP/1.1\r\n\r\n

				Response format:
					HTTP/1.1 <statcode> <statmsg>\r\n\r\n<content>
			*/

			char request[MAX_REQUEST_LEN];
			char filename[MAX_FILENAME_LEN];
			char response_header[MAX_RESPONSE_HEADER_LEN];
			char segment[MAX_SEGMENT_LEN];
			char *request_ptr = request;
			int segment_len, file_len = 0;

			recv(new_fd, request, MAX_REQUEST_LEN, 0);
			printf("server: received request '''\n%s\n'''\n", request);

			// check request prefix
			if (strncmp(request_ptr, "GET /", 5) != 0) {
				strcpy(response_header, "HTTP/1.1 400 Bad Request\r\n\r\n");
				goto ABNORMAL_RESPONSE;
			} else {
				request_ptr = &request_ptr[5];
			}

			// extract filename from request
			int space_pos = strcspn(request_ptr, " ");
			strncpy(filename, request_ptr, space_pos);
			filename[space_pos] = '\0';
			request_ptr = &request_ptr[space_pos];

			// check request suffix (FIRST LINE)
			if (strncmp(request_ptr, " HTTP/1.1\r\n", 9) != 0) {
				strcpy(response_header, "HTTP/1.1 400 Bad Request\r\n\r\n");
				goto ABNORMAL_RESPONSE;
			}

			// open requested file
			FILE *fptr = fopen(filename, "r");
			if (fptr == NULL) {
				strcpy(response_header, "HTTP/1.1 404 Not Found\r\n\r\n");
				goto ABNORMAL_RESPONSE;
			} else {
				strcpy(response_header, "HTTP/1.1 200 OK\r\n\r\n");
			}

			// send response header
			printf("server: sending response header '''\n%s\n'''\n", response_header);
			if (send(new_fd, response_header, strlen(response_header), 0) == -1)
				perror("send");

			// read and send requested file in segments
			while ((segment_len = fread(segment, 1, MAX_SEGMENT_LEN, fptr))) {
				file_len += segment_len;
				if (send(new_fd, segment, segment_len, 0) == -1)
					perror("send");
			}
			fclose(fptr);
			printf("server: sent %d bytes of file\n", file_len);
			goto RESPONSE_SENT;

ABNORMAL_RESPONSE:
			printf("server: sending response header '''\n%s\n'''\n", response_header);
			if (send(new_fd, response_header, strlen(response_header), 0) == -1)
				perror("send");

			/* HTTP-compatible server END */

RESPONSE_SENT:
			close(new_fd);
			return 0;
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

