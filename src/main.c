/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/kernel.h>

#include "led.h"

int main(void)
{
	for (int step = 0; step < 4; step++) {
		printk("LED: %s\n", led_state_at_step(step) ? "ON" : "OFF");
	}

	return 0;
}
