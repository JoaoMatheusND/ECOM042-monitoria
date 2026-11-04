#include <zephyr/ztest.h>
#include <string.h>

#include "core/shell_cmd.h"

struct mock {
	bool led;
	int btn;
	int64_t uptime;
	int led_fail; /* retorno forcado nas operacoes de LED */
};

static struct mock m;

static int mock_led_set(void *u, bool on)
{
	struct mock *p = u;

	if (p->led_fail) {
		return p->led_fail;
	}
	p->led = on;
	return 0;
}

static int mock_led_toggle(void *u)
{
	struct mock *p = u;

	if (p->led_fail) {
		return p->led_fail;
	}
	p->led = !p->led;
	return 0;
}

static int mock_led_get(void *u)
{
	return ((struct mock *)u)->led ? 1 : 0;
}

static int mock_btn_get(void *u)
{
	return ((struct mock *)u)->btn;
}

static int64_t mock_uptime(void *u)
{
	return ((struct mock *)u)->uptime;
}

static const struct shell_hw_ops hw = {
	.led_set = mock_led_set,
	.led_toggle = mock_led_toggle,
	.led_get = mock_led_get,
	.button_get = mock_btn_get,
	.uptime_ms = mock_uptime,
	.user = &m,
};

static char out[256];

static void before(void *f)
{
	ARG_UNUSED(f);
	memset(&m, 0, sizeof(m));
	memset(out, 'X', sizeof(out));
}

/* copia para buffer gravavel (a execuçao modifica a linha) */
static size_t run_hw(const struct shell_hw_ops *ops, const char *cmd)
{
	char line[96];

	strncpy(line, cmd, sizeof(line) - 1);
	line[sizeof(line) - 1] = '\0';
	return shell_cmd_execute(ops, line, out, sizeof(out));
}

static size_t run(const char *cmd)
{
	return run_hw(&hw, cmd);
}

ZTEST(shell_cmd, test_help_lists_all_commands)
{
	size_t n = run("HELP");

	zassert_equal(n, strlen(out));
	zassert_not_null(strstr(out, "HELP"));
	zassert_not_null(strstr(out, "LED"));
	zassert_not_null(strstr(out, "BTN"));
	zassert_not_null(strstr(out, "UPTIME"));
	zassert_not_null(strstr(out, "ECHO"));
	zassert_not_null(strstr(out, "VERSION"));
}

ZTEST(shell_cmd, test_response_has_no_trailing_crlf)
{
	size_t n = run("VERSION");

	zassert_true(n > 0);
	zassert_not_equal(out[n - 1], '\n');
	zassert_not_equal(out[n - 1], '\r');
}

ZTEST(shell_cmd, test_commands_are_case_insensitive)
{
	run("hElP");
	zassert_not_null(strstr(out, "LED"));

	run("led on");
	zassert_str_equal(out, "LED: ON");
	zassert_true(m.led);

	run("LeD OfF");
	zassert_str_equal(out, "LED: OFF");
	zassert_false(m.led);
}

ZTEST(shell_cmd, test_led_on_off_toggle_status)
{
	run("LED ON");
	zassert_str_equal(out, "LED: ON");
	run("LED");
	zassert_str_equal(out, "LED: ON");
	run("LED TOGGLE");
	zassert_str_equal(out, "LED: OFF");
	zassert_false(m.led);
	run("LED TOGGLE");
	zassert_str_equal(out, "LED: ON");
	run("LED OFF");
	zassert_str_equal(out, "LED: OFF");
}

ZTEST(shell_cmd, test_led_bad_argument)
{
	run("LED BLINK");
	zassert_str_equal(out, "ERR: usage: LED [ON|OFF|TOGGLE]");
	run("LED ON OFF");
	zassert_str_equal(out, "ERR: usage: LED [ON|OFF|TOGGLE]");
}

ZTEST(shell_cmd, test_led_hardware_error)
{
	m.led_fail = -5;
	run("LED ON");
	zassert_str_equal(out, "ERR: hardware error");
}

ZTEST(shell_cmd, test_button_states)
{
	m.btn = 1;
	run("BTN");
	zassert_str_equal(out, "BTN: PRESSED");
	m.btn = 0;
	run("btn");
	zassert_str_equal(out, "BTN: RELEASED");
	m.btn = -1;
	run("BTN");
	zassert_str_equal(out, "ERR: hardware error");
}

ZTEST(shell_cmd, test_uptime)
{
	m.uptime = 12345;
	run("UPTIME");
	zassert_str_equal(out, "UPTIME: 12345 ms");
}

ZTEST(shell_cmd, test_echo_preserves_case_and_joins_args)
{
	run("echo Hello   World");
	zassert_str_equal(out, "Hello World");
}

ZTEST(shell_cmd, test_echo_without_args)
{
	run("ECHO");
	zassert_str_equal(out, "ERR: usage: ECHO <text>");
}

ZTEST(shell_cmd, test_version)
{
	run("VERSION");
	zassert_not_null(strstr(out, "mini-shell"));
}

ZTEST(shell_cmd, test_unknown_command)
{
	run("FOO");
	zassert_str_equal(out, "ERR: unknown command");
}

ZTEST(shell_cmd, test_empty_and_blank_lines_produce_no_response)
{
	zassert_equal(run(""), 0);
	zassert_str_equal(out, "");
	zassert_equal(run("   \t  "), 0);
	zassert_str_equal(out, "");
}

ZTEST(shell_cmd, test_leading_and_trailing_whitespace)
{
	run("   led    on   ");
	zassert_str_equal(out, "LED: ON");
}

ZTEST(shell_cmd, test_too_many_arguments)
{
	run("ECHO 1 2 3 4 5 6 7 8 9");
	zassert_str_equal(out, "ERR: too many arguments");
}

ZTEST(shell_cmd, test_small_output_buffer_is_truncated_safely)
{
	char line[] = "HELP";
	char small[8];
	size_t n;

	memset(small, 'X', sizeof(small));
	n = shell_cmd_execute(&hw, line, small, sizeof(small));
	zassert_equal(n, sizeof(small) - 1);
	zassert_equal(small[sizeof(small) - 1], '\0');
	zassert_equal(strlen(small), n);
}

ZTEST(shell_cmd, test_missing_hw_ops_reports_not_supported)
{
	run_hw(NULL, "LED ON");
	zassert_str_equal(out, "ERR: not supported");
	run_hw(NULL, "BTN");
	zassert_str_equal(out, "ERR: not supported");
	run_hw(NULL, "UPTIME");
	zassert_str_equal(out, "ERR: not supported");
	/* comandos sem hardware continuam funcionando */
	run_hw(NULL, "ECHO ok");
	zassert_str_equal(out, "ok");
}

ZTEST_SUITE(shell_cmd, NULL, NULL, before, NULL, NULL);
