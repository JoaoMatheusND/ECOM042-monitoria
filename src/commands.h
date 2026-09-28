#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "command.h"

enum command_id {
	CMD_UART = 0,
	CMD_BLE,
	CMD_BUTTON,
	CMD_TIMER,
	CMD_COUNT
};

extern const command_t cmd_uart;
extern const command_t cmd_ble;
extern const command_t cmd_button;
extern const command_t cmd_timer;

const command_t *commands_get(enum command_id id);

int commands_dispatch(enum command_id id, char *name);

#endif /* COMMANDS_H_ */
