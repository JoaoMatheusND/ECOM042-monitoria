#include "line.h"
#include "transport.h"

#include <errno.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

struct rx_byte {
	uint32_t sequence;
	unsigned char value;
};

K_MSGQ_DEFINE(rx_queue, sizeof(struct rx_byte), 256, 4);
K_THREAD_STACK_DEFINE(shell_stack, 2048);

static struct k_thread shell_thread;
static const struct device *const uart = DEVICE_DT_GET(DT_NODELABEL(uart1));
static const struct shell_io *shell_io;
static uint32_t rx_sequence;

static void send_text(const char *text)
{
	while (*text != '\0') {
		uart_poll_out(uart, (unsigned char)*text++);
	}
}

static void uart_receive(const struct device *dev, void *user_data)
{
	struct rx_byte item;

	ARG_UNUSED(user_data);
	(void)uart_irq_update(dev);
	if (uart_irq_rx_ready(dev) <= 0) {
		return;
	}

	/* A interrupcao apenas retira os bytes da UART e enfileira. */
	while (uart_fifo_read(dev, &item.value, 1) == 1) {
		item.sequence = rx_sequence++;
		if (k_msgq_put(&rx_queue, &item, K_NO_WAIT) != 0) {
			/* A lacuna na sequencia avisa a thread sobre a perda. */
			continue;
		}
	}
}

static void shell_run(void *arg1, void *arg2, void *arg3)
{
	struct line_buffer line = {0};
	struct rx_byte item;
	uint32_t expected = 0;
	char response[CONFIG_MINI_SHELL_LINE_SIZE + 128];
	enum line_result result;
	size_t previous_length;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	send_text("> ");

	while (true) {
		if (k_msgq_get(&rx_queue, &item, K_FOREVER) != 0) {
			continue;
		}
		if (item.sequence != expected) {
			line.discard = true;
			line.after_cr = false;
		}
		expected = item.sequence + 1;

		if (line.after_cr && item.value == '\n') {
			(void)line_feed(&line, item.value);
			continue;
		}

		previous_length = line.length;
		result = line_feed(&line, item.value);
		if (result == LINE_WAIT) {
			if (item.value == 0x08 || item.value == 0x7f) {
				if (line.length < previous_length) {
					send_text("\b \b");
				}
			} else if (item.value != '\0') {
				uart_poll_out(uart, item.value);
			}
			continue;
		}

		send_text("\r\n");
		if (result == LINE_TOO_LONG) {
			send_text("ERR: line too long");
		} else if (command_dispatch(line.text, shell_io, response, sizeof(response)) < 0) {
			send_text("ERR: response too long");
		} else {
			send_text(response);
		}
		send_text("\r\n> ");
	}
}

int transport_init(const struct shell_io *io)
{
	int ret;

	if (!device_is_ready(uart)) {
		return -ENODEV;
	}
	shell_io = io;
	ret = uart_irq_callback_user_data_set(uart, uart_receive, NULL);
	if (ret < 0) {
		return ret;
	}

	k_thread_create(&shell_thread, shell_stack, K_THREAD_STACK_SIZEOF(shell_stack), shell_run,
			NULL, NULL, NULL, 7, 0, K_NO_WAIT);
	uart_irq_rx_enable(uart);
	return 0;
}