#ifndef SHELL_CMD_H_
#define SHELL_CMD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SHELL_CMD_MAX_ARGS 8

struct shell_hw_ops {
	int (*led_set)(void *user, bool on); /* 0 ok, <0 erro */
	int (*led_toggle)(void *user);       /* 0 ok, <0 erro */
	int (*led_get)(void *user);          /* 1 ligado, 0 desligado, <0 erro */
	int (*button_get)(void *user);       /* 1 pressionado, 0 solto, <0 erro */
	int64_t (*uptime_ms)(void *user);
	void *user;
};

struct shell_out {
	char *buf;
	size_t size;
	size_t len;
};

void shell_out_printf(struct shell_out *out, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

typedef void (*shell_cmd_fn)(const struct shell_hw_ops *hw, int argc, char *argv[],
							 struct shell_out *out);

struct shell_cmd {
	const char *name;  /* comparado sem diferenciar maiusculas */
	const char *usage; /* mostrado no HELP */
	const char *help;
	shell_cmd_fn fn;
};

size_t shell_cmd_execute(const struct shell_hw_ops *hw, char *line, char *out_buf, size_t out_size);

#endif
