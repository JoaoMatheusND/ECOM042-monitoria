/* Parsing + despacho de comandos: puro, sem UART/GPIO/printk.
 * O hardware entra por function pointers (struct minish_hw_ops). */
#ifndef MINISH_DISPATCH_H_
#define MINISH_DISPATCH_H_

#include <stdbool.h>
#include <stddef.h>

#ifndef CONFIG_MINISH_RESP_MAX
#define CONFIG_MINISH_RESP_MAX 256
#endif
#define MINISH_RESP_MAX CONFIG_MINISH_RESP_MAX

struct minish_hw_ops {
	int (*led_set)(bool on);
	int (*led_get)(bool *on);
	int (*btn_get)(bool *pressed);
};

void minish_set_hw_ops(const struct minish_hw_ops *ops);

/* Recebe a linha (sem terminador) e escreve a resposta em `out`, SEM "\r\n"
 * final (respostas multilinha usam "\r\n" internamente). `out` fica vazio
 * para linha em branco. Retorna 0 em sucesso, -errno em erro (a mensagem
 * "ERR: ..." tambem vai em `out`). */
int minish_dispatch(const char *line, char *out, size_t out_size);

#endif
