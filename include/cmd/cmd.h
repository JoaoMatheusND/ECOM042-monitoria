#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>


struct cmd_param {
	const char* name;
	bool optional;
	void* default_data;
    void* data; 
};

struct cmd_arg {
    const char* name; 
    void* data; 
};

typedef int (*cmd_handler_t)(struct cmd_arg *args, size_t argc); 

struct cmd_command {
    const char* name; 
    cmd_handler_t handler; 
    const struct cmd_param* params; 
    size_t param_count; 
}; 

/* Registers one command */
void cmd_register(struct cmd_command *command);

/*  Describes a parameter. Expands to an initializer, not an object. */
#define CMD_PARAM(_name, _optional, _default, _data)    \
{                                                       \
    .name = (#_name),                                   \
    .optional = (_optional),                            \
    .default_data = (&_default),                        \
    .data = (&_data),                                   \
}                                                       \
    
/* Helper macro to register a command. Commands must be registered in execution context. */
#define CMD_REGISTER(_name, _handler, ...)                              \
    do                                                                  \
    {                                                                   \
        static const struct cmd_param _name##_params[] = {__VA_ARGS__}; \
        static struct cmd_command _name##_cmd = {                       \
            .name = #_name,                                             \
            .handler = (_handler),                                      \
            .params = (_name##_params),                                 \
            .param_count = ARRAY_SIZE(_name##_params)                   \
        };                                                              \
        cmd_register(&_name##_cmd);                                     \
    } while(0)                                                          

#endif