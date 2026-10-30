/*******************************************************************
 * @file line.h
 *
 * @brief Montagem da linha de comando byte a byte.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/kernel.h>

typedef enum {
	LINE_EVENT_NONE = 0,
	LINE_EVENT_ECHO,
	LINE_EVENT_ERASE,
	LINE_EVENT_READY,
	LINE_EVENT_OVERFLOW,
} line_event_t;

typedef struct {
	char buf[CONFIG_MINISHELL_LINE_SIZE + 1];
	size_t len;
	bool discarding;
} line_t;

void line_reset(line_t *line);
line_event_t line_feed(line_t *line, char c);
const char *line_str(const line_t *line);

#endif /* LINE_H_ */
