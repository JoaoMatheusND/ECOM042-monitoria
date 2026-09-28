#include "board.h"

#include <zephyr/sys/printk.h>

static bool s_led_on = false;
static bool s_button_pressed = false;
static const uint32_t s_uptime_s = 128; /**< Uptime simulado em segundos. */

void board_led_set(bool on)
{
	s_led_on = on;
	printk("(board) LED = %s\n\n", on ? "ON" : "OFF");
}

bool board_led_get(void)
{
	return s_led_on;
}

void board_button_set_state(bool pressed)
{
	s_button_pressed = pressed;
	printk("(board) Botao = %s\n\n", pressed ? "PRESSIONADO" : "SOLTO");
}

bool board_button_is_pressed(void)
{
	return s_button_pressed;
}

uint32_t board_uptime_get(void)
{
	return s_uptime_s;
}

void board_reset(void)
{
	printk("(board) reset solicitado (simulado)\n\n");
}
