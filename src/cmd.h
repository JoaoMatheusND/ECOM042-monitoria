/*******************************************************************
 * @file cmd.h
 *
 * @brief Parsing e dispatch dos comandos da mini-shell.
 *******************************************************************/

#ifndef CMD_H_
#define CMD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Operacoes de "hardware" que os comandos precisam. */
struct cmd_hal {
	int (*led_set)(bool on);
	bool (*led_get)(void);
	int (*btn_get)(void);
	int64_t (*uptime_ms)(void);
};

/**
 * Entrada da tabela de comandos.
 *
 * @param args resto da linha depois do nome do comando ("" se nao houver)
 * @param out  buffer de resposta
 * @param len  tamanho de @p out
 * @return quantidade de bytes escritos em @p out
 */
typedef int (*cmd_handler_t)(const char *args, char *out, size_t len);

struct cmd_entry {
	const char *name;
	const char *usage;
	cmd_handler_t handler;
};

/** Registra as operacoes de hardware usadas pelos comandos. */
void cmd_init(const struct cmd_hal *hal);

/**
 * Interpreta @p line e escreve a resposta (sem o "\r\n" final) em @p out.
 * Respostas com varias linhas usam "\r\n" entre elas.
 *
 * @return bytes escritos em @p out; 0 para linha vazia (sem resposta)
 */
int cmd_dispatch(const char *line, char *out, size_t len);

#endif /* CMD_H_ */
