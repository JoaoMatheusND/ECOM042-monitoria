/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.2
 * @date 03/11/2026
 *******************************************************************/

#include <zephyr/kernel.h>

#include "board_io.h"
#include "minishell.h"
#include "uart_transport.h"

static const struct minishell_hw_ops hw_ops = {
	.led_set = board_led_set,
	.led_get = board_led_get,
	.button_read = board_button_read,
};

int main(void)
{
	int ret;

	printk("ECOM042 mini-shell (%s): comandos na uart1\n", CONFIG_BOARD);

	ret = board_io_init();
	if (ret != 0) {
		printk("board_io_init falhou (%d)\n", ret);
		return ret;
	}

	minishell_init(&hw_ops);

	ret = uart_transport_init();
	if (ret != 0) {
		printk("uart_transport_init falhou (%d)\n", ret);
		return ret;
	}

	return 0;
}
