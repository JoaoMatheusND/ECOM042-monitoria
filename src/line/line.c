#include "line.h"
#include <errno.h>

#define BACKSPACE 0x08
#define DELETE    0x7F

static bool is_printable(char c)
{
	unsigned char byte = (unsigned char)c;

	return byte >= 0x20 && byte <= 0x7E;
}

int line_feed(struct line *line, char c)
{

	if (line == NULL) {
		return -EINVAL;
	}

	if (line->line_terminated) {
		return 0;
	}

	if (c == '\r' || c == '\n') {
		/* Empty line, or the second byte of "\r\n" */
		if (line->len == 0) {
			return 0;
		}

		line->line_terminated = true;

		if (!line->overflow) {
			line->buf[line->len] = '\0';
		}

		return 0;
	}

	if (c == BACKSPACE || c == DELETE) {
		if (line->len > 0 && !line->overflow) {
			line->len--;
		}

		return 0;
	}

	if (!is_printable(c) || line->overflow) {
		return 0;
	}

	if (line->len == LINE_MAX_CHARS) {
		line->overflow = true;
		return 0;
	}

	line->buf[line->len] = c;
	line->len++;

	return 0;
}

void line_reset(struct line *line)
{
	line->len = 0;
	line->line_terminated = false;
	line->overflow = false;
}