#include "board_io.h"
#include "command.h"
#include "transport.h"

#include <zephyr/kernel.h>

static const struct shell_io io = {
	.led_set = led_set,
	.led_get = led_get,
	.button_read = button_read,
	.uptime_get = k_uptime_get,
};

int main(void)
{
	int ret = io_init();

	if (ret < 0) {
		printk("Erro ao configurar GPIO: %d\n", ret);
		return ret;
	}

	ret = transport_init(&io);
	if (ret < 0) {
		printk("Erro ao iniciar UART: %d\n", ret);
		return ret;
	}

	return 0;
}