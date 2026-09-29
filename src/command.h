#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

typedef struct command {
	const char *name;
	void (*execute)(void);
} Command;

int command_dispatch(const Command *table, size_t count, const char *name);

#endif
