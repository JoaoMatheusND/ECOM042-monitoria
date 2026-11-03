/*******************************************************************
 * @file test_cmd.c
 *
 * @brief Testes do parsing/dispatch (cmd.c) com hardware falso.
 *******************************************************************/

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "cmd.h"

static bool fake_led;
static int fake_button;
static int fake_led_err;

static int fake_led_set(bool on)
{
	if (fake_led_err) {
		return fake_led_err;
	}
	fake_led = on;
	return 0;
}

static int fake_led_get(void)
{
	return fake_led ? 1 : 0;
}

static int fake_button_read(void)
{
	return fake_button;
}

static int64_t fake_uptime_ms(void)
{
	return 1234;
}

static const struct cmd_io fake_io = {
	.led_set = fake_led_set,
	.led_get = fake_led_get,
	.button_read = fake_button_read,
	.uptime_ms = fake_uptime_ms,
};

static char resp[256];

static const char *run(const char *line)
{
	int len = cmd_execute(line, resp, sizeof(resp));

	zassert_equal(len, (int)strlen(resp), "tamanho retornado diferente da resposta");
	return resp;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_led = false;
	fake_button = 0;
	fake_led_err = 0;
	cmd_init(&fake_io);
}

ZTEST_SUITE(cmd, NULL, NULL, before, NULL, NULL);

ZTEST(cmd, test_help_lists_one_command_per_line)
{
	const char *expected[] = {"HELP", "LED ON", "LED OFF", "LED -", "BTN", "UPTIME", "ECHO"};
	const char *out = run("HELP");

	for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
		zassert_not_null(strstr(out, expected[i]), "HELP sem %s", expected[i]);
	}
	/* 7 comandos -> 6 separadores "\r\n" entre as linhas */
	int lines = 1;

	for (const char *p = strstr(out, "\r\n"); p != NULL; p = strstr(p + 2, "\r\n")) {
		lines++;
	}
	zassert_equal(lines, 7, "HELP deveria ter 7 linhas, tem %d", lines);
}

ZTEST(cmd, test_led_on_off_and_status)
{
	zassert_str_equal(run("LED"), "LED: OFF");
	zassert_str_equal(run("LED ON"), "OK");
	zassert_true(fake_led);
	zassert_str_equal(run("LED"), "LED: ON");
	zassert_str_equal(run("LED OFF"), "OK");
	zassert_false(fake_led);
	zassert_str_equal(run("LED"), "LED: OFF");
}

ZTEST(cmd, test_case_insensitive)
{
	zassert_str_equal(run("led on"), "OK");
	zassert_str_equal(run("Led"), "LED: ON");
	zassert_str_equal(run("lEd OfF"), "OK");
	zassert_str_equal(run("btn"), "BTN: 0");
	zassert_str_equal(run("echo oi"), "oi");
}

ZTEST(cmd, test_btn)
{
	fake_button = 1;
	zassert_str_equal(run("BTN"), "BTN: 1");
	fake_button = 0;
	zassert_str_equal(run("BTN"), "BTN: 0");
}

ZTEST(cmd, test_uptime)
{
	zassert_str_equal(run("UPTIME"), "UPTIME: 1234 ms");
}

ZTEST(cmd, test_echo_keeps_text_as_received)
{
	zassert_str_equal(run("ECHO oi"), "oi");
	zassert_str_equal(run("ECHO Ola   Mundo  "), "Ola   Mundo  ");
	zassert_str_equal(run("ECHO"), "");
}

ZTEST(cmd, test_unknown_commands)
{
	zassert_str_equal(run("XPTO"), "ERR: unknown command");
	zassert_str_equal(run("LED BLINK"), "ERR: unknown command");
	zassert_str_equal(run("LEDON"), "ERR: unknown command");
	zassert_str_equal(run("ECHOoi"), "ERR: unknown command");
	zassert_str_equal(run("HELP me"), "ERR: unknown command");
}

ZTEST(cmd, test_spaces_and_empty_line)
{
	zassert_equal(cmd_execute("", resp, sizeof(resp)), 0);
	zassert_equal(cmd_execute("   ", resp, sizeof(resp)), 0);
	zassert_str_equal(run("  LED ON  "), "OK");
}

ZTEST(cmd, test_hardware_error_is_reported)
{
	fake_led_err = -EIO;
	zassert_true(strncmp(run("LED ON"), "ERR:", 4) == 0);
}

ZTEST(cmd, test_small_buffer_truncates)
{
	char small[5];
	int len = cmd_execute("ECHO abcdefgh", small, sizeof(small));

	zassert_equal(len, 4);
	zassert_str_equal(small, "abcd");
}
