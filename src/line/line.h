/**
 * @file line.h
 * @brief Command line assembly: turns a stream of bytes into a line of text.
 *
 * This module knows nothing about UART, threads or the command table. It only
 * keeps the state of the line being typed and updates it one byte at a time.
 * What to do with that state (echo, dispatch, prompt) is up to the caller.
 *
 * The state is not protected against concurrent access.
 * 
 */

#ifndef LINE_H_
#define LINE_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Maximum number of characters in a line.
 *
 * One byte of the buffer is reserved for the null terminator.
 */
#define LINE_MAX_CHARS (CONFIG_UART_LINE_BUF_SIZE - 1)

/**
 * @brief State of the line being typed.
 *
 * Must be zero-initialized (@c = {0}) before the first line_feed(). Only
 * line_feed() and line_reset() should modify it.
 */
struct line {
	char buf[CONFIG_UART_LINE_BUF_SIZE]; /**< Line text; null-terminated once the line ends without overflow. */
	size_t len;                          /**< Number of characters in @c buf, excluding the terminator. */
	bool line_terminated;                /**< true once Enter was received. */
	bool overflow;                       /**< true if the line exceeded LINE_MAX_CHARS */
};

/**
 * @brief Feed one received byte into the line.
 *
 * Effect of each byte:
 * - Printable (0x20 to 0x7E): appended to the line. If the line already has
 *   LINE_MAX_CHARS characters, @c overflow is set instead, and the rest of the
 *   line is discarded.
 * - Backspace (0x08 or 0x7F): removes the last character. Does nothing on an
 *   empty line or after an overflow.
 * - Carriage return or line feed: ends the line, setting @c line_terminated
 *   (and null-terminating @c buf unless there was an overflow). On an empty
 *   line it is ignored.
 * - Any other byte (control characters, tab, 0x80 and above): ignored.
 *
 * Once @c line_terminated is set, every byte is ignored until line_reset().
 *
 *
 * @param[in,out] line Line being assembled.
 * @param[in]     c    Received byte.
 *
 * @retval 0        The byte was processed.
 * @retval -EINVAL  @p line is @c NULL.
 */
int line_feed(struct line *line, char c);

/**
 * @brief Prepare the line to receive a new one.
 *
 * Clears @c len, @c line_terminated and @c overflow. Must be called after a
 * terminated line has been handled.
 *
 * @param[in,out] line Line to reset. Must not be @c NULL.
 */
void line_reset(struct line *line);

#endif