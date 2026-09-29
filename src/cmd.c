#include "cmd.h"
#include "cmds.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

static int cmd_find_command(const char *name, struct cmd_command *out)
{
	int target_len = strlen(name);
	const struct cmd_command *table = cmd_get_command_table();

	if (table == NULL) {
		return -ENOENT; /* verificar qual retorno colocar aqui*/
	}

	while (strlen(table->name) != target_len && strncmp(table->name, name, target_len) != 0) {
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