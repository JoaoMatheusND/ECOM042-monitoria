#ifndef APP_LED_H_
#define APP_LED_H_

#include <stddef.h>

/** @brief Selecione todos os LEDs em uma unica chamada. */
#define LED_ALL (-1)

/** @brief Padroes de tempo entre alternancias, definidos no Kconfig. */
enum led_pattern {
	LED_SLOW,         /**< Intervalo lento. */
	LED_NORMAL,       /**< Intervalo normal. */
	LED_FAST,         /**< Intervalo rapido. */
	LED_PATTERN_COUNT /**< Limite dos valores validos. */
};

/**
 * @brief Configure todos os LEDs desligados e cancele as piscadas.
 * Pode ser chamada novamente; o timer e inicializado uma unica vez.
 * @return Zero em sucesso ou erro negativo de dispositivo/GPIO.
 */
int leds_init(void);

/** @brief Consulte a quantidade de aliases led0 a led7 disponiveis. */
size_t leds_count(void);

/**
 * @brief Ligue um LED e cancele sua piscada.
 * @param id Indice do LED (a partir de zero) ou LED_ALL.
 * @return Zero, -EINVAL para indice invalido, -EACCES antes da inicializacao,
 * ou erro negativo do GPIO.
 */
int led_on(int id);

/**
 * @brief Desligue um LED e cancele sua piscada.
 * @param id Indice do LED ou LED_ALL.
 * @return Zero ou erro negativo, como em led_on().
 */
int led_off(int id);

/**
 * @brief Alterne um LED e cancele sua piscada.
 * @param id Indice do LED ou LED_ALL.
 * @return Zero ou erro negativo, como em led_on().
 */
int led_toggle(int id);

/**
 * @brief Inicie a piscada, mantendo o nivel atual ate a primeira alternancia.
 * @param id Indice do LED ou LED_ALL.
 * @param pattern Padrao entre LED_SLOW e LED_FAST.
 * @return Zero ou erro negativo, como em led_on(); padrao invalido gera -EINVAL.
 */
int led_blink(int id, enum led_pattern pattern);

#endif
