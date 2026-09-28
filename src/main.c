/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author matheus
 * @version 0.1
 * @date 27/09/2027
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "commands.h"

int main(void)
{

	static const enum command_id events[] = {CMD_UART, CMD_BLE, CMD_BUTTON, CMD_TIMER};
	static const char *requests[] = {
		"uart",
		"ble",
		"button",
		"timer",
	};

	for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
		(void)commands_dispatch(events[i], requests[i]);
	}

	return 0;
}