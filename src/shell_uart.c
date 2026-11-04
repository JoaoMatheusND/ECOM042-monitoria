#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "core/line_editor.h"
#include "core/shell_cmd.h"
#include "shell_uart.h"

LOG_MODULE_REGISTER(shell_uart, LOG_LEVEL_INF);

#define PROMPT "> "
#define CRLF   "\r\n"

/* Fila ISR -> thread (1 byte por item). */
K_MSGQ_DEFINE(rx_msgq, sizeof(uint8_t), CONFIG_MINISHELL_RX_QUEUE_SIZE, 1);

K_THREAD_STACK_DEFINE(shell_stack, CONFIG_MINISHELL_THREAD_STACK_SIZE);
static struct k_thread shell_thread;

static const struct device *uart_dev;
static const struct shell_hw_ops *hw_ops;
static atomic_t rx_drops;

static char line_buf[CONFIG_MINISHELL_LINE_BUF_SIZE];
static char out_buf[CONFIG_MINISHELL_OUT_BUF_SIZE];

static void uart_rx_isr(const struct device *dev, void *user_data)
{
	uint8_t c;

	ARG_UNUSED(user_data);

	uart_irq_update(dev);
	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &c, 1) == 1) {
		if (k_msgq_put(&rx_msgq, &c, K_NO_WAIT) != 0) {
			atomic_inc(&rx_drops); /* fila cheia: byte perdido */
		}
	}
}

static void tx_buf(const char *s, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, (unsigned char)s[i]);
	}
}

static void tx_str(const char *s)
{
	tx_buf(s, strlen(s));
}

static void shell_thread_fn(void *p1, void *p2, void *p3)
{
	struct line_editor le;
	uint8_t c;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	line_editor_init(&le, line_buf, sizeof(line_buf));

	tx_str("mini-shell ready, type HELP" CRLF PROMPT);

	for (;;) {
		k_msgq_get(&rx_msgq, &c, K_FOREVER);

		switch (line_editor_feed(&le, c)) {
		case LINE_EV_ECHO:
			uart_poll_out(uart_dev, c);
			break;

		case LINE_EV_ERASE:
			tx_str("\b \b");
			break;

		case LINE_EV_LINE: {
			size_t n;

			tx_str(CRLF);
			n = shell_cmd_execute(hw_ops, line_buf, out_buf, sizeof(out_buf));
			if (n > 0) {
				tx_buf(out_buf, n);
				tx_str(CRLF);
			}
			tx_str(PROMPT);
			break;
		}

		case LINE_EV_TOO_LONG:
			tx_str(CRLF "ERR: line too long" CRLF PROMPT);
			break;

		case LINE_EV_NONE:
		default:
			break;
		}

		if (atomic_get(&rx_drops) != 0) {
			LOG_WRN("RX queue overflow: %ld byte(s) dropped",
				(long)atomic_set(&rx_drops, 0));
		}
	}
}

int shell_uart_start(const struct device *uart, const struct shell_hw_ops *hw)
{
	int ret;

	if (uart == NULL || !device_is_ready(uart)) {
		return -ENODEV;
	}
	uart_dev = uart;
	hw_ops = hw;

	ret = uart_irq_callback_user_data_set(uart, uart_rx_isr, NULL);
	if (ret < 0) {
		LOG_ERR("UART IRQ callback not available (%d)", ret);
		return ret;
	}

	k_thread_create(&shell_thread, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack),
			shell_thread_fn, NULL, NULL, NULL, CONFIG_MINISHELL_THREAD_PRIORITY, 0,
			K_NO_WAIT);
	k_thread_name_set(&shell_thread, "minishell");

	uart_irq_rx_enable(uart);
	return 0;
}
