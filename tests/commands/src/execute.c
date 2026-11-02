#include "cmd.h"
#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/* Value placed in out.ret to detect whether execute() touched it */
#define OUT_GUARD (-99)

struct handler_state {
	int calls;
	const char *last_args;
	int result;
};

static struct handler_state echo_state;
static struct handler_state led_state;

static int fake_handler(struct handler_state *state, const char *args, struct cmd_out *out)
{
	state->calls++;
	state->last_args = args;
	cmd_out_printf(out, "%s", args);

	return state->result;
}

static int echo_handler(const char *args, struct cmd_out *out)
{
	return fake_handler(&echo_state, args, out);
}

static int led_handler(const char *args, struct cmd_out *out)
{
	return fake_handler(&led_state, args, out);
}

static const struct cmd_entry cmds[] = {
	{.name = "ECHO", .description = "echo", .handler = echo_handler},
	{.name = "LED", .description = "led", .handler = led_handler},
	{.name = "NOH", .description = "no handler", .handler = NULL},
	{.name = NULL},
};

static const struct cmd_entry empty[] = {
	{.name = NULL},
};

/* A valid command placed after the sentinel must not be found */
static const struct cmd_entry after_sentinel[] = {
	{.name = NULL},
	{.name = "ECHO", .description = "echo", .handler = echo_handler},
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

static void assert_no_calls(void)
{
	zassert_equal(echo_state.calls, 0, "echo handler called");
	zassert_equal(led_state.calls, 0, "led handler called");
}

static void reset_state(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&echo_state, 0, sizeof(echo_state));
	memset(&led_state, 0, sizeof(led_state));
}

ZTEST(execute_suite, test_runs_selected_handler)
{
	struct cmd_out out;
	int ret;

	out_init(&out);

	ret = execute(cmds, "ECHO", "hi", &out);

	zassert_equal(ret, 0, "Unexpected return: %d", ret);
	zassert_equal(echo_state.calls, 1, "echo calls: %d", echo_state.calls);
	zassert_equal(led_state.calls, 0, "led calls: %d", led_state.calls);
	zassert_equal(out.ret, 0, "Unexpected out.ret: %d", out.ret);
	zassert_str_equal(out.buf, "hi", "Unexpected response: %s", out.buf);
	zassert_equal(out.len, strlen("hi"), "Unexpected len: %zu", out.len);
}

ZTEST(execute_suite, test_command_name_is_case_insensitive)
{
	const char *names[] = {"echo", "ECHO", "EcHo"};
	struct cmd_out out;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
		out_init(&out);

		ret = execute(cmds, names[i], "hi", &out);

		zassert_equal(ret, 0, "%s: unexpected return: %d", names[i], ret);
		zassert_str_equal(out.buf, "hi", "%s: unexpected response: %s", names[i], out.buf);
	}

	zassert_equal(echo_state.calls, ARRAY_SIZE(names), "echo calls: %d", echo_state.calls);
	zassert_equal(led_state.calls, 0, "led calls: %d", led_state.calls);
}

ZTEST(execute_suite, test_args_reach_handler_intact)
{
	const char *args[] = {"Ola  Mundo", ""};
	struct cmd_out out;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(args); i++) {
		out_init(&out);

		ret = execute(cmds, "ECHO", args[i], &out);

		zassert_equal(ret, 0, "Unexpected return: %d", ret);
		zassert_equal_ptr(echo_state.last_args, args[i], "args pointer changed");
		zassert_str_equal(out.buf, args[i], "Expected: '%s'. Got: '%s'", args[i], out.buf);
	}
}

ZTEST(execute_suite, test_handler_error_is_stored_in_out_ret)
{
	const int results[] = {-EIO, -ENOENT};
	struct cmd_out out;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(results); i++) {
		echo_state.result = results[i];
		out_init(&out);

		ret = execute(cmds, "ECHO", "", &out);

		/* The handler's own error must not be mistaken for a dispatch error */
		zassert_equal(ret, 0, "Unexpected return: %d", ret);
		zassert_equal(out.ret, results[i], "Expected out.ret %d. Got: %d", results[i],
			      out.ret);
	}
}

ZTEST(execute_suite, test_dispatch_errors)
{
	const struct {
		const struct cmd_entry *table;
		const char *name;
		int expected;
	} cases[] = {
		{cmds, "NOPE", -ENOENT},  {cmds, "", -ENOENT},
		{cmds, "EC", -ENOENT},    {cmds, "ECHOX", -ENOENT},
		{empty, "ECHO", -ENOENT}, {after_sentinel, "ECHO", -ENOENT},
		{cmds, "NOH", -ENOSYS},
	};
	struct cmd_out out;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		out_init(&out);

		ret = execute(cases[i].table, cases[i].name, "", &out);

		zassert_equal(ret, cases[i].expected, "case %zu: expected %d. Got: %d", i,
			      cases[i].expected, ret);
		assert_no_calls();
		assert_untouched(&out);
	}
}

ZTEST(execute_suite, test_null_parameters)
{
	/* A valid and an unknown name: invalid parameters must win over the lookup */
	const char *names[] = {"ECHO", "NOPE"};
	struct cmd_out out;
	int ret;

	out_init(&out);

	for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
		ret = execute(NULL, names[i], "", &out);
		zassert_equal(ret, -EINVAL, "%s, table NULL: unexpected return: %d", names[i], ret);

		ret = execute(cmds, names[i], NULL, &out);
		zassert_equal(ret, -EINVAL, "%s, args NULL: unexpected return: %d", names[i], ret);

		ret = execute(cmds, names[i], "", NULL);
		zassert_equal(ret, -EINVAL, "%s, out NULL: unexpected return: %d", names[i], ret);
	}

	ret = execute(cmds, NULL, "", &out);
	zassert_equal(ret, -EINVAL, "name NULL: unexpected return: %d", ret);

	assert_no_calls();
	assert_untouched(&out);
}

ZTEST_SUITE(execute_suite, NULL, NULL, reset_state, NULL, NULL);
