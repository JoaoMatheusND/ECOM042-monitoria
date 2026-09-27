#include "cmd.h"
#include <zephyr/sys/slist.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>

static sys_slist_t command_list = SYS_SLIST_STATIC_INIT(&command_list);

void cmd_register(struct cmd_command* cmd) {
    	sys_slist_append(&command_list, &(cmd->node)); 
}

static int find_command(const char* name, struct cmd_command* out) {
    size_t name_len = strlen(name); 
    struct cmd_command* command; 

    SYS_SLIST_FOR_EACH_CONTAINER(&command_list, command, node) {
        if (name_len == strlen(command->name) && strncmp(name, command->name, name_len) == 0) {
                *out = *command;
                return 0; 
        }
    }
    return -ENOENT; 
}

int cmd_help(const char* cmd_name) {
    struct cmd_command cmd; 
    int ret; 

    ret = find_command(cmd_name, &cmd); 

    if (ret != 0) {
        return ret; 
    }

    printk("[%s]: %s\r\n", cmd.name, cmd.description); 

    return 0; 
}

int cmd_execute(const char* cmd_name, struct cmd_arg* args, size_t argc) {
    struct cmd_command cmd; 
    int ret; 

    ret = find_command(cmd_name, &cmd); 
    if (ret != 0) {
        return ret; 
    }

    return cmd.handler(args, argc); 
}