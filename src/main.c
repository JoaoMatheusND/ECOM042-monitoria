/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 19/10/2026
 *******************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "commands/cmd.h"
#include "commands/commands.h"

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

int main(void)
{
	LOG_INF("Hello World! %s", CONFIG_BOARD);

	return 0;
}
