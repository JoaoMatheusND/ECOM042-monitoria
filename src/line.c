/*******************************************************************
 * @file line.c
 *
 * @brief Montagem da linha de comando byte a byte.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <stdbool.h>
#include <stddef.h>

#include "line.h"

#define ASCII_BACKSPACE 0x08
#define ASCII_DELETE    0x7F

static bool is_terminator(char c)
{
	return (c == '\n') || (c == '\r');
}

static bool is_erase(char c)
{
	return (c == ASCII_BACKSPACE) || (c == ASCII_DELETE);
}

void line_reset(line_t *line)
{
	line->len = 0U;
	line->discarding = false;
	line->buf[0] = '\0';
}

line_event_t line_feed(line_t *line, char c)
{
	if (is_terminator(c)) {
		line->buf[line->len] = '\0';

		if (line->discarding) {
			line_reset(line);
			return LINE_EVENT_OVERFLOW;
		}

		return LINE_EVENT_READY;
	}

	/* linha estourada: ignora tudo ate o fim dela */
	if (line->discarding) {
		return LINE_EVENT_NONE;
	}

	if (is_erase(c)) {
		if (line->len == 0U) {
			return LINE_EVENT_NONE;
		}

		line->len--;
		line->buf[line->len] = '\0';

		return LINE_EVENT_ERASE;
	}

	if (line->len >= CONFIG_MINISHELL_LINE_SIZE) {
		line->discarding = true;
		return LINE_EVENT_NONE;
	}

	line->buf[line->len] = c;
	line->len++;
	line->buf[line->len] = '\0';

	return LINE_EVENT_ECHO;
}

const char *line_str(const line_t *line)
{
	return line->buf;
}
