/*******************************************************************
 * @file board_io.c
 *
 * @brief LED e botao da placa, obtidos do devicetree.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/gpio.h>

#include "board_io.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Guarda o estado do LED, ja que ler um pino de saida depende do driver */
static bool led_is_on;

int board_io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	led_is_on = false;

	return 0;
}

int board_io_led_set(bool on)
{
	int ret = gpio_pin_set_dt(&led, on);

	if (ret < 0) {
		return ret;
	}

	led_is_on = on;

	return 0;
}

bool board_io_led_get(void)
{
	return led_is_on;
}

int board_io_button_get(void)
{
	return gpio_pin_get_dt(&button);
}
