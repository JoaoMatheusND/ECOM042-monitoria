#include "cmd.h"
#include "commands.h"
#include "hal.h"
#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>

/**
 * @brief ECHO: reply with the text received, exactly as typed.
 *
 * The text is printed through "%s": it is user input and must never be used
 * as a format string.
 */
static int echo_handler(const char *args, struct cmd_out *out)
{
	return cmd_out_printf(out, "%s", args);
}

/**
 * @brief HELP: list the commands, one per line, as "NAME - description".
 *
 * Takes no arguments. The lines are separated by "\r\n", with none at the end.
 */
static int help_handler(const char *args, struct cmd_out *out)
{
	const struct cmd_entry *table = commands_get_table();
	const char *description;
	size_t i;

	if (!args_equal(args, "")) {
		cmd_out_printf(out, "ERR: invalid argument");
		return -EINVAL;
	}

	for (i = 0; table[i].name != NULL; i++) {
		description = (table[i].description != NULL) ? table[i].description : "";

		if (i > 0) {
			cmd_out_printf(out, "\r\n");
		}

		cmd_out_printf(out, "%s - %s", table[i].name, description);
	}

	return 0;
}

/**
 * @brief UPTIME: reply with the time since boot, in milliseconds.
 *
 * Takes no arguments.
 */
static int uptime_handler(const char *args, struct cmd_out *out)
{

	if (!args_equal(args, "")) {
		cmd_out_printf(out, "ERR: invalid argument");
		return -EINVAL;
	}

	return cmd_out_printf(out, "UPTIME: %lld ms", (long long)k_uptime_get());
}

/**
 * @brief LED: turn the LED on ("LED ON") or off ("LED OFF"), or reply its
 *        state when there are no arguments.
 */
static int led_handler(const char *args, struct cmd_out *out)
{
	int ret;

	if (args_equal(args, "")) {
		return cmd_out_printf(out, "LED: %s", hal_led_get() ? "ON" : "OFF");
	}

	if (args_equal(args, "ON")) {
		ret = hal_led_set(true);
	} else if (args_equal(args, "OFF")) {
		ret = hal_led_set(false);
	} else {
		cmd_out_printf(out, "ERR: invalid argument");
		return -EINVAL;
	}

	if (ret < 0) {
		return ret;
	}

	return cmd_out_printf(out, "OK");
}

/**
 * @brief BTN: reply with the button state, "BTN: 0" (released) or "BTN: 1" (pressed).
 *
 * Takes no arguments.
 */
static int btn_handler(const char *args, struct cmd_out *out)
{
	int state;

	if (!args_equal(args, "")) {
		cmd_out_printf(out, "ERR: invalid argument");
		return -EINVAL;
	}

	state = hal_btn_get();

	if (state < 0) {
		return state;
	}

	return cmd_out_printf(out, "BTN: %d", state);
}

/* The table is terminated by a command name == NULL */
static const struct cmd_entry commands_table[] = {
	{"HELP", "lists the available commands", help_handler},
	{"ECHO", "replies with the given text", echo_handler},
	{"UPTIME", "replies with the time since boot, in ms", uptime_handler},
	{"LED", "turns the LED on or off (ON/OFF), or replies its state", led_handler},
	{"BTN", "replies with the button state (0 or 1)", btn_handler},
	{NULL, NULL, NULL},
};

const struct cmd_entry *commands_get_table(void)
{
	return commands_table;
}