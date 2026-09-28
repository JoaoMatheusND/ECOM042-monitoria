#include "commands.h"
#include <zephyr/sys/util.h>

#include <stddef.h>

static const command_t *const command_table[CMD_COUNT] = {
	[CMD_UART] = &cmd_uart,
	[CMD_BLE] = &cmd_ble,
	[CMD_BUTTON] = &cmd_button,
	[CMD_TIMER] = &cmd_timer,
};

const command_t *commands_get(enum command_id id)
{
	if ((unsigned int)id >= CMD_COUNT) {
		return NULL;
	}

	return command_table[id];
}

int commands_dispatch(enum command_id id, const char *name)
{

	for (size_t i = 0; i < ARRAY_SIZE(command_table); i++) {
		command_t *ptr = &command_table[i];

		if (ptr->name == NULL || strcmp(ptr->name, name) != 0) {
			continue;
		}

		if (ptr->execute == NULL) {
			return -1;
		}

		command_execute(ptr);
	}

	return 0;
}
