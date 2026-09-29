/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

// #include <zephyr/kernel.h>
#include "command.h"
#include <stdint.h>

int main(void)
{
	printk("[LOG] Starting to handle commands...");

	dispatch(UART_CMD);
	dispatch(BLE_CMD);
	dispatch(BUTTON_CMD);
	dispatch(TIMER_CMD);

	/*Constante existe mas não existe valor correspondente na tabela*/
	dispatch(ERROR);

	printk("[LOG] Finished handling");

	return 0;
}
