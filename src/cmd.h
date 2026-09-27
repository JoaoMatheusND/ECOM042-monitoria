#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

struct cmd_arg {
	const char *name;
	void *data;
};

typedef int (*cmd_handler_t)(struct cmd_arg *args, size_t argc);

struct cmd_command {
	sys_snode_t node;
	const char *name;
	const char *description;
	cmd_handler_t handler;
};

/* Registers one command */
void cmd_register(struct cmd_command *command);

/* Prints command description */
int cmd_help(const char *cmd_name);

/* Executes command */
int cmd_execute(const char *cmd_name, struct cmd_arg *args, size_t argc);

/* Helper macro to register a command. Commands must be registered in execution context. */
#define CMD_REGISTER(_name, _handler, _description)                                                \
	do {                                                                                       \
		static struct cmd_command _name = {                                                \
			.name = (#_name),                                                          \
			.handler = (_handler),                                                     \
			.description = (_description),                                             \
		};                                                                                 \
		cmd_register(&_name);                                                              \
	} while (0)

#endif