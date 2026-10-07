/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

#include "board_io.h"

/* Usado só pra simular o "mundo externo" (apertar o botão) no emulador. */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int main(void)
{
	static const int sequence[] = {0, 1, 0, 1};
	int ret = io_init();

	if (ret < 0) {
		printk("Erro em io_init: %d\n", ret);
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sequence); i++) {
		gpio_emul_input_set(button.port, button.pin, sequence[i]);

		int state = button_read();

		if (state < 0) {
			printk("Erro lendo botao: %d\n", state);
			return state;
		}

		ret = led_set(state);
		if (ret < 0) {
			printk("Erro escrevendo LED: %d\n", ret);
			return ret;
		}

		printk("Button: %d -> LED: %d\n", state, state);
	}

	return 0;
}
