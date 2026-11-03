/*******************************************************************
 * @file uart_transport.c
 *
 * @brief Transporte da mini-shell pela uart1.
 *
 * A ISR apenas enfileira bytes; uma thread monta a linha (echo,
 * backspace, limite de tamanho) e chama o parser.
 *******************************************************************/

#include "uart_transport.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "line_editor.h"
#include "minishell.h"

/** @brief Canal de comandos (o console/printk continua na uart0). */
#define UART_NODE DT_NODELABEL(uart1)

/** @brief Fila de bytes preenchida pela ISR e consumida pela thread. */
K_MSGQ_DEFINE(rx_queue, sizeof(uint8_t), CONFIG_MINISHELL_RX_QUEUE_SIZE, 1);

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
static struct line_editor editor;
static char response[CONFIG_MINISHELL_RESPONSE_SIZE];

K_THREAD_STACK_DEFINE(transport_stack, CONFIG_MINISHELL_THREAD_STACK_SIZE);
static struct k_thread transport_thread_data;

static void uart_write(const char *data, size_t len)
{
	size_t i;

	for (i = 0U; i < len; i++) {
		uart_poll_out(uart_dev, (unsigned char)data[i]);
	}
}

static void uart_write_str(const char *text)
{
	uart_write(text, strlen(text));
}

/* A ISR nao faz parsing: so copia o byte para a fila. */
static void uart_isr(const struct device *dev, void *user_data)
{
	uint8_t byte;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &byte, 1) == 1) {
		(void)k_msgq_put(&rx_queue, &byte, K_NO_WAIT);
	}
}

static void dispatch_line(const char *line)
{
	size_t len = minishell_execute(line, response, sizeof(response));

	if (len > 0U) {
		uart_write(response, len);
		uart_write_str("\r\n");
	}
}

static void transport_thread(void *arg1, void *arg2, void *arg3)
{
	uint8_t byte;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	line_editor_reset(&editor);
	uart_write_str(MINISHELL_PROMPT);

	for (;;) {
		if (k_msgq_get(&rx_queue, &byte, K_FOREVER) != 0) {
			continue;
		}

		switch (line_editor_feed(&editor, (char)byte)) {
		case LINE_EDITOR_EVENT_ECHO:
			uart_write((const char *)&byte, 1U);
			break;
		case LINE_EDITOR_EVENT_ERASE:
			uart_write_str("\b \b");
			break;
		case LINE_EDITOR_EVENT_LINE_READY:
			uart_write_str("\r\n");
			dispatch_line(line_editor_text(&editor));
			line_editor_reset(&editor);
			uart_write_str(MINISHELL_PROMPT);
			break;
		case LINE_EDITOR_EVENT_LINE_TOO_LONG:
			uart_write_str("\r\n" MINISHELL_ERR_LINE_TOO_LONG "\r\n");
			uart_write_str(MINISHELL_PROMPT);
			break;
		case LINE_EDITOR_EVENT_IGNORED:
		default:
			break;
		}
	}
}

int uart_transport_init(void)
{
	if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);
	uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
	uart_irq_rx_enable(uart_dev);

	k_thread_create(&transport_thread_data, transport_stack,
			K_THREAD_STACK_SIZEOF(transport_stack), transport_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

	return 0;
}
