#ifndef COMMAND_H_
#define COMMAND_H_

typedef struct command {
	const char *name;
	void (*execute)(void);
	void *ctx;
} command_t;

int command_execute(const struct command *cmd);

#endif /* COMMAND_H_ */
