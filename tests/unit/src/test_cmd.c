/*******************************************************************
 * @file test_cmd.c
 *
 * @brief Testes do parsing/dispatch com um HAL falso (sem GPIO).
 *******************************************************************/

#include <string.h>

#include <zephyr/ztest.h>

#include "cmd.h"

static bool fake_led;
static int fake_btn;
static int led_set_calls;

static int fake_led_set(bool on)
{
	led_set_calls++;
	fake_led = on;
	return 0;
}

static bool fake_led_get(void)
{
	return fake_led;
}

static int fake_btn_get(void)
{
	return fake_btn;
}

static int64_t fake_uptime(void)
{
	return 12345;
}

static const struct cmd_hal fake_hal = {
	.led_set = fake_led_set,
	.led_get = fake_led_get,
	.btn_get = fake_btn_get,
	.uptime_ms = fake_uptime,
};

static char out[256];

static const char *run(const char *line)
{
	int n = cmd_dispatch(line, out, sizeof(out));

	zassert_equal(n, (int)strlen(out), "retorno deve ser o tamanho da resposta");
	return out;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_led = false;
	fake_btn = 0;
	led_set_calls = 0;
	memset(out, 0, sizeof(out));
	cmd_init(&fake_hal);
}

ZTEST(cmd, test_help_lists_one_command_per_line)
{
	const char *r = run("HELP");

	zassert_not_null(strstr(r, "HELP"));
	zassert_not_null(strstr(r, "LED"));
	zassert_not_null(strstr(r, "BTN"));
	zassert_not_null(strstr(r, "UPTIME"));
	zassert_not_null(strstr(r, "ECHO"));
	zassert_not_equal(strncmp(r + strlen(r) - 2, "\r\n", 2), 0,
			  "o \\r\\n final e do transporte");
}

ZTEST(cmd, test_led_on_off)
{
	zassert_str_equal(run("LED ON"), "OK");
	zassert_true(fake_led);
	zassert_str_equal(run("LED"), "LED: ON");

	zassert_str_equal(run("LED OFF"), "OK");
	zassert_false(fake_led);
	zassert_str_equal(run("LED"), "LED: OFF");
	zassert_equal(led_set_calls, 2);
}

ZTEST(cmd, test_case_insensitive)
{
	zassert_str_equal(run("led on"), "OK");
	zassert_str_equal(run("Led"), "LED: ON");
	zassert_str_equal(run("lEd oFf"), "OK");
	zassert_str_equal(run("btn"), "BTN: 0");
}

ZTEST(cmd, test_btn)
{
	zassert_str_equal(run("BTN"), "BTN: 0");
	fake_btn = 1;
	zassert_str_equal(run("BTN"), "BTN: 1");
}

ZTEST(cmd, test_uptime)
{
	zassert_str_equal(run("UPTIME"), "UPTIME: 12345 ms");
}

ZTEST(cmd, test_echo_keeps_text)
{
	zassert_str_equal(run("ECHO oi"), "oi");
	zassert_str_equal(run("echo Ola  Mundo "), "Ola  Mundo ");
	zassert_str_equal(run("ECHO"), "");
}

ZTEST(cmd, test_unknown)
{
	zassert_str_equal(run("FOO"), "ERR: unknown command");
	zassert_str_equal(run("LEDON"), "ERR: unknown command");
	zassert_str_equal(run("LED BLINK"), "ERR: unknown command");
	zassert_str_equal(run("BTN 1"), "ERR: unknown command");
	zassert_equal(led_set_calls, 0);
}

ZTEST(cmd, test_whitespace)
{
	zassert_str_equal(run("   LED   ON  "), "OK");
	zassert_equal(cmd_dispatch("", out, sizeof(out)), 0);
	zassert_equal(cmd_dispatch("   ", out, sizeof(out)), 0);
}

ZTEST(cmd, test_small_buffer_is_truncated)
{
	char small[4];
	int n = cmd_dispatch("ECHO abcdef", small, sizeof(small));

	zassert_equal(n, 3);
	zassert_str_equal(small, "abc");
}

ZTEST_SUITE(cmd, NULL, NULL, before, NULL, NULL);
