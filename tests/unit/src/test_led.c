/*******************************************************************
 * @file test_led.c
 *
 * @brief Testes do modulo de LED observando os pinos do gpio-emul.
 *******************************************************************/

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#include "led.h"

#define NUM_TEST_LEDS  3
#define SLOW_HALF_MS   (CONFIG_APP_LED_SLOW_PERIOD_MS / 2)
#define NORMAL_HALF_MS (CONFIG_APP_LED_NORMAL_PERIOD_MS / 2)
#define FAST_HALF_MS   (CONFIG_APP_LED_FAST_PERIOD_MS / 2)

static const struct device *const gpio_dev = DEVICE_DT_GET(DT_NODELABEL(test_gpio));

static const struct gpio_dt_spec pins[NUM_TEST_LEDS] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

/* Resultado de led_init() antes de o controlador ficar pronto. */
static int init_before_ready;

static int pin(int i)
{
	return gpio_emul_output_get(pins[i].port, pins[i].pin);
}

/* Amostra os pinos a cada 1 ms por @p ms e conta as trocas de nivel. */
static void count_transitions(int32_t ms, int counts[NUM_TEST_LEDS], bool saw_high[NUM_TEST_LEDS],
			      bool saw_low[NUM_TEST_LEDS])
{
	int last[NUM_TEST_LEDS];
	int64_t end = k_uptime_get() + ms;

	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		last[i] = pin(i);
		counts[i] = 0;
		saw_high[i] = (last[i] == 1);
		saw_low[i] = (last[i] == 0);
	}

	/* Limite pelo relogio, nao por iteracoes: k_msleep(1) pode durar mais. */
	while (k_uptime_get() < end) {
		k_msleep(1);
		for (int i = 0; i < NUM_TEST_LEDS; i++) {
			int now = pin(i);

			counts[i] += (now != last[i]);
			saw_high[i] |= (now == 1);
			saw_low[i] |= (now == 0);
			last[i] = now;
		}
	}
}

static void *suite_setup(void)
{
	init_before_ready = led_init();
	device_init(gpio_dev);
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(led_init());
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	led_init();
}

ZTEST(led, test_init_fails_when_controller_not_ready)
{
	zassert_equal(init_before_ready, -ENODEV);
}

ZTEST(led, test_init_succeeds_and_all_off)
{
	zassert_equal(led_count(), NUM_TEST_LEDS);
	zassert_ok(led_on(LED_ALL));
	zassert_ok(led_init());

	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_equal(pin(i), 0, "led%d deveria comecar desligado", i);
		zassert_false(led_is_on(i));
	}
}

ZTEST(led, test_init_again_while_blinking_keeps_timer_sane)
{
	int counts[NUM_TEST_LEDS];
	bool hi[NUM_TEST_LEDS], lo[NUM_TEST_LEDS];

	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	k_msleep(3 * FAST_HALF_MS);
	zassert_ok(led_init());

	count_transitions(4 * FAST_HALF_MS, counts, hi, lo);
	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_equal(counts[i], 0, "led%d continuou piscando apos led_init", i);
		zassert_equal(pin(i), 0);
	}

	/* E o timer continua utilizavel. */
	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	count_transitions(4 * FAST_HALF_MS, counts, hi, lo);
	zassert_true(counts[0] >= 3);
}

ZTEST(led, test_on_off_toggle_change_pin)
{
	zassert_ok(led_on(0));
	zassert_equal(pin(0), 1);
	zassert_true(led_is_on(0));

	zassert_ok(led_off(0));
	zassert_equal(pin(0), 0);

	zassert_ok(led_toggle(0));
	zassert_equal(pin(0), 1);
	zassert_ok(led_toggle(0));
	zassert_equal(pin(0), 0);

	zassert_equal(pin(1), 0, "outros LEDs nao mudam");
	zassert_equal(pin(2), 0, "outros LEDs nao mudam");
}

ZTEST(led, test_blink_observes_on_and_off)
{
	int counts[NUM_TEST_LEDS];
	bool hi[NUM_TEST_LEDS], lo[NUM_TEST_LEDS];

	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	count_transitions(3 * FAST_HALF_MS, counts, hi, lo);

	zassert_true(hi[0], "led0 nunca ficou ligado");
	zassert_true(lo[0], "led0 nunca ficou desligado");
	zassert_true(counts[0] >= 2);
}

ZTEST(led, test_faster_led_toggles_more)
{
	int counts[NUM_TEST_LEDS];
	bool hi[NUM_TEST_LEDS], lo[NUM_TEST_LEDS];

	zassert_ok(led_blink(0, LED_PATTERN_SLOW));
	zassert_ok(led_blink(1, LED_PATTERN_FAST));
	count_transitions(2 * CONFIG_APP_LED_SLOW_PERIOD_MS, counts, hi, lo);

	zassert_true(counts[1] > counts[0], "rapido=%d lento=%d", counts[1], counts[0]);
	zassert_within(counts[0], 4, 1, "lento trocou %d vezes", counts[0]);
	zassert_within(counts[1], 2 * CONFIG_APP_LED_SLOW_PERIOD_MS / FAST_HALF_MS, 2,
		       "rapido trocou %d vezes", counts[1]);
}

ZTEST(led, test_other_led_does_not_change_phase)
{
	int64_t start;
	int64_t edges[4];
	int n = 0;
	int last;

	zassert_ok(led_blink(0, LED_PATTERN_NORMAL));
	start = k_uptime_get();
	last = pin(0);

	/* Mexe no led1 no meio do primeiro meio-periodo do led0. */
	k_msleep(NORMAL_HALF_MS / 3);
	zassert_ok(led_blink(1, LED_PATTERN_FAST));
	k_msleep(NORMAL_HALF_MS / 3);
	zassert_ok(led_off(1));

	while (n < ARRAY_SIZE(edges)) {
		k_msleep(1);
		if (pin(0) != last) {
			last = pin(0);
			edges[n++] = k_uptime_get() - start;
		}
		zassert_true(k_uptime_get() - start < 10 * NORMAL_HALF_MS, "led0 parou");
	}

	for (int i = 0; i < n; i++) {
		zassert_within(edges[i], (i + 1) * NORMAL_HALF_MS, 2, "troca %d em %lld ms", i,
			       edges[i]);
	}
}

ZTEST(led, test_on_off_toggle_cancel_blink)
{
	/* expected = -1: o nivel final nao importa, so precisa ficar parado */
	static const struct {
		int (*action)(int led);
		int expected;
	} cases[] = {{led_on, 1}, {led_off, 0}, {led_toggle, -1}};
	int counts[NUM_TEST_LEDS];
	bool hi[NUM_TEST_LEDS], lo[NUM_TEST_LEDS];

	for (size_t c = 0; c < ARRAY_SIZE(cases); c++) {
		zassert_ok(led_blink(0, LED_PATTERN_FAST));
		zassert_ok(led_blink(1, LED_PATTERN_FAST));
		k_msleep(FAST_HALF_MS + FAST_HALF_MS / 2);

		zassert_ok(cases[c].action(0));
		count_transitions(4 * FAST_HALF_MS, counts, hi, lo);

		zassert_equal(counts[0], 0, "caso %zu: led0 continuou piscando", c);
		if (cases[c].expected >= 0) {
			zassert_equal(pin(0), cases[c].expected, "caso %zu", c);
		}
		zassert_true(counts[1] >= 3, "caso %zu: led1 deveria seguir piscando", c);
	}
}

ZTEST(led, test_all_leds_action)
{
	int counts[NUM_TEST_LEDS];
	bool hi[NUM_TEST_LEDS], lo[NUM_TEST_LEDS];

	zassert_ok(led_on(LED_ALL));
	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_equal(pin(i), 1, "led%d", i);
	}

	zassert_ok(led_toggle(LED_ALL));
	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_equal(pin(i), 0, "led%d", i);
	}

	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	count_transitions(4 * FAST_HALF_MS, counts, hi, lo);
	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_true(counts[i] >= 3, "led%d trocou %d vezes", i, counts[i]);
	}

	zassert_ok(led_off(LED_ALL));
	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_equal(pin(i), 0, "led%d", i);
	}
}

ZTEST(led, test_invalid_arguments)
{
	zassert_equal(led_on(NUM_TEST_LEDS), -EINVAL);
	zassert_equal(led_off(-2), -EINVAL);
	zassert_equal(led_toggle(100), -EINVAL);
	zassert_equal(led_blink(0, LED_PATTERN_COUNT), -EINVAL);
	zassert_equal(led_blink(0, (enum led_pattern) - 1), -EINVAL);
	zassert_equal(led_execute(0, LED_ACTION_COUNT, LED_PATTERN_SLOW), -EINVAL);

	for (int i = 0; i < NUM_TEST_LEDS; i++) {
		zassert_equal(pin(i), 0, "argumento invalido nao pode mexer no led%d", i);
	}
}

ZTEST_SUITE(led, NULL, suite_setup, before, after, NULL);
