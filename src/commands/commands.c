#include <errno.h>
#include <stdio.h>
#include "cmd.h"
#include "commands.h"

/* The static command table and command handlers goes here */
/* The table is terminated by a command name == NULL */

const struct cmd_entry commands_table[] = { {NULL, NULL, NULL} }; 