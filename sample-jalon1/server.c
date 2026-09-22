#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

void die(int ret, char* msg){
    if (ret < 0){
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int first_fds_available(struct pollfd * fds, int size){
    for (int i = 0 ; i < size ; i++){
        // Note : j'ai corrigé le "!= -1" du code de cours en "== -1". 
        // Pour trouver une place disponible, il faut chercher une case vide (-1).
        if (fds[i].fd == -1){ 
            return i;
        }
    }
    return -1;
}

void echo_server(int sockfd) {
	char buff[MSG_LEN];
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
		printf("Received: %s", buff);
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
	}
}

int handle_bind(const char* server_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char** argv) {
	int sfd;

	if (argc !=2)
	{
		fprintf(stderr,"Il faut : ./server <server_port>");
		exit(EXIT_FAILURE);
	}
	sfd = handle_bind(argv[1]);
	if ((listen(sfd, SOMAXCONN)) != 0) 
	{
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}

	struct pollfd fds[SIZE_TAB];
	fds[0].fd = sfd;
    fds[0].events = POLLIN; 
    fds[0].revents = 0;
    
    for (int i = 1; i < SIZE_TAB; i++)
	{ 
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    while(1){
        int nb_fds = poll(fds, SIZE_TAB, -1); 
		printf("Nb active fd : %d\n", nb_fds);
        for (int i = 0 ; i < SIZE_TAB ; i++){
            
            if (i == 0 && (fds[0].revents & POLLIN))
            {
                fds[i].revents = 0;
                struct sockaddr_in client_addr;
                socklen_t addrlen = sizeof(struct sockaddr_in);
                
                int new_client = accept(sfd, (struct sockaddr*) &client_addr, &addrlen);
                die(new_client, "Accept"); 

                int fd_number = first_fds_available(fds, SIZE_TAB);

                if (fd_number != -1) {
                    fds[fd_number].fd = new_client;
                    fds[fd_number].events = POLLIN;
                    fds[fd_number].revents = 0;

                    printf("fds[%d].fd = %d \n", fd_number, fds[fd_number].fd); 
                    printf("New client (ADDR %s:%hu) with FD : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), new_client);
                } 
                else 
                {
                    close(new_client);
                }
            }
            if (i!=0 && (fds[i].revents & POLLIN))
            {
                fds[i].revents = 0;
                
                int msg_size = 0;
                
                int size_read = recv(fds[i].fd, &msg_size, sizeof(int), 0);
                if (size_read <= 0)
                {
                    printf("Connection ended\n");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    fds[i].revents = 0;
                } 
                else 
                {

                    if (msg_size > 0 && msg_size <= MSG_LEN) 
                    {

                        char *buff = malloc(msg_size + 1);
                        memset(buff, 0, msg_size + 1);
                        

                        recv(fds[i].fd, buff, msg_size, 0);
                        printf("Received from FD %d (size %d): %s", fds[i].fd, msg_size, buff);

                        send(fds[i].fd, &msg_size, sizeof(int), 0);
                        

                        send(fds[i].fd, buff, msg_size, 0); 
                        
                        free(buff); 
                    } 
                    else 
                    {
                        printf("Received invalid size from FD %d\n", fds[i].fd);
                    }
                }
            }
        }
    }
} 