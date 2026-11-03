/*******************************************************************
 * @file led_ctrl.c
 *
 * @brief LEDs com pisca independente multiplexados em um único k_timer.
 *
 * Cada LED guarda a sua própria deadline (próxima troca, em ms de
 * uptime). O timer é one-shot e sempre rearmado para a menor deadline
 * entre todos os LEDs. LED parado usa DEADLINE_NEVER, então o callback
 * não precisa saber "em que estado" cada LED está.
 *******************************************************************/

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include "led_ctrl.h"

LOG_MODULE_REGISTER(led_ctrl, CONFIG_LED_CTRL_LOG_LEVEL);

/* LEDs vindos dos aliases led0, led1, ... que existirem no devicetree. */
#define LED_ALIAS(i) DT_ALIAS(UTIL_CAT(led, i))
#define LED_SPEC(i, _)                                                                             \
	COND_CODE_1(DT_NODE_EXISTS(LED_ALIAS(i)), (GPIO_DT_SPEC_GET(LED_ALIAS(i), gpios), ), ())

static const struct gpio_dt_spec led_specs[] = {LISTIFY(CONFIG_LED_CTRL_MAX_ALIAS, LED_SPEC, ())};

#define LED_COUNT ARRAY_SIZE(led_specs)
BUILD_ASSERT(LED_COUNT > 0, "Nenhum alias ledN no devicetree");

#define DEADLINE_NEVER INT64_MAX

/* Estado de cada LED, indexado pelo número do LED. */
struct led_state {
	enum led_pattern pattern;
	uint32_t half_period_ms; /* tempo entre duas trocas de nível */
	int64_t deadline;        /* uptime (ms) da próxima troca */
	bool blinking;
	bool level;
};

typedef int (*led_action_fn)(size_t idx, enum led_pattern pattern, int64_t now);

static void blink_timer_expired(struct k_timer *timer);

/* O único timer do projeto. */
K_TIMER_DEFINE(blink_timer, blink_timer_expired, NULL);

static struct led_state leds[LED_COUNT];
static struct k_spinlock lock;

static const uint32_t pattern_period_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_LED_CTRL_PERIOD_SLOW_MS,
	[LED_PATTERN_NORMAL] = CONFIG_LED_CTRL_PERIOD_NORMAL_MS,
	[LED_PATTERN_FAST] = CONFIG_LED_CTRL_PERIOD_FAST_MS,
};

static int write_level(size_t idx, bool on)
{
	leds[idx].level = on;
	return gpio_pin_set_dt(&led_specs[idx], on);
}

static void stop_blink(struct led_state *s)
{
	s->blinking = false;
	s->deadline = DEADLINE_NEVER;
}

/* Rearma o timer para a deadline mais próxima. Chamar com o lock. */
static void reschedule(int64_t now)
{
	int64_t next = DEADLINE_NEVER;

	for (size_t i = 0; i < LED_COUNT; i++) {
		next = MIN(next, leds[i].deadline);
	}

	if (next == DEADLINE_NEVER) {
		k_timer_stop(&blink_timer);
		return;
	}

	k_timer_start(&blink_timer, K_MSEC(MAX(next - now, 0)), K_NO_WAIT);
}

/* Contexto de ISR: só GPIO, deadlines e rearme. */
static void blink_timer_expired(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_get();

	ARG_UNUSED(timer);

	for (size_t i = 0; i < LED_COUNT; i++) {
		struct led_state *s = &leds[i];

		/* LED parado tem DEADLINE_NEVER e nunca entra aqui. */
		while (s->deadline <= now) {
			s->deadline += s->half_period_ms;
			(void)write_level(i, !s->level);
		}
	}

	reschedule(now);
	k_spin_unlock(&lock, key);
}

/* --- Comandos (uma função por ação, escolhida pela tabela) --- */

static int act_off(size_t idx, enum led_pattern pattern, int64_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	stop_blink(&leds[idx]);
	return write_level(idx, false);
}

static int act_on(size_t idx, enum led_pattern pattern, int64_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	stop_blink(&leds[idx]);
	return write_level(idx, true);
}

static int act_toggle(size_t idx, enum led_pattern pattern, int64_t now)
{
	ARG_UNUSED(pattern);
	ARG_UNUSED(now);
	stop_blink(&leds[idx]);
	return write_level(idx, !leds[idx].level);
}

static int act_blink(size_t idx, enum led_pattern pattern, int64_t now)
{
	struct led_state *s = &leds[idx];

	s->pattern = pattern;
	s->half_period_ms = MAX(pattern_period_ms[pattern] / 2U, 1U);
	s->deadline = now + s->half_period_ms;
	s->blinking = true;
	return write_level(idx, true);
}

static const led_action_fn action_table[LED_ACTION_COUNT] = {
	[LED_ACTION_OFF] = act_off,
	[LED_ACTION_ON] = act_on,
	[LED_ACTION_TOGGLE] = act_toggle,
	[LED_ACTION_BLINK] = act_blink,
};

static bool led_is_valid(int led)
{
	return led == LED_CTRL_ALL || (led >= 0 && led < (int)LED_COUNT);
}

/* Único lugar que trata "um LED" x "todos os LEDs". */
static int for_each_target(int led, led_action_fn action, enum led_pattern pattern)
{
	size_t first = (led == LED_CTRL_ALL) ? 0 : (size_t)led;
	size_t end = (led == LED_CTRL_ALL) ? LED_COUNT : (size_t)led + 1;
	int64_t now = k_uptime_get();
	int err = 0;
	k_spinlock_key_t key = k_spin_lock(&lock);

	for (size_t i = first; i < end; i++) {
		int ret = action(i, pattern, now);

		err = (err != 0) ? err : ret;
	}

	reschedule(now);
	k_spin_unlock(&lock, key);

	return err;
}

int led_ctrl_init(void)
{
	k_spinlock_key_t key;
	int err = 0;

	for (size_t i = 0; i < LED_COUNT; i++) {
		if (!gpio_is_ready_dt(&led_specs[i])) {
			LOG_ERR("Controlador do LED %u (%s) nao esta pronto", (unsigned int)i,
				led_specs[i].port->name);
			return -ENODEV;
		}
	}

	/* Para o timer e zera os estados: seguro chamar de novo. */
	key = k_spin_lock(&lock);
	k_timer_stop(&blink_timer);
	for (size_t i = 0; i < LED_COUNT; i++) {
		stop_blink(&leds[i]);
		leds[i].pattern = LED_PATTERN_NORMAL;
		leds[i].level = false;
	}
	k_spin_unlock(&lock, key);

	for (size_t i = 0; i < LED_COUNT; i++) {
		int ret = gpio_pin_configure_dt(&led_specs[i], GPIO_OUTPUT_INACTIVE);

		err = (err != 0) ? err : ret;
	}

	return err;
}

int led_ctrl_count(void)
{
	return (int)LED_COUNT;
}

int led_ctrl_apply(int led, enum led_action action, enum led_pattern pattern)
{
	if (!led_is_valid(led)) {
		LOG_ERR("LED invalido: %d (validos: 0..%d ou LED_CTRL_ALL)", led,
			(int)LED_COUNT - 1);
		return -EINVAL;
	}

	if ((unsigned int)action >= LED_ACTION_COUNT) {
		LOG_ERR("Acao invalida: %d", (int)action);
		return -EINVAL;
	}

	if ((unsigned int)pattern >= LED_PATTERN_COUNT) {
		LOG_ERR("Padrao invalido: %d", (int)pattern);
		return -EINVAL;
	}

	return for_each_target(led, action_table[action], pattern);
}

int led_ctrl_on(int led)
{
	return led_ctrl_apply(led, LED_ACTION_ON, LED_PATTERN_NORMAL);
}

int led_ctrl_off(int led)
{
	return led_ctrl_apply(led, LED_ACTION_OFF, LED_PATTERN_NORMAL);
}

int led_ctrl_toggle(int led)
{
	return led_ctrl_apply(led, LED_ACTION_TOGGLE, LED_PATTERN_NORMAL);
}

int led_ctrl_blink(int led, enum led_pattern pattern)
{
	return led_ctrl_apply(led, LED_ACTION_BLINK, pattern);
}

int led_ctrl_get(int led)
{
	if (led < 0 || led >= (int)LED_COUNT) {
		LOG_ERR("LED invalido: %d", led);
		return -EINVAL;
	}

	return leds[led].level ? 1 : 0;
}
