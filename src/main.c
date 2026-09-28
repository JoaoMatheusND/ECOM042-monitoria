#include <stddef.h>

#include "source.h"

int main(void)
{
	source_send("LED", "LED_ON", NULL);
	source_send("BTN", "BTN_PRESS", NULL);
	source_send("BOARD", "STATUS", NULL);
	source_send("BTN", "BTN_RELEASE", NULL);
	source_send("LED", "LED_OFF", NULL);
	source_send("BOARD", "STATUS", NULL);
	source_send("UART", "RESET", NULL);
	source_send("I2C", "FOO_BAR", NULL);

	return 0;
}
