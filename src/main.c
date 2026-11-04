#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "board_io.h"
#include "shell_uart.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define SHELL_UART_NODE DT_ALIAS(minishell_uart)

int main(void)
{
	int ret;

	ret = board_io_init();
	if (ret < 0) {
		LOG_ERR("board_io_init failed (%d)", ret);
		return ret;
	}

	ret = shell_uart_start(DEVICE_DT_GET(SHELL_UART_NODE), board_io_ops());
	if (ret < 0) {
		LOG_ERR("shell_uart_start failed (%d)", ret);
		return ret;
	}

	LOG_INF("mini-shell started");
	return 0;
}
