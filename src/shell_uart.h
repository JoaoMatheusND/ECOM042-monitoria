/* Transporte UART: RX por interrupcao -> fila -> thread -> core. */
#ifndef SHELL_UART_H_
#define SHELL_UART_H_

#include "core/shell_cmd.h"
#include <zephyr/device.h>

int shell_uart_start(const struct device *uart, const struct shell_hw_ops *hw);

#endif
