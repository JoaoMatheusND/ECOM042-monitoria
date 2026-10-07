#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

#include "board_io.h"

int main(void)
{
	int r = io_init();
	if (r) {
		return r;
	}

	const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
	static const int seq[] = {0, 1, 0, 1};

	for (int i = 0; i < ARRAY_SIZE(seq); i++) {
		r = gpio_emul_input_set_dt(&button, seq[i]);
		if (r) {
			return r;
		}

		int v = button_read();
		if (v < 0) {
			return v;
		}

		r = led_set(v);
		if (r) {
			return r;
		}

		printk("Button: %d -> LED: %d\n", v, v);
	}

	return 0;
}
