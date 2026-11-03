/* LED e botao obtidos pelo devicetree (aliases shell-led / shell-btn). */
#include "board_io.h"

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_MINISH_SIM_BTN)
#include <zephyr/drivers/gpio/gpio_emul.h>
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(shell_led), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(shell_btn), gpios);

static bool led_state;

#if defined(CONFIG_MINISH_SIM_BTN)
static void sim_btn_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sim_btn_work, sim_btn_handler);

/* No native_sim nao ha botao fisico: alterna o pino emulado periodicamente.
 * (roda na system workqueue, nao em ISR, pois gpio_emul usa mutex) */
static void sim_btn_handler(struct k_work *work)
{
	static int level;

	ARG_UNUSED(work);
	level = !level;
	gpio_emul_input_set(btn.port, btn.pin, level);
	k_work_reschedule(&sim_btn_work, K_MSEC(CONFIG_MINISH_SIM_BTN_PERIOD_MS));
}
#endif

static int op_led_set(bool on)
{
	int rc = gpio_pin_set_dt(&led, on ? 1 : 0);

	if (rc == 0) {
		led_state = on;
	}
	return rc;
}

static int op_led_get(bool *on)
{
	*on = led_state;
	return 0;
}

static int op_btn_get(bool *pressed)
{
	int v = gpio_pin_get_dt(&btn);

	if (v < 0) {
		return v;
	}
	*pressed = (v != 0);
	return 0;
}

static const struct minish_hw_ops ops = {
	.led_set = op_led_set,
	.led_get = op_led_get,
	.btn_get = op_btn_get,
};

const struct minish_hw_ops *board_io_ops(void)
{
	return &ops;
}

int board_io_init(void)
{
	int rc;

	if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&btn)) {
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (rc != 0) {
		return rc;
	}
	rc = gpio_pin_configure_dt(&btn, GPIO_INPUT);
	if (rc != 0) {
		return rc;
	}
	led_state = false;
#if defined(CONFIG_MINISH_SIM_BTN)
	k_work_reschedule(&sim_btn_work, K_MSEC(CONFIG_MINISH_SIM_BTN_PERIOD_MS));
#endif
	return 0;
}
