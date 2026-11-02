/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file uart_shell.c
 *
 * @brief ISR curta + thread que fala com o operador.
 *
 * A interrupcao nao interpreta texto. Se o parsing ficasse aqui,
 * qualquer comando lento atrasaria a proxima recepcao e as outras
 * interrupcoes do sistema. O byte entra numa fila; a thread acorda,
 * monta a linha e so entao chama o parser.
 *******************************************************************/

#include "uart_shell.h"

#include "shell_line.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

/* A placa escolhe a UART no overlay (chosen app,shell-uart). Assim o
 * mesmo codigo roda numa placa cuja segunda UART se chame usart2. */
#if !DT_HAS_CHOSEN(app_shell_uart)
#error "Defina chosen { app,shell-uart = &uartX; } no overlay da placa."
#endif

#define SHELL_UART_NODE DT_CHOSEN(app_shell_uart)

#if !DT_NODE_HAS_STATUS(SHELL_UART_NODE, okay)
#error "A UART da shell esta desabilitada. Habilite no overlay da placa."
#endif

#define RX_QUEUE_DEPTH   256
#define SHELL_STACK_SIZE 4096
/* Prioridade positiva: thread preemptivel comum, nao uma ISR. */
#define SHELL_PRIORITY   7

static const struct device *const uart_dev = DEVICE_DT_GET(SHELL_UART_NODE);

K_MSGQ_DEFINE(rx_q, sizeof(uint8_t), RX_QUEUE_DEPTH, 1);
K_THREAD_STACK_DEFINE(shell_stack, SHELL_STACK_SIZE);

static struct k_thread shell_thread_data;
static struct shell_io shell_io;
static bool shell_started;

static void uart_write(const struct device *dev, const char *text, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		uart_poll_out(dev, (unsigned char)text[i]);
	}
}

static void uart_write_str(const struct device *dev, const char *text)
{
	uart_write(dev, text, strlen(text));
}

static void uart_rx_isr(const struct device *dev, void *user_data)
{
	uint8_t byte;

	ARG_UNUSED(user_data);

	/* Uma vez por entrada na ISR, antes de perguntar se ha byte. */
	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev) > 0) {
		if (uart_fifo_read(dev, &byte, 1) != 1) {
			break;
		}
		/* K_NO_WAIT: a ISR nao pode dormir. Fila cheia descarta o byte. */
		(void)k_msgq_put(&rx_q, &byte, K_NO_WAIT);
	}
}

static void shell_thread(void *p1, void *p2, void *p3)
{
	char storage[CONFIG_APP_CMD_LINE_MAX + 1];
	char response[CONFIG_APP_CMD_LINE_MAX + 160];
	struct shell_line line;
	uint8_t byte;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	shell_line_init(&line, storage, CONFIG_APP_CMD_LINE_MAX);
	uart_write_str(uart_dev, "> ");

	while (k_msgq_get(&rx_q, &byte, K_FOREVER) == 0) {
		char echo[8];
		size_t echo_len = 0;
		bool line_done = false;

		shell_line_feed(&line, byte, &shell_io, echo, sizeof(echo), &echo_len, response,
				sizeof(response), &line_done);
		if (echo_len > 0) {
			uart_write(uart_dev, echo, echo_len);
		}
		if (!line_done) {
			continue;
		}
		if (response[0] != '\0') {
			uart_write_str(uart_dev, response);
			uart_write_str(uart_dev, "\r\n");
		}
		uart_write_str(uart_dev, "> ");
	}
}

int uart_shell_start(const struct shell_io *io)
{
	k_tid_t tid;
	int ret;

	if (io == NULL) {
		return -EINVAL;
	}
	if (shell_started) {
		return -EBUSY;
	}
	if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}

	shell_io = *io;

	ret = uart_irq_callback_user_data_set(uart_dev, uart_rx_isr, NULL);
	if (ret < 0) {
		return ret;
	}

	uart_irq_rx_enable(uart_dev);
	shell_started = true;

	tid = k_thread_create(&shell_thread_data, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			      shell_thread, NULL, NULL, NULL, SHELL_PRIORITY, 0, K_NO_WAIT);
	(void)k_thread_name_set(tid, "uart_shell");
	return 0;
}
