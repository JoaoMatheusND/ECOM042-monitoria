#include "cmd.h"
#include "cmds.h"

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>

int main()
{
	int ret;

	/* Listing commands */
	printk("Listing commands\r\n");

	ret = cmd_list_commands();
	if (ret != 0) {
		return ret;
	}

	printk("\n");

	/* Getting a command description */
	printk("Getting [get_adc] command description\r\n");

	ret = cmd_help("get_adc");
	if (ret != 0) {
		return ret;
	}

	printk("\n");

	/* Trying to execute a command that does't exist */
	printk("Trying to execute a command that doesn't exist\r\n");

	ret = cmd_execute("none", NULL, 0);
	if (ret < 0) {
		printk("Command doesn't exist: %d\r\n", ret);
	}

	printk("\n");

	/* executing commands */
	printk("Executing all commands\r\n");

	ret = cmd_execute("get_adc", NULL, 0);
	if (ret < 0) {
		return ret;
	}
	printk("ADC reading: %d\r\n", ret);

	ret = cmd_execute("show_serial", NULL, 0);
	if (ret < 0) {
		return ret;
	}

	int a = 10;
	int b = 10;

	int *p_a = &a;
	int *p_b = &b;

	void *args[] = {(void *)p_a, (void *)p_b};

	ret = cmd_execute("add_value", args, 2);
	if (ret < 0) {
		return ret;
	}
	printk("Sum result: %d\r\n", ret);

	return 0;
}