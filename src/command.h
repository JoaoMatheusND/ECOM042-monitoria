#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/**
 * @brief Status de retorno da execução de um comando.
 */
typedef enum {
	CMD_OK = 0,           /**< Sucesso na execução. */
	CMD_ERR_NOT_FOUND,    /**< Comando não encontrado na tabela. */
	CMD_ERR_INVALID_ARGS, /**< Argumentos inválidos ou ausentes. */
	CMD_ERR_EXEC_FAILED,  /**< Falha na execução do hardware ou receptor. */
} cmd_status_t;

/**
 * @brief Argumentos repassados para a execução do comando.
 */
typedef struct {
	const char *arg; /**< String de argumentos repassada ao comando. */
} cmd_args_t;

/**
 * @brief Assinatura padrão para ponteiro de função de comando.
 *
 * @param args Ponteiro para estrutura de argumentos do comando.
 * @return cmd_status_t Código de status resultante da execução.
 */
typedef cmd_status_t (*cmd_exec_fn_t)(const cmd_args_t *args);

/**
 * @brief Entrada de registro na tabela de comandos.
 */
typedef struct {
	const char *name;                                /**< Identificador textual do comando. */
	cmd_status_t (*execute)(const cmd_args_t *args); /**< Função de execução associada. */
} command_t;

#endif /* COMMAND_H */
