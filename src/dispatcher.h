#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "command.h"

#include <stddef.h>

/**
 * @brief Despacha um comando pesquisando seu identificador na tabela informada.
 *
 * @param table Vetor de entradas de comando disponíveis.
 * @param table_len Quantidade de entradas presentes na tabela.
 * @param name Nome textual do comando a ser executado.
 * @param args Estrutura de argumentos repassada ao comando.
 * @return cmd_status_t CMD_OK em caso de sucesso, CMD_ERR_NOT_FOUND se o comando
 *                      não for localizado, ou o código de status retornado pelo comando.
 */
cmd_status_t command_dispatch(const command_t *table, size_t table_len, const char *name,
			      const cmd_args_t *args);

#endif /* DISPATCHER_H */
