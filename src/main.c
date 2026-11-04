#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "leds.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const enum leds_pattern demo[] = {
	LEDS_PATTERN_SLOW,
	LEDS_PATTERN_NORMAL,
	LEDS_PATTERN_FAST,
};

int main(void)
{
	int ret = leds_init();

	if (ret != 0) {
		LOG_ERR("leds_init falhou (%d)", ret);
		return ret;
	}

	LOG_INF("%zu LED(s) em %s", leds_count(), CONFIG_BOARD);

	for (size_t i = 0; i < MIN(leds_count(), ARRAY_SIZE(demo)); i++) {
		ret = leds_blink(i, demo[i]);
		if (ret != 0) {
			LOG_ERR("leds_blink(%zu) falhou (%d)", i, ret);
		}
	}

	/* Mostra que ligar cancela o pisca só daquele LED. */
	k_sleep(K_SECONDS(10));
	LOG_INF("LED 0 aceso fixo; os demais continuam piscando");
	(void)leds_on(0);

	/* Mostra a ação em todos de uma vez. */
	k_sleep(K_SECONDS(10));
	LOG_INF("Todos desligados");
	(void)leds_apply_all(LEDS_CMD_OFF, 0);

	return 0;
}
