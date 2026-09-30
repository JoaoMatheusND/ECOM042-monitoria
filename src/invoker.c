#include "invoker.h"

void invoker_dispatch(struct command *cmd)
{
	if (cmd && cmd->execute) {
		cmd->execute(cmd);
	}
}
