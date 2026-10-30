/*******************************************************************
 * @file board_io.h
 *
 * @brief LED e botao da placa.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

int board_io_init(void);
int board_io_led_set(bool on);
bool board_io_led_get(void);

/* 1 pressionado, 0 solto ou erro negativo */
int board_io_button_get(void);

#endif /* BOARD_IO_H_ */
