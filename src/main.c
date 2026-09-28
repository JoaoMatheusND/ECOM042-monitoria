/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include "command.h"
#include <zephyr/kernel.h>

typedef struct {

	char *name1;
	char *name2;
} Data1;

typedef struct {
	uint8_t bit0: 1;
	uint8_t bit1: 1;
	uint8_t bit2: 1;
	uint8_t bit3: 1;
	uint8_t bit4: 1;
	uint8_t bit5: 1;
	uint8_t bit6: 1;
	uint8_t bit7: 1;
} Bits;

bool get_bit(Bits *bits, uint8_t i)
{

	return (*(uint8_t *)bits & 1 << i) ? 1 : 0;
}

void f1(void *usr_data)
{
	Data1 *user_data = (Data1 *)usr_data;

	printk("Name1: %s\n", user_data->name1);
	printk("Name2: %s\n", user_data->name2);
}

void f2(void *usr_data)
{
	Bits *a_port_bits = (Bits *)usr_data;
	for (uint8_t i = 0; i < 8; i++) {

		printk("Bit number 1: %u\n", get_bit(a_port_bits, i));
	}
}

const bool State = true;
void f3()
{
	printk("State: %s\n", State ? "True" : "False");
}

void run_commands(struct command *commands[], size_t size)
{
	for (size_t i = 0; i < size; i++) {
		printk("\nCommand%d:\n", i);
		command_run(commands[i]);
	}
}

void destroy_commands(struct command *commands[], size_t size)
{
	for (size_t i = 0; i < size; i++) {
		printk("\nCommand%d: destroyed\n", i);
		command_destroy(commands[i]);
	}
}

int main(void)
{
	Data1 usr_data = {"Pessoa 1", "Pessoa 2"};
	struct command *cmd = COMMAND_INIT(f1, &usr_data);

	Bits bits = {0, 0, 1, 1, 1, 0, 1, 1};
	struct command *port_a_cmd = COMMAND_INIT(f2, &bits);

	struct command *check_state_cmd = COMMAND_INIT(f3, NULL);

	struct command *all_commands[3] = {cmd, port_a_cmd, check_state_cmd};

	size_t commands_size = sizeof(all_commands) / sizeof(struct command *);

	run_commands(all_commands, commands_size);
	destroy_commands(all_commands, commands_size);

	return 0;
}
