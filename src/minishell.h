/*******************************************************************
 * @file minishell.h
 *
 * @brief Interface do parser/dispatch da mini-shell.
 *
 * Este modulo nao conhece UART, GPIO nem printk: recebe uma linha e
 * escreve a resposta num buffer. O acesso a hardware entra por injecao
 * (struct minishell_hw_ops), o que tambem permite testar com fakes.
 *******************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Operacoes de hardware usadas pelos comandos LED e BTN. */
struct minishell_hw_ops {
	/** @brief Liga/desliga o LED. @return 0 ou errno negativo. */
	int (*led_set)(bool on);
	/** @brief Le o estado logico do LED. @return 0 ou errno negativo. */
	int (*led_get)(bool *on);
	/** @brief Le o botao sw0 (0 ou 1). @return 0 ou errno negativo. */
	int (*button_read)(int *pressed);
};

/** @brief Respostas de erro compartilhadas com a camada de transporte. */
#define MINISHELL_ERR_UNKNOWN_COMMAND "ERR: unknown command"
#define MINISHELL_ERR_LINE_TOO_LONG   "ERR: line too long"

/** @brief Prompt impresso antes de cada linha de comando. */
#define MINISHELL_PROMPT "> "

/**
 * @brief Conecta as operacoes de hardware usadas pelos comandos.
 *
 * @param ops Tabela de operacoes; precisa continuar valida enquanto a
 *            shell estiver em uso (normalmente uma const global).
 */
void minishell_init(const struct minishell_hw_ops *ops);

/**
 * @brief Executa uma linha de comando.
 *
 * @param line    Linha sem o terminador (\r ou \n).
 * @param out     Buffer onde a resposta e escrita (sem CRLF final).
 * @param out_len Tamanho do buffer de resposta.
 *
 * @return Numero de bytes escritos em @p out (0 para linha vazia).
 */
size_t minishell_execute(const char *line, char *out, size_t out_len);

#ifdef __cplusplus
}
#endif
