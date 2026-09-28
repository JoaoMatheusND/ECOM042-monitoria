#ifndef COMMANDS_H
#define COMMANDS_H

#include "command.h"

/**
 * @brief Comando para ligar o LED da placa.
 *
 * @param args Argumentos do comando (não utilizados).
 * @return cmd_status_t CMD_OK.
 */
cmd_status_t cmd_led_on(const cmd_args_t *args);

/**
 * @brief Comando para desligar o LED da placa.
 *
 * @param args Argumentos do comando (não utilizados).
 * @return cmd_status_t CMD_OK.
 */
cmd_status_t cmd_led_off(const cmd_args_t *args);

/**
 * @brief Comando para simular o pressionamento do botão da placa.
 *
 * @param args Argumentos do comando (não utilizados).
 * @return cmd_status_t CMD_OK.
 */
cmd_status_t cmd_button_press(const cmd_args_t *args);

/**
 * @brief Comando para simular a liberação do botão da placa.
 *
 * @param args Argumentos do comando (não utilizados).
 * @return cmd_status_t CMD_OK.
 */
cmd_status_t cmd_button_release(const cmd_args_t *args);

/**
 * @brief Comando para consultar o status atual do sistema (LED, botão e uptime).
 *
 * @param args Argumentos do comando (não utilizados).
 * @return cmd_status_t CMD_OK.
 */
cmd_status_t cmd_get_status(const cmd_args_t *args);

/**
 * @brief Comando para solicitar reinício do sistema.
 *
 * @param args Argumentos do comando (não utilizados).
 * @return cmd_status_t CMD_OK.
 */
cmd_status_t cmd_reset(const cmd_args_t *args);

#endif /* COMMANDS_H */
