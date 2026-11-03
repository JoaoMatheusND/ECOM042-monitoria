/*******************************************************************
 * @file led.c
 *
 * @brief Driver de LEDs independentes com um unico k_timer one-shot.
 *
 * O timer e rearmado dinamicamente para o menor deadline pendente; o
 * callback roda em contexto de ISR e so mexe em GPIO, deadlines e rearme.
 * O estado e protegido por k_spinlock porque a thread e a ISR escrevem nele.
 *******************************************************************/

#include "led.h"

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_GPIO_EMUL)
#include <zephyr/drivers/gpio/gpio_emul.h>
#endif

LOG_MODULE_REGISTER(app_led, CONFIG_LOG_DEFAULT_LEVEL);

/* O primeiro LED ancora o no do devicetree; os demais sao filhos dele.
 * LED novo no devicetree entra sozinho na tabela, sem funcao nova.
 */
#define LEDS_NODE DT_PARENT(DT_ALIAS(led0))

/** @brief Estado independente de cada LED. */
struct led_state {
	struct gpio_dt_spec spec;
	enum led_pattern pattern;
	uint32_t interval_ms;
	int64_t deadline_ms;
	bool blinking;
	bool level;
};

#define LED_DEFINE(node) {.spec = GPIO_DT_SPEC_GET(node, gpios)},

static struct led_state leds[] = {DT_FOREACH_CHILD_STATUS_OKAY(LEDS_NODE, LED_DEFINE)};

/** @brief Periodo (ciclo completo) de cada padrao, vindo do Kconfig. */
static const uint32_t pattern_period_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_LED_PERIOD_SLOW_MS,
	[LED_PATTERN_NORMAL] = CONFIG_LED_PERIOD_NORMAL_MS,
	[LED_PATTERN_FAST] = CONFIG_LED_PERIOD_FAST_MS,
};

/** @brief Operacao de um LED (tabela de comandos). */
struct led_action_ops {
	int (*apply)(struct led_state *led, enum led_pattern pattern);
};

static struct k_timer led_timer;
static struct k_spinlock lock;
static bool timer_ready;

static int action_on(struct led_state *led, enum led_pattern pattern)
{
	ARG_UNUSED(pattern);

	led->blinking = false;
	led->level = true;

	return gpio_pin_set_dt(&led->spec, 1);
}

static int action_off(struct led_state *led, enum led_pattern pattern)
{
	ARG_UNUSED(pattern);

	led->blinking = false;
	led->level = false;

	return gpio_pin_set_dt(&led->spec, 0);
}

static int action_toggle(struct led_state *led, enum led_pattern pattern)
{
	ARG_UNUSED(pattern);

	led->blinking = false;
	led->level = !led->level;

	return gpio_pin_set_dt(&led->spec, led->level ? 1 : 0);
}

static int action_blink(struct led_state *led, enum led_pattern pattern)
{
	led->pattern = pattern;
	led->interval_ms = pattern_period_ms[pattern] / 2U;
	if (led->interval_ms == 0U) {
		led->interval_ms = 1U;
	}

	led->level = true;
	led->blinking = true;
	led->deadline_ms = k_uptime_get() + led->interval_ms;

	return gpio_pin_set_dt(&led->spec, 1);
}

static const struct led_action_ops action_ops[LED_ACTION_COUNT] = {
	[LED_ACTION_ON] = {.apply = action_on},
	[LED_ACTION_OFF] = {.apply = action_off},
	[LED_ACTION_TOGGLE] = {.apply = action_toggle},
	[LED_ACTION_BLINK] = {.apply = action_blink},
};

/* Rearma o timer para o menor prazo. Chamar com o lock na mao. */
static void rearm_timer(void)
{
	int64_t now = k_uptime_get();
	int64_t next = INT64_MAX;
	size_t i;

	for (i = 0U; i < ARRAY_SIZE(leds); i++) {
		if (leds[i].blinking && (leds[i].deadline_ms < next)) {
			next = leds[i].deadline_ms;
		}
	}

	if (next == INT64_MAX) {
		return;
	}

	k_timer_start(&led_timer, K_MSEC(MAX(next - now, 1)), K_NO_WAIT);
}

/* Contexto de ISR: sem log, sem alocacao, sem bloqueio. */
static void led_timer_expiry(struct k_timer *timer)
{
	k_spinlock_key_t key;
	int64_t now;
	size_t i;

	ARG_UNUSED(timer);

	key = k_spin_lock(&lock);
	now = k_uptime_get();

	for (i = 0U; i < ARRAY_SIZE(leds); i++) {
		struct led_state *led = &leds[i];

		if (!led->blinking || (now < led->deadline_ms)) {
			continue;
		}

		led->level = !led->level;
		(void)gpio_pin_set_dt(&led->spec, led->level ? 1 : 0);
		led->deadline_ms += led->interval_ms;

		/* Se o sistema atrasou, evita acumular deadlines no passado. */
		if (led->deadline_ms <= now) {
			led->deadline_ms = now + led->interval_ms;
		}
	}

	rearm_timer();
	k_spin_unlock(&lock, key);
}

size_t led_count(void)
{
	return ARRAY_SIZE(leds);
}

int led_init(void)
{
	k_spinlock_key_t key;
	size_t i;

	if (!timer_ready) {
		k_timer_init(&led_timer, led_timer_expiry, NULL);
		timer_ready = true;
	}

	for (i = 0U; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i].spec)) {
			LOG_ERR("LED %u: GPIO controller not ready", (unsigned int)i);
			return -ENODEV;
		}
	}

	key = k_spin_lock(&lock);
	for (i = 0U; i < ARRAY_SIZE(leds); i++) {
		leds[i].pattern = LED_PATTERN_SLOW;
		leds[i].interval_ms = 0U;
		leds[i].deadline_ms = 0;
		leds[i].blinking = false;
		leds[i].level = false;
	}
	k_spin_unlock(&lock, key);

	for (i = 0U; i < ARRAY_SIZE(leds); i++) {
		int ret = gpio_pin_configure_dt(&leds[i].spec, GPIO_OUTPUT_INACTIVE);

		if (ret != 0) {
			LOG_ERR("LED %u: pin configuration failed (%d)", (unsigned int)i, ret);
			return ret;
		}
	}

	return 0;
}

int led_apply(int led, enum led_action action, enum led_pattern pattern)
{
	k_spinlock_key_t key;
	size_t first;
	size_t last;
	size_t i;
	int ret = 0;

	if ((action < 0) || (action >= LED_ACTION_COUNT)) {
		LOG_ERR("invalid action (%d)", (int)action);
		return -EINVAL;
	}

	if ((action == LED_ACTION_BLINK) && ((pattern < 0) || (pattern >= LED_PATTERN_COUNT))) {
		LOG_ERR("invalid blink pattern (%d)", (int)pattern);
		return -EINVAL;
	}

	if (led == LED_ALL) {
		first = 0U;
		last = ARRAY_SIZE(leds);
	} else if ((led < 0) || ((size_t)led >= ARRAY_SIZE(leds))) {
		LOG_ERR("invalid LED (%d), only %u available", led, (unsigned int)ARRAY_SIZE(leds));
		return -EINVAL;
	} else {
		first = (size_t)led;
		last = first + 1U;
	}

	if (!timer_ready) {
		LOG_ERR("call led_init() before using the driver");
		return -ENODEV;
	}

	key = k_spin_lock(&lock);
	for (i = first; i < last; i++) {
		int err = action_ops[action].apply(&leds[i], pattern);

		if ((err < 0) && (ret == 0)) {
			ret = err;
		}
	}
	rearm_timer();
	k_spin_unlock(&lock, key);

	if (ret < 0) {
		LOG_ERR("GPIO failure (%d)", ret);
	}

	return ret;
}

int led_on(int led)
{
	return led_apply(led, LED_ACTION_ON, LED_PATTERN_SLOW);
}

int led_off(int led)
{
	return led_apply(led, LED_ACTION_OFF, LED_PATTERN_SLOW);
}

int led_toggle(int led)
{
	return led_apply(led, LED_ACTION_TOGGLE, LED_PATTERN_SLOW);
}

int led_blink(int led, enum led_pattern pattern)
{
	return led_apply(led, LED_ACTION_BLINK, pattern);
}

int led_level_get(int led, bool *on)
{
	k_spinlock_key_t key;
	int level;

	if ((on == NULL) || (led < 0) || ((size_t)led >= ARRAY_SIZE(leds))) {
		return -EINVAL;
	}

	key = k_spin_lock(&lock);
#if defined(CONFIG_GPIO_EMUL)
	level = gpio_emul_output_get_dt(&leds[led].spec);
#else
	level = gpio_pin_get_dt(&leds[led].spec);
#endif
	k_spin_unlock(&lock, key);

	if (level < 0) {
		return level;
	}

	*on = (level != 0);

	return 0;
}
