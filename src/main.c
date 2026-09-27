/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include "cmd.h"
#include <zephyr/kernel.h>

int show_value_serial_handler(struct cmd_arg *args, size_t argc) {
	int serial_value = *(int *)args[0].data; 

	printk("Value from serial: %d\r\n", serial_value); 

	return 0; 
}


/* Sims an adc value reading */
int get_adc_handler(struct cmd_arg *args, size_t argc) {
	static int adc = 250; 

	int current_adc = adc; 

	adc++; 

	return current_adc; 
}

int add_value_handler(struct cmd_arg *args, size_t argc) {
	int A = *(int *)args[0].data;
	int B = *(int *)args[1].data;

	return A + B; 
}

int main(void)
{
	int ret;

	CMD_REGISTER(serial,
		show_value_serial_handler,
		"Shows value coming from uart"); 

	CMD_REGISTER(get_adc,
		get_adc_handler,
		"Reads value from adc");

	CMD_REGISTER(add_value,
		add_value_handler,
		"Adds two values");

	/* Get command description */
	ret = cmd_help("serial"); 

	if (ret != 0) {
		printk("Can't find cmd description. Cmd does not exist\r\n");
	}

	int serial_value = 1024; 
	struct cmd_arg args1 = {.name="serial_value", .data=&serial_value};
	ret = cmd_execute("serial", &args1, 1); 
	if (ret != 0) {
		printk("Command [serial] does not exist\r\n"); 
	}

	ret = cmd_execute("get_adc", NULL, 0); 
	if (ret < 0) {
		printk("Command [get_adc] does not exist\r\n"); 
	}

	printk("Adc reading: %d\r\n", ret); 

	int value_a = 40; 
	struct cmd_arg a = {.name="value_a", .data=(void*)&value_a};

	int value_b = 20;
	struct cmd_arg b = {.name="value_b", .data=(void*)&value_b};

	struct cmd_arg args2[] = {a, b}; 

	ret = cmd_execute("add_value", args2, 2); 
	if (ret < 0) {
		printk("Command [add_value] does not exist\r\n"); 
	}

	printk("Add result: %d\r\n", ret); 

	/* Trying to execute a non existing command */
	ret = cmd_execute("not_exists", NULL, 0);
	if (ret < 0) {
		printk("Command does not exist.\n\r");
	}

	return 0;
}
