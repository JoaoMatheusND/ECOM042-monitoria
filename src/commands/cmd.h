/**
 * @file cmd.h
 * @brief Command registry: lookup and execution of commands by name.
 *
 * This module knows nothing about UART, GPIO or line parsing. The command
 * table is always provided by the caller and must be terminated by an entry
 * whose @c name is @c NULL.
 */

#ifndef CMD_H_
#define CMD_H_

#include <stdbool.h>
#include <stddef.h>
#include <zephyr/toolchain.h>

/**
 * @brief Response of a command.
 *
 * Created and initialized (zeroed) by the caller of execute(). The handler
 * writes the response content (no trailing "\r\n" and no prompt) only through
 * cmd_out_printf(), never directly into the fields.
 */
struct cmd_out {
	char buf[CONFIG_CMD_BUF_SIZE]; /**< Response, always null-terminated. */
	int ret;                       /**< Value returned by the handler; set only by execute(). */
	size_t len;                    /**< Number of bytes in @c buf, excluding the terminator. */
	bool overflow;                 /**< true if any write did not fit and was truncated. */
};

/**
 * @brief Command handler function pointer.
 *
 * @param[in]  args Text following the command name, exactly as received
 *                  (never @c NULL; empty string when there are no arguments).
 *                  Validating it is up to the handler.
 * @param[out] out  Response; written through cmd_out_printf().
 *
 * @retval 0       Success.
 * @retval -errno  Failure. execute() stores this value in @c out->ret.
 */
typedef int (*cmd_handler_t)(const char *args, struct cmd_out *out);

/**
 * @brief Entry of the command table.
 *
 * A table is an array of entries ended by a sentinel entry with
 * @c name == @c NULL.
 */
struct cmd_entry {
	const char *name;         /**< Command name, compared case-insensitively. */
	const char *description;  /**< Short description, used by the HELP command. */
	cmd_handler_t handler;    /**< Function executed when the command is invoked. */
};

/**
 * @brief Get the description of a command.
 *
 * @param[in]  table       Command table, terminated by an entry with @c name == @c NULL.
 * @param[in]  name        Command name (case-insensitive).
 * @param[out] description Receives a pointer to the command description.
 *                         Left untouched on failure.
 *
 * @retval 0        Command found; @p description was set.
 * @retval -EINVAL  @p table, @p name or @p description is @c NULL.
 * @retval -ENOENT  No command with that name in @p table.
 */
int get_description(const struct cmd_entry *table, const char *name, const char **description);

/**
 * @brief Append formatted text to a command response.
 *
 * Works like snprintf() on the free space of @p out. Text that does not fit
 * is truncated (keeping @c buf null-terminated), @c out->overflow is set and
 * the call still returns 0. Once @c out->overflow is set, further calls
 * write nothing. @c out->len always matches what is stored in @c out->buf.
 *
 * Never pass user text as @p fmt; use @c "%s" and pass it as an argument.
 *
 * @param[in,out] out Response being built, already initialized.
 * @param[in]     fmt printf-style format string.
 * @param[in]     ... Format arguments.
 *
 * @retval 0        Text written, possibly truncated (check @c out->overflow).
 * @retval -EINVAL  @p out or @p fmt is @c NULL, or @c out->len is not smaller than
 *                  the buffer size (@p out was not initialized).
 * @retval -EIO     Formatting error; @c out is left as it was before the call.
 */
int cmd_out_printf(struct cmd_out *out, const char *fmt, ...) __printf_like(2, 3);

/**
 * @brief Find a command by name and run its handler.
 *
 * Only dispatch errors are returned here. The handler's own result is stored
 * in @c out->ret, which is left untouched when the handler is not called.
 * @p out is not initialized by this function: the caller must zero it.
 *
 * @param[in]     table Command table, terminated by an entry with @c name == @c NULL.
 * @param[in]     name  Command name (case-insensitive).
 * @param[in]     args  Arguments passed unchanged to the handler; empty string
 *                      when there are none.
 * @param[in,out] out   Response, written by the handler.
 *
 * @retval 0        The handler was called; its result is in @c out->ret.
 * @retval -EINVAL  @p table, @p name, @p args or @p out is @c NULL.
 * @retval -ENOENT  No command with that name in @p table.
 * @retval -ENOSYS  The command was found but its handler is @c NULL.
 */
int execute(const struct cmd_entry *table, const char *name, const char *args, struct cmd_out *out);

/**
 * @brief Check whether the arguments of a command are exactly @p expected.
 *
 * Meant for handlers to validate their arguments. Whitespace at the start and
 * at the end of @p args is ignored, and the comparison is case-insensitive.
 * Nothing is copied or modified.
 *
 * Pass an empty @p expected to check that there are no arguments: it matches
 * when @p args is empty or has only whitespace.
 *
 * @param[in] args     Arguments as received by the handler.
 * @param[in] expected Text that @p args must be equal to, without surrounding whitespace.
 *
 * @retval true   @p args is equal to @p expected.
 * @retval false  It is not, or @p args or @p expected is @c NULL.
 */
bool args_equal(const char *args, const char *expected);

#endif
