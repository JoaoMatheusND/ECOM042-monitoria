#include "shell.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "io.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef int (*shell_handler_t)(const char *args, char *out, size_t out_len);

struct shell_cmd {
	const char *name;
	shell_handler_t handler;
	const char *help;
};

static int icase_cmp_n(const char *a, const char *b, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		char ca = (a[i] >= 'a' && a[i] <= 'z') ? (a[i] - 32) : a[i];
		char cb = (b[i] >= 'a' && b[i] <= 'z') ? (b[i] - 32) : b[i];
		if (ca != cb) {
			return ca - cb;
		}
	}
	return 0;
}

static const char *skip_ws(const char *s)
{
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	return s;
}

static int cmd_help(const char *args, char *out, size_t out_len);
static int cmd_led(const char *args, char *out, size_t out_len);
static int cmd_btn(const char *args, char *out, size_t out_len);
static int cmd_uptime(const char *args, char *out, size_t out_len);
static int cmd_echo(const char *args, char *out, size_t out_len);

static const struct shell_cmd commands[] = {
	{"HELP", cmd_help, "HELP"},        {"LED", cmd_led, "LED [ON|OFF]"},
	{"BTN", cmd_btn, "BTN"},           {"UPTIME", cmd_uptime, "UPTIME"},
	{"ECHO", cmd_echo, "ECHO <text>"},
};

static int cmd_help(const char *args, char *out, size_t out_len)
{
	(void)args;
	size_t n = 0;
	for (size_t i = 0; i < ARRAY_SIZE(commands); i++) {
		int r = snprintf(out + n, out_len - n, "%s\r\n", commands[i].help);
		if (r < 0 || (size_t)r >= out_len - n) {
			break;
		}
		n += r;
	}
	return (int)n;
}

static int cmd_led(const char *args, char *out, size_t out_len)
{
	args = skip_ws(args);
	if (*args == '\0') {
		return snprintf(out, out_len, "LED: %s", io_led_get() ? "ON" : "OFF");
	}
	if (icase_cmp_n(args, "ON", 2) == 0 && args[2] == '\0') {
		io_led_set(true);
		return snprintf(out, out_len, "OK");
	}
	if (icase_cmp_n(args, "OFF", 3) == 0 && args[3] == '\0') {
		io_led_set(false);
		return snprintf(out, out_len, "OK");
	}
	return snprintf(out, out_len, "ERR: unknown command");
}

static int cmd_btn(const char *args, char *out, size_t out_len)
{
	(void)args;
	return snprintf(out, out_len, "BTN: %d", io_btn_get());
}

static int cmd_uptime(const char *args, char *out, size_t out_len)
{
	(void)args;
	return snprintf(out, out_len, "UPTIME: %lld ms", io_uptime_get());
}

static int cmd_echo(const char *args, char *out, size_t out_len)
{
	args = skip_ws(args);
	if (out_len > 0) {
		out[0] = '\0';
	}
	size_t len = strlen(args);
	if (len >= out_len) {
		len = out_len - 1;
	}
	memcpy(out, args, len);
	out[len] = '\0';
	return (int)len;
}

int shell_dispatch(const char *line, char *out, size_t out_len)
{
	line = skip_ws(line);
	const char *end = line;
	while (*end != '\0' && *end != ' ' && *end != '\t') {
		end++;
	}
	size_t name_len = end - line;
	const char *args = skip_ws(end);

	for (size_t i = 0; i < ARRAY_SIZE(commands); i++) {
		const char *name = commands[i].name;
		if (icase_cmp_n(line, name, name_len) == 0 && name[name_len] == '\0') {
			return commands[i].handler(args, out, out_len);
		}
	}
	return snprintf(out, out_len, "ERR: unknown command");
}
