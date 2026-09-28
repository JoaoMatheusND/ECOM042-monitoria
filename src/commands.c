#include "commands.h"

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

int commands_dispatch(enum command_id id)
{
	return command_execute(commands_get(id));
}
