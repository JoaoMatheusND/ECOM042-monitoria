/* SPDX-License-Identifier: Apache-2.0 */

/*******************************************************************
 * @file main.c
 *
 * @brief Testes do parser e da linha, sem abrir a UART.
 * O dublê (fake_io) ocupa o lugar do LED, do botao e do relogio.
 *******************************************************************/

#include "board.h"
#include "shell.h"
#include "shell_line.h"

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const char help_text[] = "HELP\r\n"
				"LED ON\r\n"
				"LED OFF\r\n"
				"LED\r\n"
				"BTN\r\n"
				"UPTIME\r\n"
				"ECHO <texto>";

static bool fake_led;
static int fake_btn;
static int fake_set_calls;
static int64_t fake_uptime;
static int fake_set_result;

static int fake_led_set(bool on)
{
	fake_set_calls++;
	if (fake_set_result < 0) {
		return fake_set_result;
	}
	fake_led = on;
	return 0;
}

static int fake_led_get(bool *on)
{
	*on = fake_led;
	return 0;
}

static int fake_button_get(int *level)
{
	*level = fake_btn;
	return 0;
}

static int64_t fake_uptime_ms(void)
{
	return fake_uptime;
}

static const struct shell_io fake_io = {
	.led_set = fake_led_set,
	.led_get = fake_led_get,
	.button_get = fake_button_get,
	.uptime_ms = fake_uptime_ms,
};

static void reset_fakes(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_led = false;
	fake_btn = 0;
	fake_set_calls = 0;
	fake_uptime = 42;
	fake_set_result = 0;
}

static void dispatch(const char *line, char *out, size_t out_len)
{
	shell_dispatch(line, &fake_io, out, out_len);
}

ZTEST_SUITE(shell_cmd, NULL, NULL, reset_fakes, NULL, NULL);

ZTEST(shell_cmd, test_help_lists_one_command_per_line)
{
	char out[128];

	dispatch("help", out, sizeof(out));
	zassert_str_equal(out, help_text);
}

ZTEST(shell_cmd, test_help_with_argument_is_unknown)
{
	char out[128];

	dispatch("HELP agora", out, sizeof(out));
	zassert_str_equal(out, "ERR: unknown command");
}

ZTEST(shell_cmd, test_led_on_off_and_status_ignore_case)
{
	char out[64];

	dispatch("led", out, sizeof(out));
	zassert_str_equal(out, "LED: OFF");

	dispatch("LED ON", out, sizeof(out));
	zassert_str_equal(out, "OK");
	zassert_equal(fake_set_calls, 1);
	zassert_true(fake_led);

	dispatch("led", out, sizeof(out));
	zassert_str_equal(out, "LED: ON");

	dispatch("led off", out, sizeof(out));
	zassert_str_equal(out, "OK");
	zassert_false(fake_led);

	dispatch("LED", out, sizeof(out));
	zassert_str_equal(out, "LED: OFF");
}

ZTEST(shell_cmd, test_led_bad_argument_does_not_touch_the_pin)
{
	char out[64];

	dispatch("LED ON extra", out, sizeof(out));
	zassert_str_equal(out, "ERR: unknown command");
	zassert_equal(fake_set_calls, 0);

	dispatch("LEDON", out, sizeof(out));
	zassert_str_equal(out, "ERR: unknown command");
	zassert_equal(fake_set_calls, 0);
}

ZTEST(shell_cmd, test_led_set_error_is_reported)
{
	char out[64];

	fake_set_result = -5;
	dispatch("LED ON", out, sizeof(out));
	zassert_str_equal(out, "ERR: led");
	zassert_false(fake_led);
}

ZTEST(shell_cmd, test_button_value)
{
	char out[32];

	fake_btn = 0;
	dispatch("btn", out, sizeof(out));
	zassert_str_equal(out, "BTN: 0");

	fake_btn = 1;
	dispatch("BTN", out, sizeof(out));
	zassert_str_equal(out, "BTN: 1");
}

ZTEST(shell_cmd, test_uptime_uses_injected_clock)
{
	char out[64];

	fake_uptime = 1234;
	dispatch("UpTime", out, sizeof(out));
	zassert_str_equal(out, "UPTIME: 1234 ms");
}

ZTEST(shell_cmd, test_echo_keeps_the_original_text)
{
	char out[64];

	dispatch("EcHo AbC 123", out, sizeof(out));
	zassert_str_equal(out, "AbC 123");

	dispatch("ECHO  dois  espacos ", out, sizeof(out));
	zassert_str_equal(out, " dois  espacos ");

	dispatch("echo", out, sizeof(out));
	zassert_equal(out[0], '\0');
}

ZTEST(shell_cmd, test_unknown_and_empty)
{
	char out[64];

	dispatch("abrir_porta", out, sizeof(out));
	zassert_str_equal(out, "ERR: unknown command");

	dispatch("   ", out, sizeof(out));
	zassert_equal(out[0], '\0');
}

struct feed_result {
	char response[128];
	char echo[8];
	size_t echo_len;
	bool done;
};

static void feed_byte(struct shell_line *line, uint8_t byte, struct feed_result *result)
{
	shell_line_feed(line, byte, &fake_io, result->echo, sizeof(result->echo), &result->echo_len,
			result->response, sizeof(result->response), &result->done);
}

static int feed_text(struct shell_line *line, const char *text, char responses[][128], int max)
{
	int count = 0;
	size_t i;

	for (i = 0; text[i] != '\0'; i++) {
		struct feed_result result;

		feed_byte(line, (uint8_t)text[i], &result);
		if (result.done && count < max) {
			snprintf(responses[count], sizeof(responses[count]), "%s", result.response);
			count++;
		}
	}
	return count;
}

ZTEST_SUITE(shell_line_edit, NULL, NULL, reset_fakes, NULL, NULL);

ZTEST(shell_line_edit, test_crlf_is_a_single_line)
{
	char storage[16];
	struct shell_line line;
	char responses[4][128];
	int count;

	shell_line_init(&line, storage, sizeof(storage) - 1);
	count = feed_text(&line, "HELP\r\nECHO z\n", responses, 4);
	zassert_equal(count, 2);
	zassert_str_equal(responses[0], help_text);
	zassert_str_equal(responses[1], "z");
}

ZTEST(shell_line_edit, test_backspace_and_delete)
{
	char storage[16];
	struct shell_line line;
	struct feed_result result;
	char responses[2][128];
	int count;

	shell_line_init(&line, storage, sizeof(storage) - 1);

	feed_byte(&line, 'A', &result);
	zassert_false(result.done);
	zassert_equal(result.echo_len, 1);
	zassert_equal(result.echo[0], 'A');

	feed_byte(&line, 0x7F, &result);
	zassert_false(result.done);
	zassert_equal(result.echo_len, 3);
	zassert_mem_equal(result.echo, "\b \b", 3);

	feed_byte(&line, 0x08, &result);
	zassert_equal(result.echo_len, 0);

	count = feed_text(&line, "ECHO oi\n", responses, 2);
	zassert_equal(count, 1);
	zassert_str_equal(responses[0], "oi");
}

ZTEST(shell_line_edit, test_line_at_the_limit_is_kept)
{
	char storage[5];
	struct shell_line line;
	char responses[2][128];
	int count;

	shell_line_init(&line, storage, 4);
	count = feed_text(&line, "ECHO\n", responses, 2);
	zassert_equal(count, 1);
	zassert_equal(responses[0][0], '\0');
}

ZTEST(shell_line_edit, test_line_past_the_limit_is_discarded_whole)
{
	char storage[5];
	struct shell_line line;
	char responses[2][128];
	int count;

	shell_line_init(&line, storage, 4);
	/* HELP caberia, mas o quinto caractere condena a linha inteira. */
	count = feed_text(&line, "HELX\x08P\nHELPX\n", responses, 2);
	zassert_equal(count, 2);
	zassert_str_equal(responses[0], help_text);
	zassert_str_equal(responses[1], "ERR: line too long");
}

ZTEST(shell_line_edit, test_overflow_is_not_undone_by_backspace)
{
	char storage[5];
	struct shell_line line;
	char responses[2][128];
	int count;

	shell_line_init(&line, storage, 4);
	count = feed_text(&line, "HELLO\x08\n", responses, 2);
	zassert_equal(count, 1);
	zassert_str_equal(responses[0], "ERR: line too long");
}

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

static const struct shell_io board_io = {
	.led_set = board_led_set,
	.led_get = board_led_get,
	.button_get = board_button_get,
	.uptime_ms = board_uptime_ms,
};

static void board_before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_equal(board_init(), 0);
	zassert_equal(gpio_emul_input_set(button.port, button.pin, 0), 0);
}

ZTEST_SUITE(shell_board, NULL, NULL, board_before, NULL, NULL);

ZTEST(shell_board, test_led_and_button_go_through_devicetree)
{
	char out[64];

	shell_dispatch("LED", &board_io, out, sizeof(out));
	zassert_str_equal(out, "LED: OFF");

	shell_dispatch("LED ON", &board_io, out, sizeof(out));
	zassert_str_equal(out, "OK");

	shell_dispatch("LED", &board_io, out, sizeof(out));
	zassert_str_equal(out, "LED: ON");

	shell_dispatch("BTN", &board_io, out, sizeof(out));
	zassert_str_equal(out, "BTN: 0");

	zassert_equal(gpio_emul_input_set(button.port, button.pin, 1), 0);
	shell_dispatch("BTN", &board_io, out, sizeof(out));
	zassert_str_equal(out, "BTN: 1");
}

ZTEST(shell_board, test_uptime_reads_the_kernel_clock)
{
	char out[64];
	int64_t shown = -1;
	int64_t before;
	int64_t after;

	before = k_uptime_get();
	shell_dispatch("UPTIME", &board_io, out, sizeof(out));
	after = k_uptime_get();

	zassert_true(sscanf(out, "UPTIME: %lld ms", (long long *)&shown) == 1);
	zassert_true(shown >= before);
	zassert_true(shown <= after);
}
