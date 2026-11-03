/* Editor de linha: puro, sem UART/GPIO/printk. */
#ifndef MINISH_LINE_EDITOR_H_
#define MINISH_LINE_EDITOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CONFIG_MINISH_LINE_MAX
#define CONFIG_MINISH_LINE_MAX 64
#endif
#define MINISH_LINE_MAX CONFIG_MINISH_LINE_MAX

#define MINISH_ERR_LINE_TOO_LONG "ERR: line too long"

enum le_event {
	LE_NONE,      /* nada a fazer (byte ignorado/descartado)            */
	LE_ECHO,      /* caractere aceito: ecoar o proprio byte             */
	LE_BACKSPACE, /* apagou 1 char: ecoar "\b \b"                       */
	LE_LINE,      /* linha completa em line_editor_line()               */
	LE_TOO_LONG,  /* terminador de uma linha que estourou o buffer      */
};

struct line_editor {
	char buf[MINISH_LINE_MAX + 1];
	size_t len;
	bool overflow;
	bool done;    /* ultima linha ja entregue; limpa no proximo byte    */
	bool last_cr; /* ultimo byte foi '\r' (para tratar CRLF como um)    */
};

void line_editor_init(struct line_editor *le);
enum le_event line_editor_feed(struct line_editor *le, uint8_t c);
const char *line_editor_line(const struct line_editor *le);

#endif
