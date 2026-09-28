#include "command.h"

#include <stdlib.h>
#include <string.h>

typedef struct command {
	void *user_data;
	void (*execute)(void *user_data);
} Command;

// Obs: em cenários que precisem de muita performance o ideal seria alocar na stack
// passando um ponteiro para a struct definida no escopo como parâmetro da função
Command *command_init(void (*execute)(void *), void *user_data, size_t size)
{
	Command *new_command = (Command *)malloc(sizeof(*new_command));

	new_command->execute = execute;

	if (user_data == NULL) {

		new_command->user_data = NULL;
		return new_command;
	}

	new_command->user_data = malloc(size);

	// Garantir que a struct tenha ownership do user_data caso seja alocado na stack
	memcpy(new_command->user_data, user_data, size);

	return new_command;
}

void command_destroy(Command *self)
{
	if (self->user_data == NULL) {
		goto null_data;
	}

	free(self->user_data);

null_data:

	free(self);

	return;
}

void command_run(Command *self)
{
	self->execute(self->user_data);
}
