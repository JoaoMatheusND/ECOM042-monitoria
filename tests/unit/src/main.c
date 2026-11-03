#include <string.h>
#include <zephyr/ztest.h>

#include "dispatch.h"
#include "line_editor.h"

/* ====================== fakes de hardware ====================== */

static bool fake_led;
static bool fake_btn;
static int fake_rc;
static int led_set_calls;

static int f_led_set(bool on)
{
	led_set_calls++;
	if (fake_rc) {
		return fake_rc;
	}
	fake_led = on;
	return 0;
}

static int f_led_get(bool *on)
{
	if (fake_rc) {
		return fake_rc;
	}
	*on = fake_led;
	return 0;
}

static int f_btn_get(bool *pressed)
{
	if (fake_rc) {
		return fake_rc;
	}
	*pressed = fake_btn;
	return 0;
}

static const struct minish_hw_ops fake_ops = {
	.led_set = f_led_set,
	.led_get = f_led_get,
	.btn_get = f_btn_get,
};

static char out[256];

static int run(const char *line)
{
	return minish_dispatch(line, out, sizeof(out));
}

static void dispatch_before(void *unused)
{
	fake_led = false;
	fake_btn = false;
	fake_rc = 0;
	led_set_calls = 0;
	out[0] = '\0';
	minish_set_hw_ops(&fake_ops);
}

ZTEST_SUITE(dispatch, NULL, NULL, dispatch_before, NULL, NULL);

/* ====================== dispatch ====================== */

ZTEST(dispatch, test_help_lists_all_commands)
{
	zassert_equal(run("HELP"), 0);
	zassert_not_null(strstr(out, "HELP"));
	zassert_not_null(strstr(out, "LED"));
	zassert_not_null(strstr(out, "BTN"));
	zassert_not_null(strstr(out, "ECHO"));
}

ZTEST(dispatch, test_led_on_off)
{
	zassert_equal(run("LED ON"), 0);
	zassert_str_equal(out, "OK");
	zassert_true(fake_led);

	zassert_equal(run("LED OFF"), 0);
	zassert_str_equal(out, "OK");
	zassert_false(fake_led);
}

ZTEST(dispatch, test_led_query)
{
	fake_led = true;
	zassert_equal(run("LED"), 0);
	zassert_str_equal(out, "LED: ON");

	fake_led = false;
	zassert_equal(run("LED"), 0);
	zassert_str_equal(out, "LED: OFF");
}

ZTEST(dispatch, test_led_toggle)
{
	zassert_equal(run("LED TOGGLE"), 0);
	zassert_true(fake_led);
	zassert_equal(run("LED TOGGLE"), 0);
	zassert_false(fake_led);
}

ZTEST(dispatch, test_led_bad_arg)
{
	zassert_not_equal(run("LED BLINK"), 0);
	zassert_str_equal(out, "ERR: usage: LED [ON|OFF|TOGGLE]");
	zassert_not_equal(run("LED ON NOW"), 0);
	zassert_equal(led_set_calls, 0);
}

ZTEST(dispatch, test_case_insensitive)
{
	zassert_equal(run("led on"), 0);
	zassert_true(fake_led);
	zassert_equal(run("LeD oFf"), 0);
	zassert_false(fake_led);
	fake_btn = true;
	zassert_equal(run("bTn"), 0);
	zassert_str_equal(out, "BTN: PRESSED");
}

ZTEST(dispatch, test_btn)
{
	fake_btn = false;
	zassert_equal(run("BTN"), 0);
	zassert_str_equal(out, "BTN: RELEASED");
	fake_btn = true;
	zassert_equal(run("BTN"), 0);
	zassert_str_equal(out, "BTN: PRESSED");
	zassert_not_equal(run("BTN now"), 0);
}

ZTEST(dispatch, test_echo_keeps_text_and_case)
{
	zassert_equal(run("ECHO oi"), 0);
	zassert_str_equal(out, "oi");
	zassert_equal(run("echo  Ola   Mundo  "), 0);
	zassert_str_equal(out, "Ola   Mundo");
	zassert_not_equal(run("ECHO"), 0);
	zassert_str_equal(out, "ERR: usage: ECHO <text>");
}

ZTEST(dispatch, test_unknown_command)
{
	zassert_equal(run("FOO"), -ENOENT);
	zassert_str_equal(out, "ERR: unknown command");
	zassert_equal(run("LEDX ON"), -ENOENT);
}

ZTEST(dispatch, test_blank_line_has_no_response)
{
	zassert_equal(run(""), 0);
	zassert_str_equal(out, "");
	zassert_equal(run("   \t "), 0);
	zassert_str_equal(out, "");
}

ZTEST(dispatch, test_leading_and_trailing_whitespace)
{
	zassert_equal(run("  LED   ON  "), 0);
	zassert_true(fake_led);
}

ZTEST(dispatch, test_hw_errors)
{
	fake_rc = -5;
	zassert_equal(run("LED ON"), -EIO);
	zassert_str_equal(out, "ERR: led failure");
	zassert_equal(run("BTN"), -EIO);
	zassert_str_equal(out, "ERR: button failure");
}

ZTEST(dispatch, test_no_hw_ops)
{
	minish_set_hw_ops(NULL);
	zassert_equal(run("LED ON"), -ENODEV);
	zassert_equal(run("BTN"), -ENODEV);
	zassert_equal(run("ECHO ok"), 0); /* nao depende de hardware */
}

ZTEST(dispatch, test_small_output_buffer_never_overflows)
{
	char small[8];
	char guard[4] = "###";

	zassert_equal(minish_dispatch("ECHO 0123456789abcdef", small, sizeof(small)), 0);
	zassert_equal(strlen(small), sizeof(small) - 1);
	zassert_str_equal(guard, "###");
	zassert_equal(minish_dispatch("HELP", small, sizeof(small)), 0);
	zassert_equal(minish_dispatch("X", small, 0), -EINVAL);
}

/* ====================== line editor ====================== */

static struct line_editor le;

static enum le_event feed_str(const char *s)
{
	enum le_event ev = LE_NONE;

	while (*s) {
		ev = line_editor_feed(&le, (uint8_t)*s++);
	}
	return ev;
}

static void le_before(void *unused)
{
	line_editor_init(&le);
}

ZTEST_SUITE(line_editor, NULL, NULL, le_before, NULL, NULL);

ZTEST(line_editor, test_simple_line_lf)
{
	zassert_equal(feed_str("LED ON\n"), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "LED ON");
}

ZTEST(line_editor, test_simple_line_cr)
{
	zassert_equal(feed_str("BTN\r"), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "BTN");
}

ZTEST(line_editor, test_chars_are_echoed)
{
	zassert_equal(line_editor_feed(&le, 'a'), LE_ECHO);
}

ZTEST(line_editor, test_crlf_is_one_line)
{
	int lines = 0;
	const char *s = "ab\r\n";

	while (*s) {
		if (line_editor_feed(&le, (uint8_t)*s++) == LE_LINE) {
			lines++;
		}
	}
	zassert_equal(lines, 1);
}

ZTEST(line_editor, test_empty_line)
{
	zassert_equal(feed_str("\r"), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "");
}

ZTEST(line_editor, test_backspace_08_and_7f)
{
	zassert_equal(feed_str("abc"), LE_ECHO);
	zassert_equal(line_editor_feed(&le, 0x08), LE_BACKSPACE);
	zassert_equal(line_editor_feed(&le, 0x7F), LE_BACKSPACE);
	feed_str("x");
	zassert_equal(line_editor_feed(&le, '\n'), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "ax");
}

ZTEST(line_editor, test_backspace_on_empty_is_ignored)
{
	zassert_equal(line_editor_feed(&le, 0x7F), LE_NONE);
	zassert_equal(line_editor_feed(&le, 0x08), LE_NONE);
}

ZTEST(line_editor, test_exactly_max_is_accepted)
{
	char big[MINISH_LINE_MAX + 2];

	memset(big, 'a', MINISH_LINE_MAX);
	big[MINISH_LINE_MAX] = '\n';
	big[MINISH_LINE_MAX + 1] = '\0';
	zassert_equal(feed_str(big), LE_LINE);
	zassert_equal(strlen(line_editor_line(&le)), MINISH_LINE_MAX);
}

ZTEST(line_editor, test_over_max_is_discarded_entirely)
{
	char big[MINISH_LINE_MAX + 12];

	memset(big, 'a', sizeof(big) - 2);
	big[sizeof(big) - 2] = '\n';
	big[sizeof(big) - 1] = '\0';
	zassert_equal(feed_str(big), LE_TOO_LONG);

	/* a linha seguinte nao herda nada da descartada */
	zassert_equal(feed_str("HELP\n"), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "HELP");
}

ZTEST(line_editor, test_max_plus_one_is_too_long)
{
	char big[MINISH_LINE_MAX + 3];

	memset(big, 'a', MINISH_LINE_MAX + 1);
	big[MINISH_LINE_MAX + 1] = '\r';
	big[MINISH_LINE_MAX + 2] = '\0';
	zassert_equal(feed_str(big), LE_TOO_LONG);
}

ZTEST(line_editor, test_too_long_with_crlf_then_next_line)
{
	char big[MINISH_LINE_MAX + 5];

	memset(big, 'a', MINISH_LINE_MAX + 2);
	big[MINISH_LINE_MAX + 2] = '\r';
	big[MINISH_LINE_MAX + 3] = '\n';
	big[MINISH_LINE_MAX + 4] = '\0';
	zassert_equal(feed_str(big), LE_NONE); /* o '\n' do CRLF e engolido */
	zassert_equal(feed_str("BTN\n"), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "BTN");
}

ZTEST(line_editor, test_control_and_non_ascii_ignored)
{
	zassert_equal(line_editor_feed(&le, 0x01), LE_NONE);
	zassert_equal(line_editor_feed(&le, 0xC3), LE_NONE);
	zassert_equal(line_editor_feed(&le, '\n'), LE_LINE);
	zassert_str_equal(line_editor_line(&le), "");
}
