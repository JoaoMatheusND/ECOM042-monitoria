#include <zephyr/ztest.h>
#include <string.h>

#include "core/line_editor.h"

#define BUF_SZ 8 /* maximo de 7 caracteres */

static struct line_editor le;
static char buf[BUF_SZ];

static void before(void *f)
{
	ARG_UNUSED(f);
	line_editor_init(&le, buf, sizeof(buf));
}

/* alimenta uma string e devolve o ultimo evento */
static enum line_ev feed(const char *s)
{
	enum line_ev ev = LINE_EV_NONE;

	for (; *s; s++) {
		ev = line_editor_feed(&le, (uint8_t)*s);
	}
	return ev;
}

ZTEST(line_editor, test_char_is_echoed)
{
	zassert_equal(line_editor_feed(&le, 'a'), LINE_EV_ECHO);
}

ZTEST(line_editor, test_lf_terminates)
{
	zassert_equal(feed("abc\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "abc");
}

ZTEST(line_editor, test_cr_terminates)
{
	zassert_equal(feed("abc\r"), LINE_EV_LINE);
	zassert_str_equal(buf, "abc");
}

ZTEST(line_editor, test_crlf_is_single_terminator)
{
	zassert_equal(feed("abc\r"), LINE_EV_LINE);
	zassert_equal(line_editor_feed(&le, '\n'), LINE_EV_NONE);
}

ZTEST(line_editor, test_empty_line)
{
	zassert_equal(feed("\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "");
}

ZTEST(line_editor, test_backspace_0x08)
{
	zassert_equal(feed("ab"), LINE_EV_ECHO);
	zassert_equal(line_editor_feed(&le, 0x08), LINE_EV_ERASE);
	zassert_equal(feed("c\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "ac");
}

ZTEST(line_editor, test_backspace_0x7f)
{
	zassert_equal(feed("ab"), LINE_EV_ECHO);
	zassert_equal(line_editor_feed(&le, 0x7F), LINE_EV_ERASE);
	zassert_equal(feed("c\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "ac");
}

ZTEST(line_editor, test_backspace_on_empty_does_nothing)
{
	zassert_equal(line_editor_feed(&le, 0x7F), LINE_EV_NONE);
	zassert_equal(feed("x\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "x");
}

ZTEST(line_editor, test_control_chars_ignored)
{
	zassert_equal(line_editor_feed(&le, '\t'), LINE_EV_NONE);
	zassert_equal(line_editor_feed(&le, 0x1B), LINE_EV_NONE);
	zassert_equal(feed("a\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "a");
}

ZTEST(line_editor, test_max_length_line_is_accepted)
{
	zassert_equal(feed("1234567"), LINE_EV_ECHO);
	zassert_equal(feed("\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "1234567");
}

ZTEST(line_editor, test_too_long_line_is_discarded_whole)
{
	zassert_equal(feed("1234567"), LINE_EV_ECHO);
	zassert_equal(line_editor_feed(&le, '8'), LINE_EV_NONE); /* estourou */
	zassert_equal(feed("9999"), LINE_EV_NONE);               /* descartando */
	zassert_equal(feed("\n"), LINE_EV_TOO_LONG);
}

ZTEST(line_editor, test_recovers_after_too_long)
{
	zassert_equal(feed("123456789\n"), LINE_EV_TOO_LONG);
	zassert_equal(feed("ok\n"), LINE_EV_LINE);
	zassert_str_equal(buf, "ok");
}

ZTEST(line_editor, test_backspace_ignored_while_overflowing)
{
	zassert_equal(feed("12345678"), LINE_EV_NONE);
	zassert_equal(line_editor_feed(&le, 0x7F), LINE_EV_NONE);
	zassert_equal(feed("\r"), LINE_EV_TOO_LONG);
}

ZTEST_SUITE(line_editor, NULL, NULL, before, NULL, NULL);
