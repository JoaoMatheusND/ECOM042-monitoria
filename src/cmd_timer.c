#include <zephyr/kernel.h>

#include "commands.h"

static void cmd_timer_execute()
{
	printk("Timer expirou\n");
}

const struct command cmd_timer = {
	.name = "timer",
	.execute = cmd_timer_execute,
	.ctx = NULL,
};
