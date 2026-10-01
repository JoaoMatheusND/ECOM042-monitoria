/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include "command.h"
#include <stdlib.h>
#include <zephyr/kernel.h>

typedef struct {

	char *d1;
	char *data;
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

	printk("Firmare: %s\n", user_data->d1);
	printk("Data: %s\n", user_data->data);
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

const char *firmwares[] = {"uart", "button", "timer"};
const size_t firmwares_number = sizeof(firmwares) / sizeof(*firmwares);
int main(void)
{
	Data1 usr_data = {"data 1", "0001010101"};
	struct command *cmd = command_init(firmwares[0], f1, &usr_data);

	Bits bits1 = {0, 0, 1, 1, 1, 0, 1, 1};
	struct command *port_a_cmd = command_init(firmwares[1], f2, &bits1);

	Bits bits2 = {1, 1, 1, 1, 1, 0, 1, 1};
	struct command *port_b_cmd = command_init(firmwares[0], f2, &bits2);

	Bits bits3 = {1, 0, 1, 1, 1, 0, 1, 1};
	struct command *port_c_cmd = command_init(firmwares[1], f2, &bits3);

	struct command *check_state_cmd = command_init(firmwares[2], f3, NULL);

	struct command *all_commands[] = {cmd, port_a_cmd, port_b_cmd, port_c_cmd, check_state_cmd};

	size_t commands_size = sizeof(all_commands) / sizeof(struct command *);

	for (size_t i = 0; i < firmwares_number; i++) {
		command_dispatch(all_commands, commands_size, firmwares[i]);
	}

	return 0;
}
