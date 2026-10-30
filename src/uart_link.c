/*******************************************************************
 * @file uart_link.c
 *
 * @brief Transporte da shell pela uart1.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include "cmd.h"
#include "line.h"
#include "uart_link.h"

#define CRLF   "\r\n"
#define PROMPT "> "

static const struct device *const uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

RING_BUF_DECLARE(rx_ring, CONFIG_MINISHELL_RX_RING_SIZE);
static K_SEM_DEFINE(rx_sem, 0, 1);

static line_t line;
static char response[CONFIG_MINISHELL_RESP_SIZE];

static K_THREAD_STACK_DEFINE(shell_stack, CONFIG_MINISHELL_THREAD_STACK_SIZE);
static struct k_thread shell_thread;

/* ISR: so copia os bytes para o ring buffer e acorda a thread */
static void uart_isr(const struct device *dev, void *user_data)
{
	uint8_t byte;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev) == 1) {
		if (uart_fifo_read(dev, &byte, sizeof(byte)) != sizeof(byte)) {
			break;
		}

		/* se o buffer estiver cheio o byte e perdido */
		(void)ring_buf_put(&rx_ring, &byte, sizeof(byte));
	}

	k_sem_give(&rx_sem);
}

static void tx_str(const char *text)
{
	while (*text != '\0') {
		uart_poll_out(uart_dev, (unsigned char)*text);
		text++;
	}
}

static void tx_reply(const char *text)
{
	tx_str(CRLF);

	if (text[0] != '\0') {
		tx_str(text);
		tx_str(CRLF);
	}

	tx_str(PROMPT);
}

static void handle_byte(char c)
{
	const char echo[2] = {c, '\0'};

	switch (line_feed(&line, c)) {
	case LINE_EVENT_ECHO:
		tx_str(echo);
		break;

	case LINE_EVENT_ERASE:
		tx_str("\b \b");
		break;

	case LINE_EVENT_READY:
		(void)cmd_dispatch(line_str(&line), response, sizeof(response));
		tx_reply(response);
		line_reset(&line);
		break;

	case LINE_EVENT_OVERFLOW:
		tx_reply(CMD_ERR_LINE_TOO_LONG);
		break;

	default:
		break;
	}
}

static void shell_thread_fn(void *p1, void *p2, void *p3)
{
	uint8_t byte;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	tx_str(CRLF PROMPT);

	for (;;) {
		(void)k_sem_take(&rx_sem, K_FOREVER);

		while (ring_buf_get(&rx_ring, &byte, sizeof(byte)) == sizeof(byte)) {
			handle_byte((char)byte);
		}
	}
}

int uart_link_init(void)
{
	int ret;

	if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}

	line_reset(&line);

	ret = uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
	if (ret < 0) {
		return ret;
	}

	uart_irq_rx_enable(uart_dev);

	(void)k_thread_create(&shell_thread, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			      shell_thread_fn, NULL, NULL, NULL, CONFIG_MINISHELL_THREAD_PRIO, 0,
			      K_NO_WAIT);

	return 0;
}
