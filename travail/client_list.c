#include "client_list.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

int client_list_add(struct client_info **clients, int fd, const struct sockaddr_in *address) {
	struct client_info *client = malloc(sizeof(*client));
	time_t now;
	struct tm *tm_info;

	if (client == NULL) {
		return -1;
	}
	client->fd = fd;
	client->address = *address;
	client->nickname[0] = '\0'; // Aucun pseudo attribué au moment du accept()

	// Enregistrement de la date et heure de connexion pour /whois (Req2.6)
	now = time(NULL);
	tm_info = localtime(&now);
	if (tm_info != NULL) {
		strftime(client->connected_since, sizeof(client->connected_since),
			"%Y/%m/%d@%H:%M", tm_info);
	} else {
		strcpy(client->connected_since, "unknown");
	}

	client->next = *clients;
	*clients = client;
	return 0;
}

void client_list_remove(struct client_info **clients, int fd) {
	struct client_info **cursor = clients;

	while (*cursor != NULL) {
		if ((*cursor)->fd == fd) {
			struct client_info *removed = *cursor;
			*cursor = removed->next;
			free(removed);
			return;
		}
		cursor = &(*cursor)->next;
	}
}

void client_list_destroy(struct client_info **clients) {
	while (*clients != NULL) {
		struct client_info *removed = *clients;
		*clients = removed->next;
		free(removed);
	}
}

struct client_info *client_list_find_by_fd(struct client_info *clients, int fd) {
	struct client_info *cursor = clients;

	while (cursor != NULL) {
		if (cursor->fd == fd) {
			return cursor;
		}
		cursor = cursor->next;
	}
	return NULL;
}

struct client_info *client_list_find_by_nick(struct client_info *clients, const char *nickname) {
	struct client_info *cursor = clients;

	if (nickname == NULL || nickname[0] == '\0') {
		return NULL;
	}
	while (cursor != NULL) {
		if (strcmp(cursor->nickname, nickname) == 0) {
			return cursor;
		}
		cursor = cursor->next;
	}
	return NULL;
}