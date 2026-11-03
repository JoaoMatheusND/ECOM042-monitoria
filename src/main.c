#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "io.h"
#include "transport.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	int err = io_init();
	if (err < 0) {
		LOG_ERR("io_init failed: %d", err);
		return err;
	}

	const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

	err = transport_init(uart_dev);
	if (err < 0) {
		LOG_ERR("transport_init failed: %d", err);
		return err;
	}

	LOG_INF("Mini-shell started on %s", CONFIG_BOARD);

	return 0;
}
