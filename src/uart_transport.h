#ifndef MINISH_UART_TRANSPORT_H_
#define MINISH_UART_TRANSPORT_H_

#include <zephyr/device.h>

int uart_transport_start(const struct device *uart);
unsigned int uart_transport_rx_dropped(void);

#endif
