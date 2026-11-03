/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file led.c
 *
 * @brief Um k_timer one-shot multiplexado entre os LEDs.
 *
 * Cada LED piscando guarda o proprio prazo (deadline) da proxima
 * troca. O timer e sempre armado para o prazo mais proximo; quando
 * dispara, troca os LEDs vencidos, avanca o prazo deles e se rearma.
 * Mexer num LED so muda o prazo dele, entao os outros mantem fase e
 * periodo.
 *******************************************************************/

#include "led.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(app_led, CONFIG_APP_LED_LOG_LEVEL);

#define LED_ALIAS(i) DT_ALIAS(UTIL_CAT(led, i))

#define LED_SPEC_IF_EXISTS(i, _)                                                                   \
	COND_CODE_1(DT_NODE_EXISTS(LED_ALIAS(i)), (GPIO_DT_SPEC_GET(LED_ALIAS(i), gpios), ), ())

static const struct gpio_dt_spec leds[] = {
	LISTIFY(CONFIG_APP_LED_MAX_ALIASES, LED_SPEC_IF_EXISTS, ())};

BUILD_ASSERT(ARRAY_SIZE(leds) > 0, "Nenhum alias led0, led1, ... no devicetree.");

struct led_state {
	enum led_pattern pattern;
	uint32_t period_ms;
	k_ticks_t half_period;
	int64_t deadline;
	bool blinking;
	bool on;
};

typedef int (*led_action_fn)(size_t idx, int64_t now, int arg);

struct led_command {
	const char *name;
	led_action_fn run;
	/* arg aceito: 0 <= arg < arg_limit. */
	int arg_limit;
};

static const uint32_t pattern_period_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_APP_LED_PERIOD_SLOW_MS,
	[LED_PATTERN_NORMAL] = CONFIG_APP_LED_PERIOD_NORMAL_MS,
	[LED_PATTERN_FAST] = CONFIG_APP_LED_PERIOD_FAST_MS,
};

/* O callback do timer roda em contexto de interrupcao e le o mesmo
 * estado que as funcoes publicas escrevem. */
static struct k_spinlock lock;
static struct led_state states[ARRAY_SIZE(leds)];
static bool initialized;

static void led_timer_expiry(struct k_timer *timer);

/* Definido estaticamente: led_init() nunca reinicializa o timer. */
K_TIMER_DEFINE(led_timer, led_timer_expiry, NULL);

static int write_level(size_t idx, bool on)
{
	int ret = gpio_pin_set_dt(&leds[idx], on ? 1 : 0);

	if (ret == 0) {
		states[idx].on = on;
	}
	return ret;
}

/* Chamado com o lock. Se nada pisca, nao rearma: um disparo pendente
 * acha tudo parado e morre sozinho. Timeout absoluto: o relativo
 * arredonda para cima e dispara um tick depois do prazo. */
static void rearm_locked(void)
{
	int64_t next = INT64_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (states[i].blinking) {
			next = MIN(next, states[i].deadline);
		}
	}

	if (next != INT64_MAX) {
		k_timer_start(&led_timer, K_TIMEOUT_ABS_TICKS(next), K_NO_WAIT);
	}
}

/* Contexto de ISR: so GPIO, prazos e rearme. Sem log aqui. */
static void led_timer_expiry(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_ticks();

	ARG_UNUSED(timer);

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		struct led_state *s = &states[i];

		if (!s->blinking || s->deadline > now) {
			continue;
		}

		(void)write_level(i, !s->on);
		s->deadline += s->half_period;
		/* Atraso maior que meio periodo: retoma do agora em vez de
		 * soltar varias trocas seguidas para alcancar o prazo. */
		if (s->deadline <= now) {
			s->deadline = now + s->half_period;
		}
	}

	rearm_locked();
	k_spin_unlock(&lock, key);
}

static int stop_and_write(size_t idx, bool on)
{
	states[idx].blinking = false;
	return write_level(idx, on);
}

static int act_on(size_t idx, int64_t now, int arg)
{
	ARG_UNUSED(now);
	ARG_UNUSED(arg);
	return stop_and_write(idx, true);
}

static int act_off(size_t idx, int64_t now, int arg)
{
	ARG_UNUSED(now);
	ARG_UNUSED(arg);
	return stop_and_write(idx, false);
}

static int act_toggle(size_t idx, int64_t now, int arg)
{
	ARG_UNUSED(now);
	ARG_UNUSED(arg);
	return stop_and_write(idx, !states[idx].on);
}

static int act_blink(size_t idx, int64_t now, int arg)
{
	struct led_state *s = &states[idx];
	int ret = write_level(idx, true);

	if (ret < 0) {
		return ret;
	}

	s->pattern = (enum led_pattern)arg;
	s->period_ms = pattern_period_ms[arg];
	/* Um periodo = meio ligado + meio desligado. */
	s->half_period = (k_ticks_t)k_ms_to_ticks_ceil64(s->period_ms / 2U);
	s->deadline = now + s->half_period;
	s->blinking = true;
	return 0;
}

enum led_command_id {
	LED_CMD_ON,
	LED_CMD_OFF,
	LED_CMD_TOGGLE,
	LED_CMD_BLINK,
};

static const struct led_command commands[] = {
	[LED_CMD_ON] = {"led_on", act_on, 1},
	[LED_CMD_OFF] = {"led_off", act_off, 1},
	[LED_CMD_TOGGLE] = {"led_toggle", act_toggle, 1},
	[LED_CMD_BLINK] = {"led_blink", act_blink, LED_PATTERN_COUNT},
};

/* "Um ou todos": o unico lugar que traduz LED_ALL num intervalo. */
static bool led_targets(int led, size_t *first, size_t *last)
{
	if (led == LED_ALL) {
		*first = 0;
		*last = ARRAY_SIZE(leds);
		return true;
	}
	if (led < 0 || (size_t)led >= ARRAY_SIZE(leds)) {
		return false;
	}
	*first = (size_t)led;
	*last = (size_t)led + 1;
	return true;
}

static int led_execute(enum led_command_id id, int led, int arg)
{
	const struct led_command *cmd = &commands[id];
	k_spinlock_key_t key;
	size_t first;
	size_t last;
	int64_t now;
	int ret = 0;

	if (!led_targets(led, &first, &last)) {
		LOG_ERR("%s: LED %d invalido (ha %u)", cmd->name, led,
			(unsigned int)ARRAY_SIZE(leds));
		return -EINVAL;
	}
	if (arg < 0 || arg >= cmd->arg_limit) {
		LOG_ERR("%s: argumento %d invalido", cmd->name, arg);
		return -EINVAL;
	}
	if (!initialized) {
		LOG_ERR("%s: chame led_init() antes", cmd->name);
		return -ENODEV;
	}

	key = k_spin_lock(&lock);
	now = k_uptime_ticks();
	for (size_t i = first; i < last; i++) {
		int err = cmd->run(i, now, arg);

		if (err < 0 && ret == 0) {
			ret = err;
		}
	}
	rearm_locked();
	k_spin_unlock(&lock, key);

	if (ret < 0) {
		LOG_ERR("%s: falha no GPIO (%d)", cmd->name, ret);
	}
	return ret;
}

int led_init(void)
{
	k_spinlock_key_t key;

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			LOG_ERR("LED %u: controlador %s nao esta pronto", (unsigned int)i,
				leds[i].port->name);
			return -ENODEV;
		}
	}

	key = k_spin_lock(&lock);
	initialized = false;
	memset(states, 0, sizeof(states));
	k_spin_unlock(&lock, key);

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			LOG_ERR("LED %u: configuracao falhou (%d)", (unsigned int)i, ret);
			return ret;
		}
	}

	initialized = true;
	return 0;
}

size_t led_count(void)
{
	return ARRAY_SIZE(leds);
}

int led_on(int led)
{
	return led_execute(LED_CMD_ON, led, 0);
}

int led_off(int led)
{
	return led_execute(LED_CMD_OFF, led, 0);
}

int led_toggle(int led)
{
	return led_execute(LED_CMD_TOGGLE, led, 0);
}

int led_blink(int led, enum led_pattern pattern)
{
	return led_execute(LED_CMD_BLINK, led, (int)pattern);
}

int led_is_on(int led)
{
	if (led < 0 || (size_t)led >= ARRAY_SIZE(leds)) {
		LOG_ERR("led_is_on: LED %d invalido", led);
		return -EINVAL;
	}
	return states[led].on ? 1 : 0;
}
