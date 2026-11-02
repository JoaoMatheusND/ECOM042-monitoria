/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file shell_line.h
 *
 * @brief Monta uma linha a partir de bytes soltos.
 *
 * Trata backspace, estouro do buffer e fim de linha (CR ou LF).
 * Nao transmite nada: devolve o eco e, quando a linha fecha, a
 * resposta pronta para o transporte escrever na UART.
 *******************************************************************/

#ifndef SHELL_LINE_H_
#define SHELL_LINE_H_

#include "shell.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct shell_line {
	char *buf;
	size_t cap;
	size_t len;
	bool overflow;
	uint8_t swallow;
};

/* storage precisa ter cap + 1 bytes (o limite do Kconfig nao inclui o '\0'). */
void shell_line_init(struct shell_line *line, char *storage, size_t cap);

/* echo recebe o que deve voltar para o terminal agora (pode ser vazio).
 * line_done fica true quando um CR ou LF fecha a linha. response nao
 * inclui o \r\n final. */
void shell_line_feed(struct shell_line *line, uint8_t byte, const struct shell_io *io, char *echo,
		     size_t echo_cap, size_t *echo_len, char *response, size_t response_cap,
		     bool *line_done);

#endif /* SHELL_LINE_H_ */
