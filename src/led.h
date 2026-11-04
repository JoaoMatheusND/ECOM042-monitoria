/*******************************************************************
 * @file led.h
 *
 * @brief Modulo de LED: liga, desliga, alterna e pisca N LEDs com
 *        periodos independentes usando um unico k_timer.
 *******************************************************************/

#ifndef LED_H_
#define LED_H_

#include <stdbool.h>

#define LED_ALL (-1)

enum led_pattern {
	LED_PATTERN_SLOW,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT,
};

enum led_action {
	LED_ACTION_ON,
	LED_ACTION_OFF,
	LED_ACTION_TOGGLE,
	LED_ACTION_BLINK,
	LED_ACTION_COUNT,
};

int led_init(void);

/** Quantidade de LEDs encontrados no devicetree. */
int led_count(void);

int led_execute(int led, enum led_action action, enum led_pattern pattern);

/* Atalhos. ON/OFF/TOGGLE cancelam o pisca em andamento do LED. */
int led_on(int led);
int led_off(int led);
int led_toggle(int led);
int led_blink(int led, enum led_pattern pattern);

/** Nivel logico atual do LED (true = ligado); false se @p led invalido. */
bool led_is_on(int led);

#endif /* LED_H_ */
