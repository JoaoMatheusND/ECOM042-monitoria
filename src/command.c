#include "command.h"

#include <stdlib.h>
#include <string.h>

typedef struct command {
	const char *name;
	void *user_data;
	void (*execute)(void *user_data);
} Command;

Command *command_init(const char *name, void (*execute)(void *), void *user_data)
{

	if (!name) {
		return NULL;
	}
	Command *new_command = (Command *)malloc(sizeof(*new_command));

	new_command->execute = execute;
	new_command->name = name;

	if (user_data == NULL) {

		new_command->user_data = NULL;
		return new_command;
	}

	new_command->user_data = user_data;

	return new_command;
}

static void command_run(Command *self)
{
	self->execute(self->user_data);
}

void command_dispatch(Command *commands[], size_t size, const char *name)
{
	for (size_t i = 0; i < size; i++) {
		if (strcmp(commands[i]->name, "name")) {
			command_run(commands[i]);
		}
	}
}
