/*******************************************************************
 * @file line.h
 *
 * @brief Montagem de linha (buffer fixo, backspace, linha longa).
 *
 * Módulo puro: não conhece UART nem printk. Recebe um byte por vez e
 * diz ao chamador o que aconteceu (ecoar, apagar, linha pronta...).
 *******************************************************************/

#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>

enum line_event {
	LINE_NONE,     /* byte ignorado, nada a fazer */
	LINE_CHAR,     /* caractere aceito: ecoar */
	LINE_ERASE,    /* backspace apagou um caractere: apagar no terminal */
	LINE_READY,    /* fim de linha: line_get() tem a linha completa */
	LINE_TOO_LONG, /* fim de uma linha que estourou o buffer (descartada) */
};

struct line_buf {
	char buf[CONFIG_MINISHELL_LINE_SIZE];
	size_t len;
	bool overflow;
	char last;
};

void line_init(struct line_buf *lb);

/* Zera o buffer para começar uma nova linha. */
void line_reset(struct line_buf *lb);

/* Processa um byte recebido e retorna o evento correspondente. */
enum line_event line_feed(struct line_buf *lb, char c);

/* Linha atual terminada em '\0' (válida após LINE_READY). */
const char *line_get(const struct line_buf *lb);

#endif /* LINE_H_ */
