/*******************************************************************
 * @file board_io.c
 *
 * @brief LED e botao obtidos pelo devicetree (led0 / sw0).
 *******************************************************************/

#include "board_io.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static bool led_on;

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

	led_on = false;

	return gpio_pin_configure_dt(&btn, GPIO_INPUT);
}

int board_io_led_set(bool on)
{
	int ret = gpio_pin_set_dt(&led, on);

	if (ret == 0) {
		led_on = on;
	}

	return ret;
}

int board_io_led_get(void)
{
	return led_on;
}

int board_io_btn_get(void)
{
	return gpio_pin_get_dt(&btn);
}
