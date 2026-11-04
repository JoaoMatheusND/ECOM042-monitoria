/*******************************************************************
 * @file line.c
 *
 * @brief Editor de linha com buffer fixo.
 *******************************************************************/

#include "line.h"

#define KEY_BS  0x08
#define KEY_DEL 0x7F

void line_init(struct line_editor *le)
{
	le->last_cr = false;
	line_reset(le);
}

void line_reset(struct line_editor *le)
{
	le->len = 0;
	le->overflow = false;
	le->buf[0] = '\0';
}

const char *line_get(const struct line_editor *le)
{
	return le->buf;
}

enum line_event line_feed(struct line_editor *le, char c)
{
	bool after_cr = le->last_cr;

	le->last_cr = (c == '\r');

	if (c == '\r' || c == '\n') {
		/* "\r\n" conta como um terminador so. */
		if (c == '\n' && after_cr) {
			return LINE_EV_NONE;
		}
		if (le->overflow) {
			line_reset(le);
			return LINE_EV_TOO_LONG;
		}
		le->buf[le->len] = '\0';
		return LINE_EV_DONE;
	}

	if (c == KEY_BS || c == KEY_DEL) {
		/* Linha estourada ja esta perdida; backspace nao a recupera. */
		if (le->overflow || le->len == 0) {
			return LINE_EV_NONE;
		}
		le->len--;
		le->buf[le->len] = '\0';
		return LINE_EV_ERASE;
	}

	if (c < ' ' || c > '~') {
		return LINE_EV_NONE;
	}

	if (le->len >= CONFIG_APP_LINE_BUF_SIZE) {
		le->overflow = true;
		return LINE_EV_CHAR;
	}
	if (!le->overflow) {
		le->buf[le->len++] = c;
	}
	return LINE_EV_CHAR;
}
