/*******************************************************************
 * @file test_line.c
 *
 * @brief Testes do editor de linha (terminador, backspace, estouro).
 *******************************************************************/

#include <string.h>

#include <zephyr/ztest.h>

#include "line.h"

static struct line_editor le;

/* Alimenta a string inteira e devolve o evento do ultimo byte. */
static enum line_event feed_str(const char *s)
{
	enum line_event ev = LINE_EV_NONE;

	while (*s != '\0') {
		ev = line_feed(&le, *s++);
	}
	return ev;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	line_init(&le);
}

ZTEST(line, test_line_ends_with_lf)
{
	zassert_equal(feed_str("HELP\n"), LINE_EV_DONE);
	zassert_str_equal(line_get(&le), "HELP");
}

ZTEST(line, test_line_ends_with_cr)
{
	zassert_equal(feed_str("BTN\r"), LINE_EV_DONE);
	zassert_str_equal(line_get(&le), "BTN");
}

ZTEST(line, test_crlf_is_single_terminator)
{
	zassert_equal(feed_str("LED\r"), LINE_EV_DONE);
	line_reset(&le);
	zassert_equal(line_feed(&le, '\n'), LINE_EV_NONE, "\\n apos \\r deve ser ignorado");
	zassert_equal(feed_str("BTN\n"), LINE_EV_DONE);
	zassert_str_equal(line_get(&le), "BTN");
}

ZTEST(line, test_char_events)
{
	zassert_equal(line_feed(&le, 'a'), LINE_EV_CHAR);
	zassert_equal(line_feed(&le, 0x01), LINE_EV_NONE, "controle deve ser ignorado");
	zassert_equal(le.len, 1);
}

ZTEST(line, test_backspace_bs_and_del)
{
	feed_str("LEDX");
	zassert_equal(line_feed(&le, 0x08), LINE_EV_ERASE);
	feed_str(" ONN");
	zassert_equal(line_feed(&le, 0x7F), LINE_EV_ERASE);
	zassert_equal(feed_str("\n"), LINE_EV_DONE);
	zassert_str_equal(line_get(&le), "LED ON");
}

ZTEST(line, test_backspace_on_empty_line)
{
	zassert_equal(line_feed(&le, 0x7F), LINE_EV_NONE);
	zassert_equal(le.len, 0);
}

ZTEST(line, test_exactly_max_is_accepted)
{
	char s[CONFIG_APP_LINE_BUF_SIZE + 1];

	memset(s, 'a', CONFIG_APP_LINE_BUF_SIZE);
	s[CONFIG_APP_LINE_BUF_SIZE] = '\0';

	feed_str(s);
	zassert_equal(line_feed(&le, '\n'), LINE_EV_DONE);
	zassert_equal(strlen(line_get(&le)), CONFIG_APP_LINE_BUF_SIZE);
}

ZTEST(line, test_too_long_is_discarded)
{
	for (int i = 0; i < CONFIG_APP_LINE_BUF_SIZE + 10; i++) {
		line_feed(&le, 'x');
	}
	zassert_equal(line_feed(&le, 0x7F), LINE_EV_NONE, "backspace nao recupera a linha");
	zassert_equal(line_feed(&le, '\r'), LINE_EV_TOO_LONG);

	/* A linha seguinte comeca limpa. */
	zassert_equal(feed_str("BTN\r"), LINE_EV_DONE);
	zassert_str_equal(line_get(&le), "BTN");
}

ZTEST(line, test_empty_line)
{
	zassert_equal(line_feed(&le, '\n'), LINE_EV_DONE);
	zassert_str_equal(line_get(&le), "");
}

ZTEST_SUITE(line, NULL, NULL, before, NULL, NULL);
