/*******************************************************************
 * @file main.c
 *
 * @brief Ponto de entrada da Atividade-05: dois LEDs, um timer.
 *******************************************************************/

#include <errno.h>

#include <zephyr/kernel.h>

#include "led.h"

/** @brief Passo de amostragem da demonstracao (ms). */
#define DEMO_STEP_MS 50

/** @brief Quantidade de amostras da demonstracao (1,5 s). */
#define DEMO_STEPS 30

/** @brief Le o nivel fisico dos dois LEDs. */
static void read_levels(int *led0, int *led1)
{
	bool led0_on = false;
	bool led1_on = false;

	(void)led_level_get(0, &led0_on);
	(void)led_level_get(1, &led1_on);

	*led0 = led0_on ? 1 : 0;
	*led1 = led1_on ? 1 : 0;
}

int main(void)
{
	int toggles[2] = {0, 0};
	int previous[2];
	int level[2];
	int ret;
	int step;

	ret = led_init();
	if (ret != 0) {
		printk("led_init failed (%d)\n", ret);
		return ret;
	}

	if (led_count() < 2U) {
		printk("devicetree must describe at least 2 LEDs\n");
		return -EINVAL;
	}

	ret = led_blink(0, LED_PATTERN_SLOW);
	if (ret != 0) {
		printk("failed to blink led0 (%d)\n", ret);
		return ret;
	}

	ret = led_blink(1, LED_PATTERN_FAST);
	if (ret != 0) {
		printk("failed to blink led1 (%d)\n", ret);
		return ret;
	}

	read_levels(&previous[0], &previous[1]);

	/* Demonstracao: o timer cuida do pisca; aqui so observamos os pinos. */
	for (step = 1; step <= DEMO_STEPS; step++) {
		k_msleep(DEMO_STEP_MS);
		read_levels(&level[0], &level[1]);

		toggles[0] += (level[0] != previous[0]);
		toggles[1] += (level[1] != previous[1]);
		previous[0] = level[0];
		previous[1] = level[1];

		printk("t=%4d ms   led0=%d   led1=%d\n", step * DEMO_STEP_MS, level[0], level[1]);
	}

	printk("\ntoggles in %d ms:  led0=%d  led1=%d\n", DEMO_STEPS * DEMO_STEP_MS, toggles[0],
	       toggles[1]);

	printk("turning all LEDs off\n");
	(void)led_off(LED_ALL);

	return 0;
}
