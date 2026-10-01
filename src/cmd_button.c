#include <zephyr/kernel.h>

#include "commands.h"

static void cmd_button_execute()
{
	printk("Botao pressionado\n");
}

const struct command cmd_button = {
	.name = "button",
	.execute = cmd_button_execute,
	.ctx = NULL,
};
