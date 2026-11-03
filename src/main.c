/*******************************************************************
 * @file main.c
 *
 * @brief Main file: inicializa e conecta as camadas da mini-shell.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include "board_io.h"
#include "cmd.h"
#include "uart_transport.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

static const struct device *const cmd_uart = DEVICE_DT_GET(DT_CHOSEN(app_cmd_uart));

static const struct cmd_io io = {
	.led_set = board_io_led_set,
	.led_get = board_io_led_get,
	.btn_get = board_io_btn_get,
	.uptime_ms = k_uptime_get,
};

static void on_line(const char *line, char *out, size_t out_size)
{
	(void)cmd_execute(&io, line, out, out_size);
}

int main(void)
{
	int ret;

	ret = board_io_init();
	if (ret < 0) {
		LOG_ERR("falha ao iniciar LED/botao (%d)", ret);
		return ret;
	}

	ret = uart_transport_start(cmd_uart, on_line);
	if (ret < 0) {
		LOG_ERR("falha ao iniciar a UART de comandos (%d)", ret);
		return ret;
	}

	LOG_INF("mini-shell pronta em %s (%s)", cmd_uart->name, CONFIG_BOARD);

	return 0;
}
