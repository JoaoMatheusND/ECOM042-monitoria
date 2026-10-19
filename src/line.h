#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>

enum line_result {
	LINE_WAIT,
	LINE_READY,
	LINE_TOO_LONG,
};

struct line_buffer {
	char text[CONFIG_MINI_SHELL_LINE_SIZE + 1];
	size_t length;
	bool discard;
	bool after_cr;
};

enum line_result line_feed(struct line_buffer *line, unsigned char byte);

#endif