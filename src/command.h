#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

typedef struct {
	const char *name;
	void (*execute)(void *ctx);
	void *ctx;
} command_t;

int command_dispatch(const command_t *table, size_t count, const char *name);

#endif
