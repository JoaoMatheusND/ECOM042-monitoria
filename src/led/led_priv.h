/**
 * @file led_priv.h
 *
 * @brief Private LED state shared by led.c and led_cmd.c.
 *
 * Not part of the public API.
 */
#ifndef LED_PRIV_H_
#define LED_PRIV_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/gpio.h>

#include "led.h"

/** Per-LED state. One instance per LED, indexed by LED number. */
struct led_state {
  struct gpio_dt_spec gpio; /**< Pin taken from the devicetree. */
  enum led_profile profile; /**< Current blink profile (valid if blinking). */
  uint32_t period_ms;       /**< Full blink cycle time in ms. */
  int64_t deadline_ms;      /**< Uptime (ms) of the next toggle. */
  bool blinking;            /**< True while the LED is blinking. */
};

/* A blink period is a full cycle: the LED changes level twice per period. */
#define LED_TOGGLES_PER_PERIOD 2

/** Time between two level changes of a blinking LED, in ms. */
static inline uint32_t led_half_period_ms(const struct led_state *led) {
  return led->period_ms / LED_TOGGLES_PER_PERIOD;
}

#endif /* LED_PRIV_H_ */