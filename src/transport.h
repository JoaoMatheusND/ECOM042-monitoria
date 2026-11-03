#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <zephyr/drivers/uart.h>

int transport_init(const struct device *uart_dev);
void transport_send(const char *s);

#endif
