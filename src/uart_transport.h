/*******************************************************************
 * @file uart_transport.h
 *
 * @brief Transporte da mini-shell: RX por interrupção, thread que
 *        monta as linhas e TX das respostas pela UART.
 *******************************************************************/

#ifndef UART_TRANSPORT_H_
#define UART_TRANSPORT_H_

#include <stddef.h>

#include <zephyr/device.h>

/* Recebe uma linha completa e escreve a resposta em resp.
 * Retorna o tamanho da resposta (0 = sem resposta). */
typedef int (*transport_line_handler_t)(const char *line, char *resp, size_t resp_size);

/* Liga a interrupção de RX da UART e inicia a thread da shell. */
int transport_start(const struct device *uart, transport_line_handler_t handler);

#endif /* UART_TRANSPORT_H_ */
