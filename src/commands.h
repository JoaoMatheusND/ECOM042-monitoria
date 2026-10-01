#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "command.h"

enum command_id {
	CMD_UART = 0,
	CMD_BLE,
	CMD_BUTTON,
	CMD_TIMER,
	CMD_COUNT,
};

extern const struct command cmd_uart;
extern const struct command cmd_ble;
extern const struct command cmd_button;
extern const struct command cmd_timer;

int commands_dispatch(const char *name);

#endif /* COMMANDS_H_ */
