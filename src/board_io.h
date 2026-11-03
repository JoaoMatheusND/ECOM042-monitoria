/*******************************************************************
 * @file board_io.h
 *
 * @brief LED e botao do devicetree (aliases led0 e sw0).
 *******************************************************************/

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Configura o LED como saida e o botao como entrada. */
int board_io_init(void);

/** @brief Liga/desliga o LED. @return 0 ou errno negativo. */
int board_led_set(bool on);

/** @brief Devolve o ultimo estado aplicado ao LED. */
int board_led_get(bool *on);

/** @brief Le o botao (0 ou 1). @return 0 ou errno negativo. */
int board_button_read(int *pressed);

#ifdef __cplusplus
}
#endif
