/*******************************************************************
 * @file cmd.c
 *
 * @brief Parsing e despacho dos comandos da mini-shell.
 *******************************************************************/

#include <ctype.h>
#include <stdio.h>

#include "cmd.h"

typedef int (*cmd_handler_t)(const char *args, char *resp, size_t size);

struct cmd_entry {
	const char *name; /* uma ou mais palavras, ex.: "LED ON" */
	const char *help;
	bool has_args; /* true: o resto da linha vai para o handler */
	cmd_handler_t handler;
};

static const struct cmd_io *hw;

static int cmd_help(const char *args, char *resp, size_t size);
static int cmd_led_on(const char *args, char *resp, size_t size);
static int cmd_led_off(const char *args, char *resp, size_t size);
static int cmd_led(const char *args, char *resp, size_t size);
static int cmd_btn(const char *args, char *resp, size_t size);
static int cmd_uptime(const char *args, char *resp, size_t size);
static int cmd_echo(const char *args, char *resp, size_t size);

/* Os nomes mais longos vêm antes ("LED ON" antes de "LED"). */
static const struct cmd_entry cmd_table[] = {
	{"HELP", "HELP - lista os comandos", false, cmd_help},
	{"LED ON", "LED ON - liga o LED", false, cmd_led_on},
	{"LED OFF", "LED OFF - desliga o LED", false, cmd_led_off},
	{"LED", "LED - mostra o estado do LED", false, cmd_led},
	{"BTN", "BTN - le o botao", false, cmd_btn},
	{"UPTIME", "UPTIME - tempo desde o boot", false, cmd_uptime},
	{"ECHO", "ECHO <texto> - devolve o texto", true, cmd_echo},
};

#define CMD_COUNT (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int put(char *resp, size_t size, int n)
{
	/* snprintf trunca; devolve o que realmente coube no buffer. */
	if (n < 0) {
		return 0;
	}

	return ((size_t)n < size) ? n : (int)size - 1;
}

static int reply_error(char *resp, size_t size, int err)
{
	return put(resp, size, snprintf(resp, size, "ERR: %d", err));
}

static int cmd_help(const char *args, char *resp, size_t size)
{
	int len = 0;

	(void)args;

	for (size_t i = 0; i < CMD_COUNT; i++) {
		const char *sep = (i == 0) ? "" : "\r\n";

		len += put(resp + len, size - len,
			   snprintf(resp + len, size - len, "%s%s", sep, cmd_table[i].help));
	}

	return len;
}

static int led_write(bool on, char *resp, size_t size)
{
	int ret = hw->led_set(on);

	if (ret < 0) {
		return reply_error(resp, size, ret);
	}

	return put(resp, size, snprintf(resp, size, "OK"));
}

static int cmd_led_on(const char *args, char *resp, size_t size)
{
	(void)args;
	return led_write(true, resp, size);
}

static int cmd_led_off(const char *args, char *resp, size_t size)
{
	(void)args;
	return led_write(false, resp, size);
}

static int cmd_led(const char *args, char *resp, size_t size)
{
	int state = hw->led_get();

	(void)args;

	if (state < 0) {
		return reply_error(resp, size, state);
	}

	return put(resp, size, snprintf(resp, size, "LED: %s", state ? "ON" : "OFF"));
}

static int cmd_btn(const char *args, char *resp, size_t size)
{
	int state = hw->button_read();

	(void)args;

	if (state < 0) {
		return reply_error(resp, size, state);
	}

	return put(resp, size, snprintf(resp, size, "BTN: %d", state));
}

static int cmd_uptime(const char *args, char *resp, size_t size)
{
	(void)args;

	return put(resp, size, snprintf(resp, size, "UPTIME: %lld ms", (long long)hw->uptime_ms()));
}

static int cmd_echo(const char *args, char *resp, size_t size)
{
	return put(resp, size, snprintf(resp, size, "%s", args));
}

/* Compara o início de line com name, ignorando maiúsculas/minúsculas.
 * Retorna o ponteiro logo após o nome, ou NULL se não casar. */
static const char *match_name(const char *line, const char *name)
{
	while (*name != '\0') {
		if (toupper((unsigned char)*line) != toupper((unsigned char)*name)) {
			return NULL;
		}
		line++;
		name++;
	}

	return line;
}

static bool only_spaces(const char *s)
{
	while (*s == ' ') {
		s++;
	}

	return *s == '\0';
}

void cmd_init(const struct cmd_io *io)
{
	hw = io;
}

int cmd_execute(const char *line, char *resp, size_t resp_size)
{
	if (resp_size == 0) {
		return 0;
	}

	resp[0] = '\0';

	while (*line == ' ') {
		line++;
	}

	if (*line == '\0') {
		return 0;
	}

	for (size_t i = 0; i < CMD_COUNT; i++) {
		const struct cmd_entry *cmd = &cmd_table[i];
		const char *rest = match_name(line, cmd->name);

		if (rest == NULL) {
			continue;
		}

		if (cmd->has_args) {
			/* "ECHO" sozinho ou "ECHO <texto>": um espaço separa o texto. */
			if (*rest == ' ') {
				return cmd->handler(rest + 1, resp, resp_size);
			}
			if (*rest == '\0') {
				return cmd->handler(rest, resp, resp_size);
			}
		} else if (only_spaces(rest)) {
			return cmd->handler(rest, resp, resp_size);
		}
	}

	return put(resp, resp_size, snprintf(resp, resp_size, "ERR: unknown command"));
}
