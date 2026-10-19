#ifndef COMMAND_H_
#define COMMAND_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct shell_io {
	int (*led_set)(bool on);
	int (*led_get)(void);
	int (*button_read)(void);
	int64_t (*uptime_get)(void);
};

/* Escreve a resposta sem CR/LF; o transporte adiciona a terminacao. */
int command_dispatch(const char *line, const struct shell_io *io, char *response, size_t capacity);

#endif