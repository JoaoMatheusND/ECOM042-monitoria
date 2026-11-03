/*******************************************************************
 * @file test_main.c
 *
 * @brief Testes ztest do parser (minishell) e do editor de linha.
 *******************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "line_editor.h"
#include "minishell.h"

static bool fake_led;
static int fake_button;

static int fake_led_set(bool on)
{
	fake_led = on;
	return 0;
}

static int fake_led_get(bool *on)
{
	*on = fake_led;
	return 0;
}

static int fake_button_read(int *pressed)
{
	*pressed = fake_button;
	return 0;
}

static const struct minishell_hw_ops fake_ops = {
	.led_set = fake_led_set,
	.led_get = fake_led_get,
	.button_read = fake_button_read,
};

static char out[CONFIG_MINISHELL_RESPONSE_SIZE];

static size_t run(const char *line)
{
	memset(out, 0, sizeof(out));
	return minishell_execute(line, out, sizeof(out));
}

ZTEST(minishell, test_help_lists_every_command)
{
	zassert_true(run("HELP") > 0U, "HELP nao respondeu nada");
	zassert_not_null(strstr(out, "LED"), "sem LED: %s", out);
	zassert_not_null(strstr(out, "BTN"), "sem BTN: %s", out);
	zassert_not_null(strstr(out, "UPTIME"), "sem UPTIME: %s", out);
	zassert_not_null(strstr(out, "ECHO"), "sem ECHO: %s", out);
}

ZTEST(minishell, test_unknown_command)
{
	zassert_true(run("FOO") > 0U);
	zassert_str_equal(out, MINISHELL_ERR_UNKNOWN_COMMAND);
}

ZTEST(minishell, test_empty_line_has_no_response)
{
	zassert_equal(run(""), 0U);
	zassert_equal(run("   "), 0U);
}

ZTEST(minishell, test_led_on_off_and_status)
{
	fake_led = false;

	zassert_true(run("LED ON") > 0U);
	zassert_str_equal(out, "OK");
	zassert_true(fake_led, "LED ON nao chegou no hardware");

	zassert_true(run("LED") > 0U);
	zassert_str_equal(out, "LED: ON");

	zassert_true(run("LED OFF") > 0U);
	zassert_str_equal(out, "OK");
	zassert_false(fake_led, "LED OFF nao chegou no hardware");

	zassert_true(run("LED") > 0U);
	zassert_str_equal(out, "LED: OFF");
}

ZTEST(minishell, test_led_invalid_argument)
{
	zassert_true(run("LED BLINK") > 0U);
	zassert_str_equal(out, MINISHELL_ERR_UNKNOWN_COMMAND);
}

ZTEST(minishell, test_echo_without_text_has_no_response)
{
	zassert_equal(run("ECHO"), 0U);
	zassert_str_equal(out, "");
}

ZTEST(minishell, test_echo_accepts_line_at_64_bytes)
{
	char line[CONFIG_MINISHELL_LINE_BUF_SIZE + 1];
	const size_t prefix = 5U; /* "ECHO " */

	memset(line, 'x', sizeof(line) - 1U);
	line[sizeof(line) - 1U] = '\0';
	memcpy(line, "ECHO ", prefix);

	/* A linha tem exatamente CONFIG_MINISHELL_LINE_BUF_SIZE bytes. */
	zassert_equal(strlen(line), CONFIG_MINISHELL_LINE_BUF_SIZE);
	zassert_true(run(line) > 0U);
	zassert_equal(strlen(out), CONFIG_MINISHELL_LINE_BUF_SIZE - prefix);
}

ZTEST(minishell, test_button_reads_value)
{
	fake_button = 1;
	zassert_true(run("BTN") > 0U);
	zassert_str_equal(out, "BTN: 1");

	fake_button = 0;
	zassert_true(run("BTN") > 0U);
	zassert_str_equal(out, "BTN: 0");
}

ZTEST(minishell, test_uptime_format)
{
	zassert_true(run("UPTIME") > 0U);
	zassert_true(strncmp(out, "UPTIME: ", 8) == 0, "formato inesperado: %s", out);
	zassert_true(strstr(out, " ms") != NULL, "sem unidade: %s", out);
}

ZTEST(minishell, test_echo_repeats_text_verbatim)
{
	zassert_true(run("ECHO oi") > 0U);
	zassert_str_equal(out, "oi");

	zassert_true(run("ECHO Ola  Mundo") > 0U);
	zassert_str_equal(out, "Ola  Mundo");
}

ZTEST(minishell, test_commands_are_case_insensitive)
{
	fake_led = false;

	zassert_true(run("help") > 0U);
	zassert_not_null(strstr(out, "ECHO"));

	zassert_true(run("led on") > 0U);
	zassert_str_equal(out, "OK");
	zassert_true(fake_led);

	zassert_true(run("echo Oi") > 0U);
	zassert_str_equal(out, "Oi");
}

ZTEST(line_editor, test_assembles_line_and_reports_echo)
{
	struct line_editor editor;
	const char *text = "HELP";
	size_t i;

	line_editor_reset(&editor);

	for (i = 0U; i < strlen(text); i++) {
		zassert_equal(line_editor_feed(&editor, text[i]), LINE_EDITOR_EVENT_ECHO);
	}

	zassert_equal(line_editor_feed(&editor, '\r'), LINE_EDITOR_EVENT_LINE_READY);
	zassert_str_equal(line_editor_text(&editor), "HELP");
}

ZTEST(line_editor, test_backspace_removes_last_char)
{
	struct line_editor editor;

	line_editor_reset(&editor);

	zassert_equal(line_editor_feed(&editor, 'a'), LINE_EDITOR_EVENT_ECHO);
	zassert_equal(line_editor_feed(&editor, 'b'), LINE_EDITOR_EVENT_ECHO);
	zassert_equal(line_editor_feed(&editor, 0x7F), LINE_EDITOR_EVENT_ERASE);
	zassert_equal(line_editor_feed(&editor, 'c'), LINE_EDITOR_EVENT_ECHO);
	zassert_equal(line_editor_feed(&editor, '\n'), LINE_EDITOR_EVENT_LINE_READY);
	zassert_str_equal(line_editor_text(&editor), "ac");
}

ZTEST(line_editor, test_backspace_on_empty_line_is_ignored)
{
	struct line_editor editor;

	line_editor_reset(&editor);

	zassert_equal(line_editor_feed(&editor, 0x08), LINE_EDITOR_EVENT_IGNORED);
	zassert_equal(line_editor_length(&editor), 0U);
}

ZTEST(line_editor, test_line_at_limit_is_accepted)
{
	struct line_editor editor;
	size_t i;

	line_editor_reset(&editor);

	for (i = 0U; i < CONFIG_MINISHELL_LINE_BUF_SIZE; i++) {
		zassert_equal(line_editor_feed(&editor, 'x'), LINE_EDITOR_EVENT_ECHO);
	}

	zassert_equal(line_editor_feed(&editor, '\r'), LINE_EDITOR_EVENT_LINE_READY);
	zassert_equal(line_editor_length(&editor), CONFIG_MINISHELL_LINE_BUF_SIZE);
}

ZTEST(line_editor, test_line_too_long_is_dropped_entirely)
{
	struct line_editor editor;
	size_t i;

	line_editor_reset(&editor);

	for (i = 0U; i < (CONFIG_MINISHELL_LINE_BUF_SIZE + 1U); i++) {
		(void)line_editor_feed(&editor, 'x');
	}

	zassert_equal(line_editor_feed(&editor, '\r'), LINE_EDITOR_EVENT_LINE_TOO_LONG);
	zassert_equal(line_editor_length(&editor), 0U);
	zassert_str_equal(line_editor_text(&editor), "");
}

ZTEST(line_editor, test_enter_on_empty_line_is_ignored)
{
	struct line_editor editor;

	line_editor_reset(&editor);

	zassert_equal(line_editor_feed(&editor, '\n'), LINE_EDITOR_EVENT_IGNORED);
}

static void *minishell_setup(void)
{
	minishell_init(&fake_ops);
	return NULL;
}

ZTEST_SUITE(minishell, NULL, minishell_setup, NULL, NULL, NULL);
ZTEST_SUITE(line_editor, NULL, NULL, NULL, NULL, NULL);
