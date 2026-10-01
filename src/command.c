#include "command.h"

#include <errno.h>
#include <stddef.h>

int command_execute(const command_t *cmd)
{
	cmd->execute();
	return 0;
}
