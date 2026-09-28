#include <zephyr/kernel.h>

#include "commands.h"

static int cmd_ble_execute(const command_t *self)
{
	printk("[%s] Recebido comando via BLE\n", self->name);
	return 0;
}

const command_t cmd_ble = {
	.name = "ble",
	.execute = cmd_ble_execute,
	.ctx = NULL,
};
