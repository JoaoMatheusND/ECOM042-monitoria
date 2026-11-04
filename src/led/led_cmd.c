/**
 * @file led_cmd.c
 *
 * @brief LED command handlers and command table.
 */
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "led_cmd.h"

LOG_MODULE_REGISTER(led_cmd, LOG_LEVEL_DBG);

/* Full blink cycle time of each profile, in ms. */
static const uint32_t profile_period_ms[LED_PROFILE_COUNT] = {
	[LED_PROFILE_SLOW] = CONFIG_LED_PERIOD_SLOW_MS,
	[LED_PROFILE_NORMAL] = CONFIG_LED_PERIOD_NORMAL_MS,
	[LED_PROFILE_FAST] = CONFIG_LED_PERIOD_FAST_MS,
};

static int cmd_on(struct led_state *led, int arg)
{
	ARG_UNUSED(arg);

	led->blinking = false;

	return gpio_pin_set_dt(&led->gpio, 1);
}

static int cmd_off(struct led_state *led, int arg)
{
	ARG_UNUSED(arg);

	led->blinking = false;

	return gpio_pin_set_dt(&led->gpio, 0);
}

static int cmd_toggle(struct led_state *led, int arg)
{
	ARG_UNUSED(arg);

	led->blinking = false;

	return gpio_pin_toggle_dt(&led->gpio);
}

static int cmd_blink(struct led_state *led, int arg)
{
	if (arg < 0 || arg >= LED_PROFILE_COUNT) {
		LOG_ERR("Invalid blink profile: %d", arg);
		return -EINVAL;
	}

	led->profile = (enum led_profile)arg;
	led->period_ms = profile_period_ms[arg];
	led->deadline_ms = k_uptime_get() + led_half_period_ms(led);
	led->blinking = true;

	/* Blinking always starts with the LED on. */
	return gpio_pin_set_dt(&led->gpio, 1);
}

static const struct led_cmd cmd_table[LED_ACT_COUNT] = {
	[LED_ACT_ON] = {.handler = cmd_on},
	[LED_ACT_OFF] = {.handler = cmd_off},
	[LED_ACT_TOGGLE] = {.handler = cmd_toggle},
	[LED_ACT_BLINK] = {.handler = cmd_blink},
};

const struct led_cmd *led_cmd_table_get(void)
{
	return cmd_table;
}
