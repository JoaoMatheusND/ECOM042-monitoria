/*******************************************************************
 * @file uart_transport.h
 *
 * @brief Transporte da shell pela UART (RX por interrupcao).
 *******************************************************************/

#ifndef UART_TRANSPORT_H_
#define UART_TRANSPORT_H_

#include <stddef.h>

#include <zephyr/device.h>

/**
 * @brief Trata uma linha completa e escreve a resposta em @p out
 *        (sem o "\r\n" final).
 */
typedef void (*uart_transport_line_cb_t)(const char *line, char *out, size_t out_size);

/**
 * @brief Habilita o RX por interrupcao e inicia a thread da shell.
 *
 * @return 0 em sucesso ou erro negativo.
 */
int uart_transport_start(const struct device *uart, uart_transport_line_cb_t on_line);

#endif /* UART_TRANSPORT_H_ */
