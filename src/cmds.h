/**
 * @file cmds.h
 * @brief Static command table and helpers to inspect it.
 */

#ifndef CMDS_H_
#define CMDS_H_

#include "cmd.h"

#define TABLE_TERMINATOR "0"
#define TERMINATOR_SIZE  1

/**
 * @brief Get the command table.
 *
 * The table is terminated by an entry whose name is @ref TABLE_TERMINATOR.
 *
 * @return Pointer to the first entry of the command table.
 */
const cmd_command *cmd_get_command_table();

/**
 * @brief Print all registered commands and their descriptions.
 *
 * @retval 0       All commands listed.
 * @retval -ENOENT Command table is unavailable.
 */
int cmd_list_commands();

#endif