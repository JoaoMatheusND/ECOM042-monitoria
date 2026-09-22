/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author coquinha0 (rapidoesso@gmail.com)
 * @version 0.2
 * @date 22/09/2026
 *******************************************************************/

#include <zephyr/kernel.h>

#include "commands.h"

int main(void)
{
	/* Simula comandos chegando de fontes diferentes (UART, BLE,
	 * botão, timer): quem despacha só conhece o nome, nunca o
	 * comando concreto por trás dele.
	 */
	commands_dispatch("LED_ON");
	commands_dispatch("LED_OFF");
	commands_dispatch("FOO");

	return 0;
}
