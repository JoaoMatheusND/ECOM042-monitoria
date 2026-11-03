/*******************************************************************
 * @file led.c
 *
 * @brief Modulo de LED com um unico k_timer one-shot multiplexado.
 *
 * Cada LED guarda seu proprio deadline; o callback do timer inverte
 * os LEDs vencidos e rearma o timer para o deadline mais proximo.
 *******************************************************************/

#include "led.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(led, CONFIG_LED_MODULE_LOG_LEVEL);

/* LED parado: deadline que nunca vence */
#define NO_DEADLINE INT64_MAX

#define LED_SPEC(i, _)                                                                             \
	IF_ENABLED(DT_HAS_ALIAS(led##i), (GPIO_DT_SPEC_GET(DT_ALIAS(led##i), gpios), ))

static const struct gpio_dt_spec leds[] = {LISTIFY(CONFIG_LED_MAX_ALIASES, LED_SPEC, ())};

BUILD_ASSERT(ARRAY_SIZE(leds) > 0, "nenhum alias ledN no devicetree");

struct led_state {
	enum led_pattern pattern;
	uint32_t half_period_ms;
	int64_t deadline_ms;
	bool blinking;
};

/* periodo completo de cada padrao; o LED inverte a cada meio periodo */
static const uint32_t pattern_period_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_LED_BLINK_SLOW_PERIOD_MS,
	[LED_PATTERN_NORMAL] = CONFIG_LED_BLINK_NORMAL_PERIOD_MS,
	[LED_PATTERN_FAST] = CONFIG_LED_BLINK_FAST_PERIOD_MS,
};

static struct led_state states[ARRAY_SIZE(leds)];
static struct k_spinlock lock;

static void led_timer_cb(struct k_timer *timer);

K_TIMER_DEFINE(led_timer, led_timer_cb, NULL);

/* Rearma o timer one-shot para o deadline mais proximo (ou para). */
static void reschedule_locked(void)
{
	int64_t next = NO_DEADLINE;

	for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
		next = MIN(next, states[i].deadline_ms);
	}

	if (next == NO_DEADLINE) {
		k_timer_stop(&led_timer);
		return;
	}

	k_timer_start(&led_timer, K_TIMEOUT_ABS_MS(next), K_NO_WAIT);
}

/* Contexto de ISR: so GPIO, deadlines e rearme. */
static void led_timer_cb(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_get();

	ARG_UNUSED(timer);

	for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
		struct led_state *st = &states[i];

		if (st->deadline_ms <= now) {
			gpio_pin_toggle_dt(&leds[i]);
			/* soma ao deadline anterior para nao acumular atraso */
			st->deadline_ms += st->half_period_ms;
		}
	}

	reschedule_locked();
	k_spin_unlock(&lock, key);
}

static void stop_blink_locked(struct led_state *st)
{
	st->blinking = false;
	st->deadline_ms = NO_DEADLINE;
}

typedef int (*led_action_t)(size_t idx, enum led_pattern pattern);

static int action_on(size_t idx, enum led_pattern pattern)
{
	ARG_UNUSED(pattern);
	stop_blink_locked(&states[idx]);
	return gpio_pin_set_dt(&leds[idx], 1);
}

static int action_off(size_t idx, enum led_pattern pattern)
{
	ARG_UNUSED(pattern);
	stop_blink_locked(&states[idx]);
	return gpio_pin_set_dt(&leds[idx], 0);
}

static int action_toggle(size_t idx, enum led_pattern pattern)
{
	ARG_UNUSED(pattern);
	stop_blink_locked(&states[idx]);
	return gpio_pin_toggle_dt(&leds[idx]);
}

static int action_blink(size_t idx, enum led_pattern pattern)
{
	struct led_state *st = &states[idx];

	st->pattern = pattern;
	st->half_period_ms = pattern_period_ms[pattern] / 2;
	st->deadline_ms = k_uptime_get() + st->half_period_ms;
	st->blinking = true;

	/* comeca aceso; a primeira inversao fica para o deadline */
	return gpio_pin_set_dt(&leds[idx], 1);
}

/* Aplica a acao em um LED ou em todos (LED_ALL) e rearma o timer. */
static int apply(int led, led_action_t action, enum led_pattern pattern)
{
	size_t first = (size_t)led;
	size_t last = (size_t)led + 1;
	k_spinlock_key_t key;
	int ret = 0;

	if (led == LED_ALL) {
		first = 0;
		last = ARRAY_SIZE(leds);
	} else if (led < 0 || (size_t)led >= ARRAY_SIZE(leds)) {
		LOG_ERR("LED invalido: %d (existem %zu)", led, ARRAY_SIZE(leds));
		return -EINVAL;
	}

	key = k_spin_lock(&lock);
	for (size_t i = first; i < last; i++) {
		int err = action(i, pattern);

		ret = (ret == 0) ? err : ret;
	}
	reschedule_locked();
	k_spin_unlock(&lock, key);

	if (ret < 0) {
		LOG_ERR("falha no GPIO do LED %d (%d)", led, ret);
	}

	return ret;
}

int led_init(void)
{
	k_spinlock_key_t key;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			LOG_ERR("controlador do LED %zu nao esta pronto", i);
			return -ENODEV;
		}
	}

	key = k_spin_lock(&lock);
	k_timer_stop(&led_timer);
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		stop_blink_locked(&states[i]);
	}
	k_spin_unlock(&lock, key);

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			LOG_ERR("falha ao configurar o LED %zu (%d)", i, ret);
			return ret;
		}
	}

	return 0;
}

size_t led_count(void)
{
	return ARRAY_SIZE(leds);
}

int led_on(int led)
{
	return apply(led, action_on, LED_PATTERN_NORMAL);
}

int led_off(int led)
{
	return apply(led, action_off, LED_PATTERN_NORMAL);
}

int led_toggle(int led)
{
	return apply(led, action_toggle, LED_PATTERN_NORMAL);
}

int led_blink(int led, enum led_pattern pattern)
{
	if ((unsigned int)pattern >= LED_PATTERN_COUNT) {
		LOG_ERR("padrao invalido: %d", pattern);
		return -EINVAL;
	}

	return apply(led, action_blink, pattern);
}
