/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include "board.h"
#include "shell.h"
#include "uart_shell.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* O main nao interpreta comando e nao le a UART. So liga as camadas:
 * a placa preenche os ponteiros, o transporte chama o parser. */
int main(void)
{
	static const struct shell_io io = {
		.led_set = board_led_set,
		.led_get = board_led_get,
		.button_get = board_button_get,
		.uptime_ms = board_uptime_ms,
	};
	int ret;

	ret = board_init();
	if (ret < 0) {
		printk("board_init failed: %d\n", ret);
		return 0;
	}

	ret = uart_shell_start(&io);
	if (ret < 0) {
		printk("uart_shell_start failed: %d\n", ret);
		return 0;
	}

	k_sleep(K_FOREVER);
	return 0;
}
