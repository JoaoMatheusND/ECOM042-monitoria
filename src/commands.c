#include "board.h"
#include "commands.h"

#include <inttypes.h>
#include <stddef.h>

#include <zephyr/sys/printk.h>

cmd_status_t cmd_led_on(const cmd_args_t *args)
{
	(void)args;
	board_led_set(true);
	return CMD_OK;
}

cmd_status_t cmd_led_off(const cmd_args_t *args)
{
	(void)args;
	board_led_set(false);
	return CMD_OK;
}

cmd_status_t cmd_button_press(const cmd_args_t *args)
{
	(void)args;
	board_button_set_state(true);
	return CMD_OK;
}

cmd_status_t cmd_button_release(const cmd_args_t *args)
{
	(void)args;
	board_button_set_state(false);
	return CMD_OK;
}

cmd_status_t cmd_get_status(const cmd_args_t *args)
{
	(void)args;
	printk("led=%s botao=%s uptime=%" PRIu32 "s\n\n", board_led_get() ? "ON" : "OFF",
	       board_button_is_pressed() ? "PRESSIONADO" : "SOLTO", board_uptime_get());
	return CMD_OK;
}

cmd_status_t cmd_reset(const cmd_args_t *args)
{
	(void)args;
	board_reset();
	return CMD_OK;
}
