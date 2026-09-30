#ifndef COMMAND_H
#define COMMAND_H

struct command {
	void (*execute)(struct command *cmd);
};

#endif
