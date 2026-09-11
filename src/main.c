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
#include <zephyr/sys/util.h>

#include "debounce.h"

int main(void)
{
	bool samples[] = {false, true, false, true, true, true};

	for (int i = 0; i < ARRAY_SIZE(samples); i++) {
		printk("stable = %d\n", debounce_update(samples[i]));
	}

	return 0;
}
