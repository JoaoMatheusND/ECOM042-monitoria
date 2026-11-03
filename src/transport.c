#include "transport.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include "shell.h"

#define RX_BUF_SIZE CONFIG_SHELL_LINE_BUF_SIZE

static const struct device *uart_dev;
static struct ring_buf rx_rb;
static uint8_t rx_buf[RX_BUF_SIZE];
static char line_buf[CONFIG_SHELL_LINE_BUF_SIZE];
static size_t line_len;
static bool overflow;

static K_SEM_DEFINE(rx_sem, 0, 1);
static K_THREAD_STACK_DEFINE(stack, CONFIG_SHELL_THREAD_STACK_SIZE);
static struct k_thread thread;

static void send_char(char c)
{
	uart_poll_out(uart_dev, c);
}

void transport_send(const char *s)
{
	while (*s != '\0') {
		send_char(*s++);
	}
}

static void dispatch_line(void)
{
	char resp[CONFIG_SHELL_LINE_BUF_SIZE];

	if (overflow) {
		overflow = false;
		line_len = 0;
		transport_send("ERR: line too long\r\n");
		return;
	}

	if (line_len == 0) {
		return;
	}

	line_buf[line_len] = '\0';
	int n = shell_dispatch(line_buf, resp, sizeof(resp));
	if (n > 0) {
		transport_send(resp);
		transport_send("\r\n");
	} else if (n == 0 && resp[0] != '\0') {
		transport_send(resp);
		transport_send("\r\n");
	}
	line_len = 0;
}

static void rx_byte(uint8_t byte)
{
	if (byte == '\r' || byte == '\n') {
		dispatch_line();
		transport_send("> ");
		return;
	}

	if (byte == 0x08 || byte == 0x7F) {
		if (line_len > 0) {
			line_len--;
			transport_send("\b \b");
		}
		return;
	}

	if (line_len < CONFIG_SHELL_LINE_BUF_SIZE - 1) {
		line_buf[line_len++] = byte;
	} else {
		overflow = true;
	}
}

static void uart_isr(const struct device *dev, void *user_data)
{
	(void)user_data;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	uint8_t byte;
	while (uart_fifo_read(dev, &byte, 1) == 1) {
		ring_buf_put(&rx_rb, &byte, 1);
		k_sem_give(&rx_sem);
	}
}

static void thread_fn(void *p1, void *p2, void *p3)
{
	(void)p1;
	(void)p2;
	(void)p3;

	transport_send("\r\n> ");

	while (1) {
		k_sem_take(&rx_sem, K_FOREVER);

		uint8_t byte;
		while (ring_buf_get(&rx_rb, &byte, 1) == 1) {
			send_char((char)byte);
			rx_byte(byte);
		}
	}
}

int transport_init(const struct device *dev)
{
	uart_dev = dev;

	if (!device_is_ready(uart_dev)) {
		return -ENODEV;
	}

	ring_buf_init(&rx_rb, sizeof(rx_buf), rx_buf);

	int err = uart_irq_callback_user_data_set(uart_dev, uart_isr, NULL);
	if (err < 0) {
		return err;
	}

	uart_irq_rx_enable(uart_dev);

	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
			thread_fn, NULL, NULL, NULL,
			CONFIG_SHELL_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread, "shell");

	return 0;
}
