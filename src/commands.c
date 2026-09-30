#include "commands.h"

#include <zephyr/kernel.h>

static int cmd_hello(void)
{
	printk("Hello World!\n");
	return 0;
}

static int cmd_status(void)
{
	printk("Tudo certo por aqui.\n");
	return 0;
}

const struct command commands[] = {
	{"hello", cmd_hello},
	{"status", cmd_status},
	{NULL, NULL},
};