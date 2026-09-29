#ifndef JALON2_CLIENT_LIST_H
#define JALON2_CLIENT_LIST_H

#include "msg_struct.h"
#include <netinet/in.h>

/*
 * Req2.3 : Structure exposée au serveur pour accéder au descripteur,
 * à l'adresse IP/port, au pseudo et à la date de connexion de chaque client.
 */
struct client_info {
	int fd;
	struct sockaddr_in address;
	char nickname[NICK_LEN];
	char connected_since[32];
	struct client_info *next;
};

int client_list_add(struct client_info **clients, int fd, const struct sockaddr_in *address);
void client_list_remove(struct client_info **clients, int fd);
void client_list_destroy(struct client_info **clients);

struct client_info *client_list_find_by_fd(struct client_info *clients, int fd);
struct client_info *client_list_find_by_nick(struct client_info *clients, const char *nickname);

#endif