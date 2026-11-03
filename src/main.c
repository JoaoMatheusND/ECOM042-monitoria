/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include "led.h"

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/* So a demonstracao amostra os LEDs. O modulo nao depende disto. */
#define SAMPLE_MS 1

struct demo_step {
	uint32_t at_ms;
	const char *label;
	int (*run)(void);
};

static int each_led_own_pattern(void)
{
	int ret = 0;

	for (size_t i = 0; i < led_count(); i++) {
		int err = led_blink((int)i, (enum led_pattern)(i % LED_PATTERN_COUNT));

		if (err < 0 && ret == 0) {
			ret = err;
		}
	}
	return ret;
}

static int hold_led0_on(void)
{
	return led_on(0);
}

static int all_fast(void)
{
	return led_blink(LED_ALL, LED_PATTERN_FAST);
}

static int all_off(void)
{
	return led_off(LED_ALL);
}

static const struct demo_step steps[] = {
	{0, "LED0 lento, LED1 padrao, LED2 rapido", each_led_own_pattern},
	{4000, "LED0 fixo ligado; os outros nao mudam de ritmo", hold_led0_on},
	{7000, "todos piscando rapido", all_fast},
	{9000, "todos desligados", all_off},
};

static uint32_t read_levels(void)
{
	uint32_t mask = 0;

	for (size_t i = 0; i < led_count(); i++) {
		if (led_is_on((int)i) == 1) {
			mask |= BIT(i);
		}
	}
	return mask;
}

static void print_levels(uint32_t at_ms, uint32_t mask)
{
	printk("[%6u ms]", at_ms);
	for (size_t i = 0; i < led_count(); i++) {
		printk("  LED%u %s", (unsigned int)i, (mask & BIT(i)) ? "ON " : "off");
	}
	printk("\n");
}

int main(void)
{
	uint32_t shown = UINT32_MAX;
	size_t next = 0;
	int64_t start;
	int ret;

	ret = led_init();
	if (ret < 0) {
		LOG_ERR("led_init falhou: %d", ret);
		return 0;
	}
	printk("%u LEDs encontrados no devicetree\n", (unsigned int)led_count());

	start = k_uptime_get();
	while (next < ARRAY_SIZE(steps) || shown != read_levels()) {
		uint32_t elapsed = (uint32_t)(k_uptime_get() - start);
		uint32_t levels;

		while (next < ARRAY_SIZE(steps) && elapsed >= steps[next].at_ms) {
			printk("--- %s\n", steps[next].label);
			ret = steps[next].run();
			if (ret < 0) {
				LOG_ERR("passo \"%s\" falhou: %d", steps[next].label, ret);
			}
			next++;
		}

		levels = read_levels();
		if (levels != shown) {
			print_levels(elapsed, levels);
			shown = levels;
		}
		k_msleep(SAMPLE_MS);
	}

	printk("--- fim da demonstracao\n");
	return 0;
}
