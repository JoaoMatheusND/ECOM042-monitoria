#include "command.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static bool word_equal(const char *text, size_t length, const char *word)
{
	if (length != strlen(word)) {
		return false;
	}

	for (size_t i = 0; i < length; i++) {
		if (toupper((unsigned char)text[i]) != toupper((unsigned char)word[i])) {
			return false;
		}
	}

	return true;
}

static bool argument_equal(const char *args, const char *word)
{
	size_t length;

	while (*args == ' ' || *args == '\t') {
		args++;
	}
	length = strlen(args);
	while (length > 0 && (args[length - 1] == ' ' || args[length - 1] == '\t')) {
		length--;
	}

	return word_equal(args, length, word);
}

static int help_execute(const char *args, const struct shell_io *io, char *out, size_t size)
{
	(void)io;
	if (!argument_equal(args, "")) {
		return snprintf(out, size, "ERR: unknown command");
	}

	return snprintf(out, size,
			"HELP\r\nLED ON\r\nLED OFF\r\nLED\r\nBTN\r\n"
			"UPTIME\r\nECHO <texto>");
}

static int led_execute(const char *args, const struct shell_io *io, char *out, size_t size)
{
	int ret;

	if (argument_equal(args, "")) {
		ret = io->led_get();
		if (ret < 0) {
			return snprintf(out, size, "ERR: GPIO");
		}
		return snprintf(out, size, "LED: %s", ret ? "ON" : "OFF");
	}

	if (!argument_equal(args, "ON") && !argument_equal(args, "OFF")) {
		return snprintf(out, size, "ERR: unknown command");
	}

	ret = io->led_set(argument_equal(args, "ON"));
	return snprintf(out, size, "%s", ret < 0 ? "ERR: GPIO" : "OK");
}

static int button_execute(const char *args, const struct shell_io *io, char *out, size_t size)
{
	int state;

	if (!argument_equal(args, "")) {
		return snprintf(out, size, "ERR: unknown command");
	}

	state = io->button_read();
	if (state < 0) {
		return snprintf(out, size, "ERR: GPIO");
	}
	return snprintf(out, size, "BTN: %d", state != 0);
}

static int uptime_execute(const char *args, const struct shell_io *io, char *out, size_t size)
{
	if (!argument_equal(args, "")) {
		return snprintf(out, size, "ERR: unknown command");
	}
	return snprintf(out, size, "UPTIME: %" PRId64 " ms", io->uptime_get());
}

static int echo_execute(const char *args, const struct shell_io *io, char *out, size_t size)
{
	(void)io;
	return snprintf(out, size, "%s", args);
}

struct command {
	const char *name;
	int (*execute)(const char *args, const struct shell_io *io, char *out, size_t size);
};

static const struct command commands[] = {
	{"HELP", help_execute},     {"LED", led_execute},   {"BTN", button_execute},
	{"UPTIME", uptime_execute}, {"ECHO", echo_execute},
};

bool shell_io_valid(const struct shell_io *io)
{
	return io != NULL && io->led_set != NULL && io->led_get != NULL &&
	       io->button_read != NULL && io->uptime_get != NULL;
}

int command_dispatch(const char *line, const struct shell_io *io, char *response, size_t capacity)
{
	size_t length;
	const char *args;
	int ret;

	if (line == NULL || !shell_io_valid(io) || response == NULL || capacity == 0) {
		return -EINVAL;
	}

	while (*line == ' ' || *line == '\t') {
		line++;
	}
	length = strcspn(line, " \t");
	args = line + length;
	if (*args != '\0') {
		args++;
	}

	/* O dispatcher so conhece o nome e o ponteiro da funcao. */
	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		if (word_equal(line, length, commands[i].name)) {
			if (commands[i].execute == NULL) {
				return -EINVAL;
			}
			ret = commands[i].execute(args, io, response, capacity);
			return ret < 0 || (size_t)ret >= capacity ? -ENOSPC : 0;
		}
	}

	ret = snprintf(response, capacity, "ERR: unknown command");
	return ret < 0 || (size_t)ret >= capacity ? -ENOSPC : 0;
}
