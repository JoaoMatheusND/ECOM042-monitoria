#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

struct command;

struct command *command_init(const char *name, void (*execute)(void *), void *user_data);

void command_dispatch(struct command *commands[], size_t size, const char *name);
#endif
