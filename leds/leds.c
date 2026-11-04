#include "leds.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(leds, CONFIG_APP_LEDS_LOG_LEVEL);


struct leds_state {
	const struct gpio_dt_spec gpio;
	enum leds_pattern pattern; /* padrão atual */
	uint32_t interval_ms;      /* tempo entre trocas de estado */
	int64_t deadline_ms;       /* próxima troca (uptime absoluto) */
	bool blinking;             /* está piscando? */
};

#define LEDS_MAX_ALIASES 8

#define LEDS_ENTRY(i, _)                                                       \
	COND_CODE_1(DT_NODE_EXISTS(DT_ALIAS(led##i)),                          \
		    ({ .gpio = GPIO_DT_SPEC_GET(DT_ALIAS(led##i), gpios) },), ())

static struct leds_state leds[] = {
	LISTIFY(LEDS_MAX_ALIASES, LEDS_ENTRY, ())
};

BUILD_ASSERT(ARRAY_SIZE(leds) > 0, "Nenhum alias led0, led1, ... no devicetree");

static const uint32_t pattern_ms[LEDS_PATTERN_COUNT] = {
	[LEDS_PATTERN_SLOW]   = CONFIG_APP_LEDS_SLOW_MS,
	[LEDS_PATTERN_NORMAL] = CONFIG_APP_LEDS_NORMAL_MS,
	[LEDS_PATTERN_FAST]   = CONFIG_APP_LEDS_FAST_MS,
};


static void leds_timer_expired(struct k_timer *timer);

K_TIMER_DEFINE(leds_timer, leds_timer_expired, NULL);

static struct k_spinlock leds_lock;
static bool initialized;

static void leds_reschedule(int64_t now)
{
	int64_t next = INT64_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		bool earlier = leds[i].blinking && leds[i].deadline_ms < next;

		next = earlier ? leds[i].deadline_ms : next;
	}

	if (next == INT64_MAX) {
		k_timer_stop(&leds_timer);
		return;
	}

	k_timer_start(&leds_timer, K_MSEC((uint32_t)MAX(next - now, 0)), K_NO_WAIT);
}

static void leds_timer_expired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_spinlock_key_t key = k_spin_lock(&leds_lock);
	int64_t now = k_uptime_get();

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		struct leds_state *led = &leds[i];

		if (!led->blinking || led->deadline_ms > now) {
			continue;
		}

		(void)gpio_pin_toggle_dt(&led->gpio);
		led->deadline_ms += led->interval_ms;

		led->deadline_ms = MAX(led->deadline_ms, now + 1);
	}

	leds_reschedule(now);
	k_spin_unlock(&leds_lock, key);
}


typedef void (*leds_cmd_fn)(struct leds_state *led, uint32_t arg, int64_t now);

struct leds_cmd_desc {
	leds_cmd_fn run;
	uint32_t arg_count; /* 0: comando sem argumento; senão, valores válidos 0..n-1 */
};

static void cmd_off(struct leds_state *led, uint32_t arg, int64_t now)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(now);

	led->blinking = false;
	(void)gpio_pin_set_dt(&led->gpio, 0);
}

static void cmd_on(struct leds_state *led, uint32_t arg, int64_t now)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(now);

	led->blinking = false;
	(void)gpio_pin_set_dt(&led->gpio, 1);
}

static void cmd_toggle(struct leds_state *led, uint32_t arg, int64_t now)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(now);

	led->blinking = false;
	(void)gpio_pin_toggle_dt(&led->gpio);
}

static void cmd_blink(struct leds_state *led, uint32_t arg, int64_t now)
{
	led->pattern = (enum leds_pattern)arg;
	led->interval_ms = pattern_ms[arg];
	led->deadline_ms = now + led->interval_ms;
	led->blinking = true;
	(void)gpio_pin_set_dt(&led->gpio, 1); /* começa ligado */
}

static const struct leds_cmd_desc cmd_table[LEDS_CMD_COUNT] = {
	[LEDS_CMD_OFF]    = { .run = cmd_off,    .arg_count = 0 },
	[LEDS_CMD_ON]     = { .run = cmd_on,     .arg_count = 0 },
	[LEDS_CMD_TOGGLE] = { .run = cmd_toggle, .arg_count = 0 },
	[LEDS_CMD_BLINK]  = { .run = cmd_blink,  .arg_count = LEDS_PATTERN_COUNT },
};


size_t leds_count(void)
{
	return ARRAY_SIZE(leds);
}

int leds_init(void)
{
	int ret = 0;
	size_t failed = 0;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i].gpio)) {
			LOG_ERR("LED %zu: controlador de GPIO nao esta pronto", i);
			return -ENODEV;
		}
	}

	k_spinlock_key_t key = k_spin_lock(&leds_lock);

	initialized = false;
	k_timer_stop(&leds_timer);

	for (size_t i = 0; i < ARRAY_SIZE(leds) && ret == 0; i++) {
		leds[i].blinking = false;
		leds[i].pattern = LEDS_PATTERN_SLOW;
		leds[i].interval_ms = 0;
		leds[i].deadline_ms = 0;

		ret = gpio_pin_configure_dt(&leds[i].gpio, GPIO_OUTPUT_INACTIVE);
		failed = i;
	}

	initialized = (ret == 0);
	k_spin_unlock(&leds_lock, key);

	if (ret != 0) {
		LOG_ERR("LED %zu: falha ao configurar GPIO (%d)", failed, ret);
	}

	return ret;
}

int leds_apply(size_t led, enum leds_cmd cmd, uint32_t arg)
{
	if ((uint32_t)cmd >= LEDS_CMD_COUNT) {
		LOG_ERR("comando invalido: %d", (int)cmd);
		return -EINVAL;
	}

	const struct leds_cmd_desc *desc = &cmd_table[cmd];

	if (desc->arg_count != 0 && arg >= desc->arg_count) {
		LOG_ERR("argumento invalido: %u (comando %d)", (unsigned int)arg, (int)cmd);
		return -EINVAL;
	}

	if (led != LEDS_ALL && led >= ARRAY_SIZE(leds)) {
		LOG_ERR("LED invalido: %zu", led);
		return -EINVAL;
	}

	size_t first = (led == LEDS_ALL) ? 0 : led;
	size_t last = (led == LEDS_ALL) ? ARRAY_SIZE(leds) : led + 1;

	k_spinlock_key_t key = k_spin_lock(&leds_lock);

	if (!initialized) {
		k_spin_unlock(&leds_lock, key);
		LOG_ERR("modulo nao inicializado");
		return -ENODEV;
	}

	int64_t now = k_uptime_get();

	for (size_t i = first; i < last; i++) {
		desc->run(&leds[i], arg, now);
	}

	leds_reschedule(now);
	k_spin_unlock(&leds_lock, key);

	return 0;
}
