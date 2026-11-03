/*******************************************************************
 * @file line_editor.h
 *
 * @brief Montagem de linha pura: recebe bytes e devolve eventos.
 *
 * Nao conhece UART nem comandos, por isso e testavel em ztest sem
 * nenhum hardware.
 *******************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Eventos produzidos ao alimentar um byte. */
enum line_editor_event {
	/** Byte aceito: o chamador deve ecoar o caractere. */
	LINE_EDITOR_EVENT_ECHO,
	/** Backspace aceito: o chamador deve apagar o caractere na tela. */
	LINE_EDITOR_EVENT_ERASE,
	/** Byte descartado (controle, linha cheia ou Enter em linha vazia). */
	LINE_EDITOR_EVENT_IGNORED,
	/** Linha completa em line_editor_text(); chamar line_editor_reset(). */
	LINE_EDITOR_EVENT_LINE_READY,
	/** Linha passou do buffer e foi descartada por inteiro. */
	LINE_EDITOR_EVENT_LINE_TOO_LONG,
};

/** @brief Estado de uma linha em edicao. */
struct line_editor {
	char buf[CONFIG_MINISHELL_LINE_BUF_SIZE + 1];
	size_t len;
	bool overflow;
};

/** @brief Zera o editor (linha vazia, sem overflow). */
void line_editor_reset(struct line_editor *editor);

/**
 * @brief Consome um byte recebido.
 *
 * @param editor Editor em uso.
 * @param byte   Byte cru vindo da UART.
 *
 * @return Evento correspondente ao byte.
 */
enum line_editor_event line_editor_feed(struct line_editor *editor, char byte);

/** @brief Texto da linha atual (sempre terminado em '\0'). */
const char *line_editor_text(const struct line_editor *editor);

/** @brief Tamanho atual da linha. */
size_t line_editor_length(const struct line_editor *editor);

#ifdef __cplusplus
}
#endif
