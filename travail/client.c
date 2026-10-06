#include "common.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int setup_connection(const char *server_ip, const char *server_port) {
	int socket_fd;
	int result;
	struct sockaddr_in server_address;

	printf("Using server IPv4 address %s.\n", server_ip);
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	result = inet_aton(server_ip, &server_address.sin_addr);
	if (result == 0) {
		fprintf(stderr, "Invalid IPv4 address: %s\n", server_ip);
		return -1;
	}

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	die(socket_fd, "socket");
	printf("TCP socket created.\n");

	server_address.sin_port = htons((unsigned short)atoi(server_port));
	result = connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
	die(result, "connect");
	printf("Connected to %s:%s.\n", inet_ntoa(server_address.sin_addr), server_port);
	printf("[Server] : please login with /nick <your pseudo>\n");
	return socket_fd;
}

int read_server_message(int socket_fd, char current_nick[NICK_LEN]) {
	struct message msg;
	char payload[MAX_MESSAGE_SIZE + 1];

	if (recv_packet(socket_fd, &msg, payload, MAX_MESSAGE_SIZE) == 0) {
		printf("Disconnected from server.\n");
		return 0;
	}

	if (msg.type == NICKNAME_NEW && msg.infos[0] != '\0') {
		strncpy(current_nick, msg.infos, NICK_LEN - 1);
		current_nick[NICK_LEN - 1] = '\0';
	}

	if (msg.pld_len > 0) {
		write(STDOUT_FILENO, payload, (size_t)msg.pld_len);
	}
	return 1;
}


int get_and_send_user_message(int socket_fd, char current_nick[NICK_LEN]) {
	char input[MAX_MESSAGE_SIZE + 1];
	ssize_t bytes_read;
	struct message msg;

	bytes_read = read(STDIN_FILENO, input, MAX_MESSAGE_SIZE);
	die((int)bytes_read, "read stdin");
	if (bytes_read == 0) {
		return 0;
	}
	input[bytes_read] = '\0';

	size_t len = (size_t)bytes_read;
	if (len > 0 && input[len - 1] == '\n') {
		input[len - 1] = '\0';
		len--;
	}

	if (len == 0) {
		return 1;
	}

	memset(&msg, 0, sizeof(msg));
	strncpy(msg.nick_sender, current_nick, NICK_LEN - 1);

	if (strcmp(input, "/quit") == 0) {
		msg.type = ECHO_SEND;
		msg.pld_len = 5;
		send_packet(socket_fd, &msg, "/quit");
		return 0;
	}

	if (strncmp(input, "/nick ", 6) == 0 || strcmp(input, "/nick") == 0) {
		const char *new_nick = (len > 6) ? (input + 6) : "";
		if (!is_valid_nickname(new_nick)) {
			fprintf(stderr, "[Client] : Invalid nickname (must be alphanumeric, no spaces, < %d chars).\n", NICK_LEN);
			return 1;
		}
		msg.type = NICKNAME_NEW;
		msg.pld_len = 0;
		strncpy(msg.infos, new_nick, INFOS_LEN - 1);
		return send_packet(socket_fd, &msg, NULL);
	}

	if (current_nick[0] == '\0') {
		fprintf(stderr, "[Client] : You must set a nickname first using /nick <pseudo>\n");
		return 1;
	}

	if (strcmp(input, "/who") == 0) {
		msg.type = NICKNAME_LIST;
		msg.pld_len = 0;
		return send_packet(socket_fd, &msg, NULL);
	}

	if (strncmp(input, "/whois ", 7) == 0 || strcmp(input, "/whois") == 0) {
		const char *target = (len > 7) ? (input + 7) : "";
		if (target[0] == '\0') {
			fprintf(stderr, "Usage: /whois <pseudo>\n");
			return 1;
		}
		msg.type = NICKNAME_INFOS;
		msg.pld_len = 0;
		strncpy(msg.infos, target, INFOS_LEN - 1);
		return send_packet(socket_fd, &msg, NULL);
	}

	if (strncmp(input, "/msgall ", 8) == 0 || strcmp(input, "/msgall") == 0) {
		const char *text = (len > 8) ? (input + 8) : "";
		if (text[0] == '\0') {
			fprintf(stderr, "Usage: /msgall <message>\n");
			return 1;
		}
		msg.type = BROADCAST_SEND;
		msg.pld_len = (int)strlen(text);
		return send_packet(socket_fd, &msg, text);
	}

	if (strncmp(input, "/msg ", 5) == 0 || strcmp(input, "/msg") == 0) {
		const char *args = (len > 5) ? (input + 5) : "";
		const char *space = strchr(args, ' ');
		if (space == NULL || *(space + 1) == '\0') {
			fprintf(stderr, "Usage: /msg <pseudo> <message>\n");
			return 1;
		}
		size_t target_len = (size_t)(space - args);
		if (target_len == 0 || target_len >= INFOS_LEN) {
			fprintf(stderr, "[Client] : Invalid target nickname length.\n");
			return 1;
		}
		const char *text = space + 1;
		msg.type = UNICAST_SEND;
		memcpy(msg.infos, args, target_len);
		msg.infos[target_len] = '\0';
		msg.pld_len = (int)strlen(text);
		return send_packet(socket_fd, &msg, text);
	}

	msg.type = ECHO_SEND;
	msg.pld_len = (int)strlen(input);
	return send_packet(socket_fd, &msg, input);
}

void client_poll_loop(int socket_fd) {
	struct pollfd watched[2];
	int running = 1;
	char current_nick[NICK_LEN];

	current_nick[0] = '\0';

	watched[0].fd = STDIN_FILENO;
	watched[0].events = POLLIN;
	watched[1].fd = socket_fd;
	watched[1].events = POLLIN;

	while (running) {
		int ready = poll(watched, 2, -1);
		die(ready, "poll");

		if ((watched[1].revents & POLLIN) != 0) {
			running = read_server_message(socket_fd, current_nick);
		}

		if (running && (watched[0].revents & POLLIN) != 0) {
			running = get_and_send_user_message(socket_fd, current_nick);
		}

		if ((watched[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
		    (watched[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
			running = 0;
		}
	}
}

int main(int argc, char **argv) {
	int socket_fd;

	if (argc != 3) {
		fprintf(stderr, "Usage: ./client <server_ipv4> <server_port>\n");
		return EXIT_FAILURE;
	}
	socket_fd = setup_connection(argv[1], argv[2]);
	if (socket_fd < 0) {
		return EXIT_FAILURE;
	}
	client_poll_loop(socket_fd);
	close(socket_fd);
	return EXIT_SUCCESS;
}