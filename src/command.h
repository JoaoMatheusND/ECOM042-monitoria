#ifndef COMMAND_H_
#define COMMAND_H_

struct command;

typedef int (*command_execute_fn)(const struct command *self);

struct command {
	const char *name;
	command_execute_fn execute;
	void *ctx;
};

typedef struct command command_t;

int command_execute(const command_t *cmd);

#endif /* COMMAND_H_ */
