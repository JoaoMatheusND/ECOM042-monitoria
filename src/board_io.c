#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "board_io.h"

#if !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(led0)) || !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(sw0))
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static bool led_state;

static int op_led_set(void *user, bool on)
{
	int ret = gpio_pin_set_dt(&led, on);

	ARG_UNUSED(user);
	if (ret == 0) {
		led_state = on;
	}
	return ret;
}

static int op_led_toggle(void *user)
{
	return op_led_set(user, !led_state);
}

static int op_led_get(void *user)
{
	ARG_UNUSED(user);
	return led_state ? 1 : 0;
}

static int op_button_get(void *user)
{
	ARG_UNUSED(user);
	return gpio_pin_get_dt(&btn);
}

static int64_t op_uptime_ms(void *user)
{
	ARG_UNUSED(user);
	return k_uptime_get();
}

static const struct shell_hw_ops ops = {
	.led_set = op_led_set,
	.led_toggle = op_led_toggle,
	.led_get = op_led_get,
	.button_get = op_button_get,
	.uptime_ms = op_uptime_ms,
	.user = NULL,
};

int board_io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&btn)) {
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}
	led_state = false;
	return gpio_pin_configure_dt(&btn, GPIO_INPUT);
}

const struct shell_hw_ops *board_io_ops(void)
{
	return &ops;
}
