/*******************************************************************
 * @file board_io.c
 *
 * @brief LED e botao obtidos do devicetree.
 *******************************************************************/

#include "board_io.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static bool led_state;

int board_io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	led_state = false;

	return 0;
}

int board_led_set(bool on)
{
	int ret = gpio_pin_set_dt(&led, on ? 1 : 0);

	if (ret == 0) {
		led_state = on;
	}

	return ret;
}

int board_led_get(bool *on)
{
	if (on == NULL) {
		return -EINVAL;
	}

	*on = led_state;

	return 0;
}

int board_button_read(int *pressed)
{
	int value;

	if (pressed == NULL) {
		return -EINVAL;
	}

	value = gpio_pin_get_dt(&button);
	if (value < 0) {
		return value;
	}

	*pressed = value;

	return 0;
}
