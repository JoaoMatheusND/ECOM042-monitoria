#include "command.h"
#include <stdio.h>

void handle_uart()
{
	printf("[LOG] Handling UART...\n");
}

void handle_ble()
{
	printf("[LOG] Handling BLE...\n");
}

void handle_button()
{
	printf("[LOG] Handling button...\n");
}

void handle_timer()
{
	printf("[LOG] Handling timer...\n");
}
const command_handler_t command_table[COMMAND_TABLE_SIZE] = {
	[UART_CMD] = &handle_uart,
	[TIMER_CMD] = &handle_timer,
	[BUTTON_CMD] = &handle_button,
	[BLE_CMD] = &handle_ble,
};

uint16_t dispatch(command_t command)
{
	if (command_table[command] == NULL) {
		printf("[ERROR] No command with ID: %u\n", command);
		return ENOCMD;
	}

	(*command_table[command])();

	return 0;
}
