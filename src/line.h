/*******************************************************************
 * @file line.h
 *
 * @brief Montagem de linha de comando (eco, backspace, overflow).
 *
 * Logica pura, sem UART: recebe um byte por vez e informa o que
 * deve ser ecoado e quando uma linha esta completa.
 *******************************************************************/

#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LINE_BUF_SIZE CONFIG_MINISHELL_LINE_BUF_SIZE

/** Maior eco gerado por um unico byte ("\b \b"). */
#define LINE_ECHO_MAX 3

enum line_status {
	LINE_PENDING,  /**< Linha ainda em construcao. */
	LINE_READY,    /**< Linha completa em line_editor.buf (pode ser vazia). */
	LINE_TOO_LONG, /**< Linha excedeu o buffer e foi descartada. */
};

struct line_editor {
	char buf[LINE_BUF_SIZE];
	size_t len;
	bool overflow;
	bool last_cr;
};

void line_init(struct line_editor *ed);

/**
 * @brief Processa um byte recebido.
 *
 * @param ed       Editor de linha.
 * @param c        Byte recebido.
 * @param echo     Saida: bytes a ecoar (ate LINE_ECHO_MAX).
 * @param echo_len Saida: quantidade de bytes em @p echo.
 *
 * @return Estado da linha apos o byte.
 */
enum line_status line_feed(struct line_editor *ed, uint8_t c, char *echo, size_t *echo_len);

#endif /* LINE_H_ */
