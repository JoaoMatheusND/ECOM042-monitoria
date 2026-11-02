/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file shell_line.c
 *
 * @brief Editor de linha. Um byte por vez, sem saber de onde ele veio.
 *******************************************************************/

#include "shell_line.h"

#include <stdio.h>
#include <string.h>

void shell_line_init(struct shell_line *line, char *storage, size_t cap)
{
	line->buf = storage;
	line->cap = cap;
	line->len = 0;
	line->overflow = false;
	line->swallow = 0;
	if (storage != NULL && cap > 0) {
		storage[0] = '\0';
	}
}

static void set_echo(char *echo, size_t echo_cap, size_t *echo_len, const char *text)
{
	size_t n = 0;

	if (echo == NULL || echo_len == NULL || echo_cap == 0) {
		return;
	}

	while (text[n] != '\0' && (n + 1) < echo_cap) {
		echo[n] = text[n];
		n++;
	}
	echo[n] = '\0';
	*echo_len = n;
}

static void finish_line(struct shell_line *line, const struct shell_io *io, char *response,
			size_t response_cap)
{
	if (line->overflow) {
		snprintf(response, response_cap, "ERR: line too long");
	} else if (line->len > 0) {
		line->buf[line->len] = '\0';
		shell_dispatch(line->buf, io, response, response_cap);
	}

	line->len = 0;
	line->overflow = false;
	if (line->buf != NULL) {
		line->buf[0] = '\0';
	}
}

void shell_line_feed(struct shell_line *line, uint8_t byte, const struct shell_io *io, char *echo,
		     size_t echo_cap, size_t *echo_len, char *response, size_t response_cap,
		     bool *line_done)
{
	if (echo_len != NULL) {
		*echo_len = 0;
	}
	if (line_done != NULL) {
		*line_done = false;
	}
	if (response != NULL && response_cap > 0) {
		response[0] = '\0';
	}
	if (line == NULL || line_done == NULL || response == NULL || response_cap == 0) {
		return;
	}

	/* "HELP\r\n" e uma linha so. O segundo terminador de um par CR+LF
	 * nao pode disparar outro comando vazio. */
	if (line->swallow != 0) {
		uint8_t expected = line->swallow;

		line->swallow = 0;
		if (byte == expected) {
			return;
		}
	}

	if (byte == '\r' || byte == '\n') {
		line->swallow = (byte == '\r') ? '\n' : '\r';
		set_echo(echo, echo_cap, echo_len, "\r\n");
		finish_line(line, io, response, response_cap);
		*line_done = true;
		return;
	}

	if (byte == 0x08 || byte == 0x7F) {
		if (!line->overflow && line->len > 0) {
			line->len--;
			line->buf[line->len] = '\0';
			/* \b espaco \b apaga o caractere na tela do terminal. */
			set_echo(echo, echo_cap, echo_len, "\b \b");
		}
		return;
	}

	/* Outros controles (Ctrl+C, por exemplo) nao entram na linha. */
	if (byte < 0x20 && byte != '\t') {
		return;
	}

	if (line->overflow || line->len >= line->cap) {
		/* O byte que estoura ja condena a linha inteira. Backspace
		 * depois disso nao recupera o que foi digitado. */
		line->overflow = true;
		if (echo_cap > 1 && echo != NULL && echo_len != NULL) {
			echo[0] = (char)byte;
			echo[1] = '\0';
			*echo_len = 1;
		}
		return;
	}

	line->buf[line->len++] = (char)byte;
	line->buf[line->len] = '\0';
	if (echo_cap > 1 && echo != NULL && echo_len != NULL) {
		echo[0] = (char)byte;
		echo[1] = '\0';
		*echo_len = 1;
	}
}
