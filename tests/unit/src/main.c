#include "led.h"

#include <errno.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_TEST_GPIO_NOT_READY)
ZTEST(leds_not_ready, test_init_fails)
{
	zassert_equal(leds_init(), -ENODEV);
	zassert_equal(leds_init(), -ENODEV);
	zassert_equal(led_on(0), -EACCES);
}
ZTEST_SUITE(leds_not_ready, NULL, NULL, NULL, NULL, NULL);
#else
static const struct gpio_dt_spec pins[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
};

static int level(size_t index)
{
	int value = gpio_emul_output_get(pins[index].port, pins[index].pin);

	zassert_true(value >= 0, "Falha ao ler GPIO emulado");
	return value;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(leds_init());
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(led_off(LED_ALL));
}

static int64_t wait_level(size_t index, int expected, int64_t deadline)
{
	while (k_uptime_get() < deadline) {
		if (level(index) == expected) {
			return k_uptime_get();
		}
		/* Amostra a cada 1 ms mesmo quando o tick do kernel vale 10 ms. */
		k_busy_wait(1000);
	}
	zassert_equal(level(index), expected, "LED nao mudou no prazo");
	return k_uptime_get();
}

ZTEST(leds, test_initially_off)
{
	zassert_equal(leds_count(), 2);
	zassert_equal(level(0), 0);
	zassert_equal(level(1), 0);
}

ZTEST(leds, test_on_off_toggle)
{
	zassert_ok(led_on(0));
	zassert_equal(level(0), 1);
	zassert_equal(level(1), 0);
	zassert_ok(led_toggle(0));
	zassert_equal(level(0), 0);
	zassert_ok(led_toggle(1));
	zassert_equal(level(1), 1);
	zassert_ok(led_off(1));
	zassert_equal(level(1), 0);
}

ZTEST(leds, test_blink_each_pattern)
{
	static const int periods[] = {CONFIG_LED_SLOW_MS, CONFIG_LED_NORMAL_MS, CONFIG_LED_FAST_MS};

	for (int pattern = LED_SLOW; pattern < LED_PATTERN_COUNT; pattern++) {
		int64_t start;

		zassert_ok(led_off(0));
		start = k_uptime_get();
		zassert_ok(led_blink(0, (enum led_pattern)pattern));
		zassert_equal(level(0), 0);
		wait_level(0, 1, start + periods[pattern] + 10);
		wait_level(0, 0, start + 2 * periods[pattern] + 10);
	}
}

ZTEST(leds, test_independent_rates)
{
	int changes[2] = {0};
	int previous[2] = {0};
	int64_t end;

	zassert_ok(led_blink(0, LED_SLOW));
	zassert_ok(led_blink(1, LED_FAST));
	end = k_uptime_get() + 3 * CONFIG_LED_SLOW_MS + 10;
	while (k_uptime_get() < end) {
		for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
			int current = level(i);

			changes[i] += current != previous[i];
			previous[i] = current;
		}
		k_msleep(1);
	}
	zassert_true(changes[0] >= 3);
	zassert_true(changes[1] > changes[0]);
	printk("Alternancias no mesmo intervalo: lento=%d rapido=%d\n", changes[0], changes[1]);
}

ZTEST(leds, test_other_led_keeps_phase)
{
	int64_t start = k_uptime_get();
	int64_t first;
	int64_t second;

	zassert_ok(led_blink(0, LED_SLOW));
	k_msleep(CONFIG_LED_SLOW_MS / 2);
	zassert_ok(led_blink(1, LED_FAST));
	first = wait_level(0, 1, start + CONFIG_LED_SLOW_MS + 10);
	zassert_true(first - start >= CONFIG_LED_SLOW_MS);
	zassert_ok(led_blink(1, LED_NORMAL));
	k_msleep(CONFIG_LED_SLOW_MS / 2);
	zassert_ok(led_off(1));
	second = wait_level(0, 0, start + 2 * CONFIG_LED_SLOW_MS + 10);
	zassert_true(second - first >= CONFIG_LED_SLOW_MS - 2);
}

ZTEST(leds, test_change_pattern)
{
	int64_t start;

	zassert_ok(led_blink(0, LED_SLOW));
	k_msleep(CONFIG_LED_FAST_MS);
	start = k_uptime_get();
	zassert_ok(led_blink(0, LED_FAST));
	wait_level(0, 1, start + CONFIG_LED_FAST_MS + 10);
	wait_level(0, 0, start + 2 * CONFIG_LED_FAST_MS + 10);
}

ZTEST(leds, test_actions_cancel_blink)
{
	static int (*const actions[])(int id) = {led_on, led_off, led_toggle};
	static const int expected[] = {1, 0, 0};

	for (size_t i = 0; i < ARRAY_SIZE(actions); i++) {
		zassert_ok(led_off(0));
		zassert_ok(led_blink(0, LED_FAST));
		wait_level(0, 1, k_uptime_get() + CONFIG_LED_FAST_MS + 10);
		zassert_ok(actions[i](0));
		zassert_equal(level(0), expected[i]);
		int64_t end = k_uptime_get() + 3 * CONFIG_LED_FAST_MS;

		while (k_uptime_get() < end) {
			zassert_equal(level(0), expected[i]);
			k_msleep(1);
		}
	}
}

ZTEST(leds, test_all_actions)
{
	zassert_ok(led_on(LED_ALL));
	zassert_equal(level(0), 1);
	zassert_equal(level(1), 1);
	zassert_ok(led_toggle(LED_ALL));
	zassert_equal(level(0), 0);
	zassert_equal(level(1), 0);
	zassert_ok(led_blink(LED_ALL, LED_FAST));
	wait_level(0, 1, k_uptime_get() + CONFIG_LED_FAST_MS + 10);
	zassert_equal(level(1), 1);
	zassert_ok(led_off(LED_ALL));
	k_msleep(3 * CONFIG_LED_FAST_MS);
	zassert_equal(level(0), 0);
	zassert_equal(level(1), 0);
}

ZTEST(leds, test_reinitialize)
{
	zassert_ok(led_blink(LED_ALL, LED_FAST));
	wait_level(0, 1, k_uptime_get() + CONFIG_LED_FAST_MS + 10);
	zassert_ok(leds_init());
	zassert_ok(leds_init());
	k_msleep(3 * CONFIG_LED_FAST_MS);
	zassert_equal(level(0), 0);
	zassert_equal(level(1), 0);
	zassert_ok(led_blink(1, LED_FAST));
	wait_level(1, 1, k_uptime_get() + CONFIG_LED_FAST_MS + 10);
	zassert_equal(level(0), 0);
}

ZTEST(leds, test_invalid_arguments)
{
	static int (*const actions[])(int id) = {led_on, led_off, led_toggle};

	for (size_t i = 0; i < ARRAY_SIZE(actions); i++) {
		zassert_equal(actions[i](-2), -EINVAL);
		zassert_equal(actions[i]((int)leds_count()), -EINVAL);
	}
	zassert_equal(led_blink(-2, LED_FAST), -EINVAL);
	zassert_equal(led_blink((int)leds_count(), LED_FAST), -EINVAL);
	zassert_equal(led_blink(0, LED_PATTERN_COUNT), -EINVAL);
	zassert_equal(led_blink(LED_ALL, (enum led_pattern) - 1), -EINVAL);
	zassert_equal(level(0), 0);
	zassert_equal(level(1), 0);
}

ZTEST(leds, test_frequent_actions_keep_other_led_blinking)
{
	int previous = 0;
	int changes = 0;
	int64_t end;

	zassert_ok(led_blink(0, LED_FAST));
	end = k_uptime_get() + 4 * CONFIG_LED_FAST_MS;
	while (k_uptime_get() < end) {
		int current;

		zassert_ok(led_on(1));
		k_busy_wait(1000);
		current = level(0);
		changes += current != previous;
		previous = current;
	}
	zassert_true(changes >= 3, "Outro LED adiou as piscadas: %d trocas", changes);
}

ZTEST_SUITE(leds, NULL, NULL, before, after, NULL);
#endif
