#include "cmd.h"
#include "dispatch.h"
#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/* Value placed in out.ret to detect whether the handler was reached */
#define OUT_GUARD (-99)

/* Longest name that fits: DISPATCH_NAME_MAX - 1 characters */
#define LIMIT_NAME "ABCDEFGHIJKLMNO"

struct handler_state {
	int calls;
	const char *last_args;
};

static struct handler_state echo_state;

static int echo_handler(const char *args, struct cmd_out *out)
{
	echo_state.calls++;
	echo_state.last_args = args;
	cmd_out_printf(out, "%s", args);

	return 0;
}

static const struct cmd_entry cmds[] = {
	{.name = "ECHO", .description = "echo", .handler = echo_handler},
	{.name = "NOH", .description = "no handler", .handler = NULL},
	{.name = LIMIT_NAME, .description = "longest name", .handler = echo_handler},
	{.name = NULL},
};

static void out_init(struct cmd_out *out)
{
	memset(out, 0, sizeof(*out));
	out->ret = OUT_GUARD;
}

static void assert_untouched(const struct cmd_out *out)
{
	zassert_equal(out->ret, OUT_GUARD, "ret changed: %d", out->ret);
	zassert_equal(out->len, 0, "len changed: %zu", out->len);
	zassert_false(out->overflow, "overflow changed");
	zassert_equal(out->buf[0], '\0', "buf changed");
}

static void reset_state(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&echo_state, 0, sizeof(echo_state));
}

ZTEST(dispatch_suite, test_splits_name_and_args)
{
	const struct {
		const char *line;
		const char *args;
	} cases[] = {
		{"ECHO hi", "hi"},  {"ECHO", ""},          {"ECHO ", ""},
		{"ECHO  a", " a"},  {"ECHO a b", "a b"},   {"ECHO hi ", "hi "},
		{"ECHO\thi", "hi"}, {" \t echo hi", "hi"}, {LIMIT_NAME " x", "x"},
	};
	struct cmd_out out;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		reset_state(NULL);
		out_init(&out);

		ret = dispatch(cmds, cases[i].line, &out);

		zassert_equal(ret, 0, "'%s': unexpected return: %d", cases[i].line, ret);
		zassert_equal(echo_state.calls, 1, "'%s': calls: %d", cases[i].line,
			      echo_state.calls);
		zassert_str_equal(echo_state.last_args, cases[i].args,
				  "'%s': expected args '%s'. Got: '%s'", cases[i].line,
				  cases[i].args, echo_state.last_args);
		zassert_equal(out.ret, 0, "'%s': unexpected out.ret: %d", cases[i].line, out.ret);
		zassert_str_equal(out.buf, cases[i].args, "'%s': unexpected response: '%s'",
				  cases[i].line, out.buf);
	}
}

ZTEST(dispatch_suite, test_errors)
{
	const struct {
		const char *line;
		int expected;
	} cases[] = {
		{"", -EINVAL},
		{"   ", -EINVAL},
		{"\t", -EINVAL},
		{"NOPE", -ENOENT},
		{"NOH", -ENOSYS},
		/* Must not be truncated to LIMIT_NAME and match it */
		{LIMIT_NAME "P", -ENOENT},
		{"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMN", -ENOENT},
	};
	struct cmd_out out;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		out_init(&out);

		ret = dispatch(cmds, cases[i].line, &out);

		zassert_equal(ret, cases[i].expected, "'%s': expected %d. Got: %d", cases[i].line,
			      cases[i].expected, ret);
		zassert_equal(echo_state.calls, 0, "'%s': handler called", cases[i].line);
		assert_untouched(&out);
	}
}

ZTEST(dispatch_suite, test_null_parameters)
{
	struct cmd_out out;
	int ret;

	out_init(&out);

	ret = dispatch(NULL, "ECHO hi", &out);
	zassert_equal(ret, -EINVAL, "table NULL: unexpected return: %d", ret);

	ret = dispatch(cmds, NULL, &out);
	zassert_equal(ret, -EINVAL, "line NULL: unexpected return: %d", ret);

	ret = dispatch(cmds, "ECHO hi", NULL);
	zassert_equal(ret, -EINVAL, "out NULL: unexpected return: %d", ret);

	zassert_equal(echo_state.calls, 0, "handler called");
	assert_untouched(&out);
}

ZTEST_SUITE(dispatch_suite, NULL, NULL, reset_state, NULL, NULL);
