#include "shell_cmd.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define VERSION_STR "mini-shell 1.0"

void shell_out_printf(struct shell_out *out, const char *fmt, ...)
{
	va_list ap;
	size_t room;
	int n;

	if (out->size == 0 || out->len >= out->size - 1) {
		return;
	}
	room = out->size - out->len;

	va_start(ap, fmt);
	n = vsnprintf(out->buf + out->len, room, fmt, ap);
	va_end(ap);

	if (n < 0) {
		return;
	}
	out->len = ((size_t)n >= room) ? out->size - 1 : out->len + (size_t)n;
}

static char lower(char c)
{
	return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static bool ci_equal(const char *a, const char *b)
{
	while (*a && *b) {
		if (lower(*a) != lower(*b)) {
			return false;
		}
		a++;
		b++;
	}
	return *a == *b;
}

/* Wrappers que tratam hw/ops ausentes */
static int hw_led_set(const struct shell_hw_ops *hw, bool on)
{
	return (hw && hw->led_set) ? hw->led_set(hw->user, on) : -ENOTSUP;
}

static int hw_led_toggle(const struct shell_hw_ops *hw)
{
	return (hw && hw->led_toggle) ? hw->led_toggle(hw->user) : -ENOTSUP;
}

static int hw_led_get(const struct shell_hw_ops *hw)
{
	return (hw && hw->led_get) ? hw->led_get(hw->user) : -ENOTSUP;
}

static int hw_button_get(const struct shell_hw_ops *hw)
{
	return (hw && hw->button_get) ? hw->button_get(hw->user) : -ENOTSUP;
}

static void err_hw(struct shell_out *out, int err)
{
	shell_out_printf(out, err == -ENOTSUP ? "ERR: not supported" : "ERR: hardware error");
}

static void cmd_help(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out);
static void cmd_led(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out);
static void cmd_btn(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out);
static void cmd_uptime(const struct shell_hw_ops *hw, int argc, char *argv[],
		       struct shell_out *out);
static void cmd_echo(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out);
static void cmd_version(const struct shell_hw_ops *hw, int argc, char *argv[],
			struct shell_out *out);

static const struct shell_cmd cmds[] = {
	{"HELP", "HELP", "list commands", cmd_help},
	{"LED", "LED [ON|OFF|TOGGLE]", "set the LED / show its state", cmd_led},
	{"BTN", "BTN", "show the button state", cmd_btn},
	{"UPTIME", "UPTIME", "time since boot in ms", cmd_uptime},
	{"ECHO", "ECHO <text>", "print the text back", cmd_echo},
	{"VERSION", "VERSION", "firmware version", cmd_version},
};

#define CMD_COUNT (sizeof(cmds) / sizeof(cmds[0]))

static void cmd_help(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out)
{
	(void)hw;
	(void)argc;
	(void)argv;

	for (size_t i = 0; i < CMD_COUNT; i++) {
		shell_out_printf(out, "%s%s - %s", i ? "\r\n" : "", cmds[i].usage, cmds[i].help);
	}
}

static void cmd_led(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out)
{
	int ret = 0;

	if (argc > 2) {
		shell_out_printf(out, "ERR: usage: LED [ON|OFF|TOGGLE]");
		return;
	}

	if (argc == 2) {
		if (ci_equal(argv[1], "on")) {
			ret = hw_led_set(hw, true);
		} else if (ci_equal(argv[1], "off")) {
			ret = hw_led_set(hw, false);
		} else if (ci_equal(argv[1], "toggle")) {
			ret = hw_led_toggle(hw);
		} else {
			shell_out_printf(out, "ERR: usage: LED [ON|OFF|TOGGLE]");
			return;
		}
		if (ret < 0) {
			err_hw(out, ret);
			return;
		}
	}

	ret = hw_led_get(hw);
	if (ret < 0) {
		err_hw(out, ret);
		return;
	}
	shell_out_printf(out, "LED: %s", ret ? "ON" : "OFF");
}

static void cmd_btn(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out)
{
	int ret;

	(void)argc;
	(void)argv;

	ret = hw_button_get(hw);
	if (ret < 0) {
		err_hw(out, ret);
		return;
	}
	shell_out_printf(out, "BTN: %s", ret ? "PRESSED" : "RELEASED");
}

static void cmd_uptime(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out)
{
	(void)argc;
	(void)argv;

	if (hw == NULL || hw->uptime_ms == NULL) {
		err_hw(out, -ENOTSUP);
		return;
	}
	shell_out_printf(out, "UPTIME: %lld ms", (long long)hw->uptime_ms(hw->user));
}

static void cmd_echo(const struct shell_hw_ops *hw, int argc, char *argv[], struct shell_out *out)
{
	(void)hw;

	if (argc < 2) {
		shell_out_printf(out, "ERR: usage: ECHO <text>");
		return;
	}
	for (int i = 1; i < argc; i++) {
		shell_out_printf(out, "%s%s", i > 1 ? " " : "", argv[i]);
	}
}

static void cmd_version(const struct shell_hw_ops *hw, int argc, char *argv[],
			struct shell_out *out)
{
	(void)hw;
	(void)argc;
	(void)argv;

	shell_out_printf(out, VERSION_STR);
}

static bool is_space(char c)
{
	return c == ' ' || c == '\t';
}

size_t shell_cmd_execute(const struct shell_hw_ops *hw, char *line, char *out_buf, size_t out_size)
{
	struct shell_out out = {.buf = out_buf, .size = out_size, .len = 0};
	char *argv[SHELL_CMD_MAX_ARGS];
	int argc = 0;
	char *p = line;

	if (out_buf == NULL || out_size == 0) {
		return 0;
	}
	out_buf[0] = '\0';
	if (line == NULL) {
		return 0;
	}

	while (*p) {
		while (is_space(*p)) {
			*p++ = '\0';
		}
		if (*p == '\0') {
			break;
		}
		if (argc >= SHELL_CMD_MAX_ARGS) {
			shell_out_printf(&out, "ERR: too many arguments");
			return out.len;
		}
		argv[argc++] = p;
		while (*p && !is_space(*p)) {
			p++;
		}
	}

	if (argc == 0) {
		return 0; /* linha vazia: so reexibe o prompt */
	}

	for (size_t i = 0; i < CMD_COUNT; i++) {
		if (ci_equal(argv[0], cmds[i].name)) {
			cmds[i].fn(hw, argc, argv, &out);
			return out.len;
		}
	}

	shell_out_printf(&out, "ERR: unknown command");
	return out.len;
}
