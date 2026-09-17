#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H

struct channel;
struct channel_list;
struct user;

void channel_list_init(struct channel_list *channels);
struct channel *channel_list_find(struct channel_list *channels, const char *name);
int channel_list_create(struct channel_list *channels, const char *name);
int channel_list_join(struct channel_list *channels, const char *name, struct user *user);
int channel_list_leave(struct channel_list *channels, struct user *user);
int channel_list_broadcast(struct channel_list *channels, const char *name,
                           struct user *sender, const void *payload);
void channel_list_destroy(struct channel_list *channels);

#endif
