/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include "command.h"
#include "commands.h"
#include "semaforo.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

static struct semaforo sem;

static const command_t tabela[] = {
	{"verde", cmd_verde, &sem},       {"amarelo", cmd_amarelo, &sem},
	{"vermelho", cmd_vermelho, &sem}, {"pedestre", cmd_pedestre, &sem},
	{"alerta", cmd_alerta, &sem},
};

int main(void)
{
	static const char *const pedidos[] = {
		"verde", "pedestre", "pedestre", "alerta", "turbo", "vermelho",
	};

	semaforo_init(&sem);

	for (size_t i = 0; i < ARRAY_SIZE(pedidos); i++) {
		printk("> despachando '%s'\n", pedidos[i]);
		int ret = command_dispatch(tabela, ARRAY_SIZE(tabela), pedidos[i]);

		printk("  resultado: %d\n", ret);
	}

	printk("fim dos comandos\n");

	return 0;
}
