/*******************************************************************
 * @file board.h
 *
 * @brief LED (led0) e botao (sw0) obtidos pelo devicetree.
 *******************************************************************/

#ifndef BOARD_H_
#define BOARD_H_

#include <stdbool.h>

int board_init(void);
int board_led_set(bool on);
bool board_led_get(void);
int board_btn_get(void);

#endif /* BOARD_H_ */
