#ifndef COMMAND_H_
#define COMMAND_H_

struct command;

typedef struct command {
	const char *name;
	void (*execute)(void);
	void *ctx;
} command_t;

int command_execute(const command_t *cmd);

#endif /* COMMAND_H_ */
