#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include "board_io.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int io_init(void)
{
	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	int r = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (r) {
		return r;
	}

	return gpio_pin_configure_dt(&button, GPIO_INPUT);
}

int led_set(bool on)
{
	return gpio_pin_set_dt(&led, on);
}

int button_read(void)
{
	return gpio_pin_get_dt(&button);
}
