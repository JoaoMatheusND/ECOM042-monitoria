#include "command.h"

#include <errno.h>
#include <stddef.h>

int command_execute(const command_t *cmd)
{
	if (cmd == NULL || cmd->execute == NULL) {
		return -EINVAL;
	}

	cmd->execute();
	return 0;
}
