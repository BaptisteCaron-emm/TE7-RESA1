#include "common.h"
#include "client_list.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 128

int setup_listening_socket(int port) {
	int listen_fd;
	int result;
	int opt = 1;
	struct sockaddr_in server_address;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	die(listen_fd, "socket");
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	printf("TCP listening socket created.\n");

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons((unsigned short)port);
	result = bind(listen_fd, (struct sockaddr *)&server_address, sizeof(server_address));
	die(result, "bind");
	printf("Socket bound to port %d.\n", port);

	result = listen(listen_fd, 20);
	die(result, "listen");
	printf("Listening for client connections.\n");
	return listen_fd;
}

void accept_and_insert_client(int listen_fd, struct pollfd poll_fds[MAX_CLIENTS], struct client_info **clients) {
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);
	int client_fd = accept(listen_fd, (struct sockaddr *)&client_address, &client_address_length);
	int slot;

	die(client_fd, "accept");
	for (slot = 1; slot < MAX_CLIENTS; slot++) {
		if (poll_fds[slot].fd < 0) {
			if (client_list_add(clients, client_fd, &client_address) < 0) {
				close(client_fd);
				die(-1, "malloc client information");
			}
			poll_fds[slot].fd = client_fd;
			poll_fds[slot].events = POLLIN;
			poll_fds[slot].revents = 0;
			printf("Accepted client %s:%u on slot %d.\n",
				inet_ntoa(client_address.sin_addr),
				(unsigned int)ntohs(client_address.sin_port), slot);
			break;
		}
	}
	if (slot == MAX_CLIENTS) {
		fprintf(stderr, "Client limit reached. Closing the new connection.\n");
		close(client_fd);
	}
}

static int send_server_reply(int fd, enum msg_type type, const char *sender, const char *infos, const char *text) {
	struct message reply;

	memset(&reply, 0, sizeof(reply));
	reply.type = type;
	if (sender != NULL) {
		strncpy(reply.nick_sender, sender, NICK_LEN - 1);
	}
	if (infos != NULL) {
		strncpy(reply.infos, infos, INFOS_LEN - 1);
	}
	reply.pld_len = (text != NULL) ? (int)strlen(text) : 0;
	return send_packet(fd, &reply, text);
}

int handle_client_message(int client_fd, struct client_info **clients) {
	struct message msg;
	char payload[MAX_MESSAGE_SIZE + 1];
	char out_buf[MAX_MESSAGE_SIZE + 256];
	struct client_info *sender = client_list_find_by_fd(*clients, client_fd);

	if (sender == NULL) {
		return 1;
	}

	if (recv_packet(client_fd, &msg, payload, MAX_MESSAGE_SIZE) == 0) {
		fprintf(stderr, "Client %d : Socket close\n", client_fd);
		return 1;
	}

	if (msg.pld_len > 0) {
		printf("[%s] : %s\n",
			(sender->nickname[0] != '\0') ? sender->nickname : "anonymous",
			payload);
	}

	switch (msg.type) {
	case NICKNAME_NEW: {
		const char *requested_nick = msg.infos;
		struct client_info *existing = client_list_find_by_nick(*clients, requested_nick);

		if (!is_valid_nickname(requested_nick)) {
			snprintf(out_buf, sizeof(out_buf),
				"[Server] : Invalid nickname '%s' (alphanumeric only, no spaces).\n", requested_nick);
			send_server_reply(client_fd, NICKNAME_NEW, "Server", "", out_buf);
		} else if (existing != NULL && existing->fd != client_fd) {
			snprintf(out_buf, sizeof(out_buf),
				"[Server] : Error, nickname '%s' is already taken.\n", requested_nick);
			send_server_reply(client_fd, NICKNAME_NEW, "Server", "", out_buf);
		} else {
			int was_unregistered = (sender->nickname[0] == '\0');
			strncpy(sender->nickname, requested_nick, NICK_LEN - 1);
			sender->nickname[NICK_LEN - 1] = '\0';

			if (was_unregistered) {
				snprintf(out_buf, sizeof(out_buf),
					"[Server] : Welcome on the chat %s\n", sender->nickname);
			} else {
				snprintf(out_buf, sizeof(out_buf),
					"[Server] : Your nickname is now %s\n", sender->nickname);
			}
			printf("[%s] connected on slot (fd=%d).\n", sender->nickname, client_fd);
			send_server_reply(client_fd, NICKNAME_NEW, "Server", sender->nickname, out_buf);
		}
		break;
	}

	case NICKNAME_LIST: {
		size_t offset = (size_t)snprintf(out_buf, sizeof(out_buf), "[Server] : Online users are\n");
		for (struct client_info *curr = *clients; curr != NULL; curr = curr->next) {
			if (curr->nickname[0] != '\0' && offset < sizeof(out_buf) - NICK_LEN - 20) {
				offset += (size_t)snprintf(out_buf + offset, sizeof(out_buf) - offset,
					"           - %s\n", curr->nickname);
			}
		}
		send_server_reply(client_fd, NICKNAME_LIST, "Server", "", out_buf);
		break;
	}

	case NICKNAME_INFOS: {
		struct client_info *target = client_list_find_by_nick(*clients, msg.infos);
		if (target == NULL) {
			snprintf(out_buf, sizeof(out_buf),
				"[Server] : User %s does not exist.\n", msg.infos);
		} else {
			snprintf(out_buf, sizeof(out_buf),
				"[Server] : %s connected since %s with IP address %s and port number %u\n",
				target->nickname,
				target->connected_since,
				inet_ntoa(target->address.sin_addr),
				(unsigned int)ntohs(target->address.sin_port));
		}
		send_server_reply(client_fd, NICKNAME_INFOS, "Server", msg.infos, out_buf);
		break;
	}

	case BROADCAST_SEND: {
		snprintf(out_buf, sizeof(out_buf), "[%s] : %s\n", sender->nickname, payload);
		for (struct client_info *curr = *clients; curr != NULL; curr = curr->next) {
			if (curr->fd != client_fd && curr->nickname[0] != '\0') {
				send_server_reply(curr->fd, BROADCAST_SEND, sender->nickname, "", out_buf);
			}
		}
		break;
	}

	case UNICAST_SEND: {
		struct client_info *target = client_list_find_by_nick(*clients, msg.infos);
		if (target == NULL) {
			snprintf(out_buf, sizeof(out_buf),
				"[Server] : User %s does not exist\n", msg.infos);
			send_server_reply(client_fd, UNICAST_SEND, "Server", msg.infos, out_buf);
		} else {
			snprintf(out_buf, sizeof(out_buf), "[%s] : %s\n", sender->nickname, payload);
			send_server_reply(target->fd, UNICAST_SEND, sender->nickname, target->nickname, out_buf);
		}
		break;
	}

	case ECHO_SEND: {
		if (strcmp(payload, "/quit") == 0) {
			printf("Client %d (%s) requested to quit.\n", client_fd, sender->nickname);
			return 1;
		}
		snprintf(out_buf, sizeof(out_buf), "[%s] : %s\n", sender->nickname, payload);
		if (send_server_reply(client_fd, ECHO_SEND, sender->nickname, "", out_buf) == 0) {
			return 1;
		}
		break;
	}

	case FILE_REQUEST: {
		struct client_info *target = client_list_find_by_nick(*clients, msg.infos);
		if (target == NULL) {
			snprintf(out_buf, sizeof(out_buf),
				"[Server] : User %s does not exist\n", msg.infos);
			send_server_reply(client_fd, UNICAST_SEND, "Server", msg.infos, out_buf);
		} else {
			send_server_reply(target->fd, FILE_REQUEST, sender->nickname, msg.infos, payload);
		}
		break;
	}

	default:
		fprintf(stderr, "Unhandled message type: %d\n", msg.type);
		break;
	}

	return 0;
}

void server_poll_loop(int listen_fd, struct pollfd poll_fds[MAX_CLIENTS],
		struct client_info **clients) {
	int running = 1;

	for (int i = 0; i < MAX_CLIENTS; i++) {
		poll_fds[i].fd = -1;
		poll_fds[i].events = 0;
		poll_fds[i].revents = 0;
	}
	poll_fds[0].fd = listen_fd;
	poll_fds[0].events = POLLIN;

	while (running) {
		int ready = poll(poll_fds, MAX_CLIENTS, -1);
		die(ready, "poll");

		if ((poll_fds[0].revents & POLLIN) != 0) {
			accept_and_insert_client(listen_fd, poll_fds, clients);
		}

		for (int slot = 1; slot < MAX_CLIENTS; slot++) {
			short returned_events = poll_fds[slot].revents;
			int close_connection = 0;
			if (poll_fds[slot].fd < 0) {
				continue;
			}

			if ((returned_events & POLLIN) != 0) {
				close_connection = handle_client_message(poll_fds[slot].fd, clients);
			}
			if ((returned_events & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
				close_connection = 1;
			}
			if (close_connection) {
				int client_fd = poll_fds[slot].fd;
				close(client_fd);
				client_list_remove(clients, client_fd);
				poll_fds[slot].fd = -1;
				poll_fds[slot].events = 0;
				poll_fds[slot].revents = 0;
			}
		}
		if ((poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
			running = 0;
		}
	}

	for (int slot = 1; slot < MAX_CLIENTS; slot++) {
		if (poll_fds[slot].fd >= 0) {
			close(poll_fds[slot].fd);
			poll_fds[slot].fd = -1;
		}
	}
	client_list_destroy(clients);
}

int main(int argc, char **argv) {
	struct pollfd poll_fds[MAX_CLIENTS];
	struct client_info *clients = NULL;
	int port;
	int listen_fd;

	if (argc != 2) {
		fprintf(stderr, "Usage: ./server <server_port>\n");
		return EXIT_FAILURE;
	}
	port = atoi(argv[1]);
	if (port < 1 || port > 65535) {
		fprintf(stderr, "Invalid port\n");
		return EXIT_FAILURE;
	}

	listen_fd = setup_listening_socket(port);
	server_poll_loop(listen_fd, poll_fds, &clients);
	close(listen_fd);
	return EXIT_SUCCESS;
}