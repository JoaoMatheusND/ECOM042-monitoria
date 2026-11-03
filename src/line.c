/*******************************************************************
 * @file line.c
 *
 * @brief Montagem de linha de comando (eco, backspace, overflow).
 *******************************************************************/

#include "line.h"

#include <string.h>

#define KEY_BS  0x08
#define KEY_DEL 0x7F

void line_init(struct line_editor *ed)
{
	memset(ed, 0, sizeof(*ed));
}

static void line_reset(struct line_editor *ed)
{
	ed->len = 0;
	ed->overflow = false;
}

enum line_status line_feed(struct line_editor *ed, uint8_t c, char *echo, size_t *echo_len)
{
	bool after_cr = ed->last_cr;

	*echo_len = 0;
	ed->last_cr = (c == '\r');

	if (c == '\r' || c == '\n') {
		/* "\r\n" conta como um unico fim de linha */
		if (c == '\n' && after_cr) {
			return LINE_PENDING;
		}

		echo[0] = '\r';
		echo[1] = '\n';
		*echo_len = 2;

		if (ed->overflow) {
			line_reset(ed);
			return LINE_TOO_LONG;
		}

		ed->buf[ed->len] = '\0';
		ed->len = 0;
		return LINE_READY;
	}

	if (c == KEY_BS || c == KEY_DEL) {
		if (!ed->overflow && ed->len > 0) {
			ed->len--;
			memcpy(echo, "\b \b", 3);
			*echo_len = 3;
		}
		return LINE_PENDING;
	}

	echo[0] = (char)c;
	*echo_len = 1;

	if (ed->overflow) {
		return LINE_PENDING;
	}

	/* reserva um byte para o '\0' */
	if (ed->len >= sizeof(ed->buf) - 1) {
		ed->overflow = true;
		return LINE_PENDING;
	}

	ed->buf[ed->len++] = (char)c;
	return LINE_PENDING;
}
