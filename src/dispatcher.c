#include "dispatcher.h"

#include <string.h>

cmd_status_t command_dispatch(const command_t *table, size_t table_len, const char *name,
			      const cmd_args_t *args)
{
	if (table == NULL || name == NULL) {
		return CMD_ERR_NOT_FOUND;
	}

	for (size_t i = 0; i < table_len; i++) {
		if (strcmp(table[i].name, name) == 0) {
			return table[i].execute(args);
		}
	}
	return CMD_ERR_NOT_FOUND;
}
