/*******************************************************************
 * @file minishell.c
 *
 * @brief Parser/dispatch da mini-shell (tabela de comandos com
 *        function pointers).
 *
 * Nenhuma chamada de UART, GPIO ou printk vive neste arquivo: a
 * resposta e escrita num buffer e o hardware chega por injecao.
 *******************************************************************/

#include "minishell.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/** @brief Numero maximo de tokens considerados por linha. */
#define MINISHELL_MAX_ARGS 8

/** @brief Buffer de resposta em construcao. */
struct response {
	char *buf;
	size_t len;
	size_t cap;
};

/** @brief Entrada da tabela de comandos. */
struct minishell_command {
	const char *name;
	const char *usage;
	const char *help;
	int (*handler)(int argc, char **argv, const char *rest, struct response *out);
};

static const struct minishell_hw_ops *hw_ops;

static void resp_puts(struct response *out, const char *text)
{
	size_t len;

	if ((out == NULL) || (text == NULL) || (out->len >= out->cap)) {
		return;
	}

	len = strlen(text);
	if (len > (out->cap - out->len - 1U)) {
		len = out->cap - out->len - 1U;
	}

	memcpy(out->buf + out->len, text, len);
	out->len += len;
	out->buf[out->len] = '\0';
}

static void resp_printf(struct response *out, const char *fmt, ...)
{
	va_list args;
	int written;

	if ((out == NULL) || (fmt == NULL) || (out->len >= out->cap)) {
		return;
	}

	va_start(args, fmt);
	written = vsnprintf(out->buf + out->len, out->cap - out->len, fmt, args);
	va_end(args);

	if (written > 0) {
		out->len += MIN((size_t)written, out->cap - out->len - 1U);
	}
}

static bool str_eq_ci(const char *left, const char *right)
{
	while ((*left != '\0') && (*right != '\0')) {
		if (tolower((unsigned char)*left) != tolower((unsigned char)*right)) {
			return false;
		}
		left++;
		right++;
	}

	return *left == *right;
}

static int cmd_help(int argc, char **argv, const char *rest, struct response *out);
static int cmd_led(int argc, char **argv, const char *rest, struct response *out);
static int cmd_btn(int argc, char **argv, const char *rest, struct response *out);
static int cmd_uptime(int argc, char **argv, const char *rest, struct response *out);
static int cmd_echo(int argc, char **argv, const char *rest, struct response *out);

static const struct minishell_command commands[] = {
	{"HELP", "HELP", "lista os comandos disponiveis", cmd_help},
	{"LED", "LED [ON|OFF]", "liga/desliga o LED ou mostra o estado", cmd_led},
	{"BTN", "BTN", "le o botao sw0", cmd_btn},
	{"UPTIME", "UPTIME", "tempo desde o boot em ms", cmd_uptime},
	{"ECHO", "ECHO <texto>", "repete o texto recebido", cmd_echo},
};

static const struct minishell_command *find_command(const char *name)
{
	size_t i;

	for (i = 0U; i < ARRAY_SIZE(commands); i++) {
		if (str_eq_ci(commands[i].name, name)) {
			return &commands[i];
		}
	}

	return NULL;
}

static int cmd_led(int argc, char **argv, const char *rest, struct response *out)
{
	bool on;
	int ret;

	ARG_UNUSED(rest);

	if (argc == 1) {
		if ((hw_ops == NULL) || (hw_ops->led_get == NULL)) {
			resp_puts(out, "ERR: led unavailable");
			return -ENOSYS;
		}

		ret = hw_ops->led_get(&on);
		if (ret != 0) {
			resp_printf(out, "ERR: led read failed (%d)", ret);
			return ret;
		}

		resp_puts(out, on ? "LED: ON" : "LED: OFF");
		return 0;
	}

	if (argc > 2) {
		resp_puts(out, MINISHELL_ERR_INVALID_ARGUMENT);
		return -EINVAL;
	}

	if (str_eq_ci(argv[1], "ON")) {
		on = true;
	} else if (str_eq_ci(argv[1], "OFF")) {
		on = false;
	} else {
		resp_puts(out, MINISHELL_ERR_INVALID_ARGUMENT);
		return -EINVAL;
	}

	if ((hw_ops == NULL) || (hw_ops->led_set == NULL)) {
		resp_puts(out, "ERR: led unavailable");
		return -ENOSYS;
	}

	ret = hw_ops->led_set(on);
	if (ret != 0) {
		resp_printf(out, "ERR: led write failed (%d)", ret);
		return ret;
	}

	resp_puts(out, "OK");
	return 0;
}

static int cmd_btn(int argc, char **argv, const char *rest, struct response *out)
{
	int pressed;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	ARG_UNUSED(rest);

	if ((hw_ops == NULL) || (hw_ops->button_read == NULL)) {
		resp_puts(out, "ERR: button unavailable");
		return -ENOSYS;
	}

	ret = hw_ops->button_read(&pressed);
	if (ret != 0) {
		resp_printf(out, "ERR: button read failed (%d)", ret);
		return ret;
	}

	resp_printf(out, "BTN: %d", pressed);
	return 0;
}

static int cmd_uptime(int argc, char **argv, const char *rest, struct response *out)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	ARG_UNUSED(rest);

	resp_printf(out, "UPTIME: %u ms", (uint32_t)k_uptime_get());
	return 0;
}

static int cmd_echo(int argc, char **argv, const char *rest, struct response *out)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if ((rest == NULL) || (*rest == '\0')) {
		resp_puts(out, MINISHELL_ERR_INVALID_ARGUMENT);
		return -EINVAL;
	}

	resp_puts(out, rest);
	return 0;
}

static int cmd_help(int argc, char **argv, const char *rest, struct response *out)
{
	size_t i;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	ARG_UNUSED(rest);

	for (i = 0U; i < ARRAY_SIZE(commands); i++) {
		if (i > 0U) {
			resp_puts(out, "\r\n");
		}
		resp_printf(out, "%s - %s", commands[i].usage, commands[i].help);
	}

	return 0;
}

static int tokenize(char *line, char **argv, size_t max_args)
{
	char *cursor = line;
	int argc = 0;

	while (*cursor != '\0') {
		while (isspace((unsigned char)*cursor)) {
			cursor++;
		}

		if (*cursor == '\0') {
			break;
		}

		if ((size_t)argc >= max_args) {
			break;
		}

		argv[argc] = cursor;
		argc++;

		while ((*cursor != '\0') && !isspace((unsigned char)*cursor)) {
			cursor++;
		}

		if (*cursor != '\0') {
			*cursor = '\0';
			cursor++;
		}
	}

	return argc;
}

void minishell_init(const struct minishell_hw_ops *ops)
{
	hw_ops = ops;
}

size_t minishell_execute(const char *line, char *out, size_t out_len)
{
	char work[CONFIG_MINISHELL_LINE_BUF_SIZE];
	char raw[CONFIG_MINISHELL_LINE_BUF_SIZE];
	char *argv[MINISHELL_MAX_ARGS];
	const struct minishell_command *command;
	const char *rest;
	struct response response;
	int argc;
	size_t offset;

	if ((line == NULL) || (out == NULL) || (out_len == 0U)) {
		return 0U;
	}

	out[0] = '\0';

	strncpy(work, line, sizeof(work) - 1U);
	work[sizeof(work) - 1U] = '\0';
	strncpy(raw, work, sizeof(raw));

	argc = tokenize(work, argv, ARRAY_SIZE(argv));
	if (argc == 0) {
		return 0U;
	}

	/* O texto do ECHO e o resto da linha original, nao os tokens ja
	 * separados: copia intacta em raw garante "exatamente como recebido".
	 */
	offset = (size_t)(argv[0] - work) + strlen(argv[0]);
	rest = raw + offset;
	while ((*rest == ' ') || (*rest == '\t')) {
		rest++;
	}

	response.buf = out;
	response.len = 0U;
	response.cap = out_len;

	command = find_command(argv[0]);
	if (command == NULL) {
		resp_puts(&response, MINISHELL_ERR_UNKNOWN_COMMAND);
		return response.len;
	}

	(void)command->handler(argc, argv, rest, &response);

	return response.len;
}
