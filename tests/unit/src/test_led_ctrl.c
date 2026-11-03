/*******************************************************************
 * @file test_led_ctrl.c
 *
 * @brief Testes do led_ctrl observando os pinos com gpio_emul.
 *******************************************************************/

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "led_ctrl.h"

#define N_LEDS    3
#define SAMPLE_MS 5

static const struct gpio_dt_spec pins[N_LEDS] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static const struct device *const late_gpio = DEVICE_DT_GET(DT_NODELABEL(late_gpio));

static int init_before_ready;
static int device_init_ret;

static int level(int led)
{
	return gpio_emul_output_get(pins[led].port, pins[led].pin);
}

/* Amostra os pinos por ms milissegundos e conta as trocas de nível. */
static void count_toggles(int ms, int toggles[N_LEDS])
{
	int prev[N_LEDS];

	for (int i = 0; i < N_LEDS; i++) {
		prev[i] = level(i);
		toggles[i] = 0;
	}

	/* Janela medida pelo relógio, não pelo número de amostras. */
	for (int64_t end = k_uptime_get() + ms; k_uptime_get() < end;) {
		k_msleep(SAMPLE_MS);
		for (int i = 0; i < N_LEDS; i++) {
			int now = level(i);

			toggles[i] += (now != prev[i]);
			prev[i] = now;
		}
	}
}

static void *suite_setup(void)
{
	/* late_gpio ainda não foi inicializado (zephyr,deferred-init). */
	init_before_ready = led_ctrl_init();
	device_init_ret = device_init(late_gpio);
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(led_ctrl_init());
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)led_ctrl_init(); /* para qualquer pisca entre um teste e outro */
}

ZTEST_SUITE(led_ctrl, NULL, suite_setup, before, after, NULL);

ZTEST(led_ctrl, test_init_fails_with_controller_not_ready)
{
	zassert_equal(init_before_ready, -ENODEV, "init deveria falhar: %d", init_before_ready);
	zassert_ok(device_init_ret);
}

ZTEST(led_ctrl, test_init_ok_and_all_leds_off)
{
	zassert_equal(led_ctrl_count(), N_LEDS);

	zassert_ok(led_ctrl_on(LED_CTRL_ALL));
	zassert_ok(led_ctrl_init());

	for (int i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %d deveria comecar desligado", i);
		zassert_equal(led_ctrl_get(i), 0);
	}
}

ZTEST(led_ctrl, test_reinit_while_blinking_stops_timer)
{
	int toggles[N_LEDS];

	zassert_ok(led_ctrl_blink(LED_CTRL_ALL, LED_PATTERN_FAST));
	k_msleep(30);
	zassert_ok(led_ctrl_init());

	count_toggles(3 * CONFIG_LED_CTRL_PERIOD_FAST_MS, toggles);
	for (int i = 0; i < N_LEDS; i++) {
		zassert_equal(toggles[i], 0, "LED %d continuou piscando apos init", i);
		zassert_equal(level(i), 0);
	}

	/* e o timer continua funcionando depois do init */
	zassert_ok(led_ctrl_blink(0, LED_PATTERN_FAST));
	count_toggles(2 * CONFIG_LED_CTRL_PERIOD_FAST_MS, toggles);
	zassert_true(toggles[0] > 0);
}

ZTEST(led_ctrl, test_on_off_toggle_change_pin)
{
	zassert_ok(led_ctrl_on(0));
	zassert_equal(level(0), 1);

	zassert_ok(led_ctrl_off(0));
	zassert_equal(level(0), 0);

	zassert_ok(led_ctrl_toggle(0));
	zassert_equal(level(0), 1);

	zassert_ok(led_ctrl_toggle(0));
	zassert_equal(level(0), 0);

	/* os outros LEDs não foram tocados */
	zassert_equal(level(1), 0);
	zassert_equal(level(2), 0);
}

ZTEST(led_ctrl, test_blink_is_seen_on_and_off)
{
	bool saw_on = false;
	bool saw_off = false;

	zassert_ok(led_ctrl_blink(1, LED_PATTERN_NORMAL));

	for (int64_t end = k_uptime_get() + 2 * CONFIG_LED_CTRL_PERIOD_NORMAL_MS;
	     k_uptime_get() < end;) {
		saw_on |= (level(1) == 1);
		saw_off |= (level(1) == 0);
		k_msleep(SAMPLE_MS);
	}

	zassert_true(saw_on, "LED nunca foi visto ligado");
	zassert_true(saw_off, "LED nunca foi visto desligado");
}

ZTEST(led_ctrl, test_faster_led_toggles_more)
{
	int toggles[N_LEDS];
	int window = 2 * CONFIG_LED_CTRL_PERIOD_SLOW_MS;

	zassert_ok(led_ctrl_blink(0, LED_PATTERN_FAST));
	zassert_ok(led_ctrl_blink(1, LED_PATTERN_SLOW));
	count_toggles(window, toggles);

	zassert_true(toggles[0] > toggles[1], "rapido=%d lento=%d", toggles[0], toggles[1]);

	/* cada LED troca a cada meio período do seu padrão (margem de 1) */
	zassert_within(toggles[0], window / (CONFIG_LED_CTRL_PERIOD_FAST_MS / 2), 1);
	zassert_within(toggles[1], window / (CONFIG_LED_CTRL_PERIOD_SLOW_MS / 2), 1);
	zassert_equal(toggles[2], 0, "LED parado nao pode trocar");
}

ZTEST(led_ctrl, test_on_off_toggle_cancel_blink)
{
	int (*const cancel[])(int) = {led_ctrl_on, led_ctrl_off, led_ctrl_toggle};
	int toggles[N_LEDS];

	for (size_t c = 0; c < ARRAY_SIZE(cancel); c++) {
		zassert_ok(led_ctrl_blink(0, LED_PATTERN_FAST));
		k_msleep(CONFIG_LED_CTRL_PERIOD_FAST_MS / 4);
		zassert_ok(cancel[c](0));

		count_toggles(3 * CONFIG_LED_CTRL_PERIOD_FAST_MS, toggles);
		zassert_equal(toggles[0], 0, "acao %u nao cancelou o pisca", (unsigned int)c);
	}
}

ZTEST(led_ctrl, test_other_led_does_not_change_phase)
{
	const int half = CONFIG_LED_CTRL_PERIOD_FAST_MS / 2;

	zassert_ok(led_ctrl_blink(0, LED_PATTERN_FAST));

	/* Amostra o LED0 no meio de cada meio-período: aceso, apagado, ...
	 * Enquanto isso o LED1 é ligado, piscado e desligado. */
	k_msleep(half / 2);
	for (int k = 0; k < 8; k++) {
		zassert_equal(level(0), (k % 2 == 0) ? 1 : 0, "fase do LED0 mudou em k=%d", k);

		if (k == 1) {
			zassert_ok(led_ctrl_blink(1, LED_PATTERN_SLOW));
		} else if (k == 3) {
			zassert_ok(led_ctrl_on(1));
		} else if (k == 5) {
			zassert_ok(led_ctrl_blink(1, LED_PATTERN_NORMAL));
		} else if (k == 6) {
			zassert_ok(led_ctrl_off(1));
		}

		k_msleep(half);
	}
}

ZTEST(led_ctrl, test_all_leds_action)
{
	int toggles[N_LEDS];

	zassert_ok(led_ctrl_on(LED_CTRL_ALL));
	for (int i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 1, "LED %d nao ligou", i);
	}

	zassert_ok(led_ctrl_toggle(LED_CTRL_ALL));
	for (int i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %d nao alternou", i);
	}

	zassert_ok(led_ctrl_blink(LED_CTRL_ALL, LED_PATTERN_FAST));
	count_toggles(2 * CONFIG_LED_CTRL_PERIOD_FAST_MS, toggles);
	for (int i = 0; i < N_LEDS; i++) {
		zassert_true(toggles[i] > 0, "LED %d nao piscou", i);
	}

	zassert_ok(led_ctrl_off(LED_CTRL_ALL));
	for (int i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %d nao desligou", i);
	}
}

ZTEST(led_ctrl, test_invalid_led_and_pattern)
{
	zassert_equal(led_ctrl_on(N_LEDS), -EINVAL);
	zassert_equal(led_ctrl_off(-2), -EINVAL);
	zassert_equal(led_ctrl_toggle(99), -EINVAL);
	zassert_equal(led_ctrl_blink(N_LEDS, LED_PATTERN_FAST), -EINVAL);

	zassert_equal(led_ctrl_blink(0, LED_PATTERN_COUNT), -EINVAL);
	zassert_equal(led_ctrl_blink(0, (enum led_pattern) - 1), -EINVAL);
	zassert_equal(led_ctrl_apply(0, LED_ACTION_COUNT, LED_PATTERN_FAST), -EINVAL);
	zassert_equal(led_ctrl_get(N_LEDS), -EINVAL);

	/* nada mudou nos pinos */
	for (int i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0);
	}
}
