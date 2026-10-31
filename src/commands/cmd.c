#include <strings.h>
#include <errno.h>
#include "cmd.h"

/**
 * @brief Look up a command entry by name.
 *
 * @param[in]  table Command table, terminated by an entry with @c name == @c NULL.
 * @param[in]  name  Command name (case-insensitive).
 * @param[out] out   Receives a pointer to the matching entry. Left untouched on failure.
 *
 * @retval 0        Command found.
 * @retval -EINVAL  @p table or @p name is @c NULL.
 * @retval -ENOENT  No command with that name in @p table.
 */
static int get_command(const struct cmd_entry *table, const char* name, const struct cmd_entry **out) {
    
    if(table == NULL || name == NULL) {
        return -EINVAL;
    }

    while(table->name != NULL) {
        if(strcasecmp(name, table->name) == 0) {
            *out = table; 
            return 0;
        }
        table++; 
    }

    return -ENOENT; 

}

int get_description(const struct cmd_entry *table, const char* name, const char** description) {
    const struct cmd_entry *command; 
    int ret;

    if(description == NULL) {
        return -EINVAL;
    }

    ret = get_command(table, name, &command);

    if(ret < 0) {
        return ret;
    }

    *description = command->description; 

    return 0; 
}

int execute(const struct cmd_entry *table, const char* name, const char* args, struct cmd_out *out) {
    const struct cmd_entry *command; 
    int ret;

    if(out == NULL) {
        return -EINVAL; 
    }

    ret = get_command(table, name, &command); 

    if(ret < 0) {
        return ret; 
    }

    if(command->handler == NULL) {
        return -ENOSYS;
    }

    return command->handler(args, out);
}