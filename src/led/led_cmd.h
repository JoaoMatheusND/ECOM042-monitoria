/**
 * @file led_cmd.h
 *
 * @brief LED command table (Command Pattern).
 *
 * Each action that can be applied to a LED has a handler. The handlers
 * live in led_cmd.c, next to a constant table indexed by enum led_action.
 * Handlers only change the pin and the LED state: arming the timer is up
 * to the caller.
 */
#ifndef LED_CMD_H_
#define LED_CMD_H_

#include "led_priv.h"

/** Actions that can be applied to a LED. Used as index of the command table. */
enum led_action {
	LED_ACT_ON,
	LED_ACT_OFF,
	LED_ACT_TOGGLE,
	LED_ACT_BLINK,
	LED_ACT_COUNT,
};

/**
 * @brief Command handler signature.
 *
 * @param led LED to act on.
 * @param arg Action argument (the blink profile for LED_ACT_BLINK, ignored
 * otherwise).
 * @retval 0 on success.
 * @retval -EINVAL if @p arg is invalid.
 * @return Other negative errno on GPIO failure.
 */
typedef int (*led_cmd_handler_t)(struct led_state *led, int arg);

/** A LED command. */
struct led_cmd {
	led_cmd_handler_t handler; /**<Command action implementation. */
};

/**
 * @brief Get the command table.
 *
 * @return Table with LED_ACT_COUNT entries, indexed by enum led_action.
 */
const struct led_cmd *led_cmd_table_get(void);

#endif /* LED_CMD_H_ */