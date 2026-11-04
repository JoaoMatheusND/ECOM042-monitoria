/**
 * @file led.h
 *
 * @brief Public LED API.
 *
 * LEDs are addressed by index, following the order of declaration of the
 * "gpio-leds" children in the devicetree (the first one declared is 0).
 * Every function also accepts LED_ALL to act on all LEDs at once.
 * Callers know nothing about the timer used to blink the LEDs.
 */
#ifndef LED_H_
#define LED_H_

/** Special LED index: apply the operation to every LED. */
#define LED_ALL (-1)

/** Blink profiles. Periods are set through Kconfig (CONFIG_LED_PERIOD_*_MS). */
enum led_profile {
	LED_PROFILE_SLOW,
	LED_PROFILE_NORMAL,
	LED_PROFILE_FAST,
	LED_PROFILE_COUNT,
};

/**
 * @brief Initialize all LEDs as outputs, turned off.
 *
 * Safe to call again: it stops any blinking in progress.
 *
 * @retval 0 on success.
 * @retval -ENODEV if a GPIO controller is not ready.
 * @return Other negative errno if a pin cannot be configured.
 */
int led_init(void);

/**
 * @brief Turn a LED on, cancelling its blinking.
 *
 * @param led LED index or LED_ALL.
 * @retval 0 on success.
 * @retval -EINVAL if @p led is out of range.
 */
int led_on(int led);

/**
 * @brief Turn a LED off, cancelling its blinking.
 *
 * @param led LED index or LED_ALL.
 * @retval 0 on success.
 * @retval -EINVAL if @p led is out of range.
 */
int led_off(int led);

/**
 * @brief Toggle a LED, cancelling its blinking.
 *
 * @param led LED index or LED_ALL.
 * @retval 0 on success.
 * @retval -EINVAL if @p led is out of range.
 */
int led_toggle(int led);

/**
 * @brief Make a LED blink with the given profile.
 *
 * @param led     LED index or LED_ALL.
 * @param profile Blink profile.
 * @retval 0 on success.
 * @retval -EINVAL if @p led or @p profile is out of range.
 */
int led_blink(int led, enum led_profile profile);

#endif /* LED_H_ */
