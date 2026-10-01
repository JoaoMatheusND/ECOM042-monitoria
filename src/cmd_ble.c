#include <zephyr/kernel.h>

#include "commands.h"

static void cmd_ble_execute()
{
	printk("Recebido comando via BLE\n");
}

const command_t cmd_ble = {
	.name = "ble",
	.execute = cmd_ble_execute,
	.ctx = NULL,
};
