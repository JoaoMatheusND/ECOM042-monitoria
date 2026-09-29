#include "command.h"
#include "commands.h"

#include <zephyr/kernel.h>

int main(void)
{
	/* Cada nome fica ligado a uma funcao. */
	const Command table[] = {
		{"ola", command_hello},
		{"placa", command_board},
	};
	const char *names[] = {"ola", "placa", "inexistente"};

	for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
		if (command_dispatch(table, ARRAY_SIZE(table), names[i]) != 0) {
			printk("Comando nao encontrado: %s\n", names[i]);
		}
	}

	return 0;
}
