#include "line.h"
#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>

#define MAX_CHARS ((size_t)LINE_MAX_CHARS)

static void feed(struct line *line, char c)
{
	int ret = line_feed(line, c);

	zassert_equal(ret, 0, "byte 0x%02x: unexpected return: %d", (unsigned char)c, ret);
}

static void feed_str(struct line *line, const char *str)
{
	for (; *str != '\0'; str++) {
		feed(line, *str);
	}
}

/* A line that ended normally, with exactly this text */
static void assert_text(const struct line *line, const char *text)
{
	zassert_true(line->line_terminated, "line not terminated");
	zassert_false(line->overflow, "unexpected overflow");
	zassert_equal(line->len, strlen(text), "expected len %zu. Got: %zu", strlen(text),
		      line->len);
	zassert_str_equal(line->buf, text, "expected '%s'. Got: '%s'", text, line->buf);
}

ZTEST(line_feed_suite, test_builds_line_and_terminates)
{
	const char terminators[] = {'\r', '\n'};

	for (size_t i = 0; i < sizeof(terminators); i++) {
		struct line line = {0};

		feed_str(&line, "led on");
		feed(&line, terminators[i]);

		assert_text(&line, "led on");
	}
}

ZTEST(line_feed_suite, test_backspace)
{
	const char keys[] = {0x08, 0x7F};

	for (size_t i = 0; i < sizeof(keys); i++) {
		struct line line = {0};
		struct line extra = {0};

		feed_str(&line, "ab");
		feed(&line, keys[i]);
		feed(&line, '\r');
		assert_text(&line, "a");

		/* More backspaces than characters must not corrupt the line */
		feed(&extra, 'a');
		feed(&extra, keys[i]);
		feed(&extra, keys[i]);
		feed(&extra, keys[i]);
		zassert_equal(extra.len, 0, "unexpected len: %zu", extra.len);
		zassert_false(extra.line_terminated, "line terminated");

		feed_str(&extra, "c\r");
		assert_text(&extra, "c");
	}
}

ZTEST(line_feed_suite, test_printable_range_and_ignored_bytes)
{
	const char ignored[] = {0x01, '\t', 0x1B, (char)0x80, (char)0xFF};
	struct line line = {0};

	/* Both ends of the printable range are accepted */
	feed_str(&line, " ~");

	for (size_t i = 0; i < sizeof(ignored); i++) {
		feed(&line, ignored[i]);
	}

	feed_str(&line, "b\r");
	assert_text(&line, " ~b");
}

ZTEST(line_feed_suite, test_overflow)
{
	struct line full = {0};
	struct line over = {0};

	/* Exactly the limit is still a valid line */
	for (size_t i = 0; i < MAX_CHARS; i++) {
		feed(&full, 'x');
	}
	feed(&full, '\r');
	zassert_true(full.line_terminated, "line not terminated");
	zassert_false(full.overflow, "unexpected overflow");
	zassert_equal(full.len, MAX_CHARS, "unexpected len: %zu", full.len);
	zassert_equal(strlen(full.buf), MAX_CHARS, "buf not terminated at len");

	/* One more character discards the line */
	for (size_t i = 0; i < MAX_CHARS; i++) {
		feed(&over, 'x');
	}
	zassert_false(over.overflow, "overflow too early");

	feed(&over, 'x');
	zassert_true(over.overflow, "overflow not set");
	zassert_equal(over.len, MAX_CHARS, "unexpected len: %zu", over.len);

	/* A discarded line stays discarded, even if the user erases characters */
	feed(&over, 'y');
	feed(&over, 0x7F);
	zassert_true(over.overflow, "overflow cleared");
	zassert_equal(over.len, MAX_CHARS, "len changed: %zu", over.len);

	feed(&over, '\r');
	zassert_true(over.line_terminated, "line not terminated");
	zassert_true(over.overflow, "overflow cleared");
}

ZTEST(line_feed_suite, test_empty_line_and_crlf_pair)
{
	const char *pairs[] = {"\r\n", "\n\r"};
	struct line empty = {0};

	/* Enter alone never ends a line */
	feed(&empty, '\r');
	feed(&empty, '\n');
	zassert_false(empty.line_terminated, "empty line terminated");
	zassert_equal(empty.len, 0, "unexpected len: %zu", empty.len);

	for (size_t i = 0; i < ARRAY_SIZE(pairs); i++) {
		struct line line = {0};

		feed_str(&line, "ab");
		feed(&line, pairs[i][0]);
		zassert_true(line.line_terminated, "line not terminated");

		/* The caller handles the line and resets it before the second byte */
		line_reset(&line);
		feed(&line, pairs[i][1]);
		zassert_false(line.line_terminated, "second byte ended a line");
		zassert_equal(line.len, 0, "unexpected len: %zu", line.len);

		feed_str(&line, "c\r");
		assert_text(&line, "c");
	}
}

ZTEST(line_feed_suite, test_terminated_ignores_until_reset)
{
	struct line line = {0};

	feed_str(&line, "ab\r");
	feed(&line, 'c');
	feed(&line, 0x7F);
	feed(&line, '\r');
	assert_text(&line, "ab");

	line_reset(&line);
	zassert_equal(line.len, 0, "unexpected len: %zu", line.len);
	zassert_false(line.line_terminated, "line_terminated not cleared");
	zassert_false(line.overflow, "overflow not cleared");

	feed_str(&line, "x\r");
	assert_text(&line, "x");

	/* Reset also clears an overflow */
	line_reset(&line);
	for (size_t i = 0; i <= MAX_CHARS; i++) {
		feed(&line, 'x');
	}
	feed(&line, '\r');
	zassert_true(line.overflow, "overflow not set");

	line_reset(&line);
	zassert_false(line.overflow, "overflow not cleared");
	zassert_false(line.line_terminated, "line_terminated not cleared");
	zassert_equal(line.len, 0, "unexpected len: %zu", line.len);
}

ZTEST(line_feed_suite, test_null_parameters)
{
	int ret = line_feed(NULL, 'a');

	zassert_equal(ret, -EINVAL, "line NULL: unexpected return: %d", ret);
}

ZTEST_SUITE(line_feed_suite, NULL, NULL, NULL, NULL, NULL);
