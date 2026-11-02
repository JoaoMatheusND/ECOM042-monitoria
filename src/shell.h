/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file shell.h
 *
 * @brief Camada de comando. Nao conhece UART, GPIO nem printk:
 * recebe uma linha ja montada e escreve a resposta num buffer.
 *
 * O hardware entra por function pointers (struct shell_io). Quem
 * chama decide se isso e a placa de verdade ou um dublê de teste.
 *******************************************************************/

#ifndef SHELL_H_
#define SHELL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct shell_io {
	int (*led_set)(bool on);
	int (*led_get)(bool *on);
	int (*button_get)(int *level);
	int64_t (*uptime_ms)(void);
};

typedef void (*shell_cmd_fn)(const char *args, const struct shell_io *io, char *out,
			     size_t out_len);

struct shell_command {
	const char *name;
	const char *usage;
	shell_cmd_fn handler;
};

/* out sempre termina em '\0'. Resposta sem o \r\n final: quem transmite
 * acrescenta o fim de linha. Linha vazia deixa out vazio. */
void shell_dispatch(const char *line, const struct shell_io *io, char *out, size_t out_len);

#endif /* SHELL_H_ */
