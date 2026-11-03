/*******************************************************************
 * @file test_cmd.c
 *
 * @brief Testes do modulo de parsing/dispatch (cmd.c) com I/O falso.
 *******************************************************************/

#include "cmd.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static bool fake_led;
static int fake_btn;
static int fake_err;
static int led_set_calls;

static int fake_led_set(bool on)
{
	led_set_calls++;
	if (fake_err) {
		return fake_err;
	}
	fake_led = on;
	return 0;
}

static int fake_led_get(void)
{
	return fake_err ? fake_err : fake_led;
}

static int fake_btn_get(void)
{
	return fake_err ? fake_err : fake_btn;
}

static int64_t fake_uptime(void)
{
	return 123456789012LL;
}

static const struct cmd_io io = {
	.led_set = fake_led_set,
	.led_get = fake_led_get,
	.btn_get = fake_btn_get,
	.uptime_ms = fake_uptime,
};

static char out[256];

static int run(const char *line)
{
	memset(out, 0xAA, sizeof(out));
	return cmd_execute(&io, line, out, sizeof(out));
}

static void cmd_before(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_led = false;
	fake_btn = 0;
	fake_err = 0;
	led_set_calls = 0;
}

ZTEST(cmd, test_help_lists_one_line_per_command)
{
	zassert_ok(run("HELP"));
	zassert_not_null(strstr(out, "HELP"));
	zassert_not_null(strstr(out, "LED"));
	zassert_not_null(strstr(out, "BTN"));
	zassert_not_null(strstr(out, "UPTIME"));
	zassert_not_null(strstr(out, "ECHO"));

	/* 5 comandos -> 4 separadores de linha, sem "\r\n" no final */
	int lines = 1;

	for (const char *p = out; (p = strstr(p, "\r\n")) != NULL; p += 2) {
		lines++;
	}
	zassert_equal(lines, 5);
	zassert_false(out[strlen(out) - 1] == '\n');
}

ZTEST(cmd, test_led_on_off)
{
	zassert_ok(run("LED ON"));
	zassert_str_equal(out, "OK");
	zassert_true(fake_led);

	zassert_ok(run("LED OFF"));
	zassert_str_equal(out, "OK");
	zassert_false(fake_led);
}

ZTEST(cmd, test_led_status)
{
	zassert_ok(run("LED"));
	zassert_str_equal(out, "LED: OFF");

	fake_led = true;
	zassert_ok(run("LED"));
	zassert_str_equal(out, "LED: ON");
}

ZTEST(cmd, test_case_insensitive)
{
	zassert_ok(run("led on"));
	zassert_true(fake_led);
	zassert_ok(run("Led Off"));
	zassert_false(fake_led);
	zassert_ok(run("help"));
	zassert_ok(run("bTn"));
	zassert_str_equal(out, "BTN: 0");
	zassert_ok(run("uptime"));
	zassert_ok(run("echo x"));
	zassert_str_equal(out, "x");
}

ZTEST(cmd, test_whitespace_around_command)
{
	zassert_ok(run("   LED   ON  "));
	zassert_str_equal(out, "OK");
	zassert_true(fake_led);
	zassert_ok(run("\tBTN\t"));
	zassert_str_equal(out, "BTN: 0");
}

ZTEST(cmd, test_led_invalid_argument)
{
	zassert_equal(run("LED BLINK"), -EINVAL);
	zassert_str_equal(out, "ERR: unknown command");
	zassert_equal(led_set_calls, 0);

	zassert_equal(run("LED ONN"), -EINVAL);
	zassert_equal(run("LED ON OFF"), -EINVAL);
	zassert_equal(led_set_calls, 0);
}

ZTEST(cmd, test_btn)
{
	zassert_ok(run("BTN"));
	zassert_str_equal(out, "BTN: 0");

	fake_btn = 1;
	zassert_ok(run("BTN"));
	zassert_str_equal(out, "BTN: 1");
}

ZTEST(cmd, test_uptime_uses_64_bits)
{
	zassert_ok(run("UPTIME"));
	zassert_str_equal(out, "UPTIME: 123456789012 ms");
}

ZTEST(cmd, test_echo_exact_text)
{
	zassert_ok(run("ECHO oi"));
	zassert_str_equal(out, "oi");

	zassert_ok(run("ECHO  Ola, MUNDO!  "));
	zassert_str_equal(out, " Ola, MUNDO!  ");

	zassert_ok(run("ECHO"));
	zassert_str_equal(out, "");

	/* sem separador nao e o comando ECHO */
	zassert_equal(run("ECHOoi"), -EINVAL);
	zassert_str_equal(out, "ERR: unknown command");
}

ZTEST(cmd, test_unknown_command)
{
	zassert_equal(run("FOO"), -EINVAL);
	zassert_str_equal(out, "ERR: unknown command");

	zassert_equal(run(""), -EINVAL);
	zassert_str_equal(out, "ERR: unknown command");

	zassert_equal(run("BTN 1"), -EINVAL);
	zassert_equal(run("HELP me"), -EINVAL);
	zassert_equal(run("UPTIME now"), -EINVAL);
	zassert_equal(run("LE"), -EINVAL);
}

ZTEST(cmd, test_hardware_error_is_reported)
{
	fake_err = -EIO;

	zassert_equal(run("LED ON"), -EIO);
	zassert_str_equal(out, "ERR: led");
	zassert_equal(run("LED"), -EIO);
	zassert_str_equal(out, "ERR: led");
	zassert_equal(run("BTN"), -EIO);
	zassert_str_equal(out, "ERR: btn");
}

ZTEST(cmd, test_small_output_buffer)
{
	char small[4];

	zassert_equal(cmd_execute(&io, "HELP", small, sizeof(small)), -ENOMEM);
	zassert_equal(cmd_execute(&io, "LED", small, sizeof(small)), -ENOMEM);
	zassert_equal(small[sizeof(small) - 1], '\0');
	zassert_equal(cmd_execute(&io, "LED", small, 0), -EINVAL);
	zassert_equal(cmd_execute(&io, "LED", NULL, 8), -EINVAL);
}

ZTEST_SUITE(cmd, NULL, NULL, cmd_before, NULL, NULL);
