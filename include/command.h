
#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/* #define SIZEOF(user_data) ((user_data) ? sizeof(*user_data) : 0) */
/* #define COMMAND_INIT(name, function, user_data) \ */
/* 	command_init(name, function, user_data, SIZEOF(user_data)) */

struct command;

struct command *command_init(const char *name, void (*execute)(void *), void *user_data);

void command_destroy(struct command *self);

void command_run(struct command *self);

void command_dispatch(struct command *commands[], size_t size, const char *name);
#endif
