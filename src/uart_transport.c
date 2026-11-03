/* Transporte UART:
 *  - RX por interrupcao: a ISR so copia bytes para um ring buffer;
 *  - uma thread consome os bytes, monta linhas e chama o dispatch;
 *  - TX por uart_poll_out (simples; as respostas sao curtas).
 */
#include "uart_transport.h"
#include "dispatch.h"
#include "line_editor.h"

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>

#define PROMPT "> "
#define BANNER "\r\nminish ready. Type HELP.\r\n"

static const struct device *uart_dev;

RING_BUF_DECLARE(rx_rb, CONFIG_MINISH_RX_RING_SIZE);
static K_SEM_DEFINE(rx_sem, 0, K_SEM_MAX_LIMIT);
static atomic_t rx_dropped;

static K_THREAD_STACK_DEFINE(shell_stack, CONFIG_MINISH_THREAD_STACK_SIZE);
static struct k_thread shell_thread;

static struct line_editor editor;
static char resp[MINISH_RESP_MAX];

/* ---------- TX ---------- */

static void tx_str(const char *s)
{
	while (*s != '\0') {
		uart_poll_out(uart_dev, (unsigned char)*s++);
	}
}

/* ---------- RX (ISR): so enfileira ---------- */

static void uart_isr(const struct device *dev, void *user_data)
{
	uint8_t chunk[16];

	ARG_UNUSED(user_data);

	uart_irq_update(dev); /* retorno varia entre versoes do Zephyr: ignorar */

	while (uart_irq_rx_ready(dev)) {
		int n = uart_fifo_read(dev, chunk, sizeof(chunk));

		if (n <= 0) {
			break;
		}
		uint32_t put = ring_buf_put(&rx_rb, chunk, (uint32_t)n);

		if (put < (uint32_t)n) {
			atomic_add(&rx_dropped, (atomic_val_t)(n - (int)put));
		}
		k_sem_give(&rx_sem);
	}
}

unsigned int uart_transport_rx_dropped(void)
{
	return (unsigned int)atomic_get(&rx_dropped);
}

/* ---------- thread: monta linha e despacha ---------- */

static void send_response(const char *text)
{
	if (text[0] != '\0') {
		tx_str(text);
		tx_str("\r\n");
	}
}

static void process_byte(uint8_t c)
{
	switch (line_editor_feed(&editor, c)) {
	case LE_ECHO:
		uart_poll_out(uart_dev, c);
		break;
	case LE_BACKSPACE:
		tx_str("\b \b");
		break;
	case LE_LINE:
		tx_str("\r\n");
		(void)minish_dispatch(line_editor_line(&editor), resp, sizeof(resp));
		send_response(resp);
		tx_str(PROMPT);
		break;
	case LE_TOO_LONG:
		tx_str("\r\n");
		send_response(MINISH_ERR_LINE_TOO_LONG);
		tx_str(PROMPT);
		break;
	case LE_NONE:
	default:
		break;
	}
}

static void shell_thread_fn(void *a, void *b, void *c)
{
	uint8_t byte;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	tx_str(BANNER);
	tx_str(PROMPT);

	for (;;) {
		k_sem_take(&rx_sem, K_FOREVER);
		while (ring_buf_get(&rx_rb, &byte, 1) == 1) {
			process_byte(byte);
		}
	}
}

int uart_transport_start(const struct device *uart)
{
	int rc;

	if (!device_is_ready(uart)) {
		return -ENODEV;
	}
	uart_dev = uart;
	line_editor_init(&editor);

	k_thread_create(&shell_thread, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			shell_thread_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	k_thread_name_set(&shell_thread, "minish");

	rc = uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
	if (rc != 0) {
		return rc;
	}
	uart_irq_rx_enable(uart_dev);
	return 0;
}