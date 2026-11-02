/**
 * @file hal.h
 * @brief Access to the board LED and button, taken from the devicetree.
 *
 * Uses the aliases @c led0 and @c sw0. The command handlers only know this
 * interface, not the GPIO API.
 */

#ifndef HAL_H_
#define HAL_H_

#include <stdbool.h>

/**
 * @brief Configure the LED (output, off) and the button (input).
 *
 * Must be called once, before any other function of this module.
 *
 * @retval 0        Ready.
 * @retval -ENODEV  A GPIO controller is not ready.
 * @retval -errno   The GPIO driver failed to configure a pin.
 */
int hal_init(void);

/**
 * @brief Turn the LED on or off.
 *
 * @param[in] on true to turn it on, false to turn it off.
 *
 * @retval 0       Done.
 * @retval -errno  The GPIO driver failed; the LED state is unchanged.
 */
int hal_led_set(bool on);

/**
 * @brief Get the state the LED was last set to.
 *
 * The state is kept by this module: not every GPIO driver reads back an
 * output pin.
 *
 * @return true if the LED is on, false if it is off.
 */
bool hal_led_get(void);

/**
 * @brief Read the button.
 *
 * @retval 1       Pressed.
 * @retval 0       Released.
 * @retval -errno  The GPIO driver failed.
 */
int hal_btn_get(void);

#endif
