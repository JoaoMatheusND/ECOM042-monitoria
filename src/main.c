#include "led.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	int ret = leds_init();

	if (ret < 0) {
		LOG_ERR("Erro ao iniciar LEDs: %d", ret);
		return ret;
	}
	ret = led_blink(0, LED_SLOW);
	if (ret < 0) {
		return ret;
	}
	ret = led_blink(1, LED_FAST);
	if (ret < 0) {
		return ret;
	}
	LOG_INF("LED 0 lento (%d ms), LED 1 rapido (%d ms): um unico timer", CONFIG_LED_SLOW_MS,
		CONFIG_LED_FAST_MS);
	return 0;
}
