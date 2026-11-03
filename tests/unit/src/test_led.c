/*******************************************************************
 * @file test_led.c
 *
 * @brief Testes do modulo de LED observando os pinos do gpio-emul.
 *******************************************************************/

#include "led.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define NUM_LEDS 3

static const struct gpio_dt_spec pins[NUM_LEDS] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

#define SLOW_HALF_MS   (CONFIG_LED_BLINK_SLOW_PERIOD_MS / 2)
#define NORMAL_HALF_MS (CONFIG_LED_BLINK_NORMAL_PERIOD_MS / 2)
#define FAST_HALF_MS   (CONFIG_LED_BLINK_FAST_PERIOD_MS / 2)
#define SAMPLE_MS      5

static int init_before_ready;

static int level(int led)
{
	return gpio_emul_output_get(pins[led].port, pins[led].pin);
}

/* Amostra os pinos por duration_ms e conta as trocas de nivel. */
static void count_toggles(int32_t duration_ms, int toggles[NUM_LEDS], bool seen_on[NUM_LEDS],
			  bool seen_off[NUM_LEDS])
{
	int prev[NUM_LEDS];
	int64_t end = k_uptime_get() + duration_ms;

	for (int i = 0; i < NUM_LEDS; i++) {
		prev[i] = level(i);
		toggles[i] = 0;
		seen_on[i] = prev[i] == 1;
		seen_off[i] = prev[i] == 0;
	}

	while (k_uptime_get() < end) {
		k_msleep(SAMPLE_MS);
		for (int i = 0; i < NUM_LEDS; i++) {
			int now = level(i);

			toggles[i] += (now != prev[i]);
			seen_on[i] |= (now == 1);
			seen_off[i] |= (now == 0);
			prev[i] = now;
		}
	}
}

static void assert_stays(int led, int expected, int32_t duration_ms)
{
	int toggles[NUM_LEDS];
	bool on[NUM_LEDS];
	bool off[NUM_LEDS];

	zassert_equal(level(led), expected, "LED %d deveria estar em %d", led, expected);
	count_toggles(duration_ms, toggles, on, off);
	zassert_equal(toggles[led], 0, "LED %d trocou %d vezes", led, toggles[led]);
	zassert_equal(level(led), expected);
}

static void *suite_setup(void)
{
	/* gpio_deferred ainda nao foi inicializado */
	init_before_ready = led_init();
	zassert_ok(device_init(pins[2].port));

	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(led_init());
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)led_off(LED_ALL);
}

ZTEST(led, test_init_fails_when_controller_not_ready)
{
	zassert_equal(init_before_ready, -ENODEV);
}

ZTEST(led, test_init_succeeds_and_all_off)
{
	zassert_ok(led_on(LED_ALL));
	zassert_ok(led_init());

	zassert_equal(led_count(), NUM_LEDS);
	for (int i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %d nao comecou desligado", i);
	}
}

ZTEST(led, test_reinit_while_blinking_stops_timer)
{
	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	k_msleep(3 * FAST_HALF_MS);
	zassert_ok(led_init());

	for (int i = 0; i < NUM_LEDS; i++) {
		assert_stays(i, 0, 4 * FAST_HALF_MS);
	}

	/* o timer continua utilizavel depois da reinicializacao */
	int toggles[NUM_LEDS];
	bool on[NUM_LEDS];
	bool off[NUM_LEDS];

	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	count_toggles(10 * FAST_HALF_MS, toggles, on, off);
	zassert_true(toggles[0] >= 8, "so %d trocas", toggles[0]);
}

ZTEST(led, test_on_off_toggle_change_pin)
{
	zassert_ok(led_on(0));
	zassert_equal(level(0), 1);
	zassert_equal(level(1), 0);

	zassert_ok(led_off(0));
	zassert_equal(level(0), 0);

	zassert_ok(led_toggle(1));
	zassert_equal(level(1), 1);
	zassert_ok(led_toggle(1));
	zassert_equal(level(1), 0);
}

ZTEST(led, test_blink_turns_pin_on_and_off)
{
	int toggles[NUM_LEDS];
	bool on[NUM_LEDS];
	bool off[NUM_LEDS];

	zassert_ok(led_blink(0, LED_PATTERN_NORMAL));
	count_toggles(4 * NORMAL_HALF_MS, toggles, on, off);

	zassert_true(on[0], "pino nunca ligou");
	zassert_true(off[0], "pino nunca desligou");
	zassert_true(toggles[0] >= 3);
	/* os outros LEDs nao mexem */
	zassert_equal(toggles[1], 0);
	zassert_equal(toggles[2], 0);
}

ZTEST(led, test_faster_led_toggles_more)
{
	int toggles[NUM_LEDS];
	bool on[NUM_LEDS];
	bool off[NUM_LEDS];

	zassert_ok(led_blink(0, LED_PATTERN_SLOW));
	zassert_ok(led_blink(1, LED_PATTERN_FAST));
	count_toggles(8 * SLOW_HALF_MS, toggles, on, off);

	zassert_true(toggles[1] > toggles[0], "rapido %d, lento %d", toggles[1], toggles[0]);
	/* razao dos periodos: 4x mais trocas, com folga de uma troca */
	zassert_within(toggles[0], 8, 1);
	zassert_within(toggles[1], 32, 1);
}

ZTEST(led, test_other_led_keeps_phase)
{
	int64_t start;
	int64_t last_change;
	int prev;

	zassert_ok(led_blink(0, LED_PATTERN_NORMAL));
	start = k_uptime_get();
	prev = level(0);
	last_change = start;

	/* mexe no LED1 varias vezes enquanto o LED0 pisca */
	for (int step = 0; step < 6; step++) {
		int64_t end = k_uptime_get() + NORMAL_HALF_MS;

		zassert_ok((step % 2) ? led_off(1) : led_blink(1, LED_PATTERN_FAST));
		while (k_uptime_get() < end) {
			k_msleep(SAMPLE_MS);
			if (level(0) != prev) {
				prev = level(0);
				last_change = k_uptime_get();
				/* cada troca do LED0 cai num multiplo do meio periodo */
				int64_t phase = (last_change - start) % NORMAL_HALF_MS;

				zassert_true(phase <= 2 * SAMPLE_MS ||
						     phase >= NORMAL_HALF_MS - 2 * SAMPLE_MS,
					     "LED0 perdeu a fase: %lld ms", phase);
			}
		}
	}

	zassert_true(last_change > start, "LED0 nao piscou");
}

ZTEST(led, test_on_off_toggle_cancel_blink)
{
	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	zassert_ok(led_on(0));
	assert_stays(0, 1, 4 * FAST_HALF_MS);

	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	zassert_ok(led_off(0));
	assert_stays(0, 0, 4 * FAST_HALF_MS);

	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	k_msleep(FAST_HALF_MS / 2);
	zassert_equal(level(0), 1);
	zassert_ok(led_toggle(0));
	assert_stays(0, 0, 4 * FAST_HALF_MS);
}

ZTEST(led, test_action_on_all_leds)
{
	int toggles[NUM_LEDS];
	bool on[NUM_LEDS];
	bool off[NUM_LEDS];

	zassert_ok(led_on(LED_ALL));
	for (int i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 1);
	}

	zassert_ok(led_toggle(LED_ALL));
	for (int i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 0);
	}

	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	count_toggles(6 * FAST_HALF_MS, toggles, on, off);
	for (int i = 0; i < NUM_LEDS; i++) {
		zassert_true(on[i] && off[i], "LED %d nao piscou", i);
	}

	zassert_ok(led_off(LED_ALL));
	for (int i = 0; i < NUM_LEDS; i++) {
		assert_stays(i, 0, 2 * FAST_HALF_MS);
	}
}

ZTEST(led, test_invalid_arguments)
{
	zassert_equal(led_on(NUM_LEDS), -EINVAL);
	zassert_equal(led_off(-2), -EINVAL);
	zassert_equal(led_toggle(100), -EINVAL);
	zassert_equal(led_blink(NUM_LEDS, LED_PATTERN_FAST), -EINVAL);
	zassert_equal(led_blink(0, LED_PATTERN_COUNT), -EINVAL);
	zassert_equal(led_blink(0, (enum led_pattern) - 1), -EINVAL);
	zassert_equal(led_blink(LED_ALL, LED_PATTERN_COUNT), -EINVAL);

	/* nada mudou */
	for (int i = 0; i < NUM_LEDS; i++) {
		zassert_equal(level(i), 0);
	}
}

ZTEST_SUITE(led, NULL, suite_setup, before_each, after_each, NULL);
