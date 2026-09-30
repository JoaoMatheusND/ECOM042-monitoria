#pragma once

/**
 * @brief A command is something that can execute itself.
 *
 * Each entry in a command table holds the name used to look the command
 * up, and a function pointer to the action that command performs.
 */
struct command {
	const char *name;
	int (*exec)(void);
};

/**
 * @brief Look up @p name in @p table and run the matching command.
 *
 * @param table Pointer to the first entry of a NULL-terminated command table.
 * @param name  Command name to search for.
 *
 * @return The value returned by the executed command, or -1 if no command
 *         with @p name is found.
 */
int command_run(const struct command *table, const char *name);
