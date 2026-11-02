/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file shell.c
 *
 * @brief Tabela de comandos (Command Pattern) e o texto de cada
 * resposta. Nenhum simbolo daqui abre a UART ou mexe num pino.
 *******************************************************************/

#include "shell.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static void reply(char *out, size_t out_len, const char *text)
{
	snprintf(out, out_len, "%s", text);
}

static const char *skip_ws(const char *text)
{
	while (*text == ' ' || *text == '\t') {
		text++;
	}
	return text;
}

/* Compara a palavra inteira, sem distinguir maiusculas. "LED" nao casa
 * com "LEDS" porque o caractere seguinte teria de ser fim ou espaco. */
static bool same_word(const char *line, const char *word, const char **rest)
{
	size_t i = 0;

	while (word[i] != '\0') {
		char left = line[i];
		char right = word[i];

		if (left >= 'a' && left <= 'z') {
			left = (char)(left - 'a' + 'A');
		}
		if (right >= 'a' && right <= 'z') {
			right = (char)(right - 'a' + 'A');
		}
		if (left != right) {
			return false;
		}
		i++;
	}

	if (line[i] != '\0' && line[i] != ' ' && line[i] != '\t') {
		return false;
	}

	*rest = line + i;
	return true;
}

static void cmd_help(const char *args, const struct shell_io *io, char *out, size_t out_len);
static void cmd_led(const char *args, const struct shell_io *io, char *out, size_t out_len);
static void cmd_btn(const char *args, const struct shell_io *io, char *out, size_t out_len);
static void cmd_uptime(const char *args, const struct shell_io *io, char *out, size_t out_len);
static void cmd_echo(const char *args, const struct shell_io *io, char *out, size_t out_len);

/* O despacho percorre esta tabela. Um comando novo e uma linha nova,
 * nao um if a mais no meio de shell_dispatch(). */
static const struct shell_command commands[] = {
	{"HELP", "HELP", cmd_help},
	{"LED", "LED ON\r\nLED OFF\r\nLED", cmd_led},
	{"BTN", "BTN", cmd_btn},
	{"UPTIME", "UPTIME", cmd_uptime},
	{"ECHO", "ECHO <texto>", cmd_echo},
};

static void cmd_help(const char *args, const struct shell_io *io, char *out, size_t out_len)
{
	size_t used = 0;
	size_t i;

	(void)io;

	if (*skip_ws(args) != '\0') {
		reply(out, out_len, "ERR: unknown command");
		return;
	}

	out[0] = '\0';
	for (i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
		int wrote;
		const char *prefix = (i == 0) ? "" : "\r\n";

		if (used >= out_len) {
			return;
		}

		wrote = snprintf(out + used, out_len - used, "%s%s", prefix, commands[i].usage);
		if (wrote < 0 || (size_t)wrote >= (out_len - used)) {
			return;
		}
		used += (size_t)wrote;
	}
}

static void cmd_led(const char *args, const struct shell_io *io, char *out, size_t out_len)
{
	const char *text = skip_ws(args);
	const char *rest;
	bool on;

	if (io == NULL || io->led_get == NULL || io->led_set == NULL) {
		reply(out, out_len, "ERR: led");
		return;
	}

	if (*text == '\0') {
		if (io->led_get(&on) < 0) {
			reply(out, out_len, "ERR: led");
			return;
		}
		snprintf(out, out_len, "LED: %s", on ? "ON" : "OFF");
		return;
	}

	if (same_word(text, "ON", &rest) && *skip_ws(rest) == '\0') {
		if (io->led_set(true) < 0) {
			reply(out, out_len, "ERR: led");
			return;
		}
		reply(out, out_len, "OK");
		return;
	}

	if (same_word(text, "OFF", &rest) && *skip_ws(rest) == '\0') {
		if (io->led_set(false) < 0) {
			reply(out, out_len, "ERR: led");
			return;
		}
		reply(out, out_len, "OK");
		return;
	}

	reply(out, out_len, "ERR: unknown command");
}

static void cmd_btn(const char *args, const struct shell_io *io, char *out, size_t out_len)
{
	int level;

	if (*skip_ws(args) != '\0') {
		reply(out, out_len, "ERR: unknown command");
		return;
	}
	if (io == NULL || io->button_get == NULL) {
		reply(out, out_len, "ERR: btn");
		return;
	}
	if (io->button_get(&level) < 0) {
		reply(out, out_len, "ERR: btn");
		return;
	}
	snprintf(out, out_len, "BTN: %d", level);
}

static void cmd_uptime(const char *args, const struct shell_io *io, char *out, size_t out_len)
{
	if (*skip_ws(args) != '\0') {
		reply(out, out_len, "ERR: unknown command");
		return;
	}
	if (io == NULL || io->uptime_ms == NULL) {
		reply(out, out_len, "ERR: uptime");
		return;
	}
	snprintf(out, out_len, "UPTIME: %" PRId64 " ms", io->uptime_ms());
}

static void cmd_echo(const char *args, const struct shell_io *io, char *out, size_t out_len)
{
	(void)io;

	/* So o separador depois de ECHO sai. "ECHO  a" devolve " a". */
	if (*args == ' ' || *args == '\t') {
		args++;
	}
	snprintf(out, out_len, "%s", args);
}

void shell_dispatch(const char *line, const struct shell_io *io, char *out, size_t out_len)
{
	const char *text;
	const char *rest;
	size_t i;

	if (out == NULL || out_len == 0) {
		return;
	}
	out[0] = '\0';
	if (line == NULL) {
		return;
	}

	text = skip_ws(line);
	if (*text == '\0') {
		return;
	}

	for (i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
		if (same_word(text, commands[i].name, &rest)) {
			commands[i].handler(rest, io, out, out_len);
			return;
		}
	}

	reply(out, out_len, "ERR: unknown command");
}
