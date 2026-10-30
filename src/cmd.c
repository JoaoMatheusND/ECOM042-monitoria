/*******************************************************************
 * @file cmd.c
 *
 * @brief Interpretacao e despacho dos comandos da shell.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "board_io.h"
#include "cmd.h"

typedef struct {
	const char *name;
	const char *usage;
	void (*handler)(const char *args, char *out, size_t out_size);
} cmd_entry_t;

static void cmd_help(const char *args, char *out, size_t out_size);
static void cmd_led(const char *args, char *out, size_t out_size);
static void cmd_btn(const char *args, char *out, size_t out_size);
static void cmd_uptime(const char *args, char *out, size_t out_size);
static void cmd_echo(const char *args, char *out, size_t out_size);

static const cmd_entry_t commands[] = {
	{.name = "HELP", .usage = "HELP - lista os comandos", .handler = cmd_help},
	{.name = "LED",
	 .usage = "LED [ON|OFF] - liga, desliga ou consulta o LED",
	 .handler = cmd_led},
	{.name = "BTN", .usage = "BTN - le o botao", .handler = cmd_btn},
	{.name = "UPTIME", .usage = "UPTIME - tempo desde o boot, em ms", .handler = cmd_uptime},
	{.name = "ECHO", .usage = "ECHO <texto> - devolve o texto", .handler = cmd_echo},
};

static char to_upper(char c)
{
	return ((c >= 'a') && (c <= 'z')) ? (char)(c - 'a' + 'A') : c;
}

static bool is_space(char c)
{
	return (c == ' ') || (c == '\t');
}

static const char *skip_spaces(const char *s)
{
	while (is_space(*s)) {
		s++;
	}

	return s;
}

static size_t token_len(const char *s)
{
	size_t n = 0U;

	while ((s[n] != '\0') && !is_space(s[n])) {
		n++;
	}

	return n;
}

/* compara sem diferenciar maiusculas, name ja vem em maiusculas */
static bool token_equals(const char *token, size_t len, const char *name)
{
	for (size_t i = 0U; i < len; i++) {
		if (to_upper(token[i]) != name[i]) {
			return false;
		}
	}

	return name[len] == '\0';
}

static void respond(char *out, size_t out_size, const char *text)
{
	(void)snprintf(out, out_size, "%s", text);
}

static void cmd_help(const char *args, char *out, size_t out_size)
{
	size_t used = 0U;

	ARG_UNUSED(args);

	for (size_t i = 0U; i < ARRAY_SIZE(commands); i++) {
		int written = snprintf(&out[used], out_size - used, "%s%s", (i == 0U) ? "" : "\r\n",
				       commands[i].usage);

		if ((written < 0) || ((size_t)written >= (out_size - used))) {
			break;
		}

		used += (size_t)written;
	}
}

static void cmd_led(const char *args, char *out, size_t out_size)
{
	size_t len = token_len(args);
	bool turn_on;

	if (len == 0U) {
		respond(out, out_size, board_io_led_get() ? "LED: ON" : "LED: OFF");
		return;
	}

	if (token_len(skip_spaces(&args[len])) != 0U) {
		respond(out, out_size, CMD_ERR_UNKNOWN);
		return;
	}

	if (token_equals(args, len, "ON")) {
		turn_on = true;
	} else if (token_equals(args, len, "OFF")) {
		turn_on = false;
	} else {
		respond(out, out_size, CMD_ERR_UNKNOWN);
		return;
	}

	respond(out, out_size, (board_io_led_set(turn_on) < 0) ? CMD_ERR_IO : "OK");
}

static void cmd_btn(const char *args, char *out, size_t out_size)
{
	int state;

	ARG_UNUSED(args);

	state = board_io_button_get();
	if (state < 0) {
		respond(out, out_size, CMD_ERR_IO);
		return;
	}

	(void)snprintf(out, out_size, "BTN: %d", state);
}

static void cmd_uptime(const char *args, char *out, size_t out_size)
{
	ARG_UNUSED(args);

	(void)snprintf(out, out_size, "UPTIME: %lld ms", (long long)k_uptime_get());
}

static void cmd_echo(const char *args, char *out, size_t out_size)
{
	respond(out, out_size, args);
}

int cmd_dispatch(const char *line, char *out, size_t out_size)
{
	const char *cursor;
	size_t name_len;

	if ((line == NULL) || (out == NULL) || (out_size == 0U)) {
		return -EINVAL;
	}

	out[0] = '\0';

	cursor = skip_spaces(line);
	name_len = token_len(cursor);

	/* linha vazia so devolve o prompt */
	if (name_len == 0U) {
		return 0;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(commands); i++) {
		if (token_equals(cursor, name_len, commands[i].name)) {
			commands[i].handler(skip_spaces(&cursor[name_len]), out, out_size);
			return 0;
		}
	}

	respond(out, out_size, CMD_ERR_UNKNOWN);

	return -ENOENT;
}
