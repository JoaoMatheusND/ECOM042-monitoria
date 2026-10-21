#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

/**
 * @brief Configure o LED desligado e o botao como entrada.
 * @return Zero em sucesso ou codigo negativo de erro do GPIO.
 */
int io_init(void);
/**
 * @brief Atualize a saida do LED.
 * @param on true para ligar; false para desligar.
 * @return Zero em sucesso ou codigo negativo de erro do GPIO.
 */
int led_set(bool on);
/**
 * @brief Consulte o ultimo estado aplicado ao LED.
 * @return 1 se ligado ou 0 se desligado.
 */
int led_get(void);
/**
 * @brief Leia o nivel logico do botao.
 * @return 0 ou 1 em sucesso; codigo negativo em erro.
 */
int button_read(void);

#endif
