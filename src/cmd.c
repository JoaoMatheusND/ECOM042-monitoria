/*******************************************************************
 * @file cmd.c
 *
 * @brief Tabela de comandos (function pointers) e dispatch.
 *******************************************************************/

#include "cmd.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/sys/util.h>

static const struct cmd_hal *hal;

static int cmd_help(const char *args, char *out, size_t len);
static int cmd_led(const char *args, char *out, size_t len);
static int cmd_btn(const char *args, char *out, size_t len);
static int cmd_uptime(const char *args, char *out, size_t len);
static int cmd_echo(const char *args, char *out, size_t len);

static const struct cmd_entry cmd_table[] = {
	{"HELP", "HELP - lista os comandos", cmd_help},
	{"LED", "LED [ON|OFF] - liga/desliga o led0 ou mostra o estado", cmd_led},
	{"BTN", "BTN - le o botao sw0", cmd_btn},
	{"UPTIME", "UPTIME - tempo desde o boot em ms", cmd_uptime},
	{"ECHO", "ECHO <texto> - responde o texto recebido", cmd_echo},
};

static int clamp_written(int n, size_t len)
{
	if (n < 0 || len == 0) {
		return 0;
	}
	return MIN((size_t)n, len - 1);
}

static int reply(char *out, size_t len, const char *msg)
{
	return clamp_written(snprintf(out, len, "%s", msg), len);
}

static bool name_equals(const char *a, const char *b, size_t n)
{
	if (strlen(a) != n) {
		return false;
	}
	for (size_t i = 0; i < n; i++) {
		if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) {
			return false;
		}
	}
	return true;
}

static bool is_blank(const char *s)
{
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	return *s == '\0';
}

static void trim_copy(const char *s, char *dst, size_t len)
{
	size_t n;

	while (*s == ' ' || *s == '\t') {
		s++;
	}
	n = strlen(s);
	while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) {
		n--;
	}
	n = MIN(n, len - 1);
	memcpy(dst, s, n);
	dst[n] = '\0';
}

static int unknown(char *out, size_t len)
{
	return reply(out, len, "ERR: unknown command");
}

static int cmd_help(const char *args, char *out, size_t len)
{
	int written = 0;

	if (!is_blank(args)) {
		return unknown(out, len);
	}

	for (size_t i = 0; i < ARRAY_SIZE(cmd_table); i++) {
		const char *sep = (i == 0) ? "" : "\r\n";

		written += clamp_written(
			snprintf(out + written, len - written, "%s%s", sep, cmd_table[i].usage),
			len - written);
	}
	return written;
}

static int cmd_led(const char *args, char *out, size_t len)
{
	char arg[8];

	trim_copy(args, arg, sizeof(arg));

	if (arg[0] == '\0') {
		return reply(out, len, hal->led_get() ? "LED: ON" : "LED: OFF");
	}
	if (name_equals("ON", arg, strlen(arg)) || name_equals("OFF", arg, strlen(arg))) {
		bool on = name_equals("ON", arg, strlen(arg));

		if (hal->led_set(on) != 0) {
			return reply(out, len, "ERR: led");
		}
		return reply(out, len, "OK");
	}
	return unknown(out, len);
}

static int cmd_btn(const char *args, char *out, size_t len)
{
	int val;

	if (!is_blank(args)) {
		return unknown(out, len);
	}
	val = hal->btn_get();
	if (val < 0) {
		return reply(out, len, "ERR: btn");
	}
	return clamp_written(snprintf(out, len, "BTN: %d", val ? 1 : 0), len);
}

static int cmd_uptime(const char *args, char *out, size_t len)
{
	if (!is_blank(args)) {
		return unknown(out, len);
	}
	return clamp_written(snprintf(out, len, "UPTIME: %lld ms", (long long)hal->uptime_ms()),
			     len);
}

static int cmd_echo(const char *args, char *out, size_t len)
{
	return reply(out, len, args);
}

void cmd_init(const struct cmd_hal *h)
{
	hal = h;
}

int cmd_dispatch(const char *line, char *out, size_t len)
{
	const char *name;
	const char *args;
	size_t name_len;

	if (len == 0) {
		return 0;
	}
	out[0] = '\0';

	name = line;
	while (*name == ' ' || *name == '\t') {
		name++;
	}
	if (*name == '\0') {
		return 0;
	}

	name_len = strcspn(name, " \t");
	args = name + name_len;
	if (*args != '\0') {
		args++;
	}

	for (size_t i = 0; i < ARRAY_SIZE(cmd_table); i++) {
		if (name_equals(cmd_table[i].name, name, name_len)) {
			return cmd_table[i].handler(args, out, len);
		}
	}
	return unknown(out, len);
}
