#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "cmd.h"

/**
 * @brief Get the production command table.
 *
 * The table is terminated by an entry with @c name == @c NULL. Pass it as the
 * @c table argument of the functions in cmd.h.
 *
 * @return Pointer to the first entry of the table (never @c NULL).
 */
const struct cmd_entry *commands_get_table(void);

#endif