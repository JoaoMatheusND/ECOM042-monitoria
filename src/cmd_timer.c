#include <zephyr/kernel.h>

#include "commands.h"

static int cmd_timer_execute(const command_t *self)
{
	printk("[%s] Timer expirou\n", self->name);
	return 0;
}

const command_t cmd_timer = {
	.name = "timer",
	.execute = cmd_timer_execute,
	.ctx = NULL,
};
