/*******************************************************************
 * @file cmd.c
 *
 * @brief Parsing e despacho de comandos (tabela de function pointers).
 *******************************************************************/

#include "cmd.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef int (*cmd_handler_t)(const struct cmd_io *io, const char *args, char *out, size_t out_size);

struct cmd_entry {
	const char *name;
	const char *help;
	cmd_handler_t handler;
};

static int cmd_help(const struct cmd_io *io, const char *args, char *out, size_t out_size);
static int cmd_led(const struct cmd_io *io, const char *args, char *out, size_t out_size);
static int cmd_btn(const struct cmd_io *io, const char *args, char *out, size_t out_size);
static int cmd_uptime(const struct cmd_io *io, const char *args, char *out, size_t out_size);
static int cmd_echo(const struct cmd_io *io, const char *args, char *out, size_t out_size);

static const struct cmd_entry cmd_table[] = {
	{"HELP", "HELP - lista os comandos", cmd_help},
	{"LED", "LED [ON|OFF] - liga/desliga ou mostra o led0", cmd_led},
	{"BTN", "BTN - le o botao sw0", cmd_btn},
	{"UPTIME", "UPTIME - tempo desde o boot em ms", cmd_uptime},
	{"ECHO", "ECHO <texto> - devolve o texto", cmd_echo},
};

static bool str_ieq(const char *a, size_t a_len, const char *b)
{
	size_t b_len = strlen(b);

	if (a_len != b_len) {
		return false;
	}

	for (size_t i = 0; i < a_len; i++) {
		if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) {
			return false;
		}
	}

	return true;
}

/* Tira espacos do inicio e do fim de args; devolve o tamanho util. */
static size_t trim(const char **args)
{
	const char *s = *args;
	size_t len;

	while (*s == ' ' || *s == '\t') {
		s++;
	}

	len = strlen(s);
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) {
		len--;
	}

	*args = s;
	return len;
}

static int reply(char *out, size_t out_size, const char *text)
{
	int n = snprintf(out, out_size, "%s", text);

	return (n < 0 || (size_t)n >= out_size) ? -ENOMEM : 0;
}

static int unknown(char *out, size_t out_size)
{
	reply(out, out_size, "ERR: unknown command");
	return -EINVAL;
}

static int cmd_help(const struct cmd_io *io, const char *args, char *out, size_t out_size)
{
	size_t pos = 0;

	(void)io;

	if (trim(&args) != 0) {
		return unknown(out, out_size);
	}

	out[0] = '\0';
	for (size_t i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++) {
		int n = snprintf(out + pos, out_size - pos, "%s%s", (i > 0) ? "\r\n" : "",
				 cmd_table[i].help);

		if (n < 0 || (size_t)n >= out_size - pos) {
			return -ENOMEM;
		}
		pos += n;
	}

	return 0;
}

static int cmd_led(const struct cmd_io *io, const char *args, char *out, size_t out_size)
{
	size_t len = trim(&args);
	int ret;

	if (len == 0) {
		ret = io->led_get();
		if (ret < 0) {
			reply(out, out_size, "ERR: led");
			return ret;
		}
		return reply(out, out_size, ret ? "LED: ON" : "LED: OFF");
	}

	if (str_ieq(args, len, "ON")) {
		ret = io->led_set(true);
	} else if (str_ieq(args, len, "OFF")) {
		ret = io->led_set(false);
	} else {
		return unknown(out, out_size);
	}

	if (ret < 0) {
		reply(out, out_size, "ERR: led");
		return ret;
	}

	return reply(out, out_size, "OK");
}

static int cmd_btn(const struct cmd_io *io, const char *args, char *out, size_t out_size)
{
	int ret;

	if (trim(&args) != 0) {
		return unknown(out, out_size);
	}

	ret = io->btn_get();
	if (ret < 0) {
		reply(out, out_size, "ERR: btn");
		return ret;
	}

	return reply(out, out_size, ret ? "BTN: 1" : "BTN: 0");
}

static int cmd_uptime(const struct cmd_io *io, const char *args, char *out, size_t out_size)
{
	int n;

	if (trim(&args) != 0) {
		return unknown(out, out_size);
	}

	n = snprintf(out, out_size, "UPTIME: %lld ms", (long long)io->uptime_ms());

	return (n < 0 || (size_t)n >= out_size) ? -ENOMEM : 0;
}

static int cmd_echo(const struct cmd_io *io, const char *args, char *out, size_t out_size)
{
	(void)io;

	/* devolve o texto exatamente como recebido, sem trim */
	return reply(out, out_size, args);
}

int cmd_execute(const struct cmd_io *io, const char *line, char *out, size_t out_size)
{
	const char *name;
	const char *args;
	size_t name_len;

	if (out == NULL || out_size == 0) {
		return -EINVAL;
	}

	while (*line == ' ' || *line == '\t') {
		line++;
	}

	name = line;
	while (*line != '\0' && *line != ' ' && *line != '\t') {
		line++;
	}
	name_len = line - name;

	/* pula so o separador: ECHO precisa do resto intacto */
	args = (*line != '\0') ? line + 1 : line;

	for (size_t i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++) {
		if (str_ieq(name, name_len, cmd_table[i].name)) {
			return cmd_table[i].handler(io, args, out, out_size);
		}
	}

	return unknown(out, out_size);
}
