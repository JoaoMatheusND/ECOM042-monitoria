/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file board.h
 *
 * @brief LED e botao obtidos do devicetree (aliases led0 e sw0).
 * Esta e a unica camada que chama a API de GPIO.
 *******************************************************************/

#ifndef BOARD_H_
#define BOARD_H_

#include <stdbool.h>
#include <stdint.h>

int board_init(void);
int board_led_set(bool on);
int board_led_get(bool *on);
int board_button_get(int *level);
int64_t board_uptime_ms(void);

#endif /* BOARD_H_ */
