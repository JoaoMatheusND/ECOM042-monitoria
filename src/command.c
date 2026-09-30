#include "command.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/printk.h>

int command_dispatch(const command_t *table, size_t count, const char *name)
{
	if ((table == NULL) || (name == NULL)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		if ((table[i].name != NULL) && (strcmp(table[i].name, name) == 0)) {
			if (table[i].execute == NULL) {
				return -EINVAL;
			}
			table[i].execute(table[i].ctx);
			return 0;
		}
	}

	printk("ERRO: comando '%s' nao encontrado\n", name);
	return -ENOENT;
}
