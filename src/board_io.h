/*******************************************************************
 * @file board_io.h
 *
 * @brief LED e botao obtidos pelo devicetree (led0 / sw0).
 *******************************************************************/

#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

int board_io_init(void);
int board_io_led_set(bool on);
int board_io_led_get(void);
int board_io_btn_get(void);

#endif /* BOARD_IO_H_ */
