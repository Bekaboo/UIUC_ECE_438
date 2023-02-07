/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <netdb.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT "3490"
#define BACKLOG 10


void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


char* concat(const char *s1, const char *s2, const char *s3)
{
	char *result = malloc(strlen(s1) + strlen(s2) + strlen(s3) + 1);
	strcpy(result, s1);
	strcat(result, s2);
	strcat(result, s3);
	return result;
}


int main(int argc, char *argv[])
{

	if (argc != 2) {
		fprintf(stderr, "File Name not specified\n");
		return 1;
	}

	FILE *fptr = fopen(argv[1], "r");
	fseek(fptr, 0, SEEK_END);
	unsigned long len = (unsigned long)ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	char *string = malloc(len + 1);
	fread(string, len, 1, fptr);
	string[len] = 0;

	const int n_temp = snprintf(NULL, 0, "%lu", len);
	assert(n_temp > 0);
	char buf_temp[n_temp + 1];
	int c_temp = snprintf(buf_temp, n_temp + 1, "%lu", len);
	assert(buf_temp[n_temp] == '\0');
	assert(c_temp == n_temp);

	if (fptr == NULL) {
		printf("Cannot open file\n");
		return 1;
	}

	fclose(fptr);

	char* result = concat(buf_temp,"\n\n\n", &string[0]);

	/* bind to socket */

	int sockfd, new_fd, rv, yes = 1;
	char addr[INET6_ADDRSTRLEN];
	struct addrinfo hints, *infos, *info;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &infos)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (info = infos; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			return 1;
		}
		if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if (info == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}
	freeaddrinfo(infos);

	/* bound */

	struct sigaction sa;
	struct sockaddr_storage their_addr;

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

	while (1) {
		socklen_t sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), addr, sizeof addr);
		printf("server: got connection from %s\n", addr);

		if (!fork()) {
			close(sockfd);
			if (send(new_fd, result, strlen(result), 0) == -1)
				perror("send");
			close(new_fd);
			return 0;
		}
		close(new_fd);
	}

	return 0;
}

