#include "commands.h"

#include <zephyr/kernel.h>

static void cmd_hello(void)
{
	printk("Hello World!\n");
}

static void cmd_status(void)
{
	printk("Tudo certo por aqui.\n");
}

const command_t commands[] = {
	{"hello", cmd_hello},
	{"status", cmd_status},
	{NULL, NULL},
};