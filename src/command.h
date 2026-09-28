#ifndef COMMAND_H_
#define COMMAND_H_

struct command;

struct command {
	const char *name;
	int (*execute)(const struct command *self);
	void *ctx;
};

typedef struct command command_t;

int command_execute(const command_t *cmd);

#endif /* COMMAND_H_ */
