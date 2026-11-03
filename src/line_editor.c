/*******************************************************************
 * @file line_editor.c
 *
 * @brief Montagem de linha: echo, backspace e limite de 64 bytes.
 *******************************************************************/

#include "line_editor.h"

#include <string.h>

void line_editor_reset(struct line_editor *editor)
{
	memset(editor, 0, sizeof(*editor));
}

enum line_editor_event line_editor_feed(struct line_editor *editor, char byte)
{
	if ((byte == '\r') || (byte == '\n')) {
		if (editor->overflow) {
			line_editor_reset(editor);
			return LINE_EDITOR_EVENT_LINE_TOO_LONG;
		}

		if (editor->len == 0U) {
			return LINE_EDITOR_EVENT_IGNORED;
		}

		return LINE_EDITOR_EVENT_LINE_READY;
	}

	if ((byte == 0x08) || (byte == 0x7F)) {
		if (editor->overflow || (editor->len == 0U)) {
			return LINE_EDITOR_EVENT_IGNORED;
		}

		editor->len--;
		editor->buf[editor->len] = '\0';
		return LINE_EDITOR_EVENT_ERASE;
	}

	if ((byte < 0x20) || (byte > 0x7E)) {
		return LINE_EDITOR_EVENT_IGNORED;
	}

	if (editor->overflow) {
		return LINE_EDITOR_EVENT_IGNORED;
	}

	if (editor->len >= CONFIG_MINISHELL_LINE_BUF_SIZE) {
		editor->overflow = true;
		return LINE_EDITOR_EVENT_IGNORED;
	}

	editor->buf[editor->len] = byte;
	editor->len++;
	editor->buf[editor->len] = '\0';

	return LINE_EDITOR_EVENT_ECHO;
}

const char *line_editor_text(const struct line_editor *editor)
{
	return editor->buf;
}

size_t line_editor_length(const struct line_editor *editor)
{
	return editor->len;
}
