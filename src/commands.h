/*******************************************************************
 * @file commands.h
 *
 * @brief Tabela de comandos e despacho por nome.
 * @author coquinha0 (rapidoesso@gmail.com)
 * @version 0.1
 * @date 22/09/2026
 *******************************************************************/

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <stddef.h>

#include "command.h"

/**
 * @brief Procura, na tabela de comandos, o comando com o nome dado.
 *
 * @param name Nome do comando (ex.: vindo de UART, BLE, botão, timer).
 * @return Ponteiro pro comando encontrado, ou NULL se não existir.
 */
const command_t *commands_find(const char *name);

/**
 * @brief Despacha (localiza e executa) o comando com o nome dado.
 *
 * Não conhece nenhum comando concreto: só percorre a tabela e chama
 * o que ela expõe. Adicionar um comando novo não muda esta função.
 *
 * @param name Nome do comando a executar.
 * @return 0 se o comando foi encontrado e executado, -1 caso contrário.
 */
int commands_dispatch(const char *name);

#endif /* COMMANDS_H_ */
