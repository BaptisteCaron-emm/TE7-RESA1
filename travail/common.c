#include "common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void die(int val, char *msg) {
	if (val < 0) {
		perror(msg);
		exit(EXIT_FAILURE);
	}
}

int read_from_socket(int fd, void *buf, size_t msg_size) {
	size_t total = 0;
	char *cursor = buf;

	while (total < msg_size) {
		ssize_t n = read(fd, cursor + total, msg_size - total);
		die((int)n, "read");
		if (n == 0) {
			return 0;
		}
		total += (size_t)n;
	}
	return (int)total;
}

int write_in_socket(int fd, void *buf, size_t msg_size) {
	size_t total = 0;
	char *cursor = buf;

	while (total < msg_size) {
		ssize_t n = write(fd, cursor + total, msg_size - total);
		die((int)n, "write");
		if (n == 0) {
			return 0;
		}
		total += (size_t)n;
	}
	return (int)total;
}

/*
 * Req2.0 : Envoie d'abord les octets de struct message, puis exactement
 * msg->pld_len octets de payload si msg->pld_len > 0.
 */
int send_packet(int fd, struct message *msg, const char *payload) {
	(void)msg_type_str; // Évite un avertissement du compilateur si non utilisé ici
	if (write_in_socket(fd, msg, sizeof(*msg)) == 0) {
		return 0;
	}
	if (msg->pld_len > 0 && payload != NULL) {
		if (write_in_socket(fd, (void *)payload, (size_t)msg->pld_len) == 0) {
			return 0;
		}
	}
	return 1;
}

/*
 * Req2.0 : Reçoit d'abord struct message, puis lit exactement msg->pld_len octets
 * de payload si msg->pld_len > 0.
 */
int recv_packet(int fd, struct message *msg, char *payload, size_t max_len) {
	if (read_from_socket(fd, msg, sizeof(*msg)) == 0) {
		return 0;
	}
	if (msg->pld_len < 0 || (size_t)msg->pld_len > max_len) {
		fprintf(stderr, "Invalid payload size: %d\n", msg->pld_len);
		return 0;
	}
	if (msg->pld_len > 0) {
		if (read_from_socket(fd, payload, (size_t)msg->pld_len) == 0) {
			return 0;
		}
		payload[msg->pld_len] = '\0';
	} else {
		payload[0] = '\0';
	}
	return 1;
}

/*
 * Req2.1 : Vérifie que le pseudo n'est ni vide, ni trop long (< NICK_LEN),
 * et ne contient que des lettres ou des chiffres (aucun espace ni caractère spécial).
 */
int is_valid_nickname(const char *nick) {
	size_t len = strlen(nick);
	if (len == 0 || len >= NICK_LEN) {
		return 0;
	}
	for (size_t i = 0; i < len; i++) {
		if (!isalnum((unsigned char)nick[i])) {
			return 0;
		}
	}
	return 1;
}