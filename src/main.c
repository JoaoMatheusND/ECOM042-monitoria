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

#include "tasks.h"

int main(void)
{
	for (int tick = 0; tick < 6; tick++) {
		if (task_a_should_run(tick)) {
			printk("tick %d: A\n", tick);
		}
		if (task_b_should_run(tick)) {
			printk("tick %d: B\n", tick);
		}
	}

	return 0;
}
