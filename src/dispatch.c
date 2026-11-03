#include "dispatch.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef int (*minish_handler_t)(const char *args, char *out, size_t out_size);

struct minish_cmd {
	const char *name;
	const char *usage;
	const char *help;
	minish_handler_t handler;
};

static const struct minish_hw_ops *hw;

void minish_set_hw_ops(const struct minish_hw_ops *ops)
{
	hw = ops;
}

/* ---------- helpers de texto ---------- */

static bool is_ws(char c)
{
	return c == ' ' || c == '\t';
}

static const char *skip_ws(const char *s)
{
	while (is_ws(*s)) {
		s++;
	}
	return s;
}

static size_t token_len(const char *s)
{
	size_t n = 0;

	while (s[n] != '\0' && !is_ws(s[n])) {
		n++;
	}
	return n;
}

static char lower(char c)
{
	return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

/* compara o token (tok, n) com um literal, ignorando maiusculas */
static bool tok_is(const char *tok, size_t n, const char *lit)
{
	if (strlen(lit) != n) {
		return false;
	}
	for (size_t i = 0; i < n; i++) {
		if (lower(tok[i]) != lower(lit[i])) {
			return false;
		}
	}
	return true;
}

static int fail(char *out, size_t sz, int rc, const char *msg)
{
	(void)snprintf(out, sz, "%s", msg);
	return rc;
}

/* ---------- handlers ---------- */

static int cmd_help(const char *args, char *out, size_t sz);

static int cmd_led(const char *args, char *out, size_t sz)
{
	static const char usage[] = "ERR: usage: LED [ON|OFF|TOGGLE]";
	args = skip_ws(args);
	size_t n = token_len(args);
	bool on = false;

	if (*skip_ws(args + n) != '\0') {
		return fail(out, sz, -EINVAL, usage);
	}
	if (n != 0 && !tok_is(args, n, "ON") && !tok_is(args, n, "OFF") &&
	    !tok_is(args, n, "TOGGLE")) {
		return fail(out, sz, -EINVAL, usage);
	}
	if (hw == NULL || hw->led_set == NULL || hw->led_get == NULL) {
		return fail(out, sz, -ENODEV, "ERR: no led");
	}

	if (n == 0) { /* consulta */
		if (hw->led_get(&on) != 0) {
			return fail(out, sz, -EIO, "ERR: led failure");
		}
		(void)snprintf(out, sz, "LED: %s", on ? "ON" : "OFF");
		return 0;
	}

	if (tok_is(args, n, "ON")) {
		on = true;
	} else if (tok_is(args, n, "OFF")) {
		on = false;
	} else { /* TOGGLE */
		if (hw->led_get(&on) != 0) {
			return fail(out, sz, -EIO, "ERR: led failure");
		}
		on = !on;
	}

	if (hw->led_set(on) != 0) {
		return fail(out, sz, -EIO, "ERR: led failure");
	}
	(void)snprintf(out, sz, "OK");
	return 0;
}

static int cmd_btn(const char *args, char *out, size_t sz)
{
	bool pressed = false;

	if (*skip_ws(args) != '\0') {
		return fail(out, sz, -EINVAL, "ERR: usage: BTN");
	}
	if (hw == NULL || hw->btn_get == NULL) {
		return fail(out, sz, -ENODEV, "ERR: no button");
	}
	if (hw->btn_get(&pressed) != 0) {
		return fail(out, sz, -EIO, "ERR: button failure");
	}
	(void)snprintf(out, sz, "BTN: %s", pressed ? "PRESSED" : "RELEASED");
	return 0;
}

static int cmd_echo(const char *args, char *out, size_t sz)
{
	args = skip_ws(args);
	size_t len = strlen(args);

	while (len > 0 && is_ws(args[len - 1])) {
		len--;
	}
	if (len == 0) {
		return fail(out, sz, -EINVAL, "ERR: usage: ECHO <text>");
	}
	(void)snprintf(out, sz, "%.*s", (int)len, args);
	return 0;
}

/* ---------- tabela de comandos (function pointers) ---------- */

static const struct minish_cmd cmds[] = {
	{"HELP", "HELP", "list commands", cmd_help},
	{"LED", "LED [ON|OFF|TOGGLE]", "set / read the LED", cmd_led},
	{"BTN", "BTN", "read the button", cmd_btn},
	{"ECHO", "ECHO <text>", "reply with <text>", cmd_echo},
};

static int cmd_help(const char *args, char *out, size_t sz)
{
	size_t used = 0;

	(void)args;
	out[0] = '\0';
	for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
		int n = snprintf(out + used, sz - used, "%s%-22s %s", i ? "\r\n" : "",
				 cmds[i].usage, cmds[i].help);

		if (n < 0 || (size_t)n >= sz - used) {
			break; /* truncou: mantem o que coube */
		}
		used += (size_t)n;
	}
	return 0;
}

int minish_dispatch(const char *line, char *out, size_t out_size)
{
	if (out == NULL || out_size == 0) {
		return -EINVAL;
	}
	out[0] = '\0';
	if (line == NULL) {
		return -EINVAL;
	}

	line = skip_ws(line);
	size_t n = token_len(line);

	if (n == 0) {
		return 0; /* linha em branco: sem resposta */
	}

	const char *args = skip_ws(line + n);

	for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
		if (tok_is(line, n, cmds[i].name)) {
			return cmds[i].handler(args, out, out_size);
		}
	}
	return fail(out, out_size, -ENOENT, "ERR: unknown command");
}
