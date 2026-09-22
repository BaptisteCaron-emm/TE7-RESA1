#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

void echo_client(int sockfd) {
	char buff[MSG_LEN];
	int n;
	int msg_size; // Variable pour stocker la taille du message

	while (1) {
		memset(buff, 0, MSG_LEN);
		printf("Message: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n' && n < MSG_LEN - 1) {} 
		buff[n] = '\0'; 
		msg_size = strlen(buff); 

		if (send(sockfd, &msg_size, sizeof(int), 0) <= 0) {
			break;
		}
		
		if (send(sockfd, buff, msg_size, 0) <= 0) {
			break;
		}
		printf("Message sent (size: %d)!\n", msg_size);

		memset(buff, 0, MSG_LEN);
		
		int recv_size = 0;
		if (recv(sockfd, &recv_size, sizeof(int), 0) <= 0) {
			break;
		}

		if (recv(sockfd, buff, recv_size, 0) <= 0) {
			break;
		}
		printf("Received (size: %d): %s", recv_size, buff);
	}
}

int handle_connect(const char* server_name, const char* server_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(server_name, server_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char** argv) 
{
	if (argc !=3)
	{
		fprintf(stderr,"Il faut : ./client.c <server_name> <server_port>");
		exit(EXIT_FAILURE);
	}

	int sfd;
	sfd = handle_connect(argv[1],argv[2]);
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}

