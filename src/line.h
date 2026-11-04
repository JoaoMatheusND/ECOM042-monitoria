/*******************************************************************
 * @file line.h
 *
 * @brief Editor de linha: monta a linha byte a byte.
 *******************************************************************/

#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>

enum line_event {
	LINE_EV_NONE,     /* byte ignorado, nada a ecoar */
	LINE_EV_CHAR,     /* caractere aceito, ecoar o byte */
	LINE_EV_ERASE,    /* backspace apagou um caractere */
	LINE_EV_DONE,     /* linha completa em line_get() */
	LINE_EV_TOO_LONG, /* linha passou do limite e foi descartada */
};

struct line_editor {
	char buf[CONFIG_APP_LINE_BUF_SIZE + 1];
	size_t len;
	bool overflow;
	bool last_cr;
};

void line_init(struct line_editor *le);

/** Alimenta um byte recebido. */
enum line_event line_feed(struct line_editor *le, char c);

/** Linha terminada em '\0'; valida apos LINE_EV_DONE. */
const char *line_get(const struct line_editor *le);

/** Descarta a linha atual (chamar apos tratar LINE_EV_DONE). */
void line_reset(struct line_editor *le);

#endif /* LINE_H_ */
