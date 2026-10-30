/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "board_io.h"
#include "uart_link.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	int ret;

	ret = board_io_init();
	if (ret < 0) {
		LOG_ERR("board_io_init falhou (%d)", ret);
		return 0;
	}

	ret = uart_link_init();
	if (ret < 0) {
		LOG_ERR("uart_link_init falhou (%d)", ret);
		return 0;
	}

	LOG_INF("mini-shell pronta na uart1");

	return 0;
}
