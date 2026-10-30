/*******************************************************************
 * @file led.h
 *
 * @brief Modulo de LEDs com pisca independente usando um unico timer.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#ifndef LED_H_
#define LED_H_

#include <stddef.h>

typedef enum {
	LED_PATTERN_SLOW = 0,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT,
} led_pattern_t;

/* usar no lugar do indice para aplicar a todos os LEDs */
#define LED_ALL ((size_t) - 1)

int led_init(void);
int led_on(size_t led);
int led_off(size_t led);
int led_toggle(size_t led);
int led_blink(size_t led, led_pattern_t pattern);
size_t led_count(void);

#endif /* LED_H_ */
