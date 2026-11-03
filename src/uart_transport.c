/*******************************************************************
 * @file uart_transport.c
 *
 * @brief Transporte da mini-shell: RX por interrupção, thread que
 *        monta as linhas e TX das respostas pela UART.
 *******************************************************************/

#include <string.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "line.h"
#include "uart_transport.h"

#define PROMPT "> "

/* ISR -> thread: um item por byte recebido. */
K_MSGQ_DEFINE(rx_queue, sizeof(uint8_t), CONFIG_MINISHELL_RX_QUEUE_LEN, 1);

K_THREAD_STACK_DEFINE(shell_stack, CONFIG_MINISHELL_THREAD_STACK_SIZE);
static struct k_thread shell_thread_data;

static const struct device *uart_dev;
static transport_line_handler_t line_handler;

static struct line_buf line;
static char resp[CONFIG_MINISHELL_RESP_SIZE];

static void uart_isr(const struct device *dev, void *user_data)
{
	uint8_t byte;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	/* Só enfileira: o parsing fica na thread. */
	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &byte, 1) != 1) {
			break;
		}
		(void)k_msgq_put(&rx_queue, &byte, K_NO_WAIT);
	}
}

static void send_str(const char *s)
{
	while (*s != '\0') {
		uart_poll_out(uart_dev, *s++);
	}
}

static void send_response(int len)
{
	if (len > 0) {
		send_str(resp);
		send_str("\r\n");
	}

	send_str(PROMPT);
}

static void shell_thread(void *p1, void *p2, void *p3)
{
	uint8_t byte;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	line_init(&line);
	send_str(PROMPT);

	while (true) {
		k_msgq_get(&rx_queue, &byte, K_FOREVER);

		switch (line_feed(&line, (char)byte)) {
		case LINE_CHAR:
			uart_poll_out(uart_dev, byte); /* eco */
			break;
		case LINE_ERASE:
			send_str("\b \b");
			break;
		case LINE_READY:
			send_str("\r\n");
			send_response(line_handler(line_get(&line), resp, sizeof(resp)));
			line_reset(&line);
			break;
		case LINE_TOO_LONG:
			send_str("\r\n");
			strcpy(resp, "ERR: line too long");
			send_response((int)strlen(resp));
			break;
		default:
			break;
		}
	}
}

int transport_start(const struct device *uart, transport_line_handler_t handler)
{
	int ret;

	if (!device_is_ready(uart)) {
		return -ENODEV;
	}

	uart_dev = uart;
	line_handler = handler;

	ret = uart_irq_callback_user_data_set(uart, uart_isr, NULL);
	if (ret < 0) {
		return ret;
	}

	k_thread_create(&shell_thread_data, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			shell_thread, NULL, NULL, NULL, CONFIG_MINISHELL_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&shell_thread_data, "minishell");

	uart_irq_rx_enable(uart);

	return 0;
}
