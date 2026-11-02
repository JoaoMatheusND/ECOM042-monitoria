/**
 * @file dispatch.h
 * @brief Splits a command line into name and arguments and runs the command.
 *
 * This module knows nothing about UART, GPIO or printk. It only parses a
 * line that is already complete and hands it to execute().
 */

#ifndef DISPATCH_H_
#define DISPATCH_H_

#include "cmd.h"

/**
 * @brief Size of the buffer that holds a command name, terminator included.
 *
 * Command names can have at most DISPATCH_NAME_MAX - 1 characters; a longer
 * name is never matched against the table.
 */
#define DISPATCH_NAME_MAX 16

/**
 * @brief Parse a command line and run the command it names.
 *
 * Line format: [whitespace] NAME [SEPARATOR ARGS]
 *
 * - Leading whitespace (isspace()) is ignored.
 * - The name ends at the first whitespace character or at the end of the line.
 * - The arguments are everything after that one separator character, exactly
 *   as typed (so "ECHO  a" passes " a"). When the line ends right after the
 *   name, the arguments are an empty string.
 *
 * The line is never modified, but it must stay valid and unchanged until this
 * function returns, because the handler reads its arguments directly from it.
 *
 * @p out must be initialized (zeroed) by the caller, for every line. It is not
 * modified when this function returns a non-zero value.
 *
 * @param[in]     table Command table, terminated by an entry with @c name == @c NULL.
 * @param[in]     line  Null-terminated command line, without "\r\n".
 * @param[in,out] out   Response, written by the handler.
 *
 * @retval 0        The handler was called; its result is in @c out->ret.
 * @retval -EINVAL  @p table, @p line or @p out is @c NULL, or the line is empty
 *                  or has only whitespace.
 * @retval -ENOENT  The name has DISPATCH_NAME_MAX characters or more, or no
 *                  command with that name exists in @p table.
 * @retval -ENOSYS  The command was found but its handler is @c NULL.
 */
int dispatch(const struct cmd_entry *table, const char *line, struct cmd_out *out);

#endif
