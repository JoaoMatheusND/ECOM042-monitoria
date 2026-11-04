/**
 * @file led.c
 *
 * @brief LED driver: several LEDs blinking independently with a single timer.
 *
 * One one-shot k_timer is shared by all LEDs. After every change it is
 * re-armed to the earliest deadline among the blinking LEDs.
 */
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include "led.h"
#include "led_cmd.h"

LOG_MODULE_REGISTER(led, LOG_LEVEL_DBG);

/*
 * LEDs come from the devicetree: every enabled child of a "gpio-leds" node.
 * The LED index is the order of declaration in the devicetree (the first
 * LED declared is index 0), regardless of the alias names (led0, led1, ...).
 * A new LED only needs a new node in the devicetree.
 */
#define LED_STATE_FROM_CHILD(node) { .gpio = GPIO_DT_SPEC_GET(node, gpios) },
#define LED_STATES_FROM_PARENT(node) DT_FOREACH_CHILD_STATUS_OKAY(node, LED_STATE_FROM_CHILD)

static struct led_state leds[] = {
	DT_FOREACH_STATUS_OKAY(gpio_leds, LED_STATES_FROM_PARENT)
};

#define LED_COUNT ARRAY_SIZE(leds)

static struct k_spinlock lock;

static void led_timer_expiry(struct k_timer *timer);

K_TIMER_DEFINE(led_timer, led_timer_expiry, NULL);

/* Arm the timer to the earliest deadline, or stop it. Call with lock held. */
static void led_timer_rearm(void)
{
	int64_t now = k_uptime_get();
	int64_t next = INT64_MAX;

	for (size_t i = 0; i < LED_COUNT; i++) {
		if (leds[i].blinking) {
			next = MIN(next, leds[i].deadline_ms);
		}
	}

	if (next == INT64_MAX) {
		k_timer_stop(&led_timer);
		return;
	}

	/* A deadline already in the past fires immediately. */
	k_timer_start(&led_timer, K_MSEC(MAX(0, next - now)), K_NO_WAIT);
}

/* ISR context */
static void led_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_get();

	for (size_t i = 0; i < LED_COUNT; i++) {
		if (leds[i].blinking && leds[i].deadline_ms <= now) {
			(void)gpio_pin_toggle_dt(&leds[i].gpio);
			leds[i].deadline_ms += led_half_period_ms(&leds[i]);
		}
	}

	led_timer_rearm();

	k_spin_unlock(&lock, key);
}

/* The only place that handles "one LED or all LEDs". */
static int led_apply(int led, enum led_action action, int arg)
{
	if (led != LED_ALL && (led < 0 || led >= (int)LED_COUNT)) {
		LOG_ERR("Invalid LED index: %d", led);
		return -EINVAL;
	}

	const struct led_cmd *cmd = &led_cmd_table_get()[action];
	size_t first = (led == LED_ALL) ? 0 : (size_t)led;
	size_t last = (led == LED_ALL) ? LED_COUNT : (size_t)led + 1;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&lock);

	for (size_t i = first; i < last && ret == 0; i++) {
		ret = cmd->handler(&leds[i], arg);
	}

	led_timer_rearm();

	k_spin_unlock(&lock, key);

	return ret;
}

int led_init(void)
{
	for (size_t i = 0; i < LED_COUNT; i++) {
		if (!gpio_is_ready_dt(&leds[i].gpio)) {
			LOG_ERR("GPIO controller of LED %zu is not ready", i);
			return -ENODEV;
		}
	}

	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&lock);

	k_timer_stop(&led_timer);

	for (size_t i = 0; i < LED_COUNT && ret == 0; i++) {
		leds[i] = (struct led_state){ .gpio = leds[i].gpio };
		ret = gpio_pin_configure_dt(&leds[i].gpio, GPIO_OUTPUT_INACTIVE);
	}

	k_spin_unlock(&lock, key);

	if (ret != 0) {
		LOG_ERR("Failed to configure LED pins (%d)", ret);
	}

	return ret;
}

int led_on(int led)
{
	return led_apply(led, LED_ACT_ON, 0);
}

int led_off(int led)
{
	return led_apply(led, LED_ACT_OFF, 0);
}

int led_toggle(int led)
{
	return led_apply(led, LED_ACT_TOGGLE, 0);
}

int led_blink(int led, enum led_profile profile)
{
	return led_apply(led, LED_ACT_BLINK, profile);
}
