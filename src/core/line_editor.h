/* Montagem de linha byte a byte. Modulo puro: sem UART/GPIO/printk/Kconfig. */
#ifndef LINE_EDITOR_H_
#define LINE_EDITOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum line_ev {
	LINE_EV_NONE,     /* nada a fazer */
	LINE_EV_ECHO,     /* caractere aceito: ecoar o byte recebido */
	LINE_EV_ERASE,    /* backspace valido: ecoar "\b \b" */
	LINE_EV_LINE,     /* linha completa em buf (terminada em '\0') */
	LINE_EV_TOO_LONG, /* linha estourou o buffer e foi descartada */
};

struct line_editor {
	char *buf;
	size_t size; /* inclui o '\0': maximo de (size - 1) caracteres */
	size_t len;
	bool overflow;
	bool prev_cr;
};

void line_editor_init(struct line_editor *le, char *buf, size_t size);
enum line_ev line_editor_feed(struct line_editor *le, uint8_t c);

#endif
