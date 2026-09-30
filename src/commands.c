#include "commands.h"
#include <zephyr/sys/printk.h>

static void uart_execute(struct command *cmd)
{
	printk("UART command executed\n");
}

static void ble_execute(struct command *cmd)
{
	printk("BLE command executed\n");
}

static void button_execute(struct command *cmd)
{
	printk("BUTTON command executed\n");
}

static void timer_execute(struct command *cmd)
{
	printk("TIMER command executed\n");
}

struct command cmd_uart = {.execute = uart_execute};
struct command cmd_ble = {.execute = ble_execute};
struct command cmd_button = {.execute = button_execute};
struct command cmd_timer = {.execute = timer_execute};
