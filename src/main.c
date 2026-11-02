/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "commands/cmd.h"
#include "commands/commands.h"
#include "hal.h"
#include "transport.h"

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

int main(void)
{
	int ret;

	LOG_INF("Hello World! %s", CONFIG_BOARD);

	ret = hal_init();

	if (ret < 0) {
		LOG_ERR("hal_init failed: %d", ret);
		return ret;
	}

	ret = transport_init(uart_dev, commands_get_table());

	if (ret < 0) {
		LOG_ERR("transport_init failed: %d", ret);
		return ret;
	}

	return 0;
}
