/*******************************************************************
 * @file main.c
 *
 * @brief Main file: demonstracao do modulo de LED.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/
#include "led.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_GPIO_EMUL
#include <zephyr/drivers/gpio/gpio_emul.h>
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

struct demo_step {
	const char *desc;
	int (*run)(void);
	int32_t duration_ms;
};

static int step_blink_mixed(void)
{
	int ret = led_blink(0, LED_PATTERN_SLOW);

	ret = ret ? ret : led_blink(1, LED_PATTERN_NORMAL);
	return ret ? ret : led_blink(2, LED_PATTERN_FAST);
}

static int step_led1_on(void)
{
	return led_on(1);
}

static int step_led1_fast(void)
{
	return led_blink(1, LED_PATTERN_FAST);
}

static int step_all_off(void)
{
	return led_off(LED_ALL);
}

static int step_all_toggle(void)
{
	return led_toggle(LED_ALL);
}

static int step_all_normal(void)
{
	return led_blink(LED_ALL, LED_PATTERN_NORMAL);
}

static const struct demo_step steps[] = {
	{"LED0 lento, LED1 padrao, LED2 rapido", step_blink_mixed, 4000},
	{"LED1 ligado fixo (LED0 e LED2 seguem na mesma fase)", step_led1_on, 3000},
	{"LED1 volta a piscar rapido", step_led1_fast, 3000},
	{"todos desligados", step_all_off, 1000},
	{"todos alternados (ligados)", step_all_toggle, 1000},
	{"todos piscando no padrao", step_all_normal, 4000},
};

#ifdef CONFIG_GPIO_EMUL
/* So no gpio emulado: mostra no log o nivel dos pinos quando muda. */
#define MON_SPEC(i, _)                                                                             \
	IF_ENABLED(DT_HAS_ALIAS(led##i), (GPIO_DT_SPEC_GET(DT_ALIAS(led##i), gpios), ))

static const struct gpio_dt_spec mon_leds[] = {LISTIFY(CONFIG_LED_MAX_ALIASES, MON_SPEC, ())};

#define MON_POLL_MS 5

static void monitor_for(int32_t duration_ms)
{
	static uint32_t last = UINT32_MAX;
	int64_t end = k_uptime_get() + duration_ms;

	while (k_uptime_get() < end) {
		uint32_t levels = 0;
		char line[ARRAY_SIZE(mon_leds) * 2 + 1];

		for (size_t i = 0; i < ARRAY_SIZE(mon_leds); i++) {
			int on = gpio_emul_output_get(mon_leds[i].port, mon_leds[i].pin) > 0;

			levels |= (uint32_t)on << i;
			line[2 * i] = on ? '#' : '.';
			line[2 * i + 1] = ' ';
		}
		line[sizeof(line) - 1] = '\0';

		if (levels != last) {
			LOG_INF("%6lld ms  %s", k_uptime_get(), line);
			last = levels;
		}
		k_msleep(MON_POLL_MS);
	}
}
#else
static void monitor_for(int32_t duration_ms)
{
	k_msleep(duration_ms);
}
#endif

int main(void)
{
	int ret = led_init();

	if (ret < 0) {
		LOG_ERR("falha ao iniciar os LEDs (%d)", ret);
		return ret;
	}

	LOG_INF("%zu LEDs, um unico k_timer (%s)", led_count(), CONFIG_BOARD);

	while (true) {
		for (size_t i = 0; i < ARRAY_SIZE(steps); i++) {
			LOG_INF("--- %s", steps[i].desc);
			ret = steps[i].run();
			if (ret < 0) {
				LOG_ERR("passo falhou (%d)", ret);
			}
			monitor_for(steps[i].duration_ms);
		}
	}

	return 0;
}
