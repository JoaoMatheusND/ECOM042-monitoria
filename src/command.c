#include "command.h"

#include <stddef.h>
#include <string.h>

/**
 * @brief Look up @p name in @p table and run the matching command.
 *
 * @param table Pointer to the first entry of a NULL-terminated command table.
 * @param name  Command name to search for.
 *
 * @return The value returned by the executed command, or -1 if no command
 *         with @p name is found.
 */
int command_run(const command_t *table, const char *name)
{
	size_t i;

	for (i = 0; table[i].name != NULL; i++) {
		if (strcmp(table[i].name, name) == 0) {
			return table[i].exec();
		}
	}

	return -1;
}
