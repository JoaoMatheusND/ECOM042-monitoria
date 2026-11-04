/*
 * leds_init() deve falhar quando algum controlador de GPIO não está pronto.
 */
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "leds.h"

ZTEST_SUITE(leds_not_ready, NULL, NULL, NULL, NULL, NULL);

ZTEST(leds_not_ready, test_init_fails_when_controller_not_ready)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_gpio_deferred));

	zassert_false(device_is_ready(dev), "controlador deveria estar sem init");

	zassert_equal(leds_init(), -ENODEV);

	/* Sem init bem-sucedido, os comandos são recusados. */
	zassert_equal(leds_on(0), -ENODEV);

	/* Inicializando o controlador, leds_init() passa a funcionar. */
	zassert_ok(device_init(dev));
	zassert_ok(leds_init());
	zassert_ok(leds_on(0));
}
