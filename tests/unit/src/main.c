/*******************************************************************
 * @file main.c
 *
 * @brief Testes do modulo de LEDs.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "led.h"

static const struct gpio_dt_spec spec[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static const struct device *const ctrl = DEVICE_DT_GET(DT_NODELABEL(gpio_test));

/* nivel do pino lido no controlador emulado */
static int level(size_t led)
{
	return gpio_emul_output_get(spec[led].port, spec[led].pin);
}

/* true se o LED foi visto aceso e apagado dentro da janela */
static bool seen_on_and_off(size_t led, int64_t window_ms)
{
	int64_t end = k_uptime_get() + window_ms;
	bool on = false;
	bool off = false;

	while ((k_uptime_get() < end) && !(on && off)) {
		k_msleep(1);
		if (level(led) == 1) {
			on = true;
		} else {
			off = true;
		}
	}

	return on && off;
}

/* conta as trocas de nivel do led0 e do led1 dentro da janela */
static void count_transitions(int64_t window_ms, int *fast, int *slow)
{
	int64_t end = k_uptime_get() + window_ms;
	int prev0 = level(0);
	int prev1 = level(1);

	*fast = 0;
	*slow = 0;

	while (k_uptime_get() < end) {
		int now0;
		int now1;

		k_msleep(1);
		now0 = level(0);
		now1 = level(1);

		if (now0 != prev0) {
			(*fast)++;
			prev0 = now0;
		}

		if (now1 != prev1) {
			(*slow)++;
			prev1 = now1;
		}
	}
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* o teste 00 precisa do controlador ainda sem init */
	if (!device_is_ready(ctrl)) {
		return;
	}

	zassert_ok(led_init());
}

ZTEST_SUITE(led_mod, NULL, NULL, before_each, NULL, NULL);

ZTEST(led_mod, test_00_init_falha_com_controlador_nao_pronto)
{
	if (device_is_ready(ctrl)) {
		ztest_test_skip();
	}

	zassert_equal(led_init(), -ENODEV);

	zassert_equal(led_on(0U), -ENODEV);

	zassert_ok(device_init(ctrl));
	zassert_true(device_is_ready(ctrl));
}

ZTEST(led_mod, test_01_init_deixa_todos_apagados)
{
	size_t i;

	zassert_equal(led_count(), ARRAY_SIZE(spec));

	for (i = 0U; i < led_count(); i++) {
		zassert_equal(level(i), 0, "LED %zu nao comecou apagado", i);
	}
}

ZTEST(led_mod, test_02_ligar_desligar_e_alternar_mudam_o_pino)
{
	zassert_ok(led_on(0U));
	zassert_equal(level(0), 1);

	zassert_ok(led_off(0U));
	zassert_equal(level(0), 0);

	zassert_ok(led_toggle(0U));
	zassert_equal(level(0), 1);

	zassert_ok(led_toggle(0U));
	zassert_equal(level(0), 0);
}

ZTEST(led_mod, test_03_piscar_acende_e_apaga)
{
	zassert_ok(led_blink(0U, LED_PATTERN_FAST));
	zassert_true(seen_on_and_off(0U, CONFIG_BLINK_FAST_MS * 6), "nao piscou");
}

ZTEST(led_mod, test_04_rapido_troca_mais_que_lento)
{
	int fast;
	int slow;

	zassert_ok(led_blink(0U, LED_PATTERN_FAST));
	zassert_ok(led_blink(1U, LED_PATTERN_SLOW));

	count_transitions(CONFIG_BLINK_SLOW_MS * 3, &fast, &slow);

	zassert_true(slow > 0, "o lento nem trocou");
	zassert_true(fast > slow, "rapido=%d lento=%d", fast, slow);
}

ZTEST(led_mod, test_05_ligar_desligar_e_alternar_cancelam_o_pisca)
{
	zassert_ok(led_blink(0U, LED_PATTERN_FAST));
	zassert_ok(led_on(0U));
	k_msleep(CONFIG_BLINK_FAST_MS * 5);
	zassert_equal(level(0), 1, "led_on nao cancelou o pisca");

	zassert_ok(led_blink(0U, LED_PATTERN_FAST));
	zassert_ok(led_off(0U));
	k_msleep(CONFIG_BLINK_FAST_MS * 5);
	zassert_equal(level(0), 0, "led_off nao cancelou o pisca");

	zassert_ok(led_blink(0U, LED_PATTERN_FAST));
	zassert_ok(led_toggle(0U));
	{
		int fixed = level(0);

		k_msleep(CONFIG_BLINK_FAST_MS * 5);
		zassert_equal(level(0), fixed, "led_toggle nao cancelou o pisca");
	}
}

ZTEST(led_mod, test_06_acao_em_todos_afeta_todos)
{
	size_t i;

	zassert_ok(led_on(LED_ALL));
	for (i = 0U; i < led_count(); i++) {
		zassert_equal(level(i), 1, "LED %zu nao acendeu", i);
	}

	zassert_ok(led_off(LED_ALL));
	for (i = 0U; i < led_count(); i++) {
		zassert_equal(level(i), 0, "LED %zu nao apagou", i);
	}

	zassert_ok(led_blink(LED_ALL, LED_PATTERN_FAST));
	for (i = 0U; i < led_count(); i++) {
		zassert_true(seen_on_and_off(i, CONFIG_BLINK_FAST_MS * 6), "LED %zu nao piscou", i);
	}
}

ZTEST(led_mod, test_07_argumentos_invalidos_retornam_erro)
{
	zassert_equal(led_on(led_count()), -EINVAL);
	zassert_equal(led_off(led_count()), -EINVAL);
	zassert_equal(led_toggle(led_count()), -EINVAL);
	zassert_equal(led_blink(led_count(), LED_PATTERN_FAST), -EINVAL);
	zassert_equal(led_blink(0U, LED_PATTERN_COUNT), -EINVAL);
}

ZTEST(led_mod, test_08_mexer_num_led_nao_altera_o_outro)
{
	zassert_ok(led_blink(1U, LED_PATTERN_FAST));

	zassert_ok(led_on(0U));
	zassert_ok(led_off(0U));
	zassert_ok(led_blink(0U, LED_PATTERN_SLOW));

	zassert_true(seen_on_and_off(1U, CONFIG_BLINK_FAST_MS * 6), "LED 1 parou de piscar");
}

ZTEST(led_mod, test_09_reinicializar_nao_corrompe_o_timer)
{
	zassert_ok(led_blink(0U, LED_PATTERN_FAST));
	k_msleep(CONFIG_BLINK_FAST_MS * 2);

	zassert_ok(led_init());
	zassert_equal(level(0), 0);

	k_msleep(CONFIG_BLINK_FAST_MS * 5);
	zassert_equal(level(0), 0, "pisca sobreviveu ao reinit");

	zassert_ok(led_blink(2U, LED_PATTERN_FAST));
	zassert_true(seen_on_and_off(2U, CONFIG_BLINK_FAST_MS * 6), "timer nao voltou");
}
