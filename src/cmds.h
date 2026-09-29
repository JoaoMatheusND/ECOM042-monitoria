#ifndef CMDS_H_
#define CMDS_H_

#include "cmd.h"

#define TABLE_TERMINATOR "0"
#define TERMINATOR_SIZE  1

const struct cmd_command *cmd_get_command_table();

int cmd_list_commands();

#endif