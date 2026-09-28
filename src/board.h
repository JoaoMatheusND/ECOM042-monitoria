#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Define o estado do LED da placa.
 *
 * @param on true para ligar o LED, false para desligar.
 */
void board_led_set(bool on);

/**
 * @brief Obtém o estado atual do LED da placa.
 *
 * @return true se o LED estiver ligado, false caso contrário.
 */
bool board_led_get(void);

/**
 * @brief Define o estado do botão da placa (simulação de acionamento).
 *
 * @param pressed true se o botão foi pressionado, false se foi solto.
 */
void board_button_set_state(bool pressed);

/**
 * @brief Obtém o estado atual do botão da placa.
 *
 * @return true se o botão estiver pressionado, false caso contrário.
 */
bool board_button_is_pressed(void);

/**
 * @brief Obtém o tempo de atividade (uptime) do sistema.
 *
 * @return uint32_t Tempo de atividade em segundos.
 */
uint32_t board_uptime_get(void);

/**
 * @brief Solicita o reinício do sistema.
 */
void board_reset(void);

#endif /* BOARD_H */
