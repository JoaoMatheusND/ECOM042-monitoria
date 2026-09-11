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

#include "command.h"

int main(void)
{
	printk("parse_command(ON) = %d\n", parse_command("ON"));
	printk("parse_command(OFF) = %d\n", parse_command("OFF"));
	printk("parse_command(FOO) = %d\n", parse_command("FOO"));

	return 0;
}
