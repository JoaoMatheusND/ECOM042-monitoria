/*
 * Testes unitários do módulo de LEDs (ztest + gpio-emul no native_sim).
 */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#include "leds.h"

#define N_LEDS 3

static const struct gpio_dt_spec pins[N_LEDS] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

/* Nível físico do pino (ativo em alto: 1 = LED ligado). */
static int level(size_t i)
{
	return gpio_emul_output_get_dt(&pins[i]);
}

/* Amostra o pino durante @p ms (tempo real do kernel) e conta as trocas. */
static unsigned int count_transitions(size_t i, int ms)
{
	unsigned int n = 0;
	int prev = level(i);
	int64_t end = k_uptime_get() + ms;

	while (k_uptime_get() < end) {
		k_msleep(1);

		int cur = level(i);

		if (cur != prev) {
			n++;
			prev = cur;
		}
	}

	return n;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(leds_init());
}

ZTEST_SUITE(leds, NULL, NULL, before_each, NULL, NULL);

ZTEST(leds, test_init_ok_all_off)
{
	zassert_equal(leds_count(), N_LEDS);

	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %zu deveria iniciar desligado", i);
	}
}

ZTEST(leds, test_reinit_stops_blink)
{
	zassert_ok(leds_blink(0, LEDS_PATTERN_FAST));
	k_msleep(CONFIG_APP_LEDS_FAST_MS / 2);

	zassert_ok(leds_init());
	zassert_equal(level(0), 0);
	zassert_equal(count_transitions(0, 4 * CONFIG_APP_LEDS_FAST_MS), 0);

	/* O timer continua funcional depois da reinicialização. */
	zassert_ok(leds_blink(0, LEDS_PATTERN_FAST));
	zassert_true(count_transitions(0, 4 * CONFIG_APP_LEDS_FAST_MS) > 0);
}

ZTEST(leds, test_on_off_toggle_change_pin)
{
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_ok(leds_on(i));
		zassert_equal(level(i), 1);

		zassert_ok(leds_off(i));
		zassert_equal(level(i), 0);

		zassert_ok(leds_toggle(i));
		zassert_equal(level(i), 1);

		zassert_ok(leds_toggle(i));
		zassert_equal(level(i), 0);
	}
}

ZTEST(leds, test_blink_observed_on_and_off)
{
	bool seen_on = false;
	bool seen_off = false;

	zassert_ok(leds_blink(0, LEDS_PATTERN_NORMAL));

	for (int t = 0; t < 3 * CONFIG_APP_LEDS_NORMAL_MS; t++) {
		int v = level(0);

		seen_on |= (v == 1);
		seen_off |= (v == 0);
		k_msleep(1);
	}

	zassert_true(seen_on, "pino nunca foi visto ligado");
	zassert_true(seen_off, "pino nunca foi visto desligado");
}

ZTEST(leds, test_two_leds_different_periods)
{
	zassert_ok(leds_blink(0, LEDS_PATTERN_SLOW));
	zassert_ok(leds_blink(1, LEDS_PATTERN_FAST));

	int window = 4 * CONFIG_APP_LEDS_SLOW_MS;
	unsigned int slow = 0;
	unsigned int fast = 0;
	int prev_slow = level(0);
	int prev_fast = level(1);

	/* Observa os dois no mesmo intervalo de tempo. */
	for (int t = 0; t < window; t++) {
		k_msleep(1);

		int s = level(0);
		int f = level(1);

		slow += (s != prev_slow);
		fast += (f != prev_fast);
		prev_slow = s;
		prev_fast = f;
	}

	zassert_true(slow > 0, "LED lento nao piscou");
	zassert_true(fast > slow, "rapido (%u) deveria trocar mais que lento (%u)", fast, slow);
}

ZTEST(leds, test_leds_are_independent)
{
	/* LED 0 pisca; mexer no LED 1 não pode alterar o período do LED 0. */
	zassert_ok(leds_blink(0, LEDS_PATTERN_NORMAL));
	k_msleep(CONFIG_APP_LEDS_NORMAL_MS / 3);

	zassert_ok(leds_blink(1, LEDS_PATTERN_FAST));
	k_msleep(CONFIG_APP_LEDS_FAST_MS * 2 + 7);
	zassert_ok(leds_blink(1, LEDS_PATTERN_SLOW));
	k_msleep(CONFIG_APP_LEDS_SLOW_MS / 2 + 3);
	zassert_ok(leds_off(1));

	int window = 10 * CONFIG_APP_LEDS_NORMAL_MS;
	unsigned int n = count_transitions(0, window);

	/* Esperado: ~10 trocas (tolerância de 1 por borda da janela). */
	zassert_between_inclusive(n, 9, 11, "LED 0 trocou %u vezes (esperado ~10)", n);
	zassert_equal(level(1), 0);
}

ZTEST(leds, test_on_off_toggle_cancel_blink)
{
	int (*const actions[])(size_t) = { leds_on, leds_off, leds_toggle };

	for (size_t a = 0; a < ARRAY_SIZE(actions); a++) {
		zassert_ok(leds_init());
		zassert_ok(leds_blink(0, LEDS_PATTERN_FAST));
		k_msleep(CONFIG_APP_LEDS_FAST_MS * 3 / 2);

		zassert_ok(actions[a](0));
		zassert_equal(count_transitions(0, 5 * CONFIG_APP_LEDS_FAST_MS), 0,
			      "acao %zu nao cancelou o pisca", a);
	}
}

ZTEST(leds, test_apply_all)
{
	zassert_ok(leds_apply_all(LEDS_CMD_ON, 0));
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 1, "LED %zu nao ligou", i);
	}

	zassert_ok(leds_apply_all(LEDS_CMD_OFF, 0));
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0, "LED %zu nao desligou", i);
	}

	zassert_ok(leds_apply_all(LEDS_CMD_TOGGLE, 0));
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 1, "LED %zu nao alternou", i);
	}

	/* LEDS_ALL também vale nos atalhos. */
	zassert_ok(leds_off(LEDS_ALL));
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0);
	}
}

ZTEST(leds, test_apply_all_blink_and_stop)
{
	zassert_ok(leds_apply_all(LEDS_CMD_BLINK, LEDS_PATTERN_FAST));

	bool seen_on[N_LEDS] = { false };
	bool seen_off[N_LEDS] = { false };

	for (int t = 0; t < 3 * CONFIG_APP_LEDS_FAST_MS; t++) {
		for (size_t i = 0; i < N_LEDS; i++) {
			int v = level(i);

			seen_on[i] |= (v == 1);
			seen_off[i] |= (v == 0);
		}
		k_msleep(1);
	}

	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_true(seen_on[i] && seen_off[i], "LED %zu nao piscou", i);
	}

	zassert_ok(leds_apply_all(LEDS_CMD_OFF, 0));
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(count_transitions(i, 3 * CONFIG_APP_LEDS_FAST_MS), 0);
		zassert_equal(level(i), 0);
	}
}

ZTEST(leds, test_invalid_arguments)
{
	/* LED fora do intervalo */
	zassert_equal(leds_on(N_LEDS), -EINVAL);
	zassert_equal(leds_off(N_LEDS + 10), -EINVAL);
	zassert_equal(leds_toggle(N_LEDS), -EINVAL);
	zassert_equal(leds_blink(N_LEDS, LEDS_PATTERN_SLOW), -EINVAL);

	/* Padrão fora do intervalo */
	zassert_equal(leds_blink(0, LEDS_PATTERN_COUNT), -EINVAL);
	zassert_equal(leds_blink(0, (enum leds_pattern)99), -EINVAL);
	zassert_equal(leds_apply_all(LEDS_CMD_BLINK, LEDS_PATTERN_COUNT), -EINVAL);

	/* Comando inválido */
	zassert_equal(leds_apply(0, LEDS_CMD_COUNT, 0), -EINVAL);

	/* Nada mudou nos pinos */
	for (size_t i = 0; i < N_LEDS; i++) {
		zassert_equal(level(i), 0);
	}
}
