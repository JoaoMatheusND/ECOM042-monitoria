#include "command.h"

#include <errno.h>
#include <stddef.h>

int command_execute(const struct command *cmd)
{
	cmd->execute();
	return 0;
}
