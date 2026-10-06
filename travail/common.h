#ifndef JALON2_COMMON_H
#define JALON2_COMMON_H

#include "msg_struct.h"
#include <stddef.h>

#define MAX_MESSAGE_SIZE 4096

void die(int val, char *msg);
int read_from_socket(int fd, void *buf, size_t msg_size);
int write_in_socket(int fd, void *buf, size_t msg_size);

int send_packet(int fd, struct message *msg, const char *payload);
int recv_packet(int fd, struct message *msg, char *payload, size_t max_len);

int is_valid_nickname(const char *nick);

#endif