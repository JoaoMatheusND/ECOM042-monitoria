/*******************************************************************
 * @file cmd.h
 *
 * @brief Parsing e despacho de comandos.
 *
 * Nao conhece UART, GPIO nem printk: o acesso ao hardware e feito
 * pelos ponteiros de struct cmd_io e a resposta e escrita num buffer.
 *******************************************************************/

#ifndef CMD_H_
#define CMD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Acesso ao hardware usado pelos comandos. */
struct cmd_io {
	int (*led_set)(bool on);
	int (*led_get)(void);
	int (*btn_get)(void);
	int64_t (*uptime_ms)(void);
};

/**
 * @brief Executa uma linha de comando.
 *
 * A resposta e escrita em @p out sem o "\r\n" final; respostas com
 * varias linhas (HELP) sao separadas por "\r\n".
 *
 * @return 0 em sucesso, negativo se o comando falhou (a mensagem de
 *         erro tambem e escrita em @p out).
 */
int cmd_execute(const struct cmd_io *io, const char *line, char *out, size_t out_size);

#endif /* CMD_H_ */
