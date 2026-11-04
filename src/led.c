/*******************************************************************
 * @file led.c
 *
 * @brief N LEDs independentes multiplexados em um unico k_timer.
 *******************************************************************/

#include "led.h"

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(app_led, CONFIG_APP_LED_LOG_LEVEL);

#define LED_MAX_ALIASES 16

#define LED_SPEC(i, _)                                                                             \
	COND_CODE_1(DT_NODE_EXISTS(DT_ALIAS(led##i)),                                              \
		    (GPIO_DT_SPEC_GET(DT_ALIAS(led##i), gpios), ), ())

static const struct gpio_dt_spec led_specs[] = {LISTIFY(LED_MAX_ALIASES, LED_SPEC, ())};

#define NUM_LEDS ((int)ARRAY_SIZE(led_specs))

static const uint32_t pattern_period_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_APP_LED_SLOW_PERIOD_MS,
	[LED_PATTERN_NORMAL] = CONFIG_APP_LED_NORMAL_PERIOD_MS,
	[LED_PATTERN_FAST] = CONFIG_APP_LED_FAST_PERIOD_MS,
};

struct led_state {
	enum led_pattern pattern;
	k_ticks_t half_period;
	k_ticks_t deadline;
	bool blinking;
	bool on;
};

struct led_command {
	const char *name;
	void (*execute)(int idx, enum led_pattern pattern, k_ticks_t now);
};

static struct led_state leds[NUM_LEDS];
static struct k_timer blink_timer;
static struct k_spinlock lock;
static bool timer_initialized;
static bool initialized;

static void led_write(int idx, bool on)
{
	gpio_pin_set_dt(&led_specs[idx], on);
	leds[idx].on = on;
}

static void cmd_on(int idx, enum led_pattern pattern, k_ticks_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	leds[idx].blinking = false;
	led_write(idx, true);
}

static void cmd_off(int idx, enum led_pattern pattern, k_ticks_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	leds[idx].blinking = false;
	led_write(idx, false);
}

static void cmd_toggle(int idx, enum led_pattern pattern, k_ticks_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	leds[idx].blinking = false;
	led_write(idx, !leds[idx].on);
}

static void cmd_blink(int idx, enum led_pattern pattern, k_ticks_t now)
{
	struct led_state *s = &leds[idx];

	s->pattern = pattern;
	s->half_period = MAX((k_ticks_t)k_ms_to_ticks_ceil64(pattern_period_ms[pattern] / 2), 1);
	s->deadline = now + s->half_period;
	s->blinking = true;
	led_write(idx, true);
}

static const struct led_command commands[LED_ACTION_COUNT] = {
	[LED_ACTION_ON] = {"on", cmd_on},
	[LED_ACTION_OFF] = {"off", cmd_off},
	[LED_ACTION_TOGGLE] = {"toggle", cmd_toggle},
	[LED_ACTION_BLINK] = {"blink", cmd_blink},
};

static void for_each_target(int led, const struct led_command *cmd, enum led_pattern pattern)
{
	int first = (led == LED_ALL) ? 0 : led;
	int last = (led == LED_ALL) ? NUM_LEDS - 1 : led;
	k_ticks_t now = k_uptime_ticks();

	for (int i = first; i <= last; i++) {
		cmd->execute(i, pattern, now);
	}
}

static void timer_rearm(void)
{
	k_ticks_t next = INT64_MAX;

	for (int i = 0; i < NUM_LEDS; i++) {
		if (leds[i].blinking) {
			next = MIN(next, leds[i].deadline);
		}
	}

	if (next == INT64_MAX) {
		k_timer_stop(&blink_timer);
		return;
	}
	k_timer_start(&blink_timer, K_TIMEOUT_ABS_TICKS(next), K_NO_WAIT);
}

static void blink_expiry(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	k_ticks_t now = k_uptime_ticks();

	ARG_UNUSED(timer);

	for (int i = 0; i < NUM_LEDS; i++) {
		struct led_state *s = &leds[i];

		if (s->blinking && s->deadline <= now) {
			led_write(i, !s->on);
			s->deadline += s->half_period;
		}
	}
	timer_rearm();

	k_spin_unlock(&lock, key);
}

static int validate(int led, enum led_action action, enum led_pattern pattern)
{
	if (led != LED_ALL && (led < 0 || led >= NUM_LEDS)) {
		LOG_ERR("LED invalido: %d (validos: 0..%d ou LED_ALL)", led, NUM_LEDS - 1);
		return -EINVAL;
	}
	if ((unsigned int)action >= LED_ACTION_COUNT) {
		LOG_ERR("acao invalida: %d", action);
		return -EINVAL;
	}
	if ((unsigned int)pattern >= LED_PATTERN_COUNT) {
		LOG_ERR("padrao invalido: %d", pattern);
		return -EINVAL;
	}
	return 0;
}

int led_init(void)
{
	k_spinlock_key_t key;
	int ret;

	for (int i = 0; i < NUM_LEDS; i++) {
		if (!gpio_is_ready_dt(&led_specs[i])) {
			LOG_ERR("GPIO do led%d nao esta pronto", i);
			initialized = false;
			return -ENODEV;
		}
	}

	if (!timer_initialized) {
		k_timer_init(&blink_timer, blink_expiry, NULL);
		timer_initialized = true;
	}
	k_timer_stop(&blink_timer);

	key = k_spin_lock(&lock);
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = (struct led_state){0};
	}
	k_spin_unlock(&lock, key);

	for (int i = 0; i < NUM_LEDS; i++) {
		ret = gpio_pin_configure_dt(&led_specs[i], GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("falha ao configurar led%d (%d)", i, ret);
			initialized = false;
			return ret;
		}
	}

	initialized = true;
	return 0;
}

int led_count(void)
{
	return NUM_LEDS;
}

int led_execute(int led, enum led_action action, enum led_pattern pattern)
{
	k_spinlock_key_t key;
	int ret = validate(led, action, pattern);

	if (ret != 0) {
		return ret;
	}
	if (!initialized) {
		LOG_ERR("led_init() nao foi chamada com sucesso");
		return -ENODEV;
	}

	LOG_DBG("%s led=%d pattern=%d", commands[action].name, led, pattern);

	key = k_spin_lock(&lock);
	for_each_target(led, &commands[action], pattern);
	timer_rearm();
	k_spin_unlock(&lock, key);

	return 0;
}

int led_on(int led)
{
	return led_execute(led, LED_ACTION_ON, LED_PATTERN_NORMAL);
}

int led_off(int led)
{
	return led_execute(led, LED_ACTION_OFF, LED_PATTERN_NORMAL);
}

int led_toggle(int led)
{
	return led_execute(led, LED_ACTION_TOGGLE, LED_PATTERN_NORMAL);
}

int led_blink(int led, enum led_pattern pattern)
{
	return led_execute(led, LED_ACTION_BLINK, pattern);
}

bool led_is_on(int led)
{
	if (led < 0 || led >= NUM_LEDS) {
		return false;
	}
	return leds[led].on;
}
