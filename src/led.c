#include "led.h"

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(leds, LOG_LEVEL_INF);

/* Os aliases devem ser consecutivos: led0, led1, ... ate led7. */
#define LED_SPEC(n, unused)                                                                        \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_ALIAS(led##n), okay),                                    \
		    (GPIO_DT_SPEC_GET(DT_ALIAS(led##n), gpios), ), ())

static const struct gpio_dt_spec pins[] = {LISTIFY(8, LED_SPEC, ())};
BUILD_ASSERT(ARRAY_SIZE(pins) >= 2, "Declare pelo menos dois LEDs no devicetree");

/* Cada intervalo precisa ser representavel por ticks inteiros do kernel. */
BUILD_ASSERT((CONFIG_LED_SLOW_MS * (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC) % MSEC_PER_SEC == 0,
	     "LED_SLOW_MS deve ser multiplo de um tick do kernel");
BUILD_ASSERT((CONFIG_LED_NORMAL_MS * (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC) % MSEC_PER_SEC == 0,
	     "LED_NORMAL_MS deve ser multiplo de um tick do kernel");
BUILD_ASSERT((CONFIG_LED_FAST_MS * (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC) % MSEC_PER_SEC == 0,
	     "LED_FAST_MS deve ser multiplo de um tick do kernel");

struct led_state {
	bool on;
	bool blinking;
	enum led_pattern pattern;
	int64_t period;
	int64_t deadline;
};

static struct led_state states[ARRAY_SIZE(pins)];
static struct k_timer blink_timer;
static struct k_spinlock lock;
static bool timer_initialized;
static bool initialized;

static const int32_t periods[LED_PATTERN_COUNT] = {
	[LED_SLOW] = CONFIG_LED_SLOW_MS,
	[LED_NORMAL] = CONFIG_LED_NORMAL_MS,
	[LED_FAST] = CONFIG_LED_FAST_MS,
};

static void rearm(int64_t now)
{
	int64_t nearest = INT64_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
		if (states[i].blinking && states[i].deadline < nearest) {
			nearest = states[i].deadline;
		}
	}
	if (nearest == INT64_MAX) {
		k_timer_stop(&blink_timer);
		return;
	}
	k_timer_start(&blink_timer, K_MSEC(MAX(nearest - now, 1)), K_NO_WAIT);
}

static void timer_expired(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_get();

	ARG_UNUSED(timer);
	for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
		struct led_state *state = &states[i];

		if (!state->blinking || state->deadline > now) {
			continue;
		}
		if (gpio_pin_set_dt(&pins[i], !state->on) < 0) {
			state->blinking = false;
			continue;
		}
		state->on = !state->on;
		/* Avanca a partir do deadline anterior para nao acumular atraso. */
		state->deadline += ((now - state->deadline) / state->period + 1) * state->period;
	}
	rearm(now);
	k_spin_unlock(&lock, key);
}

int leds_init(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int ret = 0;

	if (!timer_initialized) {
		k_timer_init(&blink_timer, timer_expired, NULL);
		timer_initialized = true;
	}
	k_timer_stop(&blink_timer);
	initialized = false;
	for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
		states[i] = (struct led_state){0};
	}
	for (size_t i = 0; i < ARRAY_SIZE(pins); i++) {
		if (!gpio_is_ready_dt(&pins[i])) {
			ret = -ENODEV;
			break;
		}
		ret = gpio_pin_configure_dt(&pins[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			break;
		}
	}
	initialized = ret == 0;
	k_spin_unlock(&lock, key);
	return ret;
}

size_t leds_count(void)
{
	return ARRAY_SIZE(pins);
}

static int set_level(size_t index, bool on)
{
	int ret;

	states[index].blinking = false;
	ret = gpio_pin_set_dt(&pins[index], on);
	if (ret == 0) {
		states[index].on = on;
	}
	return ret;
}

static int on_execute(size_t index, enum led_pattern pattern, int64_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	return set_level(index, true);
}

static int off_execute(size_t index, enum led_pattern pattern, int64_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	return set_level(index, false);
}

static int toggle_execute(size_t index, enum led_pattern pattern, int64_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	return set_level(index, !states[index].on);
}

static int blink_execute(size_t index, enum led_pattern pattern, int64_t now)
{
	states[index].pattern = pattern;
	states[index].period = periods[pattern];
	states[index].deadline = now + states[index].period;
	states[index].blinking = true;
	return 0;
}

enum led_action {
	ACTION_ON,
	ACTION_OFF,
	ACTION_TOGGLE,
	ACTION_BLINK,
	ACTION_COUNT
};

static int (*const actions[ACTION_COUNT])(size_t index, enum led_pattern pattern, int64_t now) = {
	[ACTION_ON] = on_execute,
	[ACTION_OFF] = off_execute,
	[ACTION_TOGGLE] = toggle_execute,
	[ACTION_BLINK] = blink_execute,
};

/* A selecao de um LED ou de todos fica somente neste helper. */
static int apply(int id, enum led_action action, enum led_pattern pattern)
{
	k_spinlock_key_t key;
	size_t first;
	size_t end;
	int64_t now;
	int ret = 0;

	if (id != LED_ALL && (id < 0 || (size_t)id >= ARRAY_SIZE(pins))) {
		LOG_ERR("LED invalido: %d", id);
		return -EINVAL;
	}
	key = k_spin_lock(&lock);
	if (!initialized) {
		k_spin_unlock(&lock, key);
		return -EACCES;
	}
	first = id == LED_ALL ? 0 : (size_t)id;
	end = id == LED_ALL ? ARRAY_SIZE(pins) : first + 1;
	now = k_uptime_get();
	for (size_t i = first; i < end; i++) {
		ret = actions[action](i, pattern, now);
		if (ret < 0) {
			break;
		}
	}
	rearm(now);
	k_spin_unlock(&lock, key);
	return ret;
}

int led_on(int id)
{
	return apply(id, ACTION_ON, LED_NORMAL);
}

int led_off(int id)
{
	return apply(id, ACTION_OFF, LED_NORMAL);
}

int led_toggle(int id)
{
	return apply(id, ACTION_TOGGLE, LED_NORMAL);
}

int led_blink(int id, enum led_pattern pattern)
{
	if ((int)pattern < 0 || pattern >= LED_PATTERN_COUNT) {
		LOG_ERR("Padrao invalido: %d", (int)pattern);
		return -EINVAL;
	}
	return apply(id, ACTION_BLINK, pattern);
}
