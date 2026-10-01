#ifndef COMMAND_H_
#define COMMAND_H_

struct command {
	const char *name;
	void (*execute)(void);
	void *ctx;
};

int command_execute(const struct command *cmd);

#endif /* COMMAND_H_ */
