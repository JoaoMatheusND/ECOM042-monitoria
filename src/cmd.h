/*******************************************************************
 * @file cmd.h
 *
 * @brief Parsing e despacho dos comandos da mini-shell.
 *
 * Módulo puro: não chama UART, GPIO nem printk. O acesso ao hardware
 * chega por ponteiros de função (struct cmd_io), injetados no init.
 *******************************************************************/

#ifndef CMD_H_
#define CMD_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Operações de hardware que os comandos precisam. */
struct cmd_io {
	int (*led_set)(bool on);
	int (*led_get)(void);     /* 1 ligado, 0 desligado, ou errno negativo */
	int (*button_read)(void); /* 1 pressionado, 0 solto, ou errno negativo */
	int64_t (*uptime_ms)(void);
};

/* Registra as operações de hardware usadas pelos comandos. */
void cmd_init(const struct cmd_io *io);

/* Interpreta uma linha (sem "\r\n") e escreve a resposta em resp.
 * Respostas com várias linhas são separadas por "\r\n".
 * Retorna o tamanho da resposta (0 = linha vazia, sem resposta). */
int cmd_execute(const char *line, char *resp, size_t resp_size);

#endif /* CMD_H_ */
