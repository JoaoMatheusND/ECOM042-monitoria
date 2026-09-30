#include "command.h"
#include <stdio.h>
#include <zephyr/kernel.h>

void handle_uart()
{
	printk("[LOG] Handling UART...");
}

void handle_ble()
{
	printk("[LOG] Handling BLE...");
}

void handle_button()
{
	printk("[LOG] Handling button...");
}

void handle_timer()
{
	printk("[LOG] Handling timer...");
}

uint16_t dispatch(command_t command)
{
	if (command_table[command] == NULL) {
		printk("[ERROR] No command with ID: %u", command);
		return ENOCMD;
	}

	(*command_table[command])();

	return 0;
}
