#ifndef COMMAND_H_
#define COMMAND_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Operacoes fornecidas ao parser; todos os callbacks sao obrigatorios. */
struct shell_io {
	int (*led_set)(bool on);     /**< Aplique o estado; retorne zero ou erro negativo. */
	int (*led_get)(void);        /**< Leia o LED: 0, 1 ou erro negativo. */
	int (*button_read)(void);    /**< Leia o botao: 0, 1 ou erro negativo. */
	int64_t (*uptime_get)(void); /**< Retorne o tempo desde o boot, em ms. */
};

/**
 * @brief Despache um comando e escreva a resposta sem terminador de linha final.
 * @param line Linha terminada em NUL.
 * @param io Operacoes de acesso ao hardware.
 * @param[out] response Buffer do chamador para a resposta terminada em NUL.
 * @param capacity Capacidade do buffer em bytes, incluindo o NUL.
 * @retval 0 Resposta escrita, inclusive para comandos invalidos.
 * @retval -EINVAL Ponteiro nulo ou capacidade zero.
 * @retval -ENOSPC Resposta nao coube ou houve falha na formatacao.
 */
int command_dispatch(const char *line, const struct shell_io *io, char *response, size_t capacity);

#endif
