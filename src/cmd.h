/**
 * @file cmd.h
 * @brief Command pattern interface: lookup, help and execution of commands.
 *
 * Each command is described by a @ref cmd_command and registered in a static
 * table (see cmds.h). Callers execute commands by name, without knowing the
 * handler that implements them.
 */

#ifndef CMD_H_
#define CMD_H_

/**
 * @brief Describes a single command.
 */
typedef struct {
	const char *name;
	const char *description;
	int (*handler)(void **args, int argc);
} cmd_command;

/**
 * @brief Print the description of a command.
 *
 * Output format: `[name]: description`.
 *
 * @param name Name of the command.
 *
 * @retval 0       Description printed.
 * @retval -EINVAL Command not found.
 * @retval -ENOENT Command table is unavailable.
 */
int cmd_help(const char *name);

/**
 * @brief Execute a command by name.
 *
 * @param name Name of the command.
 * @param args Array of pointers to the command arguments (may be NULL).
 * @param argc Number of elements in @p args.
 *
 * @return The value returned by the command handler, or a negative errno
 *         (-EINVAL if not found, -ENOENT if the table is unavailable).
 */
int cmd_execute(const char *name, void **args, int argc);

#endif