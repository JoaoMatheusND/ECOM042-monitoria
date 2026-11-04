/*******************************************************************
 * @file uart_shell.c
 *
 * @brief Transporte UART da mini-shell.
 *******************************************************************/

#include "uart_shell.h"

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "line.h"

LOG_MODULE_REGISTER(uart_shell, CONFIG_MAIN_LOG_LEVEL);

#define PROMPT "> "

K_MSGQ_DEFINE(rx_msgq, sizeof(uint8_t), CONFIG_APP_RX_QUEUE_SIZE, 1);
K_THREAD_STACK_DEFINE(shell_stack, CONFIG_APP_SHELL_THREAD_STACK_SIZE);

static struct k_thread shell_thread;
static const struct device *shell_uart;
static uart_shell_dispatch_t shell_dispatch;
static atomic_t rx_dropped;

/* Contexto de interrupcao: so tira os bytes do FIFO e enfileira. */
static void uart_isr(const struct device *dev, void *user_data)
{
	uint8_t byte;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &byte, 1) != 1) {
			break;
		}
		if (k_msgq_put(&rx_msgq, &byte, K_NO_WAIT) != 0) {
			atomic_inc(&rx_dropped);
		}
	}
}

static void tx_write(const char *s, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		uart_poll_out(shell_uart, s[i]);
	}
}

static void tx_puts(const char *s)
{
	tx_write(s, strlen(s));
}

static void shell_thread_fn(void *p1, void *p2, void *p3)
{
	static struct line_editor le;
	static char resp[CONFIG_APP_RESP_BUF_SIZE];
	uint8_t byte;
	int n;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	line_init(&le);
	tx_puts(PROMPT);

	while (true) {
		k_msgq_get(&rx_msgq, &byte, K_FOREVER);

		switch (line_feed(&le, (char)byte)) {
		case LINE_EV_CHAR:
			tx_write((const char *)&byte, 1);
			break;
		case LINE_EV_ERASE:
			tx_puts("\b \b");
			break;
		case LINE_EV_DONE:
			tx_puts("\r\n");
			n = shell_dispatch(line_get(&le), resp, sizeof(resp));
			if (n > 0) {
				tx_write(resp, n);
				tx_puts("\r\n");
			}
			line_reset(&le);
			tx_puts(PROMPT);
			break;
		case LINE_EV_TOO_LONG:
			tx_puts("\r\nERR: line too long\r\n" PROMPT);
			break;
		case LINE_EV_NONE:
		default:
			break;
		}

		if (atomic_get(&rx_dropped) != 0) {
			LOG_WRN("fila de RX cheia: %ld bytes perdidos", atomic_set(&rx_dropped, 0));
		}
	}
}

int uart_shell_start(const struct device *uart, uart_shell_dispatch_t dispatch)
{
	int ret;

	if (!device_is_ready(uart) || dispatch == NULL) {
		return -ENODEV;
	}

	shell_uart = uart;
	shell_dispatch = dispatch;

	ret = uart_irq_callback_user_data_set(uart, uart_isr, NULL);
	if (ret != 0) {
		return ret;
	}

	k_thread_create(&shell_thread, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			shell_thread_fn, NULL, NULL, NULL, CONFIG_APP_SHELL_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&shell_thread, "uart_shell");

	uart_irq_rx_enable(uart);
	return 0;
}
