#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "cmd.h"
#include "dispatch.h"

int dispatch(const struct cmd_entry *table, const char *line, struct cmd_out *out) {
    char name[DISPATCH_NAME_MAX];
    const char *name_start;
    const char *args;
    size_t name_len;

    if(table == NULL || line == NULL || out == NULL) {
        return -EINVAL;
    }

    while(isspace((unsigned char)*line)) {
        line++;
    }

    if(*line == '\0') {
        return -EINVAL;
    }

    name_start = line;

    while(*line != '\0' && !isspace((unsigned char)*line)) {
        line++;
    }

    name_len = (size_t)(line - name_start);

    if(name_len >= sizeof(name)) {
        return -ENOENT;
    }

    memcpy(name, name_start, name_len);
    name[name_len] = '\0';

    /* Consume only the separator; the rest is passed exactly as typed */
    args = line;
    if(*args != '\0') {
        args++;
    }

    return execute(table, name, args, out);
}
