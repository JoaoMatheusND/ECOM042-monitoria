#include "commands.h"
#include <zephyr/sys/util.h>

#include <stddef.h>
#include <string.h>

static const command_t *const command_table[CMD_COUNT] = {
	[CMD_UART] = &cmd_uart,
	[CMD_BLE] = &cmd_ble,
	[CMD_BUTTON] = &cmd_button,
	[CMD_TIMER] = &cmd_timer,
};

int commands_dispatch(const char *name)
{

	for (size_t i = 0; i < ARRAY_SIZE(command_table); i++) {
		const command_t *ptr = command_table[i];

		if (strcmp(ptr->name, name) == 0) {
			command_execute(ptr);
			return 0;
		}
	}

	return -1;
}
