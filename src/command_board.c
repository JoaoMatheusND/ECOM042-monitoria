#include "commands.h"

#include <zephyr/kernel.h>

void command_board(void)
{
	printk("Placa: %s\n", CONFIG_BOARD_TARGET);
}
