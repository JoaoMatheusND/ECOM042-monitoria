#include "cmd.h"
#include "cmds.h"
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>

static int show_serial(void **args, int argc)
{

	printk("Value from serial: %d\r\n", 1024);

	return 0;
}

static int get_adc(void **args, int argc)
{
	static int adc = 250;

	int current_adc = adc;

	adc++;

	return current_adc;
}

static int add_value(void **args, int argc)
{
	int A = *((int *)args[0]);
	int B = *((int *)args[1]);

	return A + B;
}

static const struct cmd_command table[] = {
	{.name = "show_serial",
	 .description = "Show value incoming from serial",
	 .handler = show_serial},
	{.name = "get_adc", .description = "Get adc reading", .handler = get_adc},
	{.name = "add_value", .description = "Adds two values.", .handler = add_value},
	{.name = TABLE_TERMINATOR, .description = "", .handler = NULL},
};

const struct cmd_command *cmd_get_command_table()
{
	return table;
}

int cmd_list_commands()
{
	const struct cmd_command *table = cmd_get_command_table();

	if (table == NULL) {
		return -ENOENT;
	}

	while (strcmp(table->name, TABLE_TERMINATOR) != 0) {
		printk("[%s]: %s\r\n", table->name, table->description);
		table++;
	}

	return 0;
}