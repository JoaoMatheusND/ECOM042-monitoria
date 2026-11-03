#ifndef SHELL_H
#define SHELL_H

#include <stddef.h>

int shell_dispatch(const char *line, char *out, size_t out_len);

#endif
