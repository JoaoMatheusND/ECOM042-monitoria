/*******************************************************************
 * @file led.c
 *
 * @brief Modulo de LEDs com pisca independente usando um unico timer.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "led.h"

LOG_MODULE_REGISTER(led, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_SPEC(alias) GPIO_DT_SPEC_GET(DT_ALIAS(alias), gpios)

static const struct gpio_dt_spec leds[] = {
	LED_SPEC(led0),
	LED_SPEC(led1),
	LED_SPEC(led2),
};

static const uint32_t pattern_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_BLINK_SLOW_MS,
	[LED_PATTERN_NORMAL] = CONFIG_BLINK_NORMAL_MS,
	[LED_PATTERN_FAST] = CONFIG_BLINK_FAST_MS,
};

typedef struct {
	bool blinking;
	uint32_t period_ms;
	int64_t deadline;
} led_state_t;

typedef int (*led_action_t)(size_t led, uint32_t arg);

static led_state_t states[ARRAY_SIZE(leds)];
static struct k_spinlock lock;
static bool ready;

static void blink_expiry(struct k_timer *timer);

static K_TIMER_DEFINE(blink_timer, blink_expiry, NULL);

/* arma o timer (one-shot) para o prazo mais proximo; chamar com o lock */
static void rearm_locked(void)
{
	int64_t now = k_uptime_get();
	int64_t next = INT64_MAX;

	for (size_t i = 0U; i < ARRAY_SIZE(leds); i++) {
		if (states[i].blinking && (states[i].deadline < next)) {
			next = states[i].deadline;
		}
	}

	if (next == INT64_MAX) {
		k_timer_stop(&blink_timer);
		return;
	}

	k_timer_start(&blink_timer, K_MSEC(MAX(next - now, (int64_t)1)), K_NO_WAIT);
}

/* contexto de ISR: so GPIO, prazos e rearme */
static void blink_expiry(struct k_timer *timer)
{
	k_spinlock_key_t key;
	int64_t now;

	ARG_UNUSED(timer);

	key = k_spin_lock(&lock);
	now = k_uptime_get();

	for (size_t i = 0U; i < ARRAY_SIZE(leds); i++) {
		if (!states[i].blinking || (now < states[i].deadline)) {
			continue;
		}

		(void)gpio_pin_toggle_dt(&leds[i]);

		states[i].deadline += states[i].period_ms;
		if (states[i].deadline <= now) {
			states[i].deadline = now + states[i].period_ms;
		}
	}

	rearm_locked();
	k_spin_unlock(&lock, key);
}

static int action_on(size_t led, uint32_t arg)
{
	ARG_UNUSED(arg);

	states[led].blinking = false;

	return gpio_pin_set_dt(&leds[led], 1);
}

static int action_off(size_t led, uint32_t arg)
{
	ARG_UNUSED(arg);

	states[led].blinking = false;

	return gpio_pin_set_dt(&leds[led], 0);
}

static int action_toggle(size_t led, uint32_t arg)
{
	ARG_UNUSED(arg);

	states[led].blinking = false;

	return gpio_pin_toggle_dt(&leds[led]);
}

static int action_blink(size_t led, uint32_t pattern)
{
	states[led].period_ms = pattern_ms[pattern];
	states[led].deadline = k_uptime_get() + (int64_t)states[led].period_ms;
	states[led].blinking = true;

	return 0;
}

/* aplica a acao a um LED ou a todos (LED_ALL) */
static int led_apply(size_t led, led_action_t action, uint32_t arg)
{
	k_spinlock_key_t key;
	int ret = 0;

	if (!ready) {
		LOG_ERR("modulo nao inicializado");
		return -ENODEV;
	}

	if ((led != LED_ALL) && (led >= ARRAY_SIZE(leds))) {
		LOG_ERR("LED %zu invalido", led);
		return -EINVAL;
	}

	key = k_spin_lock(&lock);

	if (led == LED_ALL) {
		for (size_t i = 0U; i < ARRAY_SIZE(leds); i++) {
			int err = action(i, arg);

			if (err < 0) {
				ret = err;
			}
		}
	} else {
		ret = action(led, arg);
	}

	rearm_locked();
	k_spin_unlock(&lock, key);

	return ret;
}

int led_init(void)
{
	k_spinlock_key_t key;

	for (size_t i = 0U; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			LOG_ERR("controlador do LED %zu nao esta pronto", i);
			return -ENODEV;
		}
	}

	/* para o timer antes de mexer no estado, por causa do reinit */
	k_timer_stop(&blink_timer);

	key = k_spin_lock(&lock);

	for (size_t i = 0U; i < ARRAY_SIZE(leds); i++) {
		int ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			k_spin_unlock(&lock, key);
			LOG_ERR("falha ao configurar o LED %zu (%d)", i, ret);
			return ret;
		}

		states[i].blinking = false;
		states[i].period_ms = 0U;
		states[i].deadline = 0;
	}

	ready = true;
	k_spin_unlock(&lock, key);

	return 0;
}

int led_on(size_t led)
{
	return led_apply(led, action_on, 0U);
}

int led_off(size_t led)
{
	return led_apply(led, action_off, 0U);
}

int led_toggle(size_t led)
{
	return led_apply(led, action_toggle, 0U);
}

int led_blink(size_t led, led_pattern_t pattern)
{
	if ((unsigned int)pattern >= (unsigned int)LED_PATTERN_COUNT) {
		LOG_ERR("padrao %u invalido", (unsigned int)pattern);
		return -EINVAL;
	}

	return led_apply(led, action_blink, (uint32_t)pattern);
}

size_t led_count(void)
{
	return ARRAY_SIZE(leds);
}
