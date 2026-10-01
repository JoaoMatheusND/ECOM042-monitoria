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

	static const char *requests[] = {
		"uart",
		"ble",
		"button",
		"timer",
	};

	for (size_t i = 0; i < ARRAY_SIZE(requests); i++) {
		commands_dispatch(requests[i]);
	}

	return 0;
}