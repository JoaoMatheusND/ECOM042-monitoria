#include "command.h"
#include "line.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static bool led;
static int button;
static int gpio_error;
static int writes;

static int fake_led_set(bool on)
{
	if (gpio_error < 0) {
		return gpio_error;
	}
	led = on;
	writes++;
	return 0;
}

static int fake_led_get(void)
{
	return gpio_error < 0 ? gpio_error : led;
}

static int fake_button_read(void)
{
	return gpio_error < 0 ? gpio_error : button;
}

static int64_t fake_uptime(void)
{
	return 12345678901LL;
}

static const struct shell_io io = {
	.led_set = fake_led_set,
	.led_get = fake_led_get,
	.button_read = fake_button_read,
	.uptime_get = fake_uptime,
};

static void before_test(void *fixture)
{
	ARG_UNUSED(fixture);
	led = false;
	button = 0;
	gpio_error = 0;
	writes = 0;
}

static void expect(const char *input, const char *expected)
{
	char response[192];

	zassert_equal(command_dispatch(input, &io, response, sizeof(response)), 0);
	zassert_str_equal(response, expected);
}

static enum line_result feed(struct line_buffer *line, const char *text)
{
	enum line_result result = LINE_WAIT;

	while (*text != '\0') {
		result = line_feed(line, (unsigned char)*text++);
	}
	return result;
}

ZTEST(shell, test_help)
{
	expect("HELP", "HELP\r\nLED ON\r\nLED OFF\r\nLED\r\nBTN\r\nUPTIME\r\nECHO <texto>");
}

ZTEST(shell, test_led_on_off_and_query)
{
	expect("LED", "LED: OFF");
	expect("LED ON", "OK");
	zassert_true(led);
	expect("LED", "LED: ON");
	expect("LED OFF", "OK");
	zassert_false(led);
	expect("LED", "LED: OFF");
	zassert_equal(writes, 2);
}

ZTEST(shell, test_case_and_spaces)
{
	expect("  lEd  oN  ", "OK");
	expect("led", "LED: ON");
	expect("bTn", "BTN: 0");
	expect("eChO Oi", "Oi");
}

ZTEST(shell, test_button_states)
{
	expect("BTN", "BTN: 0");
	button = 1;
	expect("BTN", "BTN: 1");
}

ZTEST(shell, test_uptime_64_bits)
{
	expect("UPTIME", "UPTIME: 12345678901 ms");
}

ZTEST(shell, test_echo_preserves_text)
{
	expect("ECHO oi", "oi");
	expect("ECHO  Oi  Mundo!  ", " Oi  Mundo!  ");
	expect("ECHO %s %d", "%s %d");
	expect("ECHO", "");
}

ZTEST(shell, test_unknown_and_extra_arguments)
{
	expect("INVALIDO", "ERR: unknown command");
	expect("LED ON extra", "ERR: unknown command");
	expect("LEDON", "ERR: unknown command");
	expect("LED BLINK", "ERR: unknown command");
	expect("BTN 1", "ERR: unknown command");
	expect("HELP extra", "ERR: unknown command");
	expect("UPTIME extra", "ERR: unknown command");
	expect("", "ERR: unknown command");
	zassert_equal(writes, 0);
}

ZTEST(shell, test_gpio_error)
{
	gpio_error = -EIO;
	expect("LED ON", "ERR: GPIO");
	expect("LED", "ERR: GPIO");
	expect("BTN", "ERR: GPIO");
	zassert_equal(writes, 0);
}

ZTEST(shell, test_response_capacity)
{
	char response[4] = {0};
	char exact[3];

	zassert_equal(command_dispatch("HELP", &io, response, sizeof(response)), -ENOSPC);
	zassert_equal(response[3], '\0');
	zassert_equal(command_dispatch("ECHO oi", &io, exact, sizeof(exact)), 0);
	zassert_str_equal(exact, "oi");
	zassert_equal(command_dispatch("HELP", &io, response, 0), -EINVAL);
}

ZTEST(shell, test_line_lf_cr_crlf)
{
	struct line_buffer line = {0};

	zassert_equal(feed(&line, "HELP\n"), LINE_READY);
	zassert_str_equal(line.text, "HELP");
	zassert_equal(feed(&line, "LED\r"), LINE_READY);
	zassert_str_equal(line.text, "LED");
	zassert_equal(line_feed(&line, '\n'), LINE_WAIT);
	zassert_equal(feed(&line, "BTN\r\n"), LINE_WAIT);
	zassert_str_equal(line.text, "BTN");
	zassert_equal(feed(&line, "ECHO oi\n"), LINE_READY);
	zassert_str_equal(line.text, "ECHO oi");
}

ZTEST(shell, test_backspace_and_delete)
{
	struct line_buffer line = {0};

	zassert_equal(feed(&line, "\b\x7f"), LINE_WAIT);
	zassert_equal(line.length, 0);
	zassert_equal(feed(&line, "LED OX\bN\x7f"
				  "FF\n"),
		      LINE_READY);
	zassert_str_equal(line.text, "LED OFF");
}

ZTEST(shell, test_exact_line_limit)
{
	struct line_buffer line = {0};

	for (size_t i = 0; i < CONFIG_MINI_SHELL_LINE_SIZE; i++) {
		zassert_equal(line_feed(&line, 'a'), LINE_WAIT);
	}
	zassert_equal(line_feed(&line, '\n'), LINE_READY);
	zassert_equal(strlen(line.text), CONFIG_MINI_SHELL_LINE_SIZE);
}

ZTEST(shell, test_long_line_discard_and_recovery)
{
	struct line_buffer line = {0};

	for (size_t i = 0; i <= CONFIG_MINI_SHELL_LINE_SIZE; i++) {
		(void)line_feed(&line, 'a');
	}
	/* Backspace nao recupera uma linha que ja ultrapassou o limite. */
	zassert_equal(feed(&line, "\bLED ON\r"), LINE_TOO_LONG);
	zassert_equal(line_feed(&line, '\n'), LINE_WAIT);
	zassert_equal(writes, 0);
	zassert_equal(feed(&line, "LED\n"), LINE_READY);
	expect(line.text, "LED: OFF");
}

ZTEST(shell, test_overflow_after_valid_command_does_not_execute)
{
	struct line_buffer line = {0};
	enum line_result result;

	(void)feed(&line, "LED ON");
	for (size_t i = 0; i < CONFIG_MINI_SHELL_LINE_SIZE; i++) {
		(void)line_feed(&line, ' ');
	}
	result = line_feed(&line, '\n');
	zassert_equal(result, LINE_TOO_LONG);
	zassert_equal(writes, 0);
	zassert_false(led);
}

ZTEST(shell, test_backspace_at_limit)
{
	struct line_buffer line = {0};

	for (size_t i = 0; i < CONFIG_MINI_SHELL_LINE_SIZE; i++) {
		(void)line_feed(&line, 'a');
	}
	(void)line_feed(&line, 0x7f);
	(void)line_feed(&line, 'b');
	zassert_equal(line_feed(&line, '\n'), LINE_READY);
	zassert_equal(line.text[CONFIG_MINI_SHELL_LINE_SIZE - 1], 'b');
}

ZTEST(shell, test_missing_callbacks)
{
	struct shell_io incomplete = io;
	char response[32] = "sem alteracao";

	zassert_true(shell_io_valid(&io));
	zassert_false(shell_io_valid(NULL));
	incomplete.led_set = NULL;
	zassert_equal(command_dispatch("LED ON", &incomplete, response, sizeof(response)), -EINVAL);
	incomplete = io;
	incomplete.led_get = NULL;
	zassert_equal(command_dispatch("LED", &incomplete, response, sizeof(response)), -EINVAL);
	incomplete = io;
	incomplete.button_read = NULL;
	zassert_equal(command_dispatch("BTN", &incomplete, response, sizeof(response)), -EINVAL);
	incomplete = io;
	incomplete.uptime_get = NULL;
	zassert_equal(command_dispatch("UPTIME", &incomplete, response, sizeof(response)), -EINVAL);
	zassert_str_equal(response, "sem alteracao");
	zassert_equal(writes, 0);
	zassert_false(led);
}

ZTEST(shell, test_null_arguments)
{
	char response[32];

	zassert_equal(command_dispatch(NULL, &io, response, sizeof(response)), -EINVAL);
	zassert_equal(command_dispatch("LED ON", NULL, response, sizeof(response)), -EINVAL);
	zassert_equal(command_dispatch("LED ON", &io, NULL, sizeof(response)), -EINVAL);
	zassert_equal(writes, 0);
}

ZTEST_SUITE(shell, NULL, NULL, before_test, NULL, NULL);