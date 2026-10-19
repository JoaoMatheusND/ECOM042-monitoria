#include "board_io.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static bool led_on;

int io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}
	led_on = false;
	return gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_DOWN);
}

int led_set(bool on)
{
	int ret = gpio_pin_set_dt(&led, on);

	if (ret == 0) {
		led_on = on;
	}
	return ret;
}

int led_get(void)
{
	return led_on;
}

int button_read(void)
{
	return gpio_pin_get_dt(&button);
}