#include "io.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static bool led_state;

int io_init(void)
{
	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&btn)) {
		return -ENODEV;
	}

	int err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		return err;
	}

	err = gpio_pin_configure_dt(&btn, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	return 0;
}

void io_led_set(bool on)
{
	led_state = on;
	gpio_pin_set_dt(&led, on);
}

bool io_led_get(void)
{
	return led_state;
}

int io_btn_get(void)
{
	return gpio_pin_get_dt(&btn);
}

int64_t io_uptime_get(void)
{
	return k_uptime_get();
}
