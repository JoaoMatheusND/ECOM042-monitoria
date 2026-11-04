/**
 * @file test_led.c
 *
 * @brief LED driver unit tests, with the GPIO controller ready.
 *
 * Pin levels are read from the GPIO emulator. Pin N is LED N, following
 * the order of declaration in app.overlay.
 */
#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "led.h"

#define GPIO_DEV DEVICE_DT_GET(DT_NODELABEL(test_gpio))
#define NUM_LEDS DT_CHILD_NUM_STATUS_OKAY(DT_PATH(leds))

#define SAMPLE_STEP_MS 5

/* What was observed on a pin while it was watched. */
struct pin_stats {
  int highs; /* samples with the pin on */
  int lows;  /* samples with the pin off */
  int edges; /* level changes */
};

static int pin_level(int led) { return gpio_emul_output_get(GPIO_DEV, led); }

static void assert_all_pins(int level) {
  for (int i = 0; i < NUM_LEDS; i++) {
    zassert_equal(pin_level(i), level, "LED %d: unexpected pin level", i);
  }
}

/* Sample the pins of @p count LEDs together for @p duration_ms. */
static void watch_pins(const int *leds, struct pin_stats *stats, size_t count,
                       int duration_ms) {
  int last[count];

  for (size_t i = 0; i < count; i++) {
    stats[i] = (struct pin_stats){0};
    last[i] = pin_level(leds[i]);
  }

  for (int t = 0; t < duration_ms; t += SAMPLE_STEP_MS) {
    for (size_t i = 0; i < count; i++) {
      int level = pin_level(leds[i]);

      stats[i].highs += (level != 0);
      stats[i].lows += (level == 0);
      stats[i].edges += (level != last[i]);
      last[i] = level;
    }

    k_msleep(SAMPLE_STEP_MS);
  }
}

static void led_before(void *fixture) {
  ARG_UNUSED(fixture);

  /* Also stops any blinking left over from the previous test. */
  zassert_ok(led_init());
}

ZTEST_SUITE(led, NULL, NULL, led_before, NULL, NULL);

ZTEST(led, test_init_ok_all_off) {
  led_on(LED_ALL);

  zassert_ok(led_init());
  assert_all_pins(0);
}

ZTEST(led, test_on_off_toggle) {
  zassert_ok(led_on(0));
  zassert_equal(pin_level(0), 1);

  zassert_ok(led_off(0));
  zassert_equal(pin_level(0), 0);

  zassert_ok(led_toggle(0));
  zassert_equal(pin_level(0), 1);

  zassert_ok(led_toggle(0));
  zassert_equal(pin_level(0), 0);
}

ZTEST(led, test_blink_toggles_pin) {
  const int leds[] = {0};
  struct pin_stats stats[1];

  zassert_ok(led_blink(0, LED_PROFILE_FAST));

  watch_pins(leds, stats, 1, 3 * CONFIG_LED_PERIOD_FAST_MS);

  zassert_true(stats[0].highs > 0, "pin was never on");
  zassert_true(stats[0].lows > 0, "pin was never off");
}

ZTEST(led, test_blink_independent_rates) {
  const int leds[] = {0, 1};
  struct pin_stats stats[2];

  zassert_ok(led_blink(0, LED_PROFILE_SLOW));
  zassert_ok(led_blink(1, LED_PROFILE_FAST));

  watch_pins(leds, stats, 2, 2 * CONFIG_LED_PERIOD_SLOW_MS);

  zassert_true(stats[1].edges > stats[0].edges,
               "fast LED toggled %d times, slow LED %d times", stats[1].edges,
               stats[0].edges);
}

/* Actions that must cancel a blink in progress. */
static int (*const cancel_actions[])(int) = {led_on, led_off, led_toggle};

ZTEST(led, test_actions_cancel_blink) {
  const int leds[] = {0};
  struct pin_stats stats[1];

  for (size_t i = 0; i < ARRAY_SIZE(cancel_actions); i++) {
    zassert_ok(led_blink(0, LED_PROFILE_FAST));
    k_msleep(CONFIG_LED_PERIOD_FAST_MS);

    zassert_ok(cancel_actions[i](0));

    watch_pins(leds, stats, 1, 2 * CONFIG_LED_PERIOD_FAST_MS);

    zassert_equal(stats[0].edges, 0, "action %zu did not cancel the blink", i);
  }
}

ZTEST(led, test_apply_to_all) {
  zassert_ok(led_on(LED_ALL));
  assert_all_pins(1);

  zassert_ok(led_toggle(LED_ALL));
  assert_all_pins(0);
}

ZTEST(led, test_invalid_arguments) {
  zassert_equal(led_on(NUM_LEDS), -EINVAL);
  zassert_equal(led_off(-2), -EINVAL);
  zassert_equal(led_blink(NUM_LEDS, LED_PROFILE_SLOW), -EINVAL);
  zassert_equal(led_blink(0, LED_PROFILE_COUNT), -EINVAL);
}