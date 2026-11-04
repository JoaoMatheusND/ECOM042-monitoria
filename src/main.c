/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "led.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define MONITOR_STEP_MS  10
#define MONITOR_MAX_LEDS 16

struct demo_step {
	const char *desc;
	int led;
	enum led_action action;
	enum led_pattern pattern;
	int32_t hold_ms;
};

static const struct demo_step demo[] = {
	{"led0 lento, led1 padrao, led2 rapido", 0, LED_ACTION_BLINK, LED_PATTERN_SLOW, 0},
	{NULL, 1, LED_ACTION_BLINK, LED_PATTERN_NORMAL, 0},
	{NULL, 2, LED_ACTION_BLINK, LED_PATTERN_FAST, 2000},
	{"led1 ON (cancela o pisca so dele)", 1, LED_ACTION_ON, LED_PATTERN_NORMAL, 1500},
	{"todos piscando rapido", LED_ALL, LED_ACTION_BLINK, LED_PATTERN_FAST, 1000},
	{"todos OFF", LED_ALL, LED_ACTION_OFF, LED_PATTERN_NORMAL, 200},
};

/* Mostra uma linha sempre que algum LED muda (ex.: "LEDs: #.#"). */
static void monitor(int32_t hold_ms)
{
	static char last[MONITOR_MAX_LEDS + 1];
	char now[MONITOR_MAX_LEDS + 1];
	int n = MIN(led_count(), MONITOR_MAX_LEDS);
	int64_t end = k_uptime_get() + hold_ms;

	while (k_uptime_get() < end) {
		for (int i = 0; i < n; i++) {
			now[i] = led_is_on(i) ? '#' : '.';
		}
		now[n] = '\0';

		if (strcmp(now, last) != 0) {
			LOG_INF("LEDs: %s", now);
			strcpy(last, now);
		}
		k_msleep(MONITOR_STEP_MS);
	}
}

int main(void)
{
	int ret;

	LOG_INF("Demo de LEDs em %s: %d LEDs, 1 timer", CONFIG_BOARD, led_count());

	ret = led_init();
	if (ret != 0) {
		LOG_ERR("led_init falhou (%d)", ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(demo); i++) {
		if (demo[i].desc != NULL) {
			LOG_INF("--- %s", demo[i].desc);
		}
		ret = led_execute(demo[i].led, demo[i].action, demo[i].pattern);
		if (ret != 0) {
			LOG_ERR("passo %zu falhou (%d)", i, ret);
		}
		monitor(demo[i].hold_ms);
	}

	LOG_INF("--- argumento invalido (esperado -EINVAL)");
	ret = led_blink(42, LED_PATTERN_FAST);
	LOG_INF("led_blink(42) = %d", ret);

	return 0;
}
