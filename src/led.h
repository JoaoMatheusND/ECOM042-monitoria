/*******************************************************************
 * @file led.h
 *
 * @brief Driver de LEDs independentes multiplexados em um unico timer.
 *
 * Quem chama nao sabe nada de timer: so pede "led 0 piscando devagar".
 * Use LED_ALL no lugar do indice para aplicar a acao a todos os LEDs.
 *******************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Indice especial: aplica a acao a todos os LEDs. */
#define LED_ALL (-1)

/** @brief Padroes de pisca; os periodos vem do Kconfig. */
enum led_pattern {
	LED_PATTERN_SLOW = 0,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT,
};

/** @brief Acoes aplicaveis a um LED ou a todos. */
enum led_action {
	LED_ACTION_ON = 0,
	LED_ACTION_OFF,
	LED_ACTION_TOGGLE,
	LED_ACTION_BLINK,
	LED_ACTION_COUNT,
};

/**
 * @brief Quantidade de LEDs descritos no devicetree.
 *
 * @return Numero de LEDs gerenciados pelo modulo.
 */
size_t led_count(void);

/**
 * @brief Verifica os controladores, configura todos como saida e desligados.
 *
 * Pode ser chamada mais de uma vez: o timer nao e reinicializado.
 *
 * @return 0 em sucesso, -ENODEV se algum controlador GPIO nao estiver pronto,
 *         ou outro errno negativo do driver.
 */
int led_init(void);

/**
 * @brief Liga um LED (ou todos) e cancela um pisca em andamento.
 *
 * @param led Indice do LED ou LED_ALL.
 * @return 0 em sucesso ou -EINVAL para indice invalido.
 */
int led_on(int led);

/**
 * @brief Desliga um LED (ou todos) e cancela um pisca em andamento.
 *
 * @param led Indice do LED ou LED_ALL.
 * @return 0 em sucesso ou -EINVAL para indice invalido.
 */
int led_off(int led);

/**
 * @brief Inverte o nivel de um LED (ou todos) e cancela um pisca.
 *
 * @param led Indice do LED ou LED_ALL.
 * @return 0 em sucesso ou -EINVAL para indice invalido.
 */
int led_toggle(int led);

/**
 * @brief Coloca um LED (ou todos) para piscar em um dos padroes.
 *
 * @param led     Indice do LED ou LED_ALL.
 * @param pattern Padrao desejado.
 * @return 0 em sucesso ou -EINVAL para indice/padrao invalidos.
 */
int led_blink(int led, enum led_pattern pattern);

/**
 * @brief Caminho unico de todas as acoes: "um" ou "todos" mora aqui.
 *
 * @param led     Indice do LED ou LED_ALL.
 * @param action  Acao desejada.
 * @param pattern Padrao usado apenas por LED_ACTION_BLINK.
 * @return 0 em sucesso ou -EINVAL para indice/acao/padrao invalidos.
 */
int led_apply(int led, enum led_action action, enum led_pattern pattern);

/**
 * @brief Le o nivel fisico do pino de um LED (diagnostico).
 *
 * @param led Indice do LED.
 * @param on  Recebe true se o pino estiver em nivel alto.
 * @return 0 em sucesso ou -EINVAL se o indice nao existir.
 */
int led_level_get(int led, bool *on);

#ifdef __cplusplus
}
#endif
