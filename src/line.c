/*******************************************************************
 * @file line.c
 *
 * @brief Montagem de linha (buffer fixo, backspace, linha longa).
 *******************************************************************/

#include "line.h"

#define KEY_BS  0x08
#define KEY_DEL 0x7F

void line_init(struct line_buf *lb)
{
	lb->last = '\0';
	line_reset(lb);
}

void line_reset(struct line_buf *lb)
{
	lb->len = 0;
	lb->overflow = false;
	lb->buf[0] = '\0';
}

enum line_event line_feed(struct line_buf *lb, char c)
{
	char prev = lb->last;

	lb->last = c;

	if (c == '\r' || c == '\n') {
		/* "\r\n" conta como um único fim de linha. */
		if (c == '\n' && prev == '\r') {
			return LINE_NONE;
		}

		if (lb->overflow) {
			line_reset(lb);
			return LINE_TOO_LONG;
		}

		lb->buf[lb->len] = '\0';
		return LINE_READY;
	}

	if (c == KEY_BS || c == KEY_DEL) {
		if (lb->overflow || lb->len == 0) {
			return LINE_NONE;
		}

		lb->len--;
		lb->buf[lb->len] = '\0';
		return LINE_ERASE;
	}

	/* Só caracteres imprimíveis entram na linha. */
	if (c < ' ' || c > '~') {
		return LINE_NONE;
	}

	if (lb->overflow) {
		return LINE_CHAR;
	}

	/* Último byte do buffer é reservado para o '\0'. */
	if (lb->len >= sizeof(lb->buf) - 1) {
		lb->overflow = true;
		return LINE_CHAR;
	}

	lb->buf[lb->len++] = c;
	return LINE_CHAR;
}

const char *line_get(const struct line_buf *lb)
{
	return lb->buf;
}
