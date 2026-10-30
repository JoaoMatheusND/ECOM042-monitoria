/*******************************************************************
 * @file board_io_stub.c
 *
 * @brief Stub de board_io para testar o despacho sem GPIO.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>

#include "board_io.h"
#include "board_io_stub.h"

static bool led_is_on;
static int button_state;
static bool should_fail;

void board_io_stub_reset(void)
{
	led_is_on = false;
	button_state = 0;
	should_fail = false;
}

void board_io_stub_set_button(int state)
{
	button_state = state;
}

void board_io_stub_fail(bool fail)
{
	should_fail = fail;
}

int board_io_init(void)
{
	board_io_stub_reset();

	return 0;
}

int board_io_led_set(bool on)
{
	if (should_fail) {
		return -EIO;
	}

	led_is_on = on;

	return 0;
}

bool board_io_led_get(void)
{
	return led_is_on;
}

int board_io_button_get(void)
{
	return should_fail ? -EIO : button_state;
}
