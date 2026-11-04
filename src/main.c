/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "led/led.h"
#include "led/led_cmd.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
	int ret = led_init();

	if (ret != 0) {
		LOG_ERR("LED init failed (%d)", ret);
		return ret;
	}

	led_blink(0, LED_PROFILE_SLOW);
	led_blink(1, LED_PROFILE_NORMAL);
	led_blink(2, LED_PROFILE_FAST);

	return 0;
}
