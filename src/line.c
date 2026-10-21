#include "line.h"

enum line_result line_feed(struct line_buffer *line, unsigned char byte)
{
	enum line_result result;

	if (line->after_cr && byte == '\n') {
		line->after_cr = false;
		return LINE_WAIT;
	}
	line->after_cr = false;

	if (byte == '\r' || byte == '\n') {
		line->text[line->length] = '\0';
		result = line->discard ? LINE_TOO_LONG : LINE_READY;
		line->length = 0;
		line->discard = false;
		line->after_cr = byte == '\r';
		return result;
	}

	/* Depois do limite, descarta tudo ate o fim da linha. */
	if (line->discard) {
		return LINE_WAIT;
	}
	if (byte == 0x08 || byte == 0x7f) {
		if (line->length > 0) {
			line->length--;
		}
		return LINE_WAIT;
	}
	if (line->length == CONFIG_MINI_SHELL_LINE_SIZE || byte == '\0') {
		line->discard = true;
		return LINE_WAIT;
	}

	line->text[line->length++] = (char)byte;
	return LINE_WAIT;
}