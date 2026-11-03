/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "board_io.h"
#include "dispatch.h"
#include "uart_transport.h"

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

int main(void)
{
	const struct device *uart = DEVICE_DT_GET(DT_ALIAS(shell_uart));
	int rc;

	rc = board_io_init();
	if (rc != 0) {
		LOG_ERR("Failed to initialize board I/O: %d", rc);
		return rc;
	}

	minish_set_hw_ops(board_io_ops());

	rc = uart_transport_start(uart);
	if (rc != 0) {
		LOG_ERR("Failed to start shell UART: %d", rc);
		return rc;
	}

	LOG_INF("Mini-shell started on %s", uart->name);
	return 0;
}
