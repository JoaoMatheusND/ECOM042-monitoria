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

#include "led.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	int ret;

	ret = led_init();
	if (ret < 0) {
		LOG_ERR("led_init falhou (%d)", ret);
		return 0;
	}

	LOG_INF("%zu LEDs prontos", led_count());

	ret = led_blink(0U, LED_PATTERN_SLOW);
	if (ret < 0) {
		LOG_ERR("led_blink(0) falhou (%d)", ret);
		return 0;
	}

	ret = led_blink(1U, LED_PATTERN_FAST);
	if (ret < 0) {
		LOG_ERR("led_blink(1) falhou (%d)", ret);
		return 0;
	}

	ret = led_on(2U);
	if (ret < 0) {
		LOG_ERR("led_on(2) falhou (%d)", ret);
		return 0;
	}

	LOG_INF("led0 lento, led1 rapido, led2 aceso");

	return 0;
}
