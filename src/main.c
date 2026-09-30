/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include "board_io.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	const int sequence[] = {0, 1, 0, 1};
	int ret;
	int state;
	int led_state;

	ret = io_init();
	if (ret < 0) {
		printk("Erro ao configurar GPIO: %d\n", ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sequence); i++) {
		/* Simula o botao antes de ler pela API de GPIO. */
		ret = gpio_emul_input_set(button.port, button.pin, sequence[i]);
		if (ret < 0) {
			printk("Erro ao simular botao: %d\n", ret);
			return ret;
		}

		state = button_read();
		if (state < 0) {
			printk("Erro ao ler botao: %d\n", state);
			return state;
		}

		ret = led_set(state != 0);
		if (ret < 0) {
			printk("Erro ao escrever LED: %d\n", ret);
			return ret;
		}

		/* Confere o nivel que foi escrito na saida emulada. */
		led_state = gpio_emul_output_get(led.port, led.pin);
		if (led_state < 0) {
			printk("Erro ao conferir LED: %d\n", led_state);
			return led_state;
		}

		printk("Button: %d -> LED: %d\n", state, led_state);
	}

	return 0;
}
