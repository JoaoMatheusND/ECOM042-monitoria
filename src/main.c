/*******************************************************************
 * @file main.c
 *
 * @brief Main file: inicializa e conecta placa, comandos e transporte.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "board.h"
#include "cmd.h"
#include "uart_shell.h"

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

static const struct device *const cmd_uart = DEVICE_DT_GET(DT_ALIAS(cmd_uart));

static int64_t uptime_ms(void)
{
	return k_uptime_get();
}

static const struct cmd_hal hal = {
	.led_set = board_led_set,
	.led_get = board_led_get,
	.btn_get = board_btn_get,
	.uptime_ms = uptime_ms,
};

int main(void)
{
	int ret;

	ret = board_init();
	if (ret != 0) {
		LOG_ERR("board_init falhou (%d)", ret);
		return ret;
	}

	cmd_init(&hal);

	ret = uart_shell_start(cmd_uart, cmd_dispatch);
	if (ret != 0) {
		LOG_ERR("uart_shell_start falhou (%d)", ret);
		return ret;
	}

	LOG_INF("Mini-shell pronta em %s (%s)", cmd_uart->name, CONFIG_BOARD);
	return 0;
}
