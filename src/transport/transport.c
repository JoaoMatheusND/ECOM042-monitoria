#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "cmd.h"
#include "dispatch.h"
#include "line.h"
#include "transport.h"

LOG_MODULE_REGISTER(transport, LOG_LEVEL_INF);

#define PROMPT "> "
#define THREAD_PRIORITY K_PRIO_PREEMPT(5)

K_MSGQ_DEFINE(rx_queue, sizeof(uint8_t), CONFIG_UART_RX_QUEUE_SIZE, 1);

K_THREAD_STACK_DEFINE(thread_stack, CONFIG_UART_THREAD_STACK_SIZE);
static struct k_thread thread_data;

/* Set once by transport_init(), before the thread and the interrupt start */
static const struct device *uart_dev;
static const struct cmd_entry *cmd_table;

static void tx_char(char c) {
    uart_poll_out(uart_dev, (unsigned char)c);
}

static void tx_str(const char *str) {
    for(; *str != '\0'; str++) {
        tx_char(*str);
    }
}

/* Interrupt callback: only queues the received bytes */
static void uart_cb(const struct device *dev, void *user_data) {
    uint8_t c;

    ARG_UNUSED(user_data);

    uart_irq_update(dev);

    if(uart_irq_rx_ready(dev) <= 0) {
        return;
    }

    while(uart_fifo_read(dev, &c, 1) == 1) {
        k_msgq_put(&rx_queue, &c, K_NO_WAIT);
    }
}

/**
 * @brief Send the response of a command to the user.
 *
 * @param[in] ret Value returned by dispatch().
 * @param[in] out Response filled by the handler; only valid when @p ret is 0.
 */
static void send_response(int ret, const struct cmd_out *out) {

    if(ret == -ENOENT || ret == -EINVAL) {
        tx_str("ERR: unknown command");
        return;
    }

    if(ret != 0) {
        LOG_ERR("dispatch failed: %d", ret);
        tx_str("ERR: internal error");
        return;
    }

    if(out->overflow) {
        tx_str("ERR: response too long");
    } else if(out->ret != 0 && out->len == 0) {
        tx_str("ERR: command failed");
    } else {
        tx_str(out->buf);
    }
}

/* Runs the command typed in a terminated line and sends its response */
static void handle_line(const struct line *line) {
    struct cmd_out out = {0};
    int ret;

    if(line->overflow) {
        tx_str("ERR: line too long");
        return;
    }

    ret = dispatch(cmd_table, line->buf, &out);
    send_response(ret, &out);
}

static void shell_thread(void *p1, void *p2, void *p3) {
    struct line line = {0};
    size_t prev_len;
    uint8_t c;

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    tx_str(PROMPT);

    while(true) {
        k_msgq_get(&rx_queue, &c, K_FOREVER);

        prev_len = line.len;
        line_feed(&line, (char)c);

        if(line.line_terminated) {
            tx_str("\r\n");
            handle_line(&line);
            tx_str("\r\n" PROMPT);
            line_reset(&line);
        }
        /* The echo relies on line_feed() changing len by at most one per byte */
        else if(line.len > prev_len) {
            tx_char((char)c);
        } else if(line.len < prev_len) {
            tx_str("\b \b");
        }
    }
}

int transport_init(const struct device *uart, const struct cmd_entry *table) {
    int ret;

    if(uart == NULL || table == NULL) {
        return -EINVAL;
    }

    if(!device_is_ready(uart)) {
        return -ENODEV;
    }

    uart_dev = uart;
    cmd_table = table;

    ret = uart_irq_callback_user_data_set(uart, uart_cb, NULL);

    if(ret < 0) {
        return ret;
    }

    k_thread_create(&thread_data, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack),
                    shell_thread, NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&thread_data, "shell");

    /* Last, so no byte arrives before the thread exists */
    uart_irq_rx_enable(uart);

    return 0;
}