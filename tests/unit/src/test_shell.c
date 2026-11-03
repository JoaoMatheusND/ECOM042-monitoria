#include <string.h>
#include <zephyr/ztest.h>

#include "shell.h"

static bool led_state;
static int btn_state;
static int64_t uptime_ms;

void io_led_set(bool on)
{
	led_state = on;
}

bool io_led_get(void)
{
	return led_state;
}

int io_btn_get(void)
{
	return btn_state;
}

int64_t io_uptime_get(void)
{
	return uptime_ms;
}

ZTEST_SUITE(shell_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(shell_suite, test_help)
{
	char out[128];

	shell_dispatch("HELP", out, sizeof(out));
	zassert_true(strlen(out) > 0);
	zassert_not_null(strstr(out, "HELP"));
	zassert_not_null(strstr(out, "LED"));
}

ZTEST(shell_suite, test_led_on_off)
{
	char out[64];

	shell_dispatch("LED ON", out, sizeof(out));
	zassert_str_equal("OK", out);
	zassert_true(led_state);

	shell_dispatch("led off", out, sizeof(out));
	zassert_str_equal("OK", out);
	zassert_false(led_state);
}

ZTEST(shell_suite, test_led_state)
{
	char out[64];

	led_state = true;
	shell_dispatch("LED", out, sizeof(out));
	zassert_str_equal("LED: ON", out);

	led_state = false;
	shell_dispatch("LED", out, sizeof(out));
	zassert_str_equal("LED: OFF", out);
}

ZTEST(shell_suite, test_btn)
{
	char out[64];

	btn_state = 1;
	shell_dispatch("BTN", out, sizeof(out));
	zassert_str_equal("BTN: 1", out);

	btn_state = 0;
	shell_dispatch("BTN", out, sizeof(out));
	zassert_str_equal("BTN: 0", out);
}

ZTEST(shell_suite, test_uptime)
{
	char out[64];

	uptime_ms = 12345;
	shell_dispatch("UPTIME", out, sizeof(out));
	zassert_str_equal("UPTIME: 12345 ms", out);
}

ZTEST(shell_suite, test_echo)
{
	char out[64];

	shell_dispatch("ECHO hello world", out, sizeof(out));
	zassert_str_equal("hello world", out);
}

ZTEST(shell_suite, test_unknown)
{
	char out[64];

	shell_dispatch("FOO", out, sizeof(out));
	zassert_str_equal("ERR: unknown command", out);
}

ZTEST(shell_suite, test_case_insensitive)
{
	char out[64];

	shell_dispatch("help", out, sizeof(out));
	zassert_true(strlen(out) > 0);

	shell_dispatch("Echo MixEd", out, sizeof(out));
	zassert_str_equal("MixEd", out);
}
