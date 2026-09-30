/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include "commands.h"
#include "invoker.h"
#include <zephyr/kernel.h>

int main(void)
{
	invoker_dispatch(&cmd_uart);
	invoker_dispatch(&cmd_ble);
	invoker_dispatch(&cmd_button);
	invoker_dispatch(&cmd_timer);

	return 0;
}
