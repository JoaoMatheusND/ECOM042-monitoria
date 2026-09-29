#ifndef CMD_H_
#define CMD_H_

struct cmd_command {
	const char *name;
	const char *description;
	int (*handler)(void **args, int argc);
};

/* Prints command description */
int cmd_help(const char *name);

/* Executes command */
int cmd_execute(const char *name, void **args, int argc);

#endif