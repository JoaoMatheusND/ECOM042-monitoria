#include <zephyr/kernel.h>

#include "commands.h"

static void cmd_uart_execute()
{
	printk("Recebido comando via UART\n");
}

const command_t cmd_uart = {
	.name = "uart",
	.execute = cmd_uart_execute,
	.ctx = NULL,
};
