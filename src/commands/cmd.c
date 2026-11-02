#include "cmd.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/**
 * @brief Look up a command entry by name.
 *
 * @param[in]  table Command table, terminated by an entry with @c name == @c NULL.
 * @param[in]  name  Command name (case-insensitive).
 * @param[out] out   Receives a pointer to the matching entry. Left untouched on failure.
 *
 * @retval 0        Command found.
 * @retval -EINVAL  @p table or @p name is @c NULL.
 * @retval -ENOENT  No command with that name in @p table.
 */
static int get_command(const struct cmd_entry *table, const char *name,
		       const struct cmd_entry **out)
{

	if (table == NULL || name == NULL) {
		return -EINVAL;
	}

	while (table->name != NULL) {
		if (strcasecmp(name, table->name) == 0) {
			*out = table;
			return 0;
		}
		table++;
	}

	return -ENOENT;
}

int execute(const struct cmd_entry *table, const char *name, const char *args, struct cmd_out *out)
{
	const struct cmd_entry *command;
	int ret;

	if (out == NULL || args == NULL) {
		return -EINVAL;
	}

	ret = get_command(table, name, &command);

	if (ret < 0) {
		return ret;
	}

	if (command->handler == NULL) {
		return -ENOSYS;
	}

	out->ret = command->handler(args, out);

	return 0;
}

int cmd_out_printf(struct cmd_out *out, const char *fmt, ...)
{
	va_list ap;
	size_t free_space;
	int written;

	if (out == NULL || fmt == NULL || out->len >= sizeof(out->buf)) {
		return -EINVAL;
	}

	if (out->overflow) {
		return 0;
	}

	free_space = sizeof(out->buf) - out->len;

	va_start(ap, fmt);
	written = vsnprintf(out->buf + out->len, free_space, fmt, ap);
	va_end(ap);

	if (written < 0) {
		out->buf[out->len] = '\0';
		return -EIO;
	}

	if ((size_t)written >= free_space) {
		out->len = sizeof(out->buf) - 1;
		out->overflow = true;
	} else {
		out->len += (size_t)written;
	}

	return 0;
}

bool args_equal(const char *args, const char *expected)
{
	size_t len;

	if (args == NULL || expected == NULL) {
		return false;
	}

	while (isspace((unsigned char)*args)) {
		args++;
	}

	len = strlen(args);

	while (len > 0 && isspace((unsigned char)args[len - 1])) {
		len--;
	}

	return len == strlen(expected) && strncasecmp(args, expected, len) == 0;
}