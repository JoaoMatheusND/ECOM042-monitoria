#include <zephyr/kernel.h>

#include "commands.h"

static int cmd_uart_execute(const command_t *self)
{
	printk("[%s] Recebido comando via UART\n", self->name);
	return 0;
}

const command_t cmd_uart = {
	.name = "uart",
	.execute = cmd_uart_execute,
	.ctx = NULL,
};
