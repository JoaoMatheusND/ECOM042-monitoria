#include "commands.h"

#include <zephyr/kernel.h>

void command_hello(void)
{
	printk("Ola!\n");
}
