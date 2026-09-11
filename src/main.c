/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/kernel.h>

#include "traffic_light.h"

static const char *tl_state_name(enum tl_state state)
{
	switch (state) {
	case TL_RED:
		return "RED";
	case TL_GREEN:
		return "GREEN";
	case TL_YELLOW:
		return "YELLOW";
	default:
		return "?";
	}
}

int main(void)
{
	enum tl_state state = TL_RED;

	for (int i = 0; i < 4; i++) {
		state = tl_next_state(state);
		printk("%s\n", tl_state_name(state));
	}

	return 0;
}
