/*******************************************************************
 * @file commands.c
 *
 * @brief Tabela de comandos e despacho por nome.
 * @author coquinha0 (rapidoesso@gmail.com)
 * @version 0.1
 * @date 22/09/2026
 *******************************************************************/

#include "commands.h"

#include <string.h>

#include <zephyr/kernel.h>

static void cmd_led_on(void)
{
}

static void cmd_led_off(void)
{
}

static const command_t commands[] = {
	{.name = "LED_ON", .execute = cmd_led_on},
	{.name = "LED_OFF", .execute = cmd_led_off},
};

const command_t *commands_find(const char *name)
{
	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (strcmp(commands[i].name, name) == 0) {
			return &commands[i];
		}
	}

	return NULL;
}

int commands_dispatch(const char *name)
{
	const command_t *cmd = commands_find(name);

	if (cmd == NULL) {
		printk("Comando desconhecido: %s\n", name);
		return -1;
	}

	printk("Executando: %s\n", name);
	cmd->execute();

	return 0;
}
