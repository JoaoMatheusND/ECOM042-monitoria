/*******************************************************************
 * @file main.c
 *
 * @brief Main file (Atividade-03).
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

#include "board_io.h"

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int main(void)
{
	static const int sequence[] = {0, 1, 0, 1};
	int ret;

	ret = io_init();
	if (ret < 0) {
		printk("Erro: %d\n", ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sequence); i++) {
		int button_state;
		int led_state;

		ret = gpio_emul_input_set_dt(&button, sequence[i]);
		if (ret < 0) {
			return ret;
		}

		button_state = button_read();
		if (button_state < 0) {
			return button_state;
		}

		led_state = button_state;
		ret = led_set(led_state != 0);
		if (ret < 0) {
			return ret;
		}

		printk("Button: %d -> LED: %d\n", button_state, led_state);
	}
	return 0;
}