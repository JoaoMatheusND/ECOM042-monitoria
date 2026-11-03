/*******************************************************************
 * @file uart_transport.c
 *
 * @brief Transporte da shell pela UART.
 *
 * A ISR so enfileira os bytes recebidos; a thread consome a fila,
 * monta as linhas (line.c) e devolve as respostas pela UART.
 *******************************************************************/

#include "line.h"
#include "uart_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_transport, CONFIG_MAIN_LOG_LEVEL);

#define PROMPT        "> "
#define RESP_BUF_SIZE 256

K_MSGQ_DEFINE(rx_queue, sizeof(uint8_t), CONFIG_MINISHELL_RX_QUEUE_SIZE, 1);

K_THREAD_STACK_DEFINE(shell_stack, CONFIG_MINISHELL_THREAD_STACK_SIZE);
static struct k_thread shell_thread;

static const struct device *shell_uart;
static uart_transport_line_cb_t line_cb;

static void uart_isr(const struct device *dev, void *user_data)
{
	uint8_t c;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &c, 1) != 1) {
			break;
		}
		/* fila cheia: o byte e perdido, a ISR nao pode bloquear */
		(void)k_msgq_put(&rx_queue, &c, K_NO_WAIT);
	}
}

static void tx_write(const char *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uart_poll_out(shell_uart, data[i]);
	}
}

static void tx_puts(const char *s)
{
	tx_write(s, strlen(s));
}

static void shell_thread_fn(void *p1, void *p2, void *p3)
{
	static struct line_editor editor;
	static char resp[RESP_BUF_SIZE];
	char echo[LINE_ECHO_MAX];
	size_t echo_len;
	uint8_t c;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	line_init(&editor);
	tx_puts(PROMPT);

	while (true) {
		k_msgq_get(&rx_queue, &c, K_FOREVER);

		switch (line_feed(&editor, c, echo, &echo_len)) {
		case LINE_PENDING:
			tx_write(echo, echo_len);
			break;
		case LINE_TOO_LONG:
			tx_write(echo, echo_len);
			tx_puts("ERR: line too long\r\n" PROMPT);
			break;
		case LINE_READY:
			tx_write(echo, echo_len);
			if (editor.buf[0] != '\0') {
				resp[0] = '\0';
				line_cb(editor.buf, resp, sizeof(resp));
				tx_puts(resp);
				tx_puts("\r\n");
			}
			tx_puts(PROMPT);
			break;
		}
	}
}

int uart_transport_start(const struct device *uart, uart_transport_line_cb_t on_line)
{
	int ret;

	if (!device_is_ready(uart) || on_line == NULL) {
		return -ENODEV;
	}

	shell_uart = uart;
	line_cb = on_line;

	ret = uart_irq_callback_user_data_set(uart, uart_isr, NULL);
	if (ret < 0) {
		LOG_ERR("UART sem suporte a interrupcao (%d)", ret);
		return ret;
	}

	k_thread_create(&shell_thread, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			shell_thread_fn, NULL, NULL, NULL, CONFIG_MINISHELL_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&shell_thread, "minishell");

	uart_irq_rx_enable(uart);

	return 0;
}
