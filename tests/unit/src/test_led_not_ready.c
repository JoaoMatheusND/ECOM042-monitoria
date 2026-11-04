/**
 * @file test_led_not_ready.c
 *
 * @brief LED driver unit test, with the GPIO controller not ready.
 */
#include <errno.h>

#include <zephyr/ztest.h>

#include "led.h"

ZTEST_SUITE(led_not_ready, NULL, NULL, NULL, NULL, NULL);

ZTEST(led_not_ready, test_init_fails_not_ready)
{
	zassert_equal(led_init(), -ENODEV, "init must fail if the controller is not ready");
}
