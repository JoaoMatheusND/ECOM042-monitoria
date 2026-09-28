#include <zephyr/kernel.h>

#include "commands.h"

static int cmd_button_execute(const command_t *self)
{
	printk("[%s] Botao pressionado\n", self->name);
	return 0;
}

const command_t cmd_button = {
	.name = "button",
	.execute = cmd_button_execute,
	.ctx = NULL,
};
