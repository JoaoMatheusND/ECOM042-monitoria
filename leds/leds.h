#ifndef LEDS_H_
#define LEDS_H_

#include <stddef.h>
#include <stdint.h>

#define LEDS_ALL SIZE_MAX

enum leds_pattern {
	LEDS_PATTERN_SLOW,
	LEDS_PATTERN_NORMAL,
	LEDS_PATTERN_FAST,
	LEDS_PATTERN_COUNT,
};

enum leds_cmd {
	LEDS_CMD_OFF,
	LEDS_CMD_ON,
	LEDS_CMD_TOGGLE,
	LEDS_CMD_BLINK,
	LEDS_CMD_COUNT,
};

int leds_init(void);

size_t leds_count(void);

int leds_apply(size_t led, enum leds_cmd cmd, uint32_t arg);

static inline int leds_apply_all(enum leds_cmd cmd, uint32_t arg)
{
	return leds_apply(LEDS_ALL, cmd, arg);
}

static inline int leds_on(size_t led)
{
	return leds_apply(led, LEDS_CMD_ON, 0);
}

static inline int leds_off(size_t led)
{
	return leds_apply(led, LEDS_CMD_OFF, 0);
}

static inline int leds_toggle(size_t led)
{
	return leds_apply(led, LEDS_CMD_TOGGLE, 0);
}

static inline int leds_blink(size_t led, enum leds_pattern pattern)
{
	return leds_apply(led, LEDS_CMD_BLINK, (uint32_t)pattern);
}

#endif /* LEDS_H_ */
