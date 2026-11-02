/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file uart_shell.h
 *
 * @brief Transporte da shell na uart1.
 *
 * A interrupcao de recepcao so enfileira bytes. Uma thread monta a
 * linha, pede a resposta ao parser e transmite por poll.
 *******************************************************************/

#ifndef UART_SHELL_H_
#define UART_SHELL_H_

#include "shell.h"

int uart_shell_start(const struct shell_io *io);

#endif /* UART_SHELL_H_ */
