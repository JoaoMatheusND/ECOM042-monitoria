/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file led.h
 *
 * @brief LEDs independentes, todos servidos por um unico k_timer.
 *
 * Quem usa pede "LED 1 piscando devagar" e nao sabe que existe
 * timer. Os LEDs vem dos aliases led0, led1, ... do devicetree; o
 * indice aqui e a posicao na lista encontrada.
 *******************************************************************/

#ifndef LED_H_
#define LED_H_

#include <stddef.h>

/* Aceito no lugar do indice por todas as funcoes que agem num LED. */
#define LED_ALL (-1)

enum led_pattern {
	LED_PATTERN_SLOW,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT,
};

/* Configura todos os LEDs como saida, desligados, e cancela qualquer
 * pisca. Pode ser chamada de novo. -ENODEV se algum controlador nao
 * estiver pronto (nesse caso nenhum pino e reconfigurado). */
int led_init(void);

size_t led_count(void);

/* Ligar, desligar e alternar cancelam o pisca daquele LED.
 * -EINVAL para indice invalido, -ENODEV antes de led_init(). */
int led_on(int led);
int led_off(int led);
int led_toggle(int led);

/* Comeca ligado e troca a cada meio periodo do padrao.
 * -EINVAL para indice ou padrao invalido. */
int led_blink(int led, enum led_pattern pattern);

/* 1 ligado, 0 desligado, -EINVAL para indice invalido ou LED_ALL. */
int led_is_on(int led);

#endif /* LED_H_ */
