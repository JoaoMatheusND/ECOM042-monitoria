#include "cmd.h"
#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define BUF_SIZE CONFIG_CMD_BUF_SIZE

/* Longer than the buffer, so any prefix up to BUF_SIZE can be taken from it */
static char filler[BUF_SIZE + 1];

static void fill_filler(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(filler, 'x', BUF_SIZE);
	filler[BUF_SIZE] = '\0';
}

/* Invariant that must hold after every call: buf is terminated exactly at len */
static void assert_consistent(const struct cmd_out *out)
{
	zassert_true(out->len < BUF_SIZE, "len out of range: %zu", out->len);
	zassert_equal(strlen(out->buf), out->len, "len %zu does not match buf '%s'", out->len,
		      out->buf);
}

ZTEST(cmd_out_printf_suite, test_appends_formatted_text)
{
	struct cmd_out out = {0};
	int ret;

	ret = cmd_out_printf(&out, "ab%d", 12);
	zassert_equal(ret, 0, "Unexpected return: %d", ret);

	/* An empty write changes nothing */
	ret = cmd_out_printf(&out, "");
	zassert_equal(ret, 0, "Unexpected return: %d", ret);

	ret = cmd_out_printf(&out, "-%s", "cd");
	zassert_equal(ret, 0, "Unexpected return: %d", ret);

	zassert_str_equal(out.buf, "ab12-cd", "Unexpected response: %s", out.buf);
	zassert_equal(out.len, strlen("ab12-cd"), "Unexpected len: %zu", out.len);
	zassert_false(out.overflow, "Unexpected overflow");
	assert_consistent(&out);
}

ZTEST(cmd_out_printf_suite, test_truncation_and_overflow)
{
	const struct {
		size_t prefill;
		size_t write;
		bool overflow;
	} cases[] = {
		{0, BUF_SIZE - 1, false},
		{0, BUF_SIZE, true},
		{BUF_SIZE - 3, 2, false},
		{BUF_SIZE - 3, 3, true},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct cmd_out out = {0};
		struct cmd_out before;
		int ret;

		ret = cmd_out_printf(&out, "%.*s", (int)cases[i].prefill, filler);
		zassert_equal(ret, 0, "case %zu: unexpected return: %d", i, ret);

		ret = cmd_out_printf(&out, "%.*s", (int)cases[i].write, filler);
		zassert_equal(ret, 0, "case %zu: unexpected return: %d", i, ret);

		/* Every case ends with the buffer full: only the terminator is left */
		zassert_equal(out.overflow, cases[i].overflow, "case %zu: overflow is %d", i,
			      out.overflow);
		zassert_equal(out.len, BUF_SIZE - 1, "case %zu: unexpected len: %zu", i, out.len);
		zassert_equal(strspn(out.buf, "x"), out.len, "case %zu: unexpected content", i);
		assert_consistent(&out);

		if (!cases[i].overflow) {
			continue;
		}

		/* Once overflowed, further writes leave the response untouched */
		before = out;

		ret = cmd_out_printf(&out, "x");
		zassert_equal(ret, 0, "case %zu: unexpected return: %d", i, ret);
		zassert_mem_equal(&out, &before, sizeof(out), "case %zu: out changed", i);
	}
}

ZTEST(cmd_out_printf_suite, test_invalid_parameters)
{
	struct cmd_out out = {0};
	struct cmd_out before = out;
	int ret;

	ret = cmd_out_printf(NULL, "x");
	zassert_equal(ret, -EINVAL, "out NULL: unexpected return: %d", ret);

	ret = cmd_out_printf(&out, NULL);
	zassert_equal(ret, -EINVAL, "fmt NULL: unexpected return: %d", ret);
	zassert_mem_equal(&out, &before, sizeof(out), "fmt NULL: out changed");

	/* A len that does not fit the buffer means out was never initialized */
	out.len = BUF_SIZE;
	before = out;

	ret = cmd_out_printf(&out, "x");
	zassert_equal(ret, -EINVAL, "bad len: unexpected return: %d", ret);
	zassert_mem_equal(&out, &before, sizeof(out), "bad len: out changed");
}

ZTEST_SUITE(cmd_out_printf_suite, NULL, NULL, fill_filler, NULL, NULL);
