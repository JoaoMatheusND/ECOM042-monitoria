
#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

#define SIZEOF(user_data)                 ((user_data) ? sizeof(*(user_data)) : 0)
#define COMMAND_INIT(function, user_data) command_init(function, user_data, SIZEOF(user_data))

struct command;

struct command *command_init(void (*execute)(void *), void *user_data, size_t size);

void command_destroy(struct command *self);

void command_run(struct command *self);
#endif
