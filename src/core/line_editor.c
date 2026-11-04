#include "line_editor.h"

void line_editor_init(struct line_editor *le, char *buf, size_t size)
{
	le->buf = buf;
	le->size = size;
	le->len = 0;
	le->overflow = false;
	le->prev_cr = false;
	if (buf != NULL && size > 0) {
		buf[0] = '\0';
	}
}

enum line_ev line_editor_feed(struct line_editor *le, uint8_t c)
{
	bool was_cr = le->prev_cr;

	le->prev_cr = (c == '\r');

	/* "\r\n" conta como um unico terminador */
	if (c == '\n' && was_cr) {
		return LINE_EV_NONE;
	}

	if (c == '\r' || c == '\n') {
		if (le->overflow) {
			le->overflow = false;
			le->len = 0;
			le->buf[0] = '\0';
			return LINE_EV_TOO_LONG;
		}
		le->buf[le->len] = '\0';
		le->len = 0; /* buf continua valido ate o proximo feed */
		return LINE_EV_LINE;
	}

	if (le->overflow) {
		return LINE_EV_NONE; /* descartando ate o terminador */
	}

	if (c == 0x08 || c == 0x7F) {
		if (le->len > 0) {
			le->len--;
			return LINE_EV_ERASE;
		}
		return LINE_EV_NONE;
	}

	if (c < 0x20 || c > 0x7E) {
		return LINE_EV_NONE; /* ignora controles / nao-ASCII */
	}

	if (le->len >= le->size - 1) {
		le->overflow = true;
		return LINE_EV_NONE;
	}

	le->buf[le->len++] = (char)c;
	return LINE_EV_ECHO;
}
