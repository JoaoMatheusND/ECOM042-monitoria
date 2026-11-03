/*******************************************************************
 * @file board_io.c
 *
 * @brief LED (led0) e botão (sw0) obtidos pelo devicetree.
 *******************************************************************/

#include <zephyr/drivers/gpio.h>

#include "board_io.h"

#if !DT_NODE_EXISTS(DT_ALIAS(sw0))
#error "Alias sw0 nao existe: descreva o botao no app.overlay."
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Último valor escrito no LED (saída não é relida do hardware). */
static bool led_state;

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

	led_state = false;

	return gpio_pin_configure_dt(&button, GPIO_INPUT);
}

int led_set(bool on)
{
	int ret = gpio_pin_set_dt(&led, on);

	if (ret == 0) {
		led_state = on;
	}

	return ret;
}

int led_get(void)
{
	return led_state ? 1 : 0;
}

int button_read(void)
{
	return gpio_pin_get_dt(&button);
}
