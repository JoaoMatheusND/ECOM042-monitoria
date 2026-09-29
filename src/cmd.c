#include "cmd.h"
#include "cmds.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

/**
 * @brief Look up a command in the command table by name.
 *
 * @param[in]  name Name of the command to find.
 * @param[out] out  Receives a copy of the command entry when found.
 *
 * @retval 0       Command found.
 * @retval -EINVAL Command not found.
 * @retval -ENOENT Command table is unavailable.
 */
static int cmd_find_command(const char *name, struct cmd_command *out)
{
	const struct cmd_command *table = cmd_get_command_table();

	if (table == NULL) {
		return -ENOENT; /* verificar qual retorno colocar aqui*/
	}

	while (strcmp(table->name, name) != 0) {
		if (strcmp(table->name, TABLE_TERMINATOR) == 0) {
			return -EINVAL;
		}
		table++;
	}

	*out = *table;

	return 0;
}

int cmd_help(const char *name)
{
	struct cmd_command cmd;
	int ret;

	ret = cmd_find_command(name, &cmd);

	if (ret != 0) {
		return ret;
	}

	printk("[%s]: %s\r\n", cmd.name, cmd.description);

	return 0;
}

int cmd_execute(const char *name, void **args, int argc)
{
	struct cmd_command cmd;
	int ret;

	ret = cmd_find_command(name, &cmd);
	if (ret != 0) {
		return ret;
	}

	return cmd.handler(args, argc);
}