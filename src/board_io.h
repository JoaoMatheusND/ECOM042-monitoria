/*******************************************************************
 * @file board_io.h
 *
 * @brief LED (led0) e botão (sw0) obtidos pelo devicetree.
 *******************************************************************/

#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

/* Configura LED (saída, desligado) e botão (entrada). 0 ou errno negativo. */
int io_init(void);

/* Liga/desliga o LED. 0 ou errno negativo. */
int led_set(bool on);

/* Estado atual do LED: 1 ligado, 0 desligado. */
int led_get(void);

/* Lê o botão: 1 pressionado, 0 solto, ou errno negativo. */
int button_read(void);

#endif /* BOARD_IO_H_ */
