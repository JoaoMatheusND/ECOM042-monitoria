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

#include <stddef.h>

/**
 * @brief Response buffer lent by the caller to a command handler.
 *
 * The caller owns the memory. The handler writes only the response content
 * (no trailing "\r\n" and no prompt).
 */
struct cmd_out {
	char *buf;    /**< Caller-owned buffer where the response is written. */
	size_t size;  /**< Capacity of @c buf in bytes, including the terminator. */
	size_t len;   /**< Number of bytes written by the handler, excluding the terminator. */
};

/**
 * @brief Command handler function pointer.
 *
 * @param[in]  args Text following the command name, exactly as received
 *                  (never @c NULL; empty string when there are no arguments).
 * @param[out] out  Buffer where the handler writes its response.
 *
 * @retval 0       Success; the response is in @p out.
 * @retval -errno  Failure.
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
 * @brief Find a command by name and run its handler.
 *
 * @param[in]  table Command table, terminated by an entry with @c name == @c NULL.
 * @param[in]  name  Command name (case-insensitive).
 * @param[in]  args  Arguments passed unchanged to the handler.
 * @param[out] out   Buffer where the handler writes its response.
 *
 * @retval -EINVAL  @p table, @p name or @p out is @c NULL.
 * @retval -ENOENT  No command with that name in @p table.
 * @retval -ENOSYS  The command was found but its handler is @c NULL.
 * @return Otherwise, the value returned by the handler (0 on success, or a
 *         negative errno). Note that a handler returning @c -ENOENT or
 *         @c -ENOSYS is indistinguishable from the cases above.
 */
int execute(const struct cmd_entry *table, const char *name, const char *args, struct cmd_out *out);

#endif
