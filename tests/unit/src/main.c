/* SPDX-License-Identifier: Apache-2.0 */

#include "led.h"

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TEST_LED_COUNT 3

static const struct gpio_dt_spec pins[TEST_LED_COUNT] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static int pin(size_t i)
{
	return gpio_emul_output_get_dt(&pins[i]);
}

#if DT_NODE_EXISTS(DT_NODELABEL(test_gpio_deferred))

ZTEST_SUITE(led_deferred_init, NULL, NULL, NULL, NULL, NULL);

ZTEST(led_deferred_init, test_init_fails_until_controller_is_ready)
{
	const struct device *late = DEVICE_DT_GET(DT_NODELABEL(test_gpio_deferred));

	zassert_false(device_is_ready(late));
	zassert_equal(led_init(), -ENODEV);
	zassert_equal(led_on(0), -ENODEV, "API sem init bem-sucedido deve recusar");

	zassert_ok(device_init(late));
	zassert_ok(led_init());
	zassert_ok(led_on(1));
	zassert_equal(pin(1), 1);
}

#else

#define HALF_MS(period_ms) ((period_ms) / 2)

struct observation {
	int toggles[TEST_LED_COUNT];
	bool saw_on[TEST_LED_COUNT];
	bool saw_off[TEST_LED_COUNT];
};

/* Dorme ate start + t ms. k_msleep(1) dormiria 2 ticks (o relativo
 * arredonda para cima) e a janela de observacao dobraria. */
static void sleep_until(int64_t start, int t)
{
	k_sleep(K_TIMEOUT_ABS_MS(start + t));
}

/* Amostra os pinos a cada 1 ms (um tick) durante duration_ms. */
static void observe(int duration_ms, struct observation *obs)
{
	const int64_t start = k_uptime_get();
	int last[TEST_LED_COUNT];

	*obs = (struct observation){0};
	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		last[i] = pin(i);
	}

	for (int t = 1; t <= duration_ms; t++) {
		sleep_until(start, t);
		for (size_t i = 0; i < TEST_LED_COUNT; i++) {
			int v = pin(i);

			obs->toggles[i] += (v != last[i]);
			obs->saw_on[i] |= (v == 1);
			obs->saw_off[i] |= (v == 0);
			last[i] = v;
		}
	}
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(led_init());
}

ZTEST_SUITE(led_basic, NULL, NULL, before_each, NULL, NULL);
ZTEST_SUITE(led_blink, NULL, NULL, before_each, NULL, NULL);

ZTEST(led_basic, test_init_turns_every_led_off)
{
	zassert_equal(led_count(), TEST_LED_COUNT);

	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	zassert_ok(led_init(), "init repetido nao pode falhar");

	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_equal(pin(i), 0, "LED %u deveria iniciar desligado", i);
		zassert_equal(led_is_on((int)i), 0);
	}

	/* O pisca anterior foi cancelado: nada pode mudar depois. */
	struct observation obs;

	observe(CONFIG_APP_LED_PERIOD_FAST_MS, &obs);
	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_equal(obs.toggles[i], 0);
	}
}

ZTEST(led_basic, test_on_off_toggle_drive_only_that_pin)
{
	zassert_ok(led_on(1));
	zassert_equal(pin(1), 1);
	zassert_equal(pin(0), 0);
	zassert_equal(pin(2), 0);

	zassert_ok(led_off(1));
	zassert_equal(pin(1), 0);

	zassert_ok(led_toggle(1));
	zassert_equal(pin(1), 1);
	zassert_ok(led_toggle(1));
	zassert_equal(pin(1), 0);
}

ZTEST(led_basic, test_all_reaches_every_led)
{
	zassert_ok(led_on(LED_ALL));
	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_equal(pin(i), 1);
	}

	zassert_ok(led_off(LED_ALL));
	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_equal(pin(i), 0);
	}

	zassert_ok(led_toggle(LED_ALL));
	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_equal(pin(i), 1);
	}

	struct observation obs;

	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	observe(2 * CONFIG_APP_LED_PERIOD_FAST_MS, &obs);
	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_true(obs.saw_on[i] && obs.saw_off[i], "LED %u nao piscou", i);
	}
}

ZTEST(led_basic, test_invalid_arguments_are_rejected)
{
	zassert_equal(led_on(TEST_LED_COUNT), -EINVAL);
	zassert_equal(led_off(-2), -EINVAL);
	zassert_equal(led_toggle(100), -EINVAL);
	zassert_equal(led_blink(TEST_LED_COUNT, LED_PATTERN_FAST), -EINVAL);
	zassert_equal(led_blink(0, LED_PATTERN_COUNT), -EINVAL);
	zassert_equal(led_blink(0, (enum led_pattern) - 1), -EINVAL);
	zassert_equal(led_is_on(LED_ALL), -EINVAL);

	for (size_t i = 0; i < TEST_LED_COUNT; i++) {
		zassert_equal(pin(i), 0, "chamada invalida nao pode mexer no pino");
	}
}

ZTEST(led_blink, test_blink_is_seen_on_and_off)
{
	struct observation obs;

	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	zassert_equal(pin(0), 1, "pisca comeca ligado");

	observe(2 * CONFIG_APP_LED_PERIOD_FAST_MS, &obs);
	zassert_true(obs.saw_on[0]);
	zassert_true(obs.saw_off[0]);
	zassert_within(obs.toggles[0], 4, 1);
}

ZTEST(led_blink, test_faster_led_toggles_more)
{
	const int window = 2 * CONFIG_APP_LED_PERIOD_SLOW_MS;
	struct observation obs;

	zassert_ok(led_blink(0, LED_PATTERN_SLOW));
	zassert_ok(led_blink(1, LED_PATTERN_NORMAL));
	zassert_ok(led_blink(2, LED_PATTERN_FAST));
	observe(window, &obs);

	zassert_true(obs.toggles[2] > obs.toggles[1]);
	zassert_true(obs.toggles[1] > obs.toggles[0]);
	zassert_within(obs.toggles[0], window / HALF_MS(CONFIG_APP_LED_PERIOD_SLOW_MS), 1);
	zassert_within(obs.toggles[1], window / HALF_MS(CONFIG_APP_LED_PERIOD_NORMAL_MS), 1);
	zassert_within(obs.toggles[2], window / HALF_MS(CONFIG_APP_LED_PERIOD_FAST_MS), 1);
}

ZTEST(led_blink, test_on_off_toggle_cancel_blink)
{
	static int (*const cancel[])(int) = {led_on, led_off, led_toggle};
	struct observation obs;

	for (size_t c = 0; c < ARRAY_SIZE(cancel); c++) {
		zassert_ok(led_blink(0, LED_PATTERN_FAST));
		k_msleep(CONFIG_APP_LED_PERIOD_FAST_MS / 4);

		zassert_ok(cancel[c](0));
		int frozen = pin(0);

		observe(3 * CONFIG_APP_LED_PERIOD_FAST_MS, &obs);
		zassert_equal(obs.toggles[0], 0, "comando %u nao cancelou o pisca", c);
		zassert_equal(pin(0), frozen);
	}
}

/* Mexer no LED1 varias vezes nao pode deslocar as trocas do LED0. */
ZTEST(led_blink, test_other_led_does_not_shift_phase)
{
	const int64_t half = HALF_MS(CONFIG_APP_LED_PERIOD_NORMAL_MS);
	int last = 1;
	int toggles = 0;
	int64_t t0;

	zassert_ok(led_blink(0, LED_PATTERN_NORMAL));
	t0 = k_uptime_get();

	for (int t = 1; t <= 5 * half; t++) {
		sleep_until(t0, t);

		if (t == 30) {
			zassert_ok(led_blink(1, LED_PATTERN_FAST));
		} else if (t == 170) {
			zassert_ok(led_off(1));
		} else if (t == 260) {
			zassert_ok(led_blink(1, LED_PATTERN_SLOW));
		} else if (t == 333) {
			zassert_ok(led_toggle(1));
		}

		int v = pin(0);

		if (v != last) {
			int64_t offset = (k_uptime_get() - t0) % half;

			zassert_true(offset <= 1 || offset >= half - 1,
				     "LED0 trocou fora da fase (offset %lld ms)", offset);
			toggles++;
			last = v;
		}
	}
	zassert_within(toggles, 5, 1);
}

#endif
