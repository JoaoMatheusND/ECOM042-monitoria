#include "cmd.h"
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

struct args_case {
	const char *args;
	const char *expected;
};

static void assert_all(const struct args_case *cases, size_t count, bool expected_result)
{
	for (size_t i = 0; i < count; i++) {
		bool result = args_equal(cases[i].args, cases[i].expected);

		zassert_equal(result, expected_result, "args '%s', expected '%s': got %d",
			      cases[i].args, cases[i].expected, result);
	}
}

ZTEST(args_equal_suite, test_matches_ignoring_case)
{
	const struct args_case cases[] = {
		{"ON", "ON"},
		{"on", "ON"},
		{"oN", "ON"},
	};

	assert_all(cases, ARRAY_SIZE(cases), true);
}

ZTEST(args_equal_suite, test_ignores_surrounding_whitespace)
{
	const struct args_case cases[] = {
		{" ON", "ON"},
		{"ON ", "ON"},
		{"\t ON \t", "ON"},
		/* An empty expected means "no arguments" */
		{"", ""},
		{"   ", ""},
		{"\t", ""},
	};

	assert_all(cases, ARRAY_SIZE(cases), true);
}

ZTEST(args_equal_suite, test_does_not_match_other_text)
{
	const struct args_case cases[] = {
		{"OFF", "ON"},
		{"O", "ON"},
		{"ONN", "ON"},
		{"ON OFF", "ON"},
		{"O N", "ON"},
		{"", "ON"},
		{"   ", "ON"},
		{"ON", ""},
		{"x", ""},
	};

	assert_all(cases, ARRAY_SIZE(cases), false);

	zassert_false(args_equal(NULL, "ON"), "args NULL: matched");
	zassert_false(args_equal("ON", NULL), "expected NULL: matched");
}

ZTEST_SUITE(args_equal_suite, NULL, NULL, NULL, NULL, NULL);
