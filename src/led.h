/*******************************************************************
 * @file led.h
 *
 * @brief Modulo de LED: liga, desliga, alterna e pisca LEDs de
 *        forma independente usando um unico k_timer.
 *
 * Os LEDs vem dos aliases led0, led1, ... do devicetree. Toda funcao
 * aceita LED_ALL para aplicar a acao em todos os LEDs.
 *******************************************************************/

#ifndef LED_H_
#define LED_H_

#include <stddef.h>

/** Indice especial: aplica a acao em todos os LEDs. */
#define LED_ALL (-1)

enum led_pattern {
	LED_PATTERN_SLOW,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT,
};

/**
 * @brief Verifica os GPIOs e deixa todos os LEDs desligados.
 *
 * Pode ser chamada de novo: cancela todos os piscas e para o timer.
 *
 * @return 0, ou -ENODEV se algum controlador nao estiver pronto.
 */
int led_init(void);

/** @return Quantidade de LEDs encontrados no devicetree. */
size_t led_count(void);

/** Liga o LED (ou LED_ALL), cancelando o pisca. */
int led_on(int led);

/** Desliga o LED (ou LED_ALL), cancelando o pisca. */
int led_off(int led);

/** Inverte o LED (ou LED_ALL), cancelando o pisca. */
int led_toggle(int led);

/** Faz o LED (ou LED_ALL) piscar no padrao escolhido. */
int led_blink(int led, enum led_pattern pattern);

#endif /* LED_H_ */
