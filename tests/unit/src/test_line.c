/*******************************************************************
 * @file test_line.c
 *
 * @brief Testes da montagem de linha (line.c).
 *******************************************************************/

#include <string.h>

#include <zephyr/ztest.h>

#include "line.h"

static struct line_buf lb;

/* Alimenta uma string e retorna o último evento. */
static enum line_event feed(const char *s)
{
	enum line_event ev = LINE_NONE;

	while (*s != '\0') {
		ev = line_feed(&lb, *s++);
	}

	return ev;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	line_init(&lb);
}

ZTEST_SUITE(line, NULL, NULL, before, NULL, NULL);

ZTEST(line, test_cr_and_lf_end_line)
{
	zassert_equal(feed("HELP\r"), LINE_READY);
	zassert_str_equal(line_get(&lb), "HELP");

	line_reset(&lb);
	zassert_equal(feed("BTN\n"), LINE_READY);
	zassert_str_equal(line_get(&lb), "BTN");
}

ZTEST(line, test_crlf_is_a_single_end_of_line)
{
	zassert_equal(feed("LED\r"), LINE_READY);
	line_reset(&lb);
	zassert_equal(line_feed(&lb, '\n'), LINE_NONE);
}

ZTEST(line, test_chars_are_accepted_for_echo)
{
	zassert_equal(line_feed(&lb, 'a'), LINE_CHAR);
	zassert_equal(line_feed(&lb, 0x01), LINE_NONE, "controle nao entra na linha");
}

ZTEST(line, test_backspace_and_delete)
{
	feed("LEDX");
	zassert_equal(line_feed(&lb, 0x08), LINE_ERASE);
	zassert_equal(feed(" ONN"), LINE_CHAR);
	zassert_equal(line_feed(&lb, 0x7F), LINE_ERASE);
	zassert_equal(line_feed(&lb, '\r'), LINE_READY);
	zassert_str_equal(line_get(&lb), "LED ON");
}

ZTEST(line, test_backspace_on_empty_line)
{
	zassert_equal(line_feed(&lb, 0x08), LINE_NONE);
	zassert_equal(feed("ok\r"), LINE_READY);
	zassert_str_equal(line_get(&lb), "ok");
}

ZTEST(line, test_max_length_fits)
{
	char max[CONFIG_MINISHELL_LINE_SIZE];

	memset(max, 'x', sizeof(max) - 1);
	max[sizeof(max) - 1] = '\0';

	zassert_equal(feed(max), LINE_CHAR);
	zassert_equal(line_feed(&lb, '\r'), LINE_READY);
	zassert_equal(strlen(line_get(&lb)), CONFIG_MINISHELL_LINE_SIZE - 1);
}

ZTEST(line, test_too_long_line_is_discarded)
{
	for (int i = 0; i < CONFIG_MINISHELL_LINE_SIZE + 10; i++) {
		line_feed(&lb, 'A');
	}
	/* backspace nao "salva" uma linha que ja estourou */
	zassert_equal(line_feed(&lb, 0x08), LINE_NONE);
	zassert_equal(line_feed(&lb, '\r'), LINE_TOO_LONG);
	zassert_str_equal(line_get(&lb), "", "linha longa deve ser descartada");

	/* a proxima linha volta ao normal */
	zassert_equal(feed("ECHO oi\r"), LINE_READY);
	zassert_str_equal(line_get(&lb), "ECHO oi");
}
