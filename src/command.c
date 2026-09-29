#include "command.h"

#include <errno.h>
#include <string.h>

int command_dispatch(const Command *table, size_t count, const char *name)
{
	/* Procura o nome e chama a funcao do comando. */
	for (size_t i = 0; i < count; i++) {
		if (strcmp(table[i].name, name) == 0) {
			table[i].execute();
			return 0;
		}
	}

	return -ENOENT;
}
