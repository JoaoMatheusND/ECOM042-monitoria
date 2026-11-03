/*******************************************************************
 * @file led_ctrl.h
 *
 * @brief LEDs com pisca independente multiplexados em um único k_timer.
 *
 * Os LEDs vêm do devicetree (aliases led0, led1, ...). Quem usa o
 * módulo só pede "LED n piscando no padrão X" e não sabe nada de timer.
 *******************************************************************/

#ifndef LED_CTRL_H_
#define LED_CTRL_H_

/* Índice especial: aplica a ação em todos os LEDs. */
#define LED_CTRL_ALL (-1)

enum led_pattern {
	LED_PATTERN_SLOW,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT,
};

enum led_action {
	LED_ACTION_OFF,
	LED_ACTION_ON,
	LED_ACTION_TOGGLE,
	LED_ACTION_BLINK,
	LED_ACTION_COUNT,
};

/* Verifica os GPIOs, configura todos os LEDs como saída desligada e
 * para qualquer pisca. Pode ser chamada de novo. 0 ou errno negativo. */
int led_ctrl_init(void);

/* Quantidade de LEDs encontrados no devicetree. */
int led_ctrl_count(void);

/* Aplica uma ação a um LED (0..count-1) ou a todos (LED_CTRL_ALL).
 * pattern só é usado por LED_ACTION_BLINK, mas precisa ser válido.
 * Retorna 0, -EINVAL (argumento inválido) ou erro do GPIO. */
int led_ctrl_apply(int led, enum led_action action, enum led_pattern pattern);

/* Atalhos. Ligar, desligar e alternar cancelam o pisca daquele LED. */
int led_ctrl_on(int led);
int led_ctrl_off(int led);
int led_ctrl_toggle(int led);
int led_ctrl_blink(int led, enum led_pattern pattern);

/* Nível atual do LED: 1 aceso, 0 apagado, ou -EINVAL. */
int led_ctrl_get(int led);

#endif /* LED_CTRL_H_ */
