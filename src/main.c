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

#include "field.h"

int main(void)
{
	uint32_t reg = 0x000000B4;

	printk("field = %u\n", reg_get_field(reg, 2, 3));

	return 0;
}
