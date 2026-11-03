/*******************************************************************
 * @file test_main.c
 *
 * @brief Testes ztest do driver de LED, observando o nivel dos pinos
 *        pelo gpio-emul (nenhum hardware real envolvido).
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "led.h"

#define LED_A 0U
#define LED_B 1U

/* O controlador do overlay de teste esta com zephyr,deferred-init. */
static const struct device *const gpio_test = DEVICE_DT_GET(DT_NODELABEL(gpio_test));

static const struct gpio_dt_spec spec_a = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec spec_b = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec *const specs[] = {&spec_a, &spec_b};

static int pin_level(int id)
{
	return gpio_emul_output_get_dt(specs[id]);
}

/** @brief Observa o pino por @p samples leituras e exige ver 1 e 0. */
static void check_blinking(int id, int samples, int step_ms, const char *name)
{
	int seen_on = 0;
	int seen_off = 0;
	int i;

	for (i = 0; i < samples; i++) {
		int level;

		k_msleep(step_ms);
		level = pin_level(id);
		zassert_true(level >= 0, "%s: pin read failed (%d)", name, level);

		if (level > 0) {
			seen_on++;
		} else {
			seen_off++;
		}
	}

	zassert_true(seen_on > 0, "%s was never seen on", name);
	zassert_true(seen_off > 0, "%s was never seen off", name);
}

/** @brief Conta quantas vezes o pino mudou de nivel. */
static int count_toggles(int id, int samples, int step_ms)
{
	int previous = pin_level(id);
	int toggles = 0;
	int i;

	for (i = 0; i < samples; i++) {
		int level;

		k_msleep(step_ms);
		level = pin_level(id);
		if (level != previous) {
			toggles++;
		}
		previous = level;
	}

	return toggles;
}

/** @brief Mede o intervalo entre duas trocas de nivel de um LED (ms). */
static int measure_period_ms(int led, int max_samples)
{
	int64_t last_edge = 0;
	int64_t delta = -1;
	int previous = pin_level(led);
	int i;

	for (i = 0; i < max_samples; i++) {
		int level;

		k_msleep(1);
		level = pin_level(led);

		if (level != previous) {
			int64_t now = k_uptime_get();

			if (last_edge != 0) {
				delta = now - last_edge;
				break;
			}
			last_edge = now;
		}
		previous = level;
	}

	return (int)delta;
}

static void *suite_setup(void)
{
	/* 1) Controlador deferido: o init tem que recusar. */
	zassert_equal(led_init(), -ENODEV, "led_init should fail while the GPIO is not ready");

	/* 2) Inicializa o emulador manualmente e agora o init passa. */
	zassert_ok(device_init(gpio_test), "failed to init the gpio-emul");
	zassert_true(device_is_ready(gpio_test), "gpio-emul is still not ready");
	zassert_ok(led_init(), "led_init should work after device_init");

	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* led_init e idempotente: reconfigura as saidas e zera os piscas. */
	zassert_ok(led_init());
	zassert_ok(led_off(LED_ALL));
}

ZTEST(led_driver, test_init_turns_all_leds_off)
{
	zassert_equal(led_count(), 2U, "the test overlay declares 2 LEDs");
	zassert_equal(pin_level(LED_A), 0, "led0 should start off");
	zassert_equal(pin_level(LED_B), 0, "led1 should start off");
}

ZTEST(led_driver, test_on_off_toggle_change_pin)
{
	zassert_ok(led_on(LED_A));
	zassert_equal(pin_level(LED_A), 1, "led_on did not drive the pin high");

	zassert_ok(led_off(LED_A));
	zassert_equal(pin_level(LED_A), 0, "led_off did not drive the pin low");

	zassert_ok(led_toggle(LED_A));
	zassert_equal(pin_level(LED_A), 1, "toggle should turn it on");

	zassert_ok(led_toggle(LED_A));
	zassert_equal(pin_level(LED_A), 0, "toggle should turn it off");
}

ZTEST(led_driver, test_blink_seen_on_and_off)
{
	zassert_ok(led_blink(LED_A, LED_PATTERN_FAST));

	check_blinking(LED_A, 20, 5, "led0 blinking");
}

ZTEST(led_driver, test_two_leds_different_rates)
{
	zassert_ok(led_blink(LED_A, LED_PATTERN_SLOW));
	zassert_ok(led_blink(LED_B, LED_PATTERN_FAST));

	/* 300 ms: o rapido alterna bem mais que o lento. */
	int slow = count_toggles(LED_A, 60, 5);
	int fast = count_toggles(LED_B, 60, 5);

	zassert_true(fast > slow, "the fast LED (%d) should toggle more than the slow one (%d)",
		     fast, slow);
	zassert_true(slow > 0, "the slow LED should have toggled at least once");
}

ZTEST(led_driver, test_actions_cancel_blink)
{
	int level;
	int i;

	/* led_on cancela */
	zassert_ok(led_blink(LED_A, LED_PATTERN_FAST));
	k_msleep(15);
	zassert_ok(led_on(LED_A));
	for (i = 0; i < 10; i++) {
		k_msleep(5);
		zassert_equal(pin_level(LED_A), 1, "LED resumed blinking after led_on");
	}

	/* led_off cancela */
	zassert_ok(led_blink(LED_A, LED_PATTERN_FAST));
	k_msleep(15);
	zassert_ok(led_off(LED_A));
	for (i = 0; i < 10; i++) {
		k_msleep(5);
		zassert_equal(pin_level(LED_A), 0, "LED resumed blinking after led_off");
	}

	/* led_toggle cancela e fixa o nivel */
	zassert_ok(led_blink(LED_A, LED_PATTERN_FAST));
	k_msleep(15);
	zassert_ok(led_toggle(LED_A));
	level = pin_level(LED_A);
	for (i = 0; i < 10; i++) {
		k_msleep(5);
		zassert_equal(pin_level(LED_A), level, "LED resumed blinking after led_toggle");
	}
}

ZTEST(led_driver, test_all_action_affects_every_led)
{
	zassert_ok(led_on(LED_ALL));
	zassert_equal(pin_level(LED_A), 1, "led0 was not turned on by 'all'");
	zassert_equal(pin_level(LED_B), 1, "led1 was not turned on by 'all'");

	zassert_ok(led_off(LED_ALL));
	zassert_equal(pin_level(LED_A), 0, "led0 was not turned off by 'all'");
	zassert_equal(pin_level(LED_B), 0, "led1 was not turned off by 'all'");

	zassert_ok(led_toggle(LED_ALL));
	zassert_equal(pin_level(LED_A), 1, "toggle on all should turn led0 on");
	zassert_equal(pin_level(LED_B), 1, "toggle on all should turn led1 on");
}

ZTEST(led_driver, test_blink_all_starts_every_led)
{
	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));

	check_blinking(LED_A, 20, 5, "led0 during blink_all");
	check_blinking(LED_B, 20, 5, "led1 during blink_all");
}

ZTEST(led_driver, test_invalid_arguments_return_einval)
{
	zassert_equal(led_on(led_count()), -EINVAL, "out-of-range index should fail");
	zassert_equal(led_off(led_count()), -EINVAL);
	zassert_equal(led_toggle(led_count()), -EINVAL);
	zassert_equal(led_apply(LED_A, LED_ACTION_COUNT, LED_PATTERN_SLOW), -EINVAL,
		      "invalid action should fail");
	zassert_equal(led_apply(LED_ALL, LED_ACTION_COUNT, LED_PATTERN_SLOW), -EINVAL);
	zassert_equal(led_blink(LED_A, LED_PATTERN_COUNT), -EINVAL, "invalid pattern should fail");
	zassert_equal(led_blink(LED_ALL, LED_PATTERN_COUNT), -EINVAL);
}

ZTEST(led_driver, test_reinit_keeps_timer_working)
{
	/* Reinit no meio da operacao nao pode matar o timer unico. */
	zassert_ok(led_init());
	zassert_ok(led_init());

	zassert_ok(led_blink(LED_B, LED_PATTERN_FAST));
	check_blinking(LED_B, 20, 5, "led1 after double led_init");
}

ZTEST(led_driver, test_other_led_keeps_phase)
{
	int before;
	int after;

	zassert_ok(led_blink(LED_B, LED_PATTERN_FAST));

	before = measure_period_ms(LED_B, 200);
	zassert_true(before > 0, "could not measure led1 initial period");

	/* Mexe no outro LED varias vezes, no meio do pisca do led1. */
	zassert_ok(led_blink(LED_A, LED_PATTERN_FAST));
	k_msleep(7);
	zassert_ok(led_on(LED_A));
	zassert_ok(led_off(LED_A));
	zassert_ok(led_blink(LED_A, LED_PATTERN_SLOW));

	after = measure_period_ms(LED_B, 200);
	zassert_true(after > 0, "could not measure led1 final period");

	zassert_true((after - before <= 4) && (before - after <= 4),
		     "led1 phase shifted when led0 was touched (before=%d ms, after=%d ms)", before,
		     after);
}

ZTEST_SUITE(led_driver, NULL, suite_setup, before_each, NULL, NULL);
