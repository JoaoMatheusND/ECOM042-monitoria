#include "line_editor.h"

void line_editor_init(struct line_editor *le)
{
	le->len = 0;
	le->overflow = false;
	le->done = false;
	le->last_cr = false;
	le->buf[0] = '\0';
}

const char *line_editor_line(const struct line_editor *le)
{
	return le->buf;
}

enum le_event line_editor_feed(struct line_editor *le, uint8_t c)
{
	bool was_cr = le->last_cr;

	le->last_cr = false;

	if (le->done) {
		le->len = 0;
		le->overflow = false;
		le->done = false;
		le->buf[0] = '\0';
	}

	if (c == '\r' || c == '\n') {
		/* "\r\n" conta como um unico terminador */
		if (c == '\n' && was_cr && le->len == 0 && !le->overflow) {
			return LE_NONE;
		}
		le->last_cr = (c == '\r');
		le->done = true;
		if (le->overflow) {
			return LE_TOO_LONG;
		}
		le->buf[le->len] = '\0';
		return LE_LINE;
	}

	if (c == 0x08 || c == 0x7F) {
		if (le->overflow || le->len == 0) {
			return LE_NONE;
		}
		le->len--;
		le->buf[le->len] = '\0';
		return LE_BACKSPACE;
	}

	if (c < 0x20 || c > 0x7E) {
		return LE_NONE; /* ignora controle e bytes nao-ASCII */
	}

	if (le->overflow) {
		return LE_NONE;
	}
	if (le->len >= MINISH_LINE_MAX) {
		le->overflow = true; /* descarta a linha inteira */
		return LE_NONE;
	}

	le->buf[le->len++] = (char)c;
	le->buf[le->len] = '\0';
	return LE_ECHO;
}
