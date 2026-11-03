/*******************************************************************
 * @file test_line.c
 *
 * @brief Testes da montagem de linha (line.c).
 *******************************************************************/

#include "line.h"

#include <string.h>

#include <zephyr/ztest.h>

static struct line_editor ed;
static char echo_all[512];
static size_t echo_all_len;

/* Alimenta a string byte a byte e devolve o status do ultimo byte. */
static enum line_status feed(const char *s)
{
	enum line_status st = LINE_PENDING;
	char echo[LINE_ECHO_MAX];
	size_t len;

	for (; *s != '\0'; s++) {
		st = line_feed(&ed, (uint8_t)*s, echo, &len);
		zassert_true(len <= LINE_ECHO_MAX);
		zassert_true(echo_all_len + len <= sizeof(echo_all));
		memcpy(echo_all + echo_all_len, echo, len);
		echo_all_len += len;
	}

	echo_all[echo_all_len] = '\0';
	return st;
}

static void feed_repeat(char c, size_t n)
{
	char s[2] = {c, '\0'};

	for (size_t i = 0; i < n; i++) {
		zassert_equal(feed(s), LINE_PENDING);
	}
}

static void line_before(void *fixture)
{
	ARG_UNUSED(fixture);
	line_init(&ed);
	echo_all_len = 0;
	echo_all[0] = '\0';
}

ZTEST(line, test_buffer_size_from_kconfig)
{
	zassert_equal(sizeof(ed.buf), 64);
}

ZTEST(line, test_line_terminated_by_cr_or_lf)
{
	zassert_equal(feed("HELP\r"), LINE_READY);
	zassert_str_equal(ed.buf, "HELP");

	zassert_equal(feed("BTN\n"), LINE_READY);
	zassert_str_equal(ed.buf, "BTN");
}

ZTEST(line, test_crlf_is_single_line_end)
{
	zassert_equal(feed("LED\r"), LINE_READY);
	zassert_equal(feed("\n"), LINE_PENDING);
	zassert_equal(feed("BTN\r\n"), LINE_PENDING);
	zassert_str_equal(ed.buf, "BTN");
	zassert_str_equal(echo_all, "LED\r\nBTN\r\n");
}

ZTEST(line, test_lf_lf_gives_empty_line)
{
	zassert_equal(feed("\n"), LINE_READY);
	zassert_str_equal(ed.buf, "");
	zassert_equal(feed("\n"), LINE_READY);
	zassert_str_equal(ed.buf, "");
}

ZTEST(line, test_echo_of_typed_chars)
{
	zassert_equal(feed("ab"), LINE_PENDING);
	zassert_str_equal(echo_all, "ab");
	zassert_equal(feed("\r"), LINE_READY);
	zassert_str_equal(echo_all, "ab\r\n");
}

ZTEST(line, test_backspace_0x08_and_0x7f)
{
	zassert_equal(feed("LEDX\b"), LINE_PENDING);
	zassert_str_equal(echo_all, "LEDX\b \b");
	zassert_equal(feed(" OFX\x7f\x7fN\r"), LINE_READY);
	zassert_str_equal(ed.buf, "LED ON");
}

ZTEST(line, test_backspace_on_empty_line_does_nothing)
{
	zassert_equal(feed("\b\x7f\b"), LINE_PENDING);
	zassert_str_equal(echo_all, "");
	zassert_equal(feed("A\r"), LINE_READY);
	zassert_str_equal(ed.buf, "A");
}

ZTEST(line, test_longest_line_that_fits)
{
	feed_repeat('A', LINE_BUF_SIZE - 1);
	zassert_equal(feed("\r"), LINE_READY);
	zassert_equal(strlen(ed.buf), LINE_BUF_SIZE - 1);
}

ZTEST(line, test_line_too_long_is_discarded)
{
	feed_repeat('A', LINE_BUF_SIZE);
	zassert_equal(feed("\r"), LINE_TOO_LONG);

	/* a proxima linha comeca limpa */
	zassert_equal(feed("BTN\r"), LINE_READY);
	zassert_str_equal(ed.buf, "BTN");
}

ZTEST(line, test_backspace_does_not_rescue_long_line)
{
	feed_repeat('A', LINE_BUF_SIZE + 10);
	feed_repeat('\b', 20);
	zassert_equal(feed("\n"), LINE_TOO_LONG);
}

ZTEST(line, test_very_long_line)
{
	feed_repeat('x', 300);
	zassert_equal(feed("\r\n"), LINE_PENDING);
	zassert_equal(feed("LED\r"), LINE_READY);
	zassert_str_equal(ed.buf, "LED");
}

ZTEST_SUITE(line, NULL, NULL, line_before, NULL, NULL);
