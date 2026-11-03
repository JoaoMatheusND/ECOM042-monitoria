/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "led_ctrl.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* Roteiro da demonstração: em cada instante, uma ação do módulo. */
struct demo_step {
	int64_t at_ms;
	int led;
	enum led_action action;
	enum led_pattern pattern;
	const char *desc;
};

static const struct demo_step demo[] = {
	{0, 0, LED_ACTION_BLINK, LED_PATTERN_SLOW, "LED0 pisca lento"},
	{0, 1, LED_ACTION_BLINK, LED_PATTERN_NORMAL, "LED1 pisca normal"},
	{0, 2, LED_ACTION_BLINK, LED_PATTERN_FAST, "LED2 pisca rapido"},
	{3000, 1, LED_ACTION_ON, LED_PATTERN_NORMAL, "LED1 fixo aceso (cancela o pisca)"},
	{5000, 1, LED_ACTION_BLINK, LED_PATTERN_FAST, "LED1 volta a piscar, agora rapido"},
	{7000, LED_CTRL_ALL, LED_ACTION_OFF, LED_PATTERN_NORMAL, "todos desligados"},
	{8000, LED_CTRL_ALL, LED_ACTION_BLINK, LED_PATTERN_NORMAL, "todos piscando normal"},
};

#define POLL_MS 5

static void print_levels(int64_t t)
{
	char line[64] = "";
	size_t len = 0;

	for (int i = 0; i < led_ctrl_count() && len < sizeof(line); i++) {
		len += snprintf(&line[len], sizeof(line) - len, " LED%d:%s", i,
				led_ctrl_get(i) ? "ON " : "off");
	}

	LOG_INF("[%5lld ms]%s", (long long)t, line);
}

int main(void)
{
	int ret = led_ctrl_init();
	size_t next = 0;
	int last = -1;
	int64_t start;
	int64_t wake;

	if (ret < 0) {
		LOG_ERR("led_ctrl_init falhou: %d", ret);
		return ret;
	}

	LOG_INF("%d LEDs, um unico k_timer (%s)", led_ctrl_count(), CONFIG_BOARD);
	start = k_uptime_get();
	wake = start;

	while (true) {
		int64_t t = k_uptime_get() - start;
		int levels = 0;

		while (next < ARRAY_SIZE(demo) && t >= demo[next].at_ms) {
			LOG_INF(">>> %s", demo[next].desc);
			(void)led_ctrl_apply(demo[next].led, demo[next].action, demo[next].pattern);
			next++;
		}

		/* Só imprime quando algum LED muda: o main não sabe de timer. */
		for (int i = 0; i < led_ctrl_count(); i++) {
			levels |= led_ctrl_get(i) << i;
		}
		if (levels != last) {
			print_levels(t);
			last = levels;
		}

		/* Acorda em instantes absolutos para a amostragem não atrasar. */
		wake += POLL_MS;
		k_sleep(K_TIMEOUT_ABS_MS(wake));
	}

	return 0;
}
