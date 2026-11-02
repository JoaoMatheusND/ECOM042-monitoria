/**
 * @file transport.h
 * @brief UART transport of the shell: receives bytes, edits the line, runs commands.
 *
 * Received bytes are only queued by the UART interrupt callback. A dedicated
 * thread takes them from the queue, assembles the line (line_feed()), echoes
 * what the user types, runs the command (dispatch()) and sends the response.
 *
 * Every response is followed by "\r\n" and by the prompt "> ". The transmission
 * is done by polling, from that thread.
 */

#ifndef TRANSPORT_H_
#define TRANSPORT_H_

#include "cmd.h"
#include <zephyr/device.h>

/**
 * @brief Start the shell on a UART.
 *
 * Creates the thread that serves the shell, enables the receive interrupt
 * of @p uart and sends the first prompt. It must be called once.
 *
 * Responses sent to the user, according to what the command did:
 * - the text written by the handler, when it succeeds;
 * - "ERR: line too long", when the line exceeded UART_LINE_BUF_SIZE;
 * - "ERR: unknown command", when the command does not exist or the line
 *   has only whitespace;
 * - "ERR: response too long", when the handler's response was truncated;
 * - "ERR: command failed", when the handler failed without writing a response;
 * - "ERR: internal error", when the command exists but has no handler.
 *
 * @param[in] uart  UART device used for both directions.
 * @param[in] table Command table, terminated by an entry with @c name == @c NULL.
 *
 * @retval 0        The shell is running.
 * @retval -EINVAL  @p uart or @p table is @c NULL.
 * @retval -ENODEV  @p uart is not ready.
 */
int transport_init(const struct device *uart, const struct cmd_entry *table);

#endif