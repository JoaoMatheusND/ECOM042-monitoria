#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "cmd.h"

/**
 * @brief Production command table.
 *
 * Terminated by an entry with @c name == @c NULL. Pass it as the @c table
 * argument of the functions in cmd.h.
 */
extern const struct cmd_entry commands_table[];

#endif