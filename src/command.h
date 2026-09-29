#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

struct command {
	const char *name;
	void (*execute)(void);
};

int command_dispatch(const struct command *table, size_t count, const char *name);

#endif
