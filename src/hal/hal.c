#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include "hal.h"

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

/* Kept here because not every GPIO driver reads back an output pin */
static bool led_on;

int hal_init(void) {
    int ret;

    if(!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&btn)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    if(ret < 0) {
        return ret;
    }

    led_on = false;

    return gpio_pin_configure_dt(&btn, GPIO_INPUT);
}

int hal_led_set(bool on) {
    int ret = gpio_pin_set_dt(&led, on);

    if(ret == 0) {
        led_on = on;
    }

    return ret;
}

bool hal_led_get(void) {
    return led_on;
}

int hal_btn_get(void) {
    return gpio_pin_get_dt(&btn);
}
