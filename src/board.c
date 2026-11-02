/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file board.c
 *
 * @brief GPIO do LED e do botao. O parser nunca inclui este arquivo:
 * o main entrega board_led_set, board_button_get etc. como ponteiros.
 *******************************************************************/

#include "board.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#if !DT_NODE_EXISTS(DT_ALIAS(led0)) || !DT_NODE_EXISTS(DT_ALIAS(sw0))
#error "Faltam os aliases led0 e sw0 no devicetree."
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Nem todo pino de saida devolve o nivel quando e lido. Guardamos o
 * ultimo valor pedido, que e o que o comando LED precisa responder. */
static bool led_on;

int board_init(void)
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

	return gpio_pin_configure_dt(&button, GPIO_INPUT);
}

int board_led_set(bool on)
{
	int ret = gpio_pin_set_dt(&led, on ? 1 : 0);

	if (ret < 0) {
		return ret;
	}
	led_on = on;
	return 0;
}

int board_led_get(bool *on)
{
	if (on == NULL) {
		return -EINVAL;
	}
	*on = led_on;
	return 0;
}

int board_button_get(int *level)
{
	int value;

	if (level == NULL) {
		return -EINVAL;
	}

	value = gpio_pin_get_dt(&button);
	if (value < 0) {
		return value;
	}
	*level = value;
	return 0;
}

int64_t board_uptime_ms(void)
{
	return k_uptime_get();
}
