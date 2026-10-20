#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>

/** @brief Resultado da montagem de uma linha. */
enum line_result {
	LINE_WAIT,     /**< Aguarde mais bytes. */
	LINE_READY,    /**< Linha completa disponivel em text. */
	LINE_TOO_LONG, /**< Linha descartada por excesso de bytes ou NUL. */
};

/** @brief Estado da linha; inicialize com zero antes do primeiro byte. */
struct line_buffer {
	char text[CONFIG_MINI_SHELL_LINE_SIZE + 1]; /**< Conteudo e espaco para NUL. */
	size_t length;                              /**< Quantidade de bytes acumulados. */
	bool discard;                               /**< Descarte bytes ate o proximo CR ou LF. */
	bool after_cr;                              /**< Ignore o LF imediatamente depois de CR. */
};

/**
 * @brief Acrescente um byte e trate terminadores e backspace.
 * @param[in,out] line Estado valido, nao nulo, mantido pelo chamador.
 * @param byte Byte recebido.
 * @return Estado da montagem. Consuma text em LINE_READY antes do proximo byte.
 */
enum line_result line_feed(struct line_buffer *line, unsigned char byte);

#endif
